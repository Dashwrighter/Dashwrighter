// =============================================================================
// PreviewFont.cpp — Roboto 1.200310 (2013) for all non-symbol preview text
// =============================================================================

#include "PreviewFont.h"
#include "resource.h"

#include <string>
#include <vector>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "gdi32.lib")

namespace {

std::vector<BYTE> gBytes;
HANDLE gMemFont = nullptr;
Gdiplus::PrivateFontCollection* gColl = nullptr;
Gdiplus::FontFamily* gFamily = nullptr;

bool ReadFileBytes(const wchar_t* path, std::vector<BYTE>& out) {
    HANDLE h = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE)
        return false;
    LARGE_INTEGER sz = {};
    if (!GetFileSizeEx(h, &sz) || sz.QuadPart <= 0 || sz.QuadPart > 8 * 1024 * 1024) {
        CloseHandle(h);
        return false;
    }
    out.resize(static_cast<size_t>(sz.QuadPart));
    DWORD n = 0;
    const BOOL ok = ReadFile(h, out.data(), static_cast<DWORD>(out.size()), &n, nullptr);
    CloseHandle(h);
    if (!ok || n != out.size()) {
        out.clear();
        return false;
    }
    return true;
}

bool LoadLooseTtf(std::vector<BYTE>& out) {
    std::vector<std::wstring> roots;
    wchar_t modulePath[MAX_PATH] = {};
    if (GetModuleFileNameW(nullptr, modulePath, MAX_PATH) > 0) {
        std::wstring dir(modulePath);
        const size_t slash = dir.find_last_of(L"\\/");
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
        const std::wstring path = root + L"\\Roboto-Regular-2013.ttf";
        const DWORD attr = GetFileAttributesW(path.c_str());
        if (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
            if (ReadFileBytes(path.c_str(), out))
                return true;
        }
    }
    return false;
}

bool LoadEmbeddedTtf(std::vector<BYTE>& out) {
    HMODULE hMod = GetModuleHandleW(nullptr);
    HRSRC hRes = FindResourceW(hMod, MAKEINTRESOURCEW(IDR_ROBOTO_REGULAR_2013), RT_RCDATA);
    if (!hRes) return false;
    HGLOBAL hData = LoadResource(hMod, hRes);
    if (!hData) return false;
    const void* p = LockResource(hData);
    const DWORD sz = SizeofResource(hMod, hRes);
    if (!p || sz == 0) return false;
    const auto* b = static_cast<const BYTE*>(p);
    out.assign(b, b + sz);
    return true;
}

} // namespace

namespace PreviewFont {

void EnsureLoaded() {
    if (gFamily)
        return;

    if (!LoadLooseTtf(gBytes))
        LoadEmbeddedTtf(gBytes);

    if (!gBytes.empty()) {
        gColl = new Gdiplus::PrivateFontCollection();
        if (gColl->AddMemoryFont(gBytes.data(), static_cast<INT>(gBytes.size())) == Gdiplus::Ok) {
            const INT n = gColl->GetFamilyCount();
            if (n > 0) {
                std::vector<Gdiplus::FontFamily> fams(static_cast<size_t>(n));
                INT found = 0;
                gColl->GetFamilies(n, fams.data(), &found);
                if (found > 0) {
                    WCHAR name[LF_FACESIZE] = {};
                    fams[0].GetFamilyName(name);
                    gFamily = new Gdiplus::FontFamily(name, gColl);
                    if (gFamily->GetLastStatus() != Gdiplus::Ok) {
                        delete gFamily;
                        gFamily = nullptr;
                    }
                }
            }
        }
        DWORD dummy = 0;
        gMemFont = AddFontMemResourceEx(gBytes.data(), static_cast<DWORD>(gBytes.size()),
            0, &dummy);
    }

    if (!gFamily)
        gFamily = new Gdiplus::FontFamily(L"Arial");
}

void Shutdown() {
    delete gFamily;
    gFamily = nullptr;
    delete gColl;
    gColl = nullptr;
    if (gMemFont) {
        RemoveFontMemResourceEx(gMemFont);
        gMemFont = nullptr;
    }
    gBytes.clear();
}

const Gdiplus::FontFamily& Family() {
    EnsureLoaded();
    return *gFamily;
}

HFONT CreateGdi(int px, int weight) {
    EnsureLoaded();
    px = ClampPx(px);
    return CreateFontW(-px, 0, 0, 0, weight, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, FaceW);
}

}
