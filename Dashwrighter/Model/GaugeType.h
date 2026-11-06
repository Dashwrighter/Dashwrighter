#pragma once

// =============================================================================
// GaugeType.h — Holley analogMeters\N\gaugeType integer IDs
//
// All nine Pro Dash Add-Gauge types are known (see GaugeType-Enum.md).
// Unknown IDs still load/save; the editor draws a labeled placeholder.
// =============================================================================

#include <string>

// Strongly typed view of the INI gaugeType field (stored as int on Gauge).
enum class GaugeTypeId : int {
    Analog = 0,           // Round analog with needle
    Bargraph = 1,         // Horizontal or vertical bar card
    Digital = 2,          // Numeric digital card (UI: "Digital")
    // 3 unused
    ThinAnalog = 4,       // Open arc fill (no needle)
    StatusLED = 5,        // Status indicator LED
    RpmBar = 6,           // 15-LED strip; INI 750×100, paint 80px; LOCKED 2026-08-28
    // 7 unused
    Symbol = 8,           // Font glyph via label (PUA / Sym keyboard)
    Graph = 9,            // Strip chart
    // 10 unused
    BargraphStyle2 = 11,  // LOCKED 2026-09-19 (H-style2-orient)
    // Not separate types: AFR Meter, TPI, GPS Map, G Meter — ordinary gauges
    // (usually Digital) with a label, or non-analogMeters objects.
};

// Dash Customize default channel (Battery). New gauges must not ship channel=0
// ("invalid channel" on the dash). Preserve whatever the INI already has.
constexpr int kHolleyBatteryChannel = 41;

// Short English name for logs and the property panel (never null).
const char* GaugeTypeName(int gaugeType);

// True if gaugeType is one of the nine known create-menu IDs.
bool GaugeTypeIsKnown(int gaugeType);

// Inspector Customize matrix (`Dash-Customize-Control-Matrix.csv`).
// Unknown gaugeType: every cap returns true (leave editable).
// Size-policy helpers below are independent (painter / NormalizeGaugeGeometry).
enum class CustomizeCap : int {
    Label = 0,
    FontSize,
    Units,
    Precision,
    Size,
    ScalingDivisor,
    Range,
    ZoneValues,
    LowThreshEnable,
    HighThreshEnable,
    LowZoneColors,
    NormalZoneColor,
    HighZoneColors,
    PeakHold,
    ValueDisplay,
    Channel,
    Foreground,
    Background,
    Sweep,
    Steps,
    Mtics,
    Border,
    BorderStyle,
    RpmMode,
    DisplayUnits,
    DisplayLabel,
    ChannelMath,
    ScaleValues,
    UpdateRate,
    Averaging,
    RpmLeds,
    COUNT
};
bool GaugeCustomizeHas(int gaugeType, CustomizeCap cap);

// Capability helpers. Inspector ones wrap GaugeCustomizeHas.
bool GaugeTypeUsesAngles(int gaugeType);      // Analog / ThinAnalog sweeps
bool GaugeTypeUsesHor(int gaugeType);          // type 1 Bargraph Vertical / hor only (not Style 2)
bool GaugeTypeUsesStyle2Vertical(int gaugeType); // type 11: editor Vertical; dash Customize has no row
bool GaugeTypeUsesRpmLeds(int gaugeType);      // RPM bar nnl/ncl
bool GaugeTypeUsesAnalogTicks(int gaugeType);  // face ticks (type 0 paint); not inspector Scale Div
bool GaugeTypeUsesScaleValues(int gaugeType);  // Analog, Bargraph, Style 2, Thin Analog, Graph
bool GaugeTypeUsesMtics(int gaugeType);        // same types as Scale values (Customize Minor Tics)
bool GaugeTypeUsesSteps(int gaugeType);        // Analog, Bargraph, Style 2, Thin Analog, Graph
bool GaugeTypeUsesZoneColors(int gaugeType);   // any of c0–c4
bool GaugeTypeUsesScalingDivisor(int gaugeType);
// Dash Customize size policy (2026-08-07 / T5 LED 2026-08-08):
//   0/4 square Size; 2 Digital Size=height+aspect; 1 Bargraph width-only;
//   5 Status LED Normal/Large only; 6 RPM fixed; 11 free W×H.
bool GaugeTypeUsesSquareSize(int gaugeType);     // Analog, Thin Analog
bool GaugeTypeUsesDigitalSize(int gaugeType);     // Digital: single Size, aspect from digits
bool GaugeTypeUsesBargraphWidthOnly(int gaugeType); // type 1: length only; thickness from fontSize
bool GaugeTypeUsesStatusLedSize(int gaugeType);    // type 5: Normal or Large only
bool GaugeTypeUsesFixedSize(int gaugeType);       // RPM Bar: fixed 750×100, 15 LEDs
bool GaugeTypeUsesSymbolHeight(int gaugeType);    // type 8: size= is hit box only; glyph from fontSize
// Border Style combo (INI ct): Analog only. Thin Analog has Border, no Style.
bool GaugeTypeUsesBorderStyle(int gaugeType);
bool GaugeTypeUsesUpdateRate(int gaugeType);
bool GaugeTypeUsesAveraging(int gaugeType);

// INI integer + dash combo label. updateRate: 1–10 = N/sec, 0 = full rate.
// av: sequential 0–8 (None…Very Fast). 0 = None is proven; 1–8 follow combo order.
struct GaugeModeChoice {
    int value;
    const wchar_t* label;
};
inline constexpr GaugeModeChoice kUpdateRateModes[] = {
    {1, L"1/sec"}, {2, L"2/sec"}, {3, L"3/sec"}, {4, L"4/sec"}, {5, L"5/sec"},
    {6, L"6/sec"}, {7, L"7/sec"}, {8, L"8/sec"}, {9, L"9/sec"}, {10, L"10/sec"},
    {0, L"full rate"},
};
inline constexpr GaugeModeChoice kAveragingModes[] = {
    {0, L"None"}, {1, L"Slow"}, {2, L"180"}, {3, L"Medium"}, {4, L"120"},
    {5, L"90"}, {6, L"60"}, {7, L"Fast"}, {8, L"Very Fast"},
};
// RPM Steady warning color → wfr (H-rpmbar-wfr-and-avg.ini 2026-09-19).
inline constexpr GaugeModeChoice kWfrModes[] = {
    {0, L"Steady warning color"},
    {1, L"Flash rate slow"},
    {2, L"Flash rate quick"},
};
template <int N>
constexpr int GaugeModeCount(const GaugeModeChoice (&)[N]) { return N; }

// Type 1 bargraph card — LOCKED 2026-08-29.
// Size is length only. Thickness always reserves the label band, even if the
// label (and therefore the spine) is off. Scale numerals paint outside.
//   thickness = topPad + labelPx + label–bar pad + 25px bar
constexpr int kBargraphBarPx = 25;
constexpr int kBargraphEndPadPx = 4;         // empty pad each end of the length
constexpr int kBargraphHookWidthPx = 2;
constexpr int kBargraphSpinePx = 1;
constexpr int kBargraphHookBarGapPx = 6;     // hooks stop this far from the bar
constexpr int kBargraphLabelTopPadPx = 5;    // P1 fs=18: spine at emCenter-2
constexpr int kBargraphLabelBarPadPx = 9;    // P1 fs=18: bar at y=32
int BargraphLabelPx(const struct Gauge& g);
int BargraphCardThickness(const struct Gauge& g);
// Offsets from the top (horizontal) / left (vertical) of the card.
int BargraphSpineOffset(const struct Gauge& g);
int BargraphBarOffset(const struct Gauge& g);

// Type 11 Style 2 — LOCKED 2026-09-19 (H-style2-orient).
// Free W×H trapezoid. Start height = 50% of end. Spine follows the label
// only (crown ignored; Digital is the type that gates spine on Border).
// Label-off: 2px top + 8px bottom (end height = H-10). High side at y=2.
// Label-on: bar top = BargraphBarOffset, 3px bottom (end = H-offset-3).
constexpr int kStyle2LabelOffTopPadPx = 2;
constexpr int kStyle2LabelOffBotPadPx = 8;
constexpr int kStyle2LabelOnBotPadPx = 3;
constexpr int kStyle2SidePadPx = 2;
// Status LED: Holley Size Normal / Large → shaded sphere, 45px / 90px.
// TX-LED_test: Normal ~(44,63) orientation=false; Large ~(90,109) orientation=true.
// RW-6.86-SNIPER N2O size=(89,66) orientation=false → Normal on dash.
// Large is NOT "any max(w,h)≥90" — dash Size menu writes orientation + footprint.
constexpr int kStatusLedNormalPx = 45;
constexpr int kStatusLedLargePx = 90;
// P5 label band — LOCKED 2026-08-29 (user accepted close enough).
// Label-off: LED top = pos.y. Label-on: cap top = pos.y+8; LED top = that + cap.
// Cap = round(0.72×em), same ratio as Digital. FS18 stays 8+13=21.
// AUTO em = 12. Dash clips the LED to the stored box (44×63 / 90×109).
constexpr int kStatusLedLabelTopPx = 8;
constexpr int kStatusLedAutoEmPx = 12;
constexpr float kStatusLedCapPerEm = 0.72f;
constexpr int kRpmBarDefaultW = 750;       // factory Racepak-style footprint
constexpr int kRpmBarDefaultH = 100;       // INI / hit box (dash writes 100)
constexpr int kRpmBarPaintH = 80;          // classic plate + LED layout; flush with pos.y
constexpr int kRpmLedCount = 15;           // 15-slot grid (target mode omits unlit)

// Status LED Normal vs Large (Holley Size). Prefer orientation= (TX-LED_test).
bool StatusLedIsLarge(const struct Gauge& g);
int StatusLedDiameterPx(const struct Gauge& g); // 45 (Normal) or 90 (Large)
int StatusLedLabelEmPx(const struct Gauge& g);  // fontSize, or 12 if AUTO
int StatusLedLabelCapPx(const struct Gauge& g); // round(0.72×em); FS18 = 13
int StatusLedPaintTopOffset(const struct Gauge& g); // 0 if label off; 8+cap if on

// Apply dash size policy after the user changes type (not on INI load).
// Square → W=H; Digital → aspect from digit budget; Bargraph1 → font-derived thickness;
// RPM → fixed 750×100 and steps/led count normalized.
void NormalizeGaugeGeometry(struct Gauge& g);

// Dash-policy draw/hit size without mutating the gauge (copy + Normalize).
// Types without a size policy keep stored W×H. Always returns at least 4×4.
void EffectiveGaugeSize(const struct Gauge& g, int& outW, int& outH);

// Digital size — LOCKED 2026-08-27 (P2 vs Compare.xmcd):
//   Size = height (INI width is not trustworthy — dash has no Width control).
//   Value cap 0.714×H (labeled: minus 0.72×fontSize+5); W = 8px pads + advances/1.08.
int DigitalWidthForHeight(int height, const struct Gauge& g);
int DigitalDigitBudget(const struct Gauge& g);
// Value/units path aspect W/H (digits + optional units). Label path is separate.
double DigitalAspectForGauge(const struct Gauge& g);
// Min spine stub each side when Border (crown) is on — also in label width budget.
constexpr int kDigitalSpineMinStubPx = 6;

// RPM Bar LED color bands (15-LED classic: left→right normal / caution / high).
// Dash evidence (T6 2026-08-07): on-screen bands follow nnl/ncl. Thresholds are
// used only when nnl and ncl are both 0 (factory files that omit them).
void RpmBarBandCounts(const struct Gauge& g, int& normalLeds, int& cautionLeds);
// Target RPM mode (P6 dash 2026-08-28): type 6 and rpmModeEnabled.
// Not high-side warnings. Setpoint/range live in minNormal/minCaution and do not
// select the mode.
bool RpmBarIsShiftLight(const struct Gauge& g);
// Fill span: min/max, extended by enabled threshold points that sit outside.
void RpmBarFillRange(const struct Gauge& g, double& low, double& high);
