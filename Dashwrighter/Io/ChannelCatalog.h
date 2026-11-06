#pragma once

#include <string>
#include <vector>

// Channel names from Holley-Channel-Picker.csv (not IO/Inputs or
// IO/Outputs). A loose CSV wins. The same channel,name table is compiled
// in and used when no file is found. Unknown INI channel= values are not
// in this list.
// Parked: optional sidecar .DAT next to the EXE, merged in when the picker opens.
// Parked: HDL import of io.N names as Local Channels (PENDING-CHANGES 2026-09-13).
struct ChannelEntry {
    int id = 0;
    std::wstring name;
};

class ChannelCatalog {
public:
    static const ChannelCatalog& Get();

    const std::vector<ChannelEntry>& Entries() const { return entries_; }
    bool loaded() const { return loaded_; }
    const std::wstring& sourcePath() const { return sourcePath_; }

    const ChannelEntry* FindById(int id) const;

private:
    ChannelCatalog();
    bool LoadFile(const std::wstring& path);
    bool ParseText(const std::string& text);

    std::vector<ChannelEntry> entries_;
    std::wstring sourcePath_;
    bool loaded_ = false;
};
