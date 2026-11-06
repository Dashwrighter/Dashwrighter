#pragma once

// =============================================================================
// LayoutTypes.h — Core data model for one Holley Pro Dash layout screen
//
// These structs mirror fields in Holley layout INI files (analogMeters, outputs,
// labels, [Defaults], [Layout]/[LayoutN]). Colors are Holley AARRGGBB:
//   high byte = alpha (255 opaque, 0 fully transparent), then R, G, B.
//
// Angle convention (User Manual p.34):
//   0° = 12 o'clock, positive = clockwise.
// =============================================================================

#include "GaugeType.h"
#include <set>
#include <string>
#include <vector>

// -----------------------------------------------------------------------------
// Gauge — one analogMeters\N\* block (the main widget type on a screen)
// -----------------------------------------------------------------------------
struct Gauge {
    // --- Identity ---
    // Caption for most types; for Symbol (8) this is the glyph string (decoded PUA).
    std::string label;
    std::string units;   // Unit string (e.g. "PSI"); unused on Symbol
    int channel = kHolleyBatteryChannel; // Battery; 0 = invalid on the dash

    // --- Geometry on the virtual canvas (pixels; may extend off-screen) ---
    int x = 0, y = 0;
    int width = 100, height = 100;

    // --- Value range and display formatting ---
    double minValue = 0.0;
    double maxValue = 100.0;
    double scalingDivisor = 1.0;   // Divides tick labels only (not the live value)
    int precision = 0;
    // Channel Math on the dash: value' = a*x^2 + b*x + c
    double valueMult = 1.0;        // b
    double valueMult2 = 0.0;       // a
    double valueMultB = 0.0;       // c
    bool scaleValues = false;      // Major ticks show scale numbers when true

    // --- Zone thresholds (c0–c4 coloring) ---
    // Dash UI: Low Alarm / Low Warning / High Warning / High Alarm + enables.
    double minCaution = 0.0;          // Low Alarm edge (below → c0)
    double maxCaution = 0.0;          // High Alarm edge (above → c4)
    double minNormal = 0.0;           // Low Warning / start of normal (c2)
    double maxNormal = 0.0;           // High Warning / end of normal (c2)
    bool minThresholdEnabled = false; // Low-side zones (c0/c1)
    bool maxThresholdEnabled = false; // High-side zones (c3/c4)

    // --- Peak hold (preview does not draw peak markers yet) ---
    bool minPeakEnabled = false;
    bool maxPeakEnabled = false;
    bool peakReductionEnabled = false;

    // --- On-gauge text toggles ---
    bool numericIndicatorEnabled = true; // Center / digital value
    bool displayUnits = false;
    bool displayLabel = false;           // Caption visibility (Symbol uses label as glyph)

    // --- Widget type and style flags ---
    int gaugeType = static_cast<int>(GaugeTypeId::Analog);
    bool isLED = false;          // setLEDEnabled (not Status LED type 5)
    bool orientation = false;    // Bargraph: vertical card; Status LED: Large Size (Holley)
    bool hor = false;            // Bargraph: true = horizontal (export/dash flag)
    bool rpmModeEnabled = false;
    bool crown = false;          // Border enable (INI crown); Digital = spine brackets
    bool bm = false;             // Face/background mode (opacity-related on dash)
    int wfr = 0;                 // Firmware int (files use 0, 1, 2, or true/false)
    // Pixel size (manual 5–240). >0 = explicit; ≤0 (−1) = auto.
    // On gauges: primarily the caption/label (not value/scale). Symbol: glyph size.
    int fontSize = -1;

    // --- Colors (Holley AARRGGBB) ---
    unsigned int foreground = 0xFFFFFFFF;
    unsigned int background = 0x00000000; // Alpha 0 = transparent
    unsigned int c0 = 0, c1 = 0, c2 = 0, c3 = 0, c4 = 0; // Zone colors

    // --- Geometry / segments ---
    int startAngle = -120;
    int endAngle = 120;
    int steps = 0;         // Type-dependent: majors, LED count, segments, grid
    int mtics = 0;         // Minor ticks (analog family)
    int nnl = 5;           // RPM bar: normal LED count
    int ncl = 5;           // RPM bar: caution LED count
    int av = 0;            // Averaging combo: 0 None … 8 Very Fast
    int updateRate = 6;    // 1–10 = N/sec; 0 = full rate
    bool avFlag = false;   // Editor-legacy; never written (Holley has no avFlag)
    int ct = 0;            // Border Style: 0 Original, 1 Black, 2 Grey, 3 Silver

    // Keys present at load; writer preserves optional keys for round-trip.
    std::set<std::string> originalKeys;
};

// -----------------------------------------------------------------------------
// Output — one outputs\N\* switch / digital output control
// -----------------------------------------------------------------------------
struct Output {
    std::string text;                    // Caption on the switch graphic
    int channel = 0;                     // Output channel id (non-visual for preview)
    int style = 1;                       // Holley switch style → size via GetSwitchSize
    int x = 0, y = 0;                    // Top-left on canvas
    unsigned int inactiveColor = 0xFF808080; // Color when off
    unsigned int activeColor = 0xFFFFFFFF;   // Color when on (preview uses active)
    unsigned int background = 0x00000000;
    int fontSize = 20;                   // Caption size in pixels (1:1)
    std::set<std::string> originalKeys;  // Keys seen at load time
};

// -----------------------------------------------------------------------------
// Label — free-floating text (Add → Label), not a gauge's caption
// -----------------------------------------------------------------------------
struct Label {
    std::string text;                    // Caption; INI key is label (not text)
    int x = 0, y = 0;
    int fontSize = 20;                   // Pixel size 1:1 (manual 5–240)
    unsigned int foreground = 0xFFFFFFFF;
    unsigned int background = 0x00000000;
    std::set<std::string> originalKeys;
};

// Pixel size of a switch graphic for a given Holley style id.
struct SwitchSize {
    int width;
    int height;
};

// Style → native hit-box from WidgetGraphics cell sizes (Switch-Style-Map.md).
// style 0–16 = all 17 menu styles (2026-08-07); unknown → default box.
inline SwitchSize GetSwitchSize(int style) {
    switch (style) {
    case 0: return { 80, 170 };   // Rocker
    case 1: return { 250, 100 };  // Slider
    case 2: return { 103, 204 };  // Rocker 2
    case 3: return { 150, 150 };  // Black Toggle
    case 4: return { 80, 80 };    // Black Toggle (small)
    case 5: return { 150, 150 };  // Silver Toggle
    case 6: return { 80, 80 };    // Silver Toggle (small)
    case 7: return { 150, 150 };  // Silver Momentary Switch (same art as 5)
    case 8: return { 80, 80 };    // Silver Momentary Switch (small)
    case 9: return { 80, 80 };    // Gray Momentary Button (small)
    case 10: return { 120, 120 }; // Gray Momentary Button
    case 11: return { 160, 160 }; // Gray Momentary Button (large)
    case 12: return { 80, 80 };   // Black Momentary Button (small)
    case 13: return { 120, 120 }; // Black Momentary Button
    case 14: return { 160, 160 }; // Black Momentary Button (large)
    case 15: return { 150, 150 }; // Black Momentary Switch (same art as 3)
    case 16: return { 80, 80 };   // Black Momentary Switch (small) (same art as 4)
    default: return { 100, 100 };
    }
}

// Short UI name for known soft-switch styles (nullptr if unknown).
// All 17 menu styles: style= 0–16 locked 2026-08-07.
inline const char* SwitchStyleName(int style) {
    switch (style) {
    case 0: return "Rocker";
    case 1: return "Slider";
    case 2: return "Rocker 2";
    case 3: return "Black Toggle";
    case 4: return "Black Toggle (small)";
    case 5: return "Silver Toggle";
    case 6: return "Silver Toggle (small)";
    case 7: return "Silver Momentary Switch";
    case 8: return "Silver Momentary Switch (small)";
    case 9: return "Gray Momentary Button (small)";
    case 10: return "Gray Momentary Button";
    case 11: return "Gray Momentary Button (large)";
    case 12: return "Black Momentary Button (small)";
    case 13: return "Black Momentary Button";
    case 14: return "Black Momentary Button (large)";
    case 15: return "Black Momentary Switch";
    case 16: return "Black Momentary Switch (small)";
    default: return nullptr;
    }
}

// -----------------------------------------------------------------------------
// LayoutDefaults — global [Defaults] section in multi-screen layout packs
// -----------------------------------------------------------------------------
struct LayoutDefaults {
    int brightness = 10;
    bool autoBrightness = false;
    int screen = 0;              // Startup / home screen index
    bool clockSetByGps = false;
    bool hdlLog = false;
    bool autoHomeScreen = false;
    int om = 0;                  // Operating Mode
    int numScreens = 1;
    bool present = false;        // true if a [Defaults] section was parsed
    std::set<std::string> originalKeys; // Keys present at load; empty → write full block
};

// -----------------------------------------------------------------------------
// LayoutScreen — one dash page: single-file [Layout] or multi [LayoutN]
// -----------------------------------------------------------------------------
struct LayoutScreen {
    std::string sectionName;          // "Layout" or "Layout0", "Layout1", …
    std::string backgroundPath;       // As stored in INI, e.g. /internal/images/foo.jpg
    std::string backgroundFileName;   // Basename only (foo.jpg) for local file lookup
    int version = 10;                 // Layout format version from INI when present
    std::vector<Gauge> gauges;
    std::vector<Output> outputs;
    std::vector<Label> labels;
};
