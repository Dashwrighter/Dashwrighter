#pragma once

// =============================================================================
// LayoutRenderer.h — GDI+/GDI preview of one LayoutScreen
//
// Draws the background image (or black), then gauges, labels, and outputs into
// an HDC at (originX, originY). Needle/bar/digital values use DemoValue():
//   minValue + demoNorm * (maxValue - minValue), with demoNorm in [0,1]
// (editor preview slider; default 0.40).
//
// Gauges may sit partially off-canvas; Draw() clips to the canvas rect so only
// the on-screen fragment is visible (matches Holley dash behavior).
//
// fontSize policy (TL14): Holley pixels (5–240). On free Labels / Symbol /
// switch captions it is 1:1. On other gauges it primarily sizes the *caption*
// (displayLabel); numeric value and scale use layout/auto-fit. fontSize ≤ 0
// means AUTO. Preview Latin is Roboto 2013 (see PreviewFont.h).
// =============================================================================

#include "../Model/LayoutDocument.h"
#include <windows.h>
#include <gdiplus.h>
#include <vector>

enum class PreviewHitKind { Gauge, Label, Output };

struct PreviewHit {
    PreviewHitKind kind = PreviewHitKind::Gauge;
    int index = -1;
};

class LayoutRenderer {
public:
    // Top-left of the canvas in client coordinates of the host window.
    int originX = 0;
    int originY = 0;

    // Selection indices into the screen's vectors (-1 = none). Drive highlight rings.
    int selectedGauge = -1;
    int selectedOutput = -1;
    int selectedLabel = -1;
    // Position overlay: ring every gauge (not switches / labels).
    bool ringAllGauges = false;

    // Normalized preview value 0 = minValue, 1 = maxValue (editor Appearance slider).
    // Also drives soft-switch Off/On: demoNorm < 0.5 → Off frame, ≥ 0.5 → On.
    double demoNorm = 0.40;

    // Preview value from demoNorm across [minValue, maxValue] (public for readout UI).
    double DemoValue(const Gauge& g) const;

    // Full paint of one screen into hdc (background + all widgets), clipped.
    void Draw(HDC hdc, const LayoutScreen& screen, Gdiplus::Image* background,
              int canvasW, int canvasH) const;

    // Hit-tests in window client coords; return object index or -1.
    // Only widgets that intersect the canvas and contain the point count.
    int HitTestGauge(const LayoutScreen& screen, int mx, int my, int canvasW, int canvasH) const;
    // Symbol painted-ink box in canvas coords (g.x/g.y origin). Empty label → stored size.
    void SymbolHitBox(const Gauge& g, int& hx, int& hy, int& hw, int& hh) const;
    int HitTestOutput(const LayoutScreen& screen, int mx, int my, int canvasW, int canvasH) const;
    int HitTestLabel(const LayoutScreen& screen, int mx, int my, int canvasW, int canvasH) const;
    // All widgets at the point, topmost first (reverse of draw: outputs, labels, gauges).
    // Analog / Thin Analog hit the outer circle, not the square.
    void HitTestStack(const LayoutScreen& screen, int mx, int my, int canvasW, int canvasH,
        std::vector<PreviewHit>& out) const;

    // Shared TX-all_symbols atlas (picker + DrawSymbol).
    static Gdiplus::Image* GetSymbolAtlasImage();
    // Free bezel / switch / LED / atlas caches. Call before GdiplusShutdown.
    static void ReleaseCachedImages();

private:
    // Dispatch by type / draw selection chrome
    void DrawGauge(HDC hdc, const Gauge& g, bool selected) const;
    void DrawOutput(HDC hdc, const Output& o, bool selected) const;
    void DrawLabelItem(HDC hdc, const Label& lab, bool selected) const;

    // Per-gaugeType painters (left/top/w/h are already in window coordinates)
    void DrawAnalog(HDC hdc, const Gauge& g, int left, int top, int w, int h, bool selected) const;
    void DrawThinAnalog(HDC hdc, const Gauge& g, int left, int top, int w, int h, bool selected) const;
    void DrawBargraph(HDC hdc, const Gauge& g, int left, int top, int w, int h, bool selected) const;
    void DrawBargraphStyle2(HDC hdc, const Gauge& g, int left, int top, int w, int h, bool selected) const;
    void DrawGraph(HDC hdc, const Gauge& g, int left, int top, int w, int h, bool selected) const;
    void DrawDigital(HDC hdc, const Gauge& g, int left, int top, int w, int h, bool selected) const;
    void DrawStatusLED(HDC hdc, const Gauge& g, int left, int top, int w, int h, bool selected) const;
    void DrawRPMBar(HDC hdc, const Gauge& g, int left, int top, int w, int h, bool selected) const;
    void DrawSymbol(HDC hdc, const Gauge& g, int left, int top, int w, int h, bool selected) const;
    void DrawUnknown(HDC hdc, const Gauge& g, int left, int top, int w, int h, bool selected) const;

    // Analog face helpers. Tick/arc radii are pixel offsets from size=, not fractions of r.
    void DrawAnalogTicks(HDC hdc, const Gauge& g, int cx, int cy,
        float tickOuterRx, float tickOuterRy, float majorLen, float minorLen,
        float majorPenW, float minorPenW, int layoutR) const;
    // Product Border (INI crown + ct). Blit stretched onto measured ID/OD; Original chrome OD=size, 9px ring.
    bool DrawProductBorder(HDC hdc, const Gauge& g, int left, int top, int w, int h) const;
    // Procedural rings: Original chrome (9px, outer edge at rx) or fallback.
    void DrawCrownCircle(HDC hdc, int cx, int cy, int rx, int ry, bool originalChrome = false) const;
    // Resolve WidgetGraphics bezel PNG (cached). Always the lg asset.
    static Gdiplus::Image* GetBezelImage(int ctStyle);
    // Soft-switch 3-frame strip from WidgetGraphics (cached by style id).
    static Gdiplus::Image* GetSwitchSheetImage(int style);
    // File stem for style (nullptr if unknown). See Switch-Style-Map.md.
    static const wchar_t* SwitchStyleSheetFile(int style);
    // Editor selection: blue ring/box only while the widget is selected.
    void DrawSelectionCircle(HDC hdc, int cx, int cy, int rx, int ry) const;
    void DrawSelectionRect(HDC hdc, int left, int top, int w, int h, int inflate = 1) const;
    void DrawZoneArcBars(HDC hdc, const Gauge& g, int cx, int cy,
        float outerRx, float outerRy, float barHeight) const;
    void DrawNeedle(HDC hdc, int cx, int cy, int rx, int ry, double angleDeg, COLORREF color) const;

    // Fill a rect using Holley AARRGGBB; alpha 0 means skip (fully transparent).
    void FillHolleyBackground(HDC hdc, int left, int top, int w, int h,
                              unsigned int holleyBg) const;

    // Map channel value onto the analog sweep. Signed (endAngle - startAngle)
    // may be negative → reverse / mirrored needles (e.g. default5 GPS 180→0).
    static double ValueToAngle(const Gauge& g, double value);

    // Holley angle (0° = 12 o'clock, CW+) → unit direction for GDI drawing.
    static void AngleToDir(double angleDeg, double& dx, double& dy);

    // Holley AARRGGBB → GDI COLORREF (RGB only; alpha ignored for pens/text).
    static COLORREF HolleyToColor(unsigned int c);

    // Zone color c0…c4 for a value relative to caution/normal thresholds.
    // Low/high enables gate c0/c1 and c3/c4; otherwise c2 (or light gray).
    static COLORREF ZoneColorForValue(const Gauge& g, double value);

    // Split Holley color into A,R,G,B bytes.
    static void UnpackHolley(unsigned int c, BYTE& a, BYTE& r, BYTE& g, BYTE& b);

    // Holley AARRGGBB → Gdiplus::Color (preserves alpha).
    static Gdiplus::Color HolleyToGdip(unsigned int c);

    static bool RectsIntersect(int ax, int ay, int aw, int ah, int bx, int by, int bw, int bh);
    static bool PointInRect(int px, int py, int x, int y, int w, int h);
    bool GaugeHitsPoint(const Gauge& g, int px, int py, int canvasW, int canvasH) const;
};
