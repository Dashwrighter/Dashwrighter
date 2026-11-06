# Holley Pro Dash `gaugeType` enum

**Status:** All **nine** create-gauge types identified.  
**Last updated:** 2026-09-02 (Symbol Latin locked)

---

## Confirmed map (9 of 9)

| ID | Name | Confidence | Evidence / editor status |
|----|------|------------|--------------------------|
| **0** | **Analog** | Definitive | **Editor LOCKED 2026-09-19.** Fat majors 6/4/3; needle meets inner ticks; label/value center-anchored. `ct` blit geom without Border; blit ID table. Latin face Roboto 2013. |
| **1** | **Bargraph** | Definitive | **Editor LOCKED 2026-08-29.** Length-only Size; thickness = label band + 25px bar (`Compare Notes - Bargraph`). AUTO label=12. |
| **2** | **Digital** | Definitive | **Editor LOCKED 2026-08-27** (P2 vs `Compare.xmcd`). Size=H; value cap 0.714×H; labeled −(0.72×FS+5); W = 8px pads + advances/1.08. |
| **4** | **Thin Analog** | Definitive | **Editor LOCKED 2026-08-07** (`T4-thin-analog`). Open arc, no Border. |
| **5** | **Status LED** | Definitive | **Editor LOCKED 2026-08-29** (P5 / `Compare.xmcd`, user accepted close enough). LED top = `pos.y`; label-on +8+cap (`0.72×fontSize`, AUTO=12). Clipped to stored box. |
| **6** | **RPM Bar** | Definitive | **Editor LOCKED 2026-08-28.** Mode=`rpmModeEnabled`; classic plate 750×80 from `pos.y`; INI/hit 750×100; 15 LEDs / nnl/ncl. |
| **8** | **Symbol** | Definitive | **Editor LOCKED 2026-09-02.** Atlas blit + Latin fallback (0.72 cap, X-squeeze to `size=`). Sym picker shipped. |
| **9** | **Graph** | Definitive | **Editor LOCKED 2026-08-08** (`T9-graph`). Strip chart; synthetic zone trace. |
| **11** | **Bargraph Style 2** | Definitive | **Editor LOCKED 2026-09-19.** P11 trapezoid; `orientation` 90° CCW (H×H AABB; `hor` ignored); unlit solid / lit 1px bg gaps; value 0.24×endH on dash baseline. |

There are **only nine** gauge types in the Pro Dash **Add Gauge** menu. No tenth type.  
IDs **3, 7, 10** are unused in the create-menu set (do not invent types for them).

---

## Symbol type (`gaugeType=8`)

**Locked from:** field notes 2026-08-04 + dash Symbol keyboard / atlas capture.

### Behavior

- **Not images / bitmaps.** Symbol gauges draw **characters from the dash font set**.
- **Create** a Symbol-type gauge, then **change its appearance by renaming it** (the **`label`** holds the glyph).
- Glyphs are entered with the dash virtual keyboard’s **Sym** button (manual: two symbol keyboards; Shift toggles between them).
- **Bench power is enough** to create, rename, place, and **display** symbols. **No ECU required** for viewing or editing.
- Channel / warnings still apply at runtime (zone colors react to live values when data is present).
- **Glyph inventory + proposed names:** `Symbol-Glyph-Catalog.md` (~90+ keys; reference only).
- **Editor render:** atlas blit + scale + zone color. **No Border/bezel.** **Picker:** Sym button next to Label.

### INI encoding of the glyph (`label=`)

Export stores a **C-style hex escape** of a **Unicode Private Use Area** codepoint as ASCII text (not a raw multi-byte UTF-8 character in the file):

| INI text (literal) | Codepoint | TL16 column (dash PNG) |
|--------------------|-----------|-------------------------|
| `\xf095` | **U+F095** | Left — diamond / 4-point star |
| `\xf07a` | **U+F07A** | Center — high-beam style |
| `\xf096` | **U+F096** | Right — chevron / arrow |

- On disk the value is six ASCII bytes: `\`, `x`, then four hex digits (e.g. `f095`).
- Editor **reader** must decode `\xHH` / `\xHHHH` (and preserve unknown escapes) into the real code unit(s) for the model and for re-export.
- Editor **writer** should emit the same `\x….` form Holley uses so the dash font maps correctly.
- Do **not** assume plain ASCII label names for Symbol.

### Size = `fontSize` (confirmed TL16)

Visual size tracks **`fontSize`**. The dash also writes a growing **`size=`** hit/layout box:

| `fontSize` | Typical `size=` (TL16) |
|-----------:|------------------------|
| 35 | ~(23–34) × 50 |
| 45 | ~(28–43) × 64 |
| 55 | ~(34–51) × 77 |

Width varies slightly by glyph metrics; height steps with fontSize. Glyph draw size is driven by **fontSize**, not by treating `size=` as an image scale.

### Color = channel + warning zones

Glyph **color is not a free “paint” setting** on the main Customize list. It comes from the **channel value** evaluated against **Range / Warnings** (thresholds + zone colors `c0`–`c4`), same idea as Digital.

| Use case | Typical channel | Behavior |
|----------|-----------------|----------|
| Turn signals, brights, switches | Binary / discrete (often ~0..1 range) | Off → one zone color; on → another (depends on thresholds) |
| Oil pressure, temp, etc. | Floating / analog channel | Color steps through low alarm → low warn → normal → high warn → high alarm |

**TL16** used discrete channels **2024 / 2025 / 2026**, range **0..1**, `maxThresholdEnabled=true`, `minThresholdEnabled=false` — signal-style layouts (green / blue glyphs on the PNG).

### Symbol Customize settings (photo 2026-08-04)

Dash **Customize gauge** for a Symbol is **short** — only these rows (plus geometry/move outside this list):

| Dash UI | Example on photo | INI / model |
|---------|------------------|-------------|
| **Label** | High-beam glyph | `label` (escaped PUA glyph) |
| **Range** | `0..1` | `minValue` / `maxValue` |
| **Warnings** | `Max` (summary) | Threshold enables + zones + colors (`minThresholdEnabled`, …, `c0`–`c4`) |
| **Label Transparency** | `255` | Foreground **alpha** (opaque = 255) |
| **Channel Math** | `1*x+0` | `valueMult2` / `valueMult` / `valueMultB` |

**Not** on that five-row list (vs full analog/digital): Units, Peak Hold, Gauge Sweep, FG/BG color rows, Precision, Segments, Scale Values, Value Display, Border, Horizontal, etc. Channel is the usual channel picker (outside this list).

### Other fields from TL16 export

Common on all nine Symbol meters:

| Key | TL16 value | Notes |
|-----|------------|--------|
| `gaugeType` | **8** | Definitive |
| `displayLabel` | `false` | No separate caption; glyph *is* the content |
| `displayUnits` | `false` | |
| `units` | empty | |
| `precision` | `0` | |
| `background` | `4278190080` | Opaque black in this layout |
| `foreground` | `4294967295` | White base; runtime color from zones |
| `bm` | `false` | |
| `crown` | `true` | Present; no visible bezel on Symbol PNG (may be unused chrome) |
| **`ct`** | **`0`** | **New key** in export — meaning unknown; **preserve** on load/save |
| `numericIndicatorEnabled` | `true` | No numeric visible on Symbol PNG |

**Symbol model summary:**  
`gaugeType=8` + glyph `label` (`\x….` PUA) + `fontSize` (+ `size=` box) + channel + range + warning zones (colors) + label alpha + channel math.

---

## Not separate gauge types (demoted)

Names like **AFR Meter**, **TPI**, **GPS Map**, **G Meter** that appear in manuals or stock layouts are **not** distinct `gaugeType` values. They are ordinary gauges (typically **Digital** / type **2**) with a channel and a **label** (e.g. label text `"AFR Meter"`). Do not invent enum IDs for them.

| Common name | Actual form |
|-------------|-------------|
| AFR Meter | Digital (or other) gauge + label |
| TPI / gear text | Often **Symbol** (type **8**) or channel-driven layout art — not its own `gaugeType` |
| GPS Map / G Meter | Not in Add Gauge type list; not separate `gaugeType` IDs |

---

## Related non-gauge objects

| UI name | Storage |
|---------|---------|
| Soft switches | `outputs\N\…` (`style` map partial — see below) |
| Free labels | `labels\N\…` — **label, fontSize, foreground, background** (Transparency = background alpha; not a key). Firmware defaults have no labels objects. |
| Transmission Position Indicator | `transswitchpos\…` (parked; not an `analogMeters` type) |

---

## Soft-switch `style` (editor LOCKED 2026-08-07)

Full table + WidgetGraphics: **`Switch-Style-Map.md`**. All **17** menu styles (`style=` 0–16).

| style | Name |
|------:|------|
| 0–2 | Rocker, Slider, Rocker 2 |
| 3–4 | Black Toggle (+ small) |
| 5–6 | Silver Toggle (+ small) |
| 7–8 | Silver Momentary Switch (+ small) — same art as 5–6 |
| 9–11 | Gray Momentary Button small / mid / large |
| 12–14 | Black Momentary Button small / mid / large |
| 15–16 | Black Momentary Switch (+ small) — same art as 3–4 (pattern; dash switch-count limit) |

Preserve unknown IDs on load.

---

## Angle / reverse needle

There is **no reverse flag**. Needle position is:

`angle = startAngle + t * (endAngle - startAngle)` with `t` in \[0,1\].

If `endAngle < startAngle`, the needle travels the opposite direction (e.g. default5 GPS Speed `180→0` vs paired RPM `-180→0`). Editor preview already follows this.

---

## Firmware Add Gauge menu order (UI only — not enum IDs)

Photo evidence (Pro Dash create menu):

```
Analog
Bargraph
Bargraph Style 2
Digital
Thin Analog
RPM Bar
Symbol
Status LED
Graph
Cancel
```

Menu order ≠ numeric `gaugeType` IDs.

---

## Editor working set

```
0  Analog
1  Bargraph
2  Digital
4  Thin Analog
5  Status LED
6  RPM Bar
8  Symbol
9  Graph
11 Bargraph Style 2
```

**Editor work remaining for Symbol (not enum discovery):** decode/encode `label=\x….`, preview glyph at `fontSize` in zone color, preserve `ct`, round-trip INI. Faithful dash-font render / Sym picker UI remains low priority.
