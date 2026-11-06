#pragma once

// =============================================================================
// IniLayoutReader.h — Parse Holley Pro Dash layout .ini into a LayoutDocument
//
// Supports:
//   • Single-screen files with a [Layout] section
//   • Multi-screen packs with [Defaults] + [Layout0]…[LayoutN]
//
// Keys use Holley paths: analogMeters\0\label, outputs\1\text, labels\2\label.
// transswitchpos lines are ignored (parked for later).
// =============================================================================

#include "../Model/LayoutDocument.h"
#include <functional>
#include <string>

class IniLayoutReader {
public:
    // Optional callback for progress / error lines (wired to the editor log).
    using LogFn = std::function<void(const std::string&)>;

    explicit IniLayoutReader(LogFn log = nullptr) : log_(std::move(log)) {}

    // Fills doc (clears it first). Call with a temporary document and swap
    // on success so a failed load cannot wipe the editor's current layout.
    // Returns false if the file could not be opened or has no layout section.
    bool Load(const std::string& path, LayoutDocument& doc);

private:
    LogFn log_; // May be null
    void Log(const std::string& msg) const;
};
