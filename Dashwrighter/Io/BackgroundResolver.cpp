// =============================================================================
// BackgroundResolver.cpp — Locate background images relative to a layout INI
// =============================================================================

#include "BackgroundResolver.h"
#include "IniUtils.h"
#include <windows.h>
#include <fstream>

void BackgroundResolver::SetSearchRoots(std::vector<std::string> roots) {
    roots_ = std::move(roots);
}

void BackgroundResolver::AddSearchRoot(const std::string& root) {
    if (!root.empty()) roots_.push_back(root);
}

// Search next to the INI first (factory packs use /internal/images/ and
// /Backgrounds/). Parent folders are last-resort if the user keeps a
// DefaultLayouts tree beside their files.
void BackgroundResolver::SetLayoutPath(const std::string& iniPath) {
    roots_.clear();
    std::string dir = IniUtils::DirNameFromPath(iniPath);
    if (!dir.empty()) {
        roots_.push_back(dir);
        roots_.push_back(dir + "Backgrounds\\");
        roots_.push_back(dir + "Backgrounds/");
        roots_.push_back(dir + "bg\\");
        roots_.push_back(dir + "bg/");
        roots_.push_back(dir + "internal\\images\\");
        roots_.push_back(dir + "internal/images/");
        roots_.push_back(dir + "internal\\images\\Basic Backgrounds\\");
        roots_.push_back(dir + "Backgrounds\\Basic Backgrounds\\");

        size_t slash = dir.find_last_of("/\\", dir.size() > 1 ? dir.size() - 2 : std::string::npos);
        if (slash != std::string::npos) {
            std::string parent = dir.substr(0, slash + 1);
            roots_.push_back(parent + "DefaultLayouts\\Backgrounds\\");
            roots_.push_back(parent + "DefaultLayouts\\Backgrounds\\Basic Backgrounds\\");
            roots_.push_back(parent + "DefaultLayouts\\internal\\images\\");
            roots_.push_back(parent + "internal\\images\\");
        }
    }
}

// Accepts "foo.jpg", "/internal/images/foo.jpg", "/Backgrounds/foo.jpg", or "Background=...".
std::string BackgroundResolver::NormalizeFileName(const std::string& backgroundOrFileName) {
    std::string s = IniUtils::Trim(backgroundOrFileName);
    if (s.rfind("Background=", 0) == 0) s = s.substr(11);
    s = IniUtils::Trim(s);
    // Prefer leaf name so search roots can find lab PNGs and stock JPGs.
    return IniUtils::FileNameFromPath(s);
}

// First existing root+filename wins; also tries the input as an absolute path
// and a few relative reconstructions of Holley path prefixes.
std::string BackgroundResolver::Resolve(const std::string& backgroundOrFileName) const {
    std::string file = NormalizeFileName(backgroundOrFileName);
    if (file.empty()) return {};

    auto tryPath = [](const std::string& candidate) -> std::string {
        std::ifstream test(candidate, std::ios::binary);
        if (test.good()) return candidate;
        return {};
    };

    for (const auto& root : roots_) {
        std::string candidate = root;
        if (!candidate.empty()) {
            char last = candidate.back();
            if (last != '\\' && last != '/') candidate += '\\';
        }
        std::string hit = tryPath(candidate + file);
        if (!hit.empty()) return hit;
    }

    // Absolute / verbatim path (Load Background, USB paths)
    {
        std::string hit = tryPath(backgroundOrFileName);
        if (!hit.empty()) return hit;
    }

    // Relative path with Holley-style separators from first root
    if (!roots_.empty()) {
        std::string rel = IniUtils::Trim(backgroundOrFileName);
        if (rel.rfind("Background=", 0) == 0) rel = IniUtils::Trim(rel.substr(11));
        while (!rel.empty() && (rel[0] == '/' || rel[0] == '\\')) rel.erase(0, 1);
        for (char& c : rel) if (c == '/') c = '\\';
        for (const auto& root : roots_) {
            std::string base = root;
            if (!base.empty()) {
                char last = base.back();
                if (last != '\\' && last != '/') base += '\\';
            }
            std::string hit = tryPath(base + rel);
            if (!hit.empty()) return hit;
            // Also try parent of root + rel (e.g. DefaultLayouts + Backgrounds\foo)
            size_t slash = root.find_last_of("/\\", root.size() > 1 ? root.size() - 2 : std::string::npos);
            if (slash != std::string::npos) {
                hit = tryPath(root.substr(0, slash + 1) + rel);
                if (!hit.empty()) return hit;
            }
        }
    }
    return {};
}

// GDI+ Image constructor wants a wide path on Windows.
std::wstring BackgroundResolver::ResolveWide(const std::string& backgroundOrFileName) const {
    std::string narrow = Resolve(backgroundOrFileName);
    if (narrow.empty()) return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, narrow.c_str(), -1, nullptr, 0);
    std::wstring wide(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, narrow.c_str(), -1, &wide[0], n);
    if (!wide.empty() && wide.back() == L'\0') wide.pop_back();
    return wide;
}
