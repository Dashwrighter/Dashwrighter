# Honor / ignore tests

INI probes for settings the dash **Customize** dialog does not offer on that type (see `Dash-Customize-Settings.md`). Load the same file on the dash, capture, compare.

**Do not Save these files from the editor.** Load only. A round-trip can rewrite Digital width and Style 2 `orientation` / `hor`.

Background is the same gray plate as placement: `internal/images/PlacementGray.jpg`. Copy this folder onto the USB next to that image (or copy these INIs into the placement USB folder).

Channel **41** (Battery) on every gauge.

Regenerate: `python generate_honor.py`

## Files

| File | Question | What to look at |
|------|----------|-----------------|
| `H-analog-ct-crown.ini` | With Border off, does leftover `ct=1` still use Black blit tick/arc sizing? | Type 0. Columns **off0** (`crown=false ct=0`), **off1** (`false/1`, the test), **on0** Original, **on1** Black blit. Rows size **255** and **180**. **Result 2026-09-12:** `ct` still selects geometry with Border off. Editor **LOCKED 2026-09-19**: fat majors 6/4/3; needle meets inner ticks; label/value center-anchored; off1 is blit-inner (no sprite); blit ID stretched (420/213/148/98). Captures: `H-analog-ct-crown DASH.png` / `EDITOR.png`. Do not Save from the editor. |
| `H-analog-units.ini` | Does analog paint units next to the numeric value? | Type 0 2×2: **v1u1** value+units (the test), **v1u0** value only, **v0u1** units only, **v0u0** neither. **Dig** is type 2 with both on (positive control). Battery in 0–20, `units=psi`. **Result (`Dash Dialogs/H-analog-units.jpg`):** analog paints no units; **Dig** shows `psi`. |
| `H-thin-borderstyle.ini` | Thin Analog has Border, no Border Style. Does `ct` still blit? | Four type-4, `crown=true`: **ct0** Original, **ct1** Black, **ct2** Grey, **ct3** Silver. Identical except `ct`. |
| `H-thin-scale.ini` | Do Thin Analog scale / mtics / divisor paint? | **off** = no scale, no minors. **tics** = scale on, `mtics=2`. **div10** = same + `scalingDivisor=10`. **Result (`Dash Dialogs/H-thin-scale.jpg`):** ticks on all three (including off). No obvious scale numerals on tics/div10. |
| `H-digital-valueoff.ini` | Digital has no Value Display row. Does `numericIndicatorEnabled=false` hide the number? | **Result:** no — dash always paints the number. Editor: disable Value Display on type 2; always paint. |
| `H-digital-alpha.ini` | Does the dash honor the A byte on Digital **foreground** and **zone** colors? | **Result (`Dash Dialogs/H-digital-alpha.jpg`, Battery 12.5):** background A works (`bg50`). fg A=0 hides text (`fg0`). Zone A=0 skips the plate (`b12.0`–`b12.4` gold gone, `g12.0`–`g12.4` ghost text). Zone A=128 looks fully opaque red (`c12.0`–`c12.4`). Trip split is correct (12.5 > 12.4, not > 12.6). |
| `H-rpm-labelon.ini` | RPM Customize has no Label row. Does `displayLabel=true` show a caption? | Top bar `displayLabel=false`, bottom `true`, both `label=RPM`. |
| `H-style2-orient.ini` | Style 2 has no Orientation row. Does `orientation` / `hor` rotate it? | Same 280×120 box: **o0h0** control, **o1h0**, **o0h1**, **o1h1**. **Result (`Samples/dash-renders/H-style2-orient.png`):** `orientation=true` rotates the trapezoid **90° CCW** (high side to the top). Paint/hit AABB is **H×H** at `pos=` (280×120 → ~120×120), not W×H swapped. `hor` does **not** change paint (o0h0=o0h1, o1h0=o1h1). Customize still has no Orientation row. Editor **LOCKED 2026-09-19** (orientation, dividers, value). Captures: `H-style2-orient EDITOR.png`. Do not Save from the editor. |
| `H-peaks.ini` | Peak Hold paint? | **Ignore.** Editor will never render peaks. Min/Max peak stay in More settings only. |
| `H-led-math.ini` | Status LED has no Channel Math row. Does `valueMult` still run? | **x1** `valueMult=1` (normal color). **x2** `valueMult=2` (Battery×2 should high-alarm). Large LEDs. |
| `H-led-fontsize.ini` | LED Label Font Size ladder (`Compare.xmcd`). | Normal LEDs, labels on, same Y: **8, 12, 18, 24, 36, auto**. Cap = 0.72×FS; LED drop = 8+cap; clipped to box. |
| `H-rpm-mode.ini` | Target RPM mode is `rpmModeEnabled`. | Top `false` (classic 80px plate). Bottom `true` (no plate, centered lit). |
| `H-scale-on.ini` | Placement never turns scale on. Photos show the row on Bargraph / Style 2 / Graph. | **Bar**, **S2**, **Gr** with `scaleValues=true` and minors. Paint-fidelity, not a hidden-key test. |
| `H-barh-fontsize-labelon.ini` | Does bargraph thickness / spine track `fontSize`? | Stacked H bars **8, 12, 15, 18, 24, 36, auto**. Card H should be `fontSize+39` (AUTO=12). Bar stays 25px. Spine 2px above font center. |
| `H-barh-fontsize-labeloff.ini` | Label-off still reserves the font-sized band? | Same positions/sizes as label-on. No spine. Overlay on the label-on capture: bars should line up, empty band should grow with `fontSize`. |
| `H-barv-fontsize.ini` | Vertical is the same card rotated? | **8, 18, 36** side by side. Thickness is width. |

## Dash save (optional)

Load a file, open Customize, Ok without changing anything, copy the INI back off the dash, diff. Hidden keys that vanish were stripped; keys that remain were honored as storage even if paint did not change.
