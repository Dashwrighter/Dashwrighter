#pragma once

// Shared helpers for EditorWindow.cpp and EditorWindow_Props.cpp.
// Not part of the public EditorWindow API.

#include <windows.h>
#include <string>
#include <vector>
#include <gdiplus.h>

namespace Ed {

constexpr int kCorner = 8;
constexpr int kWellInset = 3;
constexpr COLORREF kFieldFill = RGB(10, 10, 10);

inline std::string WideToUtf8(const wchar_t* wide) {
    if (!wide || !*wide) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, wide, -1, nullptr, 0, nullptr, nullptr);
    if (n <= 0) return {};
    std::string out(static_cast<size_t>(n), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide, -1, &out[0], n, nullptr, nullptr);
    if (!out.empty() && out.back() == '\0') out.pop_back();
    return out;
}

inline std::wstring Utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    if (n <= 0) return {};
    std::wstring out(static_cast<size_t>(n), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &out[0], n);
    if (!out.empty() && out.back() == L'\0') out.pop_back();
    return out;
}

inline bool GetEncoderClsid(const WCHAR* mime, CLSID* clsid) {
    if (!mime || !clsid) return false;
    UINT num = 0, size = 0;
    Gdiplus::GetImageEncodersSize(&num, &size);
    if (size == 0) return false;
    std::vector<BYTE> buf(size);
    auto* info = reinterpret_cast<Gdiplus::ImageCodecInfo*>(buf.data());
    if (Gdiplus::GetImageEncoders(num, size, info) != Gdiplus::Ok) return false;
    for (UINT i = 0; i < num; ++i) {
        if (info[i].MimeType && wcscmp(info[i].MimeType, mime) == 0) {
            *clsid = info[i].Clsid;
            return true;
        }
    }
    return false;
}

inline std::string FileBaseName(const std::string& path) {
    size_t slash = path.find_last_of("\\/");
    return (slash != std::string::npos) ? path.substr(slash + 1) : path;
}

inline std::string GetTextA(HWND h) {
    if (!h) return {};
    char buf[256] = {};
    GetWindowTextA(h, buf, sizeof(buf));
    return std::string(buf);
}

inline void SetTextA(HWND h, const std::string& s) {
    if (h) SetWindowTextA(h, s.c_str());
}

inline void SetTextW(HWND h, const wchar_t* s) {
    if (h) SetWindowTextW(h, s);
}

inline void MapHwndRectToParent(HWND h, RECT& rc) {
    GetWindowRect(h, &rc);
    HWND parent = GetParent(h);
    if (parent)
        MapWindowPoints(nullptr, parent, reinterpret_cast<LPPOINT>(&rc), 2);
}

inline void EraseParentRect(HWND child, const RECT& rc) {
    HWND parent = child ? GetParent(child) : nullptr;
    if (!parent) return;
    HDC hdc = GetDC(parent);
    HBRUSH br = reinterpret_cast<HBRUSH>(GetClassLongPtr(parent, GCLP_HBRBACKGROUND));
    if (!br) br = reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    FillRect(hdc, &rc, br);
    ReleaseDC(parent, hdc);
    InvalidateRect(parent, &rc, FALSE);
}

inline void FitLabelToText(HWND h) {
    if (!h) return;
    wchar_t text[128] = {};
    GetWindowTextW(h, text, 128);
    if (!text[0]) return;
    HDC hdc = GetDC(h);
    HFONT font = reinterpret_cast<HFONT>(SendMessageW(h, WM_GETFONT, 0, 0));
    HGDIOBJ old = font ? SelectObject(hdc, font) : nullptr;
    SIZE sz = {};
    GetTextExtentPoint32W(hdc, text, lstrlenW(text), &sz);
    if (old) SelectObject(hdc, old);
    ReleaseDC(h, hdc);
    RECT rc = {};
    MapHwndRectToParent(h, rc);
    const int newW = (sz.cx + 6 > 8) ? static_cast<int>(sz.cx + 6) : 8;
    const UINT_PTR align = reinterpret_cast<UINT_PTR>(GetPropW(h, L"HDE_R"));
    const bool right = (align == 2) ||
        (align == 0 && (GetWindowLong(h, GWL_STYLE) & SS_TYPEMASK) == SS_RIGHT);
    const LONG_PTR slot = GetWindowLongPtr(h, GWLP_USERDATA);
    const int slotX = LOWORD(slot);
    const int slotW = HIWORD(slot);
    const int slotRight = (slotW > 0) ? (slotX + slotW) : rc.right;
    const int newX = right ? (slotRight - newW) : ((slotW > 0) ? slotX : rc.left);
    if (newX == rc.left && newW == (rc.right - rc.left))
        return;
    SetWindowPos(h, nullptr, newX, rc.top, newW, rc.bottom - rc.top,
        SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW);
}

inline void SetLabelText(HWND h, const wchar_t* s) {
    if (!h || !s) return;
    wchar_t cur[128] = {};
    GetWindowTextW(h, cur, 128);
    RECT old = {};
    MapHwndRectToParent(h, old);
    SendMessageW(h, WM_SETREDRAW, FALSE, 0);
    if (wcscmp(cur, s) != 0)
        SetWindowTextW(h, s);
    FitLabelToText(h);
    SendMessageW(h, WM_SETREDRAW, TRUE, 0);
    RECT neu = {};
    MapHwndRectToParent(h, neu);
    RECT uni = {};
    UnionRect(&uni, &old, &neu);
    EraseParentRect(h, uni);
    InvalidateRect(h, nullptr, TRUE);
    RedrawWindow(h, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
}

inline void ShowSizeOrWidthCaption(HWND hSize, HWND hWidth, bool size) {
    HWND hide = size ? hWidth : hSize;
    HWND show = size ? hSize : hWidth;
    if (hide && IsWindowVisible(hide)) {
        RECT rc = {};
        MapHwndRectToParent(hide, rc);
        ShowWindow(hide, SW_HIDE);
        EraseParentRect(hide, rc);
    }
    if (show) {
        ShowWindow(show, SW_SHOW);
        InvalidateRect(show, nullptr, TRUE);
        UpdateWindow(show);
    }
}

inline bool GetChecked(HWND h) {
    return h && (SendMessage(h, BM_GETCHECK, 0, 0) == BST_CHECKED);
}

inline void SetChecked(HWND h, bool on) {
    if (h) SendMessage(h, BM_SETCHECK, on ? BST_CHECKED : BST_UNCHECKED, 0);
}

inline void ShowCtrl(HWND h, bool show) {
    if (h) ShowWindow(h, show ? SW_SHOW : SW_HIDE);
}

inline void EnableCtrl(HWND h, bool on) {
    if (h) EnableWindow(h, on ? TRUE : FALSE);
}

inline void ClampInt(int& v, int lo, int hi) {
    if (v < lo) v = lo;
    if (v > hi) v = hi;
}

inline void ClampDouble(double& v, double lo, double hi) {
    if (v < lo) v = lo;
    if (v > hi) v = hi;
}

void StyleRoundedEdit(HWND h);

inline void InvalidateEditWell(HWND h) {
    HWND parent = GetParent(h);
    if (!parent) return;
    RECT rc = {};
    GetWindowRect(h, &rc);
    MapWindowPoints(nullptr, parent, reinterpret_cast<LPPOINT>(&rc), 2);
    InflateRect(&rc, kWellInset + 1, kWellInset + 1);
    InvalidateRect(parent, &rc, FALSE);
}

struct TypeEntry { int id; const wchar_t* name; };
inline const TypeEntry kGaugeTypes[] = {
    { 0, L"0 - Analog" },
    { 1, L"1 - Bargraph" },
    { 2, L"2 - Digital" },
    { 4, L"4 - Thin Analog" },
    { 5, L"5 - Status LED" },
    { 6, L"6 - RPM Bar" },
    { 8, L"8 - Symbol" },
    { 9, L"9 - Graph" },
    { 11, L"11 - Bargraph Style 2" },
};

struct StyleEntry { int id; const wchar_t* name; };
inline const StyleEntry kSwitchStyles[] = {
    { 0, L"0 - Rocker" },
    { 1, L"1 - Slider" },
    { 2, L"2 - Rocker 2" },
    { 3, L"3 - Black Toggle" },
    { 4, L"4 - Black Toggle (small)" },
    { 5, L"5 - Silver Toggle" },
    { 6, L"6 - Silver Toggle (small)" },
    { 7, L"7 - Silver Momentary Switch" },
    { 8, L"8 - Silver Momentary Switch (small)" },
    { 9, L"9 - Gray Momentary Button (small)" },
    { 10, L"10 - Gray Momentary Button" },
    { 11, L"11 - Gray Momentary Button (large)" },
    { 12, L"12 - Black Momentary Button (small)" },
    { 13, L"13 - Black Momentary Button" },
    { 14, L"14 - Black Momentary Button (large)" },
    { 15, L"15 - Black Momentary Switch" },
    { 16, L"16 - Black Momentary Switch (small)" },
};

} // namespace Ed
