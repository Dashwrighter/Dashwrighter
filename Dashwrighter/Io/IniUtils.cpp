// =============================================================================
// IniUtils.cpp — Implementation of Holley INI string helpers
// =============================================================================

#include "IniUtils.h"
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstdio>

namespace IniUtils {

std::string Trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

std::string UnquoteIniValue(const std::string& s) {
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"')
        return s.substr(1, s.size() - 2);
    return s;
}

std::string QuoteIniValueIfNeeded(const std::string& s) {
    if (s.empty()) return s;
    const bool lead = s.front() == ' ' || s.front() == '\t';
    const bool trail = s.back() == ' ' || s.back() == '\t';
    if (!lead && !trail) return s;
    return "\"" + s + "\"";
}

bool IsWhitespaceOnly(const std::string& s) {
    for (unsigned char c : s) {
        if (c != ' ' && c != '\t' && c != '\r' && c != '\n')
            return false;
    }
    return true;
}

// Holley full keys: section\index\field  →  field
std::string GetShortKey(const std::string& fullKey) {
    size_t last = fullKey.find_last_of('\\');
    if (last != std::string::npos) return Trim(fullKey.substr(last + 1));
    return Trim(fullKey);
}

std::string BoolStr(bool v) { return v ? "true" : "false"; }

// Prefer integer form when the value is whole; otherwise up to 6 decimals, trimmed.
std::string CleanDouble(double v) {
    if (std::abs(v - std::round(v)) < 1e-9)
        return std::to_string(static_cast<long long>(std::round(v)));
    char buf[64];
    snprintf(buf, sizeof(buf), "%.6f", v);
    std::string s(buf);
    size_t dot = s.find('.');
    if (dot != std::string::npos) {
        size_t last = s.find_last_not_of('0');
        if (last != std::string::npos && last > dot) s.erase(last + 1);
        else s.erase(dot);
    }
    return s;
}

// Expects text containing "(x,y)" somewhere in the string.
void ParsePosition(const std::string& val, int& x, int& y) {
    size_t p1 = val.find('(');
    size_t comma = val.find(',', p1);
    if (p1 == std::string::npos || comma == std::string::npos) return;
    try {
        x = std::stoi(val.substr(p1 + 1, comma - p1 - 1));
        y = std::stoi(val.substr(comma + 1));
    } catch (...) {}
}

void ParseSize(const std::string& val, int& w, int& h) {
    size_t p1 = val.find('(');
    size_t comma = val.find(',', p1);
    if (p1 == std::string::npos || comma == std::string::npos) return;
    try {
        w = std::stoi(val.substr(p1 + 1, comma - p1 - 1));
        h = std::stoi(val.substr(comma + 1));
    } catch (...) {}
}

std::string FileNameFromPath(const std::string& path) {
    size_t slash = path.find_last_of("/\\");
    if (slash != std::string::npos) return path.substr(slash + 1);
    return path;
}

std::string DirNameFromPath(const std::string& path) {
    size_t slash = path.find_last_of("/\\");
    if (slash != std::string::npos) return path.substr(0, slash + 1);
    return "";
}

bool ParseBool(const std::string& value) {
    return value == "true" || value == "1" || value == "True" || value == "TRUE";
}

static int HexNibble(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

// Append one Unicode codepoint (BMP) as UTF-8.
static void AppendUtf8(std::string& out, unsigned int cp) {
    if (cp < 0x80) {
        out.push_back(static_cast<char>(cp));
    } else if (cp < 0x800) {
        out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp < 0x10000) {
        out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
}

// Holley label escapes: "\xNN" (byte) or "\xNNNN" (BMP codepoint, e.g. Symbol PUA).
// Prefer the longest valid hex run of length 2 or 4 after \x.
std::string DecodeHolleyEscapes(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 3 < s.size() && (s[i + 1] == 'x' || s[i + 1] == 'X')) {
            int nibbles[4];
            int n = 0;
            while (n < 4 && i + 2 + n < s.size()) {
                const int v = HexNibble(s[i + 2 + n]);
                if (v < 0) break;
                nibbles[n++] = v;
            }
            // Prefer 4-digit BMP escapes (Symbol PUA); else 2-digit byte.
            if (n >= 4) {
                const unsigned int cp = static_cast<unsigned int>(
                    (nibbles[0] << 12) | (nibbles[1] << 8) | (nibbles[2] << 4) | nibbles[3]);
                AppendUtf8(out, cp);
                i += 1 + 4; // skip x + 4 hex (loop adds 1)
                continue;
            }
            if (n >= 2) {
                const unsigned int b = static_cast<unsigned int>((nibbles[0] << 4) | nibbles[1]);
                AppendUtf8(out, b); // 0x00–0xFF as codepoints (Latin-1 / ASCII)
                i += 1 + 2;
                continue;
            }
        }
        out.push_back(s[i]);
    }
    return out;
}

// UTF-8 → Holley "\xNN" / "\xNNNN". Symbol PUA and other non-ASCII become escapes.
std::string EncodeHolleyEscapes(const std::string& utf8) {
    std::string out;
    out.reserve(utf8.size());
    for (size_t i = 0; i < utf8.size(); ) {
        const unsigned char c = static_cast<unsigned char>(utf8[i]);
        unsigned int cp = 0;
        size_t advance = 1;

        if (c < 0x80) {
            cp = c;
        } else if ((c & 0xE0) == 0xC0 && i + 1 < utf8.size()) {
            const unsigned char c1 = static_cast<unsigned char>(utf8[i + 1]);
            if ((c1 & 0xC0) == 0x80) {
                cp = ((c & 0x1F) << 6) | (c1 & 0x3F);
                advance = 2;
            } else {
                cp = c;
            }
        } else if ((c & 0xF0) == 0xE0 && i + 2 < utf8.size()) {
            const unsigned char c1 = static_cast<unsigned char>(utf8[i + 1]);
            const unsigned char c2 = static_cast<unsigned char>(utf8[i + 2]);
            if ((c1 & 0xC0) == 0x80 && (c2 & 0xC0) == 0x80) {
                cp = ((c & 0x0F) << 12) | ((c1 & 0x3F) << 6) | (c2 & 0x3F);
                advance = 3;
            } else {
                cp = c;
            }
        } else if ((c & 0xF8) == 0xF0 && i + 3 < utf8.size()) {
            // Outside BMP — fall back to per-byte escapes
            char buf[8];
            snprintf(buf, sizeof(buf), "\\x%02x", c);
            out += buf;
            ++i;
            continue;
        } else {
            cp = c;
        }

        i += advance;

        if (cp < 0x80 && cp != static_cast<unsigned int>('\\')) {
            out.push_back(static_cast<char>(cp));
        } else if (cp <= 0xFF) {
            char buf[8];
            snprintf(buf, sizeof(buf), "\\x%02x", cp);
            out += buf;
        } else {
            char buf[12];
            snprintf(buf, sizeof(buf), "\\x%04x", cp);
            out += buf;
        }
    }
    return out;
}

static bool IsValidUtf8(const std::string& s) {
    for (size_t i = 0; i < s.size(); ) {
        const unsigned char c = static_cast<unsigned char>(s[i]);
        if (c < 0x80) {
            ++i;
            continue;
        }
        int need = 0;
        unsigned int cp = 0;
        if ((c & 0xE0) == 0xC0) {
            need = 1;
            cp = c & 0x1F;
            if (c < 0xC2) return false;
        } else if ((c & 0xF0) == 0xE0) {
            need = 2;
            cp = c & 0x0F;
        } else if ((c & 0xF8) == 0xF0) {
            need = 3;
            cp = c & 0x07;
            if (c > 0xF4) return false;
        } else {
            return false;
        }
        if (i + static_cast<size_t>(need) >= s.size())
            return false;
        for (int k = 1; k <= need; ++k) {
            const unsigned char cc = static_cast<unsigned char>(s[i + static_cast<size_t>(k)]);
            if ((cc & 0xC0) != 0x80)
                return false;
            cp = (cp << 6) | (cc & 0x3F);
        }
        if (need == 2 && cp < 0x800) return false;
        if (need == 3 && cp < 0x10000) return false;
        if (cp >= 0xD800 && cp <= 0xDFFF) return false;
        if (cp > 0x10FFFF) return false;
        i += 1 + static_cast<size_t>(need);
    }
    return true;
}

static std::string AcpToUtf8(const std::string& acp) {
    if (acp.empty()) return {};
    const int nw = MultiByteToWideChar(CP_ACP, 0, acp.data(), static_cast<int>(acp.size()),
        nullptr, 0);
    if (nw <= 0) return acp;
    std::wstring wide(static_cast<size_t>(nw), L'\0');
    MultiByteToWideChar(CP_ACP, 0, acp.data(), static_cast<int>(acp.size()),
        wide.data(), nw);
    const int nu = WideCharToMultiByte(CP_UTF8, 0, wide.data(), nw, nullptr, 0, nullptr, nullptr);
    if (nu <= 0) return acp;
    std::string utf8(static_cast<size_t>(nu), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.data(), nw, utf8.data(), nu, nullptr, nullptr);
    return utf8;
}

std::string EditTextToUtf8(const std::string& editBytes) {
    std::string decoded = DecodeHolleyEscapes(editBytes);
    if (IsValidUtf8(decoded))
        return decoded;
    return AcpToUtf8(decoded);
}

std::string Utf8ToEditText(const std::string& utf8) {
    return EncodeHolleyEscapes(utf8);
}

std::wstring Utf8ToWide(const std::string& s) {
    std::string utf8 = s;
    if (s.find("\\x") != std::string::npos || s.find("\\X") != std::string::npos)
        utf8 = DecodeHolleyEscapes(s);
    if (!IsValidUtf8(utf8))
        utf8 = AcpToUtf8(utf8);
    if (utf8.empty())
        return {};
    const int n = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    if (n <= 0)
        return {};
    std::wstring wide(static_cast<size_t>(n), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, wide.data(), n);
    if (!wide.empty() && wide.back() == L'\0')
        wide.pop_back();
    return wide;
}

} // namespace IniUtils
