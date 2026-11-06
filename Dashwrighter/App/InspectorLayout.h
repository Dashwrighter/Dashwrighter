#pragma once

// =============================================================================
// InspectorLayout.h — ALL inspector geometry (absolute client pixels)
//
// Edit this file to move or resize inspector controls. CreateAppearancePanel
// only reads these values.
//
// Coordinate space: main-window client pixels. Preview is fixed at
// (kPreviewX, kPreviewY) size 1280×480, so the inspector band starts at
// y = 544. If you change preview size, add the same delta to every y below.
//
// Ctrl:
//   x, y, w, h     = the control itself (edit well, 16×16 checkbox, slider,
//                    combo face, color chip, or "..." button)
//   labDx, labDy   = caption offset from the control's top-left
//                    (left captions are negative labDx; labels above use
//                    negative labDy). Tweaking these nudges the caption
//                    without moving the box.
//   labW           = caption *alignment slot* (0 = no caption). The STATIC is
//                    then shrunk to the text so unused slot space is not a
//                    window and cannot paint over a neighboring control.
//
// Buddy ± width (w) includes the minus/plus buttons.
// Combo drop-list height is TypeDropH / BorderStyleDropH, not Ctrl.h.
// =============================================================================

namespace Insp {

constexpr int kPreviewX = 12;
constexpr int kPreviewY = 58;
constexpr int kPreviewW = 1280;
constexpr int kPreviewH = 480;
constexpr int kInspGap  = 6;
constexpr int kInspY    = kPreviewY + kPreviewH + kInspGap; // 544
constexpr int kPad      = 12;
constexpr int kLabH     = 16;
constexpr int kClientW  = kPreviewX + kPreviewW + kPad;     // 1304
constexpr int kClientH  = 739;

struct Ctrl {
    int x, y, w, h;
    int labDx, labDy, labW;
};

// ---------------------------------------------------------------------------
// Type, name and footprint
// ---------------------------------------------------------------------------
//                          x    y    w   h   labDx labDy labW
constexpr Ctrl Type      {  47, 544, 214, 22,  -44,    4,  40 };
constexpr Ctrl Label     {  50, 573, 176, 22,  -74,    2,  70 };
constexpr Ctrl SymbolBtn { 230, 573,  32, 22,    0,    0,   0 };
constexpr Ctrl Units     {  50, 599,  45, 22,  -74,    2,  70 };
constexpr Ctrl X         {  50, 625, 65, 22,  -22,    2,  18 };
constexpr Ctrl Y         {  50, 651, 65, 22,  -22,    2,  18 };
constexpr Ctrl FontSize  { 180, 599,  45, 22,  -74,    2,  70 };
constexpr Ctrl Size      { 180, 625, 60, 22,  -40,    2,  36 };
constexpr Ctrl Height    { 180, 651,  60, 22,  -52,    2,  48 };
constexpr int TypeDropH = 200;

// ---------------------------------------------------------------------------
// Range, ticks, and precision
// ---------------------------------------------------------------------------
constexpr Ctrl Min       { 351+15, 573, 65, 22,  -40,    2,  36 };
constexpr Ctrl Max       { 351 + 15, 599, 65, 22,  -40,    2,  36 };
constexpr Ctrl StartAngle{ 351 + 15, 625,  65, 22,  -76,    2,  72 };
constexpr Ctrl EndAngle  { 351 + 15, 651,  65, 22,  -70,    2,  66 };
constexpr Ctrl Precision { 495 + 20, 573,  45, 22,  -72,    2,  68 };
constexpr Ctrl Segments  { 495 + 20, 625,  45, 22,  -76,    2,  72 };
constexpr Ctrl ScaleDiv  { 495 + 20, 599, 45, 22,  -76,    2,  72 };
constexpr Ctrl MinTicks  { 495 + 20, 651,  65, 22,  -70,    2,  66 };

// ---------------------------------------------------------------------------
// Checkboxes and borderstyle
// ---------------------------------------------------------------------------
constexpr Ctrl DispLabel { 605+50, 576,  16, 16,   20,   -1, 108 };
constexpr Ctrl DispUnits { 605 + 50, 601,  16, 16,   20,   -1, 108 };
constexpr Ctrl ValueDisp { 605 + 50, 626,  16, 16,   20,   -1, 108 };
constexpr Ctrl ScaleVal  { 705 + 50, 576,  16, 16,   20,   -1, 108 };
constexpr Ctrl Border    { 705 + 50, 601,  16, 16,   20,   -1, 108 };
constexpr Ctrl Vertical  { 705 + 50, 626,  16, 16,   20,   -1, 100 };
constexpr Ctrl BorderStyle{ 635 + 50, 650, 100, 22,  -40,    4,  36 };
constexpr int BorderStyleDropH = 120;

// ---------------------------------------------------------------------------
// RPM Bar only
// ---------------------------------------------------------------------------
constexpr Ctrl NormLeds  { 880 + 75, 573,  90, 22,  -76,    2,  72 };
constexpr Ctrl CautLeds  { 880 + 75, 599,  90, 22,  -70,    2,  66 };

// ---------------------------------------------------------------------------
// Look (style / preview / colors / alpha right)
// ---------------------------------------------------------------------------
constexpr Ctrl Preview     { 490, 544,  300, 22,  -52,    2,  48 };

constexpr Ctrl Fg        {865+75, 625,  22, 16,  -68,    0,  64 }; // caption: Fore / back
constexpr Ctrl FgBtn     {891+75, 625,  24, 20,    0,    0,   0 };
constexpr Ctrl Bg        {919+75, 625,  22, 16,    0,    0,   0 };
constexpr Ctrl BgBtn     {945+75, 625,  24, 20,    0,    0,   0 };

constexpr Ctrl Alpha     {855+75, 651,  75, 22,  -38,    2,  36 }; // caption: Transp
constexpr Ctrl AlphaEdit {930+75, 651,  80, 22,    0,    0,   0 };

// Output colors reuse Fore / Transp rows (shown instead of Fg/Bg / Alpha).
constexpr Ctrl OutInact    { 865 + 75, 625,  22, 16,  -68,    0,  64 };
constexpr Ctrl OutInactBtn { 891 + 75, 625,  24, 20,    0,    0,   0 };
constexpr Ctrl OutAct      { 919 + 115, 625,  22, 16,  -52,    0,  48 };
constexpr Ctrl OutActBtn   { 945 + 115, 625,  24, 20,    0,    0,   0 };

// ---------------------------------------------------------------------------
// Zone strip
// ---------------------------------------------------------------------------
constexpr Ctrl LowZones  { 200, 711,  16, 16,   20,   -1,  90 };
constexpr Ctrl HighZones { 1015, 711,  16, 16,   20,   -1,  90 };

struct ZoneSlot {
    Ctrl value; // w == 0 → no edit (Normal); caption still used
    Ctrl chip;
    Ctrl btn;
};

constexpr ZoneSlot LowAlarm    { { 285, 709, 73, 22, 0, -16, 120 }, { 360, 711, 18, 16, 0, 0, 0 }, { 380, 709, 24, 20, 0, 0, 0 } };
constexpr ZoneSlot LowWarning  { { 450, 709, 73, 22, 0, -16, 120 }, { 525, 711, 18, 16, 0, 0, 0 }, { 545, 709, 24, 20, 0, 0, 0 } };
constexpr ZoneSlot Normal      { { 620, 709,  0, 22, 0, -16, 120 }, { 620, 711, 18, 16, 0, 0, 0 }, { 640, 709, 24, 20, 0, 0, 0 } };
constexpr ZoneSlot HighWarning { { 710, 709, 73, 22, 0, -16, 120 }, { 786, 711, 18, 16, 0, 0, 0 }, { 805, 709, 24, 20, 0, 0, 0 } };
constexpr ZoneSlot HighAlarm   { { 870, 709, 73, 22, 0, -16, 120 }, { 945, 711, 18, 16, 0, 0, 0 }, { 965, 709, 24, 20, 0, 0, 0 } };

} // namespace Insp
