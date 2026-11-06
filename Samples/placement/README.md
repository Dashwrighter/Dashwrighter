# Placement / size test pack

Geometry tests for editor vs dash. Same INI, 1:1 screenshots, compare the **occupied box**. Interior paint is secondary.

**Do not Save these files from the editor.** Load only. A round-trip can rewrite Digital width and Style 2 `orientation`/`hor`.

## Background

`internal/images/PlacementGray.jpg` — flat 1280×480, RGB 196,196,196.

INI key: `Background=/internal/images/PlacementGray.jpg`

Copy this folder onto the dash USB so `internal/images/PlacementGray.jpg` sits next to the INIs the way Holley expects.

## How to compare

1. Load an INI in the editor. Screenshot (1280×480 PNG).
2. Load the same INI on the dash. Capture at 1280×480, no rescale.
3. Overlay. A miss is a shifted or wrong-size plate against the gray.

Sibling files (border / label / units) share the same `pos=` / `size=` so they overlay each other too.

Gauge plates are **opaque black** (`background=4278190080`). Captions are `A`, `B`, `C`, … when labels are on.

## Files (35)

| File | Type | What varies |
|------|------|-------------|
| `P0-analog-noborder-labeloff.ini` | Analog | No border, label off |
| `P0-analog-noborder-labelon.ini` | Analog | No border, label on |
| `P0-analog-original-labeloff.ini` | Analog | Original chrome, label off |
| `P0-analog-original-labelon.ini` | Analog | Original chrome, label on |
| `P0-analog-blit-labeloff.ini` | Analog | WidgetGraphics Black (`ct=1`), label off |
| `P0-analog-blit-labelon.ini` | Analog | WidgetGraphics Black, label on |
| `P4-thin-noborder-labeloff.ini` | Thin Analog | Border off, label off |
| `P4-thin-noborder-labelon.ini` | Thin Analog | Border off, label on |
| `P4-thin-border-labeloff.ini` | Thin Analog | `crown=true` (dash may ignore), label off |
| `P4-thin-border-labelon.ini` | Thin Analog | Border on, label on |
| `P2-digital-noborder-labeloff-unitsoff.ini` | Digital | Baseline |
| `P2-digital-border-labeloff-unitsoff.ini` | Digital | Spine / Border on |
| `P2-digital-noborder-labelon-unitsoff.ini` | Digital | Label on |
| `P2-digital-noborder-labeloff-unitson.ini` | Digital | Units on (W is a side-effect of H) |
| `P1-barh-noborder-labeloff.ini` | Bargraph H | `hor=true`, thickness 52 |
| `P1-barh-noborder-labelon.ini` | Bargraph H | Label on |
| `P1-barh-border-labeloff.ini` | Bargraph H | Border on |
| `P1-barh-border-labelon.ini` | Bargraph H | Border + label |
| `P1-barv-noborder-labeloff.ini` | Bargraph V | `orientation=true` |
| `P1-barv-noborder-labelon.ini` | Bargraph V | Label on |
| `P11-style2-noborder-labeloff.ini` | Style 2 | Free W×H, `orientation=false` |
| `P11-style2-noborder-labelon.ini` | Style 2 | Label on |
| `P11-style2-border-labeloff.ini` | Style 2 | Border on |
| `P11-style2-border-labelon.ini` | Style 2 | Border + label |
| `P9-graph-noborder-labeloff.ini` | Graph | Free W×H |
| `P9-graph-noborder-labelon.ini` | Graph | Label on |
| `P9-graph-border-labeloff.ini` | Graph | Border on |
| `P9-graph-border-labelon.ini` | Graph | Border + label |
| `P5-led-labeloff.ini` | Status LED | Normal 44×63 and Large 90×109 only |
| `P5-led-labelon.ini` | Status LED | Label on |
| `P5-led-fontsize.ini` | Status LED | Label on; Font Size **8, 12, 18, 24, 36, auto** (Normal) + Large **8, 18, 36**. Same Y per row. Cap = 0.72×FS; LED clipped to box. |
| `P6-rpm.ini` | RPM Bar | INI 750×100 (paint 80px from `pos.y`), `rpmModeEnabled=false` |
| `P6-rpm-mode.ini` | RPM Bar | Same pos as P6. **A** classic. **B** `rpmModeEnabled=true` (target). **C** max thresh only — still classic. Overlay on P6. |
| `P6-rpm-mode check only.ini` | RPM Bar | Dash-saved. Only **B** target. Load-only. |
| `P6-rpm-mode threshold chg.ini` | RPM Bar | Dash-saved after setpoint/range edit. B still target; paint same as check-only. Load-only. |
| `P8-symbol.ini` | Symbol | `size=` ladder, `fontSize=35` fixed, glyph `\xf095` |
| `Psw-switches.ini` | Soft switch | Style 4 (80×80) and style 3 (150×150) |

Regenerate: `python generate_placement.py`

## Size ladders

- **Analog:** 480, 255, 180, 127, 121 (floor ~120). Ticks on, scale numbers off, warn/alarm arcs on. Sweep −120°…120°.
- **Thin Analog:** 480, 255, 180, 127, 80, 40.
- **Digital:** height 480, 255, 160, 89, 50, 28. Width from the documented aspect formula (not a free control).
- **Bargraph H:** length 1200, 750, 400, 255, 127, 80 × thickness 52.
- **Bargraph V:** length 400, 255, 180, 121; card W 101.
- **Style 2 / Graph:** mix of max, odd, wide-short, tall-narrow, flush edges.
- **Symbol:** 480, 255, 180, 127, 80, 40, 24 with fontSize held at 35.
- **LED / RPM / switches:** legal sizes only, multiple positions (origin, flush right, flush bottom).

Channel 41 (Battery) on gauges. Switches use channel 11 (`ssr.1` on this bench — not a universal Holley id). Placement does not require the switch to enable.
