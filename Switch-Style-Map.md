# Soft switch style map

**Status:** **LOCKED / STOPPING POINT 2026-08-07** (editor preview + style map)  
**Last updated:** 2026-08-07

---

## Dash Style menu (UI names)

Photos:
- `Research/switch-style-photos/IMG_20260805_182800-style-menu.jpg` (top)
- `Research/switch-style-photos/IMG_20260805_182819-style-menu-2.jpg` (mid)
- `Research/switch-style-photos/IMG_20260805_182839-style-menu-3.jpg` (end)

Dialog: **Style** — “Changes the visual switch style.”

### Full inventory (menu order) — **17 styles**

| # | UI name (exact) | Family |
|---|-----------------|--------|
| 1 | **Rocker** | Rocker |
| 2 | **Rocker 2** | Rocker |
| 3 | **Slider** | Slider |
| 4 | **Black Toggle** | Toggle latching |
| 5 | **Black Toggle (small)** | Toggle latching |
| 6 | **Silver Toggle** | Toggle latching |
| 7 | **Silver Toggle (small)** | Toggle latching |
| 8 | **Black Momentary Switch** | Toggle momentary (shared art w/ black toggle) |
| 9 | **Black Momentary Switch (small)** | Toggle momentary |
| 10 | **Silver Momentary Switch** | Toggle momentary |
| 11 | **Silver Momentary Switch (small)** | Toggle momentary |
| 12 | **Gray Momentary Button (large)** | Round button |
| 13 | **Gray Momentary Button** | Round button |
| 14 | **Gray Momentary Button (small)** | Round button |
| 15 | **Black Momentary Button (large)** | Round button |
| 16 | **Black Momentary Button** | Round button |
| 17 | **Black Momentary Button (small)** | Round button |

**List complete** (182839 ends at Black Momentary Button (small); no further rows).  
**Note:** Menu order ≠ `style=` integer order.

---

## Locked `style=` integers (0–16) — all 17 menu styles

**Evidence:** `TestLayoutDashRenders/TX-switch-style-ids.png` + `TX-switches.png` + user confirm 2026-08-07 (0–14).  
**15–16:** pattern fit (black-toggle dual-use after silver dual-use 7–8); not visible on ladder shot because the **dash caps how many switches it will create** on one layout — ladder stopped painting before 15–16.

| `style=` | UI name | WidgetGraphics | Cell (W×H) | Notes |
|---------:|---------|----------------|-----------:|-------|
| **0** | Rocker | `switches.png` | 80×170 | 3-frame sheet 242×170 |
| **1** | Slider | `switches-slider.png` | 250×100 | Sheet 750×100 |
| **2** | Rocker 2 | `switch-3.png` | ~103×204 | Sheet 310×204 |
| **3** | Black Toggle | `switches-blacktoggle-150.png` | 150×150 | |
| **4** | Black Toggle (small) | `switches-blacktoggle-80.png` | 80×80 | |
| **5** | Silver Toggle | `switches-silvertoggle-150.png` | 150×150 | |
| **6** | Silver Toggle (small) | `switches-silvertoggle-80.png` | 80×80 | |
| **7** | Silver Momentary Switch | `switches-silvertoggle-150.png` | 150×150 | **Same art as 5** |
| **8** | Silver Momentary Switch (small) | `switches-silvertoggle-80.png` | 80×80 | **Same art as 6** |
| **9** | Gray Momentary Button (small) | `sw-mom-red-sm.png` | 80×80 | Size ladder small→large |
| **10** | Gray Momentary Button | `sw-mom-red.png` | 120×120 | |
| **11** | Gray Momentary Button (large) | `sw-mom-red-lg.png` | 160×160 | Confirmed via `TX-switches` MomLarge |
| **12** | Black Momentary Button (small) | `sw-mom-dark-sm.png` | 80×80 | |
| **13** | Black Momentary Button | `sw-mom-dark.png` | 120×120 | |
| **14** | Black Momentary Button (large) | `sw-mom-dark-lg.png` | 160×160 | |
| **15** | Black Momentary Switch | `switches-blacktoggle-150.png` | 150×150 | **Same art as 3** (pattern; not on ladder PNG) |
| **16** | Black Momentary Switch (small) | `switches-blacktoggle-80.png` | 80×80 | **Same art as 4** (pattern; not on ladder PNG) |

Styles **≥17** on the oversize ladder INI: unused / invalid. Preserve unknown IDs on load if seen.

**Dash limit:** Pro Dash will not create an unbounded number of soft switches on one screen; dense ladders (e.g. 0..23) may omit trailing IDs. Prefer smaller packs or multi-screen for future ID proofs.

**Test layout:** `TestLayouts/TX-switch-style-ids.ini` · USB: `DashTransfer/TX-switch-style-ids.ini`

---

## WidgetGraphics (frame strips)

All sheets are **horizontal 3-frame strips**:

| Frame index | State | Notes |
|------------:|-------|--------|
| **0** (left) | **Off** | Inactive / released |
| **1** (center) | **On** | Active / pressed |
| **2** (right) | **Disabled** | Dimmed / unavailable |

Sources: `WidgetGraphics/` (= firmware `opt/dig-dash/content/`).

### Why this pairing

| Name cue | Asset cue |
|----------|-----------|
| Slider | `switches-slider.png` |
| Black / Silver **Toggle** + **Momentary Switch** | `switches-*-toggle-*` (150 = full, 80 = small) — dual-use skins |
| **Rocker** vs **Rocker 2** | `switches.png` (oval LED) vs `switch-3.png` (rect mesh) |
| **Gray** Momentary **Button** | `sw-mom-red*` — lighter / red-glow family |
| **Black** Momentary **Button** | `sw-mom-dark*` — darker dish family |
| **(large)** / default / **(small)** | `-lg` / bare / `-sm` (160 / 120 / 80) |

**Unique sheets:** 13 PNG files cover all named styles that have art (toggle dual-use reuses sheets).

---

## Latching vs momentary (dual-use art)

| Shared sheet | Latching (`style=`) | Momentary (`style=`) |
|--------------|---------------------|----------------------|
| `switches-silvertoggle-150.png` | **5** Silver Toggle | **7** Silver Momentary Switch |
| `switches-silvertoggle-80.png` | **6** Silver Toggle (small) | **8** Silver Momentary Switch (small) |
| `switches-blacktoggle-150.png` | **3** Black Toggle | **15** Black Momentary Switch |
| `switches-blacktoggle-80.png` | **4** Black Toggle (small) | **16** Black Momentary Switch (small) |

**No visual difference** for dual-use pairs — same 3 frames. Function (latch vs mom) is not separate chrome.

---

## Editor draw rule

```text
sheet = map[styleId]
frame = Disabled ? 2 : (Active ? 1 : 0)   // preview: usually Off (0)
blit frame at native cell size — do not recolor body; do not free-scale
```

### Product constraints

- **Few keys** on switch/output objects.
- **Body art is not recolored** — only label / plate colors from INI.
- **Size is fixed by Style** (sheet cell dimensions).
- **Label colors editable** — see Customize dialog.

### Customize Soft Switch (dash dialog)

Photo: `Research/switch-style-photos/IMG_20260805_183725-customize-soft-switch.jpg`

| UI row | INI key | Notes |
|--------|---------|--------|
| **Label** | `text` | Caption |
| **Style** | `style` | Sprite + fixed size |
| **Inactive Label Color** | `inactiveColor` | Text when off — not body |
| **Active Label Color** | `activeColor` | Text when on — not body |
| **Background Color** | `background` | Plate behind control |
| **Transparency** | alpha on `background` | Plate transparency (no separate key found; treat as BG high byte) |

**Editor panel:** Label, Style (0–16), Inactive/Active label color, Background (+ alpha), channel, position.  
**Preview slider:** `demoNorm ≥ 0.5` → On frame + active label color; else Off + inactive.

---

## Checklist

- [x] Full Style menu (17 UI names)
- [x] Glyph ↔ UI name / WidgetGraphics map
- [x] Customize Soft Switch rows
- [x] **`style=` 0–14** locked from `TX-switch-style-ids` dash PNG (user confirm 2026-08-07)
- [x] **`style=` 15–16** Black Momentary Switch (+ small) by pattern (dash switch-count limit)
- [x] Editor: WidgetGraphics blit + preview-slider Off/On (2026-08-07)
- [x] **Editor LOCKED** — residual notes only (no reopen casually):
  - 15/16 not hard-proven by export (pattern + dual-use symmetry)
  - Transparency assumed = background alpha only
  - Prop panel may still show gauge-oriented extras when switch selected
  - Disabled frame not used (channel is runtime on the dash)
