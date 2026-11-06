// =============================================================================
// LayoutRenderer.cpp — Preview drawing for gauges, labels, and switch outputs
//
// Structure:
//   1) Color / math helpers (Holley AARRGGBB, angles, demo value)
//   2) Draw() + hit-tests with canvas clip
//   3) Per-type gauge painters (Analog, Bargraph, Digital, …)
//
// Holley AARRGGBB: high byte = alpha (255 opaque, 0 fully transparent).
// Matches User Manual p.36 and factory INIs — no forced-opaque remapping.
// =============================================================================

#include "LayoutRenderer.h"
#include "DigitalLayout.h"
#include "PreviewFont.h"
#include "resource.h"
#include "../Model/GaugeType.h"
#include "../Io/IniUtils.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <cstdint>
#include <map>
#include <objbase.h>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "ole32.lib")

// ---------------------------------------------------------------------------
// Font size policy (TL14 + field notes)
//
// Holley fontSize is a pixel size (manual 5–240). On free Labels it is 1:1.
// On gauges it primarily sizes the *caption/label*, not the numeric value:
//   • Digital / Analog / ThinAnalog / Bargraph / Style2 / Graph:
//       label uses fontSize when > 0; value is layout / auto-fit.
//   • Status LED: LOCKED 2026-08-29. label cap = round(0.72×fontSize); AUTO = 12. LED drop = 8+cap.
//   • Symbol: the glyph *is* the label → fontSize sizes the glyph 1:1.
//   • fontSize <= 0 (often −1): AUTO — modest layout-derived size for labels;
//     digital value stays large auto-fit (TL14 FSauto).
// Scale numerals are never driven by fontSize (radius / card geometry).
// Types without a locked model still follow this rule (label ← FS, value ← layout)
// so behavior is consistent until on-dash evidence says otherwise.
//
// Face: Roboto Regular 1.200310 (2013) — LOCKED 2026-09-19. Symbol atlas
// PUA glyphs stay the dash-capture PNG; Latin fallback on Symbol uses Roboto.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Color / geometry helpers
// ---------------------------------------------------------------------------

void LayoutRenderer::UnpackHolley(unsigned int c, BYTE& a, BYTE& r, BYTE& g, BYTE& b) {
    a = (c >> 24) & 0xFF;
    r = (c >> 16) & 0xFF;
    g = (c >> 8) & 0xFF;
    b = c & 0xFF;
}

Gdiplus::Color LayoutRenderer::HolleyToGdip(unsigned int c) {
    BYTE a, r, g, b;
    UnpackHolley(c, a, r, g, b);
    return Gdiplus::Color(a, r, g, b);
}

// GDI pens/text ignore alpha; zero color falls back to a visible gray.
COLORREF LayoutRenderer::HolleyToColor(unsigned int c) {
    BYTE a, r, g, b;
    UnpackHolley(c, a, r, g, b);
    (void)a;
    if (c == 0) return RGB(180, 180, 180);
    return RGB(r, g, b);
}

// Pick c0…c4 based on where value sits relative to caution/normal bands.
// minThresholdEnabled / maxThresholdEnabled gate low-side and high-side zone colors.
// Used by Digital/Analog arcs and Symbol glyph tint (preview slider → DemoValue).
COLORREF LayoutRenderer::ZoneColorForValue(const Gauge& g, double value) {
    if (g.minThresholdEnabled) {
        if (value < g.minCaution) return HolleyToColor(g.c0);
        if (value < g.minNormal) return HolleyToColor(g.c1);
    }
    if (g.maxThresholdEnabled) {
        if (value > g.maxCaution) return HolleyToColor(g.c4);
        if (value > g.maxNormal) return HolleyToColor(g.c3);
    }
    // Normal band (or thresholds disabled). Prefer white over green for Symbol/UI
    // when c2 is unset so glyphs stay visible on dark wallpaper.
    if (g.c2)
        return HolleyToColor(g.c2);
    return RGB(230, 230, 230);
}

// Preview value from demoNorm (0=min … 1=max). Editor slider drives demoNorm.
double LayoutRenderer::DemoValue(const Gauge& g) const {
    double t = demoNorm;
    if (t < 0.0) t = 0.0;
    if (t > 1.0) t = 1.0;
    return g.minValue + t * (g.maxValue - g.minValue);
}

// Holley: 0° at 12 o'clock, positive clockwise → screen dx/dy.
void LayoutRenderer::AngleToDir(double angleDeg, double& dx, double& dy) {
    const double PI = 3.14159265358979323846;
    double mathRad = (90.0 - angleDeg) * PI / 180.0;
    dx = std::cos(mathRad);
    dy = -std::sin(mathRad);
}

// Linear map min→max onto startAngle→endAngle. Negative (end-start) reverses
// the needle (e.g. default5 GPS Speed startAngle=180 endAngle=0).
double LayoutRenderer::ValueToAngle(const Gauge& g, double value) {
    if (g.maxValue <= g.minValue) return static_cast<double>(g.startAngle);
    double t = (value - g.minValue) / (g.maxValue - g.minValue);
    if (t < 0.0) t = 0.0;
    if (t > 1.0) t = 1.0;
    return g.startAngle + t * (g.endAngle - g.startAngle);
}

bool LayoutRenderer::RectsIntersect(int ax, int ay, int aw, int ah, int bx, int by, int bw, int bh) {
    if (aw < 0) { ax += aw; aw = -aw; }
    if (ah < 0) { ay += ah; ah = -ah; }
    return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}

bool LayoutRenderer::PointInRect(int px, int py, int x, int y, int w, int h) {
    if (w < 0) { x += w; w = -w; }
    if (h < 0) { y += h; h = -h; }
    return px >= x && px < x + w && py >= y && py < y + h;
}

void LayoutRenderer::FillHolleyBackground(HDC hdc, int left, int top, int w, int h,
                                          unsigned int holleyBg) const {
    BYTE a, r, g, b;
    UnpackHolley(holleyBg, a, r, g, b);
    if (a == 0) return; // fully transparent — no fill
    Gdiplus::Graphics gfx(hdc);
    gfx.SetSmoothingMode(Gdiplus::SmoothingModeNone);
    Gdiplus::SolidBrush br(Gdiplus::Color(a, r, g, b));
    gfx.FillRectangle(&br, left, top, w, h);
}

// ---------------------------------------------------------------------------
// Main draw + hit-test (canvas clip)
// ---------------------------------------------------------------------------

// Paint background then all widgets; GDI clip crops off-canvas geometry.
void LayoutRenderer::Draw(HDC hdc, const LayoutScreen& screen, Gdiplus::Image* background,
                          int canvasW, int canvasH) const {
    PreviewFont::EnsureLoaded();
    // Clip all preview content to the canvas so off-screen gauges are cropped.
    HRGN clip = CreateRectRgn(originX, originY, originX + canvasW, originY + canvasH);
    int saved = SaveDC(hdc);
    SelectClipRgn(hdc, clip);

    // Always GDI-fill first so an empty layout (background, no widgets) still
    // has a destination for GDI+ DrawImage on the preview DIB.
    RECT prev = { originX, originY, originX + canvasW, originY + canvasH };
    FillRect(hdc, &prev, (HBRUSH)GetStockObject(BLACK_BRUSH));
    if (background) {
        Gdiplus::Graphics graphics(hdc);
        graphics.SetClip(Gdiplus::Rect(originX, originY, canvasW, canvasH));
        graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
        graphics.DrawImage(background, originX, originY, canvasW, canvasH);
        graphics.Flush(Gdiplus::FlushIntentionSync);
    }

    for (size_t i = 0; i < screen.gauges.size(); ++i)
        DrawGauge(hdc, screen.gauges[i], ringAllGauges || static_cast<int>(i) == selectedGauge);
    for (size_t i = 0; i < screen.labels.size(); ++i)
        DrawLabelItem(hdc, screen.labels[i], static_cast<int>(i) == selectedLabel);
    for (size_t i = 0; i < screen.outputs.size(); ++i)
        DrawOutput(hdc, screen.outputs[i], static_cast<int>(i) == selectedOutput);

    RestoreDC(hdc, saved);
    DeleteObject(clip);
}

static bool PointInEllipse(int px, int py, int x, int y, int w, int h) {
    if (w < 1 || h < 1) return false;
    const float cx = static_cast<float>(x) + w * 0.5f;
    const float cy = static_cast<float>(y) + h * 0.5f;
    const float rx = w * 0.5f;
    const float ry = h * 0.5f;
    const float dx = (static_cast<float>(px) - cx) / rx;
    const float dy = (static_cast<float>(py) - cy) / ry;
    return dx * dx + dy * dy <= 1.0f;
}

bool LayoutRenderer::GaugeHitsPoint(const Gauge& g, int px, int py, int canvasW, int canvasH) const {
    int hx = g.x, hy = g.y, hw = 0, hh = 0;
    EffectiveGaugeSize(g, hw, hh);
    if (!RectsIntersect(hx, hy, hw, hh, 0, 0, canvasW, canvasH))
        return false;
    if (!PointInRect(px, py, hx, hy, hw, hh))
        return false;
    // Analog / Thin Analog: outer ring is the size= circle; square corners pass through.
    if (g.gaugeType == 0 || g.gaugeType == 4)
        return PointInEllipse(px, py, hx, hy, hw, hh);
    return true;
}

// Topmost gauge under the cursor (iterate reverse). Point must be on-canvas.
int LayoutRenderer::HitTestGauge(const LayoutScreen& screen, int mx, int my, int canvasW, int canvasH) const {
    std::vector<PreviewHit> stack;
    HitTestStack(screen, mx, my, canvasW, canvasH, stack);
    for (const auto& h : stack) {
        if (h.kind == PreviewHitKind::Gauge)
            return h.index;
    }
    return -1;
}

void LayoutRenderer::HitTestStack(const LayoutScreen& screen, int mx, int my,
    int canvasW, int canvasH, std::vector<PreviewHit>& out) const {
    out.clear();
    int px = mx - originX;
    int py = my - originY;
    if (px < 0 || py < 0 || px >= canvasW || py >= canvasH)
        return;

    // Reverse draw order: outputs, then labels, then gauges (last painted is topmost).
    for (int i = static_cast<int>(screen.outputs.size()) - 1; i >= 0; --i) {
        const Output& o = screen.outputs[i];
        SwitchSize sz = GetSwitchSize(o.style);
        if (!RectsIntersect(o.x, o.y, sz.width, sz.height, 0, 0, canvasW, canvasH))
            continue;
        if (PointInRect(px, py, o.x, o.y, sz.width, sz.height))
            out.push_back({ PreviewHitKind::Output, i });
    }
    for (int i = static_cast<int>(screen.labels.size()) - 1; i >= 0; --i) {
        const Label& lab = screen.labels[i];
        int fs = PreviewFont::Resolve(lab.fontSize, 20);
        int lw = static_cast<int>(lab.text.size()) * fs / 2 + 8;
        int lh = fs + 8;
        if (!RectsIntersect(lab.x, lab.y, lw, lh, 0, 0, canvasW, canvasH))
            continue;
        if (PointInRect(px, py, lab.x, lab.y, lw, lh))
            out.push_back({ PreviewHitKind::Label, i });
    }
    for (int i = static_cast<int>(screen.gauges.size()) - 1; i >= 0; --i) {
        if (GaugeHitsPoint(screen.gauges[i], px, py, canvasW, canvasH))
            out.push_back({ PreviewHitKind::Gauge, i });
    }
}

// Topmost switch under the cursor (style → size via GetSwitchSize).
int LayoutRenderer::HitTestOutput(const LayoutScreen& screen, int mx, int my, int canvasW, int canvasH) const {
    int px = mx - originX;
    int py = my - originY;
    if (px < 0 || py < 0 || px >= canvasW || py >= canvasH) return -1;
    for (int i = static_cast<int>(screen.outputs.size()) - 1; i >= 0; --i) {
        const Output& o = screen.outputs[i];
        SwitchSize sz = GetSwitchSize(o.style);
        if (!RectsIntersect(o.x, o.y, sz.width, sz.height, 0, 0, canvasW, canvasH))
            continue;
        if (PointInRect(px, py, o.x, o.y, sz.width, sz.height))
            return i;
    }
    return -1;
}

// Approximate text hit box from fontSize × character count.
int LayoutRenderer::HitTestLabel(const LayoutScreen& screen, int mx, int my, int canvasW, int canvasH) const {
    int px = mx - originX;
    int py = my - originY;
    if (px < 0 || py < 0 || px >= canvasW || py >= canvasH) return -1;
    for (int i = static_cast<int>(screen.labels.size()) - 1; i >= 0; --i) {
        const Label& lab = screen.labels[i];
        int fs = PreviewFont::Resolve(lab.fontSize, 20);
        int w = static_cast<int>(lab.text.size()) * fs / 2 + 8;
        int h = fs + 8;
        if (!RectsIntersect(lab.x, lab.y, w, h, 0, 0, canvasW, canvasH))
            continue;
        if (PointInRect(px, py, lab.x, lab.y, w, h))
            return i;
    }
    return -1;
}

// Route to the painter for gaugeType (unknown IDs get a labeled placeholder).
// Digital / Bargraph1 / Status LED / square / RPM sizes come from EffectiveGaugeSize
// (dash policy), not raw INI W×H.
void LayoutRenderer::DrawGauge(HDC hdc, const Gauge& g, bool selected) const {
    int left = originX + g.x;
    int top = originY + g.y;
    int w = 0, h = 0;
    EffectiveGaugeSize(g, w, h);

    switch (g.gaugeType) {
    case 0: DrawAnalog(hdc, g, left, top, w, h, selected); break;
    case 1: DrawBargraph(hdc, g, left, top, w, h, selected); break;
    case 2: DrawDigital(hdc, g, left, top, w, h, selected); break;
    case 4: DrawThinAnalog(hdc, g, left, top, w, h, selected); break;
    case 5: DrawStatusLED(hdc, g, left, top, w, h, selected); break;
    case 6: DrawRPMBar(hdc, g, left, top, w, h, selected); break;
    case 8: DrawSymbol(hdc, g, left, top, w, h, selected); break;
    case 9: DrawGraph(hdc, g, left, top, w, h, selected); break;
    case 11: DrawBargraphStyle2(hdc, g, left, top, w, h, selected); break;
    default: DrawUnknown(hdc, g, left, top, w, h, selected); break;
    }
}

// Soft-switch preview: blit Off/On frame from WidgetGraphics 3-frame strip.
// Switch preview: Off/On strip blit. Locked paint.
// Body art is never recolored; only plate (background) and label colors apply.
// Frame order: 0 Off, 1 On, 2 Disabled (Disabled unused — channel is runtime).
// Preview slider (demoNorm): < 0.5 → Off, ≥ 0.5 → On (all switches flip together).
void LayoutRenderer::DrawOutput(HDC hdc, const Output& o, bool selected) const {
    SwitchSize sz = GetSwitchSize(o.style);
    const int left = originX + o.x;
    const int top = originY + o.y;
    int w = sz.width;
    int h = sz.height;

    // Shared with gauges: Appearance preview slider drives demoNorm 0..1.
    const bool active = (demoNorm >= 0.5);
    const int frameIndex = active ? 1 : 0;

    Gdiplus::Graphics gfx(hdc);
    gfx.SetSmoothingMode(Gdiplus::SmoothingModeNone); // crisp sprites
    gfx.SetInterpolationMode(Gdiplus::InterpolationModeNearestNeighbor);
    gfx.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
    gfx.SetCompositingMode(Gdiplus::CompositingModeSourceOver);

    Gdiplus::Image* sheet = GetSwitchSheetImage(o.style);
    bool blitted = false;
    if (sheet) {
        const int sheetW = static_cast<int>(sheet->GetWidth());
        const int sheetH = static_cast<int>(sheet->GetHeight());
        if (sheetW >= 3 && sheetH >= 1) {
            // Three equal horizontal frames (Off | On | Disabled).
            const int frameW = sheetW / 3;
            const int frameH = sheetH;
            const int srcX = frameIndex * frameW;
            if (frameW > 0) w = frameW;
            if (frameH > 0) h = frameH;
            // Optional plate behind the control (Customize Soft Switch → Background).
            FillHolleyBackground(hdc, left, top, w, h, o.background);
            Gdiplus::Rect dest(left, top, w, h);
            gfx.DrawImage(sheet, dest, srcX, 0, frameW, frameH, Gdiplus::UnitPixel);
            blitted = true;
        }
    }
    if (!blitted)
        FillHolleyBackground(hdc, left, top, w, h, o.background);

    if (!blitted) {
        // Fallback provisional chrome when assets missing (unknown style / no WidgetGraphics).
        unsigned int holley = active
            ? (o.activeColor ? o.activeColor : 0xFFFFFFFFu)
            : (o.inactiveColor ? o.inactiveColor : 0xFF666666u);
        BYTE ir, ig, ib, ia;
        UnpackHolley(holley, ia, ir, ig, ib);
        Gdiplus::SolidBrush body(Gdiplus::Color(220, ir, ig, ib));
        Gdiplus::Pen rim(Gdiplus::Color(180, 90, 90, 100), 1.5f);
        float m = static_cast<float>((w < h ? w : h) - 6);
        if (m < 8.0f) m = 8.0f;
        float cx = static_cast<float>(left) + (w - m) * 0.5f;
        float cy = static_cast<float>(top) + (h - m) * 0.5f;
        gfx.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        gfx.FillEllipse(&body, cx, cy, m, m);
        gfx.DrawEllipse(&rim, cx, cy, m, m);
    }

    if (selected)
        DrawSelectionRect(hdc, left, top, w, h, 2);

    // Caption follows Off/On: inactive vs active label color.
    if (!o.text.empty()) {
        const unsigned int labCol = active
            ? (o.activeColor ? o.activeColor : 0xFFFFFFFFu)
            : (o.inactiveColor ? o.inactiveColor : 0xFF808080u);
        BYTE a, r, g, b;
        UnpackHolley(labCol, a, r, g, b);
        if (a < 200) a = 255;
        int fs = PreviewFont::Resolve(o.fontSize, 14);
        if (fs > 22) fs = 22;
        Gdiplus::Graphics gfx(hdc);
        gfx.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAliasGridFit);
        const Gdiplus::FontFamily& family = PreviewFont::Family();
        Gdiplus::Font font(&family, static_cast<Gdiplus::REAL>(fs),
            Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
        Gdiplus::SolidBrush br(Gdiplus::Color(a, r, g, b));
        Gdiplus::StringFormat fmt(Gdiplus::StringFormat::GenericTypographic());
        fmt.SetAlignment(Gdiplus::StringAlignmentNear);
        fmt.SetLineAlignment(Gdiplus::StringAlignmentNear);
        fmt.SetFormatFlags(
            Gdiplus::StringFormatFlagsNoWrap | Gdiplus::StringFormatFlagsNoClip);
        const std::wstring w = IniUtils::Utf8ToWide(o.text);
        gfx.DrawString(w.c_str(), -1, &font,
            Gdiplus::PointF(static_cast<Gdiplus::REAL>(left),
                static_cast<Gdiplus::REAL>(top - fs - 2)), &fmt, &br);
    }
}

// Free-floating label: text + FG + BG (+ fontSize / pos). Field: very simple.
// fontSize is 1:1 pixel size (TL14 labels row).
void LayoutRenderer::DrawLabelItem(HDC hdc, const Label& lab, bool selected) const {
    int left = originX + lab.x;
    int top = originY + lab.y;
    int fs = PreviewFont::Resolve(lab.fontSize, 20);
    const int pad = 2;

    Gdiplus::Graphics gfx(hdc);
    gfx.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAliasGridFit);
    const Gdiplus::FontFamily& family = PreviewFont::Family();
    Gdiplus::Font font(&family, static_cast<Gdiplus::REAL>(fs),
        Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
    Gdiplus::StringFormat fmt(Gdiplus::StringFormat::GenericTypographic());
    fmt.SetAlignment(Gdiplus::StringAlignmentNear);
    fmt.SetLineAlignment(Gdiplus::StringAlignmentNear);
    fmt.SetFormatFlags(
        Gdiplus::StringFormatFlagsNoWrap | Gdiplus::StringFormatFlagsNoClip);
    const std::wstring w = IniUtils::Utf8ToWide(lab.text);
    Gdiplus::RectF sz;
    gfx.MeasureString(w.c_str(), -1, &font, Gdiplus::PointF(0, 0), &fmt, &sz);
    const int tw = (std::max)(1, static_cast<int>(std::ceil(sz.Width)));
    const int th = (std::max)(1, static_cast<int>(std::ceil(sz.Height)));

    BYTE a, r, g, b;
    UnpackHolley(lab.background, a, r, g, b);
    if (a > 0) {
        Gdiplus::SolidBrush br(Gdiplus::Color(a, r, g, b));
        gfx.FillRectangle(&br, left - pad, top - pad, tw + pad * 2, th + pad * 2);
    }

    BYTE fa, fr, fg, fb;
    UnpackHolley(lab.foreground ? lab.foreground : 0xFFFFFFFFu, fa, fr, fg, fb);
    if (fa < 200) fa = 255;
    Gdiplus::SolidBrush textBr(Gdiplus::Color(fa, fr, fg, fb));
    gfx.DrawString(w.c_str(), -1, &font,
        Gdiplus::PointF(static_cast<Gdiplus::REAL>(left), static_cast<Gdiplus::REAL>(top)),
        &fmt, &textBr);
    if (selected) {
        HPEN pen = CreatePen(PS_SOLID, 2, RGB(0, 180, 255));
        HPEN old = (HPEN)SelectObject(hdc, pen);
        HBRUSH oldB = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Rectangle(hdc, left - pad - 1, top - pad - 1, left + tw + pad + 1, top + th + pad + 1);
        SelectObject(hdc, old);
        SelectObject(hdc, oldB);
        DeleteObject(pen);
    }
}

// ---------------------------------------------------------------------------
// Needles / ticks / arcs (shared by Analog and ThinAnalog)
// ---------------------------------------------------------------------------

// Filled needle for the main analog value indicator (elliptical OK).
void LayoutRenderer::DrawNeedle(HDC hdc, int cx, int cy, int rx, int ry,
                                double angleDeg, COLORREF color) const {
    if (rx < 2) rx = 2;
    if (ry < 2) ry = 2;
    double dx, dy;
    AngleToDir(angleDeg, dx, dy);
    double px = -dy, py = dx;
    const double rAvg = 0.5 * (rx + ry);
    auto pt = [&](double along, double side) -> POINT {
        POINT p;
        p.x = cx + (int)(along * rx * dx + side * rAvg * px);
        p.y = cy + (int)(along * ry * dy + side * rAvg * py);
        return p;
    };
    POINT pts[4];
    // Tip at along=1 so needleRx/Ry is the painted length (honor inner-tick meet).
    pts[0] = pt(1.00, -0.016);
    pts[1] = pt(1.00, 0.016);
    pts[2] = pt(0.02, 0.048);
    pts[3] = pt(0.02, -0.048);
    HPEN hPen = CreatePen(PS_SOLID, 1, color);
    HBRUSH hBrush = CreateSolidBrush(color);
    HPEN oldP = (HPEN)SelectObject(hdc, hPen);
    HBRUSH oldB = (HBRUSH)SelectObject(hdc, hBrush);
    Polygon(hdc, pts, 4);
    SelectObject(hdc, oldP);
    SelectObject(hdc, oldB);
    DeleteObject(hPen);
    DeleteObject(hBrush);
}

// Colored arc sectors for warn/alarm zones only (software face).
// c0/c1 (low) and c3/c4 (high) draw when the matching threshold is enabled.
// **Normal band (c2) does not draw an arc** — dash leaves that span empty (2026-08-07).
// Thickness/placement from TL08 dash: fat outer rim band (~12% of min radius).
// steps does NOT gate this — dash still draws warn/alarm arcs when steps=0.
// rx/ry allow elliptical faces (dash respects non-square size= boxes).
void LayoutRenderer::DrawZoneArcBars(HDC hdc, const Gauge& g, int cx, int cy,
    float outerRx, float outerRy, float barHeight) const {
    if (g.maxValue <= g.minValue) return;
    if (outerRx < 4.f) outerRx = 4.f;
    if (outerRy < 4.f) outerRy = 4.f;
    if (barHeight < 2.f) barHeight = 2.f;
    Gdiplus::Graphics gfx(hdc);
    gfx.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    const float innerRx = (std::max)(2.f, outerRx - barHeight);
    const float innerRy = (std::max)(2.f, outerRy - barHeight);

    auto valueToAngle = [&](double val) -> double {
        double t = (val - g.minValue) / (g.maxValue - g.minValue);
        if (t < 0) t = 0;
        if (t > 1) t = 1;
        return g.startAngle + t * (g.endAngle - g.startAngle);
    };

    auto drawSector = [&](double valLo, double valHi, unsigned int color) {
        if (valHi <= valLo) return;
        BYTE a, r, gr, b;
        UnpackHolley(color, a, r, gr, b);
        if (color == 0) { a = 180; r = gr = b = 180; }
        else if (a == 0) a = 255;
        double t0 = (valLo - g.minValue) / (g.maxValue - g.minValue);
        double t1 = (valHi - g.minValue) / (g.maxValue - g.minValue);
        if (t0 < 0) t0 = 0; if (t1 > 1) t1 = 1;
        if (t1 <= t0) return;
        double ang0 = g.startAngle + t0 * (g.endAngle - g.startAngle);
        double ang1 = g.startAngle + t1 * (g.endAngle - g.startAngle);
        float startGdi = (float)(ang0 - 90.0);
        float sweepGdi = (float)(ang1 - ang0);
        Gdiplus::GraphicsPath path;
        Gdiplus::RectF outerRect(cx - outerRx, cy - outerRy, outerRx * 2, outerRy * 2);
        Gdiplus::RectF innerRect(cx - innerRx, cy - innerRy, innerRx * 2, innerRy * 2);
        path.AddArc(outerRect, startGdi, sweepGdi);
        path.AddArc(innerRect, startGdi + sweepGdi, -sweepGdi);
        path.CloseFigure();
        Gdiplus::SolidBrush brush(Gdiplus::Color(a, r, gr, b));
        gfx.FillPath(&brush, &path);
    };

    // Warn/alarm only — no normal (c2) arc bar.
    if (g.minThresholdEnabled) {
        drawSector(g.minValue, g.minCaution, g.c0);
        drawSector(g.minCaution, g.minNormal, g.c1);
    }
    if (g.maxThresholdEnabled) {
        drawSector(g.maxNormal, g.maxCaution, g.c3);
        drawSector(g.maxCaution, g.maxValue, g.c4);
    }

    // Small white tips only on zone-arc extremities (dash). Do NOT cap full-scale
    // startAngle/endAngle — that painted giant white "ticks" at 2 and 8.
    {
        const float midRx = 0.5f * (outerRx + innerRx);
        const float midRy = 0.5f * (outerRy + innerRy);
        const float halfLen = barHeight * 0.28f;
        Gdiplus::Pen capPen(Gdiplus::Color(255, 255, 255, 255),
            (std::max)(1.5f, barHeight * 0.18f));
        capPen.SetStartCap(Gdiplus::LineCapFlat);
        capPen.SetEndCap(Gdiplus::LineCapFlat);
        auto drawCap = [&](double angleDeg) {
            double dx, dy;
            AngleToDir(angleDeg, dx, dy);
            float x0 = (float)cx + (midRx - halfLen) * (float)dx;
            float y0 = (float)cy + (midRy - halfLen) * (float)dy;
            float x1 = (float)cx + (midRx + halfLen) * (float)dx;
            float y1 = (float)cy + (midRy + halfLen) * (float)dy;
            gfx.DrawLine(&capPen, x0, y0, x1, y1);
        };
        if (g.minThresholdEnabled) {
            drawCap(valueToAngle(g.minValue));
            drawCap(valueToAngle(g.minNormal));
        }
        if (g.maxThresholdEnabled) {
            drawCap(valueToAngle(g.maxNormal));
            drawCap(valueToAngle(g.maxValue));
        }
    }
}

// Major/minor tick marks; optional scaled labels when scaleValues is set.
// TL08 / RW-6.86 G1: short ticks on the zone-arc rim (not long spokes into the face).
// Start/end majors match intermediate majors (same pen + length).
// Scale numerals: center of text bounds on the radial point (not left/top of string).
// steps<=0: no ticks (but zone arc still draws — see DrawZoneArcBars).
void LayoutRenderer::DrawAnalogTicks(HDC hdc, const Gauge& g, int cx, int cy,
    float tickOuterRx, float tickOuterRy, float majorLen, float minorLen,
    float majorPenW, float minorPenW, int layoutR) const {
    if (g.steps <= 0) return;
    if (tickOuterRx < 4.f) tickOuterRx = 4.f;
    if (tickOuterRy < 4.f) tickOuterRy = 4.f;
    if (majorLen < 3.f) majorLen = 3.f;
    if (minorLen < 2.f) minorLen = 2.f;
    int majorCount = g.steps;
    int minorsPerMajor = (g.mtics > 0) ? g.mtics : 0;
    int totalTicks = majorCount * (minorsPerMajor + 1);
    if (totalTicks <= 0) return;

    const float majorInnerRx = (std::max)(2.f, tickOuterRx - majorLen);
    const float majorInnerRy = (std::max)(2.f, tickOuterRy - majorLen);
    const float minorInnerRx = (std::max)(2.f, tickOuterRx - minorLen);
    const float minorInnerRy = (std::max)(2.f, tickOuterRy - minorLen);

    BYTE ta, tr, tg, tb;
    UnpackHolley(g.foreground ? g.foreground : 0xFFC8C8C8u, ta, tr, tg, tb);
    if (ta < 200) ta = 255;

    Gdiplus::Graphics gfx(hdc);
    gfx.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    gfx.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAliasGridFit);
    Gdiplus::Pen majorPen(Gdiplus::Color(ta, tr, tg, tb), (std::max)(1.f, majorPenW));
    Gdiplus::Pen minorPen(Gdiplus::Color(ta, tr, tg, tb), (std::max)(1.f, (std::min)(2.f, minorPenW)));
    majorPen.SetLineCap(Gdiplus::LineCapFlat, Gdiplus::LineCapFlat, Gdiplus::DashCapFlat);
    minorPen.SetLineCap(Gdiplus::LineCapFlat, Gdiplus::LineCapFlat, Gdiplus::DashCapFlat);

    // Layout scale from the size= box, not the bezel hole (S0 dash cap ~13/15/9).
    if (layoutR < 4) layoutR = 4;
    const int scalePx = (std::max)(12, (int)(layoutR * 0.19 + 0.5));
    const Gdiplus::FontFamily& family = PreviewFont::Family();
    Gdiplus::Font scaleFont(&family, static_cast<Gdiplus::REAL>(scalePx),
        Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
    Gdiplus::SolidBrush textBrush(Gdiplus::Color(ta, tr, tg, tb));
    Gdiplus::StringFormat centerFmt;
    centerFmt.SetAlignment(Gdiplus::StringAlignmentCenter);
    centerFmt.SetLineAlignment(Gdiplus::StringAlignmentCenter);
    centerFmt.SetFormatFlags(Gdiplus::StringFormatFlagsNoWrap | Gdiplus::StringFormatFlagsNoClip);

    auto toWide = [](const char* s) -> std::wstring {
        return IniUtils::Utf8ToWide(s ? s : "");
    };

    for (int i = 0; i <= totalTicks; ++i) {
        double t = (double)i / (double)totalTicks;
        double angle = g.startAngle + t * (g.endAngle - g.startAngle);
        double value = g.minValue + t * (g.maxValue - g.minValue);
        double dx, dy;
        AngleToDir(angle, dx, dy);
        bool isMajor = (minorsPerMajor == 0) || (i % (minorsPerMajor + 1) == 0);
        const float iRx = isMajor ? majorInnerRx : minorInnerRx;
        const float iRy = isMajor ? majorInnerRy : minorInnerRy;
        float x0 = (float)(cx + tickOuterRx * dx);
        float y0 = (float)(cy + tickOuterRy * dy);
        float x1 = (float)(cx + iRx * dx);
        float y1 = (float)(cy + iRy * dy);
        Gdiplus::Pen* pen = isMajor ? &majorPen : &minorPen;
        gfx.DrawLine(pen, x0, y0, x1, y1);

        if (isMajor && g.scaleValues) {
            double display = value;
            if (g.scalingDivisor != 0.0 && g.scalingDivisor != 1.0)
                display = value / g.scalingDivisor;
            char buf[32];
            if (g.precision <= 0) sprintf_s(buf, "%.0f", display);
            else sprintf_s(buf, "%.*f", g.precision, display);
            float axR = majorInnerRx - 0.9f * static_cast<float>(scalePx);
            float ayR = majorInnerRy - 0.9f * static_cast<float>(scalePx);
            if (axR < 0.35f * tickOuterRx) axR = 0.35f * tickOuterRx;
            if (ayR < 0.35f * tickOuterRy) ayR = 0.35f * tickOuterRy;
            const float ax = static_cast<float>(cx + axR * dx);
            const float ay = static_cast<float>(cy + ayR * dy);
            std::wstring wlab = toWide(buf);
            // Generous box centered on (ax,ay); StringFormat does true center anchor.
            const float boxHalf = static_cast<float>(scalePx) * 1.4f;
            Gdiplus::RectF box(ax - boxHalf, ay - boxHalf, boxHalf * 2.0f, boxHalf * 2.0f);
            gfx.DrawString(wlab.c_str(), -1, &scaleFont, box, &centerFmt, &textBrush);
        }
    }
}

// Locate WidgetGraphics\<file> on disk. Disk wins over the copy compiled
// into the EXE. Order: WidgetGraphics next to the program, then
// private\WidgetGraphics.
static std::wstring FindWidgetGraphicsFile(const wchar_t* fileName) {
    std::vector<std::wstring> roots;
    wchar_t modulePath[MAX_PATH] = {};
    if (GetModuleFileNameW(nullptr, modulePath, MAX_PATH) > 0) {
        std::wstring dir(modulePath);
        size_t slash = dir.find_last_of(L"\\/");
        if (slash != std::wstring::npos)
            dir.resize(slash);
        roots.push_back(dir);
        roots.push_back(dir + L"\\..");
        roots.push_back(dir + L"\\..\\..");
        roots.push_back(dir + L"\\..\\..\\..");
    }
    roots.push_back(L".");
    roots.push_back(L"..");
    roots.push_back(L"..\\..");

    for (const auto& root : roots) {
        const std::wstring paths[] = {
            root + L"\\WidgetGraphics\\" + fileName,
            root + L"\\private\\WidgetGraphics\\" + fileName,
        };
        for (const auto& path : paths) {
            DWORD attr = GetFileAttributesW(path.c_str());
            if (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY))
                return path;
        }
    }
    return {};
}

// Soft-switch style → WidgetGraphics 3-frame strip (Switch-Style-Map.md).
const wchar_t* LayoutRenderer::SwitchStyleSheetFile(int style) {
    switch (style) {
    case 0: return L"switches.png";
    case 1: return L"switches-slider.png";
    case 2: return L"switch-3.png";
    case 3: return L"switches-blacktoggle-150.png";
    case 4: return L"switches-blacktoggle-80.png";
    case 5: return L"switches-silvertoggle-150.png";
    case 6: return L"switches-silvertoggle-80.png";
    case 7: return L"switches-silvertoggle-150.png"; // Silver Momentary = Silver Toggle art
    case 8: return L"switches-silvertoggle-80.png";
    case 9: return L"sw-mom-red-sm.png";
    case 10: return L"sw-mom-red.png";
    case 11: return L"sw-mom-red-lg.png";
    case 12: return L"sw-mom-dark-sm.png";
    case 13: return L"sw-mom-dark.png";
    case 14: return L"sw-mom-dark-lg.png";
    case 15: return L"switches-blacktoggle-150.png"; // Black Momentary Switch = Black Toggle art
    case 16: return L"switches-blacktoggle-80.png";
    default: return nullptr;
    }
}

namespace {
std::map<std::wstring, Gdiplus::Image*> gWidgetImages;
std::map<int, Gdiplus::Image*> gBezelImages;
std::map<uint64_t, Gdiplus::Bitmap*> gStretchedBezels;
std::map<uint32_t, Gdiplus::Bitmap*> gStatusLeds;
}

// Same-hue sphere. Center is the zone color. Rim is about half that.
// Fitted to the large label-off lamp in Samples/dash-renders/P5-led-labeloff.png.
static float StatusLedShade(float dist) {
    if (dist < 0.0f) dist = 0.0f;
    if (dist > 1.0f) dist = 1.0f;
    const float t = std::pow(1.0f - dist, 1.45f);
    return 0.47f + 0.53f * t;
}

static Gdiplus::Bitmap* MakeShadedStatusLed(int diameter, BYTE zr, BYTE zg, BYTE zb) {
    if (diameter < 4) return nullptr;
    auto* bmp = new Gdiplus::Bitmap(diameter, diameter, PixelFormat32bppARGB);
    if (!bmp || bmp->GetLastStatus() != Gdiplus::Ok) {
        delete bmp;
        return nullptr;
    }
    Gdiplus::BitmapData bd = {};
    Gdiplus::Rect rect(0, 0, diameter, diameter);
    if (bmp->LockBits(&rect, Gdiplus::ImageLockModeWrite, PixelFormat32bppARGB, &bd) != Gdiplus::Ok) {
        delete bmp;
        return nullptr;
    }
    const float cx = diameter * 0.5f;
    const float cy = cx;
    const float rad = diameter * 0.5f;
    BYTE* base = static_cast<BYTE*>(bd.Scan0);
    for (int y = 0; y < diameter; ++y) {
        BYTE* row = base + y * bd.Stride;
        const float dy = (static_cast<float>(y) + 0.5f) - cy;
        for (int x = 0; x < diameter; ++x) {
            BYTE* p = row + x * 4;
            const float dx = (static_cast<float>(x) + 0.5f) - cx;
            const float d = std::sqrt(dx * dx + dy * dy);
            const float cover = rad - d;
            if (cover <= 0.0f) {
                p[0] = p[1] = p[2] = p[3] = 0;
                continue;
            }
            const float shade = StatusLedShade(d / rad);
            auto ch = [&](BYTE zone) -> BYTE {
                float v = static_cast<float>(zone) * shade;
                if (v < 0.0f) v = 0.0f;
                if (v > 255.0f) v = 255.0f;
                return static_cast<BYTE>(v + 0.5f);
            };
            p[0] = ch(zb);
            p[1] = ch(zg);
            p[2] = ch(zr);
            p[3] = (cover >= 1.0f) ? 255 : static_cast<BYTE>(cover * 255.0f + 0.5f);
        }
    }
    bmp->UnlockBits(&bd);
    return bmp;
}

static Gdiplus::Bitmap* GetShadedStatusLed(int diameter, BYTE zr, BYTE zg, BYTE zb) {
    const uint32_t key = (static_cast<uint32_t>(diameter) << 24)
        | (static_cast<uint32_t>(zr) << 16)
        | (static_cast<uint32_t>(zg) << 8)
        | static_cast<uint32_t>(zb);
    auto it = gStatusLeds.find(key);
    if (it != gStatusLeds.end())
        return it->second;
    Gdiplus::Bitmap* led = MakeShadedStatusLed(diameter, zr, zg, zb);
    gStatusLeds[key] = led;
    return led;
}

// WidgetGraphics files compiled into the EXE. A loose file still wins.
static WORD EmbeddedWidgetId(const wchar_t* file) {
    struct Entry { const wchar_t* name; WORD id; };
    static const Entry kEntries[] = {
        { L"blackbezel-blackface-lg.png", IDR_BEZEL_BLACK_LG },
        { L"silverbezel-blackface-lg.png", IDR_BEZEL_SILVER_LG },
        { L"TX-all_symbols.png", IDR_SYMBOL_ATLAS },
        { L"switches.png", IDR_SWITCH_ROCKER },
        { L"switches-slider.png", IDR_SWITCH_SLIDER },
        { L"switch-3.png", IDR_SWITCH_ROCKER2 },
        { L"switches-blacktoggle-150.png", IDR_SWITCH_BLACKTOGGLE_150 },
        { L"switches-blacktoggle-80.png", IDR_SWITCH_BLACKTOGGLE_80 },
        { L"switches-silvertoggle-150.png", IDR_SWITCH_SILVERTOGGLE_150 },
        { L"switches-silvertoggle-80.png", IDR_SWITCH_SILVERTOGGLE_80 },
        { L"sw-mom-red-sm.png", IDR_SWITCH_MOM_RED_SM },
        { L"sw-mom-red.png", IDR_SWITCH_MOM_RED },
        { L"sw-mom-red-lg.png", IDR_SWITCH_MOM_RED_LG },
        { L"sw-mom-dark-sm.png", IDR_SWITCH_MOM_DARK_SM },
        { L"sw-mom-dark.png", IDR_SWITCH_MOM_DARK },
        { L"sw-mom-dark-lg.png", IDR_SWITCH_MOM_DARK_LG },
    };
    for (const Entry& e : kEntries) {
        if (_wcsicmp(file, e.name) == 0)
            return e.id;
    }
    return 0;
}

// Copy the resource into a bitmap that owns its pixels. The stream is released.
static Gdiplus::Bitmap* BitmapFromEmbedded(WORD id) {
    HMODULE hMod = GetModuleHandleW(nullptr);
    HRSRC hRes = FindResourceW(hMod, MAKEINTRESOURCEW(id), RT_RCDATA);
    if (!hRes)
        return nullptr;
    HGLOBAL hData = LoadResource(hMod, hRes);
    if (!hData)
        return nullptr;
    const void* src = LockResource(hData);
    const DWORD sz = SizeofResource(hMod, hRes);
    if (!src || sz == 0)
        return nullptr;

    HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, sz);
    if (!hg)
        return nullptr;
    void* dst = GlobalLock(hg);
    if (!dst) {
        GlobalFree(hg);
        return nullptr;
    }
    memcpy(dst, src, sz);
    GlobalUnlock(hg);

    IStream* stream = nullptr;
    if (CreateStreamOnHGlobal(hg, TRUE, &stream) != S_OK) {
        GlobalFree(hg);
        return nullptr;
    }

    Gdiplus::Bitmap fromStream(stream);
    Gdiplus::Bitmap* owned = nullptr;
    if (fromStream.GetLastStatus() == Gdiplus::Ok
        && fromStream.GetWidth() > 0 && fromStream.GetHeight() > 0) {
        owned = new Gdiplus::Bitmap(
            static_cast<INT>(fromStream.GetWidth()),
            static_cast<INT>(fromStream.GetHeight()),
            PixelFormat32bppARGB);
        if (!owned || owned->GetLastStatus() != Gdiplus::Ok) {
            delete owned;
            owned = nullptr;
        } else {
            Gdiplus::Graphics g(owned);
            g.SetCompositingMode(Gdiplus::CompositingModeSourceCopy);
            g.DrawImage(&fromStream, 0, 0,
                static_cast<INT>(owned->GetWidth()),
                static_cast<INT>(owned->GetHeight()));
        }
    }
    stream->Release();
    return owned;
}

// Cached WidgetGraphics image by filename (switch strips, bezels, symbol atlas).
// Disk wins. Otherwise the copy compiled into the EXE is used.
// A sheet with no disk file and no resource stays null, and a switch paints a circle.
static Gdiplus::Image* GetCachedWidgetImage(const wchar_t* file) {
    if (!file || !file[0]) return nullptr;
    std::wstring key(file);
    auto it = gWidgetImages.find(key);
    if (it != gWidgetImages.end())
        return it->second;

    Gdiplus::Image* loaded = nullptr;
    std::wstring path = FindWidgetGraphicsFile(file);
    if (!path.empty()) {
        loaded = new Gdiplus::Image(path.c_str());
        if (!loaded || loaded->GetLastStatus() != Gdiplus::Ok) {
            delete loaded;
            loaded = nullptr;
        }
    }
    if (!loaded) {
        const WORD id = EmbeddedWidgetId(file);
        if (id)
            loaded = BitmapFromEmbedded(id);
    }

    gWidgetImages[key] = loaded; // may be null — avoid re-probing missing files every frame
    return loaded;
}

// Cached full strips; dual-use styles share one Image* via same file load key.
Gdiplus::Image* LayoutRenderer::GetSwitchSheetImage(int style) {
    const wchar_t* file = SwitchStyleSheetFile(style);
    if (!file) return nullptr;
    return GetCachedWidgetImage(file);
}

// Knock out the opaque black face disk from *bezel-blackface* assets so only the
// chrome ring remains (face is not transparent in the Holley PNGs).
// Radial cut only — annulus is never touched. sm vs lg use different face radii
// (sm chrome starts ~0.66; lg face holds to ~0.76).
static Gdiplus::Bitmap* MakeRingOnlyBezel(Gdiplus::Image* src, bool lgAsset) {
    if (!src) return nullptr;
    const int w = static_cast<int>(src->GetWidth());
    const int h = static_cast<int>(src->GetHeight());
    if (w < 8 || h < 8) return nullptr;

    auto* bmp = new Gdiplus::Bitmap(w, h, PixelFormat32bppARGB);
    if (!bmp || bmp->GetLastStatus() != Gdiplus::Ok) {
        delete bmp;
        return nullptr;
    }
    {
        Gdiplus::Graphics g(bmp);
        g.SetCompositingMode(Gdiplus::CompositingModeSourceCopy);
        g.DrawImage(src, 0, 0, w, h);
    }

    Gdiplus::BitmapData data = {};
    Gdiplus::Rect rect(0, 0, w, h);
    if (bmp->LockBits(&rect, Gdiplus::ImageLockModeRead | Gdiplus::ImageLockModeWrite,
            PixelFormat32bppARGB, &data) != Gdiplus::Ok) {
        delete bmp;
        return nullptr;
    }

    const float cx = (w - 1) * 0.5f;
    const float cy = (h - 1) * 0.5f;
    const float rMax = (std::min)(cx, cy);
    // Measured mid/45° luminance jumps (WidgetGraphics *bezel-blackface*):
    //   sm: face ~r≤0.64; ring from ~0.66 (silver earlier on diagonal)
    //   lg: face ~r≤0.76; ring from ~0.78
    // Stay just inside the ring so the inner lip is not clipped.
    const float faceRadius = lgAsset ? 0.75f : 0.64f;

    auto* scan0 = static_cast<BYTE*>(data.Scan0);
    for (int y = 0; y < h; ++y) {
        auto* row = scan0 + y * data.Stride;
        for (int x = 0; x < w; ++x) {
            // BGRA
            BYTE* p = row + x * 4;
            if (p[3] == 0) continue;
            const float dx = static_cast<float>(x) - cx;
            const float dy = static_cast<float>(y) - cy;
            const float rn = std::sqrt(dx * dx + dy * dy) / rMax;
            if (rn < faceRadius)
                p[3] = 0; // face disk only
        }
    }
    bmp->UnlockBits(&data);
    return bmp;
}

// Cached ring-only bezel sprites. Grey (ct=2) shares black art.
// Always the lg (450) assets: scaled down they read thinner and cover
// less of the zone arc / ticks than sm (300) scaled up (which looked heavy).
Gdiplus::Image* LayoutRenderer::GetBezelImage(int ctStyle) {
    // ct: 1 Black, 2 Grey (black art), 3 Silver. Original (0) has no sprite.
    if (ctStyle != 1 && ctStyle != 2 && ctStyle != 3)
        return nullptr;
    const int family = (ctStyle == 3) ? 1 : 0; // 0 = black/grey, 1 = silver
    const int key = family; // always lg

    auto it = gBezelImages.find(key);
    if (it != gBezelImages.end())
        return it->second;

    const wchar_t* file =
        (family == 1) ? L"silverbezel-blackface-lg.png" : L"blackbezel-blackface-lg.png";

    // Disk or embedded (do not own — cached by filename). Clone for ring process.
    Gdiplus::Image* src = GetCachedWidgetImage(file);
    Gdiplus::Image* ringOnly = nullptr;
    if (src)
        ringOnly = MakeRingOnlyBezel(src, /*lgAsset=*/true);
    gBezelImages[key] = ringOnly;
    return ringOnly;
}

// Analog type 0 — LOCKED 2026-09-19. Pixel offsets from size= S, not fractions of radius.
// Dash-snipped black bezel PNG is 480×480; chrome ~0.880–0.984 of image rMax.
// OD is that PNG scaled to size= (nailed). ID is a measured table, not linear
// scale — blit rings get thicker as S shrinks (H-analog-ct-crown 2026-09-12).
constexpr float kBezelSrcOuter = 0.984f;
constexpr float kBezelSrcInner = 0.880f;
// Skip the PNG's bright inner lip so stretched ID is not a white hairline.
constexpr float kBezelSrcInnerSample = 0.928f;

static bool AnalogIsBlit(const Gauge& g) {
    // ct selects blit inner geom even when Border is off (crown=false).
    return g.ct == 1 || g.ct == 2 || g.ct == 3;
}

static float AnalogKnot(int S, int a, float va, int b, float vb) {
    if (S <= a) return va;
    if (S >= b) return vb;
    return va + (vb - va) * static_cast<float>(S - a) / static_cast<float>(b - a);
}

// Blit bezel inner diameter (px). Compare Notes Analog + honor dash.
static float AnalogBlitInnerDiam(int S) {
    if (S >= 480) return 420.f;
    if (S >= 255) return AnalogKnot(S, 255, 213.f, 480, 420.f);
    if (S >= 180) return AnalogKnot(S, 180, 148.f, 255, 213.f);
    if (S >= 127) return AnalogKnot(S, 127, 98.f, 180, 148.f);
    return AnalogKnot(S, 80, 58.f, 127, 98.f);
}

static float AnalogMajorInk(int S, bool blit) {
    if (blit) {
        if (S >= 255) return 20.f;
        if (S >= 180) return AnalogKnot(S, 180, 16.f, 255, 20.f);
        if (S >= 127) return AnalogKnot(S, 127, 11.f, 180, 16.f);
        return AnalogKnot(S, 80, 8.f, 127, 11.f);
    }
    if (S >= 255) return 20.f;
    if (S >= 180) return AnalogKnot(S, 180, 15.f, 255, 20.f);
    if (S >= 127) return AnalogKnot(S, 127, 10.f, 180, 15.f);
    return AnalogKnot(S, 80, 7.f, 127, 10.f);
}

static float AnalogMajorPen(int S, bool blit) {
    // Honor H-analog-ct-crown: ct=0 majors are as thin as minors; ct>=1 are fat.
    // Fat ladder is dash-measured: 6px at S>=255, 4 at 180, 3 at 127.
    if (!blit) return 2.f;
    if (S >= 255) return 6.f;
    if (S >= 180) return AnalogKnot(S, 180, 4.f, 255, 6.f);
    if (S >= 127) return AnalogKnot(S, 127, 3.f, 180, 4.f);
    return AnalogKnot(S, 80, 2.f, 127, 3.f);
}

struct AnalogFaceGeom {
    float boxRx = 4.f, boxRy = 4.f;
    float plateRx = 4.f, plateRy = 4.f;
    float bezelOuterRx = 0.f, bezelOuterRy = 0.f;
    float bezelInnerRx = 0.f, bezelInnerRy = 0.f;
    float tickOuterRx = 4.f, tickOuterRy = 4.f;
    float arcOuterRx = 4.f, arcOuterRy = 4.f;
    float majorLen = 20.f, minorLen = 12.f, arcThick = 20.f;
    float majorPen = 6.f, minorPen = 2.f;
    int blitDestW = 0, blitDestH = 0;
    bool blit = false;
};

static AnalogFaceGeom AnalogFaceGeomFor(const Gauge& g, int w, int h) {
    AnalogFaceGeom z;
    float rx = w * 0.5f;
    float ry = h * 0.5f;
    if (rx < 4.f) rx = 4.f;
    if (ry < 4.f) ry = 4.f;
    z.boxRx = rx;
    z.boxRy = ry;
    const int S = (std::max)(8, (std::min)(w, h));
    z.blit = AnalogIsBlit(g);
    z.majorLen = AnalogMajorInk(S, z.blit);
    z.minorLen = z.majorLen * 0.60f;
    z.majorPen = AnalogMajorPen(S, z.blit);
    z.minorPen = 2.f;

    if (z.blit) {
        // OD = PNG chrome scaled to size= (nailed). ID = measured table (thicker
        // ring at small S). Sprite is radially stretched onto that annulus.
        z.bezelOuterRx = kBezelSrcOuter * rx;
        z.bezelOuterRy = kBezelSrcOuter * ry;
        float innerR = AnalogBlitInnerDiam(S) * 0.5f;
        if (innerR > z.bezelOuterRx - 2.f) innerR = z.bezelOuterRx - 2.f;
        if (innerR < 4.f) innerR = 4.f;
        z.bezelInnerRx = innerR;
        z.bezelInnerRy = innerR * (ry / rx);
        z.blitDestW = (int)(z.bezelOuterRx * 2.f + 0.5f);
        z.blitDestH = (int)(z.bezelOuterRy * 2.f + 0.5f);
        // Tuck the face under the ring so no grey gap at the ID.
        z.plateRx = z.bezelInnerRx + 1.5f;
        z.plateRy = z.bezelInnerRy + 1.5f;
        z.tickOuterRx = z.bezelInnerRx + 1.f;
        z.tickOuterRy = z.bezelInnerRy + 1.f;
        z.arcOuterRx = z.bezelInnerRx + 1.f;
        z.arcOuterRy = z.bezelInnerRy + 1.f;
        z.arcThick = z.majorLen * 0.5f;
    } else if (g.crown) {
        z.bezelOuterRx = rx;
        z.bezelOuterRy = ry;
        z.bezelInnerRx = (std::max)(4.f, rx - 9.f);
        z.bezelInnerRy = (std::max)(4.f, ry - 9.f);
        z.plateRx = rx;
        z.plateRy = ry;
        z.tickOuterRx = (std::max)(4.f, rx - 14.f);
        z.tickOuterRy = (std::max)(4.f, ry - 14.f);
        z.arcOuterRx = z.tickOuterRx + 1.f;
        z.arcOuterRy = z.tickOuterRy + 1.f;
        z.arcThick = z.majorLen;
    } else {
        z.plateRx = (std::max)(4.f, rx - 9.f);
        z.plateRy = (std::max)(4.f, ry - 9.f);
        z.tickOuterRx = (std::max)(4.f, rx - 14.f);
        z.tickOuterRy = (std::max)(4.f, ry - 14.f);
        z.arcOuterRx = z.tickOuterRx + 1.f;
        z.arcOuterRy = z.tickOuterRy + 1.f;
        z.arcThick = z.majorLen;
    }
    if (z.arcThick < 2.f) z.arcThick = 2.f;
    if (z.majorLen < 3.f) z.majorLen = 3.f;
    if (z.minorLen < 2.f) z.minorLen = 2.f;
    return z;
}

static void SampleBgra(const BYTE* bits, int stride, int sw, int sh,
    float fx, float fy, BYTE out[4]) {
    if (fx < 0.f) fx = 0.f;
    if (fy < 0.f) fy = 0.f;
    if (fx > sw - 1) fx = static_cast<float>(sw - 1);
    if (fy > sh - 1) fy = static_cast<float>(sh - 1);
    const int x0 = static_cast<int>(fx);
    const int y0 = static_cast<int>(fy);
    const int x1 = (std::min)(x0 + 1, sw - 1);
    const int y1 = (std::min)(y0 + 1, sh - 1);
    const float tx = fx - static_cast<float>(x0);
    const float ty = fy - static_cast<float>(y0);
    const BYTE* p00 = bits + y0 * stride + x0 * 4;
    const BYTE* p10 = bits + y0 * stride + x1 * 4;
    const BYTE* p01 = bits + y1 * stride + x0 * 4;
    const BYTE* p11 = bits + y1 * stride + x1 * 4;
    for (int c = 0; c < 4; ++c) {
        const float v = (1.f - tx) * (1.f - ty) * p00[c] + tx * (1.f - ty) * p10[c]
            + (1.f - tx) * ty * p01[c] + tx * ty * p11[c];
        out[c] = static_cast<BYTE>(v + 0.5f);
    }
}

// Remap the PNG chrome band (src inner/outer fractions) onto dest ID/OD.
static Gdiplus::Bitmap* StretchBezelAnnulus(Gdiplus::Image* src,
    float innerRx, float innerRy, float outerRx, float outerRy) {
    if (!src || outerRx < 8.f || outerRy < 8.f) return nullptr;
    if (innerRx < 2.f) innerRx = 2.f;
    if (innerRy < 2.f) innerRy = 2.f;
    if (innerRx > outerRx - 2.f) innerRx = outerRx - 2.f;
    if (innerRy > outerRy - 2.f) innerRy = outerRy - 2.f;

    const int dw = (std::max)(8, static_cast<int>(outerRx * 2.f + 1.5f));
    const int dh = (std::max)(8, static_cast<int>(outerRy * 2.f + 1.5f));
    auto* dest = new Gdiplus::Bitmap(dw, dh, PixelFormat32bppARGB);
    if (!dest || dest->GetLastStatus() != Gdiplus::Ok) {
        delete dest;
        return nullptr;
    }

    const int sw = static_cast<int>(src->GetWidth());
    const int sh = static_cast<int>(src->GetHeight());
    Gdiplus::Bitmap srcBmp(sw, sh, PixelFormat32bppARGB);
    {
        Gdiplus::Graphics g(&srcBmp);
        g.SetCompositingMode(Gdiplus::CompositingModeSourceCopy);
        g.DrawImage(src, 0, 0, sw, sh);
    }
    Gdiplus::BitmapData srcData = {};
    Gdiplus::Rect srcRect(0, 0, sw, sh);
    if (srcBmp.LockBits(&srcRect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &srcData) != Gdiplus::Ok) {
        delete dest;
        return nullptr;
    }
    Gdiplus::BitmapData dstData = {};
    Gdiplus::Rect dstRect(0, 0, dw, dh);
    if (dest->LockBits(&dstRect, Gdiplus::ImageLockModeWrite, PixelFormat32bppARGB, &dstData) != Gdiplus::Ok) {
        srcBmp.UnlockBits(&srcData);
        delete dest;
        return nullptr;
    }

    const float scx = (sw - 1) * 0.5f;
    const float scy = (sh - 1) * 0.5f;
    const float sRmax = (std::min)(scx, scy);
    const float srcInner = kBezelSrcInnerSample * sRmax;
    const float srcOuter = kBezelSrcOuter * sRmax;
    const float dcx = (dw - 1) * 0.5f;
    const float dcy = (dh - 1) * 0.5f;
    const float innerN = innerRx / outerRx;
    const float feather = 0.75f / outerRx;
    auto* srcBits = static_cast<const BYTE*>(srcData.Scan0);
    auto* dstBits = static_cast<BYTE*>(dstData.Scan0);

    for (int y = 0; y < dh; ++y) {
        auto* row = dstBits + y * dstData.Stride;
        for (int x = 0; x < dw; ++x) {
            BYTE* p = row + x * 4;
            p[0] = p[1] = p[2] = p[3] = 0;
            const float nx = (static_cast<float>(x) - dcx) / outerRx;
            const float ny = (static_cast<float>(y) - dcy) / outerRy;
            const float rn = std::sqrt(nx * nx + ny * ny);
            if (rn > 1.f + feather || rn < innerN) continue;
            float t = (rn - innerN) / (1.f - innerN);
            if (t < 0.f) t = 0.f;
            if (t > 1.f) t = 1.f;
            float aMul = 1.f;
            if (rn > 1.f) aMul = 1.f - (rn - 1.f) / feather;
            const float sr = srcInner + t * (srcOuter - srcInner);
            const float ang = std::atan2(ny, nx);
            BYTE s[4];
            SampleBgra(srcBits, srcData.Stride, sw, sh,
                scx + sr * std::cos(ang), scy + sr * std::sin(ang), s);
            p[0] = s[0];
            p[1] = s[1];
            p[2] = s[2];
            p[3] = static_cast<BYTE>(s[3] * aMul + 0.5f);
        }
    }
    dest->UnlockBits(&dstData);
    srcBmp.UnlockBits(&srcData);
    return dest;
}

static Gdiplus::Bitmap* GetStretchedBezel(int ctStyle, Gdiplus::Image* src,
    float innerRx, float innerRy, float outerRx, float outerRy) {
    const int family = (ctStyle == 3) ? 1 : 0;
    const int inD = (std::max)(1, static_cast<int>(innerRx * 2.f + 0.5f));
    const int outD = (std::max)(1, static_cast<int>(outerRx * 2.f + 0.5f));
    const uint64_t key = (static_cast<uint64_t>(family) << 32)
        | (static_cast<uint32_t>(inD & 0xFFFF) << 16)
        | static_cast<uint32_t>(outD & 0xFFFF);
    auto it = gStretchedBezels.find(key);
    if (it != gStretchedBezels.end())
        return it->second;
    Gdiplus::Bitmap* stretched = StretchBezelAnnulus(src, innerRx, innerRy, outerRx, outerRy);
    gStretchedBezels[key] = stretched;
    return stretched;
}

// Product Border when crown=true. Blits WidgetGraphics for Black/Grey/Silver;
// Original uses a bright procedural chrome ring. Returns true if a sprite was drawn.
bool LayoutRenderer::DrawProductBorder(HDC hdc, const Gauge& g, int left, int top, int w, int h) const {
    if (!g.crown) return false;
    int cx = left + w / 2;
    int cy = top + h / 2;
    const AnalogFaceGeom z = AnalogFaceGeomFor(g, w, h);
    int ct = g.ct;
    if (ct < 0) ct = 0;

    if (z.blit) {
        Gdiplus::Image* srcRing = GetBezelImage(ct);
        Gdiplus::Bitmap* ring = GetStretchedBezel(ct, srcRing,
            z.bezelInnerRx, z.bezelInnerRy, z.bezelOuterRx, z.bezelOuterRy);
        if (ring) {
            Gdiplus::Graphics gfx(hdc);
            gfx.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
            gfx.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);
            gfx.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
            const int dw = static_cast<int>(ring->GetWidth());
            const int dh = static_cast<int>(ring->GetHeight());
            gfx.DrawImage(ring, cx - dw / 2, cy - dh / 2, dw, dh);
            // Cover PNG inner highlight so ID is not a white hairline (honor on1).
            {
                BYTE a, r, gre, b;
                UnpackHolley(g.background, a, r, gre, b);
                if (a == 0) { r = gre = b = 0; }
                Gdiplus::Pen cover(Gdiplus::Color(255, r, gre, b), 2.0f);
                gfx.DrawEllipse(&cover,
                    static_cast<float>(cx) - z.bezelInnerRx,
                    static_cast<float>(cy) - z.bezelInnerRy,
                    z.bezelInnerRx * 2.f, z.bezelInnerRy * 2.f);
            }
            if (ct == 2) {
                Gdiplus::Pen lift(Gdiplus::Color(90, 180, 180, 185), 2.0f);
                gfx.DrawEllipse(&lift,
                    static_cast<float>(cx) - z.bezelOuterRx + 1.f,
                    static_cast<float>(cy) - z.bezelOuterRy + 1.f,
                    (z.bezelOuterRx - 1.f) * 2.f, (z.bezelOuterRy - 1.f) * 2.f);
            }
            return true;
        }
    }

    DrawCrownCircle(hdc, cx, cy,
        static_cast<int>(z.bezelOuterRx + 0.5f), static_cast<int>(z.bezelOuterRy + 0.5f),
        /*originalChrome=*/(ct == 0));
    return false;
}

// Procedural border ring. originalChrome: 9px band, outer edge at rx (OD = size).
void LayoutRenderer::DrawCrownCircle(HDC hdc, int cx, int cy, int rx, int ry, bool originalChrome) const {
    if (rx < 4) rx = 4;
    if (ry < 4) ry = 4;
    Gdiplus::Graphics gfx(hdc);
    gfx.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    if (originalChrome) {
        const float t = 9.0f;
        Gdiplus::Pen band(Gdiplus::Color(255, 210, 212, 218), t);
        gfx.DrawEllipse(&band, cx - rx + t * 0.5f, cy - ry + t * 0.5f,
            (rx - t * 0.5f) * 2, (ry - t * 0.5f) * 2);
        Gdiplus::Pen mid(Gdiplus::Color(255, 245, 246, 250), 2.5f);
        gfx.DrawEllipse(&mid, cx - rx + 2, cy - ry + 2, (rx - 2) * 2, (ry - 2) * 2);
        Gdiplus::Pen inner(Gdiplus::Color(255, 140, 142, 150), 1.2f);
        if (rx > 8 && ry > 8)
            gfx.DrawEllipse(&inner, cx - rx + t - 1, cy - ry + t - 1,
                (rx - t + 1) * 2, (ry - t + 1) * 2);
    } else {
        Gdiplus::Pen outer(Gdiplus::Color(255, 150, 150, 158), 3.0f);
        Gdiplus::Pen inner(Gdiplus::Color(255, 70, 70, 78), 1.0f);
        gfx.DrawEllipse(&outer, cx - rx, cy - ry, rx * 2, ry * 2);
        if (rx > 6 && ry > 6)
            gfx.DrawEllipse(&inner, cx - rx + 3, cy - ry + 3, (rx - 3) * 2, (ry - 3) * 2);
    }
}

// Selection chrome: blue ring while the gauge is selected (edit / position / range).
void LayoutRenderer::DrawSelectionCircle(HDC hdc, int cx, int cy, int rx, int ry) const {
    if (rx < 2) rx = 2;
    if (ry < 2) ry = 2;
    HPEN pen = CreatePen(PS_SOLID, 3, RGB(0, 180, 255));
    HPEN oldP = (HPEN)SelectObject(hdc, pen);
    HBRUSH oldB = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Ellipse(hdc, cx - rx, cy - ry, cx + rx, cy + ry);
    SelectObject(hdc, oldP);
    SelectObject(hdc, oldB);
    DeleteObject(pen);
}

void LayoutRenderer::DrawSelectionRect(HDC hdc, int left, int top, int w, int h, int inflate) const {
    HPEN pen = CreatePen(PS_SOLID, 2, RGB(0, 180, 255));
    HPEN oldP = (HPEN)SelectObject(hdc, pen);
    HBRUSH oldB = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, left - inflate, top - inflate, left + w + inflate, top + h + inflate);
    SelectObject(hdc, oldP);
    SelectObject(hdc, oldB);
    DeleteObject(pen);
}

// ---------------------------------------------------------------------------
// Type 0: Analog
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Per-type gauge painters
// ---------------------------------------------------------------------------

// gaugeType 0 — analog face (circular or elliptical from size= w×h).
// LOCKED 2026-09-19 (H-analog-ct-crown accepted). Fat major pen 6/4/3;
// label/value center-anchored; needle meets inner ticks (tickOuter-13
// original, -11 blit). OD/ID/blit table unchanged. Latin face Roboto 2013.
void LayoutRenderer::DrawAnalog(HDC hdc, const Gauge& g, int left, int top, int w, int h, bool selected) const {
    int cx = left + w / 2;
    int cy = top + h / 2;
    const AnalogFaceGeom z = AnalogFaceGeomFor(g, w, h);
    const int rMin = (std::max)(4, (int)((std::min)(z.boxRx, z.boxRy) + 0.5f));

    // Soft face from background alpha (0 = fully clear). Always allowed — not gated on steps.
    // Original → full disk under the ring. No border → pie at OD size-18. Blit → disk in the hole.
    {
        BYTE a, r, gre, b;
        UnpackHolley(g.background, a, r, gre, b);
        if (a > 0) {
            Gdiplus::Graphics gfx(hdc);
            gfx.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
            Gdiplus::SolidBrush faceBrush(Gdiplus::Color(a, r, gre, b));
            Gdiplus::RectF rect(cx - z.plateRx, cy - z.plateRy,
                z.plateRx * 2.f, z.plateRy * 2.f);
            if (g.crown) {
                gfx.FillEllipse(&faceBrush, rect);
            } else {
                float startGdi = (float)(g.startAngle - 90.0);
                float sweepGdi = (float)(g.endAngle - g.startAngle);
                gfx.FillPie(&faceBrush, rect, startGdi, sweepGdi);
            }
        }
    }

    DrawZoneArcBars(hdc, g, cx, cy, z.arcOuterRx, z.arcOuterRy, z.arcThick);
    DrawAnalogTicks(hdc, g, cx, cy, z.tickOuterRx, z.tickOuterRy,
        z.majorLen, z.minorLen, z.majorPen, z.minorPen, rMin);

    // Text roles (TL14 analog row):
    //   fontSize → mid-face label only
    //   value / units / scale → layout (face size), not fontSize
    // Label/value are center-anchored on the pivot vertical (honor ink-center:
    // label ~0.30×R above, value ~0.23×R below).
    double demoVal = DemoValue(g);
    {
        Gdiplus::Graphics gfx(hdc);
        gfx.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        gfx.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAliasGridFit);
        const Gdiplus::FontFamily& family = PreviewFont::Family();
        Gdiplus::StringFormat centerFmt(Gdiplus::StringFormat::GenericTypographic());
        centerFmt.SetAlignment(Gdiplus::StringAlignmentCenter);
        centerFmt.SetLineAlignment(Gdiplus::StringAlignmentCenter);
        centerFmt.SetFormatFlags(
            Gdiplus::StringFormatFlagsNoWrap | Gdiplus::StringFormatFlagsNoClip);
        BYTE ta, tr, tg, tb;
        UnpackHolley(g.foreground ? g.foreground : 0xFFFFFFFFu, ta, tr, tg, tb);
        if (ta < 200) ta = 255;
        Gdiplus::SolidBrush textBrush(Gdiplus::Color(ta, tr, tg, tb));
        const float labCy = static_cast<float>(cy) - z.boxRy * 0.30f;
        const float valCy = static_cast<float>(cy) + z.boxRy * 0.23f + 2.f;
        auto drawCentered = [&](const std::wstring& w, int px, float midY) {
            if (w.empty() || px < 1) return;
            Gdiplus::Font font(&family, static_cast<Gdiplus::REAL>(px),
                Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
            Gdiplus::RectF sz;
            gfx.MeasureString(w.c_str(), -1, &font, Gdiplus::PointF(0, 0), &centerFmt, &sz);
            if (sz.Width < 1.f) sz.Width = 1.f;
            if (sz.Height < 1.f) sz.Height = static_cast<Gdiplus::REAL>(px);
            Gdiplus::RectF box(static_cast<Gdiplus::REAL>(cx) - sz.Width * 0.5f,
                midY - sz.Height * 0.5f, sz.Width, sz.Height);
            gfx.DrawString(w.c_str(), -1, &font, box, &centerFmt, &textBrush);
        };
        if (g.numericIndicatorEnabled) {
            char buf[64];
            if (g.precision <= 0) sprintf_s(buf, "%.0f", demoVal);
            else sprintf_s(buf, "%.*f", g.precision, demoVal);
            const int fontPx = (std::max)(18, (int)(rMin / 4.0 + 0.5));
            drawCentered(IniUtils::Utf8ToWide(buf), fontPx, valCy);
        }
        if (g.displayLabel && !g.label.empty()) {
            const int labPx = PreviewFont::Resolve(g.fontSize, (std::max)(11, rMin / 14));
            drawCentered(IniUtils::Utf8ToWide(g.label), labPx, labCy);
        }
    }
    // Dash analog faces do not paint units (displayUnits is a Digital-style flag).

    // Tip meets the inner end of the majors (honor: original tickOuter-13, blit -11).
    const float needleInset = z.blit ? 11.f : 13.f;
    const int needleRx = (std::max)(4, (int)(z.tickOuterRx - needleInset + 0.5f));
    const int needleRy = (std::max)(4, (int)(z.tickOuterRy - needleInset + 0.5f));
    DrawNeedle(hdc, cx, cy, needleRx, needleRy, ValueToAngle(g, demoVal), RGB(220, 30, 30));

    // Border last: Original chrome / blit ring covers tick and arc tips (dash).
    DrawProductBorder(hdc, g, left, top, w, h);

    if (selected)
        DrawSelectionCircle(hdc, cx, cy,
            (int)(z.boxRx + 1.f + 0.5f), (int)(z.boxRy + 1.f + 0.5f));
}

// ---------------------------------------------------------------------------
// Type 4: Thin Analog
// ---------------------------------------------------------------------------

// gaugeType 4 — Thin Analog: open value arc, majors only.
//   • Outer ring: open arc start→end, pen 2, outer edge on size= (S4 dash).
//   • Value bar: constant ~8px, contacts outer ring. FG in normal; zone in warn/alarm.
//   • Major ticks only (steps); **mtics ignored**. Pen 2; longer at Start/End.
//   • No Border (crown ignored). Plate uses Holley background alpha as stored.
void LayoutRenderer::DrawThinAnalog(HDC hdc, const Gauge& g, int left, int top, int w, int h, bool selected) const {
    int cx = left + w / 2;
    int cy = top + h / 2;
    int rx = w / 2;
    int ry = h / 2;
    if (rx < 4) rx = 4;
    if (ry < 4) ry = 4;
    const float rMin = static_cast<float>((std::min)(rx, ry));

    // Soft plate — respect Holley alpha (do not boost low A; that mismatched dash).
    {
        BYTE ba, br, bgc, bb;
        UnpackHolley(g.background, ba, br, bgc, bb);
        if (ba > 0) {
            Gdiplus::Graphics plateGfx(hdc);
            plateGfx.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
            Gdiplus::SolidBrush plate(Gdiplus::Color(ba, br, bgc, bb));
            plateGfx.FillRectangle(&plate, left + 1, top + 1, w - 2, h - 2);
        }
    }

    const double demo = DemoValue(g);
    double t = 0.0;
    if (g.maxValue > g.minValue) {
        t = (demo - g.minValue) / (g.maxValue - g.minValue);
        if (t < 0.0) t = 0.0;
        if (t > 1.0) t = 1.0;
    }

    BYTE fa, fr, fg, fb;
    UnpackHolley(g.foreground ? g.foreground : 0xFFFFFFFFu, fa, fr, fg, fb);
    if (fa < 200) fa = 255;

    // Value bar color: FG in normal; zone color in warn/alarm.
    const bool lowWarn = g.minThresholdEnabled && (demo < g.minNormal);
    const bool highWarn = g.maxThresholdEnabled && (demo > g.maxNormal);
    COLORREF barCol;
    if (lowWarn || highWarn)
        barCol = ZoneColorForValue(g, demo);
    else
        barCol = RGB(fr, fg, fb);
    const BYTE brc = GetRValue(barCol), bgc2 = GetGValue(barCol), bbc = GetBValue(barCol);

    // Dash S4: outer ring sits on size= (Dout/size ≈ 1.00). Pen 2, outer edge at rx.
    const float ringPen = 2.0f;
    const float Rx = (std::max)(4.0f, static_cast<float>(rx) - ringPen * 0.5f);
    const float Ry = (std::max)(4.0f, static_cast<float>(ry) - ringPen * 0.5f);
    // Dash value-bar thickness is nearly constant ~8px at every size.
    const float barPen = 8.0f;
    const float inset = (barPen - ringPen) * 0.5f;
    const float RbarX = (std::max)(4.0f, Rx - inset);
    const float RbarY = (std::max)(4.0f, Ry - inset);

    const float startGdi = static_cast<float>(g.startAngle) - 90.0f;
    const float sweepGdi = static_cast<float>(g.endAngle - g.startAngle);

    Gdiplus::Graphics gfx(hdc);
    gfx.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    gfx.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);

    Gdiplus::RectF ringRect(cx - Rx, cy - Ry, Rx * 2.0f, Ry * 2.0f);
    Gdiplus::RectF barRect(cx - RbarX, cy - RbarY, RbarX * 2.0f, RbarY * 2.0f);

    // 1) Outer ring: open arc only, pen 2
    {
        Gdiplus::Pen ringPenObj(Gdiplus::Color(230, fr, fg, fb), ringPen);
        ringPenObj.SetStartCap(Gdiplus::LineCapFlat);
        ringPenObj.SetEndCap(Gdiplus::LineCapFlat);
        if (std::fabs(sweepGdi) > 0.05f)
            gfx.DrawArc(&ringPenObj, ringRect, startGdi, sweepGdi);
    }

    // 2) Value arc bar: start → value (FG or zone color)
    if (t > 0.001 && std::fabs(sweepGdi) > 0.05f) {
        const float valSweep = sweepGdi * static_cast<float>(t);
        Gdiplus::Pen barPenObj(Gdiplus::Color(255, brc, bgc2, bbc), barPen);
        barPenObj.SetStartCap(Gdiplus::LineCapFlat);
        barPenObj.SetEndCap(Gdiplus::LineCapFlat);
        gfx.DrawArc(&barPenObj, barRect, startGdi, valSweep);
    }

    // 3) Major ticks only (mtics ignored). Longer at Start/End; sit close to ring.
    {
        Gdiplus::Pen tickPen(Gdiplus::Color(230, fr, fg, fb), 2.0f);
        tickPen.SetStartCap(Gdiplus::LineCapFlat);
        tickPen.SetEndCap(Gdiplus::LineCapFlat);

        // Outer tip close to ring (~0.5–1px gap); ticks a bit longer than prior pass.
        const float tickOuter = 1.0f - (ringPen * 0.5f + 0.8f) / rMin;
        const float shortLen = (std::max)(9.0f, rMin * 0.105f) / rMin;
        const float longLen = (std::max)(16.0f, rMin * 0.19f) / rMin;
        const float fOut = (std::max)(0.55f, tickOuter);
        const float fInShort = fOut - shortLen;
        const float fInLong = fOut - longLen;

        auto drawTick = [&](double angleDeg, bool longTick) {
            double dx, dy;
            AngleToDir(angleDeg, dx, dy);
            const float fIn = longTick ? fInLong : fInShort;
            const float x0 = static_cast<float>(cx) + fIn * Rx * static_cast<float>(dx);
            const float y0 = static_cast<float>(cy) + fIn * Ry * static_cast<float>(dy);
            const float x1 = static_cast<float>(cx) + fOut * Rx * static_cast<float>(dx);
            const float y1 = static_cast<float>(cy) + fOut * Ry * static_cast<float>(dy);
            gfx.DrawLine(&tickPen, x0, y0, x1, y1);
        };

        drawTick(static_cast<double>(g.startAngle), true);
        drawTick(static_cast<double>(g.endAngle), true);

        // Intermediate majors from steps only (no mtics subdivision)
        const int majors = (g.steps > 0) ? g.steps : 0;
        if (majors > 1) {
            for (int i = 1; i < majors; ++i) {
                const double u = static_cast<double>(i) / static_cast<double>(majors);
                const double ang = g.startAngle + u * (g.endAngle - g.startAngle);
                drawTick(ang, false);
            }
        }
    }

    if (selected)
        DrawSelectionRect(hdc, left, top, w, h, 1);

    const int rMinI = static_cast<int>(rMin);
    {
        Gdiplus::Graphics gfx(hdc);
        gfx.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAliasGridFit);
        const Gdiplus::FontFamily& family = PreviewFont::Family();
        Gdiplus::SolidBrush textBr(Gdiplus::Color(255, fr, fg, fb));
        Gdiplus::StringFormat centerFmt(Gdiplus::StringFormat::GenericTypographic());
        centerFmt.SetAlignment(Gdiplus::StringAlignmentCenter);
        centerFmt.SetLineAlignment(Gdiplus::StringAlignmentCenter);
        centerFmt.SetFormatFlags(
            Gdiplus::StringFormatFlagsNoWrap | Gdiplus::StringFormatFlagsNoClip);
        auto drawCentered = [&](const std::wstring& w, int px, float midX, float midY) {
            if (w.empty() || px < 1) return;
            Gdiplus::Font font(&family, static_cast<Gdiplus::REAL>(px),
                Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
            Gdiplus::RectF sz;
            gfx.MeasureString(w.c_str(), -1, &font, Gdiplus::PointF(0, 0), &centerFmt, &sz);
            if (sz.Width < 1.f) sz.Width = 1.f;
            if (sz.Height < 1.f) sz.Height = static_cast<Gdiplus::REAL>(px);
            Gdiplus::RectF box(midX - sz.Width * 0.5f, midY - sz.Height * 0.5f, sz.Width, sz.Height);
            gfx.DrawString(w.c_str(), -1, &font, box, &centerFmt, &textBr);
        };
        if (g.numericIndicatorEnabled) {
            char buf[64];
            if (g.precision <= 0) sprintf_s(buf, "%.0f", demo);
            else sprintf_s(buf, "%.*f", g.precision, demo);
            const int fontPx = (std::max)(16, rMinI / 4);
            const float midY = static_cast<float>(cy) - static_cast<float>(rMinI) / 18.f;
            drawCentered(IniUtils::Utf8ToWide(buf), fontPx, static_cast<float>(cx), midY);
        }
        if (g.displayLabel && !g.label.empty()) {
            const int labPx = PreviewFont::Resolve(g.fontSize, (std::max)(11, rMinI / 12));
            const float midY = static_cast<float>(cy + rMinI / 5) + static_cast<float>(labPx) * 0.5f;
            drawCentered(IniUtils::Utf8ToWide(g.label), labPx, static_cast<float>(cx), midY);
        }
    }
}

// ---------------------------------------------------------------------------
// Type 1: Bargraph — LOCKED 2026-08-29.
// Reserved label band + 25px bar. Spine only when labeled.
// Vertical = same card rotated 90° CCW. Scale paints outside the card.
// ---------------------------------------------------------------------------

void LayoutRenderer::DrawBargraph(HDC hdc, const Gauge& g, int left, int top, int w, int h, bool selected) const {
    if (w < 8) w = 8;
    if (h < 8) h = 8;

    const bool isVertical = g.orientation;
    if (selected)
        DrawSelectionRect(hdc, left, top, w, h, 1);

    const double demo = DemoValue(g);
    double t = 0.0;
    if (g.maxValue > g.minValue) {
        t = (demo - g.minValue) / (g.maxValue - g.minValue);
        if (t < 0.0) t = 0.0;
        if (t > 1.0) t = 1.0;
    }

    COLORREF zc = ZoneColorForValue(g, demo);
    Gdiplus::Color fillColor(230, GetRValue(zc), GetGValue(zc), GetBValue(zc));

    BYTE bga, bgr, bgg, bgb;
    UnpackHolley(g.background, bga, bgr, bgg, bgb);
    const bool bgTransparent = (bga == 0);
    const bool paintUnlitTrack = !bgTransparent && bga > 0;
    Gdiplus::Color trackColor(160, 18, 18, 24);
    if (paintUnlitTrack)
        trackColor = Gdiplus::Color((std::min)(bga, static_cast<BYTE>(180)), bgr, bgg, bgb);

    const int tickLen = 4;
    const int endPad = kBargraphEndPadPx;
    const int barPx = kBargraphBarPx;
    const int hookW = kBargraphHookWidthPx;
    const int hookGap = kBargraphHookBarGapPx;
    const int labelPx = BargraphLabelPx(g);
    const int labelTop = kBargraphLabelTopPadPx;
    const int spineAlong = BargraphSpineOffset(g);
    const int bar0 = BargraphBarOffset(g);
    const int hookEnd = bar0 - hookGap;
    const bool showSpine = g.displayLabel && !g.label.empty();

    auto toWide = [](const std::string& s) -> std::wstring {
        return IniUtils::Utf8ToWide(s);
    };

    std::string valueStr;
    if (g.numericIndicatorEnabled) {
        char buf[64];
        if (g.precision <= 0) sprintf_s(buf, "%.0f", demo);
        else sprintf_s(buf, "%.*f", g.precision, demo);
        valueStr = buf;
        if (g.displayUnits && !g.units.empty()) {
            valueStr.push_back(' ');
            valueStr += g.units;
        }
    }
    const std::wstring wValue = toWide(valueStr);
    const std::wstring wLabel = showSpine ? toWide(g.label) : std::wstring();

    const int stepCount = (g.steps > 0) ? g.steps : 1;
    const bool drawScale = g.scaleValues && (g.maxValue > g.minValue);

    auto scaleLabel = [&](int i) -> std::wstring {
        double v = g.minValue + (g.maxValue - g.minValue) * static_cast<double>(i) / static_cast<double>(stepCount);
        if (g.scalingDivisor != 0.0 && g.scalingDivisor != 1.0)
            v /= g.scalingDivisor;
        char buf[32];
        if (g.precision <= 0) sprintf_s(buf, "%.0f", v);
        else sprintf_s(buf, "%.*f", g.precision, v);
        return toWide(buf);
    };

    Gdiplus::Graphics gfx(hdc);
    gfx.SetSmoothingMode(Gdiplus::SmoothingModeNone);
    gfx.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);
    const Gdiplus::FontFamily& family = PreviewFont::Family();

    BYTE fa, fr, fg, fb;
    UnpackHolley(g.foreground ? g.foreground : 0xFFFFFFFFu, fa, fr, fg, fb);
    if (fa < 200) fa = 255;
    Gdiplus::SolidBrush textBrush(Gdiplus::Color(fa, fr, fg, fb));
    Gdiplus::SolidBrush scaleBrush(Gdiplus::Color(220, 180, 180, 190));
    Gdiplus::SolidBrush spineBrush(Gdiplus::Color(255, 255, 255, 255));
    Gdiplus::StringFormat centerFmt;
    centerFmt.SetAlignment(Gdiplus::StringAlignmentCenter);
    centerFmt.SetLineAlignment(Gdiplus::StringAlignmentCenter);

    const int length = isVertical ? h : w;
    int span0 = endPad;
    int span1 = length - endPad;
    if (span1 < span0 + 2) span1 = span0 + 2;

    // Local (lx,ly) is horizontal card space. Vertical = 90° CCW.
    auto fillLocal = [&](int lx, int ly, int lw, int lh, Gdiplus::Brush& br) {
        if (lw < 1 || lh < 1) return;
        if (!isVertical) {
            gfx.FillRectangle(&br, left + lx, top + ly, lw, lh);
            return;
        }
        gfx.FillRectangle(&br, left + ly, top + (length - (lx + lw)), lh, lw);
    };

    Gdiplus::SolidBrush track(trackColor);
    Gdiplus::SolidBrush fill(fillColor);
    const int barSpan = span1 - span0;
    const int fillPx = static_cast<int>(barSpan * t + 0.5);
    const int segs = (stepCount > 1) ? stepCount : 1;

    // Solid unlit track. Lit fill grows over it. Dividers are 1px of bar
    // background only at segment boundaries the fill has passed (same as Style 2).
    if (paintUnlitTrack)
        fillLocal(span0, bar0, barSpan, barPx, track);
    if (fillPx > 0)
        fillLocal(span0, bar0, fillPx, barPx, fill);
    if (segs > 1 && fillPx > 0) {
        for (int i = 1; i < segs; ++i) {
            const double u = static_cast<double>(i) / static_cast<double>(segs);
            const int lx = span0 + static_cast<int>(barSpan * u + 0.5);
            if (lx >= span0 + fillPx) break;
            fillLocal(lx, bar0, 1, barPx, track);
        }
    }

    // Segment ticks outside the card (below H / right of V).
    {
        Gdiplus::Pen underTick(Gdiplus::Color(220, 220, 220, 230), 1.0f);
        for (int i = 0; i <= segs; ++i) {
            const double u = static_cast<double>(i) / static_cast<double>(segs);
            const int lx = span0 + static_cast<int>(barSpan * u + 0.5);
            if (!isVertical) {
                const float x = static_cast<float>(left + lx);
                const float y0 = static_cast<float>(top + bar0 + barPx);
                gfx.DrawLine(&underTick, x, y0, x, y0 + static_cast<float>(tickLen));
            } else {
                const float x0 = static_cast<float>(left + bar0 + barPx);
                const float y = static_cast<float>(top + (length - lx));
                gfx.DrawLine(&underTick, x0, y, x0 + static_cast<float>(tickLen), y);
            }
        }
    }

    Gdiplus::Font labelFont(&family, static_cast<Gdiplus::REAL>(labelPx),
        Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);

    if (showSpine && hookEnd > spineAlong) {
        const int hookH = hookEnd - spineAlong;
        float gapL = static_cast<float>(span0);
        float gapR = static_cast<float>(span0);
        bool haveGap = false;
        Gdiplus::RectF labSize(0, 0, 0, 0);
        if (!wLabel.empty()) {
            gfx.MeasureString(wLabel.c_str(), -1, &labelFont, Gdiplus::PointF(0, 0), &labSize);
            const float gapPad = 6.0f;
            const float labW = labSize.Width + gapPad * 2.0f;
            const float mid = 0.5f * static_cast<float>(span0 + span1);
            gapL = mid - labW * 0.5f;
            gapR = mid + labW * 0.5f;
            if (gapL < static_cast<float>(span0) + 8.0f) {
                gapL = static_cast<float>(span0) + 8.0f;
                gapR = gapL + labW;
            }
            if (gapR > static_cast<float>(span1) - 8.0f) {
                gapR = static_cast<float>(span1) - 8.0f;
                gapL = gapR - labW;
            }
            haveGap = (gapR > gapL + 2.0f);
        }

        auto drawSpineRun = [&](int xA, int xB, bool hookLeft, bool hookRight) {
            if (xB - xA < 2) return;
            fillLocal(xA, spineAlong, xB - xA, kBargraphSpinePx, spineBrush);
            if (hookLeft && hookH > 0)
                fillLocal(xA, spineAlong, hookW, hookH, spineBrush);
            if (hookRight && hookH > 0)
                fillLocal(xB - hookW, spineAlong, hookW, hookH, spineBrush);
        };

        if (haveGap) {
            drawSpineRun(span0, static_cast<int>(gapL + 0.5f), true, false);
            drawSpineRun(static_cast<int>(gapR + 0.5f), span1, false, true);
            if (!isVertical) {
                Gdiplus::RectF labBox(static_cast<Gdiplus::REAL>(left) + gapL,
                    static_cast<Gdiplus::REAL>(top + labelTop),
                    gapR - gapL, static_cast<Gdiplus::REAL>(labelPx));
                gfx.DrawString(wLabel.c_str(), -1, &labelFont, labBox, &centerFmt, &textBrush);
            } else {
                const float cx = static_cast<float>(left + spineAlong);
                const float cy = static_cast<float>(top) + static_cast<float>(length) - 0.5f * (gapL + gapR);
                gfx.TranslateTransform(cx, cy);
                gfx.RotateTransform(-90.0f);
                Gdiplus::RectF labBox(-(gapR - gapL) * 0.5f, -static_cast<Gdiplus::REAL>(labelPx) * 0.5f,
                    gapR - gapL, static_cast<Gdiplus::REAL>(labelPx));
                gfx.DrawString(wLabel.c_str(), -1, &labelFont, labBox, &centerFmt, &textBrush);
                gfx.ResetTransform();
            }
        } else {
            drawSpineRun(span0, span1, true, true);
        }
    }

    if (!wValue.empty()) {
        int fontPx = 16;
        if (fontPx > barPx - 2) fontPx = (std::max)(10, barPx - 2);
        Gdiplus::Font font(&family, static_cast<Gdiplus::REAL>(fontPx),
            Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
        if (!isVertical) {
            Gdiplus::RectF box(static_cast<Gdiplus::REAL>(left + span0),
                static_cast<Gdiplus::REAL>(top + bar0),
                static_cast<Gdiplus::REAL>(barSpan), static_cast<Gdiplus::REAL>(barPx));
            gfx.DrawString(wValue.c_str(), -1, &font, box, &centerFmt, &textBrush);
        } else {
            const float cx = static_cast<float>(left + bar0) + barPx * 0.5f;
            const float cy = static_cast<float>(top) + static_cast<float>(length - span0) - barSpan * 0.5f;
            gfx.TranslateTransform(cx, cy);
            gfx.RotateTransform(-90.0f);
            Gdiplus::RectF box(-static_cast<Gdiplus::REAL>(barSpan) * 0.5f, -static_cast<Gdiplus::REAL>(barPx) * 0.5f,
                static_cast<Gdiplus::REAL>(barSpan), static_cast<Gdiplus::REAL>(barPx));
            gfx.DrawString(wValue.c_str(), -1, &font, box, &centerFmt, &textBrush);
            gfx.ResetTransform();
        }
    }

    if (drawScale) {
        const int fontPx = 9;
        Gdiplus::Font font(&family, static_cast<Gdiplus::REAL>(fontPx),
            Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
        Gdiplus::StringFormat sf;
        const float scaleOff = static_cast<float>(bar0 + barPx + tickLen + 1);
        for (int i = 0; i <= stepCount; ++i) {
            const double u = static_cast<double>(i) / static_cast<double>(stepCount);
            const int lx = span0 + static_cast<int>(barSpan * u + 0.5);
            std::wstring lab = scaleLabel(i);
            if (!isVertical) {
                const float scaleY = static_cast<float>(top) + scaleOff;
                Gdiplus::RectF box;
                if (i == 0) {
                    sf.SetAlignment(Gdiplus::StringAlignmentNear);
                    box = Gdiplus::RectF(static_cast<Gdiplus::REAL>(left + span0), scaleY, 48.0f, 12.0f);
                } else if (i == stepCount) {
                    sf.SetAlignment(Gdiplus::StringAlignmentFar);
                    box = Gdiplus::RectF(static_cast<Gdiplus::REAL>(left + span1 - 48), scaleY, 48.0f, 12.0f);
                } else {
                    sf.SetAlignment(Gdiplus::StringAlignmentCenter);
                    box = Gdiplus::RectF(static_cast<Gdiplus::REAL>(left + lx) - 28.0f, scaleY, 56.0f, 12.0f);
                }
                sf.SetLineAlignment(Gdiplus::StringAlignmentNear);
                gfx.DrawString(lab.c_str(), -1, &font, box, &sf, &scaleBrush);
            } else {
                sf.SetAlignment(Gdiplus::StringAlignmentNear);
                sf.SetLineAlignment(Gdiplus::StringAlignmentCenter);
                const float scaleX = static_cast<float>(left) + scaleOff;
                const float y = static_cast<float>(top + (length - lx));
                Gdiplus::RectF box(scaleX, y - 9.0f, 40.0f, 18.0f);
                gfx.DrawString(lab.c_str(), -1, &font, box, &sf, &scaleBrush);
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Type 2: Digital meter
// Type 2 Digital: Size = height; plate W from measured ink. Locked paint.
// ---------------------------------------------------------------------------

// Holley AARRGGBB for the active warning zone (c0…c4). Returns 0 when the zone
// color is unset (common for normal c2) — caller treats 0 as no plate fill.
static unsigned int ZoneHolleyForValue(const Gauge& g, double value) {
    if (g.minThresholdEnabled) {
        if (value < g.minCaution) return g.c0;
        if (value < g.minNormal) return g.c1;
    }
    if (g.maxThresholdEnabled) {
        if (value > g.maxCaution) return g.c4;
        if (value > g.maxNormal) return g.c3;
    }
    return g.c2;
}

// gaugeType 2 — Digital numeric card (zone plate, spine label, value + units).
// Digital layout (Compare.xmcd): Size=H; value cap 0.714×H (− label cap+5 if labeled);
//   value bottom 4px; label cap top 8px; W from measured ink.
void LayoutRenderer::DrawDigital(HDC hdc, const Gauge& g, int left, int top, int w, int h, bool selected) const {
    if (w < 8) w = 8;
    if (h < 8) h = 8;

    const double demo = DemoValue(g);
    const bool lowWarn = g.minThresholdEnabled && (demo < g.minNormal);
    const bool highWarn = g.maxThresholdEnabled && (demo > g.maxNormal);

    BYTE plateA = 0, plateR = 0, plateG = 0, plateB = 0;
    bool platePainted = false;
    if (lowWarn || highWarn) {
        // Zone flood: A=0 skips the plate; A>=1 is opaque (H-digital-alpha).
        const unsigned int zoneH = ZoneHolleyForValue(g, demo);
        if (zoneH != 0) {
            UnpackHolley(zoneH, plateA, plateR, plateG, plateB);
            if (plateA > 0) {
                plateA = 255;
                platePainted = true;
            }
        }
    } else {
        UnpackHolley(g.background, plateA, plateR, plateG, plateB);
        if (plateA > 0)
            platePainted = true;
    }

    // Plate: warn/alarm flood zone color; normal uses Holley background (AARRGGBB).
    if (platePainted) {
        Gdiplus::Graphics plateGfx(hdc);
        plateGfx.SetSmoothingMode(Gdiplus::SmoothingModeNone);
        Gdiplus::SolidBrush plate(Gdiplus::Color(plateA, plateR, plateG, plateB));
        plateGfx.FillRectangle(&plate, left, top, w, h);
    }

    // Digital has no analog bezel; Holley Border (crown) = white spine brackets only.

    if (selected)
        DrawSelectionRect(hdc, left, top, w, h, 2);

    auto toWide = [](const std::string& s) -> std::wstring {
        return IniUtils::Utf8ToWide(s);
    };

    // Dash Digital always paints the value (H-digital-valueoff). Ignore the flag.
    std::string valueStr;
    {
        char buf[64];
        if (g.precision <= 0) sprintf_s(buf, "%.0f", demo);
        else sprintf_s(buf, "%.*f", g.precision, demo);
        valueStr = buf;
    }
    const std::wstring wValue = toWide(valueStr);
    // Dash digital: compact lowercase units suffix ("11.9v"), not spaced "V".
    const std::wstring wUnits = DigitalDisplayUnitsW(g);
    const std::wstring wLabel =
        (g.displayLabel && !g.label.empty()) ? toWide(g.label) : std::wstring();

    Gdiplus::Graphics gfx(hdc);
    gfx.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    gfx.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);
    const Gdiplus::FontFamily& family = PreviewFont::Family();

    // Text + Border spine: Holley uses gauge foreground (T2-digital-spine-labels).
    // A=0 skips glyphs (fg0); A>=1 is opaque. Auto-contrast vs painted plate.
    BYTE fa, fr, fg, fb;
    UnpackHolley(g.foreground ? g.foreground : 0xFFFFFFFFu, fa, fr, fg, fb);
    if (fa == 0)
        return;
    fa = 255;
    Gdiplus::SolidBrush textBrush(Gdiplus::Color(fa, fr, fg, fb));
    Gdiplus::StringFormat centerFmt;
    centerFmt.SetAlignment(Gdiplus::StringAlignmentCenter);
    centerFmt.SetLineAlignment(Gdiplus::StringAlignmentCenter);
    Gdiplus::StringFormat nearFmt;
    nearFmt.SetAlignment(Gdiplus::StringAlignmentNear);
    nearFmt.SetLineAlignment(Gdiplus::StringAlignmentCenter);

    // Spine brackets = same color as label/value (not fixed white).
    Gdiplus::Pen spinePen(Gdiplus::Color(fa, fr, fg, fb), 1.0f);
    spinePen.SetLineJoin(Gdiplus::LineJoinMiter);
    spinePen.SetStartCap(Gdiplus::LineCapFlat);
    spinePen.SetEndCap(Gdiplus::LineCapFlat);

    auto drawHSpineSeg = [&](float xA, float xB, float ySpine, bool hookLeft, bool hookRight) {
        const float hook = 4.0f;
        if (xB - xA < 2.0f) return;
        Gdiplus::GraphicsPath path;
        if (hookLeft)
            path.AddLine(xA, ySpine + hook, xA, ySpine);
        path.AddLine(xA, ySpine, xB, ySpine);
        if (hookRight)
            path.AddLine(xB, ySpine, xB, ySpine + hook);
        gfx.DrawPath(&spinePen, &path);
    };

    // Digital text: fontSize → label; value/units from shared layout (also drives W).
    const DigitalTextLayout dtl = ComputeDigitalTextLayout(h, g);
    const int pad = dtl.pad;
    const int labelFontPx = dtl.labelPx;

    // Label band when displayLabel is on (RW-6.86 G2: no band over value-only cards).
    // Spine brackets only when Holley Border is on (INI crown).
    if (!wLabel.empty()) {
        const float ySpine = static_cast<float>(top + kDigitalLabelTopPx)
            + static_cast<float>(dtl.labelCapPx) * 0.5f;
        const float xSpine0 = static_cast<float>(left + pad) + 1.0f;
        const float xSpine1 = static_cast<float>(left + w - pad) - 1.0f;

        Gdiplus::Font labelFont(&family, static_cast<Gdiplus::REAL>(labelFontPx),
            Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
        Gdiplus::StringFormat labFmt(Gdiplus::StringFormat::GenericTypographic());
        labFmt.SetFormatFlags(
            Gdiplus::StringFormatFlagsNoWrap | Gdiplus::StringFormatFlagsNoClip);
        labFmt.SetAlignment(Gdiplus::StringAlignmentCenter);
        labFmt.SetLineAlignment(Gdiplus::StringAlignmentNear);

        Gdiplus::RectF labSize;
        gfx.MeasureString(wLabel.c_str(), -1, &labelFont, Gdiplus::PointF(0, 0), &labFmt, &labSize);
        const float gapPad = 6.0f;
        const float labW = labSize.Width + gapPad * 2.0f;
        const float mid = 0.5f * (xSpine0 + xSpine1);
        // Allow large labels to extend past the box (dash does this at high FS).
        const float labBoxL = mid - labW * 0.5f;

        // Holley Border (crown): FG-colored spine stubs. Off when Border off.
        // Min stubs for long labels (RW-6.86-SNIPER G13; kDigitalSpineMinStubPx).
        if (g.crown) {
            const float kMinStub = static_cast<float>(kDigitalSpineMinStubPx);
            float gapL = mid - labW * 0.5f;
            float gapR = mid + labW * 0.5f;
            if (gapL < xSpine0 + kMinStub)
                gapL = xSpine0 + kMinStub;
            if (gapR > xSpine1 - kMinStub)
                gapR = xSpine1 - kMinStub;
            if (gapR <= gapL) {
                drawHSpineSeg(xSpine0, xSpine0 + kMinStub, ySpine, true, false);
                drawHSpineSeg(xSpine1 - kMinStub, xSpine1, ySpine, false, true);
            } else {
                drawHSpineSeg(xSpine0, gapL, ySpine, true, false);
                drawHSpineSeg(gapR, xSpine1, ySpine, false, true);
            }
        }

        // Capital-letter top is 8px from the plate top (not centered in the band).
        constexpr INT kLabStyle = Gdiplus::FontStyleRegular;
        const INT labEm = family.GetEmHeight(kLabStyle);
        const INT labAscent = family.GetCellAscent(kLabStyle);
        const float ascentLab = (labEm > 0)
            ? static_cast<float>(labelFontPx) * labAscent / static_cast<float>(labEm)
            : static_cast<float>(labelFontPx);
        const float capLab = static_cast<float>(dtl.labelCapPx);
        const float extra = ascentLab - capLab;
        const float labY = static_cast<float>(top + kDigitalLabelTopPx) - extra;
        Gdiplus::RectF labBox(labBoxL, labY, labW, capLab + 4.0f);
        gfx.DrawString(wLabel.c_str(), -1, &labelFont, labBox, &labFmt, &textBrush);
    }

    // Value: right-justified; capital bottom 4px above plate. Skip if cap was squashed.
    // GenericTypographic + cell metrics — default MeasureString padding floated glyphs.
    if (dtl.valuePx > 0 && (!wValue.empty() || !wUnits.empty())) {
        const float bandW = static_cast<float>(w - pad - dtl.rightPad);
        int valuePx = dtl.valuePx;
        int unitsPx = dtl.unitsPx;

        Gdiplus::StringFormat typoFmt(Gdiplus::StringFormat::GenericTypographic());
        typoFmt.SetFormatFlags(
            Gdiplus::StringFormatFlagsNoWrap | Gdiplus::StringFormatFlagsNoClip);
        typoFmt.SetAlignment(Gdiplus::StringAlignmentNear);
        typoFmt.SetLineAlignment(Gdiplus::StringAlignmentNear);

        const float gap = (!wValue.empty() && !wUnits.empty()) ? 1.0f : 0.0f;
        Gdiplus::RectF vSize, uSize;

        auto measure = [&](int vPx, int uPx) {
            Gdiplus::Font vf(&family, static_cast<Gdiplus::REAL>(vPx),
                Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
            Gdiplus::Font uf(&family, static_cast<Gdiplus::REAL>(uPx),
                Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
            vSize = Gdiplus::RectF();
            uSize = Gdiplus::RectF();
            if (!wValue.empty())
                gfx.MeasureString(wValue.c_str(), -1, &vf, Gdiplus::PointF(0, 0), &typoFmt, &vSize);
            if (!wUnits.empty())
                gfx.MeasureString(wUnits.c_str(), -1, &uf, Gdiplus::PointF(0, 0), &typoFmt, &uSize);
        };

        measure(valuePx, unitsPx);
        auto valueDrawW = [&]() {
            return vSize.Width / kDigitalWidthAdvanceScale;
        };
        float totalW = valueDrawW() + gap + uSize.Width;
        // Width stress: shrink value only; keep units size (dash pattern).
        if (totalW > bandW && totalW > 1.0f && !wValue.empty()) {
            const float unitsPart = uSize.Width + gap;
            const float valueBudget = (std::max)(8.0f, bandW - unitsPart);
            const float drawW = valueDrawW();
            if (drawW > valueBudget && drawW > 1.0f) {
                valuePx = (std::max)(9, static_cast<int>(valuePx * valueBudget / drawW));
                measure(valuePx, unitsPx);
                totalW = valueDrawW() + gap + uSize.Width;
            }
        }

        // Lining figures sit on the baseline (almost no ink below it).
        // Dash: bottom of the value cap is always 4px above the plate.
        constexpr INT kStyle = Gdiplus::FontStyleRegular;
        const INT em = family.GetEmHeight(kStyle);
        const INT cellAscent = family.GetCellAscent(kStyle);
        auto ascentPx = [&](int px) -> float {
            return em > 0 ? static_cast<float>(px) * cellAscent / static_cast<float>(em) : static_cast<float>(px);
        };

        const float baseline = static_cast<float>(top + h) - static_cast<float>(kDigitalValueBottomPx);

        // Pack from the right edge of the plate (tight right pad).
        const float rightEdge = static_cast<float>(left + w) - static_cast<float>(dtl.rightPad);
        float cursor = rightEdge;

        if (!wUnits.empty()) {
            Gdiplus::Font unitsFont(&family, static_cast<Gdiplus::REAL>(unitsPx),
                Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
            cursor -= uSize.Width;
            // Units slightly above baseline of the big number (optical).
            const float uY = baseline - ascentPx(unitsPx) * 0.92f;
            gfx.DrawString(wUnits.c_str(), -1, &unitsFont,
                Gdiplus::PointF(cursor, uY), &typoFmt, &textBrush);
            cursor -= gap;
        }
        if (!wValue.empty()) {
            Gdiplus::Font valueFont(&family, static_cast<Gdiplus::REAL>(valuePx),
                Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
            const float drawW = valueDrawW();
            cursor -= drawW;
            const float vY = baseline - ascentPx(valuePx);
            // Roboto 2013 matches dash advances (scale 1.0). Arial used 1.08.
            if (kDigitalWidthAdvanceScale != 1.0f) {
                Gdiplus::Matrix cur;
                gfx.GetTransform(&cur);
                gfx.TranslateTransform(cursor, 0.0f);
                gfx.ScaleTransform(1.0f / kDigitalWidthAdvanceScale, 1.0f);
                gfx.DrawString(wValue.c_str(), -1, &valueFont,
                    Gdiplus::PointF(0.0f, vY), &typoFmt, &textBrush);
                gfx.SetTransform(&cur);
            } else {
                gfx.DrawString(wValue.c_str(), -1, &valueFont,
                    Gdiplus::PointF(cursor, vY), &typoFmt, &textBrush);
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Type 5 Status LED / 6 RPM Bar / 8 Symbol / 9 Graph / 11 Style2 / unknown
// ---------------------------------------------------------------------------

// gaugeType 5 — Status LED.
// Type 5 Status LED — placement LOCKED 2026-08-29. Lamp is a same-hue sphere:
// zone color at the center, about half that at the rim. No white core, no outer glow.
//   • Size: Normal / Large via orientation → 45 / 90
//   • Label-off: LED top = pos.y (not centered in the stored 44×63 / 90×109 box)
//   • Label-on: cap top = pos.y+8; LED top = that + cap (0.72×fontSize).
//     FS18 remains 8+13. AUTO em = 12.
//   • LED is clipped to the stored box (dash clips the bottom at FS≥18).
//   • Zone color c0–c4. Lit in every zone including low alarm.
void LayoutRenderer::DrawStatusLED(HDC hdc, const Gauge& g, int left, int top, int w, int h, bool selected) const {
    const double demo = DemoValue(g);
    const COLORREF zone = ZoneColorForValue(g, demo);
    const BYTE zr = GetRValue(zone);
    const BYTE zg = GetGValue(zone);
    const BYTE zb = GetBValue(zone);

    const int diameter = StatusLedIsLarge(g) ? kStatusLedLargePx : kStatusLedNormalPx;
    const int ledW = diameter;
    const int ledH = diameter;
    const bool showLabel = g.displayLabel && !g.label.empty();
    const int capPx = StatusLedLabelCapPx(g);
    const int labelFontPx = StatusLedLabelEmPx(g);
    const int dx = left + (w - ledW) / 2;
    const int dy = top + StatusLedPaintTopOffset(g);
    const float cx = static_cast<float>(dx + ledW / 2);

    Gdiplus::Graphics gfx(hdc);
    gfx.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    gfx.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    gfx.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);

    {
        // Dash clips the LED to the INI box; label is allowed to overflow.
        Gdiplus::GraphicsState saved = gfx.Save();
        gfx.SetClip(Gdiplus::Rect(left, top, w, h), Gdiplus::CombineModeIntersect);

        if (Gdiplus::Bitmap* led = GetShadedStatusLed(diameter, zr, zg, zb))
            gfx.DrawImage(led, dx, dy, ledW, ledH);

        if (selected) {
            Gdiplus::Pen ring(Gdiplus::Color(255, 0, 180, 255), 2.0f);
            const int pad = 2;
            gfx.DrawEllipse(&ring, dx - pad, dy - pad, ledW + pad * 2, ledH + pad * 2);
        }
        gfx.Restore(saved);
    }

    if (showLabel) {
        const std::wstring wLabel = DigitalUtf8ToWide(g.label);
        const Gdiplus::FontFamily& family = PreviewFont::Family();
        Gdiplus::Font labelFont(&family, static_cast<Gdiplus::REAL>(labelFontPx),
            Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
        Gdiplus::StringFormat labFmt(Gdiplus::StringFormat::GenericTypographic());
        labFmt.SetFormatFlags(
            Gdiplus::StringFormatFlagsNoWrap | Gdiplus::StringFormatFlagsNoClip);
        labFmt.SetAlignment(Gdiplus::StringAlignmentCenter);
        labFmt.SetLineAlignment(Gdiplus::StringAlignmentNear);

        Gdiplus::RectF labSize;
        gfx.MeasureString(wLabel.c_str(), -1, &labelFont, Gdiplus::PointF(0, 0), &labFmt, &labSize);
        const float labW = labSize.Width + 4.0f;

        constexpr INT kLabStyle = Gdiplus::FontStyleRegular;
        const INT labEm = family.GetEmHeight(kLabStyle);
        const INT labAscent = family.GetCellAscent(kLabStyle);
        const float ascentLab = (labEm > 0)
            ? static_cast<float>(labelFontPx) * labAscent / static_cast<float>(labEm)
            : static_cast<float>(labelFontPx);
        const float capLab = static_cast<float>(capPx);
        const float extra = ascentLab - capLab;
        const float labY = static_cast<float>(top + kStatusLedLabelTopPx) - extra;
        Gdiplus::RectF labBox(static_cast<float>(cx) - labW * 0.5f, labY, labW, capLab + 4.0f);
        Gdiplus::SolidBrush textBrush(Gdiplus::Color(255, 220, 220, 220));
        gfx.DrawString(wLabel.c_str(), -1, &labelFont, labBox, &labFmt, &textBrush);
    }
}

// Type 6 RPM Bar: INI/hit 750×100; classic paint plate 750×80 from pos.y (P6 2026-08-28).
// Dash never paints a caption. Target RPM mode = rpmModeEnabled: no plate, hide unlit,
// center the lit run, zone color. Classic: 15-LED nnl/ncl with dim unlit.
void LayoutRenderer::DrawRPMBar(HDC hdc, const Gauge& g, int left, int top, int w, int h, bool selected) const {
    if (w < 40) w = 40;
    if (h < 24) h = 24;

    const bool shiftLight = RpmBarIsShiftLight(g);
    int paintH = kRpmBarPaintH;
    if (paintH > h) paintH = h;

    if (!shiftLight)
        FillHolleyBackground(hdc, left, top, w, paintH, g.background);
    if (selected)
        DrawSelectionRect(hdc, left, top, w, h, 1);

    constexpr int leds = kRpmLedCount;

    // Zone bands: nnl/ncl when set (T6 dash). Unused in target mode.
    int normalLeds = 0;
    int cautionLeds = 0;
    RpmBarBandCounts(g, normalLeds, cautionLeds);

    // LED disks packed in the 80px paint strip (not the 100px INI box).
    const double Ww = static_cast<double>(w);
    const double Hh = static_cast<double>(paintH);
    // Diameter: mostly height-driven so LEDs stay round and equal; width caps packing.
    double diam = Hh * 0.72;
    const double packCap = Ww / static_cast<double>(leds) * 0.92;
    if (diam > packCap) diam = packCap;
    if (diam < 8.0) diam = 8.0;
    if (diam > Hh * 0.95) diam = Hh * 0.95;
    const float ledR = static_cast<float>(diam * 0.5);

    // Shallow arc: centers sit in an inset (W−D)×(H−D). Apex high, ends low
    // (matches dash T6 / factory Racepak strip language).
    const double arcW = Ww - diam;
    // Use a modest sagitta so the strip is nearly flat (dash is gentle, not a U).
    const double maxSagitta = (std::max)(2.0, Hh - diam);
    const double s = (std::min)(maxSagitta * 0.55, diam * 0.55);
    const double boxCx = static_cast<double>(left) + Ww * 0.5;
    const double boxCy = static_cast<double>(top) + Hh * 0.5;
    const double yApex = boxCy - s * 0.35; // slight lift of middle LEDs
    const double a = arcW * 0.5;

    double R;
    double circleCy;
    if (s < 1e-3) {
        R = 1e9;
        circleCy = yApex + R;
    } else {
        R = (a * a + s * s) / (2.0 * s);
        circleCy = yApex + R;
    }

    double theta = 0.0;
    if (R > 1e-6 && a > 1e-6) {
        double c = (R - s) / R;
        if (c > 1.0) c = 1.0;
        if (c < -1.0) c = -1.0;
        theta = std::acos(c);
    }

    // Lit count from fill span (min/max, or extended when thresh points sit outside).
    const double demo = DemoValue(g);
    double fillLo = 0.0;
    double fillHi = 0.0;
    RpmBarFillRange(g, fillLo, fillHi);
    int lit = 0;
    if (fillHi > fillLo) {
        double t = (demo - fillLo) / (fillHi - fillLo);
        if (t < 0.0) t = 0.0;
        if (t > 1.0) t = 1.0;
        lit = static_cast<int>(std::lround(t * leds));
        if (lit < 1 && t > 0.0) lit = 1;
        if (lit > leds) lit = leds;
    }
    const int litStart = shiftLight ? (leds - lit) / 2 : 0;

    COLORREF shiftCr = ZoneColorForValue(g, demo);
    const Gdiplus::Color shiftCol(255,
        GetRValue(shiftCr), GetGValue(shiftCr), GetBValue(shiftCr));

    auto zoneColor = [&](int index) -> Gdiplus::Color {
        unsigned int holley = 0;
        if (index < normalLeds)
            holley = g.c2 ? g.c2 : 0xFF00FF00u;
        else if (index < normalLeds + cautionLeds)
            holley = g.c3 ? g.c3 : 0xFFFF0000u;
        else
            holley = g.c4 ? g.c4 : 0xFF0000FFu;
        BYTE aa, rr, gr, bb;
        UnpackHolley(holley, aa, rr, gr, bb);
        if (aa < 200) aa = 255;
        return Gdiplus::Color(aa, rr, gr, bb);
    };

    Gdiplus::Graphics gfx(hdc);
    gfx.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    gfx.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);

    for (int i = 0; i < leds; ++i) {
        double phi = 0.0;
        if (leds > 1)
            phi = -theta + (2.0 * theta) * (static_cast<double>(i) / static_cast<double>(leds - 1));

        const float lx = static_cast<float>(boxCx + R * std::sin(phi));
        const float ly = static_cast<float>(circleCy - R * std::cos(phi));

        const bool isLit = (i >= litStart && i < litStart + lit);
        if (shiftLight && !isLit)
            continue;
        const Gdiplus::Color zc = shiftLight ? shiftCol : zoneColor(i);

        if (isLit) {
            Gdiplus::Color glow(90, zc.GetR(), zc.GetG(), zc.GetB());
            Gdiplus::SolidBrush glowBrush(glow);
            gfx.FillEllipse(&glowBrush, lx - ledR * 1.25f, ly - ledR * 1.25f,
                ledR * 2.5f, ledR * 2.5f);

            Gdiplus::SolidBrush body(zc);
            gfx.FillEllipse(&body, lx - ledR, ly - ledR, ledR * 2.0f, ledR * 2.0f);

            Gdiplus::Color hi(150, 255, 255, 255);
            Gdiplus::SolidBrush hiBrush(hi);
            const float hr = ledR * 0.32f;
            gfx.FillEllipse(&hiBrush, lx - hr * 0.55f, ly - ledR * 0.52f, hr * 1.5f, hr * 1.1f);
        } else {
            // Dim unlit: dark body with faint zone tint (dash T6 language).
            const BYTE rr = static_cast<BYTE>(zc.GetR() / 6 + 22);
            const BYTE gr = static_cast<BYTE>(zc.GetG() / 6 + 22);
            const BYTE bb = static_cast<BYTE>(zc.GetB() / 6 + 26);
            Gdiplus::SolidBrush dim(Gdiplus::Color(230, rr, gr, bb));
            gfx.FillEllipse(&dim, lx - ledR, ly - ledR, ledR * 2.0f, ledR * 2.0f);

            Gdiplus::Pen rim(Gdiplus::Color(110, 12, 12, 16), 1.0f);
            gfx.DrawEllipse(&rim, lx - ledR, ly - ledR, ledR * 2.0f, ledR * 2.0f);
        }
    }

    // Dash never paints an RPM Bar caption. Inspector still shows Label (writes label=).
}

// Type 11 Bargraph Style 2: free W×H trapezoid.
// LOCKED 2026-09-19 (H-style2-orient accepted: orientation, dividers, value).
//   • Slope: left height = 50% of right (high) height
//   • Label-off: 2px top, 8px bottom; no spine
//   • Label-on: bar top = BargraphBarOffset, 3px bottom; spine at BargraphSpineOffset
//   • Spine follows the label only (crown ignored — Digital is the Border-gated type)
//   • Value: top-left at trapezoid centroid, em 0.24×endH, clipped to the bar
//   • orientation=true: 90° CCW; paint/hit AABB is H×H at pos= (INI W unused
//     for paint); size= unchanged; hor ignored
//   • Scale numbers stay upright and track ticks (same exception as type 1)
//   • Unlit bar is a solid trapezoid. Lit fill only: 1px bar-background
//     gaps at step/minor boundaries the fill has passed (not canvas, not a pen).
void LayoutRenderer::DrawBargraphStyle2(HDC hdc, const Gauge& g, int left, int top, int w, int h, bool selected) const {
    if (w < 1) w = 1;
    if (h < 1) h = 1;

    if (selected)
        DrawSelectionRect(hdc, left, top, w, h, 1);

    const bool vert = g.orientation;
    // Painted AABB is (w,h). Unrotated card is stored W×H (along × thick).
    const int cardW = vert ? h : w;
    const int cardH = vert ? w : h;
    const int along = cardW;

    const double demo = DemoValue(g);
    double t = 0.0;
    if (g.maxValue > g.minValue) {
        t = (demo - g.minValue) / (g.maxValue - g.minValue);
        if (t < 0.0) t = 0.0;
        if (t > 1.0) t = 1.0;
    }

    const bool showSpine = g.displayLabel && !g.label.empty();
    const int sidePad = kStyle2SidePadPx;
    int barLeft = sidePad;
    int barRight = cardW - sidePad;
    if (barRight <= barLeft)
        barRight = barLeft + 1;
    const int barW = barRight - barLeft;

    const int barTopLocal = showSpine ? BargraphBarOffset(g) : kStyle2LabelOffTopPadPx;
    const int botPad = showSpine ? kStyle2LabelOnBotPadPx : kStyle2LabelOffBotPadPx;
    int barBotLocal = cardH - botPad;
    if (barBotLocal < barTopLocal + 2)
        barBotLocal = barTopLocal + 2;
    const int barBot = barBotLocal;
    const int endH = barBot - barTopLocal;

    const int steps = (g.steps > 0) ? g.steps : 10;
    const float segW = static_cast<float>(barW) / static_cast<float>(steps);

    Gdiplus::Graphics gfx(hdc);
    gfx.SetSmoothingMode(Gdiplus::SmoothingModeNone);
    gfx.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);

    auto mapPt = [&](float lx, float ly) -> Gdiplus::PointF {
        if (!vert)
            return { static_cast<float>(left) + lx, static_cast<float>(top) + ly };
        return { static_cast<float>(left) + ly,
            static_cast<float>(top) + static_cast<float>(along) - lx };
    };
    auto fillLocal = [&](int lx, int ly, int lw, int lh, Gdiplus::Brush& br) {
        if (lw < 1 || lh < 1) return;
        if (!vert) {
            gfx.FillRectangle(&br, left + lx, top + ly, lw, lh);
            return;
        }
        gfx.FillRectangle(&br, left + ly, top + (along - (lx + lw)), lh, lw);
    };
    auto mapPoly4 = [&](const Gdiplus::PointF* loc, Gdiplus::PointF* outPts) {
        for (int i = 0; i < 4; ++i)
            outPts[i] = mapPt(loc[i].X, loc[i].Y);
    };

    COLORREF litCol = ZoneColorForValue(g, demo);
    const Gdiplus::Color fillColor(230, GetRValue(litCol), GetGValue(litCol), GetBValue(litCol));
    BYTE ba, br, bgc, bb;
    UnpackHolley(g.background, ba, br, bgc, bb);
    Gdiplus::Color trackColor;
    if (ba == 0 && br == 0 && bgc == 0 && bb == 0) {
        trackColor = Gdiplus::Color(160, 18, 18, 24);
    } else {
        const BYTE ta = (ba < 160) ? static_cast<BYTE>(200) : ba;
        trackColor = Gdiplus::Color(ta, br, bgc, bb);
    }
    auto heightAt = [&](float u) -> float {
        if (u < 0.f) u = 0.f;
        if (u > 1.f) u = 1.f;
        return static_cast<float>(endH) * (0.50f + 0.50f * u);
    };

    const float yBot = static_cast<float>(barBot);
    Gdiplus::PointF trapLoc[4] = {
        { static_cast<float>(barLeft), yBot },
        { static_cast<float>(barRight), yBot },
        { static_cast<float>(barRight), yBot - heightAt(1.f) },
        { static_cast<float>(barLeft), yBot - heightAt(0.f) },
    };
    Gdiplus::PointF trapPts[4];
    mapPoly4(trapLoc, trapPts);

    // Solid unlit trapezoid. No canvas gaps, no stroked dividers (H-style2-orient).
    Gdiplus::SolidBrush dim(trackColor);
    Gdiplus::SolidBrush fill(fillColor);
    gfx.FillPolygon(&dim, trapPts, 4);

    // Lit fill grows from the low end. Dividers are 1px of bar background
    // showing through at step/minor boundaries the fill has passed.
    const float xLit = static_cast<float>(barLeft) + static_cast<float>(t) * static_cast<float>(barW);
    if (t > 0.0 && xLit > static_cast<float>(barLeft) + 0.5f) {
        const float uLit = static_cast<float>(t);
        Gdiplus::PointF litLoc[4] = {
            { static_cast<float>(barLeft), yBot },
            { xLit, yBot },
            { xLit, yBot - heightAt(uLit) },
            { static_cast<float>(barLeft), yBot - heightAt(0.f) },
        };
        Gdiplus::PointF litPts[4];
        mapPoly4(litLoc, litPts);
        gfx.FillPolygon(&fill, litPts, 4);

        const int minors = (g.mtics > 0) ? g.mtics : 0;
        const int subdiv = steps * (minors + 1);
        Gdiplus::Pen gapPen(trackColor, 1.0f);
        gapPen.SetLineCap(Gdiplus::LineCapFlat, Gdiplus::LineCapFlat, Gdiplus::DashCapFlat);
        for (int i = 1; i < subdiv; ++i) {
            const float u = static_cast<float>(i) / static_cast<float>(subdiv);
            const float x = static_cast<float>(barLeft) + u * static_cast<float>(barW);
            if (x >= xLit) break;
            const Gdiplus::PointF a = mapPt(x, yBot);
            const Gdiplus::PointF b = mapPt(x, yBot - heightAt(u));
            gfx.DrawLine(&gapPen, a, b);
        }
    }

    auto toWide = [](const std::string& s, UINT /*cp*/) -> std::wstring {
        return IniUtils::Utf8ToWide(s);
    };

    BYTE fa, fr, fg, fb;
    UnpackHolley(g.foreground ? g.foreground : 0xFFFFFFFFu, fa, fr, fg, fb);
    if (fa < 200) fa = 255;
    Gdiplus::SolidBrush textBr(Gdiplus::Color(fa, fr, fg, fb));
    Gdiplus::SolidBrush spineBrush(Gdiplus::Color(255, 255, 255, 255));
    const Gdiplus::FontFamily& family = PreviewFont::Family();
    Gdiplus::StringFormat center;
    center.SetAlignment(Gdiplus::StringAlignmentCenter);
    center.SetLineAlignment(Gdiplus::StringAlignmentCenter);

    if (showSpine) {
        const int labelPx = BargraphLabelPx(g);
        const int spineAlong = BargraphSpineOffset(g);
        const int hookW = kBargraphHookWidthPx;
        const int hookEnd = BargraphBarOffset(g) - kBargraphHookBarGapPx;
        const int hookH = (hookEnd > spineAlong) ? (hookEnd - spineAlong) : 0;
        const int span0 = sidePad;
        const int span1 = cardW - sidePad;
        const std::wstring wLabel = toWide(g.label, CP_UTF8);

        Gdiplus::Font labelFont(&family, static_cast<Gdiplus::REAL>(labelPx),
            Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
        float gapL = static_cast<float>(span0);
        float gapR = static_cast<float>(span0);
        bool haveGap = false;
        Gdiplus::RectF labSize(0, 0, 0, 0);
        if (!wLabel.empty()) {
            gfx.MeasureString(wLabel.c_str(), -1, &labelFont, Gdiplus::PointF(0, 0), &labSize);
            const float gapPad = 6.0f;
            const float labW = labSize.Width + gapPad * 2.0f;
            const float mid = 0.5f * static_cast<float>(span0 + span1);
            gapL = mid - labW * 0.5f;
            gapR = mid + labW * 0.5f;
            if (gapL < static_cast<float>(span0) + 8.0f) {
                gapL = static_cast<float>(span0) + 8.0f;
                gapR = gapL + labW;
            }
            if (gapR > static_cast<float>(span1) - 8.0f) {
                gapR = static_cast<float>(span1) - 8.0f;
                gapL = gapR - labW;
            }
            haveGap = (gapR > gapL + 2.0f);
        }

        auto drawSpineRun = [&](int xA, int xB, bool hookLeft, bool hookRight) {
            if (xB - xA < 2) return;
            fillLocal(xA, spineAlong, xB - xA, kBargraphSpinePx, spineBrush);
            if (hookLeft && hookH > 0)
                fillLocal(xA, spineAlong, hookW, hookH, spineBrush);
            if (hookRight && hookH > 0)
                fillLocal(xB - hookW, spineAlong, hookW, hookH, spineBrush);
        };

        if (haveGap) {
            drawSpineRun(span0, static_cast<int>(gapL + 0.5f), true, false);
            drawSpineRun(static_cast<int>(gapR + 0.5f), span1, false, true);
            if (!vert) {
                Gdiplus::RectF labBox(static_cast<Gdiplus::REAL>(left) + gapL,
                    static_cast<Gdiplus::REAL>(top + kBargraphLabelTopPadPx),
                    gapR - gapL, static_cast<Gdiplus::REAL>(labelPx));
                gfx.DrawString(wLabel.c_str(), -1, &labelFont, labBox, &center, &textBr);
            } else {
                const float cx = static_cast<float>(left + spineAlong);
                const float cy = static_cast<float>(top) + static_cast<float>(along)
                    - 0.5f * (gapL + gapR);
                gfx.TranslateTransform(cx, cy);
                gfx.RotateTransform(-90.0f);
                Gdiplus::RectF labBox(-(gapR - gapL) * 0.5f, -static_cast<Gdiplus::REAL>(labelPx) * 0.5f,
                    gapR - gapL, static_cast<Gdiplus::REAL>(labelPx));
                gfx.DrawString(wLabel.c_str(), -1, &labelFont, labBox, &center, &textBr);
                gfx.ResetTransform();
            }
        } else {
            drawSpineRun(span0, span1, true, true);
        }
    }

    if (g.numericIndicatorEnabled) {
        char buf[64];
        if (g.precision <= 0) sprintf_s(buf, "%.0f", demo);
        else sprintf_s(buf, "%.*f", g.precision, demo);
        std::wstring wv = toWide(buf, CP_ACP);
        if (!wv.empty()) {
            int valuePx = (int)(static_cast<float>(endH) * 0.24f + 0.5f);
            if (valuePx < 12) valuePx = 12;
            if (valuePx > 48) valuePx = 48;
            Gdiplus::Font vfont(&family, static_cast<Gdiplus::REAL>(valuePx),
                Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
            const float Wbar = static_cast<float>(barW);
            const float hh0 = heightAt(0.f);
            const float hh1 = heightAt(1.f);
            const float hmin = (std::min)(hh0, hh1);
            const float hmax = (std::max)(hh0, hh1);
            const float Ar = Wbar * hmin;
            const float At = Wbar * (hmax - hmin) * 0.5f;
            const float Area = Ar + At;
            float cx = Wbar * 0.5f;
            float cyFromBot = hh1 * 0.5f;
            if (Area >= 1.f) {
                const float cxTri = (hh1 >= hh0) ? (Wbar * 2.f / 3.f) : (Wbar / 3.f);
                cx = (Ar * (Wbar * 0.5f) + At * cxTri) / Area;
                cyFromBot = (Ar * (hmin * 0.5f) + At * ((2.f * hmin + hmax) / 3.f)) / Area;
            }
            Gdiplus::StringFormat nearFmt(Gdiplus::StringFormat::GenericTypographic());
            nearFmt.SetAlignment(Gdiplus::StringAlignmentNear);
            nearFmt.SetLineAlignment(Gdiplus::StringAlignmentNear);
            nearFmt.SetFormatFlags(Gdiplus::StringFormatFlagsNoWrap | Gdiplus::StringFormatFlagsNoClip);
            Gdiplus::GraphicsPath clipPath;
            clipPath.AddPolygon(trapPts, 4);
            gfx.SetClip(&clipPath);
            const Gdiplus::PointF origin = mapPt(
                static_cast<float>(barLeft) + cx,
                static_cast<float>(barBot) - cyFromBot);
            // Cell-top at centroid sits the ink too low (GDI+ leading). Nudge
            // by em−cap+2 so the baseline matches honor H-style2-orient.
            const float capPx = static_cast<float>(valuePx) * 0.72f;
            const float yNudge = static_cast<float>(valuePx) - capPx + 2.f;
            if (!vert) {
                gfx.DrawString(wv.c_str(), -1, &vfont,
                    Gdiplus::PointF(origin.X, origin.Y - yNudge), &nearFmt, &textBr);
            } else {
                gfx.TranslateTransform(origin.X, origin.Y);
                gfx.RotateTransform(-90.0f);
                gfx.DrawString(wv.c_str(), -1, &vfont,
                    Gdiplus::PointF(0.f, 0.f), &nearFmt, &textBr);
                gfx.ResetTransform();
            }
            gfx.ResetClip();
        }
    }

    if (g.scaleValues && g.maxValue > g.minValue) {
        Gdiplus::Font sfont(&family, 9.f, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
        Gdiplus::SolidBrush scaleBr(Gdiplus::Color(220, 190, 190, 200));
        for (int i = 0; i <= steps; ++i) {
            double v = g.minValue + (g.maxValue - g.minValue) * (double)i / (double)steps;
            if (g.scalingDivisor != 0.0 && g.scalingDivisor != 1.0) v /= g.scalingDivisor;
            char buf[32];
            if (g.precision <= 0) sprintf_s(buf, "%.0f", v);
            else sprintf_s(buf, "%.*f", g.precision, v);
            std::wstring ws = toWide(buf, CP_ACP);
            float x = static_cast<float>(barLeft) + static_cast<float>(i) * segW;
            // barLeft/barBot are card-local; mapPt is screen (horizontal was using
            // local Y as canvas Y — numbers piled at the top-left).
            const Gdiplus::PointF p = mapPt(x, static_cast<float>(barBot + 7));
            if (!vert) {
                Gdiplus::RectF sbox(p.X - 14.f, p.Y - 6.f, 28.f, 12.f);
                gfx.DrawString(ws.c_str(), -1, &sfont, sbox, &center, &scaleBr);
            } else {
                Gdiplus::RectF sbox(p.X, p.Y - 6.f, 40.f, 12.f);
                Gdiplus::StringFormat nearFmt;
                nearFmt.SetAlignment(Gdiplus::StringAlignmentNear);
                nearFmt.SetLineAlignment(Gdiplus::StringAlignmentCenter);
                gfx.DrawString(ws.c_str(), -1, &sfont, sbox, &nearFmt, &scaleBr);
            }
        }
    }
}

// gaugeType 9 — Graph / strip chart.
// Type 9 Graph: L-frame, horizontal grid, zone-colored trace. Locked paint.
//   • L-frame only (no hard top/right); solid horizontal grid; no vertical X grid
//   • Trace = dense zone-colored dots; synthetic strip-history for design preview
//   • Ticks always; scale numbers only if scaleValues; labels at every major (steps)
// Residual (not pursued): exact dash waveform shape; pixel-perfect grid weight.
void LayoutRenderer::DrawGraph(HDC hdc, const Gauge& g, int left, int top, int w, int h, bool selected) const {
    if (w < 24) w = 24;
    if (h < 24) h = 24;

    FillHolleyBackground(hdc, left, top, w, h, g.background);
    if (selected)
        DrawSelectionRect(hdc, left, top, w, h, 1);

    const int labPx = PreviewFont::Resolve(g.fontSize, 10);
    int labelBand = 2;
    if (g.displayLabel && !g.label.empty()) {
        labelBand = PreviewFont::IsExplicit(g.fontSize)
            ? (std::min)(h / 3, labPx + 4)
            : 16;
        if (labelBand < 12) labelBand = 12;
    }
    // Left margin only when Y scale numerals are shown (scaleValues).
    // Dash Graph sm / FS24: scaleValues=false → no numbers, tight left edge (ticks remain).
    const bool showScale = g.scaleValues;
    const int leftAxis = showScale ? 42 : 10;
    const int bottomAxis = 12;
    const int plotL = left + leftAxis;
    const int plotT = top + labelBand;
    const int plotR = left + w - 4;
    const int plotB = top + h - bottomAxis;
    const int plotW = plotR - plotL;
    const int plotH = plotB - plotT;
    if (plotW < 8 || plotH < 8) return;

    Gdiplus::Graphics gfx(hdc);
    gfx.SetSmoothingMode(Gdiplus::SmoothingModeNone); // crisp ticks/grid/dots
    gfx.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);

    // L-frame: dash measures ~1px pure white; use width 2 + bright white so it
    // reads heavier on the preview (1px gray looked too thin).
    Gdiplus::Pen axisPen(Gdiplus::Color(255, 245, 245, 250), 2.f);
    gfx.DrawLine(&axisPen, plotL, plotT, plotL, plotB);
    gfx.DrawLine(&axisPen, plotL, plotB, plotR, plotB);

    const int yDivs = (g.steps > 0) ? g.steps : 8;
    // Minor horizontal rows between majors (mtics); dash looks denser than majors alone.
    const int minorPerMajor = (g.mtics > 0) ? g.mtics : 1;
    const int yMinorDivs = yDivs * (minorPerMajor + 1);

    // Horizontal grid = solid lines (dash). Trace is dots, not the grid.
    Gdiplus::Pen majorGrid(Gdiplus::Color(140, 110, 110, 120), 1.f);
    Gdiplus::Pen minorGrid(Gdiplus::Color(80, 70, 70, 78), 1.f);
    for (int i = 1; i < yMinorDivs; ++i) {
        const bool isMajor = (i % (minorPerMajor + 1)) == 0;
        float y = static_cast<float>(plotB)
            - static_cast<float>(plotH) * (static_cast<float>(i) / static_cast<float>(yMinorDivs));
        Gdiplus::Pen& pen = isMajor ? majorGrid : minorGrid;
        gfx.DrawLine(&pen, static_cast<float>(plotL), y, static_cast<float>(plotR), y);
    }

    // Tick marks always (dash Graph sm/FS24 still tick without scale numbers).
    Gdiplus::Pen tickPen(Gdiplus::Color(255, 230, 230, 235), 1.f);
    for (int i = 0; i <= yDivs; ++i) {
        float y = static_cast<float>(plotB)
            - static_cast<float>(plotH) * (static_cast<float>(i) / static_cast<float>(yDivs));
        gfx.DrawLine(&tickPen, static_cast<float>(plotL - 5), y, static_cast<float>(plotL), y);
    }
    // Bottom X ticks: dash T9-graph med pitch ~44px — much sparser than plotW/40.
    const int xTickPitch = (std::max)(36, plotW / 10);
    for (int x = plotL; x <= plotR; x += xTickPitch)
        gfx.DrawLine(&tickPen, x, plotB, x, plotB + 5);

    // Synthetic strip history: flat baseline → event spike/dip → settle.
    const double demo = DemoValue(g);
    const double range = (g.maxValue > g.minValue) ? (g.maxValue - g.minValue) : 1.0;
    // Dense samples so the dashed/dotted look reads as continuous on wide graphs.
    const int kSamples = (std::max)(64, plotW); // ~1 sample per plot pixel
    std::vector<double> samples(static_cast<size_t>(kSamples));
    for (int i = 0; i < kSamples; ++i) {
        const float u = static_cast<float>(i) / static_cast<float>(kSamples - 1);
        double v = demo;
        if (u < 0.42f) {
            v = demo;
        } else if (u < 0.50f) {
            const float t = (u - 0.42f) / 0.08f;
            v = demo + range * 0.12 * t;
        } else if (u < 0.56f) {
            const float t = (u - 0.50f) / 0.06f;
            v = demo + range * (0.12 + 0.38 * t);
        } else if (u < 0.62f) {
            const float t = (u - 0.56f) / 0.06f;
            v = demo + range * (0.50 - 0.95 * t);
        } else if (u < 0.70f) {
            const float t = (u - 0.62f) / 0.08f;
            v = demo + range * (-0.45 + 0.55 * t);
        } else if (u < 0.78f) {
            const float t = (u - 0.70f) / 0.08f;
            v = demo + range * (0.10 + 0.08 * std::sin(t * 6.28f * 2.f));
        } else {
            const float t = (u - 0.78f) / 0.22f;
            v = demo + range * 0.10 * (1.0 - t);
        }
        if (v < g.minValue) v = g.minValue;
        if (v > g.maxValue) v = g.maxValue;
        samples[static_cast<size_t>(i)] = v;
    }

    auto valueToY = [&](double v) -> int {
        double tn = (v - g.minValue) / range;
        if (tn < 0.0) tn = 0.0;
        if (tn > 1.0) tn = 1.0;
        return plotB - static_cast<int>(plotH * tn + 0.5);
    };

    // Plotted line = tightly spaced dots (dash), zone-colored per sample.
    for (int i = 0; i < kSamples; ++i) {
        const double v = samples[static_cast<size_t>(i)];
        COLORREF zc = ZoneColorForValue(g, v);
        BYTE zr = GetRValue(zc), zg = GetGValue(zc), zb = GetBValue(zc);
        if (zr + zg + zb < 40) {
            BYTE fa2, fr2, fg2, fb2;
            UnpackHolley(g.foreground ? g.foreground : 0xFFFFFFFFu, fa2, fr2, fg2, fb2);
            (void)fa2;
            zr = fr2; zg = fg2; zb = fb2;
        }
        const int x = plotL
            + static_cast<int>(static_cast<double>(plotW) * i / static_cast<double>(kSamples - 1) + 0.5);
        const int y = valueToY(v);
        // 2×2 dot reads better than 1px on high-DPI-ish preview; still “dotty” not stroked.
        Gdiplus::SolidBrush br(Gdiplus::Color(240, zr, zg, zb));
        gfx.FillRectangle(&br, x, y - 1, 2, 2);
    }

    BYTE fa, fr, fg, fb;
    UnpackHolley(g.foreground ? g.foreground : 0xFFFFFFFFu, fa, fr, fg, fb);
    if (fa < 200) fa = 255;
    Gdiplus::SolidBrush textBr(Gdiplus::Color(fa, fr, fg, fb));
    const Gdiplus::FontFamily& family = PreviewFont::Family();
    Gdiplus::Font font(&family, static_cast<Gdiplus::REAL>(labPx),
        Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
    // Scale numerals: 12 * 1.25 ≈ 15 (user tweak after T9 compare).
    Gdiplus::Font sfont(&family, 15.f, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
    Gdiplus::StringFormat center;
    center.SetAlignment(Gdiplus::StringAlignmentCenter);
    center.SetLineAlignment(Gdiplus::StringAlignmentCenter);
    Gdiplus::StringFormat right;
    right.SetAlignment(Gdiplus::StringAlignmentFar);
    right.SetLineAlignment(Gdiplus::StringAlignmentCenter);

    if (g.displayLabel && !g.label.empty()) {
        const std::wstring wl = IniUtils::Utf8ToWide(g.label);
        Gdiplus::RectF box(static_cast<float>(left), static_cast<float>(top),
            static_cast<float>(w), static_cast<float>(labelBand));
        gfx.DrawString(wl.c_str(), -1, &font, box, &center, &textBr);
    }

    // Y-axis scale numbers — only when scaleValues is true.
    // Dash Graph med (steps=8): labels at every major including ends
    // (18.0 … 8.0 step ~1.25) — not every-other.
    if (showScale) {
        for (int i = 0; i <= yDivs; ++i) {
            double v = g.minValue
                + (g.maxValue - g.minValue) * static_cast<double>(i) / static_cast<double>(yDivs);
            char buf[32];
            if (g.precision <= 0) sprintf_s(buf, "%.0f", v);
            else sprintf_s(buf, "%.*f", g.precision, v);
            int n = MultiByteToWideChar(CP_ACP, 0, buf, -1, nullptr, 0);
            std::wstring ws(n > 0 ? n : 0, L'\0');
            if (n > 0) MultiByteToWideChar(CP_ACP, 0, buf, -1, &ws[0], n);
            if (!ws.empty() && ws.back() == L'\0') ws.pop_back();
            float y = static_cast<float>(plotB)
                - static_cast<float>(plotH) * (static_cast<float>(i) / static_cast<float>(yDivs));
            Gdiplus::RectF sbox(static_cast<float>(left), y - 8.f,
                static_cast<float>(leftAxis - 4), 16.f);
            gfx.DrawString(ws.c_str(), -1, &sfont, sbox, &right, &textBr);
        }
    }
}

#include "SymbolAtlasData.inc"

Gdiplus::Image* LayoutRenderer::GetSymbolAtlasImage() {
    // Disk (WidgetGraphics, then private\WidgetGraphics) wins, else the EXE copy.
    return GetCachedWidgetImage(L"TX-all_symbols.png");
}

void LayoutRenderer::ReleaseCachedImages() {
    for (auto& kv : gWidgetImages) {
        delete kv.second;
        kv.second = nullptr;
    }
    gWidgetImages.clear();
    for (auto& kv : gBezelImages) {
        delete kv.second;
        kv.second = nullptr;
    }
    gBezelImages.clear();
    for (auto& kv : gStretchedBezels) {
        delete kv.second;
        kv.second = nullptr;
    }
    gStretchedBezels.clear();
    for (auto& kv : gStatusLeds) {
        delete kv.second;
        kv.second = nullptr;
    }
    gStatusLeds.clear();
    PreviewFont::Shutdown();
}

static const SymbolAtlasEntry* FindSymbolAtlasEntry(unsigned codepoint) {
    for (int i = 0; i < kSymbolAtlasCount; ++i) {
        if (kSymbolAtlas[i].codepoint == codepoint)
            return &kSymbolAtlas[i];
    }
    return nullptr;
}

static const SymbolVerticalInk* FindSymbolVerticalInk(unsigned codepoint) {
    for (int i = 0; i < kSymbolVerticalInkCount; ++i) {
        if (kSymbolVerticalInk[i].codepoint == codepoint)
            return &kSymbolVerticalInk[i];
    }
    return nullptr;
}

static const SymbolNudge* FindSymbolNudge(unsigned codepoint) {
    for (int i = 0; i < kSymbolNudgeCount; ++i) {
        if (kSymbolNudge[i].codepoint == codepoint)
            return &kSymbolNudge[i];
    }
    return nullptr;
}

static int SymbolNudgeXY(unsigned codepoint, int fontPx, int& dx, int& dy) {
    dx = 0;
    dy = 0;
    const SymbolNudge* n = FindSymbolNudge(codepoint);
    if (!n || fontPx <= 0)
        return 0;
    if (n->scaled) {
        const double s = static_cast<double>(fontPx) / 35.0;
        dx = static_cast<int>(std::lround(static_cast<double>(n->dx) * s));
        dy = static_cast<int>(std::lround(static_cast<double>(n->dy) * s));
    } else {
        dx = n->dx;
        dy = n->dy;
    }
    return 1;
}

// Blit one atlas glyph tinted to zone color (luminance → alpha mask).
static void BlitTintedAtlasGlyph(Gdiplus::Graphics& gfx, Gdiplus::Image* atlas,
    int srcX, int srcY, int srcW, int srcH,
    int destX, int destY, int destW, int destH,
    BYTE zr, BYTE zg, BYTE zb, BYTE za)
{
    if (!atlas || srcW < 1 || srcH < 1 || destW < 1 || destH < 1)
        return;

    Gdiplus::Bitmap srcBmp(srcW, srcH, PixelFormat32bppARGB);
    {
        Gdiplus::Graphics g(&srcBmp);
        g.SetCompositingMode(Gdiplus::CompositingModeSourceCopy);
        g.DrawImage(atlas, Gdiplus::Rect(0, 0, srcW, srcH),
            srcX, srcY, srcW, srcH, Gdiplus::UnitPixel);
    }

    Gdiplus::BitmapData data = {};
    Gdiplus::Rect lockRect(0, 0, srcW, srcH);
    if (srcBmp.LockBits(&lockRect, Gdiplus::ImageLockModeRead | Gdiplus::ImageLockModeWrite,
            PixelFormat32bppARGB, &data) != Gdiplus::Ok)
        return;

    // Atlas: dark lab BG ~lum 36; glyph ink ~100–180. Only brighter pixels draw;
    // near-bg dark must be fully transparent (else semi-opaque “plates”).
    constexpr int kDarkFloor = 50;   // <= floor → alpha 0 (atlas wallpaper)
    constexpr int kBrightRef = 170;  // typical solid ink → full alpha

    auto* scan0 = static_cast<BYTE*>(data.Scan0);
    for (int y = 0; y < srcH; ++y) {
        auto* row = scan0 + y * data.Stride;
        for (int x = 0; x < srcW; ++x) {
            BYTE* p = row + x * 4; // BGRA
            const int lum = (int)(0.299 * p[2] + 0.587 * p[1] + 0.114 * p[0]);
            if (lum <= kDarkFloor) {
                p[0] = p[1] = p[2] = p[3] = 0;
                continue;
            }
            int a = (lum - kDarkFloor) * 255 / (kBrightRef - kDarkFloor);
            if (a < 0) a = 0;
            if (a > 255) a = 255;
            a = (a * (int)za) / 255;
            p[0] = zb;
            p[1] = zg;
            p[2] = zr;
            p[3] = static_cast<BYTE>(a);
        }
    }
    srcBmp.UnlockBits(&data);

    gfx.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    gfx.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);
    gfx.DrawImage(&srcBmp, Gdiplus::Rect(destX, destY, destW, destH),
        0, 0, srcW, srcH, Gdiplus::UnitPixel);
}

// Model stores UTF-8 after IniLayoutReader (DecodeHolleyEscapes: "\xf095" → U+F095).
// Also accept still-escaped Holley form if present. Never Latin-1-expand UTF-8 bytes
// (that produced 3 junk code units for one PUA and empty atlas blits).
static std::wstring LabelToWideCodepoints(const std::string& label);

static int SymbolFontPx(const Gauge& g) {
    // Explicit fontSize sizes the glyph. AUTO must not use Height (size= is the hit box).
    // Atlas was captured at fontSize 35; that is the AUTO default.
    if (g.fontSize > 0)
        return PreviewFont::ClampPx(g.fontSize);
    return 35;
}

// Latin fallback (not in the PUA atlas). Roboto 2013 at the same fontSize as
// the glyphs; GenericTypographic advance so "1"/"P" match Bronco size= boxes
// instead of the old 10×14 stub with a larger overflowing font.
static void MeasureLatinGlyph(wchar_t ch, int fontPx, int& dw, int& dh) {
    if (fontPx < 4) fontPx = 4;
    Gdiplus::Bitmap bmp(8, 8, PixelFormat32bppARGB);
    Gdiplus::Graphics gfx(&bmp);
    gfx.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);
    const Gdiplus::FontFamily& family = PreviewFont::Family();
    Gdiplus::Font font(&family, static_cast<Gdiplus::REAL>(fontPx),
        Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
    Gdiplus::StringFormat fmt(Gdiplus::StringFormat::GenericTypographic());
    fmt.SetFormatFlags(Gdiplus::StringFormatFlagsNoWrap | Gdiplus::StringFormatFlagsNoClip
        | Gdiplus::StringFormatFlagsMeasureTrailingSpaces);
    wchar_t s[2] = { ch, 0 };
    Gdiplus::RectF bounds;
    gfx.MeasureString(s, 1, &font, Gdiplus::PointF(0, 0), &fmt, &bounds);
    dw = (std::max)(1, static_cast<int>(std::ceil(bounds.Width)));
    // Cap band matches atlas ink at this FS (Digital/LED 0.72). Full em looks
    // oversized next to PUA cells captured at fontSize 35.
    const int cap = (std::max)(4, static_cast<int>(std::lround(0.72 * static_cast<double>(fontPx))));
    dh = cap;
}

struct SymbolGlyphDraw {
    const SymbolAtlasEntry* e;
    int dw, dh;
    wchar_t ch; // 0 = space
};

static void LayoutSymbolGlyphs(const Gauge& g, std::vector<SymbolGlyphDraw>& glyphs, int& maxDh) {
    glyphs.clear();
    const std::wstring ws = LabelToWideCodepoints(g.label);
    const int fontPx = SymbolFontPx(g);
    const double scale = static_cast<double>(fontPx) / 35.0;
    const int spaceW = (std::max)(2, static_cast<int>(std::lround(12.0 * scale)));
    const int lineH = (std::max)(4, static_cast<int>(std::lround(0.72 * static_cast<double>(fontPx))));
    maxDh = lineH;
    for (wchar_t ch : ws) {
        if (ch == L' ' || ch == L'\t') {
            glyphs.push_back({ nullptr, spaceW, lineH, 0 });
            continue;
        }
        const SymbolAtlasEntry* e = FindSymbolAtlasEntry(static_cast<unsigned>(ch));
        if (e && e->w > 0 && e->h > 0) {
            int dw = static_cast<int>(std::lround(static_cast<double>(e->w) * scale));
            int dh = static_cast<int>(std::lround(static_cast<double>(e->h) * scale));
            if (dw < 1) dw = 1;
            if (dh < 1) dh = 1;
            glyphs.push_back({ e, dw, dh, ch });
            if (dh > maxDh) maxDh = dh;
        } else {
            int dw = 0, dh = 0;
            MeasureLatinGlyph(ch, fontPx, dw, dh);
            glyphs.push_back({ nullptr, dw, dh, ch });
            if (dh > maxDh) maxDh = dh;
        }
    }
}

static void MeasureSymbolInk(const Gauge& g, int& inkW, int& inkH, int& glyphCount) {
    inkW = 0;
    inkH = 4;
    glyphCount = 0;
    std::vector<SymbolGlyphDraw> glyphs;
    int maxDh = 4;
    LayoutSymbolGlyphs(g, glyphs, maxDh);
    for (const auto& gly : glyphs) {
        inkW += gly.dw;
        if (gly.ch != 0)
            ++glyphCount;
    }
    inkH = maxDh;
    if (inkW < 4) inkW = 4;
}

// Top of one snip, relative to pos.y.
// PUA: RobotoPlus hhea baseline (ascent 2353 in a 2048 em) below pos.y.
// The snip is centered on that glyph's font ink. No side bearing.
// Latin keeps the 1.43×fontSize cell.
static int SymbolGlyphTop(const SymbolGlyphDraw& gly, int fontPx, int maxDh, int boxH) {
    if (gly.e && fontPx > 0) {
        const SymbolVerticalInk* v = FindSymbolVerticalInk(gly.e->codepoint);
        if (v && v->ymax > v->ymin) {
            const double em = static_cast<double>(fontPx) / 2048.0;
            const double fontTop = (2353.0 - static_cast<double>(v->ymax)) * em;
            const double fontH = static_cast<double>(v->ymax - v->ymin) * em;
            int top = static_cast<int>(std::lround(fontTop + (fontH - static_cast<double>(gly.dh)) / 2.0));
            int ndx = 0, ndy = 0;
            SymbolNudgeXY(gly.e->codepoint, fontPx, ndx, ndy);
            return top + ndy;
        }
    }
    if (gly.ch && fontPx > 0) {
        // Roboto 2013 cap of H (y 0..1456) on the hhea baseline (ascent 1900).
        const double em = static_cast<double>(fontPx) / 2048.0;
        const double fontTop = (1900.0 - 1456.0) * em;
        const double fontH = 1456.0 * em;
        return static_cast<int>(std::lround(fontTop + (fontH - static_cast<double>(gly.dh)) / 2.0));
    }
    const int natH = (std::max)(maxDh, static_cast<int>(std::lround(static_cast<double>(fontPx) * 1.43)));
    const int cellH = (std::min)(boxH, natH);
    const int yBase = (std::max)(0, (cellH - maxDh) / 2);
    return yBase + (maxDh - gly.dh) / 2;
}

void LayoutRenderer::SymbolHitBox(const Gauge& g, int& hx, int& hy, int& hw, int& hh) const {
    int boxW = 0, boxH = 0;
    EffectiveGaugeSize(g, boxW, boxH);
    int inkW = 0, inkH = 0, glyphCount = 0;
    MeasureSymbolInk(g, inkW, inkH, glyphCount);
    if (glyphCount <= 0) {
        hx = g.x;
        hy = g.y;
        hw = boxW;
        hh = boxH;
        return;
    }
    // Dash: glyph origin is pos=, not the center of size=. Extra size= is hit box only.
    // Vertical: PUA snips sit on the hhea baseline. Latin stays in the 1.43× cell.
    const int fontPx = SymbolFontPx(g);
    std::vector<SymbolGlyphDraw> placed;
    int placedMaxDh = 4;
    LayoutSymbolGlyphs(g, placed, placedMaxDh);
    int minTop = 0, maxBot = 0, runW = 0;
    bool any = false;
    for (const auto& gly : placed) {
        const int gy = SymbolGlyphTop(gly, fontPx, placedMaxDh, boxH);
        if (!any) {
            minTop = gy;
            maxBot = gy + gly.dh;
            any = true;
        } else {
            if (gy < minTop) minTop = gy;
            if (gy + gly.dh > maxBot) maxBot = gy + gly.dh;
        }
        runW += gly.dw;
    }
    hx = g.x;
    hy = g.y + minTop;
    hw = runW > 0 ? runW : inkW;
    hh = any ? (maxBot - minTop) : inkH;
    if (hw > boxW) hw = boxW;
    if (hh > boxH) hh = boxH;
    if (hy + hh > g.y + boxH) hh = g.y + boxH - hy;
    if (hh < 1) hh = 1;
}

static std::wstring LabelToWideCodepoints(const std::string& label) {
    return IniUtils::Utf8ToWide(label);
}

// gaugeType 8 — Symbol: atlas-tinted glyphs (TX-all_symbols.png).
// Type 8 Symbol: atlas blit, scale fontSize/35, zone tint from demo slider.
// PUA vertical is the RobotoPlus hhea baseline below pos.y, plus a per-glyph
// dash nudge. Latin uses the Roboto 2013 cap baseline. X-squeeze to size= stays.
void LayoutRenderer::DrawSymbol(HDC hdc, const Gauge& g, int left, int top, int w, int h, bool selected) const {
    if (w < 4) w = 4;
    if (h < 4) h = 4;

    // Symbol never paints a face plate — wallpaper shows through (dash language).
    // (INI may still store background= black; ignore for preview.)
    int selX = 0, selY = 0, selW = 0, selH = 0;
    SymbolHitBox(g, selX, selY, selW, selH);
    if (selected)
        DrawSelectionRect(hdc, originX + selX, originY + selY, selW, selH, 1);

    if (g.label.empty()) return;

    // Color = zone (c0–c4) from demo value. Low/High checkboxes gate c0/c1 and
    // c3/c4; unchecked sides stay on normal (c2), same as analog/digital.
    // Label Transparency = foreground alpha only — do not replace zone RGB with FG.
    const double demo = DemoValue(g);
    COLORREF zc = ZoneColorForValue(g, demo);
    BYTE zr = GetRValue(zc), zg = GetGValue(zc), zb = GetBValue(zc);
    BYTE za = 255;
    if (g.foreground) {
        BYTE fa, fr, fg, fb;
        UnpackHolley(g.foreground, fa, fr, fg, fb);
        (void)fr; (void)fg; (void)fb;
        if (fa > 0 && fa < 255)
            za = fa;
    }
    if ((zr | zg | zb) == 0) {
        zr = zg = zb = 220;
    }

    // Uniform scale from atlas capture size — do NOT normalize each glyph's
    // tight bbox height to the same drawH (that made short glyphs look big and
    // tall glyphs small). One scale for all: preserves relative ink sizes.
    // Atlas TX-all_symbols captured at fontSize=35.
    const int fontPx = SymbolFontPx(g);

    std::vector<SymbolGlyphDraw> glyphs;
    int maxDh = 4;
    LayoutSymbolGlyphs(g, glyphs, maxDh);
    if (glyphs.empty()) return;

    Gdiplus::Image* atlas = GetSymbolAtlasImage();
    Gdiplus::Graphics gfx(hdc);
    gfx.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    gfx.SetCompositingMode(Gdiplus::CompositingModeSourceOver);
    gfx.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);
    gfx.SetClip(Gdiplus::Rect(left, top, w, h));

    // Dash: left-align at pos=. Do not center in size= (oversized box is hit only).
    int x = left;

    // X-only squeeze to the stored width; height (already accepted) stays put.
    int runW = 0;
    for (const auto& gly : glyphs)
        runW += gly.dw;
    float xFit = 1.f;
    if (runW > w && w > 0)
        xFit = static_cast<float>(w) / static_cast<float>(runW);
    const int saved = gfx.Save();
    if (xFit < 1.f) {
        gfx.TranslateTransform(static_cast<float>(left), 0.f);
        gfx.ScaleTransform(xFit, 1.f);
        gfx.TranslateTransform(-static_cast<float>(left), 0.f);
    }

    const Gdiplus::FontFamily& latinFamily = PreviewFont::Family();
    Gdiplus::Font asciiFont(&latinFamily, static_cast<Gdiplus::REAL>(fontPx),
        Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
    Gdiplus::SolidBrush asciiBrush(Gdiplus::Color(za, zr, zg, zb));
    Gdiplus::StringFormat asciiFmt(Gdiplus::StringFormat::GenericTypographic());
    asciiFmt.SetAlignment(Gdiplus::StringAlignmentNear);
    asciiFmt.SetLineAlignment(Gdiplus::StringAlignmentNear);
    asciiFmt.SetFormatFlags(Gdiplus::StringFormatFlagsNoWrap | Gdiplus::StringFormatFlagsNoClip);

    for (const auto& gly : glyphs) {
        const int gy = top + SymbolGlyphTop(gly, fontPx, maxDh, h);
        int nx = x;
        if (gly.e) {
            int ndx = 0, ndy = 0;
            SymbolNudgeXY(gly.e->codepoint, fontPx, ndx, ndy);
            nx += ndx;
        } else if (gly.ch) {
            nx += static_cast<int>(std::lround(-2.0 * static_cast<double>(fontPx) / 35.0));
        }
        if (gly.e && atlas) {
            BlitTintedAtlasGlyph(gfx, atlas,
                gly.e->x, gly.e->y, gly.e->w, gly.e->h,
                nx, gy, gly.dw, gly.dh,
                zr, zg, zb, za);
        } else if (gly.ch) {
            wchar_t s[2] = { gly.ch, 0 };
            gfx.DrawString(s, 1, &asciiFont,
                Gdiplus::PointF(static_cast<Gdiplus::REAL>(nx), static_cast<Gdiplus::REAL>(gy)),
                &asciiFmt, &asciiBrush);
        }
        x += gly.dw;
    }
    gfx.Restore(saved);
}

// Unknown gaugeType — selection chrome + type tag only.
void LayoutRenderer::DrawUnknown(HDC hdc, const Gauge& g, int left, int top, int w, int h, bool selected) const {
    if (selected)
        DrawSelectionRect(hdc, left, top, w, h, 1);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, selected ? RGB(0, 100, 200) : RGB(200, 200, 200));
    char typeBuf[64];
    sprintf_s(typeBuf, "T%d %s", g.gaugeType, GaugeTypeName(g.gaugeType));
    TextOutA(hdc, left + 4, top + 4, typeBuf, (int)strlen(typeBuf));
}
