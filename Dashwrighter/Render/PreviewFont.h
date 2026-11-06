#pragma once

// Preview Latin face: Google Roboto Version 1.200310 (2013), Apache 2.0.
// Same cut Holley used for RobotoPlus Latin. Symbol atlas PUA glyphs stay
// the dash-capture PNG. Inspector chrome is unchanged (system UI font).

#include <windows.h>
#include <gdiplus.h>

namespace PreviewFont {
    constexpr const wchar_t* FaceW = L"Roboto";
    constexpr int kMin = 1;
    constexpr int kMax = 240;

    inline int ClampPx(int px) {
        if (px < kMin) return kMin;
        if (px > kMax) return kMax;
        return px;
    }

    inline int Resolve(int fontSize, int autoPx) {
        if (fontSize > 0) return ClampPx(fontSize);
        return ClampPx(autoPx);
    }

    inline bool IsExplicit(int fontSize) { return fontSize > 0; }

    // Load embedded/loose 2013 Roboto into a process-private collection.
    void EnsureLoaded();
    void Shutdown();

    // GDI+ family from the private collection (not a system-installed Roboto).
    const Gdiplus::FontFamily& Family();

    HFONT CreateGdi(int px, int weight = FW_NORMAL);
}
