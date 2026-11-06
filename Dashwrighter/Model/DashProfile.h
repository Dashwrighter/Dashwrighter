#pragma once

// =============================================================================
// DashProfile.h — Hardware / canvas profile for a Holley Pro Dash form factor
//
// 12.3" and 6.86" Pro Dash share the same 1280×480 pixel canvas (same
// proportions). Layout INIs created or edited here work on either unit.
// displayName is the status-bar and log label. It does not change geometry.
// =============================================================================

#include <string>

// Canvas dimensions and a human-readable name for the status bar / log.
struct DashProfile {
    static constexpr int kCanvasW = 1280;
    static constexpr int kCanvasH = 480;

    int canvasWidth = kCanvasW;
    int canvasHeight = kCanvasH;
    const char* displayName = "12.3\" Pro Dash"; // Shown in UI chrome

    static DashProfile For12_3() {
        DashProfile p;
        p.canvasWidth = kCanvasW;
        p.canvasHeight = kCanvasH;
        p.displayName = "12.3\" Pro Dash";
        return p;
    }

    static DashProfile For6_86() {
        DashProfile p;
        p.canvasWidth = kCanvasW;
        p.canvasHeight = kCanvasH;
        p.displayName = "6.86\" Pro Dash";
        return p;
    }
};
