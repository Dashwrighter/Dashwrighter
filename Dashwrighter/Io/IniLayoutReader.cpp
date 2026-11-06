// =============================================================================
// IniLayoutReader.cpp — Line-oriented parser for Holley layout INI files
//
// Walks the file section by section. Object fields arrive out of order as
//   analogMeters\N\field=value
// so each index is accumulated in a map, then flushed into vectors when the
// section ends (or a new layout section begins).
// =============================================================================

#include "IniLayoutReader.h"
#include "IniUtils.h"
#include <cctype>
#include <fstream>
#include <map>

using namespace IniUtils;

void IniLayoutReader::Log(const std::string& msg) const {
    if (log_) log_(msg);
}

// Apply one short key (e.g. "minValue") to a Gauge and record it in originalKeys.
static void ApplyGaugeKey(Gauge& g, const std::string& shortKey, const std::string& value) {
    // Editor-legacy. Holley has av only. Parse so old files load; do not round-trip.
    if (shortKey == "avFlag") {
        try { g.avFlag = ParseBool(value); } catch (...) {}
        return;
    }
    g.originalKeys.insert(shortKey);
    try {
        if (shortKey == "label") g.label = DecodeHolleyEscapes(value);
        else if (shortKey == "units") g.units = DecodeHolleyEscapes(value);
        else if (shortKey == "channel") g.channel = std::stoi(value);
        else if (shortKey == "pos") ParsePosition(value, g.x, g.y);
        else if (shortKey == "size") ParseSize(value, g.width, g.height);
        else if (shortKey == "minValue") g.minValue = std::stod(value);
        else if (shortKey == "maxValue") g.maxValue = std::stod(value);
        else if (shortKey == "scalingDivisor") g.scalingDivisor = std::stod(value);
        else if (shortKey == "precision") g.precision = std::stoi(value);
        else if (shortKey == "valueMult") g.valueMult = std::stod(value);
        else if (shortKey == "valueMult2") g.valueMult2 = std::stod(value);
        else if (shortKey == "valueMultB") g.valueMultB = std::stod(value);
        else if (shortKey == "scaleValues") g.scaleValues = ParseBool(value);
        else if (shortKey == "minCaution") g.minCaution = std::stod(value);
        else if (shortKey == "maxCaution") g.maxCaution = std::stod(value);
        else if (shortKey == "minNormal") g.minNormal = std::stod(value);
        else if (shortKey == "maxNormal") g.maxNormal = std::stod(value);
        else if (shortKey == "minThresholdEnabled") g.minThresholdEnabled = ParseBool(value);
        else if (shortKey == "maxThresholdEnabled") g.maxThresholdEnabled = ParseBool(value);
        else if (shortKey == "minPeakEnabled") g.minPeakEnabled = ParseBool(value);
        else if (shortKey == "maxPeakEnabled") g.maxPeakEnabled = ParseBool(value);
        else if (shortKey == "numericIndicatorEnabled") g.numericIndicatorEnabled = ParseBool(value);
        else if (shortKey == "peakReductionEnabled") g.peakReductionEnabled = ParseBool(value);
        else if (shortKey == "displayUnits") g.displayUnits = ParseBool(value);
        else if (shortKey == "displayLabel") g.displayLabel = ParseBool(value);
        else if (shortKey == "isLED") g.isLED = ParseBool(value);
        else if (shortKey == "gaugeType") g.gaugeType = std::stoi(value);
        else if (shortKey == "orientation") g.orientation = ParseBool(value);
        else if (shortKey == "hor") g.hor = ParseBool(value);
        else if (shortKey == "bm") g.bm = ParseBool(value);
        else if (shortKey == "wfr") {
            if (value == "true" || value == "True" || value == "TRUE") g.wfr = 1;
            else if (value == "false" || value == "False" || value == "FALSE") g.wfr = 0;
            else g.wfr = std::stoi(value);
        }
        else if (shortKey == "crown") g.crown = ParseBool(value);
        else if (shortKey == "rpmModeEnabled") g.rpmModeEnabled = ParseBool(value);
        else if (shortKey == "fontSize") g.fontSize = std::stoi(value);
        else if (shortKey == "startAngle") g.startAngle = std::stoi(value);
        else if (shortKey == "endAngle") g.endAngle = std::stoi(value);
        else if (shortKey == "steps") g.steps = std::stoi(value);
        else if (shortKey == "mtics") g.mtics = std::stoi(value);
        else if (shortKey == "nnl") g.nnl = std::stoi(value);
        else if (shortKey == "ncl") g.ncl = std::stoi(value);
        else if (shortKey == "av") g.av = std::stoi(value);
        else if (shortKey == "c0") g.c0 = static_cast<unsigned>(std::stoul(value));
        else if (shortKey == "c1") g.c1 = static_cast<unsigned>(std::stoul(value));
        else if (shortKey == "c2") g.c2 = static_cast<unsigned>(std::stoul(value));
        else if (shortKey == "c3") g.c3 = static_cast<unsigned>(std::stoul(value));
        else if (shortKey == "c4") g.c4 = static_cast<unsigned>(std::stoul(value));
        else if (shortKey == "foreground") g.foreground = static_cast<unsigned>(std::stoul(value));
        else if (shortKey == "background") g.background = static_cast<unsigned>(std::stoul(value));
        else if (shortKey == "updateRate") g.updateRate = std::stoi(value);
        else if (shortKey == "ct") g.ct = std::stoi(value);
    } catch (...) {}
}

// Apply one short key to an Output (switch) object.
static void ApplyOutputKey(Output& o, const std::string& shortKey, const std::string& value) {
    o.originalKeys.insert(shortKey);
    try {
        if (shortKey == "text") o.text = DecodeHolleyEscapes(value);
        else if (shortKey == "pos") ParsePosition(value, o.x, o.y);
        else if (shortKey == "channel") o.channel = std::stoi(value);
        else if (shortKey == "style") o.style = std::stoi(value);
        else if (shortKey == "inactiveColor") o.inactiveColor = static_cast<unsigned>(std::stoul(value));
        else if (shortKey == "activeColor") o.activeColor = static_cast<unsigned>(std::stoul(value));
        else if (shortKey == "background") o.background = static_cast<unsigned>(std::stoul(value));
        else if (shortKey == "fontSize") o.fontSize = std::stoi(value);
    } catch (...) {}
}

// Apply one short key to a free-floating Label.
static void ApplyLabelKey(Label& lab, const std::string& shortKey, const std::string& value) {
    // Editor-legacy. Caption key is label. Parse so old files load; do not round-trip.
    if (shortKey == "text") {
        try { lab.text = DecodeHolleyEscapes(value); } catch (...) {}
        lab.originalKeys.insert("label");
        return;
    }
    lab.originalKeys.insert(shortKey);
    try {
        if (shortKey == "label") lab.text = DecodeHolleyEscapes(value);
        else if (shortKey == "pos") ParsePosition(value, lab.x, lab.y);
        else if (shortKey == "fontSize") lab.fontSize = std::stoi(value);
        else if (shortKey == "foreground") lab.foreground = static_cast<unsigned>(std::stoul(value));
        else if (shortKey == "background") lab.background = static_cast<unsigned>(std::stoul(value));
    } catch (...) {}
}

// True for "[Layout]" or multi-screen "[Layout0]", "[Layout1]", …
static bool IsLayoutSection(const std::string& name) {
    if (name == "Layout") return true;
    if (name.rfind("Layout", 0) == 0 && name.size() > 6) {
        return std::isdigit(static_cast<unsigned char>(name[6]));
    }
    return false;
}

// Full file load. Holley object indices are 1-based in the INI.
bool IniLayoutReader::Load(const std::string& path, LayoutDocument& doc) {
    doc.Clear();
    doc.sourcePath = path;
    doc.ApplyProfileFromSourcePath(path);

    std::ifstream file(path);
    if (!file.is_open()) {
        Log("ERROR: Could not open file");
        return false;
    }

    Log("=== Loading layout ===");
    Log(std::string("Profile: ") + doc.profile.displayName);

    std::string line;
    std::string section;              // Current [Section] name
    LayoutScreen* curScreen = nullptr; // Screen receiving analogMeters/outputs/labels
    // Index → object while parsing one section (Holley indices are 1-based)
    std::map<int, Gauge> gaugeMap;
    std::map<int, Output> outputMap;
    std::map<int, Label> labelMap;

    // Move maps into the current screen's vectors (ordered by index).
    // Keep INI size= as stored. Paint/hit use EffectiveGaugeSize.
    auto flushScreen = [&]() {
        if (!curScreen) return;
        curScreen->gauges.clear();
        curScreen->outputs.clear();
        curScreen->labels.clear();
        for (auto& kv : gaugeMap) {
            curScreen->gauges.push_back(std::move(kv.second));
        }
        for (auto& kv : outputMap) curScreen->outputs.push_back(std::move(kv.second));
        for (auto& kv : labelMap) curScreen->labels.push_back(std::move(kv.second));
        gaugeMap.clear();
        outputMap.clear();
        labelMap.clear();
    };

    // Start a new LayoutScreen after flushing any previous one.
    auto beginScreen = [&](const std::string& name) {
        flushScreen();
        doc.screens.push_back(LayoutScreen{});
        curScreen = &doc.screens.back();
        curScreen->sectionName = name;
    };

    while (std::getline(file, line)) {
        line = Trim(line);
        if (line.empty() || line[0] == ';' || line[0] == '#') continue;

        if (line.front() == '[' && line.back() == ']') {
            section = line.substr(1, line.size() - 2);
            if (section == "Defaults") {
                doc.defaults.present = true;
                curScreen = nullptr;
            } else if (IsLayoutSection(section)) {
                beginScreen(section);
            } else {
                // Plugins and unknown sections ignored
                curScreen = nullptr;
            }
            continue;
        }

        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = Trim(line.substr(0, eq));
        // Wrapping "..." is INI syntax. Inner spaces are padding, not glyphs.
        std::string value = UnquoteIniValue(Trim(line.substr(eq + 1)));

        if (section == "Defaults") {
            try {
                if (key == "brightness") doc.defaults.brightness = std::stoi(value);
                else if (key == "auto-brightness") doc.defaults.autoBrightness = ParseBool(value);
                else if (key == "Screen") doc.defaults.screen = std::stoi(value);
                else if (key == "clocksetbygps") doc.defaults.clockSetByGps = ParseBool(value);
                else if (key == "hdllog") doc.defaults.hdlLog = ParseBool(value);
                else if (key == "autoHomeScreen") doc.defaults.autoHomeScreen = ParseBool(value);
                else if (key == "om") doc.defaults.om = std::stoi(value);
                else if (key == "numScreens") doc.defaults.numScreens = std::stoi(value);
                else continue;
                doc.defaults.originalKeys.insert(key);
            } catch (...) {}
            continue;
        }

        if (!curScreen) {
            // Bare single-screen file may omit [Layout] on first lines — tolerate Background before section
            if (key == "Background" || key == "v") {
                if (doc.screens.empty()) beginScreen("Layout");
                else curScreen = &doc.screens.back();
            } else {
                continue;
            }
        }
        if (!curScreen) continue;

        // Ignore transswitchpos entirely (parked)
        if (key.rfind("transswitchpos", 0) == 0) continue;

        if (key == "Background") {
            curScreen->backgroundPath = value;
            curScreen->backgroundFileName = FileNameFromPath(value);
            Log("Background: " + curScreen->backgroundFileName);
            continue;
        }
        if (key == "v") {
            try { curScreen->version = std::stoi(value); } catch (...) {}
            continue;
        }

        // Collection size lines
        if (key == "analogMeters\\size" || key == "outputs\\size" || key == "labels\\size")
            continue;

        auto parseIndexed = [&](const char* prefix) -> std::pair<int, std::string> {
            // prefix\N\shortKey
            std::string p = prefix;
            if (key.rfind(p + "\\", 0) != 0) return { -1, {} };
            size_t first = p.size() + 1;
            size_t second = key.find('\\', first);
            if (second == std::string::npos) return { -1, {} };
            std::string idxStr = key.substr(first, second - first);
            if (idxStr.empty() || !std::isdigit(static_cast<unsigned char>(idxStr[0])))
                return { -1, {} };
            int idx = std::stoi(idxStr);
            std::string shortKey = key.substr(second + 1);
            return { idx, shortKey };
        };

        {
            auto [idx, sk] = parseIndexed("analogMeters");
            if (idx > 0) {
                ApplyGaugeKey(gaugeMap[idx], sk, value);
                if (sk == "label") Log("Gauge " + std::to_string(idx) + ": " + value);
                continue;
            }
        }
        {
            auto [idx, sk] = parseIndexed("outputs");
            if (idx > 0) {
                ApplyOutputKey(outputMap[idx], sk, value);
                if (sk == "text") Log("Output " + std::to_string(idx) + ": " + value);
                continue;
            }
        }
        {
            auto [idx, sk] = parseIndexed("labels");
            if (idx > 0) {
                ApplyLabelKey(labelMap[idx], sk, value);
                if (sk == "label" || sk == "text")
                    Log("Label " + std::to_string(idx) + ": " + value);
                continue;
            }
        }
    }

    flushScreen();

    if (doc.screens.empty()) {
        Log("ERROR: No layout section found");
        return false;
    }

    // Active screen from Defaults.Screen when multi-screen
    if (doc.defaults.present) {
        int want = doc.defaults.screen;
        // Prefer LayoutN matching Screen index
        for (size_t i = 0; i < doc.screens.size(); ++i) {
            const auto& name = doc.screens[i].sectionName;
            if (name == "Layout" + std::to_string(want) ||
                (want == 0 && name == "Layout0")) {
                doc.activeScreenIndex = static_cast<int>(i);
                break;
            }
        }
    } else {
        doc.activeScreenIndex = 0;
    }

    auto* active = doc.ActiveScreen();
    Log("=== Parsing Complete ===");
    Log("Screens: " + std::to_string(doc.screens.size()) +
        "   Active: " + (active ? active->sectionName : "?"));
    if (active) {
        Log("Gauges: " + std::to_string(active->gauges.size()) +
            "   Outputs: " + std::to_string(active->outputs.size()) +
            "   Labels: " + std::to_string(active->labels.size()));
    }
    return true;
}
