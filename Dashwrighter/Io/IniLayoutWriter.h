#pragma once

// =============================================================================
// IniLayoutWriter.h — Serialize a LayoutDocument back to a Holley layout .ini
//
// Multi-screen: writes [Defaults] (if present) then each [LayoutN].
// Single-screen: writes one [Layout] section.
//
// For each object and [Defaults], only keys that were present at load
// (originalKeys) are written, unless originalKeys is empty — then a full
// standard key set is emitted. transswitchpos is not written.
// Style 2 orientation/hor, rpmModeEnabled, and wfr are stored as loaded.
// =============================================================================

#include "../Model/LayoutDocument.h"
#include <functional>
#include <string>

class IniLayoutWriter {
public:
    using LogFn = std::function<void(const std::string&)>;

    explicit IniLayoutWriter(LogFn log = nullptr) : log_(std::move(log)) {}

    // Overwrites path with the document contents. Returns false on I/O failure.
    bool Save(const std::string& path, const LayoutDocument& doc);

private:
    LogFn log_;
    void Log(const std::string& msg) const;
};
