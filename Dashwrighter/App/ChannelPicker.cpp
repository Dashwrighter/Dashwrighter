// =============================================================================
// ChannelPicker.cpp — Channel list. Find matches name or decimal id.
//
// Gauge-only. Writes channel= id; does not rename or apply gauge defaults.
// Esc / Cancel leaves the integer. OK disabled until a listed channel is selected.
// =============================================================================

#include "ChannelPicker.h"
#include "../Io/ChannelCatalog.h"
#include <windows.h>
#include <uxtheme.h>
#include <string>

#pragma comment(lib, "uxtheme.lib")

namespace {

constexpr int kPad = 12;
constexpr int kLabH = 16;
constexpr int kFilterH = 22;
// Former category column (180) plus the gap (10) plus the channel column (300).
constexpr int kChW = 490;
constexpr int kListH = 320;
constexpr int kBtnW = 88;
constexpr int kBtnH = 26;
constexpr int kStatusH = 18;

enum CtrlId {
    IdFilter = 100,
    IdCh = 102,
    IdStatus = 103,
    IdOk = IDOK,
    IdCancel = IDCANCEL
};

struct PickerState {
    HWND hwnd = nullptr;
    HWND hFilter = nullptr;
    HWND hCh = nullptr;
    HWND hOk = nullptr;
    HWND hCancel = nullptr;
    HWND hStatus = nullptr;
    HBRUSH hBg = nullptr;
    HBRUSH hField = nullptr;
    HBRUSH hBtn = nullptr;
    HBRUSH hRed = nullptr;
    HFONT hFont = nullptr;
    int currentId = 0;
    int pickedId = -1;
    bool accepted = false;
};

std::wstring LowerAscii(const std::wstring& s) {
    std::wstring o = s;
    for (wchar_t& c : o) {
        if (c >= L'A' && c <= L'Z')
            c = static_cast<wchar_t>(c - L'A' + L'a');
    }
    return o;
}

bool MatchesFilter(const ChannelEntry& e, const std::wstring& filt) {
    if (filt.empty())
        return true;
    if (LowerAscii(e.name).find(filt) != std::wstring::npos)
        return true;
    if (std::to_wstring(e.id).find(filt) != std::wstring::npos)
        return true;
    return false;
}

std::wstring FilterText(HWND h) {
    if (!h) return {};
    wchar_t buf[128] = {};
    GetWindowTextW(h, buf, 128);
    return LowerAscii(buf);
}

void SetStatus(PickerState& st, const wchar_t* text) {
    if (st.hStatus)
        SetWindowTextW(st.hStatus, text);
}

void SyncOk(PickerState& st) {
    EnableWindow(st.hOk, st.pickedId >= 0 ? TRUE : FALSE);
}

void FillChannels(PickerState& st) {
    if (!st.hCh) return;
    const ChannelCatalog& cat = ChannelCatalog::Get();
    const std::wstring filt = FilterText(st.hFilter);

    SendMessageW(st.hCh, WM_SETREDRAW, FALSE, 0);
    SendMessageW(st.hCh, LB_RESETCONTENT, 0, 0);

    int matchSel = -1;
    int shown = 0;
    for (const auto& e : cat.Entries()) {
        if (!MatchesFilter(e, filt))
            continue;
        int idx = static_cast<int>(SendMessageW(st.hCh, LB_ADDSTRING, 0,
            reinterpret_cast<LPARAM>(e.name.c_str())));
        SendMessageW(st.hCh, LB_SETITEMDATA, idx, e.id);
        if (st.pickedId == e.id || (st.pickedId < 0 && st.currentId == e.id))
            matchSel = idx;
        ++shown;
    }
    SendMessageW(st.hCh, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(st.hCh, nullptr, TRUE);

    if (matchSel >= 0) {
        SendMessageW(st.hCh, LB_SETCURSEL, matchSel, 0);
        LRESULT id = SendMessageW(st.hCh, LB_GETITEMDATA, matchSel, 0);
        st.pickedId = static_cast<int>(id);
    } else {
        st.pickedId = -1;
    }
    SyncOk(st);

    wchar_t status[160] = {};
    if (!cat.loaded()) {
        swprintf_s(status, L"Could not load Holley-Channel-Picker.csv");
    } else if (cat.FindById(st.currentId) == nullptr) {
        swprintf_s(status, L"%d shown  ·  current %d (not in list)", shown, st.currentId);
    } else {
        swprintf_s(status, L"%d shown", shown);
    }
    SetStatus(st, status);
}

void Accept(PickerState& st) {
    if (st.pickedId < 0) return;
    st.accepted = true;
    DestroyWindow(st.hwnd);
}

void DrawOwnerButton(const PickerState& st, const DRAWITEMSTRUCT* di) {
    const bool pressed = (di->itemState & ODS_SELECTED) != 0;
    const bool disabled = (di->itemState & ODS_DISABLED) != 0;
    const bool primary = (di->CtlID == IdOk);
    RECT rc = di->rcItem;
    FillRect(di->hDC, &rc, st.hBg);
    HBRUSH br = primary ? st.hRed : st.hBtn;
    HGDIOBJ oldBr = SelectObject(di->hDC, br);
    HGDIOBJ oldPen = SelectObject(di->hDC, GetStockObject(NULL_PEN));
    RoundRect(di->hDC, rc.left, rc.top, rc.right, rc.bottom, 8, 8);
    SelectObject(di->hDC, oldPen);
    SelectObject(di->hDC, oldBr);
    wchar_t text[32] = {};
    GetWindowTextW(di->hwndItem, text, 32);
    SetBkMode(di->hDC, TRANSPARENT);
    SetTextColor(di->hDC, disabled ? RGB(110, 110, 110) : RGB(255, 255, 255));
    if (pressed) OffsetRect(&rc, 1, 1);
    DrawTextW(di->hDC, text, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

HWND MakeLab(HWND parent, HFONT font, int x, int y, int w, const wchar_t* t) {
    HWND h = CreateWindowW(L"STATIC", t,
        WS_CHILD | WS_VISIBLE | SS_NOPREFIX | SS_LEFT,
        x, y, w, kLabH, parent, nullptr, GetModuleHandle(nullptr), nullptr);
    SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    return h;
}

LRESULT CALLBACK PickerProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_CREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        auto* st = reinterpret_cast<PickerState*>(cs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(st));
        st->hwnd = hwnd;
        return 0;
    }
    auto* st = reinterpret_cast<PickerState*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (!st) return DefWindowProc(hwnd, msg, wParam, lParam);

    switch (msg) {
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc = {};
        GetClientRect(hwnd, &rc);
        FillRect(hdc, &rc, st->hBg);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_CTLCOLORSTATIC: {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(210, 210, 210));
        return reinterpret_cast<LRESULT>(st->hBg);
    }
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX: {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        SetBkColor(hdc, RGB(10, 10, 10));
        SetTextColor(hdc, RGB(232, 232, 232));
        return reinterpret_cast<LRESULT>(st->hField);
    }
    case WM_DRAWITEM: {
        auto* di = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
        if (di && di->CtlType == ODT_BUTTON) {
            DrawOwnerButton(*st, di);
            return TRUE;
        }
        break;
    }
    case WM_COMMAND: {
        const int id = LOWORD(wParam);
        const int code = HIWORD(wParam);
        if (id == IdCancel || (id == IdOk && code == BN_CLICKED && st->pickedId < 0)) {
            if (id == IdCancel) {
                st->accepted = false;
                DestroyWindow(hwnd);
            }
            return 0;
        }
        if (id == IdOk && code == BN_CLICKED) {
            Accept(*st);
            return 0;
        }
        if (id == IdCh && code == LBN_SELCHANGE) {
            int sel = static_cast<int>(SendMessageW(st->hCh, LB_GETCURSEL, 0, 0));
            if (sel >= 0) {
                st->pickedId = static_cast<int>(SendMessageW(st->hCh, LB_GETITEMDATA, sel, 0));
            } else {
                st->pickedId = -1;
            }
            SyncOk(*st);
            return 0;
        }
        if (id == IdCh && code == LBN_DBLCLK) {
            Accept(*st);
            return 0;
        }
        if (id == IdFilter && code == EN_CHANGE) {
            FillChannels(*st);
            return 0;
        }
        return 0;
    }
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            st->accepted = false;
            DestroyWindow(hwnd);
            return 0;
        }
        if (wParam == VK_RETURN) {
            Accept(*st);
            return 0;
        }
        return 0;
    case WM_CLOSE:
        st->accepted = false;
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

} // namespace

bool RunChannelPicker(HWND owner, int& channelId) {
    PickerState st;
    st.currentId = channelId;
    st.hBg = CreateSolidBrush(RGB(14, 14, 14));
    st.hField = CreateSolidBrush(RGB(10, 10, 10));
    st.hBtn = CreateSolidBrush(RGB(22, 22, 22));
    st.hRed = CreateSolidBrush(RGB(227, 28, 35));
    st.hFont = CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

    static bool registered = false;
    if (!registered) {
        WNDCLASSW wc = {};
        wc.lpfnWndProc = PickerProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.lpszClassName = L"HolleyChannelPicker";
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
        RegisterClassW(&wc);
        registered = true;
    }

    const int cw = kPad + kChW + kPad;
    const int ch = kPad + kLabH + 4 + kFilterH + 8 + kLabH + 4 + kListH + 8 + kBtnH + 6 + kStatusH + kPad;
    RECT wr = { 0, 0, cw, ch };
    AdjustWindowRect(&wr, WS_CAPTION | WS_SYSMENU | WS_POPUP, FALSE);

    RECT orc{};
    GetWindowRect(owner, &orc);
    const int ww = wr.right - wr.left;
    const int wh = wr.bottom - wr.top;
    HWND hwnd = CreateWindowExW(WS_EX_DLGMODALFRAME | WS_EX_CONTROLPARENT, L"HolleyChannelPicker", L"Channel",
        WS_CAPTION | WS_SYSMENU | WS_POPUP | WS_CLIPCHILDREN,
        orc.left + ((orc.right - orc.left) - ww) / 2,
        orc.top + ((orc.bottom - orc.top) - wh) / 2,
        ww, wh, owner, nullptr, GetModuleHandle(nullptr), &st);
    if (!hwnd) {
        DeleteObject(st.hBg);
        DeleteObject(st.hField);
        DeleteObject(st.hBtn);
        DeleteObject(st.hRed);
        DeleteObject(st.hFont);
        return false;
    }

    int y = kPad;
    MakeLab(hwnd, st.hFont, kPad, y, 80, L"Find");
    st.hFilter = CreateWindowExW(0, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        kPad + 44, y - 2, cw - kPad - 44 - kPad, kFilterH,
        hwnd, (HMENU)(INT_PTR)IdFilter, GetModuleHandle(nullptr), nullptr);
    SendMessageW(st.hFilter, WM_SETFONT, reinterpret_cast<WPARAM>(st.hFont), TRUE);
    SetWindowTheme(st.hFilter, L"", L"");
    y += kFilterH + 8;

    MakeLab(hwnd, st.hFont, kPad, y, kChW, L"Channel");
    y += kLabH + 4;

    st.hCh = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT,
        kPad, y, kChW, kListH,
        hwnd, (HMENU)(INT_PTR)IdCh, GetModuleHandle(nullptr), nullptr);
    SendMessageW(st.hCh, WM_SETFONT, reinterpret_cast<WPARAM>(st.hFont), TRUE);
    SetWindowTheme(st.hCh, L"", L"");
    y += kListH + 8;

    const int btnX = cw - kPad - kBtnW;
    st.hOk = CreateWindowW(L"BUTTON", L"OK",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW | BS_DEFPUSHBUTTON,
        btnX - 8 - kBtnW, y, kBtnW, kBtnH,
        hwnd, (HMENU)(INT_PTR)IdOk, GetModuleHandle(nullptr), nullptr);
    st.hCancel = CreateWindowW(L"BUTTON", L"Cancel",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
        btnX, y, kBtnW, kBtnH,
        hwnd, (HMENU)(INT_PTR)IdCancel, GetModuleHandle(nullptr), nullptr);
    SendMessageW(st.hOk, WM_SETFONT, reinterpret_cast<WPARAM>(st.hFont), TRUE);
    SendMessageW(st.hCancel, WM_SETFONT, reinterpret_cast<WPARAM>(st.hFont), TRUE);
    y += kBtnH + 6;

    st.hStatus = CreateWindowW(L"STATIC", L"",
        WS_CHILD | WS_VISIBLE | SS_NOPREFIX | SS_LEFT | SS_ENDELLIPSIS,
        kPad, y, cw - 2 * kPad, kStatusH,
        hwnd, (HMENU)(INT_PTR)IdStatus, GetModuleHandle(nullptr), nullptr);
    SendMessageW(st.hStatus, WM_SETFONT, reinterpret_cast<WPARAM>(st.hFont), TRUE);

    FillChannels(st);

    EnableWindow(owner, FALSE);
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    SetFocus(st.hFilter);

    MSG msg = {};
    while (IsWindow(hwnd) && GetMessage(&msg, nullptr, 0, 0) > 0) {
        if (!IsDialogMessageW(hwnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    EnableWindow(owner, TRUE);
    SetForegroundWindow(owner);

    DeleteObject(st.hBg);
    DeleteObject(st.hField);
    DeleteObject(st.hBtn);
    DeleteObject(st.hRed);
    DeleteObject(st.hFont);

    if (!st.accepted || st.pickedId < 0)
        return false;
    channelId = st.pickedId;
    return true;
}
