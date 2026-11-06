#pragma once

// =============================================================================
// LayoutDocument.h — In-memory representation of a loaded (or empty) layout file
//
// Owns zero or more LayoutScreen pages, optional multi-screen Defaults, the
// path the file was loaded/saved from, and the DashProfile for canvas size.
// Editor code always edits the active screen via ActiveScreen() / Gauges() etc.
// =============================================================================

#include "DashProfile.h"
#include "LayoutTypes.h"
#include <string>
#include <vector>

class LayoutDocument {
public:
    DashProfile profile = DashProfile::For12_3(); // Label + 1280×480 canvas (same for 6.86")
    LayoutDefaults defaults;                      // [Defaults] when multi-screen
    std::vector<LayoutScreen> screens;            // One entry per [Layout] / [LayoutN]
    int activeScreenIndex = 0;                    // Which screen the UI is editing
    std::string sourcePath;                       // Last load/save path (may be empty)

    // Reset to a blank document (1280×480 canvas, no screens).
    void Clear();

    // True if there are no screens, or every screen has no gauges/outputs/labels.
    bool Empty() const;

    // Pointer to screens[activeScreenIndex], or nullptr if no screens.
    // Out-of-range index falls back to screen 0.
    LayoutScreen* ActiveScreen();
    const LayoutScreen* ActiveScreen() const;

    // Convenience: gauges/outputs/labels of the active screen (empty static if none).
    std::vector<Gauge>& Gauges();
    std::vector<Output>& Outputs();
    std::vector<Label>& Labels();
    const std::vector<Gauge>& Gauges() const;
    const std::vector<Output>& Outputs() const;
    const std::vector<Label>& Labels() const;

    // Basename of the active screen's background image, or empty.
    std::string ActiveBackgroundFileName() const;

    // Sets basename and rebuilds Holley-style backgroundPath (/internal/images/…).
    void SetActiveBackgroundFileName(const std::string& fileName);

    // Switch the active page; returns false if index is out of range.
    bool SetActiveScreenIndex(int index);

    int ScreenCount() const { return static_cast<int>(screens.size()); }

    // Multi-screen pack heuristic: more than one screen, or [Defaults] was present.
    bool IsMultiScreen() const { return screens.size() > 1 || defaults.present; }

    // Path hint only: "6.86" / "6_86" → For6_86() display name, else For12_3().
    // Canvas is 1280×480 either way; INIs are interchangeable between units.
    void ApplyProfileFromSourcePath(const std::string& path);
};
