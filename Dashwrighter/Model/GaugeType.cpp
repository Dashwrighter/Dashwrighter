// =============================================================================
// GaugeType.cpp — Name lookup, known-ID tests, and type capability helpers
// =============================================================================

#include "GaugeType.h"
#include "LayoutTypes.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

const char* GaugeTypeName(int gaugeType) {
    switch (gaugeType) {
    case 0: return "Analog";
    case 1: return "Bargraph";
    case 2: return "Digital";
    case 4: return "Thin Analog";
    case 5: return "Status LED";
    case 6: return "RPM Bar";
    case 8: return "Symbol";
    case 9: return "Graph";
    case 11: return "Bargraph Style 2";
    default: return "Unknown";
    }
}

bool GaugeTypeIsKnown(int gaugeType) {
    switch (gaugeType) {
    case 0: case 1: case 2: case 4: case 5: case 6:
    case 8: case 9: case 11:
        return true;
    default:
        return false;
    }
}

// Columns match Dash-Customize-Control-Matrix.csv gauge types:
// Analog, Bargraph, Style 2, Digital, Thin Analog, RPM Bar, Status LED, Symbol, Graph
static int CustomizeColumn(int gaugeType) {
    switch (gaugeType) {
    case 0:  return 0;
    case 1:  return 1;
    case 11: return 2;
    case 2:  return 3;
    case 4:  return 4;
    case 6:  return 5;
    case 5:  return 6;
    case 8:  return 7;
    case 9:  return 8;
    default: return -1;
    }
}

bool GaugeCustomizeHas(int gaugeType, CustomizeCap cap) {
    const int col = CustomizeColumn(gaugeType);
    if (col < 0) return true;
    const int row = static_cast<int>(cap);
    if (row < 0 || row >= static_cast<int>(CustomizeCap::COUNT)) return true;
    static const bool kMat[static_cast<int>(CustomizeCap::COUNT)][9] = {
        // A  B S2  D  T RP LED Sy  G
        /* Label */            {1,1,1,1,1, 0, 1,1,1},
        /* FontSize */         {1,1,1,1,1, 0, 1,1,1},
        /* Units */            {1,1,1,1,1, 0, 0,0,1},
        /* Precision */        {1,1,1,1,1, 0, 0,0,1},
        /* Size */             {1,1,1,1,1, 0, 0,0,1},
        /* ScalingDivisor */   {1,1,1,0,1, 0, 0,0,1},
        /* Range */            {1,1,1,1,1, 1, 1,1,1},
        /* ZoneValues */       {1,1,1,1,1, 1, 1,1,1},
        /* LowThreshEnable */  {1,1,1,1,1, 0, 1,1,1},
        /* HighThreshEnable */ {1,1,1,1,1, 0, 1,1,1},
        /* LowZoneColors */    {1,1,1,1,1, 0, 1,1,1},
        /* NormalZoneColor */  {1,1,1,1,1, 0, 1,1,1},
        /* HighZoneColors */   {1,1,1,1,1, 1, 1,1,1},
        /* PeakHold */         {1,0,0,0,1, 0, 0,0,0},
        /* ValueDisplay */     {1,1,1,0,1, 0, 0,0,0},
        /* Channel */          {1,1,1,1,1, 1, 1,1,1},
        /* Foreground */       {1,1,1,1,1, 0, 1,0,1},
        /* Background */       {1,1,1,1,1, 1, 0,0,1},
        /* Sweep */            {1,0,0,0,1, 0, 0,0,0},
        /* Steps */            {1,1,1,0,1, 0, 0,0,1},
        /* Mtics */            {1,1,1,0,1, 0, 0,0,1},
        /* Border */           {1,1,0,1,1, 0, 0,0,0},
        /* BorderStyle */      {1,0,0,0,0, 0, 0,0,0},
        /* RpmMode */          {0,0,0,0,0, 1, 0,0,0},
        /* DisplayUnits */     {1,1,1,1,1, 0, 0,0,1},
        /* DisplayLabel */     {1,1,1,1,1, 0, 1,0,1},
        /* ChannelMath */      {1,1,1,1,1, 1, 0,1,1},
        /* ScaleValues */      {1,1,1,0,1, 0, 0,0,1},
        /* UpdateRate */       {1,1,1,1,1, 0, 0,0,0},
        /* Averaging */        {1,1,1,1,1, 1, 0,0,1},
        /* RpmLeds */          {0,0,0,0,0, 1, 0,0,0},
    };
    static_assert(sizeof(kMat) / sizeof(kMat[0]) == static_cast<int>(CustomizeCap::COUNT),
        "kMat rows must match CustomizeCap");
    return kMat[row][col] != 0;
}

bool GaugeTypeUsesAngles(int gaugeType) {
    return GaugeCustomizeHas(gaugeType, CustomizeCap::Sweep);
}

bool GaugeTypeUsesHor(int gaugeType) {
    // Type 1 only. Style 2 uses orientation (Vertical checkbox) but not hor.
    return gaugeType == 1;
}

bool GaugeTypeUsesStyle2Vertical(int gaugeType) {
    return gaugeType == 11;
}

bool GaugeTypeUsesRpmLeds(int gaugeType) {
    return GaugeCustomizeHas(gaugeType, CustomizeCap::RpmLeds);
}

bool GaugeTypeUsesAnalogTicks(int gaugeType) {
    // Major/minor face ticks (type 0 paint). Inspector Scale Div uses ScalingDivisor.
    return gaugeType == 0;
}

bool GaugeTypeUsesScaleValues(int gaugeType) {
    return GaugeCustomizeHas(gaugeType, CustomizeCap::ScaleValues);
}

bool GaugeTypeUsesMtics(int gaugeType) {
    return GaugeCustomizeHas(gaugeType, CustomizeCap::Mtics);
}

bool GaugeTypeUsesSteps(int gaugeType) {
    return GaugeCustomizeHas(gaugeType, CustomizeCap::Steps);
}

bool GaugeTypeUsesZoneColors(int gaugeType) {
    return GaugeCustomizeHas(gaugeType, CustomizeCap::LowZoneColors)
        || GaugeCustomizeHas(gaugeType, CustomizeCap::NormalZoneColor)
        || GaugeCustomizeHas(gaugeType, CustomizeCap::HighZoneColors);
}

bool GaugeTypeUsesScalingDivisor(int gaugeType) {
    return GaugeCustomizeHas(gaugeType, CustomizeCap::ScalingDivisor);
}

bool GaugeTypeUsesSquareSize(int gaugeType) {
    // Dash Customize: single Size for round gauges (not free W×H).
    // Status LED is fixed footprint — not square Size.
    return gaugeType == 0 || gaugeType == 4;
}

bool GaugeTypeUsesDigitalSize(int gaugeType) {
    return gaugeType == 2;
}

bool GaugeTypeUsesBargraphWidthOnly(int gaugeType) {
    return gaugeType == 1;
}

int BargraphLabelPx(const Gauge& g) {
    if (g.fontSize > 0) {
        int px = g.fontSize;
        if (px < 1) px = 1;
        if (px > 240) px = 240;
        return px;
    }
    return 12; // AUTO — not derived from card H (that would be circular)
}

int BargraphCardThickness(const Gauge& g) {
    // Always includes label band + pad + 25px bar, even when the label is off.
    return kBargraphLabelTopPadPx + BargraphLabelPx(g)
        + kBargraphLabelBarPadPx + kBargraphBarPx;
}

int BargraphSpineOffset(const Gauge& g) {
    // 2px above the vertical center of the font (label band starts after top pad).
    const int labelPx = BargraphLabelPx(g);
    int spine = kBargraphLabelTopPadPx + labelPx / 2 - 2;
    if (spine < 0) spine = 0;
    return spine;
}

int BargraphBarOffset(const Gauge& g) {
    return kBargraphLabelTopPadPx + BargraphLabelPx(g) + kBargraphLabelBarPadPx;
}

bool GaugeTypeUsesStatusLedSize(int gaugeType) {
    return gaugeType == 5;
}

bool GaugeTypeUsesFixedSize(int gaugeType) {
    return gaugeType == 6;
}

bool GaugeTypeUsesSymbolHeight(int gaugeType) {
    return gaugeType == 8;
}

bool GaugeTypeUsesBorderStyle(int gaugeType) {
    return GaugeCustomizeHas(gaugeType, CustomizeCap::BorderStyle);
}

bool GaugeTypeUsesUpdateRate(int gaugeType) {
    return GaugeCustomizeHas(gaugeType, CustomizeCap::UpdateRate);
}

bool GaugeTypeUsesAveraging(int gaugeType) {
    return GaugeCustomizeHas(gaugeType, CustomizeCap::Averaging);
}

int DigitalDigitBudget(const Gauge& g) {
    // Character count of formatted min/max (precision included).
    // T2-digital-aspect: p0 "100"=3, p1 "100.0"=5, p3 "100.000"=7.
    auto countChars = [&](double v) -> int {
        char buf[64];
        if (g.precision <= 0)
            sprintf_s(buf, "%.0f", v);
        else
            sprintf_s(buf, "%.*f", g.precision, v);
        return static_cast<int>(std::strlen(buf));
    };
    int digits = (std::max)(countChars(g.minValue), countChars(g.maxValue));
    if (g.maxPeakEnabled || g.minPeakEnabled)
        digits = (std::max)(digits, countChars(g.maxValue));
    if (digits < 1) digits = 1;
    if (digits > 12) digits = 12;
    return digits;
}

double DigitalAspectForGauge(const Gauge& g) {
    // Value/units path only (not label). Base: T2-digital-aspect 0.40+0.34×digits.
    // 2026-08-17 eval: Holley does not widen for 1-char units ("v", "%");
    // multi-letter (MPH/deg/rpm) still add the 2026-08-10 term.
    const int digits = DigitalDigitBudget(g);
    double aspect = 0.40 + 0.34 * static_cast<double>(digits);
    bool unitsInk = false;
    for (unsigned char c : g.units) {
        if (c != ' ' && c != '\t') { unitsInk = true; break; }
    }
    if (g.displayUnits && unitsInk) {
        const int ulen = static_cast<int>(g.units.size());
        if (ulen >= 2)
            aspect += 0.12 + 0.06 * static_cast<double>((std::min)(ulen, 6));
    }
    if (aspect < 1.05) aspect = 1.05;
    if (aspect > 3.8) aspect = 3.8;
    return aspect;
}

bool StatusLedIsLarge(const Gauge& g) {
    // Holley Customize Size: Normal / Large (not "Small").
    // TX-LED_test: Large writes orientation=true + size~(90,109);
    // Normal writes orientation=false + size~(44,63). SNIPER N2O (89,66) is Normal.
    return g.orientation;
}

int StatusLedDiameterPx(const Gauge& g) {
    return StatusLedIsLarge(g) ? kStatusLedLargePx : kStatusLedNormalPx;
}

int StatusLedLabelEmPx(const Gauge& g) {
    if (g.fontSize > 0) {
        int px = g.fontSize;
        if (px < 1) px = 1;
        if (px > 240) px = 240;
        return px;
    }
    return kStatusLedAutoEmPx;
}

int StatusLedLabelCapPx(const Gauge& g) {
    int cap = static_cast<int>(static_cast<float>(StatusLedLabelEmPx(g)) * kStatusLedCapPerEm + 0.5f);
    if (cap < 1)
        cap = 1;
    return cap;
}

int StatusLedPaintTopOffset(const Gauge& g) {
    if (!(g.displayLabel && !g.label.empty()))
        return 0;
    return kStatusLedLabelTopPx + StatusLedLabelCapPx(g);
}

void NormalizeGaugeGeometry(Gauge& g) {
    if (GaugeTypeUsesStatusLedSize(g.gaugeType)) {
        // Dash: Normal or Large only. Keep factory hit-box aspect when present;
        // paint diameter from Size (orientation). LED top = pos.y (label-on: +8+cap).
        // Do not force square size= — dash keeps (44,63) / (90,109).
        (void)g;
        return;
    }
    if (GaugeTypeUsesSquareSize(g.gaugeType)) {
        const int side = (g.width > 0) ? g.width : g.height;
        g.width = side > 0 ? side : 100;
        g.height = g.width;
        return;
    }
    if (GaugeTypeUsesDigitalSize(g.gaugeType)) {
        // Size = height. Width measured from value/units/label — never INI W.
        int sizeH = g.height;
        if (sizeH <= 0)
            sizeH = (g.width > 0) ? g.width / 2 : 72;
        g.height = sizeH > 0 ? sizeH : 72;
        g.width = DigitalWidthForHeight(g.height, g);
        return;
    }
    if (GaugeTypeUsesBargraphWidthOnly(g.gaugeType)) {
        // Size = length. Thickness = reserved label band + pad + 25px bar.
        // Scale paints outside. Vertical is the same card rotated 90° CCW.
        const int thick = BargraphCardThickness(g);
        if (g.orientation) {
            const int length = (g.height > 0) ? g.height : ((g.width > 0) ? g.width : 200);
            g.height = length > 0 ? length : 200;
            g.width = thick;
        } else {
            const int length = (g.width > 0) ? g.width : 200;
            g.width = length;
            g.height = thick;
        }
        return;
    }
    if (GaugeTypeUsesFixedSize(g.gaugeType)) {
        g.width = kRpmBarDefaultW;
        g.height = kRpmBarDefaultH;
        g.steps = kRpmLedCount;
        return;
    }
}

void EffectiveGaugeSize(const Gauge& g, int& outW, int& outH) {
    Gauge tmp = g;
    NormalizeGaugeGeometry(tmp);
    // Style 2: INI size= stays W×H. orientation=true paints a 90° CCW card
    // whose along and thickness are both H (honor H-style2-orient: 280×120
    // paints ~H×H at pos=, not W×H swapped).
    if (tmp.gaugeType == 11 && tmp.orientation) {
        const int side = tmp.height < 4 ? 4 : tmp.height;
        outW = side;
        outH = side;
        return;
    }
    outW = tmp.width < 4 ? 4 : tmp.width;
    outH = tmp.height < 4 ? 4 : tmp.height;
}

void RpmBarBandCounts(const Gauge& g, int& normalLeds, int& cautionLeds) {
    constexpr int N = kRpmLedCount;
    // Durable on-dash color counts. T6 proof: rows with nnl=5/ncl=5 painted 5/5/5
    // even with rpmMode + maxNormal/maxCaution set; nnl=8/ncl=4 painted 8/4/3.
    const bool countsPresent = (g.nnl > 0 || g.ncl > 0);
    if (countsPresent) {
        normalLeds = g.nnl > 0 ? g.nnl : 0;
        cautionLeds = g.ncl > 0 ? g.ncl : 0;
    } else {
        // Do not use rpmModeEnabled — dash paint is nnl/ncl (default 5/5).
        normalLeds = 5;
        cautionLeds = 5;
    }
    if (normalLeds < 0) normalLeds = 0;
    if (cautionLeds < 0) cautionLeds = 0;
    if (normalLeds > N) normalLeds = N;
    if (normalLeds + cautionLeds > N) {
        cautionLeds = N - normalLeds;
        if (cautionLeds < 0) {
            normalLeds = N;
            cautionLeds = 0;
        }
    }
}

bool RpmBarIsShiftLight(const Gauge& g) {
    return g.gaugeType == 6 && g.rpmModeEnabled;
}

void RpmBarFillRange(const Gauge& g, double& low, double& high) {
    low = g.minValue;
    high = g.maxValue;
    if (g.minThresholdEnabled) {
        low = (std::min)(low, (std::min)(g.minNormal, g.minCaution));
    }
    if (g.maxThresholdEnabled) {
        high = (std::max)(high, (std::max)(g.maxNormal, g.maxCaution));
    }
    if (!(high > low)) {
        low = g.minValue;
        high = g.maxValue;
    }
}
