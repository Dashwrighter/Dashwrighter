#pragma once

// Shared Digital (type 2) text layout. LOCKED 2026-08-27 (P2 vs Compare.xmcd);
// Latin face Roboto 2013 LOCKED 2026-09-19 (Arial 1.08 squeeze retired).
//   Size = height; pos = top-left (unchanged).
//   Value cap-height (capital ink): unlabeled 0.714×H;
//     labeled 0.714×H − (0.72×fontSize + 5). em = cap/0.72.
//   Value cap bottom 4px above plate; label cap top 8px below plate top.
//   cap≤0 → no value (dash F labeled).
//   Plate W: 8px each side + value advances (Roboto 2013 matches dash Latin;
//     the old Arial 1.08 X-squeeze is retired).
// Paint and Normalize must match.

#include "../Model/LayoutTypes.h"
#include <string>

constexpr float kDigitalValueCapFrac = 0.714f;
constexpr float kDigitalCapPerEm = 0.72f;
constexpr int kDigitalLabelTopPx = 8;
constexpr int kDigitalValueBottomPx = 4;
constexpr int kDigitalLabelValuePadPx = 5; // FS18: cap 13 + 5 = 18
constexpr int kDigitalPadPx = 8;           // left inset (was 4)
constexpr int kDigitalRightPadPx = 8;      // right inset (was 3)
// 1.0 — preview Latin is Roboto 2013 (same cut as the dash). Was 1.08 for Arial.
constexpr float kDigitalWidthAdvanceScale = 1.0f;

struct DigitalTextLayout {
    int pad = kDigitalPadPx;
    int rightPad = kDigitalRightPadPx;
    int labelPx = 12;
    int labelCapPx = 0;      // capital ink of the label (0.72×fontSize)
    int spineBand = 0;       // 0 = no label band
    int spineGap = 0;
    int valueBandH = 0;
    int valueCapPx = 0;      // capital ink of the value; 0 = do not paint value
    int valuePx = 0;         // em; 0 = do not paint value
    int unitsPx = 0;
};

DigitalTextLayout ComputeDigitalTextLayout(int height, const Gauge& g);

// Worst-case value string (formatted min/max at precision) and Holley units
// (single-letter units lowercased, matching DrawDigital).
std::string DigitalWorstCaseValue(const Gauge& g);
std::wstring DigitalDisplayUnitsW(const Gauge& g);
std::wstring DigitalUtf8ToWide(const std::string& s);
