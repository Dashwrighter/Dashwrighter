#pragma once

// =============================================================================
// EditorWindow.h — Main Win32 UI for the Holley Pro Dash layout editor
//
// Owns:
//   • Dark toolbar + status (Holley red primary)
//   • Preview canvas on top (1280×480)
//   • Dense inspector under the preview (fixed slots, enable/disable from Customize matrix)
//   • More settings dialog: channel / math / rate / peaks / firmware flags
//
// Selection: first click selects (no move). Drag starts on a later click of the
// already-selected widget. Right-click a stack for a pick menu.
// Property controls map to LayoutDocument fields; live updates on EN_CHANGE.
// =============================================================================

#include "InspectorLayout.h"
#include "../Model/LayoutDocument.h"
#include "../Io/BackgroundResolver.h"
#include "../Render/LayoutRenderer.h"
#include <windows.h>
#include <commctrl.h>
#include <gdiplus.h>
#include <string>
#include <vector>

// Control IDs.
// Property edits start at 100. Toolbar commands are named below
// (no 3, no 8). Stack menus are 4000 / 4100 / 4200.
namespace PropId {
    // --- Appearance tab: text / numeric edits ---
    constexpr int R_Label      = 100;
    constexpr int R_Units      = 101;
    constexpr int R_X          = 102;
    constexpr int R_Y          = 103;
    constexpr int R_Width      = 104;
    constexpr int R_Height     = 105;
    constexpr int R_GaugeType  = 106; // COMBOBOX: gauge type or output style
    constexpr int R_MinValue   = 107;
    constexpr int R_MaxValue   = 108;
    constexpr int R_MinCaution = 109;
    constexpr int R_MaxCaution = 110;
    constexpr int R_MinNormal  = 111;
    constexpr int R_MaxNormal  = 112;
    constexpr int R_StartAngle = 113;
    constexpr int R_EndAngle   = 114;
    constexpr int R_FontSize   = 115;
    constexpr int R_Steps      = 116;
    constexpr int R_Mtics      = 117;
    constexpr int R_ScaleDiv   = 118;
    constexpr int R_Precision  = 119;
    constexpr int R_Nnl        = 120;
    constexpr int R_Ncl        = 121;
    constexpr int R_Alpha      = 122; // transparency value edit (0–255)
    constexpr int R_AlphaSlider = 123;

    constexpr int R_NumericInd  = 130;
    constexpr int R_ScaleValues = 131;
    constexpr int R_DispUnits   = 132;
    constexpr int R_DispLabel   = 133;
    constexpr int R_Hor         = 134;
    constexpr int R_Orient      = 142; // orientation (vertical bargraph)

    // Appearance warnings / border (dash user-facing; affect zones preview)
    constexpr int R_MinThresh   = 135;
    constexpr int R_MaxThresh   = 136;
    constexpr int R_Border      = 140; // maps to INI crown (Border on/off)
    constexpr int R_BorderStyle = 143; // COMBOBOX → INI ct (Border Style)
    constexpr int R_PreviewSlider = 141; // demo value 0..1000 → min..max

    // Data tab: peak hold (markers not drawn in preview yet)
    constexpr int R_MinPeak     = 137;
    constexpr int R_MaxPeak     = 138;
    constexpr int R_PeakRed     = 139;

    constexpr int R_FgSwatch = 200;
    constexpr int R_FgBtn    = 201;
    constexpr int R_BgSwatch = 202;
    constexpr int R_BgBtn    = 203;
    constexpr int R_C0Swatch = 210;
    constexpr int R_C0Btn    = 220;
    constexpr int R_OutInactSwatch = 230;
    constexpr int R_OutInactBtn    = 231;
    constexpr int R_OutActSwatch   = 232;
    constexpr int R_OutActBtn      = 233;

    // Data tab (channel / math / rate) — not drawn in preview
    constexpr int D_Channel     = 300;
    constexpr int D_ValueMult   = 301;
    constexpr int D_ValueMult2  = 302;
    constexpr int D_ValueMultB  = 303;
    constexpr int D_UpdateRate  = 304;
    constexpr int D_Av          = 305;
    constexpr int D_Ct          = 306;

    // Advanced tab (firmware flags; still load/save)
    constexpr int D_Bm          = 310;
    constexpr int D_IsLed       = 316;
    constexpr int D_RpmMode     = 318;
    constexpr int D_Wfr         = 320;

    // Toolbar commands.
    constexpr int LoadLayout    = 1;
    constexpr int Background    = 2;
    constexpr int SaveAs        = 4;
    constexpr int Clear         = 5;
    constexpr int Screenshot    = 6;
    constexpr int PositionsShot = 7;  // screenshot with a ring on every gauge
    constexpr int AddGauge      = 9;
    constexpr int AddLabel      = 10;
    constexpr int Duplicate     = 11;
    constexpr int Delete        = 12;
    constexpr int AddOutput     = 13;
    constexpr int MoreSettings  = 14;
    constexpr int R_SymbolPick  = 15; // Label-adjacent; Symbol type only
    constexpr int D_ChannelPick = 16; // More settings; gauges only (not switches)
    constexpr int StackMenuBase  = 4000; // overlap: select hit i
    constexpr int StackFrontBase = 4100; // overlap: bring gauge hit i to front
    constexpr int StackBackBase  = 4200; // overlap: send gauge hit i to back
    constexpr int StackMenuSlotMax = 64;
}

class EditorWindow {
public:
    EditorWindow();
    ~EditorWindow();

    bool Create();
    void Run();

private:
    enum class PropMode { None, Gauge, Output, Label };

    // Toolbar / status only. Inspector + preview rects live in InspectorLayout.h.
    static constexpr int kPad        = Insp::kPad;
    static constexpr int kToolbarY   = 4;
    static constexpr int kToolbarH   = 28;
    static constexpr int kStatusY    = 36;
    static constexpr int kStatusH    = 18;

    HWND hWnd = nullptr;
    HWND hLayoutTitle = nullptr; // status line under toolbar
    HWND hMoreWnd = nullptr;     // More settings popup
    HWND hBtnMore = nullptr;
    HFONT hUiFont_ = nullptr;
    HBRUSH hBrBg_ = nullptr;
    HBRUSH hBrField_ = nullptr;
    HBRUSH hBrBar_ = nullptr;
    HBRUSH hBrRed_ = nullptr;
    std::vector<HWND> toolbarBtns_;

    HWND hLabLabel = nullptr, hLabUnits = nullptr;
    HWND hLabX = nullptr, hLabY = nullptr, hLabW = nullptr, hLabWWidth = nullptr, hLabH = nullptr;
    HWND hLabGaugeType = nullptr;
    HWND hLabMin = nullptr, hLabMax = nullptr;
    HWND hLabMinCaut = nullptr, hLabMaxCaut = nullptr;
    HWND hLabMinNorm = nullptr, hLabMaxNorm = nullptr;
    HWND hLabStart = nullptr, hLabEnd = nullptr;
    HWND hLabFont = nullptr, hLabSteps = nullptr, hLabMtics = nullptr;
    HWND hLabScaleDiv = nullptr, hLabPrec = nullptr;
    HWND hLabNnl = nullptr, hLabNcl = nullptr, hLabAlpha = nullptr;
    HWND hLabFg = nullptr, hLabBg = nullptr;
    HWND hLabZone[5] = {};
    HWND hLabOutInact = nullptr, hLabOutAct = nullptr;

    HWND hEditLabel = nullptr, hEditUnits = nullptr;
    HWND hBtnSymbol = nullptr;
    HWND hEditX = nullptr, hEditY = nullptr, hEditW = nullptr, hEditH = nullptr;
    HWND hComboGaugeType = nullptr;
    HWND hEditMin = nullptr, hEditMax = nullptr;
    HWND hEditMinCaut = nullptr, hEditMaxCaut = nullptr;
    HWND hEditMinNorm = nullptr, hEditMaxNorm = nullptr;
    HWND hEditStart = nullptr, hEditEnd = nullptr;
    HWND hEditFont = nullptr, hEditSteps = nullptr, hEditMtics = nullptr;
    HWND hEditScaleDiv = nullptr, hEditPrec = nullptr;
    HWND hEditNnl = nullptr, hEditNcl = nullptr, hEditAlpha = nullptr;
    HWND hAlphaSlider = nullptr;
    HWND hLabPreview = nullptr;
    HWND hPreviewSlider = nullptr;

    HWND hChkNumeric = nullptr, hChkScaleVal = nullptr;
    HWND hChkDispUnits = nullptr, hChkDispLabel = nullptr, hChkHor = nullptr;
    HWND hChkOrient = nullptr;
    HWND hChkMinThresh = nullptr, hChkMaxThresh = nullptr;
    HWND hChkBorder = nullptr;
    HWND hLabBorderStyle = nullptr;
    HWND hComboBorderStyle = nullptr;
    HWND hChkMinPeak = nullptr, hChkMaxPeak = nullptr, hChkPeakRed = nullptr;

    HWND hFgSwatch = nullptr, hBgSwatch = nullptr;
    HWND hFgBtn = nullptr, hBgBtn = nullptr;
    HWND hZoneSwatch[5] = {};
    HWND hZoneBtn[5] = {};
    HWND hOutInactSwatch = nullptr, hOutActSwatch = nullptr;
    HWND hOutInactBtn = nullptr, hOutActBtn = nullptr;

    HWND hDataLabChannel = nullptr;
    HWND hDataLabMult = nullptr, hDataLabMult2 = nullptr, hDataLabMultB = nullptr;
    HWND hDataLabUpdate = nullptr, hDataLabAv = nullptr, hDataLabWfr = nullptr, hDataLabCt = nullptr;
    HWND hDataEditChannel = nullptr;
    HWND hBtnChannel = nullptr;
    HWND hDataEditMult = nullptr, hDataEditMult2 = nullptr, hDataEditMultB = nullptr;
    HWND hComboUpdate = nullptr, hComboAv = nullptr, hComboWfr = nullptr;
    HWND hDataEditCt = nullptr;
    HWND hDataChkBm = nullptr;
    HWND hDataChkIsLed = nullptr, hDataChkRpm = nullptr;
    HWND hDataLabAdvanced = nullptr;

    struct SpinBuddy {
        HWND edit = nullptr;
        HWND minus = nullptr;
        HWND plus = nullptr;
        int lo = 0;
        int hi = 0;
        int editId = 0;
    };
    std::vector<SpinBuddy> spins_;
    std::vector<HWND> sectionLabels_;
    std::vector<std::pair<HWND, HWND>> checkCaps_;

    // Rounded well drawn on the parent; the EDIT sits inset inside it.
    struct FieldWell {
        HWND parent = nullptr;
        HWND edit = nullptr;
        RECT rc = {};
    };
    std::vector<FieldWell> fieldWells_;

    ULONG_PTR gdiplusToken = 0;
    Gdiplus::Image* backgroundImage = nullptr;

    HDC previewMemDC_ = nullptr;
    HBITMAP previewDib_ = nullptr;
    HBITMAP previewOldBmp_ = nullptr;
    int previewCacheW_ = 0;
    int previewCacheH_ = 0;

    LayoutDocument doc_;
    BackgroundResolver bgResolver_;
    LayoutRenderer renderer_;

    int selectedGauge = -1;
    int selectedOutput = -1;
    int selectedLabel = -1;
    PropMode propMode_ = PropMode::None;
    bool suppressLiveUpdate = false;

    // Unsaved document changes (title shows " *").
    bool docDirty_ = false;

    // Canvas drag-move state (armed only when clicking an already-selected widget)
    bool dragging_ = false;
    int dragStartMx_ = 0;
    int dragStartMy_ = 0;
    int dragOriginX_ = 0;
    int dragOriginY_ = 0;
    bool dragMoved_ = false;
    static constexpr int kDragSlopPx = 5;

    std::vector<PreviewHit> stackHits_;

    int PreviewX() const { return Insp::kPreviewX; }
    int PreviewY() const { return Insp::kPreviewY; }
    int PreviewW() const { return doc_.profile.canvasWidth; }
    int PreviewH() const { return doc_.profile.canvasHeight; }
    int InspectorY() const { return Insp::kInspY; }

    void Log(const std::string& text);
    void InvalidatePreview();
    void ReleasePreviewCache();
    bool EnsurePreviewCache(HDC hdc, int bw, int bh);
    void UpdateLayoutTitle();
    void UpdateWindowTitle();
    void ReloadBackgroundImage();

    void MarkDirty();
    void ClearDirty();
    // Returns true if caller may discard current doc (saved or user chose No).
    bool ConfirmDiscardOrSave();

    HWND MakeLabel(int x, int y, int w, int h, const wchar_t* text, bool rightAlign = true);
    HWND MakeEdit(int x, int y, int w, int h, int id, bool numberOnly);
    HWND MakeBuddyEdit(int x, int y, int w, int h, int id, int lo, int hi, bool numberOnly = true);
    HWND MakeCheck(int x, int y, int w, int h, int id, const wchar_t* text);
    HWND MakeCheckOn(HWND parent, int x, int y, int w, int h, int id, const wchar_t* text);
    void SetCheckCaption(HWND box, const wchar_t* text);
    HWND MakeCombo(int x, int y, int w, int h, int id);
    HWND MakeComboOn(HWND parent, int x, int y, int w, int h, int id);
    void RegisterFieldWell(HWND parent, HWND edit, int x, int y, int w, int h);
    void PaintFieldWells(HWND parent, HDC hdc);

    void CreateAppearancePanel();
    void CreateMoreSettingsHost();
    void ShowMoreSettings();
    void ShowSymbolPicker();
    void ShowChannelPicker();
    void ApplyUiFont(HWND h);
    void EnablePair(HWND lab, HWND edit, bool on);
    void NudgeSpin(HWND btn);
    static LRESULT CALLBACK BuddyBtnProc(HWND h, UINT msg, WPARAM wParam, LPARAM lParam,
        UINT_PTR id, DWORD_PTR ref);

    void SetPropMode(PropMode mode);
    void ApplyPropTabVisibility();
    void ApplyGaugeTypeVisibility(int gaugeType);
    void SyncBuddyUpDownVisibility();
    void ClearSelection();
    void ApplyPreviewHit(const PreviewHit& hit);
    bool PreviewHitIsSelected(const PreviewHit& hit) const;
    void ArmPreviewDrag(const PreviewHit& hit, int mx, int my);
    void ShowOverlapMenu(const std::vector<PreviewHit>& hits, int mx, int my);
    void ReorderGauge(int index, bool toFront);
    void ClearPropertyPanel();
    void PopulatePropertyPanel();
    void RebuildGaugeTypeCombo();
    bool ReadPropertiesIntoSelected(bool showError, bool applyComboType = false);
    void LiveUpdateFromPanel(bool applyComboType = false);
    void SyncPositionEditsFromModel();
    void SetTransparencyUi(int alpha);
    void SyncPreviewSliderFromRenderer();
    void ApplyPreviewSliderToRenderer();
    int GetTransparencyUi() const;

    void UpdateColorSwatches();
    bool PickColor(unsigned int& holleyColor);

    // Ensure at least one Layout screen exists (for Add on empty doc).
    bool EnsureActiveScreen();

    void LoadINI();
    bool SaveINI(); // true if user completed a successful save
    // Canvas-only PNG at 1280×480 (dash-comparable; no window chrome / selection).
    void SaveScreenshot();
    // Same PNG with a blue selection ring around every analogMeters gauge.
    void SavePositionScreenshot();
    void SaveCanvasPng(bool ringAllGauges);
    void LoadBackground();
    void ClearLayout();
    void OnScreenChanged();

    void AddGauge();
    void AddLabel();
    void AddOutput();
    void DuplicateSelected();
    void DeleteSelected();

    void EndDrag(bool commit);

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK MoreSettingsProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void CloseMoreSettings();
};
