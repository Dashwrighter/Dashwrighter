#pragma once

// Compiled into the EXE. A loose file still wins when the program finds one
// (Roboto, Holley-Channel-Picker.csv, or WidgetGraphics).
// Apache-2.0 Roboto 1.200310 (2013). The license text is IDR_LICENSE_ROBOTO.
// IDI_APP is the window and file icon (Dashwrighter.ico). Lowest id so
// Explorer uses it for the EXE.
#define IDI_APP                            1
#define IDR_ROBOTO_REGULAR_2013          260
#define IDR_LICENSE_ROBOTO               261
#define IDR_CHANNEL_PICKER_CSV           262
#define IDR_BEZEL_BLACK_LG               263
#define IDR_BEZEL_SILVER_LG              264
#define IDR_SYMBOL_ATLAS                 265
#define IDR_SWITCH_ROCKER                266
#define IDR_SWITCH_SLIDER                267
#define IDR_SWITCH_ROCKER2               268
#define IDR_SWITCH_BLACKTOGGLE_150       269
#define IDR_SWITCH_BLACKTOGGLE_80        270
#define IDR_SWITCH_SILVERTOGGLE_150      271
#define IDR_SWITCH_SILVERTOGGLE_80       272
#define IDR_SWITCH_MOM_RED_SM            273
#define IDR_SWITCH_MOM_RED               274
#define IDR_SWITCH_MOM_RED_LG            275
#define IDR_SWITCH_MOM_DARK_SM           276
#define IDR_SWITCH_MOM_DARK              277
#define IDR_SWITCH_MOM_DARK_LG           278

#ifndef RC_INVOKED
#include <windows.h>

// LR_SHARED icons belong to the process. Do not DestroyIcon them.
inline void ApplyAppIcon(WNDCLASSEXW& wc) {
    const HINSTANCE inst = wc.hInstance ? wc.hInstance : GetModuleHandleW(nullptr);
    wc.hIcon = static_cast<HICON>(LoadImageW(
        inst, MAKEINTRESOURCEW(IDI_APP), IMAGE_ICON,
        GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_SHARED));
    wc.hIconSm = static_cast<HICON>(LoadImageW(
        inst, MAKEINTRESOURCEW(IDI_APP), IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED));
}
#endif
