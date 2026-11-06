// =============================================================================
// main.cpp — Application entry point
//
// Creates the main editor window and runs the Win32 message loop.
// All UI and document work lives in EditorWindow.
// Hidden: --shot <layout.ini> <out.png> writes a 1280×480 canvas PNG (no UI).
// =============================================================================

#include "App/EditorWindow.h"
#include "App/EditorInternals.h"
#include "Io/BackgroundResolver.h"
#include "Io/IniLayoutReader.h"
#include "Render/LayoutRenderer.h"
#include <shellapi.h>

static int RunHeadlessShot(const wchar_t* iniPath, const wchar_t* pngPath) {
    ULONG_PTR gdiplusToken = 0;
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    if (Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr) != Gdiplus::Ok)
        return 2;

    LayoutDocument doc;
    IniLayoutReader reader;
    if (!reader.Load(Ed::WideToUtf8(iniPath), doc) || !doc.ActiveScreen()) {
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return 3;
    }

    BackgroundResolver bgRes;
    bgRes.SetLayoutPath(Ed::WideToUtf8(iniPath));
    Gdiplus::Image* background = nullptr;
    const std::wstring bgPath = bgRes.ResolveWide(doc.ActiveBackgroundFileName());
    if (!bgPath.empty()) {
        background = new Gdiplus::Image(bgPath.c_str());
        if (!background || background->GetLastStatus() != Gdiplus::Ok) {
            delete background;
            background = nullptr;
        }
    }

    const int cw = 1280;
    const int ch = 480;
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = cw;
    bmi.bmiHeader.biHeight = -ch;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    HDC screenDC = GetDC(nullptr);
    HDC memDC = CreateCompatibleDC(screenDC);
    void* bits = nullptr;
    HBITMAP dib = CreateDIBSection(memDC, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    int rc = 4;
    if (dib && bits) {
        HBITMAP oldBmp = static_cast<HBITMAP>(SelectObject(memDC, dib));
        RECT canvasRect = { 0, 0, cw, ch };
        FillRect(memDC, &canvasRect, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));

        LayoutRenderer renderer;
        renderer.originX = 0;
        renderer.originY = 0;
        renderer.selectedGauge = -1;
        renderer.selectedOutput = -1;
        renderer.selectedLabel = -1;
        renderer.demoNorm = 0.12;
        renderer.Draw(memDC, *doc.ActiveScreen(), background, cw, ch);

        SelectObject(memDC, oldBmp);
        Gdiplus::Bitmap src(cw, ch, cw * 4, PixelFormat32bppRGB, static_cast<BYTE*>(bits));
        CLSID pngClsid = {};
        if (src.GetLastStatus() == Gdiplus::Ok && Ed::GetEncoderClsid(L"image/png", &pngClsid)
            && src.Save(pngPath, &pngClsid, nullptr) == Gdiplus::Ok)
            rc = 0;
        DeleteObject(dib);
    }
    DeleteDC(memDC);
    ReleaseDC(nullptr, screenDC);
    delete background;
    LayoutRenderer::ReleaseCachedImages();
    Gdiplus::GdiplusShutdown(gdiplusToken);
    return rc;
}

int WINAPI wWinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/,
                    PWSTR /*lpCmdLine*/, int /*nShowCmd*/) {
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argv && argc >= 4 && wcscmp(argv[1], L"--shot") == 0) {
        const int rc = RunHeadlessShot(argv[2], argv[3]);
        LocalFree(argv);
        return rc;
    }
    if (argv) LocalFree(argv);

    EditorWindow editor;
    if (editor.Create())
        editor.Run();
    return 0;
}
