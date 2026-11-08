// =============================================================================
// SymbolPicker.cpp — Holley two-page symbol keyboard (gaugeType 8 Label)
//
// Page 1 / Page 2 via Shift. Glyph art from the locked TX-all_symbols atlas.
// Order is atlas catalog order until keyboard photos are joined to escapes.
// Esc cancels; Clear / Backspace / Space / Ok match the dash keyboard chrome.
// =============================================================================

#include "SymbolPicker.h"
#include "resource.h"
#include "../Render/LayoutRenderer.h"
#include "../Render/SymbolAtlasData.inc"
#include <windows.h>
#include <gdiplus.h>
#include <vector>
#include <string>
#include <algorithm>

#pragma comment(lib, "gdiplus.lib")

namespace {

constexpr int kPage1Count = 45;
constexpr int kCols = 13;
constexpr int kKeyW = 58;
constexpr int kKeyH = 50;
constexpr int kGap = 4;
constexpr int kPad = 12;
constexpr int kChromeH = 36;
constexpr int kPreviewH = 44;

enum ChromeId {
    ChEsc = 1,
    ChClear,
    ChShift,
    ChBksp,
    ChSpace,
    ChOk
};

struct PickerState {
    HWND hwnd = nullptr;
    int page = 0;          // 0 = page 1, 1 = page 2 (Shift)
    int hoverAtlas = -1;
    bool accepted = false;
    std::vector<unsigned> picked; // 0 = space
    RECT preview{};
    RECT nameBar{};
    RECT chrome[7]{};
    RECT glyph[96]{};
    int glyphCount = 0;
    int glyphAtlas[96]{};
};

int PageStart(int page) {
    return page == 0 ? 0 : kPage1Count;
}
int PageCount(int page) {
    if (page == 0) return (std::min)(kPage1Count, kSymbolAtlasCount);
    return (std::max)(0, kSymbolAtlasCount - kPage1Count);
}

void LayoutKeys(PickerState& st, int cw) {
    const int gridX = kPad + 72;
    const int gridY = kPad + kPreviewH + kGap + 8;
    st.glyphCount = PageCount(st.page);
    const int start = PageStart(st.page);
    for (int i = 0; i < st.glyphCount; ++i) {
        const int r = i / kCols;
        const int c = i % kCols;
        RECT rc;
        rc.left = gridX + c * (kKeyW + kGap);
        rc.top = gridY + r * (kKeyH + kGap);
        rc.right = rc.left + kKeyW;
        rc.bottom = rc.top + kKeyH;
        st.glyph[i] = rc;
        st.glyphAtlas[i] = start + i;
    }

    const int bottom = gridY + 4 * (kKeyH + kGap) + 8;
    st.chrome[ChEsc] = { kPad, kPad, kPad + 64, kPad + kPreviewH };
    st.chrome[ChClear] = { cw - kPad - 72, kPad, cw - kPad, kPad + kPreviewH };
    st.preview = { kPad + 68, kPad, cw - kPad - 76, kPad + kPreviewH };
    st.chrome[ChShift] = { kPad, bottom, kPad + 80, bottom + kChromeH };
    st.chrome[ChBksp] = { kPad + 84, bottom, kPad + 148, bottom + kChromeH };
    st.chrome[ChSpace] = { kPad + 152, bottom, cw - kPad - 88, bottom + kChromeH };
    st.chrome[ChOk] = { cw - kPad - 80, bottom, cw - kPad, bottom + kChromeH };
    st.nameBar = { kPad, bottom + kChromeH + 6, cw - kPad, bottom + kChromeH + 24 };
}

void BlitGlyph(Gdiplus::Graphics& gfx, Gdiplus::Image* atlas, const SymbolAtlasEntry& e,
    int dx, int dy, int dw, int dh) {
    if (!atlas || e.w < 1 || e.h < 1 || dw < 1 || dh < 1) return;
    // Floor 50 knocks out the dark atlas background. 170 is the ink
    // level that becomes opaque. Keys are drawn RGB 232.
    Gdiplus::Bitmap srcBmp(e.w, e.h, PixelFormat32bppARGB);
    {
        Gdiplus::Graphics g(&srcBmp);
        g.SetCompositingMode(Gdiplus::CompositingModeSourceCopy);
        g.DrawImage(atlas, Gdiplus::Rect(0, 0, e.w, e.h),
            e.x, e.y, e.w, e.h, Gdiplus::UnitPixel);
    }
    Gdiplus::BitmapData data = {};
    Gdiplus::Rect lock(0, 0, e.w, e.h);
    if (srcBmp.LockBits(&lock, Gdiplus::ImageLockModeRead | Gdiplus::ImageLockModeWrite,
            PixelFormat32bppARGB, &data) != Gdiplus::Ok)
        return;
    auto* scan0 = static_cast<BYTE*>(data.Scan0);
    constexpr int kDarkFloor = 50;
    constexpr int kBrightRef = 170;
    for (int y = 0; y < e.h; ++y) {
        auto* row = scan0 + y * data.Stride;
        for (int x = 0; x < e.w; ++x) {
            BYTE* p = row + x * 4;
            const int lum = static_cast<int>(0.299 * p[2] + 0.587 * p[1] + 0.114 * p[0]);
            if (lum <= kDarkFloor) {
                p[0] = p[1] = p[2] = p[3] = 0;
                continue;
            }
            int a = (lum - kDarkFloor) * 255 / (kBrightRef - kDarkFloor);
            if (a < 0) a = 0;
            if (a > 255) a = 255;
            p[0] = p[1] = p[2] = 232;
            p[3] = static_cast<BYTE>(a);
        }
    }
    srcBmp.UnlockBits(&data);
    gfx.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBilinear);
    gfx.DrawImage(&srcBmp, Gdiplus::Rect(dx, dy, dw, dh));
}

void DrawChromeLabel(Gdiplus::Graphics& gfx, const RECT& rc, const wchar_t* text, bool accent) {
    Gdiplus::Color fill = accent
        ? Gdiplus::Color(255, 227, 28, 35)
        : Gdiplus::Color(255, 36, 36, 36);
    Gdiplus::SolidBrush br(fill);
    Gdiplus::SolidBrush tb(Gdiplus::Color(255, 255, 255, 255));
    Gdiplus::Pen pen(Gdiplus::Color(255, 70, 70, 70), 1);
    Gdiplus::RectF box(
        static_cast<Gdiplus::REAL>(rc.left),
        static_cast<Gdiplus::REAL>(rc.top),
        static_cast<Gdiplus::REAL>(rc.right - rc.left),
        static_cast<Gdiplus::REAL>(rc.bottom - rc.top));
    gfx.FillRectangle(&br, box);
    gfx.DrawRectangle(&pen, box);
    Gdiplus::FontFamily fam(L"Segoe UI");
    Gdiplus::Font font(&fam, 12.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
    Gdiplus::StringFormat fmt;
    fmt.SetAlignment(Gdiplus::StringAlignmentCenter);
    fmt.SetLineAlignment(Gdiplus::StringAlignmentCenter);
    gfx.DrawString(text, -1, &font, box, &fmt, &tb);
}

void PaintPicker(HWND hwnd, PickerState& st) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    RECT client{};
    GetClientRect(hwnd, &client);
    Gdiplus::Graphics gfx(hdc);
    gfx.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    Gdiplus::SolidBrush bg(Gdiplus::Color(255, 14, 14, 14));
    gfx.FillRectangle(&bg, 0, 0, client.right, client.bottom);

    DrawChromeLabel(gfx, st.chrome[ChEsc], L"Esc", false);
    DrawChromeLabel(gfx, st.chrome[ChClear], L"Clear", false);
    DrawChromeLabel(gfx, st.chrome[ChShift], L"Shift", st.page == 1);
    DrawChromeLabel(gfx, st.chrome[ChBksp], L"\u2190", false);
    DrawChromeLabel(gfx, st.chrome[ChSpace], L"Space", false);
    DrawChromeLabel(gfx, st.chrome[ChOk], L"Ok", true);

    Gdiplus::SolidBrush well(Gdiplus::Color(255, 10, 10, 10));
    Gdiplus::Pen wellPen(Gdiplus::Color(255, 70, 70, 70), 1);
    Gdiplus::RectF prev(
        static_cast<Gdiplus::REAL>(st.preview.left),
        static_cast<Gdiplus::REAL>(st.preview.top),
        static_cast<Gdiplus::REAL>(st.preview.right - st.preview.left),
        static_cast<Gdiplus::REAL>(st.preview.bottom - st.preview.top));
    gfx.FillRectangle(&well, prev);
    gfx.DrawRectangle(&wellPen, prev);

    Gdiplus::Image* atlas = LayoutRenderer::GetSymbolAtlasImage();
    int px = st.preview.left + 8;
    const int py = st.preview.top + 6;
    const int ph = (st.preview.bottom - st.preview.top) - 12;
    Gdiplus::FontFamily asciiFamily(L"Segoe UI");
    Gdiplus::Font asciiFont(&asciiFamily, static_cast<Gdiplus::REAL>((std::max)(12, ph - 8)),
        Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
    Gdiplus::SolidBrush asciiBrush(Gdiplus::Color(255, 220, 220, 220));
    gfx.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);
    for (unsigned cp : st.picked) {
        if (cp == 0) {
            px += (std::max)(8, ph / 3);
            continue;
        }
        const SymbolAtlasEntry* e = nullptr;
        for (int i = 0; i < kSymbolAtlasCount; ++i) {
            if (kSymbolAtlas[i].codepoint == cp) { e = &kSymbolAtlas[i]; break; }
        }
        if (e) {
            int dw = e->w, dh = e->h;
            if (dh > ph && dh > 0) {
                dw = dw * ph / dh;
                dh = ph;
            }
            BlitGlyph(gfx, atlas, *e, px, py + (ph - dh) / 2, dw, dh);
            px += dw + 4;
        } else {
            wchar_t s[2] = {};
            s[0] = static_cast<wchar_t>(cp);
            Gdiplus::RectF layout(static_cast<Gdiplus::REAL>(px),
                static_cast<Gdiplus::REAL>(py),
                64.0f, static_cast<Gdiplus::REAL>(ph));
            Gdiplus::RectF bounds;
            gfx.MeasureString(s, -1, &asciiFont, layout, &bounds);
            gfx.DrawString(s, -1, &asciiFont, layout, nullptr, &asciiBrush);
            px += (std::max)(8, static_cast<int>(bounds.Width + 1.0f));
        }
        if (px > st.preview.right - 8) break;
    }

    Gdiplus::SolidBrush keyFill(Gdiplus::Color(255, 42, 42, 42));
    Gdiplus::SolidBrush keyHover(Gdiplus::Color(255, 60, 28, 30));
    Gdiplus::Pen keyPen(Gdiplus::Color(255, 70, 70, 70), 1);
    for (int i = 0; i < st.glyphCount; ++i) {
        const RECT& rc = st.glyph[i];
        const int ai = st.glyphAtlas[i];
        if (ai < 0 || ai >= kSymbolAtlasCount) continue;
        Gdiplus::RectF box(
            static_cast<Gdiplus::REAL>(rc.left),
            static_cast<Gdiplus::REAL>(rc.top),
            static_cast<Gdiplus::REAL>(rc.right - rc.left),
            static_cast<Gdiplus::REAL>(rc.bottom - rc.top));
        gfx.FillRectangle(ai == st.hoverAtlas ? &keyHover : &keyFill, box);
        gfx.DrawRectangle(&keyPen, box);
        const SymbolAtlasEntry& e = kSymbolAtlas[ai];
        const int kw = rc.right - rc.left - 8;
        const int kh = rc.bottom - rc.top - 8;
        int dw = e.w, dh = e.h;
        if (dw > kw && dw > 0) { dh = dh * kw / dw; dw = kw; }
        if (dh > kh && dh > 0) { dw = dw * kh / dh; dh = kh; }
        const int gx = rc.left + (rc.right - rc.left - dw) / 2;
        const int gy = rc.top + (rc.bottom - rc.top - dh) / 2;
        BlitGlyph(gfx, atlas, e, gx, gy, dw, dh);
    }

    const wchar_t* name = L"";
    wchar_t nameBuf[128] = {};
    if (st.hoverAtlas >= 0 && st.hoverAtlas < kSymbolAtlasCount) {
        const char* n = kSymbolAtlas[st.hoverAtlas].name;
        MultiByteToWideChar(CP_UTF8, 0, n ? n : "", -1, nameBuf, 128);
        name = nameBuf;
    }
    Gdiplus::FontFamily fam(L"Segoe UI");
    Gdiplus::Font font(&fam, 12.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
    Gdiplus::SolidBrush nb(Gdiplus::Color(255, 180, 180, 180));
    Gdiplus::RectF nbx(
        static_cast<Gdiplus::REAL>(st.nameBar.left),
        static_cast<Gdiplus::REAL>(st.nameBar.top),
        static_cast<Gdiplus::REAL>(st.nameBar.right - st.nameBar.left),
        static_cast<Gdiplus::REAL>(st.nameBar.bottom - st.nameBar.top));
    gfx.DrawString(name, -1, &font, Gdiplus::PointF(nbx.X, nbx.Y), &nb);

    EndPaint(hwnd, &ps);
}

int HitChrome(const PickerState& st, int x, int y) {
    POINT p{ x, y };
    for (int id = ChEsc; id <= ChOk; ++id) {
        if (PtInRect(&st.chrome[id], p)) return id;
    }
    return 0;
}
int HitGlyph(const PickerState& st, int x, int y) {
    POINT p{ x, y };
    for (int i = 0; i < st.glyphCount; ++i) {
        if (PtInRect(&st.glyph[i], p)) return st.glyphAtlas[i];
    }
    return -1;
}

bool IsPuaCp(unsigned cp) {
    return cp >= 0xE000 && cp <= 0xF8FF;
}

void AppendUtf8Cp(std::string& utf8, unsigned cp) {
    if (cp < 0x80) {
        utf8.push_back(static_cast<char>(cp));
    } else if (cp < 0x800) {
        utf8.push_back(static_cast<char>(0xC0 | (cp >> 6)));
        utf8.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else {
        utf8.push_back(static_cast<char>(0xE0 | (cp >> 12)));
        utf8.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        utf8.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
}

void Utf8FromPicked(const std::vector<unsigned>& picked, std::string& utf8) {
    utf8.clear();
    unsigned prev = 0xFFFFFFFFu;
    for (unsigned cp : picked) {
        if (cp == 0) {
            utf8.push_back(' ');
            prev = 0;
            continue;
        }
        // Dash puts a space between atlas glyphs; keep ASCII runs intact.
        if (prev != 0xFFFFFFFFu && prev != 0 && (IsPuaCp(prev) || IsPuaCp(cp)))
            utf8.push_back(' ');
        AppendUtf8Cp(utf8, cp);
        prev = cp;
    }
}

void PickedFromUtf8(const std::string& utf8, std::vector<unsigned>& picked) {
    picked.clear();
    for (size_t i = 0; i < utf8.size(); ) {
        const unsigned char c = static_cast<unsigned char>(utf8[i]);
        unsigned cp = 0;
        size_t n = 1;
        if (c < 0x80) {
            cp = c;
        } else if ((c & 0xE0) == 0xC0 && i + 1 < utf8.size()) {
            cp = ((c & 0x1F) << 6) | (utf8[i + 1] & 0x3F);
            n = 2;
        } else if ((c & 0xF0) == 0xE0 && i + 2 < utf8.size()) {
            cp = ((c & 0x0F) << 12) | ((utf8[i + 1] & 0x3F) << 6) | (utf8[i + 2] & 0x3F);
            n = 3;
        } else {
            ++i;
            continue;
        }
        i += n;
        if (cp == ' ')
            picked.push_back(0);
        else
            picked.push_back(cp);
    }
}

LRESULT CALLBACK PickerProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_CREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        auto* st = reinterpret_cast<PickerState*>(cs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(st));
        st->hwnd = hwnd;
        RECT rc{};
        GetClientRect(hwnd, &rc);
        LayoutKeys(*st, rc.right);
        return 0;
    }
    auto* st = reinterpret_cast<PickerState*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (!st) return DefWindowProc(hwnd, msg, wParam, lParam);

    switch (msg) {
    case WM_PAINT:
        PaintPicker(hwnd, *st);
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_MOUSEMOVE: {
        int x = static_cast<short>(LOWORD(lParam));
        int y = static_cast<short>(HIWORD(lParam));
        int hov = HitGlyph(*st, x, y);
        if (hov != st->hoverAtlas) {
            st->hoverAtlas = hov;
            InvalidateRect(hwnd, &st->nameBar, FALSE);
            for (int i = 0; i < st->glyphCount; ++i)
                InvalidateRect(hwnd, &st->glyph[i], FALSE);
        }
        return 0;
    }
    case WM_LBUTTONDOWN: {
        int x = static_cast<short>(LOWORD(lParam));
        int y = static_cast<short>(HIWORD(lParam));
        int ch = HitChrome(*st, x, y);
        if (ch == ChEsc) {
            st->accepted = false;
            DestroyWindow(hwnd);
            return 0;
        }
        if (ch == ChOk) {
            st->accepted = true;
            DestroyWindow(hwnd);
            return 0;
        }
        if (ch == ChClear) {
            st->picked.clear();
            InvalidateRect(hwnd, &st->preview, FALSE);
            return 0;
        }
        if (ch == ChBksp) {
            if (!st->picked.empty()) st->picked.pop_back();
            InvalidateRect(hwnd, &st->preview, FALSE);
            return 0;
        }
        if (ch == ChSpace) {
            st->picked.push_back(0);
            InvalidateRect(hwnd, &st->preview, FALSE);
            return 0;
        }
        if (ch == ChShift) {
            st->page = st->page ? 0 : 1;
            RECT rc{};
            GetClientRect(hwnd, &rc);
            LayoutKeys(*st, rc.right);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        int gi = HitGlyph(*st, x, y);
        if (gi >= 0 && gi < kSymbolAtlasCount) {
            st->picked.push_back(kSymbolAtlas[gi].codepoint);
            InvalidateRect(hwnd, &st->preview, FALSE);
        }
        return 0;
    }
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            st->accepted = false;
            DestroyWindow(hwnd);
            return 0;
        }
        if (wParam == VK_RETURN) {
            st->accepted = true;
            DestroyWindow(hwnd);
            return 0;
        }
        if (wParam == VK_BACK) {
            if (!st->picked.empty()) st->picked.pop_back();
            InvalidateRect(hwnd, &st->preview, FALSE);
            return 0;
        }
        if (wParam == VK_SHIFT) {
            st->page = st->page ? 0 : 1;
            RECT rc{};
            GetClientRect(hwnd, &rc);
            LayoutKeys(*st, rc.right);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        return 0;
    case WM_CLOSE:
        st->accepted = false;
        DestroyWindow(hwnd);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

} // namespace

bool RunSymbolPicker(HWND owner, std::string& utf8Label) {
    PickerState st;
    PickedFromUtf8(utf8Label, st.picked);

    static bool registered = false;
    if (!registered) {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = PickerProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.lpszClassName = L"HolleySymbolPicker";
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
        ApplyAppIcon(wc);
        RegisterClassExW(&wc);
        registered = true;
    }

    const int cw = kPad + 72 + kCols * (kKeyW + kGap) + kPad;
    const int ch = kPad + kPreviewH + 8 + 4 * (kKeyH + kGap) + 8 + kChromeH + 32;
    RECT wr = { 0, 0, cw, ch };
    AdjustWindowRect(&wr, WS_CAPTION | WS_SYSMENU | WS_POPUP, FALSE);

    RECT orc{};
    GetWindowRect(owner, &orc);
    const int ww = wr.right - wr.left;
    const int wh = wr.bottom - wr.top;
    HWND hwnd = CreateWindowExW(WS_EX_DLGMODALFRAME, L"HolleySymbolPicker", L"Symbol",
        WS_CAPTION | WS_SYSMENU | WS_POPUP,
        orc.left + ((orc.right - orc.left) - ww) / 2,
        orc.top + ((orc.bottom - orc.top) - wh) / 2,
        ww, wh, owner, nullptr, GetModuleHandle(nullptr), &st);
    if (!hwnd) return false;

    EnableWindow(owner, FALSE);
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    MSG msg = {};
    while (IsWindow(hwnd) && GetMessage(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    EnableWindow(owner, TRUE);
    SetForegroundWindow(owner);
    if (!st.accepted) return false;
    Utf8FromPicked(st.picked, utf8Label);
    return true;
}
