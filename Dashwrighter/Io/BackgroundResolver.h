#pragma once

// =============================================================================
// BackgroundResolver.h — Map Holley Background= paths to a local image file
//
// INIs store device paths like /internal/images/DarkPerforated.jpg. On disk the
// file usually lives under DefaultLayouts/internal/images or similar project
// folders. This class tries a list of search roots near the loaded layout.
// =============================================================================

#include <string>
#include <vector>

class BackgroundResolver {
public:
    // Replace the entire ordered list of directories to search.
    void SetSearchRoots(std::vector<std::string> roots);

    // Append one directory (e.g. folder the user picked an image from).
    void AddSearchRoot(const std::string& root);

    // Rebuild roots based on the directory of the loaded .ini (and parents).
    void SetLayoutPath(const std::string& iniPath);

    // Resolve basename or full Background= value to a wide path for GDI+.
    // Returns empty string if not found.
    std::wstring ResolveWide(const std::string& backgroundOrFileName) const;

    // Same as ResolveWide but UTF-8 / narrow path for logging.
    std::string Resolve(const std::string& backgroundOrFileName) const;

    const std::vector<std::string>& Roots() const { return roots_; }

private:
    std::vector<std::string> roots_; // Directories tried in order

    // Strip Background= prefix and path, leaving just the file name.
    static std::string NormalizeFileName(const std::string& backgroundOrFileName);
};
