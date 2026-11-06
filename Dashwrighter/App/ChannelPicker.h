#pragma once

#include <windows.h>

// Modal channel picker (one list + Find filter).
// Lists Holley-Channel-Picker.csv only. On OK, channelId is the picked id.
bool RunChannelPicker(HWND owner, int& channelId);
