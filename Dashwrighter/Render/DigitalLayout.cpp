// =============================================================================
// DigitalLayout.cpp — Digital text layout (Compare.xmcd 2026-08-25)
// =============================================================================

#include "DigitalLayout.h"
#include "PreviewFont.h"
#include "../Model/GaugeType.h"
#include "../Io/IniUtils.h"

#include <windows.h>
#include <gdiplus.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

#pragma comment(lib, "gdiplus.lib")

namespace {

int ClampPx(int px) {
    if (px < 1) return 1;
    if (px > 240) return 240;
    return px;
}

int ResolveLabelPx(int fontSize, int autoPx) {
    if (fontSize > 0)
        return ClampPx(fontSize);
    return ClampPx(autoPx);
}

int MeasureWide(const std::wstring& s, int px) {
    if (s.empty() || px < 1)
        return 0;
    Gdiplus::Bitmap bmp(8, 8, PixelFormat32bppARGB);
    Gdiplus::Graphics gfx(&bmp);
    gfx.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);
    const Gdiplus::FontFamily& family = PreviewFont::Family();
    if (family.GetLastStatus() != Gdiplus::Ok)
        return 0;
    Gdiplus::Font font(&family, static_cast<Gdiplus::REAL>(px),
        Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
    Gdiplus::StringFormat fmt(Gdiplus::StringFormat::GenericTypographic());
    fmt.SetFormatFlags(
        Gdiplus::StringFormatFlagsNoWrap | Gdiplus::StringFormatFlagsNoClip);
    Gdiplus::RectF box;
    gfx.MeasureString(s.c_str(), -1, &font, Gdiplus::PointF(0.0f, 0.0f), &fmt, &box);
    if (box.Width <= 0.0f)
        return 0;
    return static_cast<int>(std::ceil(box.Width));
}

std::string FormatDigital(double v, int precision) {
    char buf[64];
    if (precision <= 0)
        sprintf_s(buf, "%.0f", v);
    else
        sprintf_s(buf, "%.*f", precision, v);
    return std::string(buf);
}

} // namespace

std::wstring DigitalUtf8ToWide(const std::string& s) {
    return IniUtils::Utf8ToWide(s);
}

std::string DigitalWorstCaseValue(const Gauge& g) {
    const std::string a = FormatDigital(g.minValue, g.precision);
    const std::string b = FormatDigital(g.maxValue, g.precision);
    return (a.size() >= b.size()) ? a : b;
}

std::wstring DigitalDisplayUnitsW(const Gauge& g) {
    // Whitespace-only units (Cutlass Trip 1 quoted padding) paint as none.
    if (!g.displayUnits || IniUtils::IsWhitespaceOnly(g.units))
        return std::wstring();
    std::wstring u = DigitalUtf8ToWide(g.units);
    if (u.size() == 1 && u[0] >= L'A' && u[0] <= L'Z')
        u[0] = static_cast<wchar_t>(u[0] - L'A' + L'a');
    return u;
}

DigitalTextLayout ComputeDigitalTextLayout(int height, const Gauge& g) {
    DigitalTextLayout L;
    if (height < 1)
        height = 1;
    L.pad = kDigitalPadPx;
    L.rightPad = kDigitalRightPadPx;
    const int autoLab = height < 50 ? 10 : 12;
    L.labelPx = ResolveLabelPx(g.fontSize, autoLab);
    L.labelCapPx = static_cast<int>(L.labelPx * kDigitalCapPerEm + 0.5f);

    const bool hasLabel = g.displayLabel && !g.label.empty();
    if (hasLabel) {
        // Cap top at 8px; band is that inset plus capital ink (for spine Y).
        L.spineBand = kDigitalLabelTopPx + L.labelCapPx;
        L.spineGap = 0;
    }

    const int capUnl = static_cast<int>(kDigitalValueCapFrac * static_cast<double>(height) + 0.5);
    int capPx = capUnl;
    if (hasLabel)
        capPx = capUnl - (L.labelCapPx + kDigitalLabelValuePadPx);

    L.valueCapPx = capPx;
    if (capPx < 4) {
        // Dash F labeled: value is pushed off the plate. Do not clamp a ghost em.
        L.valuePx = 0;
        L.unitsPx = 0;
        L.valueBandH = 0;
        return L;
    }

    L.valuePx = static_cast<int>(static_cast<float>(capPx) / kDigitalCapPerEm + 0.5f);
    if (L.valuePx < 1)
        L.valuePx = 1;

    L.valueBandH = capPx + kDigitalValueBottomPx;
    constexpr int kUnitsPx = 15;
    L.unitsPx = kUnitsPx;
    if (L.unitsPx > L.valuePx - 2)
        L.unitsPx = (std::max)(1, L.valuePx - 2);

    return L;
}

int DigitalWidthForHeight(int height, const Gauge& g) {
    // Plate W from measured value+units (and label), not H×aspect.
    if (height < 1)
        height = 1;
    const DigitalTextLayout L = ComputeDigitalTextLayout(height, g);

    const std::wstring wVal = DigitalUtf8ToWide(DigitalWorstCaseValue(g));
    const std::wstring wUnits = DigitalDisplayUnitsW(g);
    const int vW = MeasureWide(wVal, L.valuePx);
    const int uW = MeasureWide(wUnits, L.unitsPx);
    const int gap = (!wVal.empty() && !wUnits.empty()) ? 1 : 0;
    // Squeeze value advances only (units extra width already matched P2).
    const int vWFit = (vW > 0)
        ? static_cast<int>(static_cast<float>(vW) / kDigitalWidthAdvanceScale + 0.5f)
        : 0;
    int wValue = L.pad + vWFit + gap + uW + L.rightPad;
    if (vW <= 0 && uW <= 0) {
        // GDI+ unavailable — last-resort digit aspect (old formula, no 1-char units).
        const int digits = DigitalDigitBudget(g);
        double aspect = 0.40 + 0.34 * static_cast<double>(digits);
        const int ulen = static_cast<int>(wUnits.size());
        if (ulen >= 2)
            aspect += 0.12 + 0.06 * static_cast<double>((std::min)(ulen, 6));
        if (aspect < 1.05) aspect = 1.05;
        if (aspect > 3.8) aspect = 3.8;
        wValue = static_cast<int>(std::lround(height * aspect));
    }

    int wLabel = 0;
    if (g.displayLabel && !g.label.empty()) {
        const int labInk = MeasureWide(DigitalUtf8ToWide(g.label), L.labelPx);
        constexpr int kGapEach = 6;
        const int hooks = g.crown ? (2 * kDigitalSpineMinStubPx) : 0;
        const int ink = (labInk > 0) ? labInk : L.labelPx;
        wLabel = 2 * L.pad + hooks + 2 * kGapEach + ink;
    }

    int w = (std::max)(wValue, wLabel);
    if (w < 40)
        w = 40;
    // Never trust INI width. Dash Size is height only; stored W is a side-effect.
    return w;
}
