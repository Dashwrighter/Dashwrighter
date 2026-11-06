#include "ChannelCatalog.h"
#include "resource.h"
#include <windows.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace {

std::wstring Utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    if (n <= 0) return {};
    std::wstring out(static_cast<size_t>(n), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &out[0], n);
    if (!out.empty() && out.back() == L'\0') out.pop_back();
    return out;
}

bool FileExists(const std::wstring& path) {
    DWORD attr = GetFileAttributesW(path.c_str());
    return attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY);
}

std::wstring FindPickerCsv() {
    std::vector<std::wstring> roots;
    wchar_t modulePath[MAX_PATH] = {};
    if (GetModuleFileNameW(nullptr, modulePath, MAX_PATH) > 0) {
        std::wstring dir(modulePath);
        size_t slash = dir.find_last_of(L"\\/");
        if (slash != std::wstring::npos)
            dir.resize(slash);
        roots.push_back(dir);
        roots.push_back(dir + L"\\..");
        roots.push_back(dir + L"\\..\\..");
        roots.push_back(dir + L"\\..\\..\\..");
    }
    roots.push_back(L".");
    roots.push_back(L"..");
    roots.push_back(L"..\\..");

    for (const auto& root : roots) {
        const std::wstring paths[] = {
            root + L"\\Holley-Channel-Picker.csv",
            root + L"\\private\\Holley-Channel-Picker.csv",
        };
        for (const auto& path : paths) {
            if (FileExists(path))
                return path;
        }
    }
    return {};
}

// IDR_CHANNEL_PICKER_CSV. Empty when the resource is missing.
bool LoadEmbeddedPickerCsv(std::string& out) {
    out.clear();
    HMODULE hMod = GetModuleHandleW(nullptr);
    HRSRC hRes = FindResourceW(hMod, MAKEINTRESOURCEW(IDR_CHANNEL_PICKER_CSV), RT_RCDATA);
    if (!hRes)
        return false;
    HGLOBAL hData = LoadResource(hMod, hRes);
    if (!hData)
        return false;
    const void* p = LockResource(hData);
    const DWORD sz = SizeofResource(hMod, hRes);
    if (!p || sz == 0)
        return false;
    out.assign(static_cast<const char*>(p), static_cast<const char*>(p) + sz);
    return true;
}

bool NextCsvField(const char*& p, std::string& out) {
    out.clear();
    if (!p || !*p || *p == '\n' || *p == '\r')
        return false;
    if (*p == '"') {
        ++p;
        while (*p && *p != '\n' && *p != '\r') {
            if (*p == '"') {
                ++p;
                if (*p == '"') {
                    out.push_back('"');
                    ++p;
                    continue;
                }
                break;
            }
            out.push_back(*p++);
        }
        if (*p == ',')
            ++p;
        return true;
    }
    while (*p && *p != ',' && *p != '\n' && *p != '\r')
        out.push_back(*p++);
    if (*p == ',')
        ++p;
    return true;
}

} // namespace

ChannelCatalog::ChannelCatalog() {
    std::wstring path = FindPickerCsv();
    if (!path.empty() && LoadFile(path))
        return;
    std::string text;
    if (LoadEmbeddedPickerCsv(text) && ParseText(text))
        sourcePath_ = L"embedded";
}

const ChannelCatalog& ChannelCatalog::Get() {
    static ChannelCatalog inst;
    return inst;
}

const ChannelEntry* ChannelCatalog::FindById(int id) const {
    for (const auto& e : entries_) {
        if (e.id == id)
            return &e;
    }
    return nullptr;
}

bool ChannelCatalog::LoadFile(const std::wstring& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in)
        return false;
    std::stringstream buf;
    buf << in.rdbuf();
    std::string text = buf.str();
    if (!ParseText(text))
        return false;
    sourcePath_ = path;
    return true;
}

bool ChannelCatalog::ParseText(const std::string& raw) {
    std::string text = raw;
    if (text.size() >= 3 &&
        static_cast<unsigned char>(text[0]) == 0xEF &&
        static_cast<unsigned char>(text[1]) == 0xBB &&
        static_cast<unsigned char>(text[2]) == 0xBF) {
        text.erase(0, 3);
    }

    entries_.clear();
    loaded_ = false;
    sourcePath_.clear();

    const char* p = text.c_str();
    int idCol = 0;
    int nameCol = 1;
    bool sawHeader = false;
    while (*p) {
        if (*p == '\r') { ++p; continue; }
        if (*p == '\n') { ++p; continue; }

        std::vector<std::string> fields;
        while (*p && *p != '\n' && *p != '\r') {
            std::string field;
            if (!NextCsvField(p, field))
                break;
            fields.push_back(std::move(field));
        }
        if (*p == '\r') ++p;
        if (*p == '\n') ++p;
        if (fields.empty())
            continue;

        if (!sawHeader && fields[0] == "channel") {
            sawHeader = true;
            for (int i = 0; i < static_cast<int>(fields.size()); ++i) {
                if (fields[static_cast<size_t>(i)] == "channel")
                    idCol = i;
                else if (fields[static_cast<size_t>(i)] == "name")
                    nameCol = i;
            }
            continue;
        }

        if (idCol < 0 || nameCol < 0
            || idCol >= static_cast<int>(fields.size())
            || nameCol >= static_cast<int>(fields.size()))
            continue;

        int id = 0;
        try {
            id = std::stoi(fields[static_cast<size_t>(idCol)]);
        } catch (...) {
            continue;
        }
        const std::string& name = fields[static_cast<size_t>(nameCol)];
        if (id <= 0 || name.empty())
            continue;

        ChannelEntry e;
        e.id = id;
        e.name = Utf8ToWide(name);
        entries_.push_back(std::move(e));
    }

    loaded_ = !entries_.empty();
    return loaded_;
}
