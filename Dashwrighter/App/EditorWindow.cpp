// =============================================================================
// EditorWindow.cpp — window chrome, file actions, preview hit-test / drag
// Inspector + property sync: EditorWindow_Props.cpp
// =============================================================================

#include "EditorWindow.h"
#include "EditorInternals.h"
#include "SymbolPicker.h"
#include "../Io/IniLayoutReader.h"
#include "../Io/IniLayoutWriter.h"
#include "../Io/IniUtils.h"
#include "../Model/GaugeType.h"
#include <commdlg.h>
#include <commctrl.h>
#include <sstream>
#include <algorithm>
#include <uxtheme.h>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "uxtheme.lib")

using namespace Ed;

namespace Ed {

LRESULT CALLBACK RoundedEditProc(HWND h, UINT msg, WPARAM wParam, LPARAM lParam,
    UINT_PTR, DWORD_PTR) {
    switch (msg) {
    case WM_SETFOCUS:
    case WM_KILLFOCUS:
    case WM_ENABLE:
        InvalidateEditWell(h);
        break;
    case WM_NCDESTROY:
        RemoveWindowSubclass(h, RoundedEditProc, 1);
        break;
    default:
        break;
    }
    return DefSubclassProc(h, msg, wParam, lParam);
}

void StyleRoundedEdit(HWND h) {
    if (!h) return;
    LONG style = GetWindowLong(h, GWL_STYLE);
    SetWindowLong(h, GWL_STYLE, style & ~WS_BORDER);
    LONG_PTR ex = GetWindowLongPtr(h, GWL_EXSTYLE);
    SetWindowLongPtr(h, GWL_EXSTYLE,
        ex & ~(WS_EX_CLIENTEDGE | WS_EX_WINDOWEDGE | WS_EX_STATICEDGE));
    SetWindowPos(h, nullptr, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    SendMessage(h, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(6, 6));
    SetWindowSubclass(h, RoundedEditProc, 1, 0);
}

} // namespace Ed

namespace {

LRESULT CALLBACK RoundedComboProc(HWND h, UINT msg, WPARAM wParam, LPARAM lParam,
    UINT_PTR, DWORD_PTR) {
    switch (msg) {
    case WM_NCPAINT:
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(h, &ps);
        RECT rc = {};
        COMBOBOXINFO cbi = {};
        cbi.cbSize = sizeof(cbi);
        if (GetComboBoxInfo(h, &cbi)) {
            rc = cbi.rcItem;
            UnionRect(&rc, &cbi.rcItem, &cbi.rcButton);
        } else {
            GetClientRect(h, &rc);
            rc.bottom = rc.top + 22;
        }
        HBRUSH parent = CreateSolidBrush(RGB(14, 14, 14));
        RECT full = {};
        GetClientRect(h, &full);
        FillRect(hdc, &full, parent);
        HBRUSH field = CreateSolidBrush(kFieldFill);
        HGDIOBJ oldBr = SelectObject(hdc, field);
        HGDIOBJ oldPen = SelectObject(hdc, GetStockObject(NULL_PEN));
        RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, kCorner, kCorner);
        const bool focus = (GetFocus() == h);
        HPEN pen = CreatePen(PS_SOLID, 1, focus ? RGB(227, 28, 35) : RGB(70, 70, 70));
        SelectObject(hdc, GetStockObject(NULL_BRUSH));
        SelectObject(hdc, pen);
        RoundRect(hdc, rc.left, rc.top, rc.right - 1, rc.bottom - 1, kCorner, kCorner);

        wchar_t text[128] = {};
        int sel = static_cast<int>(SendMessageW(h, CB_GETCURSEL, 0, 0));
        if (sel >= 0)
            SendMessageW(h, CB_GETLBTEXT, sel, (LPARAM)text);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(232, 232, 232));
        RECT tr = rc;
        tr.left += 8;
        tr.right -= 22;
        DrawTextW(hdc, text, -1, &tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        RECT cr = { rc.right - 18, rc.top, rc.right - 6, rc.bottom };
        DrawTextW(hdc, L"\u25BE", -1, &cr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        SelectObject(hdc, oldPen);
        SelectObject(hdc, oldBr);
        DeleteObject(pen);
        DeleteObject(field);
        DeleteObject(parent);
        EndPaint(h, &ps);
        return 0;
    }
    case WM_NCDESTROY:
        RemoveWindowSubclass(h, RoundedComboProc, 2);
        break;
    default:
        break;
    }
    return DefSubclassProc(h, msg, wParam, lParam);
}

} // namespace

// ============================================================
// Lifecycle
// ============================================================

EditorWindow::EditorWindow() {
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);
}

EditorWindow::~EditorWindow() {
    ReleasePreviewCache();
    if (backgroundImage) {
        delete backgroundImage;
        backgroundImage = nullptr;
    }
    LayoutRenderer::ReleaseCachedImages();
    if (hUiFont_) { DeleteObject(hUiFont_); hUiFont_ = nullptr; }
    if (hBrBg_) { DeleteObject(hBrBg_); hBrBg_ = nullptr; }
    if (hBrField_) { DeleteObject(hBrField_); hBrField_ = nullptr; }
    if (hBrBar_) { DeleteObject(hBrBar_); hBrBar_ = nullptr; }
    if (hBrRed_) { DeleteObject(hBrRed_); hBrRed_ = nullptr; }
    if (gdiplusToken) {
        Gdiplus::GdiplusShutdown(gdiplusToken);
        gdiplusToken = 0;
    }
}

// ============================================================
// Control factories
// ============================================================

void EditorWindow::ApplyUiFont(HWND h) {
    if (h && hUiFont_)
        SendMessageW(h, WM_SETFONT, (WPARAM)hUiFont_, TRUE);
}

void EditorWindow::EnablePair(HWND lab, HWND edit, bool on) {
    // Labels stay enabled so Win32 does not paint COLOR_GRAYTEXT on black.
    (void)lab;
    EnableCtrl(edit, on);
}

HWND EditorWindow::MakeLabel(int x, int y, int w, int h, const wchar_t* text, bool rightAlign) {
    // SS_NOPREFIX: show literal '&' (avoids "Scale & Sweep" mnemonic glitch).
    DWORD style = WS_CHILD | WS_VISIBLE | SS_NOPREFIX | (rightAlign ? SS_RIGHT : SS_LEFT);
    HWND c = CreateWindowW(L"STATIC", text, style,
        x, y, w, h, hWnd, nullptr, GetModuleHandle(nullptr), nullptr);
    ApplyUiFont(c);
    if (c) {
        SetWindowLongPtr(c, GWLP_USERDATA, MAKELONG(x, w));
        SetPropW(c, L"HDE_R", reinterpret_cast<HANDLE>(static_cast<UINT_PTR>(rightAlign ? 2 : 1)));
    }
    FitLabelToText(c);
    return c;
}

void EditorWindow::RegisterFieldWell(HWND parent, HWND edit, int x, int y, int w, int h) {
    if (!parent || !edit) return;
    FieldWell well;
    well.parent = parent;
    well.edit = edit;
    well.rc = { x, y, x + w, y + h };
    fieldWells_.push_back(well);
}

void EditorWindow::PaintFieldWells(HWND parent, HDC hdc) {
    if (!hdc || !parent) return;
    for (const auto& well : fieldWells_) {
        if (well.parent != parent || !well.edit) continue;
        if (!IsWindowVisible(well.edit)) continue;
        const bool focus = (GetFocus() == well.edit);
        const bool enabled = IsWindowEnabled(well.edit) != FALSE;
        const COLORREF fill = enabled ? kFieldFill : RGB(16, 16, 16);
        const COLORREF stroke = focus ? RGB(227, 28, 35)
            : (enabled ? RGB(70, 70, 70) : RGB(40, 40, 40));
        HBRUSH br = CreateSolidBrush(fill);
        HPEN pen = CreatePen(PS_SOLID, 1, stroke);
        HGDIOBJ oldBr = SelectObject(hdc, br);
        HGDIOBJ oldPen = SelectObject(hdc, pen);
        RoundRect(hdc, well.rc.left, well.rc.top, well.rc.right, well.rc.bottom,
            kCorner, kCorner);
        SelectObject(hdc, oldPen);
        SelectObject(hdc, oldBr);
        DeleteObject(pen);
        DeleteObject(br);
    }
}

// numberOnly enables ES_NUMBER (digits only; blocks '-').
HWND EditorWindow::MakeEdit(int x, int y, int w, int h, int id, bool numberOnly) {
    DWORD style = WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL;
    if (numberOnly) style |= ES_NUMBER;
    HWND c = CreateWindowW(L"EDIT", L"", style,
        x + kWellInset, y + kWellInset, w - 2 * kWellInset, h - 2 * kWellInset,
        hWnd, (HMENU)(INT_PTR)id, GetModuleHandle(nullptr), nullptr);
    ApplyUiFont(c);
    if (c) SetWindowTheme(c, L"", L"");
    StyleRoundedEdit(c);
    RegisterFieldWell(hWnd, c, x, y, w, h);
    return c;
}

// Edit + − / + buttons (mock). Minus id = id+500, plus id = id+600.
HWND EditorWindow::MakeBuddyEdit(int x, int y, int w, int h, int id, int lo, int hi, bool numberOnly) {
    const int btnW = 20;
    const int gap = 2;
    const int editW = (std::max)(40, w - 2 * btnW - 2 * gap);
    HWND hEdit = MakeEdit(x, y, editW, h, id, numberOnly);
    auto mkBtn = [&](int bx, const wchar_t* glyph, int btnId) {
        HWND b = CreateWindowW(L"BUTTON", glyph,
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            bx, y, btnW, h, hWnd, (HMENU)(INT_PTR)btnId, GetModuleHandle(nullptr), nullptr);
        ApplyUiFont(b);
        return b;
    };
    HWND minus = mkBtn(x + editW + gap, L"\x2212", id + 500);
    HWND plus = mkBtn(x + editW + gap + btnW + gap, L"+", id + 600);
    spins_.push_back({ hEdit, minus, plus, lo, hi, id });
    if (minus)
        SetWindowSubclass(minus, BuddyBtnProc, 3, reinterpret_cast<DWORD_PTR>(this));
    if (plus)
        SetWindowSubclass(plus, BuddyBtnProc, 3, reinterpret_cast<DWORD_PTR>(this));
    return hEdit;
}

HWND EditorWindow::MakeCheckOn(HWND parent, int x, int y, int w, int h, int id, const wchar_t* text) {
    HWND box = CreateWindowW(L"BUTTON", L"",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        x, y + 2, 16, 16, parent, (HMENU)(INT_PTR)id, GetModuleHandle(nullptr), nullptr);
    if (box) SetWindowTheme(box, L"", L"");
    HWND lab = CreateWindowW(L"STATIC", text,
        WS_CHILD | WS_VISIBLE | SS_NOPREFIX | SS_LEFT,
        x + 20, y + 1, (std::max)(20, w - 20), h, parent,
        nullptr, GetModuleHandle(nullptr), nullptr);
    ApplyUiFont(lab);
    FitLabelToText(lab);
    checkCaps_.push_back({ box, lab });
    return box;
}

HWND EditorWindow::MakeCheck(int x, int y, int w, int h, int id, const wchar_t* text) {
    return MakeCheckOn(hWnd, x, y, w, h, id, text);
}

void EditorWindow::SetCheckCaption(HWND box, const wchar_t* text) {
    for (auto& p : checkCaps_) {
        if (p.first == box && p.second) {
            SetWindowTextW(p.second, text);
            FitLabelToText(p.second);
            return;
        }
    }
}

HWND EditorWindow::MakeComboOn(HWND parent, int x, int y, int w, int h, int id) {
    HWND c = CreateWindowW(L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL,
        x, y, w, h, parent, (HMENU)(INT_PTR)id, GetModuleHandle(nullptr), nullptr);
    ApplyUiFont(c);
    if (c) {
        SetWindowTheme(c, L"", L"");
        SendMessage(c, CB_SETITEMHEIGHT, static_cast<WPARAM>(-1), 22);
        SendMessage(c, CB_SETITEMHEIGHT, 0, 22);
        SetWindowSubclass(c, RoundedComboProc, 2, 0);
    }
    return c;
}

HWND EditorWindow::MakeCombo(int x, int y, int w, int h, int id) {
    return MakeComboOn(hWnd, x, y, w, h, id);
}

bool EditorWindow::Create() {
    INITCOMMONCONTROLSEX icc = { sizeof(icc),
        ICC_UPDOWN_CLASS | ICC_STANDARD_CLASSES | ICC_BAR_CLASSES };
    InitCommonControlsEx(&icc);

    hBrBg_ = CreateSolidBrush(RGB(14, 14, 14));
    hBrField_ = CreateSolidBrush(kFieldFill);
    hBrBar_ = CreateSolidBrush(RGB(22, 22, 22));
    hBrRed_ = CreateSolidBrush(RGB(227, 28, 35));
    hUiFont_ = CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = L"Dashwrighter";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = hBrBg_;
    RegisterClassW(&wc);

    const int clientW = Insp::kClientW;
    const int clientH = Insp::kClientH;
    RECT wr = { 0, 0, clientW, clientH };
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

    hWnd = CreateWindowExW(
        0, L"Dashwrighter", L"Dashwrighter",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT,
        wr.right - wr.left, wr.bottom - wr.top,
        nullptr, nullptr, GetModuleHandle(nullptr), this);

    if (!hWnd) return false;

    const int by = kToolbarY;
    const int bh = kToolbarH;
    int x = kPad;

    auto btn = [&](const wchar_t* label, int id, int w) {
        HWND h = CreateWindowW(L"BUTTON", label,
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            x, by, w, bh, hWnd, (HMENU)(INT_PTR)id, GetModuleHandle(nullptr), nullptr);
        ApplyUiFont(h);
        toolbarBtns_.push_back(h);
        x += w + 4;
    };

    btn(L"Load Layout", PropId::LoadLayout, 96);
    btn(L"Background", PropId::Background, 86);
    btn(L"Save As", PropId::SaveAs, 68);
    btn(L"Screenshot", PropId::Screenshot, 86);
    btn(L"Positions", PropId::PositionsShot, 78);
    btn(L"Clear", PropId::Clear, 52);
    x += 10;
    btn(L"Add Gauge", PropId::AddGauge, 78);
    btn(L"Add Label", PropId::AddLabel, 76);
    btn(L"Add Output", PropId::AddOutput, 84);
    btn(L"Duplicate", PropId::Duplicate, 72);
    btn(L"Delete", PropId::Delete, 56);
    x += 10;
    btn(L"More settings...", PropId::MoreSettings, 110);

    hLayoutTitle = CreateWindowW(L"STATIC", L"No layout loaded",
        WS_CHILD | WS_VISIBLE | SS_NOPREFIX | SS_LEFT,
        kPad, kStatusY, clientW - 2 * kPad, kStatusH, hWnd, nullptr, GetModuleHandle(nullptr), nullptr);
    ApplyUiFont(hLayoutTitle);

    CreateAppearancePanel();
    CreateMoreSettingsHost();

    SetPropMode(PropMode::None);
    UpdateLayoutTitle();
    UpdateWindowTitle();

    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);
    return true;
}

// Blocks until the user closes the main window. Ctrl+S / Ctrl+D accelerators.
void EditorWindow::Run() {
    ACCEL accels[] = {
        { FVIRTKEY | FCONTROL, 'S', 4 },  // Save
        { FVIRTKEY | FCONTROL, 'D', 11 }, // Duplicate
    };
    HACCEL hAccel = CreateAcceleratorTable(accels, static_cast<int>(sizeof(accels) / sizeof(accels[0])));

    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (!hAccel || !TranslateAccelerator(hWnd, hAccel, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    if (hAccel) DestroyAcceleratorTable(hAccel);
}

// Status feedback: errors via MessageBox; info is quiet (no permanent log pane).
void EditorWindow::Log(const std::string& text) {
    if (text.empty()) return;
    if (text.rfind("ERROR", 0) == 0 || text.find("ERROR:") != std::string::npos) {
        MessageBoxA(hWnd, text.c_str(), "Dashwrighter", MB_OK | MB_ICONWARNING);
    }
}

void EditorWindow::InvalidatePreview() {
    RECT r = {
        PreviewX() - 1,
        PreviewY() - 1,
        PreviewX() + PreviewW() + 1,
        PreviewY() + PreviewH() + 1
    };
    InvalidateRect(hWnd, &r, FALSE);
}

void EditorWindow::ReleasePreviewCache() {
    if (previewMemDC_ && previewOldBmp_)
        SelectObject(previewMemDC_, previewOldBmp_);
    previewOldBmp_ = nullptr;
    if (previewDib_) {
        DeleteObject(previewDib_);
        previewDib_ = nullptr;
    }
    if (previewMemDC_) {
        DeleteDC(previewMemDC_);
        previewMemDC_ = nullptr;
    }
    previewCacheW_ = 0;
    previewCacheH_ = 0;
}

bool EditorWindow::EnsurePreviewCache(HDC hdc, int bw, int bh) {
    if (previewMemDC_ && previewDib_ && previewCacheW_ == bw && previewCacheH_ == bh)
        return true;
    ReleasePreviewCache();
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = bw;
    bmi.bmiHeader.biHeight = -bh;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    void* bits = nullptr;
    previewMemDC_ = CreateCompatibleDC(hdc);
    previewDib_ = CreateDIBSection(previewMemDC_, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!previewMemDC_ || !previewDib_ || !bits) {
        ReleasePreviewCache();
        return false;
    }
    previewOldBmp_ = static_cast<HBITMAP>(SelectObject(previewMemDC_, previewDib_));
    previewCacheW_ = bw;
    previewCacheH_ = bh;
    return true;
}

void EditorWindow::MarkDirty() {
    if (docDirty_) return;
    docDirty_ = true;
    UpdateWindowTitle();
    UpdateLayoutTitle();
}

void EditorWindow::ClearDirty() {
    if (!docDirty_) return;
    docDirty_ = false;
    UpdateWindowTitle();
    UpdateLayoutTitle();
}

void EditorWindow::UpdateWindowTitle() {
    if (!hWnd) return;
    std::wstring title = L"Dashwrighter";
    if (!doc_.sourcePath.empty()) {
        title += L" - ";
        title += Utf8ToWide(FileBaseName(doc_.sourcePath));
    }
    if (docDirty_) title += L" *";
    SetWindowTextW(hWnd, title.c_str());
}

// Yes = save then proceed if saved; No = discard; Cancel = abort. Returns true to proceed.
bool EditorWindow::ConfirmDiscardOrSave() {
    if (!docDirty_) return true;
    int r = MessageBoxW(hWnd,
        L"The layout has unsaved changes.\n\nSave before continuing?",
        L"Unsaved Changes",
        MB_YESNOCANCEL | MB_ICONWARNING);
    if (r == IDCANCEL) return false;
    if (r == IDYES) {
        if (!SaveINI()) return false; // user cancelled save dialog or save failed
    }
    return true;
}

void EditorWindow::UpdateLayoutTitle() {
    if (!hLayoutTitle) return;

    std::string layoutName = "(none)";
    if (!doc_.sourcePath.empty())
        layoutName = FileBaseName(doc_.sourcePath);

    std::string screenName = "(none)";
    if (const LayoutScreen* s = doc_.ActiveScreen()) {
        screenName = s->sectionName.empty() ? "Layout" : s->sectionName;
    }

    std::string bgName = doc_.ActiveBackgroundFileName();
    if (bgName.empty()) bgName = "(none)";

    std::string profileName = doc_.profile.displayName ? doc_.profile.displayName : "Unknown";

    std::ostringstream oss;
    oss << "Layout: " << layoutName
        << "  |  Screen: " << screenName
        << " (" << (doc_.ScreenCount() > 0 ? doc_.activeScreenIndex + 1 : 0) << "/"
        << (doc_.ScreenCount() > 0 ? doc_.ScreenCount() : 0) << ")"
        << "  |  Background: " << bgName
        << "  |  Profile: " << profileName;
    if (docDirty_) oss << "  |  Unsaved";
    if (selectedGauge >= 0 && selectedGauge < static_cast<int>(doc_.Gauges().size()))
        oss << "  |  " << GaugeTypeName(doc_.Gauges()[selectedGauge].gaugeType);
    else if (selectedOutput >= 0)
        oss << "  |  Switch";
    else if (selectedLabel >= 0)
        oss << "  |  Label";
    SetWindowTextA(hLayoutTitle, oss.str().c_str());
}

void EditorWindow::ReloadBackgroundImage() {
    if (backgroundImage) {
        delete backgroundImage;
        backgroundImage = nullptr;
    }

    if (doc_.sourcePath.empty()) {
        InvalidatePreview();
        return;
    }

    bgResolver_.SetLayoutPath(doc_.sourcePath);
    const std::string bg = doc_.ActiveBackgroundFileName();
    if (bg.empty()) {
        Log("No background filename on active screen.");
        InvalidatePreview();
        return;
    }

    std::wstring wide = bgResolver_.ResolveWide(bg);
    if (wide.empty()) {
        Log("ERROR: Could not resolve background: " + bg);
        InvalidatePreview();
        return;
    }

    backgroundImage = new Gdiplus::Image(wide.c_str());
    if (!backgroundImage || backgroundImage->GetLastStatus() != Gdiplus::Ok) {
        Log("ERROR: Could not load background: " + bg);
        delete backgroundImage;
        backgroundImage = nullptr;
    } else {
        Log("Background loaded: " + bg);
    }
    InvalidatePreview();
}

// ============================================================
// Property panel show/hide + model <-> UI sync
// ============================================================

void EditorWindow::SyncBuddyUpDownVisibility() {
    for (const auto& s : spins_) {
        const BOOL on = s.edit && IsWindowEnabled(s.edit);
        if (s.minus) EnableWindow(s.minus, on);
        if (s.plus) EnableWindow(s.plus, on);
    }
}

void EditorWindow::NudgeSpin(HWND btn) {
    for (const auto& s : spins_) {
        if (btn != s.minus && btn != s.plus) continue;
        if (!s.edit || !IsWindowEnabled(s.edit)) return;
        const int delta = (btn == s.plus) ? 1 : -1;
        int v = 0;
        try { v = std::stoi(GetTextA(s.edit)); } catch (...) { v = 0; }
        v += delta;
        if (v < s.lo) v = s.lo;
        if (v > s.hi) v = s.hi;
        SetTextA(s.edit, std::to_string(v));
        LiveUpdateFromPanel();
        return;
    }
}

LRESULT CALLBACK EditorWindow::BuddyBtnProc(HWND h, UINT msg, WPARAM wParam, LPARAM lParam,
    UINT_PTR, DWORD_PTR ref) {
    auto* self = reinterpret_cast<EditorWindow*>(ref);
    constexpr UINT_PTR kTimer = 1;
    switch (msg) {
    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK: {
        LRESULT r = DefSubclassProc(h, msg, wParam, lParam);
        if (self) {
            self->NudgeSpin(h);
            SetCapture(h);
            SetTimer(h, kTimer, 400, nullptr);
        }
        return r;
    }
    case WM_TIMER:
        if (wParam == kTimer && self) {
            if ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) == 0) {
                KillTimer(h, kTimer);
                if (GetCapture() == h) ReleaseCapture();
                return 0;
            }
            SetTimer(h, kTimer, 55, nullptr);
            self->NudgeSpin(h);
        }
        return 0;
    case WM_LBUTTONUP:
    case WM_CAPTURECHANGED:
        KillTimer(h, kTimer);
        if (msg == WM_LBUTTONUP && GetCapture() == h)
            ReleaseCapture();
        break;
    case WM_NCDESTROY:
        KillTimer(h, kTimer);
        RemoveWindowSubclass(h, BuddyBtnProc, 3);
        break;
    }
    return DefSubclassProc(h, msg, wParam, lParam);
}

void EditorWindow::LoadINI() {
    if (!ConfirmDiscardOrSave()) return;

    wchar_t szFile[520] = {};
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = static_cast<DWORD>(sizeof(szFile) / sizeof(wchar_t));
    ofn.lpstrFilter = L"INI Files\0*.ini\0All Files\0*.*\0";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (!GetOpenFileNameW(&ofn)) return;

    std::string path = WideToUtf8(szFile);
    LayoutDocument loaded;
    IniLayoutReader reader([this](const std::string& msg) { Log(msg); });
    if (!reader.Load(path, loaded))
        return;

    doc_ = std::move(loaded);
    selectedGauge = -1;
    selectedOutput = -1;
    selectedLabel = -1;
    suppressLiveUpdate = true;
    ClearPropertyPanel();
    SetPropMode(PropMode::None);
    suppressLiveUpdate = false;

    if (doc_.activeScreenIndex < 0 || doc_.activeScreenIndex >= doc_.ScreenCount())
        doc_.SetActiveScreenIndex(0);

    ReloadBackgroundImage();
    UpdateLayoutTitle();

    const auto& gauges = doc_.Gauges();
    const auto& outputs = doc_.Outputs();
    const auto& labels = doc_.Labels();
    Log("Screens: " + std::to_string(doc_.ScreenCount()) +
        "   Gauges: " + std::to_string(gauges.size()) +
        "   Outputs: " + std::to_string(outputs.size()) +
        "   Labels: " + std::to_string(labels.size()));

    if (!gauges.empty()) {
        selectedGauge = 0;
        selectedOutput = -1;
        selectedLabel = -1;
        PopulatePropertyPanel();
        Log("Auto-selected Gauge 1: " + gauges[0].label +
            "  (type " + std::to_string(gauges[0].gaugeType) + " " +
            GaugeTypeName(gauges[0].gaugeType) + ")");
    }

    ClearDirty();
    UpdateWindowTitle();
    InvalidateRect(hWnd, nullptr, TRUE);
}

// Save-file dialog â†’ IniLayoutWriter. Returns true only on successful write.
bool EditorWindow::SaveINI() {
    wchar_t szFile[520] = {};
    if (!doc_.sourcePath.empty()) {
        std::wstring base = Utf8ToWide(FileBaseName(doc_.sourcePath));
        wcsncpy_s(szFile, base.c_str(), _TRUNCATE);
    }

    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = static_cast<DWORD>(sizeof(szFile) / sizeof(wchar_t));
    ofn.lpstrFilter = L"INI Files\0*.ini\0All Files\0*.*\0";
    ofn.lpstrDefExt = L"ini";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    if (!GetSaveFileNameW(&ofn)) return false;

    std::string path = WideToUtf8(szFile);
    IniLayoutWriter writer([this](const std::string& msg) { Log(msg); });
    if (writer.Save(path, doc_)) {
        doc_.sourcePath = path;
        ClearDirty();
        UpdateLayoutTitle();
        UpdateWindowTitle();
        Log("Successfully saved: " + path);
        MessageBoxW(hWnd, L"Layout saved successfully!", L"Save Layout", MB_OK | MB_ICONINFORMATION);
        return true;
    }
    Log("ERROR: Failed to save layout.");
    MessageBoxW(hWnd, L"Failed to save layout.", L"Save Layout", MB_OK | MB_ICONERROR);
    return false;
}

void EditorWindow::SaveScreenshot() {
    SaveCanvasPng(false);
}

void EditorWindow::SavePositionScreenshot() {
    SaveCanvasPng(true);
}

// Render only the dash canvas (no toolbar/panel). ringAllGauges draws a blue
// selection ring around every analogMeters gauge (position overlay).
void EditorWindow::SaveCanvasPng(bool ringAllGauges) {
    constexpr int kOutW = 1280;
    constexpr int kOutH = 480;
    const wchar_t* title = ringAllGauges ? L"Positions" : L"Screenshot";

    const int cw = PreviewW();
    const int ch = PreviewH();
    if (cw <= 0 || ch <= 0) {
        MessageBoxW(hWnd, L"Canvas size is invalid.", title, MB_OK | MB_ICONWARNING);
        return;
    }

    wchar_t szFile[520] = {};
    if (!doc_.sourcePath.empty()) {
        std::string base = FileBaseName(doc_.sourcePath);
        // Strip trailing .ini if present for a natural PNG name.
        if (base.size() > 4) {
            std::string ext = base.substr(base.size() - 4);
            for (char& c : ext) if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
            if (ext == ".ini") base = base.substr(0, base.size() - 4);
        }
        std::wstring wbase = Utf8ToWide(base);
        wcsncpy_s(szFile, wbase.c_str(), _TRUNCATE);
        wcsncat_s(szFile, ringAllGauges ? L"-positions.png" : L".png", _TRUNCATE);
    } else {
        wcsncpy_s(szFile, ringAllGauges ? L"positions.png" : L"screenshot.png", _TRUNCATE);
    }

    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = static_cast<DWORD>(sizeof(szFile) / sizeof(wchar_t));
    ofn.lpstrFilter = L"PNG Image\0*.png\0All Files\0*.*\0";
    ofn.lpstrDefExt = L"png";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    if (!GetSaveFileNameW(&ofn)) return;

    // Offscreen 32-bpp top-down DIB at native canvas size.
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = cw;
    bmi.bmiHeader.biHeight = -ch; // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HDC screenDC = GetDC(hWnd);
    HDC memDC = CreateCompatibleDC(screenDC);
    HBITMAP dib = CreateDIBSection(memDC, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!dib || !bits) {
        if (dib) DeleteObject(dib);
        DeleteDC(memDC);
        ReleaseDC(hWnd, screenDC);
        MessageBoxW(hWnd, L"Failed to create screenshot bitmap.", title, MB_OK | MB_ICONERROR);
        return;
    }
    HBITMAP oldBmp = static_cast<HBITMAP>(SelectObject(memDC, dib));

    RECT canvasRect = { 0, 0, cw, ch };
    FillRect(memDC, &canvasRect, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));

    // Dash-comparable image: no window chrome. Positions: ring every gauge.
    const int saveOriginX = renderer_.originX;
    const int saveOriginY = renderer_.originY;
    const int saveSelG = renderer_.selectedGauge;
    const int saveSelO = renderer_.selectedOutput;
    const int saveSelL = renderer_.selectedLabel;
    const bool saveRingAll = renderer_.ringAllGauges;

    renderer_.originX = 0;
    renderer_.originY = 0;
    renderer_.selectedGauge = -1;
    renderer_.selectedOutput = -1;
    renderer_.selectedLabel = -1;
    renderer_.ringAllGauges = ringAllGauges;

    if (const LayoutScreen* screen = doc_.ActiveScreen())
        renderer_.Draw(memDC, *screen, backgroundImage, cw, ch);

    renderer_.originX = saveOriginX;
    renderer_.originY = saveOriginY;
    renderer_.selectedGauge = saveSelG;
    renderer_.selectedOutput = saveSelO;
    renderer_.selectedLabel = saveSelL;
    renderer_.ringAllGauges = saveRingAll;

    // Unselect before GDI+ wraps the DIB bits.
    SelectObject(memDC, oldBmp);

    // GDI DIB is BGRX; PixelFormat32bppRGB matches that channel order.
    Gdiplus::Bitmap src(cw, ch, cw * 4, PixelFormat32bppRGB, static_cast<BYTE*>(bits));
    Gdiplus::Status st = src.GetLastStatus();

    CLSID pngClsid = {};
    bool ok = false;
    if (st == Gdiplus::Ok && GetEncoderClsid(L"image/png", &pngClsid)) {
        if (cw == kOutW && ch == kOutH) {
            ok = (src.Save(szFile, &pngClsid, nullptr) == Gdiplus::Ok);
        } else {
            Gdiplus::Bitmap dest(kOutW, kOutH, PixelFormat32bppARGB);
            if (dest.GetLastStatus() == Gdiplus::Ok) {
                Gdiplus::Graphics gfx(&dest);
                gfx.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
                gfx.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);
                gfx.Clear(Gdiplus::Color(255, 0, 0, 0));
                gfx.DrawImage(&src, 0, 0, kOutW, kOutH);
                ok = (dest.Save(szFile, &pngClsid, nullptr) == Gdiplus::Ok);
            }
        }
    }

    DeleteObject(dib);
    DeleteDC(memDC);
    ReleaseDC(hWnd, screenDC);

    if (ok) {
        Log(std::string(ringAllGauges ? "Positions saved: " : "Screenshot saved: ") + WideToUtf8(szFile));
        MessageBoxW(hWnd, ringAllGauges ? L"Positions saved" : L"Screenshot saved",
            title, MB_OK | MB_ICONINFORMATION);
    } else {
        Log(ringAllGauges ? "ERROR: Failed to save positions PNG." : "ERROR: Failed to save screenshot.");
        MessageBoxW(hWnd, ringAllGauges ? L"Failed to save positions PNG." : L"Failed to save screenshot.",
            title, MB_OK | MB_ICONERROR);
    }
}

// Pick an image file, assign its basename to the active screen, show it immediately.
void EditorWindow::LoadBackground() {
    wchar_t szFile[520] = {};
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = static_cast<DWORD>(sizeof(szFile) / sizeof(wchar_t));
    ofn.lpstrFilter = L"Images\0*.jpg;*.jpeg;*.png;*.bmp\0All Files\0*.*\0";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (!GetOpenFileNameW(&ofn)) return;

    Gdiplus::Image* img = new Gdiplus::Image(szFile);
    if (!img || img->GetLastStatus() != Gdiplus::Ok) {
        Log("ERROR: Could not load background image.");
        delete img;
        return;
    }
    // Empty document has no screen until Add Gauge; still show the wallpaper.
    if (!EnsureActiveScreen()) {
        delete img;
        return;
    }

    if (backgroundImage) {
        delete backgroundImage;
        backgroundImage = nullptr;
    }
    backgroundImage = img;

    std::string full = WideToUtf8(szFile);
    std::string fileName = FileBaseName(full);
    doc_.SetActiveBackgroundFileName(fileName);
    if (!doc_.sourcePath.empty())
        bgResolver_.SetLayoutPath(doc_.sourcePath);
    bgResolver_.AddSearchRoot(IniUtils::DirNameFromPath(full));

    Log("Background loaded: " + fileName);
    MarkDirty();
    UpdateLayoutTitle();
    InvalidatePreview();
}

// Discard the document and preview image; reset UI to empty state.
void EditorWindow::ClearLayout() {
    if (!ConfirmDiscardOrSave()) return;

    doc_.Clear();
    selectedGauge = -1;
    selectedOutput = -1;
    selectedLabel = -1;
    if (backgroundImage) {
        delete backgroundImage;
        backgroundImage = nullptr;
    }
    suppressLiveUpdate = true;
    ClearPropertyPanel();
    SetPropMode(PropMode::None);
    suppressLiveUpdate = false;
    ClearDirty();
    UpdateLayoutTitle();
    UpdateWindowTitle();
    InvalidateRect(hWnd, nullptr, TRUE);
}

bool EditorWindow::EnsureActiveScreen() {
    if (doc_.ActiveScreen()) return true;
    LayoutScreen s;
    s.sectionName = "Layout";
    s.version = 10;
    doc_.screens.push_back(std::move(s));
    doc_.activeScreenIndex = 0;
    return true;
}

void EditorWindow::AddGauge() {
    if (!EnsureActiveScreen()) return;
    Gauge g;
    g.label = "New Gauge";
    g.units = "";
    g.x = 40;
    g.y = 40;
    g.width = 120;
    g.height = 120;
    g.minValue = 0;
    g.maxValue = 100;
    g.minCaution = 10;
    g.minNormal = 20;
    g.maxNormal = 80;
    g.maxCaution = 90;
    g.gaugeType = 0;
    g.foreground = 0xFFFFFFFFu;
    g.background = 0xFF202020u;
    g.c0 = 0xFFFF0000u;
    g.c1 = 0xFFFFFF00u;
    g.c2 = 0xFF00FF00u;
    g.c3 = 0xFFFFFF00u;
    g.c4 = 0xFFFF0000u;
    g.numericIndicatorEnabled = true;
    g.displayLabel = true;
    g.channel = kHolleyBatteryChannel;
    g.rpmModeEnabled = false;
    // originalKeys empty → writer emits full key set

    auto& gauges = doc_.Gauges();
    gauges.push_back(std::move(g));
    selectedGauge = static_cast<int>(gauges.size()) - 1;
    selectedOutput = -1;
    selectedLabel = -1;
    PopulatePropertyPanel();
    MarkDirty();
    UpdateLayoutTitle();
    InvalidatePreview();
    Log("Added Gauge " + std::to_string(selectedGauge + 1));
}

void EditorWindow::AddLabel() {
    if (!EnsureActiveScreen()) return;
    Label lab;
    lab.text = "Label";
    lab.x = 60;
    lab.y = 60;
    lab.fontSize = 20;
    lab.foreground = 0xFFFFFFFFu;

    auto& labels = doc_.Labels();
    labels.push_back(std::move(lab));
    selectedLabel = static_cast<int>(labels.size()) - 1;
    selectedGauge = -1;
    selectedOutput = -1;
    PopulatePropertyPanel();
    MarkDirty();
    InvalidatePreview();
    Log("Added Label " + std::to_string(selectedLabel + 1));
}

void EditorWindow::AddOutput() {
    if (!EnsureActiveScreen()) return;
    Output o;
    o.text = "Switch";
    o.channel = 0;
    o.style = 1;
    o.x = 80;
    o.y = 80;
    o.fontSize = 16;
    o.inactiveColor = 0xFF808080u;
    o.activeColor = 0xFFFFFFFFu;
    o.background = 0x00000000u;
    // originalKeys empty -> writer emits full key set

    auto& outputs = doc_.Outputs();
    outputs.push_back(std::move(o));
    selectedOutput = static_cast<int>(outputs.size()) - 1;
    selectedGauge = -1;
    selectedLabel = -1;
    PopulatePropertyPanel();
    MarkDirty();
    UpdateLayoutTitle();
    InvalidatePreview();
    Log("Added Output " + std::to_string(selectedOutput + 1));
}

void EditorWindow::DuplicateSelected() {
    if (selectedGauge >= 0) {
        auto& gauges = doc_.Gauges();
        if (selectedGauge >= static_cast<int>(gauges.size())) return;
        Gauge copy = gauges[static_cast<size_t>(selectedGauge)];
        copy.x += 20;
        copy.y += 20;
        copy.originalKeys.clear(); // force full write on save
        gauges.push_back(std::move(copy));
        selectedGauge = static_cast<int>(gauges.size()) - 1;
        selectedOutput = -1;
        selectedLabel = -1;
        PopulatePropertyPanel();
        MarkDirty();
        InvalidatePreview();
        Log("Duplicated Gauge -> " + std::to_string(selectedGauge + 1));
        return;
    }
    if (selectedLabel >= 0) {
        auto& labels = doc_.Labels();
        if (selectedLabel >= static_cast<int>(labels.size())) return;
        Label copy = labels[static_cast<size_t>(selectedLabel)];
        copy.x += 20;
        copy.y += 20;
        copy.originalKeys.clear();
        labels.push_back(std::move(copy));
        selectedLabel = static_cast<int>(labels.size()) - 1;
        selectedGauge = -1;
        selectedOutput = -1;
        PopulatePropertyPanel();
        MarkDirty();
        InvalidatePreview();
        Log("Duplicated Label -> " + std::to_string(selectedLabel + 1));
        return;
    }
    if (selectedOutput >= 0) {
        auto& outputs = doc_.Outputs();
        if (selectedOutput >= static_cast<int>(outputs.size())) return;
        Output copy = outputs[static_cast<size_t>(selectedOutput)];
        copy.x += 20;
        copy.y += 20;
        copy.originalKeys.clear();
        outputs.push_back(std::move(copy));
        selectedOutput = static_cast<int>(outputs.size()) - 1;
        selectedGauge = -1;
        selectedLabel = -1;
        PopulatePropertyPanel();
        MarkDirty();
        InvalidatePreview();
        Log("Duplicated Output -> " + std::to_string(selectedOutput + 1));
        return;
    }
    Log("Nothing selected to duplicate.");
}

void EditorWindow::DeleteSelected() {
    if (selectedGauge >= 0) {
        auto& gauges = doc_.Gauges();
        if (selectedGauge >= static_cast<int>(gauges.size())) return;
        int idx = selectedGauge;
        gauges.erase(gauges.begin() + idx);
        ClearSelection();
        MarkDirty();
        InvalidatePreview();
        Log("Deleted Gauge " + std::to_string(idx + 1));
        return;
    }
    if (selectedLabel >= 0) {
        auto& labels = doc_.Labels();
        if (selectedLabel >= static_cast<int>(labels.size())) return;
        int idx = selectedLabel;
        labels.erase(labels.begin() + idx);
        ClearSelection();
        MarkDirty();
        InvalidatePreview();
        Log("Deleted Label " + std::to_string(idx + 1));
        return;
    }
    if (selectedOutput >= 0) {
        auto& outputs = doc_.Outputs();
        if (selectedOutput >= static_cast<int>(outputs.size())) return;
        int idx = selectedOutput;
        outputs.erase(outputs.begin() + idx);
        ClearSelection();
        MarkDirty();
        InvalidatePreview();
        Log("Deleted Output " + std::to_string(idx + 1));
        return;
    }
    Log("Nothing selected to delete.");
}

void EditorWindow::EndDrag(bool commit) {
    if (!dragging_) return;
    dragging_ = false;
    ReleaseCapture();
    SetCursor(LoadCursor(nullptr, IDC_ARROW));
    if (commit && dragMoved_) {
        SyncPositionEditsFromModel();
        MarkDirty();
    }
    dragMoved_ = false;
}

// After Prev/Next screen: drop selection, reload that page's background.
void EditorWindow::OnScreenChanged() {
    ClearSelection();
    ReloadBackgroundImage();
    UpdateLayoutTitle();
    InvalidateRect(hWnd, nullptr, TRUE);
}

// No selected widget; hide property values.
void EditorWindow::ClearSelection() {
    selectedGauge = -1;
    selectedOutput = -1;
    selectedLabel = -1;
    suppressLiveUpdate = true;
    ClearPropertyPanel();
    SetPropMode(PropMode::None);
    suppressLiveUpdate = false;
    InvalidatePreview();
}

static std::wstring TruncateMenu(const std::wstring& s, size_t maxChars) {
    if (s.size() <= maxChars) return s;
    if (maxChars < 2) return s;
    return s.substr(0, maxChars - 1) + L"...";
}

bool EditorWindow::PreviewHitIsSelected(const PreviewHit& hit) const {
    switch (hit.kind) {
    case PreviewHitKind::Gauge:  return selectedGauge == hit.index;
    case PreviewHitKind::Label:  return selectedLabel == hit.index;
    case PreviewHitKind::Output: return selectedOutput == hit.index;
    }
    return false;
}

void EditorWindow::ApplyPreviewHit(const PreviewHit& hit) {
    const LayoutScreen* screen = doc_.ActiveScreen();
    if (!screen) return;
    selectedGauge = -1;
    selectedOutput = -1;
    selectedLabel = -1;
    switch (hit.kind) {
    case PreviewHitKind::Gauge:
        if (hit.index < 0 || hit.index >= static_cast<int>(screen->gauges.size()))
            return;
        selectedGauge = hit.index;
        PopulatePropertyPanel();
        {
            const Gauge& g = screen->gauges[static_cast<size_t>(hit.index)];
            Log("Selected Gauge " + std::to_string(hit.index + 1) + ": " + g.label +
                "  (type " + std::to_string(g.gaugeType) + " " + GaugeTypeName(g.gaugeType) + ")");
        }
        break;
    case PreviewHitKind::Label:
        if (hit.index < 0 || hit.index >= static_cast<int>(screen->labels.size()))
            return;
        selectedLabel = hit.index;
        PopulatePropertyPanel();
        Log("Selected Label " + std::to_string(hit.index + 1) + ": " +
            screen->labels[static_cast<size_t>(hit.index)].text);
        break;
    case PreviewHitKind::Output:
        if (hit.index < 0 || hit.index >= static_cast<int>(screen->outputs.size()))
            return;
        selectedOutput = hit.index;
        PopulatePropertyPanel();
        {
            const Output& o = screen->outputs[static_cast<size_t>(hit.index)];
            Log("Selected Output " + std::to_string(hit.index + 1) + ": " + o.text +
                "  (style " + std::to_string(o.style) + ")");
        }
        break;
    }
    InvalidatePreview();
}

void EditorWindow::ArmPreviewDrag(const PreviewHit& hit, int mx, int my) {
    const LayoutScreen* screen = doc_.ActiveScreen();
    if (!screen) return;
    int ox = 0, oy = 0;
    switch (hit.kind) {
    case PreviewHitKind::Gauge:
        if (hit.index < 0 || hit.index >= static_cast<int>(screen->gauges.size()))
            return;
        ox = screen->gauges[static_cast<size_t>(hit.index)].x;
        oy = screen->gauges[static_cast<size_t>(hit.index)].y;
        break;
    case PreviewHitKind::Label:
        if (hit.index < 0 || hit.index >= static_cast<int>(screen->labels.size()))
            return;
        ox = screen->labels[static_cast<size_t>(hit.index)].x;
        oy = screen->labels[static_cast<size_t>(hit.index)].y;
        break;
    case PreviewHitKind::Output:
        if (hit.index < 0 || hit.index >= static_cast<int>(screen->outputs.size()))
            return;
        ox = screen->outputs[static_cast<size_t>(hit.index)].x;
        oy = screen->outputs[static_cast<size_t>(hit.index)].y;
        break;
    }
    dragging_ = true;
    dragMoved_ = false;
    dragStartMx_ = mx;
    dragStartMy_ = my;
    dragOriginX_ = ox;
    dragOriginY_ = oy;
    SetCapture(hWnd);
    SetCursor(LoadCursor(nullptr, IDC_SIZEALL));
}

void EditorWindow::ReorderGauge(int index, bool toFront) {
    auto& gauges = doc_.Gauges();
    const int n = static_cast<int>(gauges.size());
    if (index < 0 || index >= n || n < 2) return;
    if (toFront && index == n - 1) return;
    if (!toFront && index == 0) return;

    Gauge g = std::move(gauges[static_cast<size_t>(index)]);
    gauges.erase(gauges.begin() + index);
    int newIndex = 0;
    if (toFront) {
        gauges.push_back(std::move(g));
        newIndex = static_cast<int>(gauges.size()) - 1;
        if (selectedGauge == index)
            selectedGauge = newIndex;
        else if (selectedGauge > index)
            selectedGauge -= 1;
    } else {
        gauges.insert(gauges.begin(), std::move(g));
        newIndex = 0;
        if (selectedGauge == index)
            selectedGauge = 0;
        else if (selectedGauge >= 0 && selectedGauge < index)
            selectedGauge += 1;
    }
    selectedOutput = -1;
    selectedLabel = -1;
    PopulatePropertyPanel();
    MarkDirty();
    InvalidatePreview();
    Log(std::string(toFront ? "Brought to front" : "Sent to back") +
        ": Gauge " + std::to_string(newIndex + 1));
}

void EditorWindow::ShowOverlapMenu(const std::vector<PreviewHit>& hits, int mx, int my) {
    const LayoutScreen* screen = doc_.ActiveScreen();
    if (!screen || hits.empty()) return;
    stackHits_ = hits;
    HMENU menu = CreatePopupMenu();
    if (!menu) return;
    const int nGauges = static_cast<int>(screen->gauges.size());
    const int nHits = static_cast<int>(hits.size());
    const int slotMax = PropId::StackMenuSlotMax;
    for (int i = 0; i < nHits && i < slotMax; ++i) {
        const PreviewHit& hit = hits[static_cast<size_t>(i)];
        std::wstring line;
        switch (hit.kind) {
        case PreviewHitKind::Gauge:
            if (hit.index >= 0 && hit.index < nGauges) {
                const Gauge& g = screen->gauges[static_cast<size_t>(hit.index)];
                line = L"Gauge " + std::to_wstring(hit.index + 1) + L"  " +
                    Utf8ToWide(GaugeTypeName(g.gaugeType)) + L"  \"" +
                    TruncateMenu(Utf8ToWide(g.label), 32) + L"\"";
            }
            break;
        case PreviewHitKind::Label:
            if (hit.index >= 0 && hit.index < static_cast<int>(screen->labels.size())) {
                line = L"Label  \"" +
                    TruncateMenu(Utf8ToWide(screen->labels[static_cast<size_t>(hit.index)].text), 32) +
                    L"\"";
            }
            break;
        case PreviewHitKind::Output:
            if (hit.index >= 0 && hit.index < static_cast<int>(screen->outputs.size())) {
                line = L"Switch  \"" +
                    TruncateMenu(Utf8ToWide(screen->outputs[static_cast<size_t>(hit.index)].text), 32) +
                    L"\"";
            }
            break;
        }
        if (line.empty())
            line = L"(unknown)";
        const bool checked = PreviewHitIsSelected(hit);
        if (hit.kind == PreviewHitKind::Gauge && hit.index >= 0 && hit.index < nGauges) {
            HMENU sub = CreatePopupMenu();
            if (!sub) continue;
            AppendMenuW(sub, MF_STRING, static_cast<UINT>(PropId::StackMenuBase + i), L"Select");
            UINT frontFlags = MF_STRING;
            if (hit.index >= nGauges - 1)
                frontFlags |= MF_GRAYED;
            AppendMenuW(sub, frontFlags, static_cast<UINT>(PropId::StackFrontBase + i), L"Bring to front");
            UINT backFlags = MF_STRING;
            if (hit.index <= 0)
                backFlags |= MF_GRAYED;
            AppendMenuW(sub, backFlags, static_cast<UINT>(PropId::StackBackBase + i), L"Send to back");
            UINT popFlags = MF_POPUP | MF_STRING;
            if (checked)
                popFlags |= MF_CHECKED;
            AppendMenuW(menu, popFlags, reinterpret_cast<UINT_PTR>(sub), line.c_str());
        } else {
            UINT flags = MF_STRING;
            if (checked)
                flags |= MF_CHECKED;
            AppendMenuW(menu, flags, static_cast<UINT>(PropId::StackMenuBase + i), line.c_str());
        }
    }
    POINT pt = { mx, my };
    ClientToScreen(hWnd, &pt);
    UINT cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_LEFTALIGN,
        pt.x, pt.y, 0, hWnd, nullptr);
    DestroyMenu(menu);
    if (cmd == 0) return;
    const int n = static_cast<int>(stackHits_.size());
    auto hitAt = [&](UINT base) -> const PreviewHit* {
        if (cmd < base) return nullptr;
        const int i = static_cast<int>(cmd - base);
        if (i < 0 || i >= n || i >= slotMax) return nullptr;
        return &stackHits_[static_cast<size_t>(i)];
    };
    if (const PreviewHit* h = hitAt(static_cast<UINT>(PropId::StackFrontBase))) {
        if (h->kind == PreviewHitKind::Gauge)
            ReorderGauge(h->index, true);
        return;
    }
    if (const PreviewHit* h = hitAt(static_cast<UINT>(PropId::StackBackBase))) {
        if (h->kind == PreviewHitKind::Gauge)
            ReorderGauge(h->index, false);
        return;
    }
    if (const PreviewHit* h = hitAt(static_cast<UINT>(PropId::StackMenuBase)))
        ApplyPreviewHit(*h);
}

// ============================================================
// Win32 message handling
// ============================================================

// Static WndProc: on WM_CREATE store EditorWindow* in GWLP_USERDATA, then forward.
LRESULT CALLBACK EditorWindow::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    EditorWindow* pThis = nullptr;
    if (uMsg == WM_CREATE) {
        auto* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        pThis = reinterpret_cast<EditorWindow*>(pCreate->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
    } else {
        pThis = reinterpret_cast<EditorWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }
    if (pThis) return pThis->HandleMessage(hwnd, uMsg, wParam, lParam);
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// Instance message handler: toolbar, property edits, color pickers, hit-test, paint.
LRESULT EditorWindow::HandleMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_COMMAND: {
        // LOWORD = control id, HIWORD = notification (EN_CHANGE, BN_CLICKED, ...)
        int id = LOWORD(wParam);
        int code = HIWORD(wParam);
        if (id == PropId::LoadLayout) LoadINI();
        else if (id == PropId::Background) LoadBackground();
        else if (id == PropId::SaveAs) SaveINI();
        else if (id == PropId::Screenshot) SaveScreenshot();
        else if (id == PropId::PositionsShot) SavePositionScreenshot();
        else if (id == PropId::Clear) ClearLayout();
        else if (id == PropId::AddGauge) AddGauge();
        else if (id == PropId::AddLabel) AddLabel();
        else if (id == PropId::AddOutput) AddOutput();
        else if (id == PropId::Duplicate) DuplicateSelected();
        else if (id == PropId::Delete) DeleteSelected();
        else if (id == PropId::MoreSettings) ShowMoreSettings();
        else if (id == PropId::R_SymbolPick) ShowSymbolPicker();
        else if (id == PropId::D_ChannelPick) ShowChannelPicker();
        else if (id == PropId::R_FgBtn) {
            if (hFgBtn && !IsWindowEnabled(hFgBtn)) break;
            auto& gauges = doc_.Gauges();
            auto& labels = doc_.Labels();
            if (selectedGauge >= 0 && selectedGauge < static_cast<int>(gauges.size())) {
                if (PickColor(gauges[selectedGauge].foreground)) {
                    UpdateColorSwatches();
                    MarkDirty();
                    InvalidatePreview();
                }
            } else if (selectedLabel >= 0 && selectedLabel < static_cast<int>(labels.size())) {
                if (PickColor(labels[selectedLabel].foreground)) {
                    UpdateColorSwatches();
                    MarkDirty();
                    InvalidatePreview();
                }
            }
        } else if (id == PropId::R_BgBtn) {
            if (hBgBtn && !IsWindowEnabled(hBgBtn)) break;
            auto& gauges = doc_.Gauges();
            auto& labels = doc_.Labels();
            if (selectedGauge >= 0 && selectedGauge < static_cast<int>(gauges.size())) {
                if (PickColor(gauges[selectedGauge].background)) {
                    suppressLiveUpdate = true;
                    SetTransparencyUi(static_cast<int>(
                        (gauges[selectedGauge].background >> 24) & 0xFF));
                    suppressLiveUpdate = false;
                    UpdateColorSwatches();
                    MarkDirty();
                    InvalidatePreview();
                }
            } else if (selectedLabel >= 0 && selectedLabel < static_cast<int>(labels.size())) {
                if (PickColor(labels[selectedLabel].background)) {
                    suppressLiveUpdate = true;
                    SetTransparencyUi(static_cast<int>(
                        (labels[selectedLabel].background >> 24) & 0xFF));
                    suppressLiveUpdate = false;
                    UpdateColorSwatches();
                    MarkDirty();
                    InvalidatePreview();
                }
            }
        } else if (id >= PropId::R_C0Btn && id <= PropId::R_C0Btn + 4) {
            int zone = id - PropId::R_C0Btn;
            if (zone >= 0 && zone < 5 && hZoneBtn[zone] && !IsWindowEnabled(hZoneBtn[zone])) break;
            auto& gauges = doc_.Gauges();
            if (selectedGauge >= 0 && selectedGauge < static_cast<int>(gauges.size()) &&
                zone >= 0 && zone < 5) {
                unsigned int* pCol = nullptr;
                switch (zone) {
                case 0: pCol = &gauges[selectedGauge].c0; break;
                case 1: pCol = &gauges[selectedGauge].c1; break;
                case 2: pCol = &gauges[selectedGauge].c2; break;
                case 3: pCol = &gauges[selectedGauge].c3; break;
                case 4: pCol = &gauges[selectedGauge].c4; break;
                }
                if (pCol && PickColor(*pCol)) {
                    UpdateColorSwatches();
                    MarkDirty();
                    InvalidatePreview();
                }
            }
        } else if (id == PropId::R_OutInactBtn) {
            auto& outputs = doc_.Outputs();
            if (selectedOutput >= 0 && selectedOutput < static_cast<int>(outputs.size())) {
                if (PickColor(outputs[selectedOutput].inactiveColor)) {
                    UpdateColorSwatches();
                    MarkDirty();
                    InvalidatePreview();
                }
            }
        } else if (id == PropId::R_OutActBtn) {
            auto& outputs = doc_.Outputs();
            if (selectedOutput >= 0 && selectedOutput < static_cast<int>(outputs.size())) {
                if (PickColor(outputs[selectedOutput].activeColor)) {
                    UpdateColorSwatches();
                    MarkDirty();
                    InvalidatePreview();
                }
            }
        } else if (id == PropId::R_GaugeType && code == CBN_SELCHANGE) {
            InvalidateRect(hComboGaugeType, nullptr, FALSE);
            LiveUpdateFromPanel(true);
            // Size / Border Style visibility depends on type
            if (selectedGauge >= 0) {
                const auto& gauges = doc_.Gauges();
                if (selectedGauge < static_cast<int>(gauges.size()))
                    ApplyGaugeTypeVisibility(gauges[selectedGauge].gaugeType);
            }
        } else if (id == PropId::R_BorderStyle && code == CBN_SELCHANGE) {
            InvalidateRect(hComboBorderStyle, nullptr, FALSE);
            LiveUpdateFromPanel();
        } else if ((id == PropId::D_UpdateRate || id == PropId::D_Av) && code == CBN_SELCHANGE) {
            if (id == PropId::D_UpdateRate) InvalidateRect(hComboUpdate, nullptr, FALSE);
            else InvalidateRect(hComboAv, nullptr, FALSE);
            LiveUpdateFromPanel();
        } else if (id == PropId::R_Alpha && (code == EN_CHANGE || code == EN_KILLFOCUS)) {
            // Keep slider aligned when user types a transparency value
            if (!suppressLiveUpdate) {
                try {
                    int a = std::stoi(GetTextA(hEditAlpha));
                    if (a < 0) a = 0;
                    if (a > 255) a = 255;
                    if (hAlphaSlider)
                        SendMessage(hAlphaSlider, TBM_SETPOS, TRUE, a);
                } catch (...) {}
                LiveUpdateFromPanel();
            }
        } else if ((id >= PropId::R_Label && id <= PropId::R_Ncl) ||
                   (id >= PropId::D_Channel && id <= PropId::D_Ct)) {
            if (code == EN_CHANGE || code == EN_KILLFOCUS)
                LiveUpdateFromPanel();
        } else if ((id >= PropId::R_NumericInd && id <= PropId::R_Orient) ||
                   (id >= PropId::D_Bm && id <= PropId::D_Wfr)) {
            if (code == BN_CLICKED)
                LiveUpdateFromPanel();
        }
        break;
    }
    case WM_HSCROLL: {
        HWND hScroll = reinterpret_cast<HWND>(lParam);
        // Transparency trackbar — update edit + model live
        if (hScroll == hAlphaSlider) {
            if (!suppressLiveUpdate) {
                int pos = static_cast<int>(SendMessage(hAlphaSlider, TBM_GETPOS, 0, 0));
                suppressLiveUpdate = true;
                SetTextA(hEditAlpha, std::to_string(pos));
                suppressLiveUpdate = false;
                LiveUpdateFromPanel();
            }
            return 0;
        }
        // Preview value: minValue (left) ... maxValue (right); not saved to INI
        if (hScroll == hPreviewSlider) {
            if (!suppressLiveUpdate) {
                ApplyPreviewSliderToRenderer();
                InvalidatePreview();
            }
            return 0;
        }
        break;
    }
    case WM_CTLCOLORSTATIC: {
        // Color the swatch statics with the Holley RGB from the selected object.
        HDC hdcStatic = reinterpret_cast<HDC>(wParam);
        HWND hCtrl = reinterpret_cast<HWND>(lParam);

        auto paint = [&](unsigned int c) -> LRESULT {
            BYTE r = (c >> 16) & 0xFF;
            BYTE gre = (c >> 8) & 0xFF;
            BYTE b = c & 0xFF;
            SetBkColor(hdcStatic, RGB(r, gre, b));
            // Cache brushes per HWND slot — recreate each paint is fine for few swatches
            static HBRUSH brushes[16] = {};
            static HWND owners[16] = {};
            int slot = -1;
            for (int i = 0; i < 16; ++i) {
                if (owners[i] == hCtrl) { slot = i; break; }
                if (owners[i] == nullptr && slot < 0) slot = i;
            }
            if (slot < 0) slot = 0;
            owners[slot] = hCtrl;
            if (brushes[slot]) DeleteObject(brushes[slot]);
            brushes[slot] = CreateSolidBrush(RGB(r, gre, b));
            return reinterpret_cast<LRESULT>(brushes[slot]);
        };

        const auto& gauges = doc_.Gauges();
        const auto& outputs = doc_.Outputs();
        const auto& labels = doc_.Labels();

        if (hCtrl == hFgSwatch) {
            unsigned int c = 0xFF808080;
            if (selectedGauge >= 0 && selectedGauge < static_cast<int>(gauges.size()))
                c = gauges[selectedGauge].foreground;
            else if (selectedLabel >= 0 && selectedLabel < static_cast<int>(labels.size()))
                c = labels[selectedLabel].foreground;
            return paint(c);
        }
        if (hCtrl == hBgSwatch) {
            unsigned int c = 0xFF808080;
            if (selectedGauge >= 0 && selectedGauge < static_cast<int>(gauges.size()))
                c = gauges[selectedGauge].background;
            else if (selectedLabel >= 0 && selectedLabel < static_cast<int>(labels.size()))
                c = labels[selectedLabel].background;
            return paint(c);
        }
        for (int z = 0; z < 5; ++z) {
            if (hCtrl == hZoneSwatch[z]) {
                unsigned int c = 0xFF808080;
                if (selectedGauge >= 0 && selectedGauge < static_cast<int>(gauges.size())) {
                    const Gauge& gg = gauges[selectedGauge];
                    switch (z) {
                    case 0: c = gg.c0; break;
                    case 1: c = gg.c1; break;
                    case 2: c = gg.c2; break;
                    case 3: c = gg.c3; break;
                    case 4: c = gg.c4; break;
                    }
                }
                return paint(c);
            }
        }
        if (hCtrl == hOutInactSwatch) {
            unsigned int c = 0xFF808080;
            if (selectedOutput >= 0 && selectedOutput < static_cast<int>(outputs.size()))
                c = outputs[selectedOutput].inactiveColor;
            return paint(c);
        }
        if (hCtrl == hOutActSwatch) {
            unsigned int c = 0xFF808080;
            if (selectedOutput >= 0 && selectedOutput < static_cast<int>(outputs.size()))
                c = outputs[selectedOutput].activeColor;
            return paint(c);
        }
        SetTextColor(hdcStatic, RGB(220, 220, 220));
        SetBkColor(hdcStatic, RGB(14, 14, 14));
        return reinterpret_cast<LRESULT>(hBrBg_ ? hBrBg_ : GetStockObject(BLACK_BRUSH));
    }
    case WM_CTLCOLOREDIT: {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        HWND hCtrl = reinterpret_cast<HWND>(lParam);
        const bool enabled = !hCtrl || IsWindowEnabled(hCtrl);
        SetTextColor(hdc, enabled ? RGB(232, 232, 232) : RGB(110, 110, 110));
        SetBkColor(hdc, kFieldFill);
        return reinterpret_cast<LRESULT>(hBrField_ ? hBrField_ : GetStockObject(BLACK_BRUSH));
    }
    case WM_CTLCOLORLISTBOX: {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        SetTextColor(hdc, RGB(232, 232, 232));
        SetBkColor(hdc, kFieldFill);
        return reinterpret_cast<LRESULT>(hBrField_ ? hBrField_ : GetStockObject(BLACK_BRUSH));
    }
    case WM_CTLCOLORBTN: {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        SetTextColor(hdc, RGB(232, 232, 232));
        SetBkColor(hdc, RGB(14, 14, 14));
        return reinterpret_cast<LRESULT>(hBrBg_ ? hBrBg_ : GetStockObject(BLACK_BRUSH));
    }
    case WM_MEASUREITEM: {
        auto* mi = reinterpret_cast<MEASUREITEMSTRUCT*>(lParam);
        if (mi && mi->CtlType == ODT_COMBOBOX)
            mi->itemHeight = 22;
        return TRUE;
    }
    case WM_DRAWITEM: {
        auto* di = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
        if (!di) break;
        if (di->CtlType == ODT_COMBOBOX) {
            if (di->itemID == static_cast<UINT>(-1)) return TRUE;
            const bool sel = (di->itemState & ODS_SELECTED) != 0;
            HBRUSH br = CreateSolidBrush(sel ? RGB(50, 22, 24) : RGB(18, 18, 18));
            FillRect(di->hDC, &di->rcItem, br);
            DeleteObject(br);
            wchar_t text[128] = {};
            SendMessageW(di->hwndItem, CB_GETLBTEXT, di->itemID, (LPARAM)text);
            SetBkMode(di->hDC, TRANSPARENT);
            SetTextColor(di->hDC, RGB(232, 232, 232));
            RECT tr = di->rcItem;
            tr.left += 8;
            DrawTextW(di->hDC, text, -1, &tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
            return TRUE;
        }
        if (di->CtlType != ODT_BUTTON) break;
        const bool primary = (di->CtlID == PropId::LoadLayout);
        const bool pressed = (di->itemState & ODS_SELECTED) != 0;
        RECT rc = di->rcItem;
        HBRUSH br = primary ? hBrRed_ : hBrBar_;
        if (!br) br = (HBRUSH)GetStockObject(DKGRAY_BRUSH);
        FillRect(di->hDC, &rc, hBrBg_ ? hBrBg_ : reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
        HGDIOBJ oldBr = SelectObject(di->hDC, br);
        HGDIOBJ oldPen = SelectObject(di->hDC, GetStockObject(NULL_PEN));
        RoundRect(di->hDC, rc.left, rc.top, rc.right, rc.bottom, kCorner, kCorner);
        SelectObject(di->hDC, oldPen);
        SelectObject(di->hDC, oldBr);
        wchar_t text[64] = {};
        GetWindowTextW(di->hwndItem, text, 64);
        SetBkMode(di->hDC, TRANSPARENT);
        const bool disabled = (di->itemState & ODS_DISABLED) != 0;
        SetTextColor(di->hDC, disabled ? RGB(110, 110, 110) : RGB(255, 255, 255));
        if (pressed) OffsetRect(&rc, 1, 1);
        DrawTextW(di->hDC, text, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        return TRUE;
    }
    case WM_NOTIFY: {
        LPNMHDR hdr = reinterpret_cast<LPNMHDR>(lParam);
        if (hdr->code == UDN_DELTAPOS) {
            // Spin control clicked — apply after the buddy edit text updates.
            PostMessage(hWnd, WM_APP + 1, 0, 0);
        }
        break;
    }
    case WM_APP + 1:
        // Deferred live update from UDN_DELTAPOS (see WM_NOTIFY).
        LiveUpdateFromPanel();
        return 0;
    case WM_LBUTTONDOWN: {
        // Selection only changes from clicks on the dash preview.
        // Inspector / chrome / disabled controls never clear the current gauge.
        // Empty preview (no widget hit box) is the only way to have nothing selected.
        // One widget at a time. A click that misses the current selection
        // replaces it with the topmost hit. A click on the current selection
        // drags it, even when a later gauge is painted over it.
        int mx = static_cast<short>(LOWORD(lParam));
        int my = static_cast<short>(HIWORD(lParam));

        const int px = PreviewX();
        const int py = PreviewY();
        const int cw = PreviewW();
        const int ch = PreviewH();
        if (mx < px || my < py || mx >= px + cw || my >= py + ch)
            break;

        const LayoutScreen* screen = doc_.ActiveScreen();
        if (!screen) {
            ClearSelection();
            break;
        }

        renderer_.originX = px;
        renderer_.originY = py;

        std::vector<PreviewHit> stack;
        renderer_.HitTestStack(*screen, mx, my, cw, ch, stack);
        const PreviewHit* held = nullptr;
        for (const auto& hit : stack) {
            if (PreviewHitIsSelected(hit)) {
                held = &hit;
                break;
            }
        }
        if (stack.empty()) {
            if (selectedGauge >= 0 || selectedOutput >= 0 || selectedLabel >= 0) {
                ClearSelection();
                Log("Selection cleared.");
            }
        } else if (held) {
            // Flyout may have selected a gauge under the front one. Drag that
            // one. A click that misses the current selection still takes the top.
            ArmPreviewDrag(*held, mx, my);
        } else {
            ApplyPreviewHit(stack[0]);
        }
        break;
    }
    case WM_RBUTTONDOWN: {
        int mx = static_cast<short>(LOWORD(lParam));
        int my = static_cast<short>(HIWORD(lParam));
        const int px = PreviewX();
        const int py = PreviewY();
        const int cw = PreviewW();
        const int ch = PreviewH();
        if (mx < px || my < py || mx >= px + cw || my >= py + ch)
            break;
        const LayoutScreen* screen = doc_.ActiveScreen();
        if (!screen) break;
        renderer_.originX = px;
        renderer_.originY = py;
        std::vector<PreviewHit> stack;
        renderer_.HitTestStack(*screen, mx, my, cw, ch, stack);
        if (stack.size() >= 2)
            ShowOverlapMenu(stack, mx, my);
        else if (stack.size() == 1)
            ApplyPreviewHit(stack[0]);
        break;
    }
    case WM_MOUSEMOVE: {
        if (!dragging_) break;
        int mx = static_cast<short>(LOWORD(lParam));
        int my = static_cast<short>(HIWORD(lParam));
        int dx = mx - dragStartMx_;
        int dy = my - dragStartMy_;
        if (!dragMoved_ && dx * dx + dy * dy < kDragSlopPx * kDragSlopPx)
            break;
        int nx = dragOriginX_ + dx;
        int ny = dragOriginY_ + dy;

        if (selectedGauge >= 0) {
            auto& gauges = doc_.Gauges();
            if (selectedGauge < static_cast<int>(gauges.size())) {
                if (gauges[selectedGauge].x != nx || gauges[selectedGauge].y != ny) {
                    gauges[selectedGauge].x = nx;
                    gauges[selectedGauge].y = ny;
                    dragMoved_ = true;
                    SyncPositionEditsFromModel();
                    InvalidatePreview();
                }
            }
        } else if (selectedLabel >= 0) {
            auto& labels = doc_.Labels();
            if (selectedLabel < static_cast<int>(labels.size())) {
                if (labels[selectedLabel].x != nx || labels[selectedLabel].y != ny) {
                    labels[selectedLabel].x = nx;
                    labels[selectedLabel].y = ny;
                    dragMoved_ = true;
                    SyncPositionEditsFromModel();
                    InvalidatePreview();
                }
            }
        } else if (selectedOutput >= 0) {
            auto& outputs = doc_.Outputs();
            if (selectedOutput < static_cast<int>(outputs.size())) {
                if (outputs[selectedOutput].x != nx || outputs[selectedOutput].y != ny) {
                    outputs[selectedOutput].x = nx;
                    outputs[selectedOutput].y = ny;
                    dragMoved_ = true;
                    SyncPositionEditsFromModel();
                    InvalidatePreview();
                }
            }
        }
        break;
    }
    case WM_LBUTTONUP: {
        if (dragging_) EndDrag(true);
        break;
    }
    case WM_CAPTURECHANGED: {
        // Lost capture without LBUTTONUP (alt-tab, etc.)
        if (dragging_ && reinterpret_cast<HWND>(lParam) != hwnd)
            EndDrag(true);
        break;
    }
    case WM_CLOSE: {
        if (!ConfirmDiscardOrSave()) return 0;
        DestroyWindow(hwnd);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT: {
        // Chrome + wells on the parent; preview is double-buffered and blitted
        // only into the canvas rect so inspector text is never overwritten.
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT client;
        GetClientRect(hwnd, &client);

        const int pw = PreviewW();
        const int ph = PreviewH();
        const RECT preview = { PreviewX(), PreviewY(), PreviewX() + pw, PreviewY() + ph };
        // 1px frame lives in the backbuffer with the canvas (slider invalidate
        // used to fill these pixels with chrome, then stroke them on the window DC).
        const RECT framed = { preview.left - 1, preview.top - 1,
            preview.right + 1, preview.bottom + 1 };
        const int bw = pw + 2;
        const int bh = ph + 2;

        // Fill chrome only where it actually needs paint (slider ticks skip this).
        HRGN rgnPaint = CreateRectRgnIndirect(&ps.rcPaint);
        HRGN rgnClient = CreateRectRgnIndirect(&client);
        HRGN rgnFramed = CreateRectRgnIndirect(&framed);
        CombineRgn(rgnClient, rgnClient, rgnFramed, RGN_DIFF);
        CombineRgn(rgnClient, rgnClient, rgnPaint, RGN_AND);
        SelectClipRgn(hdc, rgnClient);
        FillRect(hdc, &client, hBrBg_ ? hBrBg_
            : reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
        RECT bar = { 0, 0, client.right, kStatusY };
        FillRect(hdc, &bar, hBrBar_ ? hBrBar_ : hBrBg_);
        SelectClipRgn(hdc, nullptr);
        DeleteObject(rgnPaint);
        DeleteObject(rgnClient);
        DeleteObject(rgnFramed);

        RECT hit = {};
        if (IntersectRect(&hit, &ps.rcPaint, &framed) && EnsurePreviewCache(hdc, bw, bh)) {
            HDC memDC = previewMemDC_;
            RECT memAll = { 0, 0, bw, bh };
            FillRect(memDC, &memAll, hBrBg_ ? hBrBg_
                : reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
            {
                HPEN pen = CreatePen(PS_SOLID, 1, RGB(88, 88, 92));
                HGDIOBJ oldPen = SelectObject(memDC, pen);
                HGDIOBJ oldBr = SelectObject(memDC, GetStockObject(NULL_BRUSH));
                Rectangle(memDC, 0, 0, bw, bh);
                SelectObject(memDC, oldPen);
                SelectObject(memDC, oldBr);
                DeleteObject(pen);
            }

            renderer_.originX = 1;
            renderer_.originY = 1;
            renderer_.selectedGauge = selectedGauge;
            renderer_.selectedOutput = selectedOutput;
            renderer_.selectedLabel = selectedLabel;

            LayoutScreen emptyScreen;
            const LayoutScreen* screen = doc_.ActiveScreen();
            renderer_.Draw(memDC, screen ? *screen : emptyScreen, backgroundImage, pw, ph);

            BitBlt(hdc, framed.left, framed.top, bw, bh, memDC, 0, 0, SRCCOPY);
        }
        renderer_.originX = PreviewX();
        renderer_.originY = PreviewY();

        PaintFieldWells(hwnd, hdc);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_DESTROY:
        EndDrag(false);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
