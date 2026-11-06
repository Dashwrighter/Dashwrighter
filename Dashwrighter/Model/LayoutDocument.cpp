// =============================================================================
// LayoutDocument.cpp — Active-screen helpers and document lifecycle
// =============================================================================

#include "LayoutDocument.h"
#include <algorithm>
#include <cctype>

// Drop all screens/defaults and restore the default 1280×480 profile.
void LayoutDocument::Clear() {
    defaults = LayoutDefaults{};
    screens.clear();
    activeScreenIndex = 0;
    sourcePath.clear();
    profile = DashProfile::For12_3();
}

// Empty means "nothing useful to edit" — no screens or all screens are blank.
bool LayoutDocument::Empty() const {
    if (screens.empty()) return true;
    for (const auto& s : screens) {
        if (!s.gauges.empty() || !s.outputs.empty() || !s.labels.empty())
            return false;
    }
    return true;
}

// Returns the screen currently being edited (clamped to [0] if index is bad).
LayoutScreen* LayoutDocument::ActiveScreen() {
    if (screens.empty()) return nullptr;
    if (activeScreenIndex < 0 || activeScreenIndex >= static_cast<int>(screens.size()))
        return &screens[0];
    return &screens[activeScreenIndex];
}

const LayoutScreen* LayoutDocument::ActiveScreen() const {
    if (screens.empty()) return nullptr;
    if (activeScreenIndex < 0 || activeScreenIndex >= static_cast<int>(screens.size()))
        return &screens[0];
    return &screens[activeScreenIndex];
}

// Non-const gauge list for the active screen. Static empty avoids null refs
// when no screen exists (callers can still iterate safely).
std::vector<Gauge>& LayoutDocument::Gauges() {
    static std::vector<Gauge> empty;
    auto* s = ActiveScreen();
    return s ? s->gauges : empty;
}

std::vector<Output>& LayoutDocument::Outputs() {
    static std::vector<Output> empty;
    auto* s = ActiveScreen();
    return s ? s->outputs : empty;
}

std::vector<Label>& LayoutDocument::Labels() {
    static std::vector<Label> empty;
    auto* s = ActiveScreen();
    return s ? s->labels : empty;
}

const std::vector<Gauge>& LayoutDocument::Gauges() const {
    static const std::vector<Gauge> empty;
    auto* s = ActiveScreen();
    return s ? s->gauges : empty;
}

const std::vector<Output>& LayoutDocument::Outputs() const {
    static const std::vector<Output> empty;
    auto* s = ActiveScreen();
    return s ? s->outputs : empty;
}

const std::vector<Label>& LayoutDocument::Labels() const {
    static const std::vector<Label> empty;
    auto* s = ActiveScreen();
    return s ? s->labels : empty;
}

std::string LayoutDocument::ActiveBackgroundFileName() const {
    auto* s = ActiveScreen();
    return s ? s->backgroundFileName : std::string();
}

// Updates both the local basename and the Holley device path written to INI.
void LayoutDocument::SetActiveBackgroundFileName(const std::string& fileName) {
    auto* s = ActiveScreen();
    if (!s) return;
    s->backgroundFileName = fileName;
    if (!fileName.empty())
        s->backgroundPath = "/internal/images/" + fileName;
    else
        s->backgroundPath.clear();
}

bool LayoutDocument::SetActiveScreenIndex(int index) {
    if (index < 0 || index >= static_cast<int>(screens.size())) return false;
    activeScreenIndex = index;
    return true;
}

// Path-based display-name guess. Both form factors use 1280×480; layouts work
// on either dash. This only labels 6.86-named packs in the status line.
void LayoutDocument::ApplyProfileFromSourcePath(const std::string& path) {
    std::string lower = path;
    std::transform(lower.begin(), lower.end(), lower.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (lower.find("6.86") != std::string::npos || lower.find("6_86") != std::string::npos) {
        profile = DashProfile::For6_86();
    } else {
        profile = DashProfile::For12_3();
    }
}
