// =============================================================================
// EditorWindow_Props.cpp — Inspector + More settings + property sync
// =============================================================================

#include "EditorWindow.h"
#include "EditorInternals.h"
#include "SymbolPicker.h"
#include "ChannelPicker.h"
#include "../Io/IniUtils.h"
#include "../Model/GaugeType.h"
#include <commdlg.h>
#include <commctrl.h>
#include <sstream>
#include <algorithm>
#include <uxtheme.h>
#include <gdiplus.h>

#pragma comment(lib, "gdiplus.lib")

using namespace Ed;

namespace {

bool EditHasHighByte(const std::string& s) {
    for (unsigned char c : s) {
        if (c >= 0x80)
            return true;
    }
    return false;
}

// Alt-code / ACP glyphs in the ANSI box → Holley \x form. Do not rewrite
// ASCII (including in-progress "\xb0") or Encode will turn '\' into \x5c.
void NormalizeEditIfHigh(HWND h, const std::string& utf8, const std::string& raw,
    bool& suppress) {
    if (!h || !EditHasHighByte(raw))
        return;
    const bool prev = suppress;
    suppress = true;
    SetTextA(h, IniUtils::Utf8ToEditText(utf8));
    suppress = prev;
}

void FillModeCombo(HWND combo, const GaugeModeChoice* items, int count, int current) {
    if (!combo) return;
    SendMessage(combo, CB_RESETCONTENT, 0, 0);
    int sel = -1;
    for (int i = 0; i < count; ++i) {
        int idx = static_cast<int>(SendMessageW(combo, CB_ADDSTRING, 0,
            reinterpret_cast<LPARAM>(items[i].label)));
        SendMessage(combo, CB_SETITEMDATA, idx, items[i].value);
        if (items[i].value == current) sel = idx;
    }
    if (sel < 0) {
        wchar_t buf[40] = {};
        swprintf_s(buf, L"%d", current);
        int idx = static_cast<int>(SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)buf));
        SendMessage(combo, CB_SETITEMDATA, idx, current);
        sel = idx;
    }
    SendMessage(combo, CB_SETCURSEL, sel, 0);
}

int ComboData(HWND combo, int fallback) {
    if (!combo) return fallback;
    int sel = static_cast<int>(SendMessage(combo, CB_GETCURSEL, 0, 0));
    if (sel < 0) return fallback;
    LRESULT data = SendMessage(combo, CB_GETITEMDATA, sel, 0);
    if (data == CB_ERR) return fallback;
    return static_cast<int>(data);
}

} // namespace

void EditorWindow::RebuildGaugeTypeCombo() {
    if (!hComboGaugeType) return;
    // CB_RESETCONTENT can notify CBN_SELCHANGE — only call while suppressLiveUpdate.
    SendMessage(hComboGaugeType, CB_RESETCONTENT, 0, 0);
    for (const auto& t : kGaugeTypes) {
        int idx = static_cast<int>(SendMessageW(hComboGaugeType, CB_ADDSTRING, 0, (LPARAM)t.name));
        SendMessage(hComboGaugeType, CB_SETITEMDATA, idx, t.id);
    }
}

// Dense inspector under the preview. Fixed slots; type enables / disables / relabels.
void EditorWindow::CreateAppearancePanel() {
    // Geometry is Insp::* in InspectorLayout.h — do not hard-code x/y/w here.
    auto cap = [&](const Insp::Ctrl& c, HWND& lab, const wchar_t* name, bool right) {
        if (c.labW <= 0) return;
        lab = MakeLabel(c.x + c.labDx, c.y + c.labDy, c.labW, Insp::kLabH, name, right);
    };
    auto edit = [&](const Insp::Ctrl& c, HWND& lab, HWND& ed, const wchar_t* name, int id,
                    bool number, bool buddy, int lo, int hi) {
        cap(c, lab, name, true);
        if (buddy)
            ed = MakeBuddyEdit(c.x, c.y, c.w, c.h, id, lo, hi, number);
        else
            ed = MakeEdit(c.x, c.y, c.w, c.h, id, number);
    };
    auto chk = [&](const Insp::Ctrl& c, HWND& box, int id, const wchar_t* name) {
        box = CreateWindowW(L"BUTTON", L"",
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            c.x, c.y, c.w, c.h, hWnd, (HMENU)(INT_PTR)id, GetModuleHandle(nullptr), nullptr);
        if (box) SetWindowTheme(box, L"", L"");
        HWND lab = nullptr;
        cap(c, lab, name, false);
        if (lab) checkCaps_.push_back({ box, lab });
    };
    auto slider = [&](const Insp::Ctrl& c, HWND& lab, HWND& bar, const wchar_t* name, int id) {
        cap(c, lab, name, true);
        bar = CreateWindowExW(0, TRACKBAR_CLASS, L"",
            WS_CHILD | WS_VISIBLE | TBS_HORZ,
            c.x, c.y, c.w, c.h, hWnd, (HMENU)(INT_PTR)id, GetModuleHandle(nullptr), nullptr);
    };
    auto chip = [&](const Insp::Ctrl& sw, const Insp::Ctrl& btn, int swId, int btnId,
                    HWND& hsw, HWND& hbtn) {
        hsw = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_NOTIFY | WS_BORDER,
            sw.x, sw.y, sw.w, sw.h, hWnd, (HMENU)(INT_PTR)swId, GetModuleHandle(nullptr), nullptr);
        hbtn = CreateWindowW(L"BUTTON", L"...", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            btn.x, btn.y, btn.w, btn.h, hWnd, (HMENU)(INT_PTR)btnId, GetModuleHandle(nullptr), nullptr);
        ApplyUiFont(hbtn);
    };

    edit(Insp::Label, hLabLabel, hEditLabel, L"Label", PropId::R_Label, false, false, 0, 0);
    hBtnSymbol = CreateWindowW(L"BUTTON", L"Sym",
        WS_CHILD | BS_OWNERDRAW,
        Insp::SymbolBtn.x, Insp::SymbolBtn.y, Insp::SymbolBtn.w, Insp::SymbolBtn.h,
        hWnd, (HMENU)(INT_PTR)PropId::R_SymbolPick, GetModuleHandle(nullptr), nullptr);
    ApplyUiFont(hBtnSymbol);
    edit(Insp::Units, hLabUnits, hEditUnits, L"Units", PropId::R_Units, false, false, 0, 0);
    cap(Insp::Type, hLabGaugeType, L"Type", true);
    hComboGaugeType = MakeCombo(Insp::Type.x, Insp::Type.y, Insp::Type.w, Insp::TypeDropH,
        PropId::R_GaugeType);
    RebuildGaugeTypeCombo();
    edit(Insp::FontSize, hLabFont, hEditFont, L"Font size", PropId::R_FontSize, false, true, -1, 240);
    edit(Insp::Precision, hLabPrec, hEditPrec, L"Precision", PropId::R_Precision, true, true, 0, 8);
    edit(Insp::Min, hLabMin, hEditMin, L"Min", PropId::R_MinValue, false, true, -1000000, 1000000);
    edit(Insp::Max, hLabMax, hEditMax, L"Max", PropId::R_MaxValue, false, true, -1000000, 1000000);

    edit(Insp::X, hLabX, hEditX, L"X", PropId::R_X, false, true, -2000, 4000);
    edit(Insp::Y, hLabY, hEditY, L"Y", PropId::R_Y, false, true, -2000, 4000);
    edit(Insp::Size, hLabW, hEditW, L"Size", PropId::R_Width, true, true, 1, 4000);
    hLabWWidth = MakeLabel(Insp::Size.x + Insp::Size.labDx, Insp::Size.y + Insp::Size.labDy,
        Insp::Size.labW, Insp::kLabH, L"Width", true);
    ShowWindow(hLabWWidth, SW_HIDE);
    edit(Insp::Height, hLabH, hEditH, L"Height", PropId::R_Height, true, true, 1, 4000);
    chk(Insp::Vertical, hChkOrient, PropId::R_Orient, L"Vertical");

    edit(Insp::StartAngle, hLabStart, hEditStart, L"Start angle", PropId::R_StartAngle, false, true, -360, 360);
    edit(Insp::EndAngle, hLabEnd, hEditEnd, L"End angle", PropId::R_EndAngle, false, true, -360, 360);
    edit(Insp::Segments, hLabSteps, hEditSteps, L"Segments", PropId::R_Steps, true, true, 0, 200);
    edit(Insp::MinTicks, hLabMtics, hEditMtics, L"Min ticks", PropId::R_Mtics, true, true, 0, 50);
    edit(Insp::ScaleDiv, hLabScaleDiv, hEditScaleDiv, L"Scale div", PropId::R_ScaleDiv, false, false, 0, 0);
    edit(Insp::NormLeds, hLabNnl, hEditNnl, L"Norm LEDs", PropId::R_Nnl, true, true, 0, 100);
    edit(Insp::CautLeds, hLabNcl, hEditNcl, L"Caut LEDs", PropId::R_Ncl, true, true, 0, 100);

    chk(Insp::DispLabel, hChkDispLabel, PropId::R_DispLabel, L"Display label");
    chk(Insp::DispUnits, hChkDispUnits, PropId::R_DispUnits, L"Display units");
    chk(Insp::ValueDisp, hChkNumeric, PropId::R_NumericInd, L"Value display");
    chk(Insp::ScaleVal, hChkScaleVal, PropId::R_ScaleValues, L"Scale values");
    chk(Insp::Border, hChkBorder, PropId::R_Border, L"Border");

    cap(Insp::BorderStyle, hLabBorderStyle, L"Style", true);
    hComboBorderStyle = MakeCombo(Insp::BorderStyle.x, Insp::BorderStyle.y, Insp::BorderStyle.w,
        Insp::BorderStyleDropH, PropId::R_BorderStyle);
    SendMessageW(hComboBorderStyle, CB_ADDSTRING, 0, (LPARAM)L"Original");
    SendMessageW(hComboBorderStyle, CB_ADDSTRING, 0, (LPARAM)L"Black");
    SendMessageW(hComboBorderStyle, CB_ADDSTRING, 0, (LPARAM)L"Grey");
    SendMessageW(hComboBorderStyle, CB_ADDSTRING, 0, (LPARAM)L"Silver");
    for (int i = 0; i < 4; ++i)
        SendMessage(hComboBorderStyle, CB_SETITEMDATA, i, i);
    SendMessage(hComboBorderStyle, CB_SETCURSEL, 0, 0);

    slider(Insp::Preview, hLabPreview, hPreviewSlider, L"Preview", PropId::R_PreviewSlider);
    SendMessage(hPreviewSlider, TBM_SETRANGE, TRUE, MAKELPARAM(0, 1000));
    SendMessage(hPreviewSlider, TBM_SETPOS, TRUE, 400);

    cap(Insp::Fg, hLabFg, L"Fore / back", true);
    chip(Insp::Fg, Insp::FgBtn, PropId::R_FgSwatch, PropId::R_FgBtn, hFgSwatch, hFgBtn);
    chip(Insp::Bg, Insp::BgBtn, PropId::R_BgSwatch, PropId::R_BgBtn, hBgSwatch, hBgBtn);

    slider(Insp::Alpha, hLabAlpha, hAlphaSlider, L"Transp", PropId::R_AlphaSlider);
    SendMessage(hAlphaSlider, TBM_SETRANGE, TRUE, MAKELPARAM(0, 255));
    hEditAlpha = MakeBuddyEdit(Insp::AlphaEdit.x, Insp::AlphaEdit.y, Insp::AlphaEdit.w,
        Insp::AlphaEdit.h, PropId::R_Alpha, 0, 255, true);

    cap(Insp::OutInact, hLabOutInact, L"Inactive", true);
    chip(Insp::OutInact, Insp::OutInactBtn, PropId::R_OutInactSwatch, PropId::R_OutInactBtn,
        hOutInactSwatch, hOutInactBtn);
    cap(Insp::OutAct, hLabOutAct, L"Active", true);
    chip(Insp::OutAct, Insp::OutActBtn, PropId::R_OutActSwatch, PropId::R_OutActBtn,
        hOutActSwatch, hOutActBtn);
    ShowCtrl(hLabOutInact, false);
    ShowCtrl(hOutInactSwatch, false);
    ShowCtrl(hOutInactBtn, false);
    ShowCtrl(hLabOutAct, false);
    ShowCtrl(hOutActSwatch, false);
    ShowCtrl(hOutActBtn, false);

    chk(Insp::LowZones, hChkMinThresh, PropId::R_MinThresh, L"Low zones");
    chk(Insp::HighZones, hChkMaxThresh, PropId::R_MaxThresh, L"High zones");

    const Insp::ZoneSlot* zones[5] = {
        &Insp::LowAlarm, &Insp::LowWarning, &Insp::Normal, &Insp::HighWarning, &Insp::HighAlarm
    };
    const wchar_t* znames[5] = { L"Low alarm", L"Low warning", L"Normal", L"High warning", L"High alarm" };
    const int zids[5] = { PropId::R_MinCaution, PropId::R_MinNormal, 0, PropId::R_MaxNormal, PropId::R_MaxCaution };
    HWND* zlabs[5] = { &hLabMinCaut, &hLabMinNorm, &hLabZone[2], &hLabMaxNorm, &hLabMaxCaut };
    HWND* zedits[5] = { &hEditMinCaut, &hEditMinNorm, nullptr, &hEditMaxNorm, &hEditMaxCaut };

    for (int i = 0; i < 5; ++i) {
        const Insp::ZoneSlot& z = *zones[i];
        cap(z.value, *zlabs[i], znames[i], false);
        if (z.value.w > 0 && zedits[i])
            *zedits[i] = MakeEdit(z.value.x, z.value.y, z.value.w, z.value.h, zids[i], false);
        chip(z.chip, z.btn, PropId::R_C0Swatch + i, PropId::R_C0Btn + i, hZoneSwatch[i], hZoneBtn[i]);
    }
    hLabZone[0] = hLabMinCaut;
    hLabZone[1] = hLabMinNorm;
    hLabZone[3] = hLabMaxNorm;
    hLabZone[4] = hLabMaxCaut;
}

LRESULT CALLBACK EditorWindow::MoreSettingsProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    auto* self = reinterpret_cast<EditorWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (msg == WM_CREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
        return 0;
    }
    if (!self) return DefWindowProc(hwnd, msg, wParam, lParam);
    if (msg == WM_CLOSE || (msg == WM_COMMAND && LOWORD(wParam) == IDOK)) {
        self->CloseMoreSettings();
        return 0;
    }
    if (msg == WM_COMMAND) {
        SendMessage(self->hWnd, WM_COMMAND, wParam, lParam);
        return 0;
    }
    if (msg == WM_DRAWITEM || msg == WM_MEASUREITEM || msg == WM_CTLCOLORLISTBOX)
        return self->HandleMessage(self->hWnd, msg, wParam, lParam);
    if (msg == WM_CTLCOLORSTATIC || msg == WM_CTLCOLOREDIT)
        return self->HandleMessage(self->hWnd, msg, wParam, lParam);
    if (msg == WM_ERASEBKGND)
        return 1;
    if (msg == WM_PAINT) {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc = {};
        GetClientRect(hwnd, &rc);
        FillRect(hdc, &rc, self->hBrBg_ ? self->hBrBg_
            : reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
        self->PaintFieldWells(hwnd, hdc);
        EndPaint(hwnd, &ps);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void EditorWindow::CloseMoreSettings() {
    if (hMoreWnd) ShowWindow(hMoreWnd, SW_HIDE);
    if (hWnd) {
        EnableWindow(hWnd, TRUE);
        SetForegroundWindow(hWnd);
    }
}

void EditorWindow::CreateMoreSettingsHost() {
    WNDCLASSW wc = {};
    wc.lpfnWndProc = EditorWindow::MoreSettingsProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = L"HolleyMoreSettings";
    wc.hbrBackground = hBrBg_ ? hBrBg_ : (HBRUSH)(COLOR_BTNFACE + 1);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassW(&wc);

    hMoreWnd = CreateWindowExW(WS_EX_DLGMODALFRAME, L"HolleyMoreSettings", L"More settings",
        WS_CAPTION | WS_SYSMENU | WS_POPUP | WS_CLIPCHILDREN,
        200, 200, 440, 476, hWnd, nullptr, GetModuleHandle(nullptr), this);

    auto lab = [&](int x, int y, int w, const wchar_t* t) {
        HWND h = CreateWindowW(L"STATIC", t, WS_CHILD | WS_VISIBLE | SS_NOPREFIX | SS_RIGHT,
            x, y, w, 16, hMoreWnd, nullptr, GetModuleHandle(nullptr), nullptr);
        ApplyUiFont(h);
        FitLabelToText(h);
        return h;
    };
    auto ed = [&](int x, int y, int w, int id, bool num) {
        DWORD st = WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL;
        if (num) st |= ES_NUMBER;
        HWND h = CreateWindowW(L"EDIT", L"", st,
            x + kWellInset, y + kWellInset, w - 2 * kWellInset, 22 - 2 * kWellInset,
            hMoreWnd, (HMENU)(INT_PTR)id, GetModuleHandle(nullptr), nullptr);
        ApplyUiFont(h);
        SetWindowTheme(h, L"", L"");
        StyleRoundedEdit(h);
        RegisterFieldWell(hMoreWnd, h, x, y, w, 22);
        return h;
    };
    auto chk = [&](int x, int y, int w, int id, const wchar_t* t) {
        return MakeCheckOn(hMoreWnd, x, y, w, 18, id, t);
    };

    int y = 16;
    hDataLabChannel = lab(16, y, 120, L"Channel");
    hDataEditChannel = ed(144, y, 200, PropId::D_Channel, true);
    hBtnChannel = CreateWindowW(L"BUTTON", L"Ch",
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        352, y, 32, 22, hMoreWnd, (HMENU)(INT_PTR)PropId::D_ChannelPick,
        GetModuleHandle(nullptr), nullptr);
    ApplyUiFont(hBtnChannel);
    y += 28;
    hDataLabUpdate = lab(16, y, 120, L"Updates / sec");
    hComboUpdate = MakeComboOn(hMoreWnd, 144, y, 240, 260, PropId::D_UpdateRate);
    FillModeCombo(hComboUpdate, kUpdateRateModes, GaugeModeCount(kUpdateRateModes), 6);
    y += 28;
    hDataLabMult2 = lab(16, y, 120, L"Math a (x^2)");
    hDataEditMult2 = ed(144, y, 240, PropId::D_ValueMult2, false); y += 28;
    hDataLabMult = lab(16, y, 120, L"Math b (x)");
    hDataEditMult = ed(144, y, 240, PropId::D_ValueMult, false); y += 28;
    hDataLabMultB = lab(16, y, 120, L"Math c");
    hDataEditMultB = ed(144, y, 240, PropId::D_ValueMultB, false); y += 28;
    hDataLabAv = lab(16, y, 120, L"Averaging");
    hComboAv = MakeComboOn(hMoreWnd, 144, y, 240, 220, PropId::D_Av);
    FillModeCombo(hComboAv, kAveragingModes, GaugeModeCount(kAveragingModes), 0);
    y += 28;
    hDataLabWfr = lab(16, y, 120, L"Warning flash");
    hComboWfr = MakeComboOn(hMoreWnd, 144, y, 240, 240, PropId::D_Wfr);
    FillModeCombo(hComboWfr, kWfrModes, GaugeModeCount(kWfrModes), 0);
    y += 28;
    hChkMinPeak = chk(144, y, 120, PropId::R_MinPeak, L"Min peak");
    hChkMaxPeak = chk(270, y, 120, PropId::R_MaxPeak, L"Max peak"); y += 26;
    hChkPeakRed = chk(144, y, 200, PropId::R_PeakRed, L"Peak reduction"); y += 26;
    hChkHor = chk(144, y, 220, PropId::R_Hor, L"hor"); y += 28;
    hDataChkBm = chk(16, y, 70, PropId::D_Bm, L"bm");
    hDataChkIsLed = chk(90, y, 80, PropId::D_IsLed, L"isLED");
    hDataChkRpm = chk(250, y, 150, PropId::D_RpmMode, L"rpmModeEnabled"); y += 28;
    hDataLabCt = lab(16, y, 120, L"ct");
    hDataEditCt = ed(144, y, 80, PropId::D_Ct, true);

    hDataLabAdvanced = nullptr;
}

void EditorWindow::ShowSymbolPicker() {
    if (propMode_ != PropMode::Gauge) return;
    std::string utf8 = IniUtils::EditTextToUtf8(GetTextA(hEditLabel));
    if (!RunSymbolPicker(hWnd, utf8)) return;
    suppressLiveUpdate = true;
    SetTextA(hEditLabel, IniUtils::Utf8ToEditText(utf8));
    suppressLiveUpdate = false;
    LiveUpdateFromPanel();
}

void EditorWindow::ShowChannelPicker() {
    if (propMode_ != PropMode::Gauge) return;
    int channel = 0;
    try {
        channel = std::stoi(GetTextA(hDataEditChannel));
    } catch (...) {
        channel = 0;
    }
    HWND owner = (hMoreWnd && IsWindowVisible(hMoreWnd)) ? hMoreWnd : hWnd;
    if (!RunChannelPicker(owner, channel)) return;
    suppressLiveUpdate = true;
    SetTextA(hDataEditChannel, std::to_string(channel));
    suppressLiveUpdate = false;
    LiveUpdateFromPanel();
}

void EditorWindow::ShowMoreSettings() {
    if (!hMoreWnd) return;
    if (propMode_ == PropMode::None) {
        MessageBoxW(hWnd, L"Select a gauge or switch first.", L"More settings", MB_OK | MB_ICONINFORMATION);
        return;
    }
    RECT pr = {};
    GetWindowRect(hWnd, &pr);
    RECT mr = {};
    GetWindowRect(hMoreWnd, &mr);
    int mw = mr.right - mr.left, mh = mr.bottom - mr.top;
    SetWindowPos(hMoreWnd, HWND_TOP,
        pr.left + ((pr.right - pr.left) - mw) / 2,
        pr.top + ((pr.bottom - pr.top) - mh) / 2,
        0, 0, SWP_NOSIZE);
    EnableWindow(hWnd, FALSE);
    ShowWindow(hMoreWnd, SW_SHOW);
}


void EditorWindow::SetPropMode(PropMode mode) {
    propMode_ = mode;
    ApplyPropTabVisibility();
}

void EditorWindow::ApplyPropTabVisibility() {
    const bool g = (propMode_ == PropMode::Gauge);
    const bool o = (propMode_ == PropMode::Output);
    const bool l = (propMode_ == PropMode::Label);
    const bool any = g || o || l;

    if (g) SetLabelText(hLabLabel, L"Label");
    else if (o || l) SetLabelText(hLabLabel, L"Text");
    if (g) SetLabelText(hLabGaugeType, L"Type");
    else if (o) SetLabelText(hLabGaugeType, L"Style");

    EnablePair(hLabLabel, hEditLabel, any);
    EnablePair(hLabUnits, hEditUnits, g);
    EnablePair(hLabX, hEditX, any);
    EnablePair(hLabY, hEditY, any);
    EnablePair(hLabW, hEditW, g);
    EnablePair(hLabH, hEditH, g);
    EnableCtrl(hComboGaugeType, g || o);
    EnablePair(hLabFont, hEditFont, any);

    auto en = [&](HWND lab, HWND ed, bool on) { EnablePair(lab, ed, on); };
    en(hLabMin, hEditMin, g);
    en(hLabMax, hEditMax, g);
    en(hLabMinCaut, hEditMinCaut, g);
    en(hLabMaxCaut, hEditMaxCaut, g);
    en(hLabMinNorm, hEditMinNorm, g);
    en(hLabMaxNorm, hEditMaxNorm, g);
    en(hLabStart, hEditStart, g);
    en(hLabEnd, hEditEnd, g);
    en(hLabSteps, hEditSteps, g);
    en(hLabMtics, hEditMtics, g);
    en(hLabScaleDiv, hEditScaleDiv, g);
    en(hLabPrec, hEditPrec, g);
    en(hLabNnl, hEditNnl, g);
    en(hLabNcl, hEditNcl, g);

    EnableCtrl(hAlphaSlider, g || l || o);
    EnableCtrl(hEditAlpha, g || l || o);
    EnableCtrl(hChkNumeric, g);
    EnableCtrl(hChkScaleVal, g);
    EnableCtrl(hChkDispUnits, g);
    EnableCtrl(hChkDispLabel, g);
    EnableCtrl(hChkHor, g);
    EnableCtrl(hChkOrient, g);
    EnableCtrl(hChkBorder, g);
    EnableCtrl(hComboBorderStyle, g);
    EnableCtrl(hChkMinThresh, g);
    EnableCtrl(hChkMaxThresh, g);
    EnableCtrl(hPreviewSlider, true); // preview value is global; no selection required
    EnableCtrl(hFgSwatch, g || l);
    EnableCtrl(hFgBtn, g || l);
    EnableCtrl(hBgSwatch, g || l);
    EnableCtrl(hBgBtn, g || l);

    for (int z = 0; z < 5; ++z) {
        EnableCtrl(hZoneSwatch[z], g);
        EnableCtrl(hZoneBtn[z], g);
    }

    const bool sw = o;
    ShowCtrl(hLabOutInact, sw);
    ShowCtrl(hOutInactSwatch, sw);
    ShowCtrl(hOutInactBtn, sw);
    ShowCtrl(hLabOutAct, sw);
    ShowCtrl(hOutActSwatch, sw);
    ShowCtrl(hOutActBtn, sw);
    ShowCtrl(hLabFg, !sw);
    ShowCtrl(hFgSwatch, !sw);
    ShowCtrl(hFgBtn, !sw);
    ShowCtrl(hBgSwatch, !sw);
    ShowCtrl(hBgBtn, !sw);

    ShowCtrl(hBtnSymbol, false);

    EnableCtrl(hDataEditChannel, g || o);
    EnableCtrl(hBtnChannel, g);
    EnableCtrl(hDataEditMult, g);
    EnableCtrl(hDataEditMult2, g);
    EnableCtrl(hDataEditMultB, g);
    EnableCtrl(hComboUpdate, g);
    EnableCtrl(hComboAv, g);
    EnableCtrl(hDataLabWfr, g);
    EnableCtrl(hComboWfr, g);
    EnableCtrl(hChkMinPeak, g);
    EnableCtrl(hChkMaxPeak, g);
    EnableCtrl(hChkPeakRed, g); // editor-only; not a Customize row
    EnableCtrl(hDataChkBm, g);  // editor-only firmware flags — keep
    EnableCtrl(hDataChkIsLed, g);
    EnableCtrl(hDataChkRpm, g);
    EnableCtrl(hDataEditCt, g);

    if (g && selectedGauge >= 0) {
        const auto& gauges = doc_.Gauges();
        if (selectedGauge < static_cast<int>(gauges.size()))
            ApplyGaugeTypeVisibility(gauges[selectedGauge].gaugeType);
    }

    SyncBuddyUpDownVisibility();
}

// Enable/disable + relabel slots for the current gaugeType. Seats stay put.
// Source: Dash-Customize-Control-Matrix.csv. Editor-only More settings
// (bm / warning-flash / isLED / raw hor / peakReduction / raw ct) stay available.
void EditorWindow::ApplyGaugeTypeVisibility(int gaugeType) {
    if (propMode_ != PropMode::Gauge) return;

    auto has = [gaugeType](CustomizeCap c) { return GaugeCustomizeHas(gaugeType, c); };

    const bool isSymbol = (gaugeType == 8);
    const bool hor = GaugeTypeUsesHor(gaugeType);
    const bool statusLed = GaugeTypeUsesStatusLedSize(gaugeType);
    const bool square = GaugeTypeUsesSquareSize(gaugeType);
    const bool digSize = GaugeTypeUsesDigitalSize(gaugeType);
    const bool barWOnly = GaugeTypeUsesBargraphWidthOnly(gaugeType);
    const bool fixedSize = GaugeTypeUsesFixedSize(gaugeType) || statusLed;
    const bool borderStyle = has(CustomizeCap::BorderStyle);

    EnablePair(hLabLabel, hEditLabel, has(CustomizeCap::Label));
    EnablePair(hLabFont, hEditFont, has(CustomizeCap::FontSize));
    EnablePair(hLabUnits, hEditUnits, has(CustomizeCap::Units));
    EnablePair(hLabPrec, hEditPrec, has(CustomizeCap::Precision));
    EnablePair(hLabMin, hEditMin, has(CustomizeCap::Range));
    EnablePair(hLabMax, hEditMax, has(CustomizeCap::Range));
    EnablePair(hLabStart, hEditStart, has(CustomizeCap::Sweep));
    EnablePair(hLabEnd, hEditEnd, has(CustomizeCap::Sweep));
    EnablePair(hLabMtics, hEditMtics, has(CustomizeCap::Mtics));
    EnablePair(hLabScaleDiv, hEditScaleDiv, has(CustomizeCap::ScalingDivisor));
    EnablePair(hLabSteps, hEditSteps, has(CustomizeCap::Steps));
    EnablePair(hLabNnl, hEditNnl, has(CustomizeCap::RpmLeds));
    EnablePair(hLabNcl, hEditNcl, has(CustomizeCap::RpmLeds));

    EnableCtrl(hChkScaleVal, has(CustomizeCap::ScaleValues));
    EnableCtrl(hChkBorder, has(CustomizeCap::Border));
    EnableCtrl(hChkNumeric, has(CustomizeCap::ValueDisplay));
    EnableCtrl(hChkDispUnits, has(CustomizeCap::DisplayUnits));
    EnableCtrl(hChkDispLabel, has(CustomizeCap::DisplayLabel));

    EnableCtrl(hChkHor, hor); // editor-only raw hor; type 1 only
    EnableCtrl(hChkOrient, hor || statusLed || GaugeTypeUsesStyle2Vertical(gaugeType));
    EnableCtrl(hDataChkRpm, has(CustomizeCap::RpmMode));
    if (hChkOrient) {
        SetCheckCaption(hChkOrient, statusLed ? L"Size = Large" : L"Vertical");
    }
    ShowCtrl(hBtnSymbol, isSymbol);

    if (fixedSize || isSymbol) {
        // Symbol: Width/Height are the hit box, not glyph size (Font Size only).
        ShowSizeOrWidthCaption(hLabW, hLabWWidth, true);
        EnablePair(hLabW, hEditW, false);
        EnablePair(hLabH, hEditH, false);
    } else if (square || digSize) {
        ShowSizeOrWidthCaption(hLabW, hLabWWidth, true);
        EnablePair(hLabW, hEditW, true);
        EnablePair(hLabH, hEditH, false);
    } else if (barWOnly) {
        ShowSizeOrWidthCaption(hLabW, hLabWWidth, false);
        EnablePair(hLabW, hEditW, true);
        EnablePair(hLabH, hEditH, false);
    } else {
        ShowSizeOrWidthCaption(hLabW, hLabWWidth, false);
        EnablePair(hLabW, hEditW, true);
        EnablePair(hLabH, hEditH, true);
    }

    // ct still selects analog geom when Border (crown) is off (H-analog-ct-crown).
    // Do not gate Style on hChkBorder.
    EnableCtrl(hLabBorderStyle, borderStyle);
    EnableCtrl(hComboBorderStyle, borderStyle);
    EnableCtrl(hDataEditCt, !borderStyle);
    EnableCtrl(hComboUpdate, has(CustomizeCap::UpdateRate));
    EnableCtrl(hComboAv, has(CustomizeCap::Averaging));

    const bool math = has(CustomizeCap::ChannelMath);
    EnableCtrl(hDataEditMult, math);
    EnableCtrl(hDataEditMult2, math);
    EnableCtrl(hDataEditMultB, math);

    const bool peaks = has(CustomizeCap::PeakHold);
    EnableCtrl(hChkMinPeak, peaks);
    EnableCtrl(hChkMaxPeak, peaks);

    EnableCtrl(hFgSwatch, has(CustomizeCap::Foreground));
    EnableCtrl(hFgBtn, has(CustomizeCap::Foreground));
    EnableCtrl(hBgSwatch, has(CustomizeCap::Background));
    EnableCtrl(hBgBtn, has(CustomizeCap::Background));
    EnableCtrl(hAlphaSlider, has(CustomizeCap::Background));
    EnableCtrl(hEditAlpha, has(CustomizeCap::Background));

    const bool zoneVals = has(CustomizeCap::ZoneValues);
    EnablePair(hLabMinCaut, hEditMinCaut, zoneVals);
    EnablePair(hLabMaxCaut, hEditMaxCaut, zoneVals);
    EnablePair(hLabMinNorm, hEditMinNorm, zoneVals);
    EnablePair(hLabMaxNorm, hEditMaxNorm, zoneVals);

    const bool lowEn = has(CustomizeCap::LowThreshEnable);
    const bool highEn = has(CustomizeCap::HighThreshEnable);
    EnableCtrl(hChkMinThresh, lowEn);
    EnableCtrl(hChkMaxThresh, highEn);

    // Alarm/caution color pickers follow the matching enable checkbox.
    // Types with no checkbox (RPM high-side) stay on. Normal is never gated.
    const bool lowPick = has(CustomizeCap::LowZoneColors)
        && (!lowEn || GetChecked(hChkMinThresh));
    const bool highPick = has(CustomizeCap::HighZoneColors)
        && (!highEn || GetChecked(hChkMaxThresh));
    const bool normalPick = has(CustomizeCap::NormalZoneColor);
    EnableCtrl(hZoneSwatch[0], lowPick);
    EnableCtrl(hZoneBtn[0], lowPick);
    EnableCtrl(hZoneSwatch[1], lowPick);
    EnableCtrl(hZoneBtn[1], lowPick);
    EnableCtrl(hZoneSwatch[2], normalPick);
    EnableCtrl(hZoneBtn[2], normalPick);
    EnableCtrl(hZoneSwatch[3], highPick);
    EnableCtrl(hZoneBtn[3], highPick);
    EnableCtrl(hZoneSwatch[4], highPick);
    EnableCtrl(hZoneBtn[4], highPick);

    SyncBuddyUpDownVisibility();
}

void EditorWindow::SetTransparencyUi(int alpha) {
    if (alpha < 0) alpha = 0;
    if (alpha > 255) alpha = 255;
    if (hAlphaSlider)
        SendMessage(hAlphaSlider, TBM_SETPOS, TRUE, alpha);
    if (hEditAlpha)
        SetTextA(hEditAlpha, std::to_string(alpha));
}

int EditorWindow::GetTransparencyUi() const {
    if (hAlphaSlider)
        return static_cast<int>(SendMessage(hAlphaSlider, TBM_GETPOS, 0, 0));
    return 255;
}

void EditorWindow::SyncPreviewSliderFromRenderer() {
    if (!hPreviewSlider) return;
    int pos = static_cast<int>(renderer_.demoNorm * 1000.0 + 0.5);
    if (pos < 0) pos = 0;
    if (pos > 1000) pos = 1000;
    suppressLiveUpdate = true;
    SendMessage(hPreviewSlider, TBM_SETPOS, TRUE, pos);
    suppressLiveUpdate = false;
}

void EditorWindow::ApplyPreviewSliderToRenderer() {
    if (!hPreviewSlider) return;
    int pos = static_cast<int>(SendMessage(hPreviewSlider, TBM_GETPOS, 0, 0));
    if (pos < 0) pos = 0;
    if (pos > 1000) pos = 1000;
    renderer_.demoNorm = static_cast<double>(pos) / 1000.0;
}

// Blank all property edits/checks. Uses suppressLiveUpdate so EN_CHANGE is ignored.
void EditorWindow::ClearPropertyPanel() {
    suppressLiveUpdate = true;
    auto clear = [](HWND h) { if (h) SetWindowTextA(h, ""); };
    clear(hEditLabel); clear(hEditUnits);
    clear(hEditX); clear(hEditY); clear(hEditW); clear(hEditH);
    clear(hEditMin); clear(hEditMax);
    clear(hEditMinCaut); clear(hEditMaxCaut);
    clear(hEditMinNorm); clear(hEditMaxNorm);
    clear(hEditStart); clear(hEditEnd);
    clear(hEditFont); clear(hEditSteps); clear(hEditMtics);
    clear(hEditScaleDiv); clear(hEditPrec);
    clear(hEditNnl); clear(hEditNcl); clear(hEditAlpha);
    clear(hDataEditChannel);
    clear(hDataEditMult); clear(hDataEditMult2); clear(hDataEditMultB);
    clear(hDataEditCt);
    FillModeCombo(hComboUpdate, kUpdateRateModes, GaugeModeCount(kUpdateRateModes), 6);
    FillModeCombo(hComboAv, kAveragingModes, GaugeModeCount(kAveragingModes), 0);
    FillModeCombo(hComboWfr, kWfrModes, GaugeModeCount(kWfrModes), 0);

    SetTransparencyUi(255);

    SetChecked(hChkNumeric, false);
    SetChecked(hChkScaleVal, false);
    SetChecked(hChkDispUnits, false);
    SetChecked(hChkDispLabel, false);
    SetChecked(hChkHor, false);
    SetChecked(hChkOrient, false);
    SetChecked(hChkBorder, false);
    SetChecked(hChkMinThresh, false);
    SetChecked(hChkMaxThresh, false);
    SetChecked(hChkMinPeak, false);
    SetChecked(hChkMaxPeak, false);
    SetChecked(hChkPeakRed, false);
    SetChecked(hDataChkBm, false);
    SetChecked(hDataChkIsLed, false);
    SetChecked(hDataChkRpm, false);

    if (hComboGaugeType) SendMessage(hComboGaugeType, CB_SETCURSEL, 0, 0);
    // Caller clears suppressLiveUpdate after SetPropMode / fill. Dropping it here
    // let EnableWindow/BN_CLICKED write Type=Analog onto a just-selected Symbol.
}

// Model -> UI for the current selection. Sets prop mode and color swatches.
void EditorWindow::PopulatePropertyPanel() {
    suppressLiveUpdate = true;
    ClearPropertyPanel();
    const auto& gauges = doc_.Gauges();
    const auto& outputs = doc_.Outputs();
    const auto& labels = doc_.Labels();

    if (selectedGauge >= 0 && selectedGauge < static_cast<int>(gauges.size())) {
        SetPropMode(PropMode::Gauge);
        const Gauge& g = gauges[selectedGauge];

        // Show Holley \x escapes for Symbol PUA (and any non-ASCII); plain text unchanged.
        // SetTextA of raw UTF-8 PUA is mojibake and breaks editing.
        SetTextA(hEditLabel, IniUtils::Utf8ToEditText(g.label));
        SetTextA(hEditUnits, IniUtils::Utf8ToEditText(g.units));
        SetTextA(hEditX, std::to_string(g.x));
        SetTextA(hEditY, std::to_string(g.y));
        if (GaugeTypeUsesStatusLedSize(g.gaugeType)) {
            // Holley Size Normal/Large: keep hit-box (44x63 / 90x109); art from orientation.
            SetTextA(hEditW, std::to_string(g.width > 0 ? g.width : StatusLedDiameterPx(g)));
            SetTextA(hEditH, std::to_string(g.height > 0 ? g.height : StatusLedDiameterPx(g)));
        } else if (GaugeTypeUsesSquareSize(g.gaugeType)) {
            const int side = (g.width > 0) ? g.width : g.height;
            SetTextA(hEditW, std::to_string(side > 0 ? side : 100));
            SetTextA(hEditH, std::to_string(side > 0 ? side : 100));
        } else if (GaugeTypeUsesDigitalSize(g.gaugeType)) {
            // Size = height; width from measured value/units (INI W ignored).
            const int sizeH = (g.height > 0) ? g.height : 72;
            const int effW = DigitalWidthForHeight(sizeH, g);
            SetTextA(hEditW, std::to_string(sizeH));
            SetTextA(hEditH, std::to_string(effW));
        } else if (GaugeTypeUsesBargraphWidthOnly(g.gaugeType)) {
            // Width field = long axis (length). Secondary = derived thickness.
            const int length = g.orientation
                ? ((g.height > 0) ? g.height : g.width)
                : ((g.width > 0) ? g.width : 200);
            SetTextA(hEditW, std::to_string(length > 0 ? length : 200));
            SetTextA(hEditH, std::to_string(BargraphCardThickness(g)));
        } else if (GaugeTypeUsesFixedSize(g.gaugeType)) {
            SetTextA(hEditW, std::to_string(g.width > 0 ? g.width : kRpmBarDefaultW));
            SetTextA(hEditH, std::to_string(g.height > 0 ? g.height : kRpmBarDefaultH));
        } else if (GaugeTypeUsesSymbolHeight(g.gaugeType)) {
            SetTextA(hEditW, std::to_string(g.width > 0 ? g.width : 120));
            SetTextA(hEditH, std::to_string(g.height > 0 ? g.height : 120));
        } else {
            SetTextA(hEditW, std::to_string(g.width));
            SetTextA(hEditH, std::to_string(g.height));
        }

        RebuildGaugeTypeCombo();
        int sel = -1;
        for (int i = 0; i < static_cast<int>(sizeof(kGaugeTypes) / sizeof(kGaugeTypes[0])); ++i) {
            if (kGaugeTypes[i].id == g.gaugeType) { sel = i; break; }
        }
        if (sel < 0) {
            wchar_t buf[64];
            swprintf_s(buf, L"%d - Unknown", g.gaugeType);
            int idx = static_cast<int>(SendMessageW(hComboGaugeType, CB_ADDSTRING, 0, (LPARAM)buf));
            SendMessage(hComboGaugeType, CB_SETITEMDATA, idx, static_cast<LPARAM>(g.gaugeType));
            sel = idx;
        }
        SendMessage(hComboGaugeType, CB_SETCURSEL, sel, 0);
        InvalidateRect(hComboGaugeType, nullptr, FALSE);

        SetTextA(hEditMin, IniUtils::CleanDouble(g.minValue));
        SetTextA(hEditMax, IniUtils::CleanDouble(g.maxValue));
        SetTextA(hEditMinCaut, IniUtils::CleanDouble(g.minCaution));
        SetTextA(hEditMaxCaut, IniUtils::CleanDouble(g.maxCaution));
        SetTextA(hEditMinNorm, IniUtils::CleanDouble(g.minNormal));
        SetTextA(hEditMaxNorm, IniUtils::CleanDouble(g.maxNormal));
        SetTextA(hEditStart, std::to_string(g.startAngle));
        SetTextA(hEditEnd, std::to_string(g.endAngle));
        SetTextA(hEditFont, std::to_string(g.fontSize));
        SetTextA(hEditSteps, std::to_string(g.steps));
        SetTextA(hEditMtics, std::to_string(g.mtics));
        SetTextA(hEditScaleDiv, IniUtils::CleanDouble(g.scalingDivisor));
        SetTextA(hEditPrec, std::to_string(g.precision));
        SetTextA(hEditNnl, std::to_string(g.nnl));
        SetTextA(hEditNcl, std::to_string(g.ncl));
        SetTransparencyUi(static_cast<int>((g.background >> 24) & 0xFF));

        SetChecked(hChkNumeric, g.numericIndicatorEnabled);
        SetChecked(hChkScaleVal, g.scaleValues);
        SetChecked(hChkDispUnits, g.displayUnits);
        SetChecked(hChkDispLabel, g.displayLabel);
        SetChecked(hChkHor, g.hor);
        SetChecked(hChkOrient, g.orientation);
        SetChecked(hChkBorder, g.crown);
        {
            int style = g.ct;
            if (style < 0) style = 0;
            if (style > 3) style = 3;
            SendMessage(hComboBorderStyle, CB_SETCURSEL, style, 0);
        }
        SetChecked(hChkMinThresh, g.minThresholdEnabled);
        SetChecked(hChkMaxThresh, g.maxThresholdEnabled);
        SetChecked(hChkMinPeak, g.minPeakEnabled);
        SetChecked(hChkMaxPeak, g.maxPeakEnabled);
        SetChecked(hChkPeakRed, g.peakReductionEnabled);

        // Data section
        SetTextA(hDataEditChannel, std::to_string(g.channel));
        SetTextA(hDataEditMult, IniUtils::CleanDouble(g.valueMult));
        SetTextA(hDataEditMult2, IniUtils::CleanDouble(g.valueMult2));
        SetTextA(hDataEditMultB, IniUtils::CleanDouble(g.valueMultB));
        FillModeCombo(hComboUpdate, kUpdateRateModes, GaugeModeCount(kUpdateRateModes), g.updateRate);
        FillModeCombo(hComboAv, kAveragingModes, GaugeModeCount(kAveragingModes), g.av);
        FillModeCombo(hComboWfr, kWfrModes, GaugeModeCount(kWfrModes), g.wfr);
        SetTextA(hDataEditCt, std::to_string(g.ct));
        SetChecked(hDataChkBm, g.bm);
        SetChecked(hDataChkIsLed, g.isLED);
        SetChecked(hDataChkRpm, g.rpmModeEnabled);

        UpdateColorSwatches();
        ApplyGaugeTypeVisibility(g.gaugeType);
        suppressLiveUpdate = false;
        SyncPreviewSliderFromRenderer();
    } else if (selectedOutput >= 0 && selectedOutput < static_cast<int>(outputs.size())) {
        SetPropMode(PropMode::Output);
        const Output& o = outputs[selectedOutput];

        SetTextA(hEditLabel, IniUtils::Utf8ToEditText(o.text));
        SetTextA(hEditX, std::to_string(o.x));
        SetTextA(hEditY, std::to_string(o.y));
        SetTextA(hEditFont, std::to_string(o.fontSize));
        SetTextA(hDataEditChannel, std::to_string(o.channel));
        SetTransparencyUi(static_cast<int>((o.background >> 24) & 0xFF));

        // Style: known styles from Misc + preserve unknown IDs
        SendMessage(hComboGaugeType, CB_RESETCONTENT, 0, 0);
        bool foundStyle = false;
        for (const auto& st : kSwitchStyles) {
            int idx = static_cast<int>(SendMessageW(hComboGaugeType, CB_ADDSTRING, 0, (LPARAM)st.name));
            SendMessage(hComboGaugeType, CB_SETITEMDATA, idx, st.id);
            if (st.id == o.style) {
                SendMessage(hComboGaugeType, CB_SETCURSEL, idx, 0);
                foundStyle = true;
            }
        }
        if (!foundStyle) {
            wchar_t buf[48];
            swprintf_s(buf, L"%d - Style (unknown)", o.style);
            int idx = static_cast<int>(SendMessageW(hComboGaugeType, CB_ADDSTRING, 0, (LPARAM)buf));
            SendMessage(hComboGaugeType, CB_SETITEMDATA, idx, o.style);
            SendMessage(hComboGaugeType, CB_SETCURSEL, idx, 0);
        }

        UpdateColorSwatches();
        suppressLiveUpdate = false;
    } else if (selectedLabel >= 0 && selectedLabel < static_cast<int>(labels.size())) {
        SetPropMode(PropMode::Label);
        const Label& lab = labels[selectedLabel];

        SetTextA(hEditLabel, IniUtils::Utf8ToEditText(lab.text));
        SetTextA(hEditX, std::to_string(lab.x));
        SetTextA(hEditY, std::to_string(lab.y));
        SetTextA(hEditFont, std::to_string(lab.fontSize));
        SetTransparencyUi(static_cast<int>((lab.background >> 24) & 0xFF));

        UpdateColorSwatches();
        suppressLiveUpdate = false;
    } else {
        SetPropMode(PropMode::None);
        suppressLiveUpdate = false;
    }
}

// UI -> model. Empty fields leave the previous value (parse* returns true).
// Integers/doubles are clamped to sensible ranges; zone thresholds stay in min/max.
bool EditorWindow::ReadPropertiesIntoSelected(bool showError, bool applyComboType) {
    auto parseInt = [](const std::string& s, int& out) -> bool {
        if (s.empty()) return true;
        try { out = std::stoi(s); return true; } catch (...) { return false; }
    };
    auto parseDbl = [](const std::string& s, double& out) -> bool {
        if (s.empty()) return true;
        try { out = std::stod(s); return true; } catch (...) { return false; }
    };

    auto& gauges = doc_.Gauges();
    auto& outputs = doc_.Outputs();
    auto& labels = doc_.Labels();

    try {
        if (selectedGauge >= 0 && selectedGauge < static_cast<int>(gauges.size())) {
            Gauge& g = gauges[selectedGauge];
            // Accept typed \xf095 / \xb0 or Alt-code °; store UTF-8 in model.
            const std::string rawLabel = GetTextA(hEditLabel);
            const std::string rawUnits = GetTextA(hEditUnits);
            g.label = IniUtils::EditTextToUtf8(rawLabel);
            g.units = IniUtils::EditTextToUtf8(rawUnits);
            NormalizeEditIfHigh(hEditLabel, g.label, rawLabel, suppressLiveUpdate);
            NormalizeEditIfHigh(hEditUnits, g.units, rawUnits, suppressLiveUpdate);

            if (!parseInt(GetTextA(hEditX), g.x)) throw std::runtime_error("X");
            if (!parseInt(GetTextA(hEditY), g.y)) throw std::runtime_error("Y");

            const int prevType = g.gaugeType;
            if (applyComboType) {
                int csel = static_cast<int>(SendMessage(hComboGaugeType, CB_GETCURSEL, 0, 0));
                if (csel >= 0) {
                    LRESULT data = SendMessage(hComboGaugeType, CB_GETITEMDATA, csel, 0);
                    if (data != CB_ERR) g.gaugeType = static_cast<int>(data);
                }
            }
            const bool typeChanged = (g.gaugeType != prevType);

            // Size fields: write only what the user can edit. Paint uses EffectiveGaugeSize.
            if (GaugeTypeUsesStatusLedSize(g.gaugeType)) {
                // Footprint applied below only when Size = Large changes.
            } else if (GaugeTypeUsesFixedSize(g.gaugeType)) {
                if (typeChanged) {
                    g.width = kRpmBarDefaultW;
                    g.height = kRpmBarDefaultH;
                }
            } else if (GaugeTypeUsesSquareSize(g.gaugeType)) {
                if (!parseInt(GetTextA(hEditW), g.width)) throw std::runtime_error("Size");
                ClampInt(g.width, 1, 4000);
                g.height = g.width;
            } else if (GaugeTypeUsesDigitalSize(g.gaugeType)) {
                if (!parseInt(GetTextA(hEditW), g.height)) throw std::runtime_error("Size");
                ClampInt(g.height, 1, 4000);
            } else if (GaugeTypeUsesBargraphWidthOnly(g.gaugeType)) {
                // Length applied after Vertical is read.
            } else if (g.gaugeType == 8) {
                // Symbol: size= is the hit box. Glyph size is Font Size only.
            } else {
                if (!parseInt(GetTextA(hEditW), g.width)) throw std::runtime_error("Width");
                if (!parseInt(GetTextA(hEditH), g.height)) throw std::runtime_error("Height");
                ClampInt(g.width, 1, 4000);
                ClampInt(g.height, 1, 4000);
            }

            if (!parseDbl(GetTextA(hEditMin), g.minValue)) throw std::runtime_error("MinValue");
            if (!parseDbl(GetTextA(hEditMax), g.maxValue)) throw std::runtime_error("MaxValue");
            if (!parseDbl(GetTextA(hEditMinCaut), g.minCaution)) throw std::runtime_error("MinCaution");
            if (!parseDbl(GetTextA(hEditMaxCaut), g.maxCaution)) throw std::runtime_error("MaxCaution");
            if (!parseDbl(GetTextA(hEditMinNorm), g.minNormal)) throw std::runtime_error("MinNormal");
            if (!parseDbl(GetTextA(hEditMaxNorm), g.maxNormal)) throw std::runtime_error("MaxNormal");

            if (!parseInt(GetTextA(hEditStart), g.startAngle)) throw std::runtime_error("StartAngle");
            if (!parseInt(GetTextA(hEditEnd), g.endAngle)) throw std::runtime_error("EndAngle");
            ClampInt(g.startAngle, -360, 360);
            ClampInt(g.endAngle, -360, 360);

            if (!parseInt(GetTextA(hEditFont), g.fontSize)) throw std::runtime_error("FontSize");
            ClampInt(g.fontSize, -1, 240);

            if (!parseInt(GetTextA(hEditSteps), g.steps)) throw std::runtime_error("Steps");
            if (!parseInt(GetTextA(hEditMtics), g.mtics)) throw std::runtime_error("mtics");
            ClampInt(g.steps, 0, 200);
            ClampInt(g.mtics, 0, 50);

            if (!parseDbl(GetTextA(hEditScaleDiv), g.scalingDivisor)) throw std::runtime_error("ScaleDiv");
            if (!parseInt(GetTextA(hEditPrec), g.precision)) throw std::runtime_error("Precision");
            ClampInt(g.precision, 0, 8);

            if (!parseInt(GetTextA(hEditNnl), g.nnl)) throw std::runtime_error("nnl");
            if (!parseInt(GetTextA(hEditNcl), g.ncl)) throw std::runtime_error("ncl");
            ClampInt(g.nnl, 0, 100);
            ClampInt(g.ncl, 0, 100);

            // Prefer slider position; fall back to edit if parse succeeds from edit-only path.
            // Skip when Transparency is disabled (LED / Symbol have no background-alpha row).
            if (hAlphaSlider && IsWindowEnabled(hAlphaSlider)) {
                int alpha = GetTransparencyUi();
                int alphaEdit = alpha;
                if (parseInt(GetTextA(hEditAlpha), alphaEdit))
                    alpha = alphaEdit;
                ClampInt(alpha, 0, 255);
                g.background = (g.background & 0x00FFFFFFu) | (static_cast<unsigned>(alpha) << 24);
                SendMessage(hAlphaSlider, TBM_SETPOS, TRUE, alpha);
            }

            if (g.gaugeType != 2)
                g.numericIndicatorEnabled = GetChecked(hChkNumeric);
            g.scaleValues = GetChecked(hChkScaleVal);
            g.displayUnits = GetChecked(hChkDispUnits);
            g.displayLabel = GetChecked(hChkDispLabel);
            if (hChkHor && IsWindowEnabled(hChkHor))
                g.hor = GetChecked(hChkHor);
            if (hChkOrient && IsWindowEnabled(hChkOrient)) {
                const bool newOrient = GetChecked(hChkOrient);
                if (GaugeTypeUsesStatusLedSize(g.gaugeType) && newOrient != g.orientation) {
                    g.orientation = newOrient;
                    if (g.orientation) {
                        g.width = 90;
                        g.height = 109;
                    } else {
                        g.width = 44;
                        g.height = 63;
                    }
                } else {
                    g.orientation = newOrient;
                }
            }
            if (GaugeTypeUsesBargraphWidthOnly(g.gaugeType)) {
                int length = 200;
                if (!parseInt(GetTextA(hEditW), length)) throw std::runtime_error("Width");
                ClampInt(length, 1, 4000);
                if (g.orientation)
                    g.height = length;
                else
                    g.width = length;
            }
            g.crown = GetChecked(hChkBorder);
            if (GaugeTypeUsesBorderStyle(g.gaugeType)) {
                int bsel = static_cast<int>(SendMessage(hComboBorderStyle, CB_GETCURSEL, 0, 0));
                if (bsel >= 0 && bsel <= 3)
                    g.ct = bsel;
                else
                    g.ct = 0;
            }
            g.minThresholdEnabled = GetChecked(hChkMinThresh);
            g.maxThresholdEnabled = GetChecked(hChkMaxThresh);
            g.minPeakEnabled = GetChecked(hChkMinPeak);
            g.maxPeakEnabled = GetChecked(hChkMaxPeak);
            g.peakReductionEnabled = GetChecked(hChkPeakRed);

            // Data / non-visual
            if (!parseInt(GetTextA(hDataEditChannel), g.channel)) throw std::runtime_error("Channel");
            if (!parseDbl(GetTextA(hDataEditMult), g.valueMult)) throw std::runtime_error("valueMult");
            if (!parseDbl(GetTextA(hDataEditMult2), g.valueMult2)) throw std::runtime_error("valueMult2");
            if (!parseDbl(GetTextA(hDataEditMultB), g.valueMultB)) throw std::runtime_error("valueMultB");
            g.updateRate = ComboData(hComboUpdate, g.updateRate);
            g.av = ComboData(hComboAv, g.av);
            g.wfr = ComboData(hComboWfr, g.wfr);
            // ct from Border Style combo for analog family; Advanced edit for others
            if (!GaugeTypeUsesBorderStyle(g.gaugeType)) {
                if (!parseInt(GetTextA(hDataEditCt), g.ct)) throw std::runtime_error("ct");
            }
            ClampInt(g.ct, 0, 99);

            g.bm = GetChecked(hDataChkBm);
            g.isLED = GetChecked(hDataChkIsLed);
            if (hDataChkRpm && IsWindowEnabled(hDataChkRpm))
                g.rpmModeEnabled = GetChecked(hDataChkRpm);

            // Zone thresholds within min/max
            const double lo = (std::min)(g.minValue, g.maxValue);
            const double hi = (std::max)(g.minValue, g.maxValue);
            ClampDouble(g.minCaution, lo, hi);
            ClampDouble(g.maxCaution, lo, hi);
            ClampDouble(g.minNormal, lo, hi);
            ClampDouble(g.maxNormal, lo, hi);

            if (typeChanged)
                NormalizeGaugeGeometry(g);

            UpdateColorSwatches();
            return true;
        }

        if (selectedOutput >= 0 && selectedOutput < static_cast<int>(outputs.size())) {
            Output& o = outputs[selectedOutput];
            const std::string rawText = GetTextA(hEditLabel);
            o.text = IniUtils::EditTextToUtf8(rawText);
            NormalizeEditIfHigh(hEditLabel, o.text, rawText, suppressLiveUpdate);
            if (!parseInt(GetTextA(hEditX), o.x)) throw std::runtime_error("X");
            if (!parseInt(GetTextA(hEditY), o.y)) throw std::runtime_error("Y");
            if (!parseInt(GetTextA(hEditFont), o.fontSize)) throw std::runtime_error("FontSize");
            ClampInt(o.fontSize, 1, 240);
            if (!parseInt(GetTextA(hDataEditChannel), o.channel)) throw std::runtime_error("Channel");

            if (applyComboType) {
                int csel = static_cast<int>(SendMessage(hComboGaugeType, CB_GETCURSEL, 0, 0));
                if (csel >= 0) {
                    LRESULT data = SendMessage(hComboGaugeType, CB_GETITEMDATA, csel, 0);
                    if (data != CB_ERR) o.style = static_cast<int>(data);
                }
            }
            ClampInt(o.style, 0, 32);

            int alpha = GetTransparencyUi();
            int alphaEdit = alpha;
            if (parseInt(GetTextA(hEditAlpha), alphaEdit))
                alpha = alphaEdit;
            ClampInt(alpha, 0, 255);
            o.background = (o.background & 0x00FFFFFFu) | (static_cast<unsigned>(alpha) << 24);
            if (hAlphaSlider)
                SendMessage(hAlphaSlider, TBM_SETPOS, TRUE, alpha);

            UpdateColorSwatches();
            return true;
        }

        if (selectedLabel >= 0 && selectedLabel < static_cast<int>(labels.size())) {
            Label& lab = labels[selectedLabel];
            const std::string rawText = GetTextA(hEditLabel);
            lab.text = IniUtils::EditTextToUtf8(rawText);
            NormalizeEditIfHigh(hEditLabel, lab.text, rawText, suppressLiveUpdate);
            if (!parseInt(GetTextA(hEditX), lab.x)) throw std::runtime_error("X");
            if (!parseInt(GetTextA(hEditY), lab.y)) throw std::runtime_error("Y");
            if (!parseInt(GetTextA(hEditFont), lab.fontSize)) throw std::runtime_error("FontSize");
            ClampInt(lab.fontSize, 1, 240);
            int alpha = GetTransparencyUi();
            int alphaEdit = alpha;
            if (parseInt(GetTextA(hEditAlpha), alphaEdit))
                alpha = alphaEdit;
            ClampInt(alpha, 0, 255);
            lab.background = (lab.background & 0x00FFFFFFu) | (static_cast<unsigned>(alpha) << 24);
            if (hAlphaSlider)
                SendMessage(hAlphaSlider, TBM_SETPOS, TRUE, alpha);
            UpdateColorSwatches();
            return true;
        }
    } catch (...) {
        if (showError) {
            Log("ERROR: Invalid numeric value in properties.");
            MessageBoxW(hWnd, L"One or more fields contain invalid numbers.", L"Properties",
                MB_OK | MB_ICONWARNING);
        }
        return false;
    }
    return false;
}

// Called on every edit/check/combo change while a selection is active.
void EditorWindow::LiveUpdateFromPanel(bool applyComboType) {
    if (suppressLiveUpdate) return;
    if (selectedGauge < 0 && selectedOutput < 0 && selectedLabel < 0) return;
    if (ReadPropertiesIntoSelected(false, applyComboType)) {
        if (selectedGauge >= 0) {
            const auto& gauges = doc_.Gauges();
            if (selectedGauge < static_cast<int>(gauges.size()))
                ApplyGaugeTypeVisibility(gauges[selectedGauge].gaugeType);
        }
        MarkDirty();
        InvalidatePreview();
    }
}

// Push model x/y into the property edits without triggering live update.
void EditorWindow::SyncPositionEditsFromModel() {
    suppressLiveUpdate = true;
    if (selectedGauge >= 0) {
        const auto& g = doc_.Gauges();
        if (selectedGauge < static_cast<int>(g.size())) {
            SetTextA(hEditX, std::to_string(g[selectedGauge].x));
            SetTextA(hEditY, std::to_string(g[selectedGauge].y));
        }
    } else if (selectedLabel >= 0) {
        const auto& labs = doc_.Labels();
        if (selectedLabel < static_cast<int>(labs.size())) {
            SetTextA(hEditX, std::to_string(labs[selectedLabel].x));
            SetTextA(hEditY, std::to_string(labs[selectedLabel].y));
        }
    } else if (selectedOutput >= 0) {
        const auto& outs = doc_.Outputs();
        if (selectedOutput < static_cast<int>(outs.size())) {
            SetTextA(hEditX, std::to_string(outs[selectedOutput].x));
            SetTextA(hEditY, std::to_string(outs[selectedOutput].y));
        }
    }
    suppressLiveUpdate = false;
}

// Force repaint of color swatch statics (actual brush comes from WM_CTLCOLORSTATIC).
void EditorWindow::UpdateColorSwatches() {
    auto paint = [](HWND h) {
        if (h) InvalidateRect(h, nullptr, TRUE);
    };
    paint(hFgSwatch);
    paint(hBgSwatch);
    for (int z = 0; z < 5; ++z) paint(hZoneSwatch[z]);
    paint(hOutInactSwatch);
    paint(hOutActSwatch);
}

// Dash-style color dialog: R/G/B/Alpha 0–255. System RGB… keeps current A (including 0).
bool EditorWindow::PickColor(unsigned int& holleyColor) {
    struct State {
        unsigned int color = 0;
        HWND hDlg = nullptr;
        HWND hR = nullptr, hG = nullptr, hB = nullptr, hA = nullptr;
        HWND hRs = nullptr, hGs = nullptr, hBs = nullptr, hAs = nullptr;
        RECT swatch = {};
        bool ok = false;
        bool done = false;
        bool suppress = false;
        HFONT font = nullptr;
        HBRUSH bg = nullptr;
    };
    enum {
        kRgbBtn = 20,
        kREdit = 30,
        kGEdit = 31,
        kBEdit = 32,
        kAEdit = 33,
        kRSlider = 40,
        kGSlider = 41,
        kBSlider = 42,
        kASlider = 43,
    };

    auto fillEdits = [&](State& st) {
        BYTE a = static_cast<BYTE>((st.color >> 24) & 0xFF);
        BYTE r = static_cast<BYTE>((st.color >> 16) & 0xFF);
        BYTE g = static_cast<BYTE>((st.color >> 8) & 0xFF);
        BYTE b = static_cast<BYTE>(st.color & 0xFF);
        st.suppress = true;
        SetWindowTextA(st.hR, std::to_string(r).c_str());
        SetWindowTextA(st.hG, std::to_string(g).c_str());
        SetWindowTextA(st.hB, std::to_string(b).c_str());
        SetWindowTextA(st.hA, std::to_string(a).c_str());
        if (st.hRs) SendMessage(st.hRs, TBM_SETPOS, TRUE, r);
        if (st.hGs) SendMessage(st.hGs, TBM_SETPOS, TRUE, g);
        if (st.hBs) SendMessage(st.hBs, TBM_SETPOS, TRUE, b);
        if (st.hAs) SendMessage(st.hAs, TBM_SETPOS, TRUE, a);
        st.suppress = false;
    };

    State st;
    st.color = holleyColor;
    st.font = hUiFont_;
    st.bg = hBrBg_;

    static bool registered = false;
    if (!registered) {
        WNDCLASSW wc = {};
        wc.lpfnWndProc = [](HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) -> LRESULT {
            State* s = reinterpret_cast<State*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            if (msg == WM_NCCREATE) {
                auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
                SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
                s = reinterpret_cast<State*>(cs->lpCreateParams);
            }
            if (!s) return DefWindowProcW(hwnd, msg, wParam, lParam);
            if (msg == WM_COMMAND) {
                const int id = LOWORD(wParam);
                const int code = HIWORD(wParam);
                if (id == IDOK) {
                    auto one = [&](HWND h) {
                        char buf[16] = {};
                        GetWindowTextA(h, buf, 16);
                        int v = atoi(buf);
                        if (v < 0) v = 0;
                        if (v > 255) v = 255;
                        return v;
                    };
                    s->color =
                        (static_cast<unsigned>(one(s->hA)) << 24) |
                        (static_cast<unsigned>(one(s->hR)) << 16) |
                        (static_cast<unsigned>(one(s->hG)) << 8) |
                        static_cast<unsigned>(one(s->hB));
                    s->ok = true;
                    s->done = true;
                    return 0;
                }
                if (id == IDCANCEL) {
                    s->ok = false;
                    s->done = true;
                    return 0;
                }
                if (id == kRgbBtn) {
                    static COLORREF custom[16] = {};
                    CHOOSECOLORW cc = {};
                    cc.lStructSize = sizeof(cc);
                    cc.hwndOwner = hwnd;
                    BYTE a, r, g, b;
                    a = static_cast<BYTE>((s->color >> 24) & 0xFF);
                    r = static_cast<BYTE>((s->color >> 16) & 0xFF);
                    g = static_cast<BYTE>((s->color >> 8) & 0xFF);
                    b = static_cast<BYTE>(s->color & 0xFF);
                    cc.rgbResult = RGB(r, g, b);
                    cc.lpCustColors = custom;
                    cc.Flags = CC_FULLOPEN | CC_RGBINIT;
                    if (ChooseColorW(&cc)) {
                        s->color = (static_cast<unsigned>(a) << 24) |
                            (static_cast<unsigned>(GetRValue(cc.rgbResult)) << 16) |
                            (static_cast<unsigned>(GetGValue(cc.rgbResult)) << 8) |
                            GetBValue(cc.rgbResult);
                        s->suppress = true;
                        SetWindowTextA(s->hR, std::to_string(GetRValue(cc.rgbResult)).c_str());
                        SetWindowTextA(s->hG, std::to_string(GetGValue(cc.rgbResult)).c_str());
                        SetWindowTextA(s->hB, std::to_string(GetBValue(cc.rgbResult)).c_str());
                        SendMessage(s->hRs, TBM_SETPOS, TRUE, GetRValue(cc.rgbResult));
                        SendMessage(s->hGs, TBM_SETPOS, TRUE, GetGValue(cc.rgbResult));
                        SendMessage(s->hBs, TBM_SETPOS, TRUE, GetBValue(cc.rgbResult));
                        s->suppress = false;
                        InvalidateRect(hwnd, &s->swatch, FALSE);
                    }
                    return 0;
                }
                if (code == EN_CHANGE && !s->suppress &&
                    (id == kREdit || id == kGEdit || id == kBEdit || id == kAEdit)) {
                    auto one = [&](HWND h) {
                        char buf[16] = {};
                        GetWindowTextA(h, buf, 16);
                        int v = atoi(buf);
                        if (v < 0) v = 0;
                        if (v > 255) v = 255;
                        return v;
                    };
                    s->color =
                        (static_cast<unsigned>(one(s->hA)) << 24) |
                        (static_cast<unsigned>(one(s->hR)) << 16) |
                        (static_cast<unsigned>(one(s->hG)) << 8) |
                        static_cast<unsigned>(one(s->hB));
                    s->suppress = true;
                    if (id == kREdit) SendMessage(s->hRs, TBM_SETPOS, TRUE, one(s->hR));
                    if (id == kGEdit) SendMessage(s->hGs, TBM_SETPOS, TRUE, one(s->hG));
                    if (id == kBEdit) SendMessage(s->hBs, TBM_SETPOS, TRUE, one(s->hB));
                    if (id == kAEdit) SendMessage(s->hAs, TBM_SETPOS, TRUE, one(s->hA));
                    s->suppress = false;
                    InvalidateRect(hwnd, &s->swatch, FALSE);
                    return 0;
                }
            }
            if (msg == WM_HSCROLL) {
                HWND hBar = reinterpret_cast<HWND>(lParam);
                if (s->suppress || !hBar) return 0;
                const int pos = static_cast<int>(SendMessage(hBar, TBM_GETPOS, 0, 0));
                BYTE a = static_cast<BYTE>((s->color >> 24) & 0xFF);
                BYTE r = static_cast<BYTE>((s->color >> 16) & 0xFF);
                BYTE g = static_cast<BYTE>((s->color >> 8) & 0xFF);
                BYTE b = static_cast<BYTE>(s->color & 0xFF);
                s->suppress = true;
                if (hBar == s->hRs) {
                    r = static_cast<BYTE>(pos);
                    SetWindowTextA(s->hR, std::to_string(pos).c_str());
                } else if (hBar == s->hGs) {
                    g = static_cast<BYTE>(pos);
                    SetWindowTextA(s->hG, std::to_string(pos).c_str());
                } else if (hBar == s->hBs) {
                    b = static_cast<BYTE>(pos);
                    SetWindowTextA(s->hB, std::to_string(pos).c_str());
                } else if (hBar == s->hAs) {
                    a = static_cast<BYTE>(pos);
                    SetWindowTextA(s->hA, std::to_string(pos).c_str());
                }
                s->suppress = false;
                s->color = (static_cast<unsigned>(a) << 24) | (static_cast<unsigned>(r) << 16) |
                    (static_cast<unsigned>(g) << 8) | b;
                InvalidateRect(hwnd, &s->swatch, FALSE);
                return 0;
            }
            if (msg == WM_CLOSE) {
                s->ok = false;
                s->done = true;
                return 0;
            }
            if (msg == WM_ERASEBKGND)
                return 1;
            if (msg == WM_PAINT) {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hwnd, &ps);
                RECT rc = {};
                GetClientRect(hwnd, &rc);
                FillRect(hdc, &rc, s->bg ? s->bg
                    : reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
                RECT sw = s->swatch;
                const int cell = 8;
                for (int y = sw.top; y < sw.bottom; y += cell) {
                    for (int x = sw.left; x < sw.right; x += cell) {
                        RECT c = { x, y,
                            (x + cell < sw.right) ? x + cell : sw.right,
                            (y + cell < sw.bottom) ? y + cell : sw.bottom };
                        const bool alt = (((x - sw.left) / cell) + ((y - sw.top) / cell)) & 1;
                        HBRUSH br = CreateSolidBrush(alt ? RGB(200, 200, 200) : RGB(240, 240, 240));
                        FillRect(hdc, &c, br);
                        DeleteObject(br);
                    }
                }
                BYTE a = static_cast<BYTE>((s->color >> 24) & 0xFF);
                BYTE r = static_cast<BYTE>((s->color >> 16) & 0xFF);
                BYTE g = static_cast<BYTE>((s->color >> 8) & 0xFF);
                BYTE b = static_cast<BYTE>(s->color & 0xFF);
                Gdiplus::Graphics gfx(hdc);
                Gdiplus::SolidBrush overlay(Gdiplus::Color(a, r, g, b));
                gfx.FillRectangle(&overlay,
                    static_cast<INT>(sw.left), static_cast<INT>(sw.top),
                    static_cast<INT>(sw.right - sw.left),
                    static_cast<INT>(sw.bottom - sw.top));
                EndPaint(hwnd, &ps);
                return 0;
            }
            if (msg == WM_CTLCOLORSTATIC || msg == WM_CTLCOLOREDIT) {
                HDC hdc = reinterpret_cast<HDC>(wParam);
                SetBkColor(hdc, RGB(32, 32, 32));
                SetTextColor(hdc, RGB(230, 230, 230));
                return reinterpret_cast<LRESULT>(s->bg ? s->bg
                    : GetStockObject(BLACK_BRUSH));
            }
            return DefWindowProcW(hwnd, msg, wParam, lParam);
        };
        wc.hInstance = GetModuleHandle(nullptr);
        wc.lpszClassName = L"HolleyColorPick";
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = hBrBg_ ? hBrBg_ : (HBRUSH)(COLOR_BTNFACE + 1);
        RegisterClassW(&wc);
        registered = true;
    }

    RECT pr = {};
    GetWindowRect(hWnd, &pr);
    const int dw = 520, dh = 260;
    HWND dlg = CreateWindowExW(WS_EX_DLGMODALFRAME, L"HolleyColorPick", L"Color",
        WS_CAPTION | WS_SYSMENU | WS_POPUP | WS_CLIPCHILDREN,
        pr.left + 80, pr.top + 80, dw, dh, hWnd, nullptr, GetModuleHandle(nullptr), &st);
    if (!dlg) return false;
    st.hDlg = dlg;

    auto lab = [&](int x, int y, int w, const wchar_t* t) {
        HWND h = CreateWindowW(L"STATIC", t,
            WS_CHILD | WS_VISIBLE | SS_NOPREFIX | SS_RIGHT,
            x, y, w, 18, dlg, nullptr, GetModuleHandle(nullptr), nullptr);
        if (st.font) SendMessageW(h, WM_SETFONT, (WPARAM)st.font, TRUE);
        return h;
    };
    auto ed = [&](int x, int y, int id) {
        HWND h = CreateWindowW(L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_NUMBER | ES_AUTOHSCROLL | WS_TABSTOP,
            x, y, 56, 22, dlg, (HMENU)(INT_PTR)id, GetModuleHandle(nullptr), nullptr);
        if (st.font) SendMessageW(h, WM_SETFONT, (WPARAM)st.font, TRUE);
        return h;
    };
    auto btn = [&](int x, int y, int w, int id, const wchar_t* t) {
        HWND h = CreateWindowW(L"BUTTON", t,
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
            x, y, w, 26, dlg, (HMENU)(INT_PTR)id, GetModuleHandle(nullptr), nullptr);
        if (st.font) SendMessageW(h, WM_SETFONT, (WPARAM)st.font, TRUE);
        return h;
    };
    auto slider = [&](int x, int y, int w, int id) {
        HWND h = CreateWindowW(TRACKBAR_CLASSW, L"",
            WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_NOTICKS | TBS_TOOLTIPS,
            x, y, w, 24, dlg, (HMENU)(INT_PTR)id, GetModuleHandle(nullptr), nullptr);
        SendMessage(h, TBM_SETRANGE, TRUE, MAKELPARAM(0, 255));
        SendMessage(h, TBM_SETPAGESIZE, 0, 16);
        return h;
    };

    st.swatch = { 16, 16, 112, 128 };
    lab(128, 18, 48, L"Red");
    st.hRs = slider(180, 16, 220, kRSlider);
    st.hR = ed(408, 16, kREdit);
    lab(128, 48, 48, L"Green");
    st.hGs = slider(180, 46, 220, kGSlider);
    st.hG = ed(408, 46, kGEdit);
    lab(128, 78, 48, L"Blue");
    st.hBs = slider(180, 76, 220, kBSlider);
    st.hB = ed(408, 76, kBEdit);
    lab(128, 108, 48, L"Alpha");
    st.hAs = slider(180, 106, 220, kASlider);
    st.hA = ed(408, 106, kAEdit);
    btn(16, 150, 96, kRgbBtn, L"RGB...");
    btn(316, 150, 80, IDOK, L"OK");
    btn(404, 150, 80, IDCANCEL, L"Cancel");
    fillEdits(st);

    EnableWindow(hWnd, FALSE);
    ShowWindow(dlg, SW_SHOW);
    SetForegroundWindow(dlg);

    MSG msg = {};
    while (!st.done && GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) {
            st.ok = false;
            st.done = true;
            continue;
        }
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_RETURN) {
            SendMessageW(dlg, WM_COMMAND, MAKEWPARAM(IDOK, BN_CLICKED), 0);
            continue;
        }
        if (!IsDialogMessageW(dlg, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    EnableWindow(hWnd, TRUE);
    SetForegroundWindow(hWnd);
    DestroyWindow(dlg);
    if (st.ok)
        holleyColor = st.color;
    return st.ok;
}

