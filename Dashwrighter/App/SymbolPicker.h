#pragma once

#include <windows.h>
#include <string>

// Modal Holley-style two-page symbol keyboard.
// On OK, utf8Label is the picked run (PUA codepoints + ASCII spaces).
bool RunSymbolPicker(HWND owner, std::string& utf8Label);
