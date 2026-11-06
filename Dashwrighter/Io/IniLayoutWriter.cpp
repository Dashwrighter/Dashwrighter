// =============================================================================
// IniLayoutWriter.cpp — Emit Holley-compatible layout INI text
//
// Round-trip strategy: if Gauge/Output/Label.originalKeys is non-empty, only
// those keys are written (preserves optional fields and load order intent).
// New objects (empty originalKeys) get a full standard key set.
// Indices are 1-based to match Holley files.
// =============================================================================

#include "IniLayoutWriter.h"
#include "IniUtils.h"
#include <fstream>
#include <set>

using namespace IniUtils;

void IniLayoutWriter::Log(const std::string& msg) const {
    if (log_) log_(msg);
}

// Write section\index\key=value only if key was present when the object was loaded.
static void WriteKeyIfPresent(std::ofstream& out, const std::set<std::string>& keys,
    const std::string& section, int index,
    const std::string& shortKey, const std::string& value) {
    if (keys.count(shortKey))
        out << section << "\\" << index << "\\" << shortKey << "=" << value << "\n";
}

// Write the key if force==true (new object) or it was in originalKeys.
static void WriteKeyAlways(std::ofstream& out, const std::set<std::string>& keys, bool force,
    const std::string& section, int index,
    const std::string& shortKey, const std::string& value) {
    if (force || keys.count(shortKey))
        out << section << "\\" << index << "\\" << shortKey << "=" << value << "\n";
}

// Emit one analogMeters\idx\* block.
static void WriteGauge(std::ofstream& out, const Gauge& g, int idx) {
    const auto& k = g.originalKeys;
    bool force = k.empty();
    auto W = [&](const std::string& sk, const std::string& v) {
        WriteKeyAlways(out, k, force, "analogMeters", idx, sk, v);
    };
    W("label", QuoteIniValueIfNeeded(EncodeHolleyEscapes(g.label)));
    W("fontSize", std::to_string(g.fontSize));
    W("pos", "\"(" + std::to_string(g.x) + "," + std::to_string(g.y) + ")\"");
    W("size", "\"(" + std::to_string(g.width) + "," + std::to_string(g.height) + ")\"");
    W("units", QuoteIniValueIfNeeded(EncodeHolleyEscapes(g.units)));
    W("precision", std::to_string(g.precision));
    W("minValue", CleanDouble(g.minValue));
    W("maxValue", CleanDouble(g.maxValue));
    W("scalingDivisor", CleanDouble(g.scalingDivisor));
    W("minCaution", CleanDouble(g.minCaution));
    W("maxCaution", CleanDouble(g.maxCaution));
    W("minNormal", CleanDouble(g.minNormal));
    W("maxNormal", CleanDouble(g.maxNormal));
    W("minThresholdEnabled", BoolStr(g.minThresholdEnabled));
    W("maxThresholdEnabled", BoolStr(g.maxThresholdEnabled));
    W("minPeakEnabled", BoolStr(g.minPeakEnabled));
    W("maxPeakEnabled", BoolStr(g.maxPeakEnabled));
    W("numericIndicatorEnabled", BoolStr(g.numericIndicatorEnabled));
    W("peakReductionEnabled", BoolStr(g.peakReductionEnabled));
    W("channel", std::to_string(g.channel));
    W("foreground", std::to_string(g.foreground));
    W("background", std::to_string(g.background));
    W("startAngle", std::to_string(g.startAngle));
    W("endAngle", std::to_string(g.endAngle));
    W("steps", std::to_string(g.steps));
    W("mtics", std::to_string(g.mtics));
    W("crown", BoolStr(g.crown));
    W("rpmModeEnabled", BoolStr(g.rpmModeEnabled));
    W("gaugeType", std::to_string(g.gaugeType));
    W("displayUnits", BoolStr(g.displayUnits));
    W("displayLabel", BoolStr(g.displayLabel));
    W("isLED", BoolStr(g.isLED));
    W("valueMult2", CleanDouble(g.valueMult2));
    W("valueMult", CleanDouble(g.valueMult));
    W("valueMultB", CleanDouble(g.valueMultB));
    W("scaleValues", BoolStr(g.scaleValues));
    W("orientation", BoolStr(g.orientation));
    W("updateRate", std::to_string(g.updateRate));
    W("c0", std::to_string(g.c0));
    W("c1", std::to_string(g.c1));
    W("c2", std::to_string(g.c2));
    W("c3", std::to_string(g.c3));
    W("c4", std::to_string(g.c4));
    W("wfr", std::to_string(g.wfr));
    W("bm", BoolStr(g.bm));
    W("hor", BoolStr(g.hor));
    W("nnl", std::to_string(g.nnl));
    W("ncl", std::to_string(g.ncl));
    W("av", std::to_string(g.av));
    // Never write avFlag — editor invention; Holley only has av.
    // ct appears on some exports (e.g. Symbol); keep when loaded or force-full write.
    if (k.count("ct") || force) W("ct", std::to_string(g.ct));
}

// Emit one outputs\idx\* block.
static void WriteOutput(std::ofstream& out, const Output& o, int idx) {
    const auto& k = o.originalKeys;
    bool force = k.empty();
    auto W = [&](const std::string& sk, const std::string& v) {
        WriteKeyAlways(out, k, force, "outputs", idx, sk, v);
    };
    W("text", QuoteIniValueIfNeeded(EncodeHolleyEscapes(o.text)));
    W("pos", "\"(" + std::to_string(o.x) + "," + std::to_string(o.y) + ")\"");
    W("channel", std::to_string(o.channel));
    W("style", std::to_string(o.style));
    W("inactiveColor", std::to_string(o.inactiveColor));
    W("activeColor", std::to_string(o.activeColor));
    W("background", std::to_string(o.background));
    W("fontSize", std::to_string(o.fontSize));
}

// Emit one labels\idx\* block. Caption key is label (not text).
static void WriteLabel(std::ofstream& out, const Label& lab, int idx) {
    const auto& k = lab.originalKeys;
    bool force = k.empty();
    auto W = [&](const std::string& sk, const std::string& v) {
        WriteKeyAlways(out, k, force, "labels", idx, sk, v);
    };
    // Never write text — editor-legacy on labels. Switch caption is outputs\text.
    W("label", QuoteIniValueIfNeeded(EncodeHolleyEscapes(lab.text)));
    W("pos", "\"(" + std::to_string(lab.x) + "," + std::to_string(lab.y) + ")\"");
    W("fontSize", std::to_string(lab.fontSize));
    W("foreground", std::to_string(lab.foreground));
    W("background", std::to_string(lab.background));
}

// One [Layout] / [LayoutN] section: background, version, then objects + sizes.
static void WriteScreen(std::ofstream& out, const LayoutScreen& screen) {
    out << "[" << screen.sectionName << "]\n";
    if (!screen.backgroundPath.empty())
        out << "Background=" << screen.backgroundPath << "\n";
    else if (!screen.backgroundFileName.empty())
        out << "Background=/internal/images/" << screen.backgroundFileName << "\n";
    else
        out << "Background=/internal/images/Holley EFI gauges RaceDash-PreLoaded.jpg\n";
    out << "v=" << screen.version << "\n";

    for (size_t i = 0; i < screen.gauges.size(); ++i)
        WriteGauge(out, screen.gauges[i], static_cast<int>(i) + 1);
    out << "analogMeters\\size=" << screen.gauges.size() << "\n";

    for (size_t i = 0; i < screen.labels.size(); ++i)
        WriteLabel(out, screen.labels[i], static_cast<int>(i) + 1);
    out << "labels\\size=" << screen.labels.size() << "\n";

    for (size_t i = 0; i < screen.outputs.size(); ++i)
        WriteOutput(out, screen.outputs[i], static_cast<int>(i) + 1);
    out << "outputs\\size=" << screen.outputs.size() << "\n";
}

// Write the whole document. Multi-screen packs get a leading [Defaults] block.
bool IniLayoutWriter::Save(const std::string& path, const LayoutDocument& doc) {
    if (doc.screens.empty()) {
        Log("Nothing to save.");
        return false;
    }

    std::ofstream out(path);
    if (!out.is_open()) {
        Log("ERROR: Could not open file for writing");
        return false;
    }

    Log("=== Saving layout ===");

    if (doc.defaults.present || doc.screens.size() > 1) {
        out << "[Defaults]\n";
        const auto& dk = doc.defaults.originalKeys;
        const bool forceDef = dk.empty();
        auto WD = [&](const std::string& sk, const std::string& v) {
            if (forceDef || dk.count(sk))
                out << sk << "=" << v << "\n";
        };
        WD("brightness", std::to_string(doc.defaults.brightness));
        WD("auto-brightness", BoolStr(doc.defaults.autoBrightness));
        WD("Screen", std::to_string(doc.defaults.screen));
        WD("clocksetbygps", BoolStr(doc.defaults.clockSetByGps));
        WD("hdllog", BoolStr(doc.defaults.hdlLog));
        WD("autoHomeScreen", BoolStr(doc.defaults.autoHomeScreen));
        WD("om", std::to_string(doc.defaults.om));
        WD("numScreens", std::to_string(doc.defaults.numScreens > 0
            ? doc.defaults.numScreens
            : static_cast<int>(doc.screens.size())));
        out << "\n";
    }

    for (size_t i = 0; i < doc.screens.size(); ++i) {
        if (i > 0) out << "\n";
        WriteScreen(out, doc.screens[i]);
    }

    out.flush();
    if (!out.good()) {
        Log("ERROR: Failed to write file");
        return false;
    }
    out.close();
    Log("Successfully saved: " + path);
    return true;
}
