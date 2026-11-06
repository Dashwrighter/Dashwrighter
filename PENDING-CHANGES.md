# Current state

Gauge painters: Analog (P0) **LOCKED 2026-09-19** (fat majors 6/4/3; label/value center-anchored; needle meets inner ticks). All other create-gauge painters locked. **Preview Latin is Roboto 1.200310 (2013)** — not Arial; Symbol atlas PUA unchanged. **Bargraph (P1) LOCKED 2026-08-29** — length-only Size; thickness = label band + 25px bar (AUTO label=12). Digital (P2) locked. Thin Analog (P4) locked. **Status LED (P5) LOCKED 2026-08-29** — label-off LED top = `pos.y`; label-on cap top = `pos.y+8`, LED top = that + cap (`round(0.72×fontSize)`; AUTO=12). Art/tint unchanged; LED clipped to stored box. **RPM Bar (P6) LOCKED 2026-08-28** — mode = `rpmModeEnabled`; classic plate 750×80 from `pos.y`; INI/hit 750×100. **Style 2 (P11) LOCKED 2026-09-19** — P11 trapezoid; `orientation` 90° CCW (H×H AABB; `hor` ignored); unlit solid / lit 1px background gaps; value 0.24×endH on dash baseline. Honor `H-style2-orient` accepted. **Symbol (P8) LOCKED 2026-09-02** — atlas blit + Latin fallback (cap 0.72×FS, X-squeeze to `size=`; `1` still pinched, accepted).

**Dash Customize:** nested dialog interiors catalogued 2026-08-28 in `Dash-Customize-Settings.md`. Remaining photo gaps: Range interior, Target RPM with Enable on. Free-label key set locked (label, fontSize, foreground, background; Transparency = background alpha). Averaging list is complete (None through Very Fast). More settings: Averaging and Updates / sec are dropdowns (2026-09-05). RPM **Steady warning color** → `wfr` 0/1/2 proven; More settings dropdown. Do not paint flash. **F to C** Channel Math: no extra key yet (need Save after Ok). **Target RPM mode** is RPM Bar only (2026-09-12): Enable off = fixed-width 15-LED; on = width grows with RPM. P6 painter not reopened. RPM LED group typed counts (1000/2000 on analogMeters\31) are not stored. **Editor LED counts = `nnl`/`ncl` only** (2026-09-12). Analog **LOCKED 2026-09-19** (fat pen 6/4/3; center-anchored label/value; needle inner-tick).

**Canvas:** 1280×480 for 12.3" and 6.86" Pro Dash. INIs are interchangeable.

**Shell:** Dark + Holley red; inspector under the preview (`InspectorLayout.h`), enable/disable from Customize matrix (2026-09-08); More settings (channel / math / rate / peaks / flags / `hor`); Symbol **Sym** picker; gauge **Ch** picker (`Holley-Channel-Picker.csv`). Low/High zone checkboxes gate alarm/caution color pickers; Normal is not gated. **Positions** toolbar: screenshot with a blue ring on every gauge.

**Selection:** Empty preview click clears. First click on a widget selects (no move). Drag starts on a later click of the already-selected widget (5px slop). Right-click a stack of 2+ for a pick menu. Analog / Thin Analog hit the outer circle. One widget at a time.

Build: x64 Debug / Release (VS 18 / v145).

---

## Implemented (this pass)

49. **Bargraph unlit dividers — 2026-09-20.** Same rule as Style 2: solid unlit track; 1px of bar background only at segment boundaries the fill has passed. Honor `H-barh-fontsize-labeloff` (dash: no gray gaps in the black).

48. **Style 2 scale values screen-space — 2026-09-20.** After orientation, horizontal scale used card-local `barBot` as canvas Y, so numerals sat at the top-left offset by gauge height (`H-style2-scalevalues`). Both orientations now `mapPt` then draw upright.

47. **Color picker Alpha — 2026-09-19.** Change… opens R/G/B/Alpha (0–255) plus checkerboard swatch. RGB… keeps current A (including 0). No force-opaque. Background picker updates the Transparency slider. Same dialog for FG, BG, zones, switch colors.

46. **wfr combo + Digital value/alpha — 2026-09-19.** More settings **Warning flash** dropdown: 0 Steady / 1 Flash slow / 2 Flash quick. No preview flash. Digital always paints the value (ignore `numericIndicatorEnabled`). Digital paint: background A as stored; fg A=0 skip text; zone A=0 skip warn plate; A≥1 opaque for fg and zone flood.

45. **Dash Dialogs + honor photos 2026-09-19 (car, phone).** See `Dash Dialogs/` and `H-rpmbar-wfr-and-avg.ini`. **`wfr`:** 0/1/2 = Steady / Flash slow / Flash quick (bars 1–3). **`av`:** user explicitly set Slow / Medium / Fast; file is 1 / 2 / 3. Those names map to those ints. Dropdown may alias (e.g. 180 also writing 2) — not tested. Editor list 2=180 / 3=Medium is therefore wrong if Medium=2 and Fast=3. Target RPM setpoint/range still land in `minNormal` / `minCaution` (6000/3000, 4000/2000, 5000/2000). **Digital alpha** (`H-digital-alpha.jpg`, Battery 12.5): background A is real (`bg50`); foreground A=0 hides text (`fg0`); A=128 text looks opaque; zone flood A=0 skips the plate (`b12.0`–`b12.4`, `g12.0`–`g12.4`); zone A=128 looks fully opaque red (`c12.0`–`c12.4`). Complete vanish = opaque plate + fg A=0 + zone A=0. **Digital `numericIndicatorEnabled=false` does not hide the number** (`H-digital-valueoff.jpg`). **Analog does not paint units**; Digital does (`H-analog-units.jpg`). Gauge Sweep 270/120/90 are presets that fill start/end (photo: 270 → −135/135); no extra keys. F to C radio does not fill a,b,c (still 0,1,0). Symbol/Switch Transparency copy is background-color alpha. Range interior is min/max only. Do not implement from this catalog pass.

44. **Style 2 value size/anchor — LOCKED 2026-09-19 (user accepted).** Em = 0.24×endH (clamp 12–48). Cell-top at trapezoid centroid, then up by em−cap+2 so the baseline matches honor `H-style2-orient` (cap 14px, ink y 122–135). `EDITOR.png` reshot.

43. **Style 2 dividers — LOCKED 2026-09-19 (user accepted).** Honor `H-style2-orient`. Unlit trapezoid is solid (no canvas gaps, no black strokes). Lit fill only: 1px of bar `background` at step/minor boundaries the fill has passed. `H-style2-orient EDITOR.png` reshot.

42. **Style 2 orientation — LOCKED 2026-09-19 (user accepted).** Honor `H-style2-orient`. `orientation=true` is 90° CCW (high side to the top). `hor` ignored. INI `size=` unchanged. Paint/hit AABB is **H×H** at `pos=` (280×120 paints ~120×120, not W×H swapped). Label/spine/bar/value rotate; scale numbers stay upright. Inspector **Vertical** enabled for type 11 as editor-only (Customize still has no Orientation row; matrix unchanged). Raw `hor` stays type 1 only.

41. **Preview Latin = Roboto 2013 — LOCKED 2026-09-19.** Embedded `Roboto-Regular-2013.ttf` (Google Version 1.200310; Apache 2.0). Same cut as Holley RobotoPlus Latin. All non-symbol preview text (gauges, scale, free labels, switch captions, Symbol Latin fallback) uses a process-private collection so a system Roboto v2 install cannot win. Digital Arial 1.08 X-squeeze retired (`kDigitalWidthAdvanceScale` 1.0). Symbol atlas PNG unchanged. Inspector chrome stays the system UI font. Loose TTF next to the EXE / repo overrides the embed.

40. **Analog ticks / needle / face text — LOCKED 2026-09-19.** Honor `H-analog-ct-crown` accepted. Fat major pen (`ct>=1`) 6/4/3 at S 255/180/127 (was 8/5/4). `ct=0` stays 2px. Needle tip at tickOuter−13 original / −11 blit (was nearly tick outer; dash meets the inner majors). Label and value GDI+ center-anchored on the pivot vertical (label 0.30×R above, value 0.23×R+2 below). Font formulas unchanged (`fontSize` label, `max(18, round(R/4))` value). Honor `EDITOR.png` reshot. Do not reopen casually.

39. **INI quoted strings — 2026-09-15.** Wrapping `"..."` is INI syntax, not glyphs. Reader `UnquoteIniValue` after Trim (inner spaces kept). Writer `QuoteIniValueIfNeeded` on label/units/switch `text` when leading/trailing whitespace (Holley factory stays unquoted). Whitespace-only `units` paint as empty on Digital (Cutlass Trip 1). Labels keep padding (GREEN-PACMAN `   DOWN SHIFT   `).

38. **Overlap Front/Back — 2026-09-13.** Right-click stack (2+ widgets): each gauge has a submenu Select / Bring to front / Send to back. Moves `analogMeters` vector order (INI index on Save). Labels/switches stay select-only (firmware layers). Ctrl+Tab dropped 2026-09-26: beta testers are satisfied with the flyout. Do not add it.

37. **Select does not rewrite Type — 2026-09-13.** Populate holds `suppressLiveUpdate` through `ClearPropertyPanel` + `SetPropMode` (EnableWindow/BN_CLICKED used to write Analog onto a just-selected Symbol). `gaugeType` / switch `style` copy from the Type combo only on real `CBN_SELCHANGE`. GREEN-PACMAN paddle shifter.

36. **Analog tick / needle / blit-ID hairline — 2026-09-12.** `ct=0` major pen = minor (2px); `ct>=1` fat (8 at S≥255). Needle tip at tick rim, wider polygon. Blit inner sample skips PNG lip (`kBezelSrcInnerSample`); hard inner clip; face-color cover stroke on ID. Honor `H-analog-ct-crown EDITOR.png` reshot. User: INI renders correctly; **relocked**.

35. **Border Style stays live when Border is off — 2026-09-12.** Analog inspector Style combo is not gated on `crown`. `ct` still paints with Border unchecked. Comment in `ApplyGaugeTypeVisibility` so it is not later tied to the Border checkbox.

34. **Analog sizing pass — 2026-09-12, then LOCKED.** `AnalogIsBlit` is `ct` 1/2/3 (Border off still uses blit inner geom, no sprite). Blit OD stays PNG×size= (`kBezelSrcOuter` 0.984). Blit ID from Compare Notes table (480→420, 255→213, 180→148, 127→98); chrome annulus stretched onto that ring. Ticks/arcs at ID+1, arc thickness 50% of blit major. Original/no-border `ct=0`. Honor `H-analog-ct-crown` accepted.

33. **Escape codes on every string field — 2026-09-11.** Inspector Label/Units (gauges), switch `text`, and free-label `label` ingest via `EditTextToUtf8` (typed `\xNN`/`\xNNNN` plus Alt-code/ACP high bytes) and display via `Utf8ToEditText`. High-byte Alt-codes rewrite to `\x` in the box on apply (ASCII `\xb0` typing is not rewritten, so a trailing `\` is not turned into `\x5c`). Caption paint uses `Utf8ToWide` / `TextOutW` (analog/thin labels, switches, free labels; Digital/Bargraph/LED/Graph/Symbol already wide). Disk still Holley escapes. Round-trip: type → Save → Load → box and preview match.

32. **Positions screenshot — 2026-09-09.** Toolbar **Positions** writes a 1280×480 PNG like Screenshot, but with the blue selection ring on **every** `analogMeters` gauge (not switches or labels). Overlay for drawing layout backgrounds. Default name `layout-positions.png`.

31. **Channel picker — 2026-09-09; category column removed.** More settings **Ch** button (gauges only; switches stay integer — SSR is user I/O). One list plus a Find box. Find matches the channel name or the decimal id. `Holley-Channel-Picker.csv` is `channel,name` only: no category column and no `short_name`. The list is 490 px wide (the old category column, the gap, and the channel column). Integer box stays the source of truth; unknown ids (Bronco `2024`, `383`) survive Cancel. OK writes `channel=` only (no rename, no Use Defaults). CSV is embedded RCDATA (`IDR_CHANNEL_PICKER_CSV`). A loose file next to the EXE or the repo overrides it.

30. **Inspector enable/disable from Customize matrix — 2026-09-08.** Enable-only (seats stay put; no hide/collapse). `GaugeCustomizeHas` is `Dash-Customize-Control-Matrix.csv`. Type enables exactly the `X` rows, including Switch and free Label. Low/High zone checkboxes gate alarm/caution color pickers immediately; Normal picker is not gated. RPM has no Low/High checkboxes (high-side `c3`/`c4` stay on). Editor-only More settings kept: `bm` / `wfr` / `isLED` / raw `hor` (type 1) / `peakReductionEnabled` / raw `ct` (when Border Style combo is off). Switch Transparency writes `outputs` background alpha; Background RGB has no free chip (inactive/active occupy that row).

29. **Free-label caption is `label`, not `text` — 2026-09-07.** Holley `text` is the switch caption (`outputs\N\text=`). analogMeters never has `text`. Firmware has no labels objects. Writer emits `labels\N\label=`; old editor `labels\N\text=` still loads and is not written back.

28. **Averaging + Updates per second dropdowns — 2026-09-05.** More settings combos match the dash/firmware lists. `av` 0–8 = None…Very Fast. `updateRate` 1–10 = N/sec, 0 = full rate. Unknown values keep a raw extra item. Type gating matches Customize (no Averaging on LED/Symbol; no Updates on Graph/RPM/LED/Symbol).

27. **Select-then-drag + overlap menu — LOCKED 2026-09-02 (user accepted).** First left-click selects only. Drag arms on a later click of the already-selected widget (5px slop). Hit list is paint order (outputs, labels, gauges); Analog/Thin Analog use the outer circle. Right-click a stack of 2+ opens a pick menu; single-hit right-click selects.

26. **Symbol Latin metrics — LOCKED 2026-09-02 (user accepted).** Non-atlas characters (Bronco PRND321) used a 10×14 stub cell and a larger overflowing Arial. Arial at the same `fontSize` as the glyphs, GenericTypographic advance, cap-height cell (0.72×FS). Width: X-only squeeze to `size=`; height unchanged. `1` still pinched — do not reopen for that. Atlas PUA path unchanged.

25. **Analog silver blit asset — 2026-09-01.** `Proposed silverbezel.png` (405×405, opaque blue key) keyed to alpha and scaled to 480×480 to match the black blit canvas. Replaces `silverbezel-blackface-lg.png`. Same `ct=3` blit path; analog geometry not reopened. Source capture kept as Proposed.

24. **INI writer cleanup — 2026-09-01.** Stop writing `avFlag` (Holley has `av` only). Old editor files still load the key; it is not stored in `originalKeys` and is never written back. More-settings “Averaging enabled” checkbox removed; Averaging is the `av` combo (item 28). New gauges default `channel=41` (Battery); loaded channels are preserved. Booleans already write `true`/`false`. `rpmModeEnabled` still defaults `false` and is forced `false` on non–type-6.

23. **Style 2 trapezoid — LOCKED 2026-09-01 (P11 / `Compare.xmcd`); value/orient revised 2026-09-19 (items 42–44).** Not a type-1 card with a sloped top. Slope start = 50% of end. Label-off: 2px top + 8px bottom (end = H−10), high side at `pos.y+2`, **no spine**. Label-on: bar top = `BargraphBarOffset` (5+fs+9), 3px bottom, spine at `BargraphSpineOffset`. Spine follows the label only — `crown` ignored. Side inset 2px. Current value/orientation/dividers: items 42–44. Do not reopen casually.

22. **Status LED Label Font Size — LOCKED 2026-08-29 (user accepted close enough).** `Compare.xmcd` P5-led-fontsize: dash cap and LED drop track Font Size. Cap = `round(0.72×em)` (same as Digital). Label-on LED top = `pos.y+8+cap` (FS18 stays 8+13). AUTO em = 12 (`auto` is lowercase so cap ink is x-height; LED offset +4 vs FS8 is nearest FS12). LED clipped to the stored 44×63 / 90×109 box (dash clips the bottom at FS≥18). Art/tint unchanged. MathCAD 24 cap listed as 12 is the LED offset, not cap; 0.72×24=17 and LED +12 vs FS8 both match the offset ladder `round(0.72×(FS−8))`. Do not reopen for 1px cosmetics.

21. **RPM Bar mode + plate — LOCKED 2026-08-28.** Target RPM paint is `rpmModeEnabled` (type 6 only). `RpmBarIsShiftLight` no longer uses high-side thresholds. Writer and inspector persist the flag on type 6; other types still force `false`. Classic plate is **750×80** from `pos.y` (no vertical pad); INI/hit stay **750×100**. Target mode: no plate, hide unlit, center lit run. Setpoint/range stay in `minNormal`/`minCaution` (no new keys; not a mode switch). Lit-count vs setpoint parked.

20. **Font face stays Arial — CLOSED 2026-08-28, superseded 2026-09-19 (item 41).** Evaluated Holley `RobotoPlus-Regular.ttf` (Google 2013 + ISO2575 PUA), AOSP Roboto **v1.200310** (`Roboto-Regular-2013.ttf`), and Google Roboto **v2.138** (`Roboto-Regular.ttf`). Dash Latin, especially `1`, matches 2013 Roboto. Official v2.138 redesigned the `1` (worse). Item 41 ships the 2013 cut for preview Latin.

19. **Dash Customize nested interiors — catalogued 2026-08-28.** `Dash-Customize-Settings.md` now locks Label / Units / Warnings / Peak Hold / Sweep / Math / Averaging (photographed choices) / Precision / Border Style / Style 2 Size / color picker / Transparency / RPM Configuration / Target RPM (Enable off). Unknown INI keys left unmarked.

18. **Status LED label band (type 5) — LOCKED 2026-08-27, FS ladder 2026-08-29.** Label-off: LED top = `pos.y`. Label-on: cap top = `pos.y+8`; LED top = that + cap. FS18 is still 13 cap. See item 22.

17. **Digital plate width — LOCKED 2026-08-27.** Stacked-error fit from P2 MathCAD: 8px side pads (was 4+3) and value advances / 1.08 (Arial fat vs dash at the same cap). Units not scaled. Paint uses the same X squeeze so cap-height is unchanged.

16. **Digital value cap (type 2) — LOCKED 2026-08-27.** Unlabeled cap = round(0.714×H). Labeled = that minus (0.72×fontSize + 5). Cap bottom 4px above plate; label cap top 8px from plate top. Cap under 4px → no value (dash F labeled). `0` overshoot below 4px is Arial, not the pad.

15. **Bargraph card (type 1) — LOCKED 2026-08-29.** Thickness is not a fixed 52px. Card = 5px top + `fontSize` label band + 9px pad + **25px bar**. Spine (1px) + 2px hooks only when the label is on; hooks stop 6px short of the bar; 4px pad on each end of the length. Label-off still reserves the band. Scale stays outside. Vertical = 90° CCW. `NormalizeGaugeGeometry` writes that thickness. Dash crash at label 24+ is firmware — do not shrink the 25px bar.

## Implemented (prior)

1. **Symbol — Width and Height disabled.** Type 8 inspector: both disabled. Only **Font Size** sizes the glyph. AUTO is atlas 35, not Height.

2. **RPM bar — no preview caption; no RPM Mode.** Inspector still shows Label (writes `label=`). Type 6: `rpmModeEnabled` checkbox disabled; apply and writer always `false`.

3. **Style 2 — no Vertical / hor.** Superseded 2026-09-19 (item 42): editor Vertical is on; `hor` still type 1 only.

4. **Analog — bezel + scale font (first pass).** Sprite ring ~11px, outer ~size-6; Original ~8px. Then items 6–9 (stretch, no units, ticks, fonts).

5. **Thin Analog — diameter = size=.** Outer ring edge on `size=`; value bar constant 8px.

6. **Analog Small bezel** — sprite chrome band radially stretched onto the target annulus.

7. **Analog — no units paint** on type 0. Dash S0 (`displayUnits` mix) also paints none. Closed.

8. **Analog ticks** — majors `0.18×r`, minors `0.11×r`.

9. **Analog scale/value fonts (first pass)** — superseded by item 11.

10. **Analog zone arcs** — `barHeight = max(8, 0.20 × arc rMin)`. Outer rim `0.98`. Editor S0 bands now match dash thickness.

11. **Analog scale + value fonts** — scale `max(12, round(0.19 × boxR))` from `size=`; anchor `tick_inner − 0.9×scalePx`. Value `max(18, round(boxR / 4))`. Editor S0 numerals match dash size.

12. **Symbol `!` atlas cell** — `{ 0xF06C, 30, 28, 8, 35 }`. Editor S8 has a clean bang; four high-beam specks gone.

13. **RPM bar shift-light** — `RpmBarIsShiftLight` / `RpmBarFillRange`. Editor S6: Battery and Mode on are 7 centered greens; Open range is 15-LED with 7 lit; Alternator is 15-LED with 1 lit. Matches dash mode. Writer still forces `rpmModeEnabled=false`.

14. **Analog pixel-offset sizing (type 0) — LOCKED 2026-09-12; pen/needle revised 2026-09-19 (item 40).** `ct` selects blit geom even with Border off. Original/`ct=0`: OD=`size`, ID=`size-18`, no-border pie OD=`size-18`, ticks OD=`size-28`; major ink 20/15/10. Blit `ct>=1`: OD PNG×size=; ID 420/213/148/98; annulus stretch; ticks/arcs at ID+1; major ink 20/16/11; arc thickness 50%; ID cover stroke. Silver: `silverbezel-blackface-lg.png`. Grey shares black art. Current pen/needle: item 40. Do not reopen casually.

---

**Holley INI behavior:** use only `ProDashFirmware/internal/defaults/**/*.ini`. Samples, honor, CustomBackgrounds, ProDash Group, and editor Saves are not evidence of what Holley writes.

## Smoke INI

- **S0** — `displayUnits` mix. Dash and editor both omit analog units.
- **S6** — four-row shift-light test. Mode-on rows need `rpmModeEnabled=true`.
- **S8** — fontSize ladder + mixed glyphs. `!` cell is tight.
- **S11** — tall Style 2 with `orientation=false`.
- **Switches** — smoke only: dash enable needs a real output (this bench `ssr.1` = channel 11 in `SSR1-chan-test.ini`). Not an editor issue.
- **Honor pack** — `Samples/honor/H-*.ini`. Hidden-key and scale-on probes vs `Dash-Customize-Settings.md`. Load only; do not Save from the editor.
- **H-led-fontsize** / **P5-led-fontsize** — LED Label Font Size ladder. Locked (0.72 cap, AUTO=12, clip to box).
- **P6-rpm-mode** — overlay on P6. A both off; B `rpmModeEnabled=true`; C max thresh only (`rpmModeEnabled=false` → classic).
- **P6-rpm-mode check only** / **threshold chg** — dash-saved. Only B is target mode. Editor must not flip A/C.

---

## Open / next session

**Custom background generator — planning, not built.** See `Custom-Background-Generator.md`. Bake the gauge's configured ticks, colored arc bars, and scale values into a 1280×480 PNG (mostly analog, locked painter geometry). Wells are out of reach (2026-09-25): a well must blend into the user's background texture. Sidecar only; Holley gets a normal INI plus the PNG. Second program, not a mode in the shipping editor.

All nine create-gauge painters are locked.

**Symbol positioning — COMPLETE 2026-09-26.** Tom accepted the editor placement against the dash captures. Do not reopen. Do not replace the dash-capture atlas with Federal Register art; that set is incomplete and will not be used.

**Dropped 2026-09-26.** Ctrl+Tab gauge cycling: beta testers are satisfied with the selection flyout. Analog needle taper: tip radius is what matters, and it is solid. RPM bar lit count versus setpoint: nobody notices it. Do not reopen these.

**Analog LOCKED 2026-09-19.** Honor `H-analog-ct-crown` accepted. Do not reopen casually. Honor INI: load only.

**Style 2 LOCKED 2026-09-19.** Honor `H-style2-orient` accepted (orientation, dividers, value size/baseline). Do not reopen casually. Honor INI: load only.

**`wfr` proven 2026-09-19.** 0 = Steady warning color, 1 = Flash rate slow, 2 = Flash rate quick. More settings is a dropdown (item 47). Do not paint flash.

**Channel Math F to C (open).** No dedicated INI key found. Only `valueMult2` / `valueMult` / `valueMultB` (a, b, c). F to C radio with boxes still 0, 1, 0 is not a stored flag. Need a dash Save after F to C → Ok: either those three coeffs change, or a new key appears in `originalKeys`. Do not invent coefficients.

**Digital alpha:** background A as stored; fg A=0 skip text; zone A=0 skip warn plate; A≥1 opaque (item 46).

**Digital value:** dash cannot hide it. Editor always paints the number (item 46). Value display stays matrix-disabled for type 2.

**Peaks:** never render in the editor. Keep Min/Max peak in More settings only (load/save). Ignore `H-peaks.jpg`.

**Gauge Sweep presets — NEVER.** Dash 270/120/90 only fill start/end. Editor keeps numeric start/end. Do not add a preset dropdown.

Parked (not this next pass):
- **Custom channel DAT (sidecar)** — optional `.DAT` next to the EXE. On picker open, if present, append those user-defined channels to the embedded picker list. Does **not** reopen auto-resolve from `.terx` / `.dl` / pins (still closed). User authors the names. Format (id / name / category) not chosen yet. Integer box remains the escape hatch when the DAT is missing.
- **HDL import → Local Channels (parked 2026-09-13)** — user-picked Pro Dash `.hdl` could append dash-defined `io.N` names to the picker under **Local Channels** (not EFI duplicates; skip `[screenshot]`). HDL stores labels + `io.N`, not 20xx; Bronco evidence `io.N` = Dash Channel N (canonical 796+N / HEFI 2016+N). Do not rewrite `Holley-Channel-Picker.csv`. Same closed rule: no auto-resolve from `.terx` / `.dl` / pins. Unchosen: canonical vs HEFI id on write; session-only vs save beside EXE/layout. Overlaps the DAT sidecar (HDL would be a source for that list).
- **Warning-range non-integers (dash keypad)** — user 2026-09-12: dash sometimes accepts fractions on Low/High Alarm/Warning, sometimes not; no pattern. Editor-written non-integers were still honored on the unit. Keep inspector doubles (`minCaution` / `minNormal` / `maxNormal` / `maxCaution`). Do not integer-restrict. Revisit only if a pattern shows up.

**Inspector vs Customize — DONE 2026-09-08 (enable/disable only).** Matrix CSV had no remaining `?` cells. Inspector now gates from `GaugeCustomizeHas`. Hide/collapse not taken. Editor-only More settings kept (see item 30).

Parked from that pass:
- Hide unused inspector rows (holes vs greyed; collapse would be a layout rewrite).
- Switch Background Color RGB — no free chip next to inactive/active.
- RPM hide-outside-range still unproven (`isLED` candidate).
- Channel Math **F to C** — no key yet; need Save after Ok.

**RPM LED group counts — CLOSED 2026-09-12.** Editor uses `nnl` / `ncl` only (third band = remainder `15 − nnl − ncl`). Dash Configuration spinners can show other typed numbers; those are not stored and are not an editor model. No extra keys or UI. Keep the existing Norm LEDs / Caut LEDs fields.

**RPM Target setpoint/range UI — CLOSED 2026-09-05.** Keep the current implementation. Setpoint/range stay in existing `minNormal` / `minCaution` inspector fields (no dedicated Target RPM range UI, no new INI keys).

**LED Channel Math in preview — CLOSED 2026-09-05.** Will not paint Channel Math on Status LED in the editor preview. Math coefficients stay in More settings (`valueMult2` / `valueMult` / `valueMultB`). Dash Customize has no Channel Math row on LED.

**User-defined channel names — CLOSED 2026-09-07.** Will not try to resolve custom I/O (tune `.terx`, ECU `.dl`, pin B11, Dash Channel 20xx) to a stable name/number. Pin ≠ dash mailbox (2024 is Dash Channel 8, not B11). Preserve loaded `channel=` on edit; picker is the embedded Holley list. A user-authored sidecar `.DAT` (parked above) is the future way to *add* names, not auto-lookup.

Bargraph large-font dash crash (24+) is a firmware limit — do not shrink the editor 25px bar to match.

## Locked (do not reopen casually)

Analog (type 0) **LOCKED 2026-09-19**, **Bargraph (type 1)** card (label band + 25px bar, AUTO=12), **Digital (type 2)** cap-height / 4px+8px vertical / plate width (8px pads + 1.08), Thin Analog (type 4), **Status LED (type 5)** art/tint + label band (8+cap, AUTO=12, clip to box), **RPM Bar** mode=`rpmModeEnabled` + classic 80px plate (INI 100), Graph, **Symbol** atlas + Latin (2026-09-02), **Style 2 (type 11)** trapezoid + orientation + dividers + value (LOCKED 2026-09-19), Switches, Border, **selection** (select-then-drag + right-click stack menu, 2026-09-02), **font face (Roboto 2013)** — LOCKED 2026-09-19; Arial closed. Symbol atlas PUA unchanged.
