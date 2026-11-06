#pragma once

// =============================================================================
// IniUtils.h — Small string helpers shared by layout INI read/write
//
// Holley keys look like: analogMeters\0\label=Oil
// GetShortKey turns that into "label". Positions/sizes are quoted "(x,y)" pairs.
// =============================================================================

#include <string>

namespace IniUtils {

// Strip leading/trailing whitespace and CR/LF.
std::string Trim(const std::string& s);

// Wrapping ASCII "..." is INI syntax (padding so Trim does not eat spaces).
// Strips one outer pair; inner spaces stay. Unquoted values unchanged.
std::string UnquoteIniValue(const std::string& s);

// Wrap in "..." when the value has leading or trailing whitespace so a later
// Trim+Unquote round-trips. Ordinary strings stay unquoted (Holley factory).
std::string QuoteIniValueIfNeeded(const std::string& s);

// True when s is empty or only space/tab/CR/LF (quoted units padding).
bool IsWhitespaceOnly(const std::string& s);

// Last path segment after '\\' — e.g. "analogMeters\\0\\label" → "label".
std::string GetShortKey(const std::string& fullKey);

// "true" / "false" for Holley boolean fields.
std::string BoolStr(bool v);

// Compact double for INI: integers without ".0", else trim trailing zeros.
std::string CleanDouble(double v);

// Parse Holley pos "\"(x,y)\"" (or unquoted) into x,y. No-op on bad input.
void ParsePosition(const std::string& val, int& x, int& y);

// Parse Holley size "\"(w,h)\"" into w,h.
void ParseSize(const std::string& val, int& w, int& h);

// Basename from a path (handles / and \).
std::string FileNameFromPath(const std::string& path);

// Directory including trailing separator, or empty if none.
std::string DirNameFromPath(const std::string& path);

// Accepts true/1/True/TRUE as true; everything else false.
bool ParseBool(const std::string& value);

// Holley string escapes → UTF-8.
//   \xNN   — one byte / codepoint 0x00–0xFF (e.g. units "\xb0\x46" for °F)
//   \xNNNN — BMP codepoint (e.g. Symbol PUA "\xf095" → U+F095)
std::string DecodeHolleyEscapes(const std::string& s);

// Inverse for save: non-ASCII → \xNN or \xNNNN (4 hex for codepoints > 0xFF).
std::string EncodeHolleyEscapes(const std::string& utf8);

// Inspector box (ANSI GetWindowTextA) → UTF-8 model.
// Accepts typed \xNN / \xNNNN and Alt-code / ACP high bytes (e.g. °).
// Does not Latin-1-expand valid UTF-8.
std::string EditTextToUtf8(const std::string& editBytes);

// UTF-8 model → inspector box (Holley escapes for non-ASCII). Same as Encode.
std::string Utf8ToEditText(const std::string& utf8);

// UTF-8 model → UTF-16 for GDI/GDI+ paint. Still-escaped \x in the string is
// decoded first; invalid UTF-8 falls back to ACP (legacy Latin-1 in the model).
std::wstring Utf8ToWide(const std::string& utf8);

} // namespace IniUtils
