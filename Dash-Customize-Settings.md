# Dash Customize Settings — locked reference

**Status:** LOCKED from Pro Dash **Customize** dialog photographs in `Dash Dialogs/`. Nested-dialog interiors photographed and catalogued 2026-08-28.  
**Date:** 2026-08-28  
**Rule:** These photographs are the definitive list of settings a user can access per gauge type, switch, or label. Do not invent extra Customize rows. Do not hide a row the photos show.

This file is a **capability catalog** (what the dash offers). It is not a render-faithfulness spec and it does not change painter locks.

---

## Source photographs

| Widget | Files | Completeness |
|--------|-------|----------------|
| Analog (0) | `1 Analog 1.jpg` … `3.jpg` | Full list; last row = Channel Math |
| Bargraph (1) | `2 Bargraph 1.jpg` … `3.jpg` | Full list; last row = Channel Math |
| Bargraph Style 2 (11) | `3 Bargraph2 1.jpg` … `3.jpg` | Full list; last row = Channel Math |
| Digital (2) | `4 Digital 1.jpg`, `2.jpg` | Full list; last row = Channel Math |
| Thin Analog (4) | `5 Thin Analog 1.jpg` … `3.jpg` | Full list; last row = Channel Math |
| RPM Bar (6) | `6 RPMBar 1.jpg` | Full list (one screen) |
| Status LED (5) | `7 Status LED 1.jpg` | Full list (one screen) |
| Graph (9) | `8 Graph 1.jpg`, `2.jpg` | Full list; last row = Channel Math |
| Symbol (8) | `9 Symbol 1.jpg` | Full list (one screen) |
| Soft Switch | `Switch.jpg` | Full list (one screen) |
| Free Label | `Label.jpg` | Label, Foreground, Background, Transparency. Font Size is in the Label path (same as gauges/switches). Scrollbar after Transparency; no extra keys. |

Honor/layout captures (2026-09-19, car phone): `H-digital-alpha.jpg`, `H-digital-valueoff.jpg`, `H-analog-units.jpg`, `H-peaks.jpg`, `H-thin-scale.jpg`. RPM `wfr`/`av` Save: `Samples/honor/H-rpmbar-wfr-and-avg.ini`.

Nested interiors (2026-08-28; interiors filled 2026-09-19):

| Dialog | File | Notes |
|--------|------|-------|
| Label (gauge) | `Dialog - Label.jpg` | Analog; visible + text + font size |
| Label (Symbol) | `Dialog - Symbol Label.jpg` | text + font size; **no** visible checkbox |
| Label (Switch) | `Dialog - Switch label.jpg` | text + font size; **no** visible checkbox |
| Units | `Dialog - Units.jpg` | visible + unit string |
| Warnings | `Dialog - Warnings Alarms.jpg` | enables + four edges + zone colors; no Steady warning color on Analog |
| Peak Hold | `Dialog - Peaks.jpg` | Max / Min peak enable only |
| Gauge Sweep | `Dialog - Gauge sweep.jpg` | 270/120/90 presets + start/end |
| Channel Math | `Dialog - Channel math.jpg` | F to C / User-defined a,b,c |
| Averaging | `Dialog - Averaging.jpg` | dropdown; scrollbar so list may continue |
| Precision | `Dialog - Precision.jpg` | `1` / `1.2` / `1.23` / `1.234` |
| Border Style | `Dialog - Analog border style.jpg` | Original / Black / Grey / Silver |
| Size (Style 2) | `Dialog - Bargraph2 size.jpg` | Width + Height |
| Color picker | `Dialog - Color picker.jpg` | HSV / RGB / Alpha |
| Transparency (gauge) | `Dialog - Transparency.jpg` | slider; background-color alpha |
| Transparency (Label) | `Dialog - Label transparency.jpg` | numeric field (free Label widget) |
| RPM Configuration | `Dialog - RPM bar configuration.jpg` | range + LED groups + hide-outside-range + Steady warning color |
| Target RPM mode | `Dialog - RPM bar target rpm disabled.jpg`, `Dialog-TgtRPM-Enabled.jpg` | Enable on: setpoint + range + Steady warning color. Saved as `minNormal` / `minCaution`. |
| Bargraph Orientation | `Dialog - Bargraph vertical.jpg` | top-level list; summary `Vertical` |
| LED Size | `Dialog - LED size normal.jpg`, `… large.jpg` | top-level summaries only |

All gauge captures used the same live **Battery** channel (range 8.0 .. 20.0) unless noted.

---

## Reading rules

1. **Top-level rows** below are photo-proven, in dash order.
2. **Chrome** (Channel button / dropdown, Use Defaults, Ok) is on the dialog chrome, not in the scrolling list.
3. **Font Size is hidden** from the Customize list. It lives in the **Label** subdialog (photographed). AUTO is font size `-1`.
4. Nested interiors in the next section are photo-proven. Remaining gaps (Range, Target RPM with the checkbox on, Averaging items off-scroll) stay open. Free-label keys are locked: `label`, `fontSize`, `foreground`, `background` (Transparency = background alpha).
5. Position (drag on the layout canvas) is outside this dialog. Not catalogued here.
6. INI keys in the mapping column are the existing editor model names. They are a translation aid, not extra settings. Do not invent keys for photographed fields that have no known INI name.

---

## Dialog chrome (every gauge)

Present on every **Customize gauge** photo:

| Chrome | Notes | INI / model |
|--------|-------|-------------|
| **Channel button** (top right) | Shows current channel name (`Battery` in these shots). Opens the channel picker. | `channel` |
| **Use Defaults** | Restores factory defaults for this widget. Not an INI field. | — |
| **Ok** | Closes the dialog. | — |
| Live preview | Right-hand preview of the widget as settings change. | — |

**Customize Soft Switch** uses a **channel dropdown** (`SSR1`) instead of the Battery button, and has **no Use Defaults**.

**Customize label** (free label) has **no channel** and **no Use Defaults**.

---

## Nested dialog interiors

Tapping a summary row opens another screen (or, for LED Size / Orientation, the parent list just shows the chosen value). Interiors below are locked from the 2026-08-28 photos.

### Label (gauge) — `Dialog - Label.jpg`

Opened from Analog (Battery). Title **Label**. Copy: “The name or symbol for the gauge may be modified here. The default channel name is *Battery*.”

| Control | Photo | INI / model |
|---------|-------|-------------|
| **visible** checkbox | unchecked | `displayLabel` |
| **text** | `Battery` | `label` |
| **font size** | `-1` | `fontSize` (AUTO = `-1`) |
| Ok | | |

### Label (Symbol) — `Dialog - Symbol Label.jpg`

Same title and copy. **No visible checkbox** (the glyph *is* the widget).

| Control | Photo | INI / model |
|---------|-------|-------------|
| **text** | `Battery` | `label` (PUA glyph on Symbol) |
| **font size** | `35` | `fontSize` |
| Ok | | |

### Label (Switch) — `Dialog - Switch label.jpg`

Copy: “The name or symbol for the switch may be modified here.” **No visible checkbox.**

| Control | Photo | INI / model |
|---------|-------|-------------|
| **text** | `Switch` | `text` |
| **font size** | `20` | `fontSize` |
| Ok | | |

### Units — `Dialog - Units.jpg`

Copy: “Change the text displayed as the units. If you require conversion you will need to edit the values under Channel Math.”

| Control | Photo | INI / model |
|---------|-------|-------------|
| **visible** checkbox | checked | `displayUnits` |
| unit string | `V` | `units` |
| Ok | | |

### Warnings — `Dialog - Warnings Alarms.jpg`

Copy: “Adjust the warning set point and colors. Tap on the color bar to adjust the colors.”

| Control | Photo | INI / model |
|---------|-------|-------------|
| **Enable Low warnings** | unchecked | `minThresholdEnabled` |
| **Enable High warnings** | checked | `maxThresholdEnabled` |
| **Low Alarm** value | `8` | `minCaution` |
| **Low Warning** value | `10` | `minNormal` |
| **Normal** | no value box; thin color line | `c2` (tap color bar) |
| **High Warning** value | `16` | `maxNormal` |
| **High Alarm** value | `18` | `maxCaution` |
| Color bars (Low Alarm → High Alarm) | red / yellow / line / yellow / red | `c0`–`c4` |
| Ok | | |

Analog **Warnings** has no **Steady warning color** dropdown. That control is on RPM **Configuration** and **Target RPM mode** (below).

**Non-integer entry (user 2026-09-12, parked):** dash keypad sometimes accepts fractions on these value boxes and sometimes does not. No pattern yet (type, precision, channel, or which of the four edges). Editor-written non-integers in `minCaution` / `minNormal` / `maxNormal` / `maxCaution` were still honored on the unit. INI keys stay doubles. Do not integer-restrict the inspector. Do not invent a dash rule until a pattern exists.

### Peak Hold — `Dialog - Peaks.jpg`

| Control | Photo | INI / model |
|---------|-------|-------------|
| **Max peak enable** | checked | `maxPeakEnabled` |
| **Min peak enable** | checked | `minPeakEnabled` |
| Ok | | |

**Not in this dialog:** `peakReductionEnabled`. That key stays firmware-only.

### Gauge Sweep — `Dialog - Gauge sweep.jpg`

| Control | Photo | INI / model |
|---------|-------|-------------|
| Preset **270 deg** | button | writes `startAngle` / `endAngle` (values not photographed) |
| Preset **120 deg** | button | same |
| Preset **90 deg** | button | same |
| **start** | `-120` | `startAngle` |
| **end** | `120` | `endAngle` |
| Ok | | |

Do not guess what the presets write. The stored pair is the two numeric fields.

### Channel Math — `Dialog - Channel math.jpg`

Copy: “Use math to convert or scale the channel data.” Formula shown: `a*x^2+b*x+c`.

| Control | Photo | INI / model |
|---------|-------|-------------|
| **F to C** radio | off | preset; coefficients not photographed |
| **User-defined** radio | on | |
| **x^2 term (a)** | `0` | `valueMult2` |
| **Multiplier (b)** | `1` | `valueMult` |
| **Offset (c)** | `0` | `valueMultB` |
| Ok | | |

Do not invent the F to C coefficients from this photo.

### Averaging — `Dialog - Averaging.jpg`

Label: **Gauge Averaging Mode**. Dropdown open. Photographed choices, top to bottom:

`None` (selected), `Slow`, `180`, `Medium`, `120`, `90`, `60`, `Fast`, `Very Fast`

Scrollbar is visible — more items may exist off-screen. Encoding (`av` / `avFlag`) is **not** proven from the labels.

### Precision — `Dialog - Precision.jpg`

Copy: “Changes the displayed precision of values (the number of decimal places).”

Dropdown: `1` | `1.2` | `1.23` | `1.234` (selected `1.2`).

That is 0 / 1 / 2 / 3 decimal places. Matches integer INI `precision`. It is not a format-string editor.

### Border Style — `Dialog - Analog border style.jpg`

Copy: “Select the appearance of the border.” Dropdown:

| Choice | INI `ct` |
|--------|----------|
| Original | 0 |
| Black | 1 |
| Grey | 2 |
| Silver | 3 |

Photo has **Black** selected (Analog, Battery).

### Size (Style 2) — `Dialog - Bargraph2 size.jpg`

Copy: “Change the size of the gauge (in pixels).”

| Control | Photo | INI / model |
|---------|-------|-------------|
| **Width** | `200` | `width` |
| **Height** | `100` | `height` |
| Ok | | |

### Color picker — `Dialog - Color picker.jpg`

Opened from a Background Color `change...` row (title clipped to “…ground color”).

Photographed controls: Basic colors palette, Custom colors + **Add to Custom Colors**, HSV (**Hue** / **Sat** / **Val**), RGB (**Red** / **Green** / **Blue**), **Alpha channel**, swatch, OK / Cancel.

Applies to Foreground / Background / Bar Background / switch color rows. Alpha here is independent of the Transparency row.

### Transparency (gauge) — `Dialog - Transparency.jpg`

Copy: “Set the transparency (or alpha blending) of the **background color**.” Slider + spinner, 0 = transparent, 255 = opaque. Analog photo: `255`.

This is background alpha, not a whole-widget fade.

### Transparency (free Label) — `Dialog - Label transparency.jpg`

Title **Transparency**. Single numeric field (`32` in the photo). Preview is a free Label. Switch Transparency `change...` is the same kind of opener; Switch interior not separately photographed.

Symbol **Label Transparency** still shows a number on the parent list (`255`). Its interior was not photographed as its own file.

### RPM Configuration — `Dialog - RPM bar configuration.jpg`

Copy: “Tap on the values and colors below to adjust the range for each group of LEDs. You can also change the number of LEDs in each group.” Battery example:

| Control | Photo | INI / model |
|---------|-------|-------------|
| **Min** | `8` | `minValue` |
| **Warning** | `16` | `maxNormal` (high warning) |
| **Alarm** | `18` | `maxCaution` (high alarm) |
| **Max** | `20` | `maxValue` |
| Warning color bar | yellow | `c3` |
| Alarm color bar | red | `c4` |
| LED count under Min–Warning | `5` spinner | `nnl` (editor model) |
| LED count under Warning–Alarm | `5` spinner | `ncl` (editor model) |
| LED count under Alarm–Max | `5` **no spinner** | remainder (`15 − nnl − ncl`) |
| **Gauge is hidden if outside range** | unchecked | **unknown key** |
| **Steady warning color** dropdown | closed; selected **Steady warning color** | **unknown key** (`wfr` 0/1/2 is a candidate, not locked) |
| Ok | | |

No low-side alarm/warning columns on this dialog. Parent summary `8.20` is Min..Max.

LED group counts: the dash dialog lets you type the number of lights in each color band. Those typed numbers are **not** stored as themselves. User 2026-09-12, `CustomBackgrounds/Bronco II Test rpm bar.ini` **analogMeters\31** (type 6, `rpmModeEnabled=true`): set the two spinner values to **1000** and **2000**. Neither appears on gauge 31 (`2000` is nowhere in the file; the only `1000` is `scalingDivisor=1000`, also on analogMeters\2). Dash wrote `nnl=5` `ncl=5`. **Editor: `nnl` / `ncl` only** (third band = remainder). Do not invent keys or UI for the typed spinner numbers.

Dropdown choices (user 2026-09-12; list was closed in the photo): **Steady warning color**, **Flash rate slow**, **Flash rate quick**. Suspected: one setting for every warn/alarm zone. Non-steady: in a warn or alarm zone the gauge alternates between Normal (`c2`) and that zone’s color. Flash period unknown. **Do not mimic flash in the editor.**

### Target RPM mode — `Dialog - RPM bar target rpm disabled.jpg`

Copy: “Adjust the gauge warning ranges.” Checkbox **off** in this photo. Preview is the 15-LED bar.

| Control | Photo | INI / model |
|---------|-------|-------------|
| **Enable target RPM mode** | unchecked | `rpmModeEnabled` |
| **Target RPM setpoint** | `600` | **unknown key** |
| **Target RPM range** | `500` | **unknown key** |
| **Steady warning color** dropdown | closed; selected **Steady warning color** | same control as Configuration; **unknown key** |
| Ok | | |

Parent list summary is `disabled` when off. `Dialog - RPM bar target rpm enabled.jpg` is the **parent** list with the mode on: summary becomes **`configure`**, preview is the short centered green bar (shift-light). Interior with the checkbox **on** was not photographed — extra fields may appear.

**Only RPM Bar Customize offers this option** (no other type). The Enable checkbox is the mode switch (user 2026-09-12):

- **Off** (`rpmModeEnabled=false`) — classic **fixed-width** 15-LED bar (Configuration `nnl`/`ncl`/remainder).
- **On** (`rpmModeEnabled=true`) — bar **grows in width as RPM increases** (centered; no classic plate). Setpoint/range are not a second mode switch.

Writer still forces `rpmModeEnabled=false` on non–type-6. Do not reopen the P6 painter for this catalog note.

### Parent-list values now photographed (not nested interiors)

| Row | Photo | Values |
|-----|-------|--------|
| **Orientation** (Bargraph) | `Dialog - Bargraph vertical.jpg` | `Normal` (earlier list) / **`Vertical`** |
| **LED Size** | `Dialog - LED size normal.jpg`, `… large.jpg` | **`Normal`** / **`Large`** → `orientation` |

Orientation still maps to `orientation` (+ `hor` on Bargraph export). LED Size is still not a pixel Size row.

### Still unphotographed

| Row | Status |
|-----|--------|
| **Range** | Parent summary `8.0 .. 20.0` only. Interior not photographed. Existing model: `minValue` / `maxValue`. |
| **Style** (Switch) | Parent summary `Slider`. Interior of the 0–16 picker not re-shot. |

---

## Per-type catalogs (dash order)

Shared example values from the Battery captures are in parentheses. They are **not** defaults for every layout.

### Analog — `gaugeType=0`

Photos: `1 Analog 1–3.jpg`

| # | Dash UI | Example | INI / model |
|---|---------|---------|-------------|
| 1 | Label | Battery | `label` + `displayLabel` + `fontSize` (subdialog) |
| 2 | Units | V | `units` + `displayUnits` (subdialog) |
| 3 | Range | 8.0 .. 20.0 | `minValue` / `maxValue` (interior not photographed) |
| 4 | Warnings | Max | enables + four edges + `c0`–`c4` |
| 5 | Peak Hold | Min,Max | `minPeakEnabled` / `maxPeakEnabled` (no reduction) |
| 6 | Gauge Sweep | -120 .. 120 | `startAngle` / `endAngle` (+ 270/120/90 presets) |
| 7 | Foreground Color | change... | `foreground` (color picker) |
| 8 | Background Color | change... | `background` (color picker) |
| 9 | Transparency | 255 | background alpha (slider 0–255) |
| 10 | Precision | 1.2 | `precision` 0/1/2/3 decimals (`1` / `1.2` / `1.23` / `1.234`) |
| 11 | Segments | 3 | `steps` |
| 12 | Minor Tics | 1 | `mtics` |
| 13 | Scale values visible | Enabled | `scaleValues` |
| 14 | Scale Divisor | 1 | `scalingDivisor` |
| 15 | Size | 200 | square `size=` |
| 16 | Value Display | Enabled | `numericIndicatorEnabled` |
| 17 | Updates per second | 6 / sec | `updateRate` |
| 18 | Averaging | None | `av` / `avFlag` (named + numeric modes; encoding unknown) |
| 19 | Border | Enabled | `crown` |
| 20 | Border Style | Black | `ct` Original/Black/Grey/Silver. Editor Style combo stays enabled when Border is off (`ct` still paints). |
| 21 | Channel Math | 1 * x + 0 | F to C or `a*x^2+b*x+c` |

**Not on Analog:** Orientation, LED Size, Configuration, Target RPM mode, Bar Background, Label Transparency.

---

### Bargraph — `gaugeType=1`

Photos: `2 Bargraph 1–3.jpg`. Vertical summary: `Dialog - Bargraph vertical.jpg`.

| # | Dash UI | Example | INI / model |
|---|---------|---------|-------------|
| 1 | Label | Battery | `label` + `displayLabel` + `fontSize` |
| 2 | Units | V | `units` + `displayUnits` |
| 3 | Range | 8.0 .. 20.0 | `minValue` / `maxValue` |
| 4 | Warnings | Max | enables + four edges + `c0`–`c4` |
| 5 | Foreground Color | change... | `foreground` (color picker) |
| 6 | **Bar Background** | change... | `background` (dash name, not “Background Color”) |
| 7 | Transparency | 192 | background alpha |
| 8 | Precision | 1.2 | `precision` 0/1/2/3 decimals |
| 9 | Segments | 3 | `steps` |
| 10 | Minor Tics | 1 | `mtics` |
| 11 | Scale values visible | Enabled | `scaleValues` |
| 12 | Scale Divisor | 1 | `scalingDivisor` |
| 13 | Size | 200 | length only (scalar) |
| 14 | Value Display | Enabled | `numericIndicatorEnabled` |
| 15 | Updates per second | 6 / sec | `updateRate` |
| 16 | Averaging | None | `av` / `avFlag` (encoding unknown) |
| 17 | Border | Enabled | `crown` |
| 18 | Orientation | Normal / Vertical | `orientation` (+ `hor` on export) |
| 19 | Channel Math | 1 * x + 0 | F to C or `a*x^2+b*x+c` |

**Not on Bargraph:** Peak Hold, Gauge Sweep, Border Style, Background Color (uses Bar Background).

---

### Bargraph Style 2 — `gaugeType=11`

Photos: `3 Bargraph2 1–3.jpg`. Size interior: `Dialog - Bargraph2 size.jpg`.

| # | Dash UI | Example | INI / model |
|---|---------|---------|-------------|
| 1 | Label | Battery | `label` + `displayLabel` + `fontSize` |
| 2 | Units | V | `units` + `displayUnits` |
| 3 | Range | 8.0 .. 20.0 | `minValue` / `maxValue` |
| 4 | Warnings | Max | enables + four edges + `c0`–`c4` |
| 5 | Foreground Color | change... | `foreground` (color picker) |
| 6 | **Bar Background** | change... | `background` |
| 7 | Transparency | 192 | background alpha |
| 8 | Precision | 1.2 | `precision` 0/1/2/3 decimals |
| 9 | Segments | 3 | `steps` |
| 10 | Minor Tics | 1 | `mtics` |
| 11 | Scale values visible | Enabled | `scaleValues` |
| 12 | Scale Divisor | 1 | `scalingDivisor` |
| 13 | Size | **200 x 100** | Width + Height fields (`width` × `height`) |
| 14 | Value Display | Enabled | `numericIndicatorEnabled` |
| 15 | Updates per second | 6 / sec | `updateRate` |
| 16 | Averaging | None | `av` / `avFlag` (encoding unknown) |
| 17 | Channel Math | 1 * x + 0 | F to C or `a*x^2+b*x+c` |

**Not on Style 2:** Peak Hold, Gauge Sweep, Border, Border Style, Orientation.

---

### Digital — `gaugeType=2`

Photos: `4 Digital 1–2.jpg`

| # | Dash UI | Example | INI / model |
|---|---------|---------|-------------|
| 1 | Label | Battery | `label` + `displayLabel` + `fontSize` |
| 2 | Units | V | `units` + `displayUnits` |
| 3 | Range | 8.0 .. 20.0 | `minValue` / `maxValue` |
| 4 | Warnings | Max | enables + four edges + `c0`–`c4` |
| 5 | Foreground Color | change... | `foreground` (color picker) |
| 6 | Background Color | change... | `background` (color picker) |
| 7 | Transparency | 0 | background alpha |
| 8 | Precision | 1.2 | `precision` 0/1/2/3 decimals |
| 9 | Size | 50 | scalar height (`size=`) |
| 10 | Updates per second | 6 / sec | `updateRate` |
| 11 | Averaging | None | `av` / `avFlag` (encoding unknown) |
| 12 | Border | Enabled | `crown` (spine brackets) |
| 13 | Channel Math | 1 * x + 0 | F to C or `a*x^2+b*x+c` |

**Not on Digital:** Peak Hold, Gauge Sweep, Segments, Minor Tics, Scale values, Scale Divisor, Value Display, Border Style, Orientation.

---

### Thin Analog — `gaugeType=4`

Photos: `5 Thin Analog 1–3.jpg`

Same list as Analog **except there is no Border Style**.

| # | Dash UI | Example | INI / model |
|---|---------|---------|-------------|
| 1 | Label | Battery | `label` + `displayLabel` + `fontSize` |
| 2 | Units | V | `units` + `displayUnits` |
| 3 | Range | 8.0 .. 20.0 | `minValue` / `maxValue` |
| 4 | Warnings | Max | enables + four edges + `c0`–`c4` |
| 5 | Peak Hold | Min,Max | `minPeakEnabled` / `maxPeakEnabled` (no reduction) |
| 6 | Gauge Sweep | -120 .. 120 | `startAngle` / `endAngle` (+ 270/120/90 presets) |
| 7 | Foreground Color | change... | `foreground` (color picker) |
| 8 | Background Color | change... | `background` (color picker) |
| 9 | Transparency | 255 | background alpha |
| 10 | Precision | 1.2 | `precision` 0/1/2/3 decimals |
| 11 | Segments | 3 | `steps` |
| 12 | Minor Tics | 1 | `mtics` |
| 13 | Scale values visible | Enabled | `scaleValues` |
| 14 | Scale Divisor | 1 | `scalingDivisor` |
| 15 | Size | 200 | square `size=` |
| 16 | Value Display | Enabled | `numericIndicatorEnabled` |
| 17 | Updates per second | 6 / sec | `updateRate` |
| 18 | Averaging | None | `av` / `avFlag` (encoding unknown) |
| 19 | Border | Enabled | `crown` |
| 20 | Channel Math | 1 * x + 0 | F to C or `a*x^2+b*x+c` |

**Not on Thin Analog:** Border Style, Orientation, LED Size, Configuration, Target RPM mode.

---

### RPM Bar — `gaugeType=6`

Photo: `6 RPMBar 1.jpg`. Interiors: `Dialog - RPM bar configuration.jpg`, `Dialog - RPM bar target rpm disabled.jpg`. Parent with mode on: `Dialog - RPM bar target rpm enabled.jpg`.

| # | Dash UI | Example | INI / model |
|---|---------|---------|-------------|
| 1 | **Configuration** | 8.20 | Min/Warning/Alarm/Max + `nnl`/`ncl` + remainder; hide-outside-range unknown. LED spinner typed values (1000/2000) are not stored. Steady warning color: three choices known; INI unknown |
| 2 | **Target RPM mode** | `disabled` / `configure` | `rpmModeEnabled` (type 6 only). Off = fixed-width 15-LED; on = width grows with RPM. Setpoint `600` / range `500` unknown keys. Same Steady warning color dropdown. Interior with checkbox on not photographed. |
| 3 | Background Color | change... | `background` (color picker) |
| 4 | Transparency | 32 | background alpha |
| 5 | Averaging | None | `av` / `avFlag` (encoding unknown) |
| 6 | Channel Math | 1 * x + 0 | F to C or `a*x^2+b*x+c` |

**Not on RPM Bar Customize list:** Label, Units, Range (as its own row), Warnings (as its own row), Foreground Color, Precision, Segments, Minor Tics, Scale, Size, Value Display, Updates per second, Border, Border Style, Peak Hold, Gauge Sweep, Orientation.

Channel is still the top-right Battery button. Inspector Label (`label=`) is **not** a Customize row. Font Size, if any, would only exist via a Label path this dialog does not offer.

---

### Status LED — `gaugeType=5`

Photo: `7 Status LED 1.jpg`. Size summaries: `Dialog - LED size normal.jpg`, `Dialog - LED size large.jpg`.

| # | Dash UI | Example | INI / model |
|---|---------|---------|-------------|
| 1 | Label | Battery | `label` + `displayLabel` + `fontSize` |
| 2 | Range | 8.0 .. 20.0 | `minValue` / `maxValue` |
| 3 | Warnings | Max | enables + four edges + `c0`–`c4` |
| 4 | Foreground Color | change... | `foreground` (color picker) |
| 5 | **LED Size** | Normal / Large | `orientation` |

**Not on Status LED:** Units, Peak Hold, Gauge Sweep, Background Color, Transparency, Precision, Segments, Minor Tics, Scale, pixel Size, Value Display, Updates per second, Averaging, Border, Border Style, Channel Math, Orientation-by-that-name.

---

### Graph — `gaugeType=9`

Photos: `8 Graph 1–2.jpg`

| # | Dash UI | Example | INI / model |
|---|---------|---------|-------------|
| 1 | Label | Battery | `label` + `displayLabel` + `fontSize` |
| 2 | Units | V | `units` + `displayUnits` |
| 3 | Range | 8.0 .. 20.0 | `minValue` / `maxValue` |
| 4 | Warnings | Max | enables + four edges + `c0`–`c4` |
| 5 | Foreground Color | change... | `foreground` (color picker) |
| 6 | Background Color | change... | `background` (color picker) |
| 7 | Transparency | 0 | background alpha |
| 8 | Precision | 1.2 | `precision` 0/1/2/3 decimals |
| 9 | Segments | 3 | `steps` |
| 10 | Minor Tics | 1 | `mtics` |
| 11 | Scale values visible | Enabled | `scaleValues` |
| 12 | Scale Divisor | 1 | `scalingDivisor` |
| 13 | Size | **200 x 200** | free `width` × `height` |
| 14 | Averaging | None | `av` / `avFlag` (encoding unknown) |
| 15 | Channel Math | 1 * x + 0 | F to C or `a*x^2+b*x+c` |

**Not on Graph:** Peak Hold, Gauge Sweep, Value Display, **Updates per second**, Border, Border Style, Orientation.

---

### Symbol — `gaugeType=8`

Photo: `9 Symbol 1.jpg`

| # | Dash UI | Example | INI / model |
|---|---------|---------|-------------|
| 1 | Label | Battery (glyph via this row) | `label` PUA + `fontSize`; **no** visible checkbox |
| 2 | Range | 8.0 .. 20.0 | `minValue` / `maxValue` |
| 3 | Warnings | Max | enables + four edges + `c0`–`c4` (glyph color) |
| 4 | **Label Transparency** | 255 | glyph alpha (interior not separately photographed) |
| 5 | Channel Math | 1 * x + 0 | F to C or `a*x^2+b*x+c` |

**Not on Symbol:** Units, Peak Hold, Gauge Sweep, Foreground/Background Color rows, Precision, Segments, Minor Tics, Scale, Size, Value Display, Updates per second, Averaging, Border, Border Style, Orientation.

Channel is the top-right button. Color is zone-driven (Warnings), not a free FG picker.

---

### Soft Switch — `outputs`

Photo: `Switch.jpg`  
Title: **Customize Soft Switch**

| # | Dash UI | Example | INI / model |
|---|---------|---------|-------------|
| 1 | Label | ssr.1 | `text` + `fontSize`; **no** visible checkbox |
| 2 | Style | Slider | `style` (0–16); picker interior not re-shot |
| 3 | Inactive Label Color | change... | `inactiveColor` (color picker) |
| 4 | Active Label Color | change... | `activeColor` (color picker) |
| 5 | Background Color | change... | `background` (color picker) |
| 6 | Transparency | change... | numeric Transparency dialog (Switch interior not separately shot) |

Chrome: **channel dropdown** (`SSR1`), Ok. No Use Defaults.

**Not on Switch:** Range, Warnings, math, size (size comes from Style), Border, peaks, updates, averaging.

---

### Free Label — `labels`

Photo: `Label.jpg`  
Title: **Customize label**

| # | Dash UI | Example | INI / model |
|---|---------|---------|-------------|
| 1 | Label | Label | `label` + `fontSize` (Label subdialog, same pattern as gauges/switches) |
| 2 | Foreground Color | change... | `foreground` (color picker) |
| 3 | Background Color | change... | `background` (color picker) |
| 4 | Transparency | change... | numeric field (`Dialog - Label transparency.jpg`, value `32`); alpha of `background`, not its own key |

**Key set (locked):** `label`, `fontSize`, `foreground`, `background`. Firmware defaults have no `labels` objects. Caption key is `label` (not `text`; `text` is the switch caption). Transparency is encoded in `background`. Scrollbar after Transparency does not add keys.

No channel. No Use Defaults.

---

## Master matrix

Y = photo-proven Customize row (or chrome).  
— = not on that dialog.  
\* = different dash name (see notes).  
? = row exists; interior / extra rows not photographed.

| Setting | 0 Analog | 1 Bar | 11 Style2 | 2 Digital | 4 Thin | 6 RPM | 5 LED | 8 Symbol | 9 Graph | Switch | Label |
|---------|:--------:|:-----:|:---------:|:---------:|:------:|:-----:|:-----:|:--------:|:-------:|:------:|:-----:|
| Channel (chrome) | Y | Y | Y | Y | Y | Y | Y | Y | Y | Y* | — |
| Use Defaults | Y | Y | Y | Y | Y | Y | Y | Y | Y | — | — |
| Label | Y | Y | Y | Y | Y | — | Y | Y | Y | Y | Y |
| Units | Y | Y | Y | Y | Y | — | — | — | Y | — | — |
| Range | Y | Y | Y | Y | Y | ?¹ | Y | Y | Y | — | — |
| Warnings | Y | Y | Y | Y | Y | ?¹ | Y | Y | Y | — | — |
| Peak Hold | Y | — | — | — | Y | — | — | — | — | — | — |
| Gauge Sweep | Y | — | — | — | Y | — | — | — | — | — | — |
| Foreground Color | Y | Y | Y | Y | Y | — | Y | — | Y | — | Y |
| Background Color | Y | \*² | \*² | Y | Y | Y | — | — | Y | Y | Y |
| Bar Background | — | Y | Y | — | — | — | — | — | — | — | — |
| Label Transparency | — | — | — | — | — | — | — | Y | — | — | — |
| Transparency | Y | Y | Y | Y | Y | Y | — | — | Y | Y³ | Y³ |
| Precision | Y | Y | Y | Y | Y | — | — | — | Y | — | — |
| Segments | Y | Y | Y | — | Y | — | — | — | Y | — | — |
| Minor Tics | Y | Y | Y | — | Y | — | — | — | Y | — | — |
| Scale values visible | Y | Y | Y | — | Y | — | — | — | Y | — | — |
| Scale Divisor | Y | Y | Y | — | Y | — | — | — | Y | — | — |
| Size (scalar) | Y | Y | — | Y | Y | — | — | — | — | — | — |
| Size (W × H) | — | — | Y | — | — | — | — | — | Y | — | — |
| LED Size | — | — | — | — | — | — | Y | — | — | — | — |
| Value Display | Y | Y | Y | — | Y | — | — | — | — | — | — |
| Updates per second | Y | Y | Y | Y | Y | — | — | — | — | — | — |
| Averaging | Y | Y | Y | Y | Y | Y | — | — | Y | — | — |
| Border | Y | Y | — | Y | Y | — | — | — | — | — | — |
| Border Style | Y | — | — | — | — | — | — | — | — | — | — |
| Orientation | — | Y | — | — | — | — | — | — | — | — | — |
| Configuration | — | — | — | — | — | Y | — | — | — | — | — |
| Target RPM mode | — | — | — | — | — | Y | — | — | — | — | — |
| Channel Math | Y | Y | Y | Y | Y | Y | — | Y | Y | — | — |
| Style | — | — | — | — | — | — | — | — | — | Y | — |
| Inactive Label Color | — | — | — | — | — | — | — | — | — | Y | — |
| Active Label Color | — | — | — | — | — | — | — | — | — | Y | — |
| Font Size (hidden) | via Label | via Label | via Label | via Label | via Label | —⁴ | via Label | via Label | via Label | via Label | via Label |

Notes:

1. RPM has no Range / Warnings **rows**. Range, high-side Warning/Alarm, `nnl`/`ncl`, remainder, and **Steady warning color** live in **Configuration**. Target RPM mode is a separate dialog (`rpmModeEnabled` + unknown setpoint/range + the same Steady warning color dropdown).
2. Bargraph / Style 2 name the fill backdrop **Bar Background**, not Background Color.
3. Label and Switch show Transparency as `change...` (numeric dialog), not a 0–255 number on the list. Gauge Transparency is a background-alpha slider.
4. RPM Customize has no Label row, so no documented Font Size path.
5. Free-label Font Size is in the Label subdialog (`fontSize`). Not a fifth INI key.

---

## Size policy (from the Size row itself)

| Type | Size row shows | Meaning |
|------|----------------|---------|
| Analog, Thin Analog | one number (`200`) | Square. `width = height`. |
| Bargraph | one number (`200`) | Length only. Thickness is not user-set. |
| Digital | one number (`50`) | Single Size (height). Width is not a control. |
| Style 2 | `200 x 100` | Free width × height. |
| Graph | `200 x 200` | Free width × height. |
| Status LED | **LED Size** Normal / Large | Not a pixel Size row. |
| RPM, Symbol | no Size row | RPM fixed; Symbol sized by Font Size (Label subdialog). |
| Switch | no Size row | Footprint from Style. |
| Label | no Size row | Sized by Font Size. |

---

## Firmware keys with **no** Customize row

These exist in INI / the editor **More settings** panel. They are **not** user-facing on the photographed dialogs. Preserve on load/save. Do not present them as dash Customize settings.

| Key | Notes |
|-----|--------|
| `bm` | Face/background mode |
| `wfr` | Proven 2026-09-19: **0** Steady, **1** Flash rate slow, **2** Flash rate quick (`H-rpmbar-wfr-and-avg.ini`). true/false in older files = 1/0. |
| `isLED` / `setLEDEnabled` | Not Status LED type 5. Not proven as “Gauge is hidden if outside range.” |
| `hor` | Export companion to Bargraph Orientation; not its own dash row |
| `ct` except via Analog **Border Style** | Other types may still store `ct=0` |
| `peakReductionEnabled` | Not in Peak Hold. Firmware-only. |
| `x` / `y` | Canvas placement, not this dialog |

Nested, not top-level (do not treat as missing Customize settings):

| Key | Where |
|-----|--------|
| `displayLabel` / `fontSize` | Label subdialog (gauge). Symbol/Switch Label have font size, no visible. |
| `displayUnits` | Units subdialog |
| `nnl` / `ncl` | RPM Configuration (third count is remainder, no spinner) |
| `minPeakEnabled` / `maxPeakEnabled` | Peak Hold |
| `minThresholdEnabled` / `maxThresholdEnabled` + `minCaution` / `minNormal` / `maxNormal` / `maxCaution` + `c0`–`c4` | Warnings; RPM Configuration uses high-side only |

---

## Capture gaps (do not guess)

Closed 2026-09-19 (car photos in `Dash Dialogs/` + `H-rpmbar-wfr-and-avg.ini`):

- **Range** interior: min / max only (`Dialog-digital-range.jpg`).
- **Target RPM Enable on:** Enable checkbox, Target RPM setpoint, Target RPM range, Steady warning color (`Dialog-TgtRPM-Enabled.jpg`). Saved keys stay `minNormal` / `minCaution`.
- **Averaging:** user set Slow / Medium / Fast → `av` 1 / 2 / 3. Other menu rows may write the same int (untested). Editor combo 2=180 / 3=Medium is stale.
- **F to C** radio does not fill a,b,c (still 0, 1, 0 on `Dialog-FtoC-enabled.jpg`). No coefficient values to store.
- **Gauge Sweep** 270/120/90 only fill start/end on the dash (`Dialog-GaugeSweep1.jpg`: 270 → −135 / 135). Editor: numeric start/end only. **Never** a preset dropdown.
- **`wfr`:** 0 Steady, 1 Flash slow, 2 Flash quick.
- Switch **Style** picker: four pages (`Dialog-SwitchStyle-pg1.jpg` … `pg4.jpg`).
- Switch / Symbol **Transparency:** same “alpha blending of the background color” slider (`Dialog-SwitchTransparency1.jpg`, `Dialog-SymbolLabelTransparency.jpg`).

Still open:

1. RPM **Gauge is hidden if outside range** INI key unknown (`isLED` still a candidate, not locked).
2. **F to C** actual runtime coefficients (radio does not write 0,1,0 replacements; need a Save after Ok if the dash applies a hidden formula).
3. Warning-range **non-integers** (parked): keypad sometimes accepts fractions, sometimes not.
4. RPM Configuration LED-count spinners: typed values are not stored (`nnl` / `ncl` only).

---

## Disagreements vs existing editor / docs

Compared against `GaugeType.cpp` capability helpers, `EditorWindow_Props.cpp` inspector visibility, `GaugeType-Enum.md`, `README.md`, and `PENDING-CHANGES.md`.

### A. Dash offers it; editor currently hides or disables it

| Setting | Dash types | Editor today |
|---------|------------|--------------|
| **Minor Tics** | Analog, Bargraph, Style 2, Thin Analog, Graph | Only Analog (0) and Style 2 (11). `GaugeTypeUsesMtics` comment says Thin Analog ignores mtics. |
| **Scale values visible** | Analog, Bargraph, Style 2, **Thin Analog**, Graph | Thin Analog excluded (`GaugeTypeUsesScaleValues`). |
| **Scale Divisor** | Analog, Bargraph, Style 2, Thin Analog, Graph | Only Analog (`GaugeTypeUsesAnalogTicks` = type 0). |
| **Target RPM mode** | **RPM Bar only** (`disabled` / `configure`) | Inspector checkbox is type 6 only (2026-08-28). Setpoint / range stay in existing `minNormal` / `minCaution` fields (accepted 2026-09-05; no dedicated UI). |
| **RPM Configuration** | RPM Bar | No such composite row. Editor exposes Range / nnl / ncl / Warnings as separate inspector fields. Configuration is Min/Warning/Alarm/Max + LED group counts. |

### B. Editor shows it; dash Customize does not offer it for that type

| Setting | Editor shows on | Dash Customize |
|---------|-----------------|----------------|
| **Border** | All gauges except Symbol | Only Analog, Bargraph, Digital, Thin Analog. **Not** Style 2, Graph, RPM, LED, Symbol. |
| **Border Style (`ct`)** | Analog **and Thin Analog** (`GaugeTypeUsesBorderStyle`) | **Analog only.** Thin Analog has Border on/off, no Style. |
| **Value Display** | All except Symbol | Analog, Bargraph, Style 2, Thin Analog only. **Not** Digital, Graph, RPM, LED, Symbol. |
| **Units / Display units / Display label** | All except Symbol | No Units row on RPM, LED, Symbol. `displayLabel` / `displayUnits` live in Label / Units subdialogs. Symbol and Switch Label have **no** visible checkbox. |
| **Peak Hold** | All gauges (More settings) | Analog and Thin Analog only. |
| **Updates per second** | All gauges (More settings) | Analog, Bargraph, Style 2, Digital, Thin Analog. **Not** Graph, RPM, LED, Symbol. |
| **Averaging** | All gauges | All except LED and Symbol. |
| **Channel Math** | All gauges | All except **Status LED**. |
| **Transparency** | All gauges | All except **Status LED** (Symbol uses Label Transparency instead). |
| **Precision** | All gauges | Not RPM, LED, Symbol. |
| **RPM `rpmModeEnabled`** | Type 6 only (2026-08-28) | **Only** RPM Bar. Other types force `false` on write. |
| **RPM Label** | Type 6 inspector still shows Label | No Label row on RPM Customize. |
| **Norm LEDs / Caut LEDs** | Type 6 inspector | Nested in **Configuration** (`nnl` / `ncl`; third count is remainder). |
| **Peak reduction** | More settings (`peakReductionEnabled`) | Not in Peak Hold. |
| **Foreground Color** | All gauges | Not RPM, not Symbol. |
| **Background Color** | All gauges | Not LED, not Symbol. Bargraph/Style2 use **Bar Background**. |
| **`bm` / `wfr` / `isLED` / raw `hor` / raw `ct`** | More settings | Never a **proven** Customize row. `wfr` is a candidate for RPM Steady warning color (not locked). |
| **Width × Height** | Style 2 and Graph correctly free; others gated | Matches Size-row policy except editor still **displays** disabled W/H for RPM / LED / Symbol. |

### C. Documentation that the photos contradict

| Document | Claim | Photo |
|----------|-------|-------|
| `GaugeType-Enum.md` Digital | “no Border” | Digital **Border Enabled** |
| `GaugeType-Enum.md` Thin Analog | “no Border” | Thin Analog **Border Enabled** |
| `README.md` | “Border / Border Style on analog-family faces” | Border Style is **Analog only**. Border also on Digital and Bargraph. Thin Analog has Border, not Style. |
| `PENDING-CHANGES.md` / `ApplyGaugeTypeVisibility` | RPM Mode is not a dash toggle on type 6 | **Target RPM mode** is on the RPM dialog |
| `GaugeType.cpp` Thin Analog | mtics / scale values / scale divisor not used in UI | All three rows are on Thin Analog Customize |
| `GaugeType-Enum.md` Symbol | five-row list (Label, Range, Warnings, Label Transparency, Channel Math) | **Agrees** |

### D. Naming / structure mismatches (same capability, different UI)

| Dash | Editor |
|------|--------|
| Orientation (`Normal` / `Vertical`) on Bargraph only | `Vertical` checkbox (`orientation`) + separate `hor` in More settings |
| LED Size (`Normal` / `Large`) | `Size = Large` checkbox on `orientation` |
| Bar Background | generic Background |
| Label Transparency (Symbol) | generic Transparency slider |
| Configuration (RPM): Min/Warning/Alarm/Max + LED groups | split Range / LED counts / Warnings |
| Target RPM: Enable + setpoint + range | `rpmModeEnabled` checkbox only; setpoint/range unmodeled |
| Channel as a top-right button | Channel in More settings |
| Font Size only in Label subdialog (`-1` = AUTO) | Font Size on the main inspector |
| Precision `1` / `1.2` / `1.23` / `1.234` | integer `precision` |
| Gauge Transparency = background alpha slider | generic Transparency |
| Channel Math: F to C vs User-defined a,b,c | three coeff fields only |

### E. Agrees with existing understanding

- Nine Add-Gauge types; IDs 0, 1, 2, 4, 5, 6, 8, 9, 11.
- Style 2: free W×H, **no Vertical / hor**.
- Bargraph: scalar Size (length); Orientation on type 1 only.
- Analog / Thin Analog: square Size; Gauge Sweep.
- Digital: scalar Size; no Segments / scale / Value Display row.
- Status LED: Normal/Large only; no pixel Size.
- Symbol: short list; no Units, Border, Value Display, Size row; Font Size via Label.
- Switch: Label, Style, inactive/active colors, background, transparency; size from Style.
- Channel picker is chrome, not a list row.
- Analog Border Style: Original / Black / Grey / Silver (`ct` 0–3).
- Bargraph Orientation includes **Vertical** (photographed).
- Status LED Size includes **Large** (photographed).
- Peak Hold is Max/Min enable only; no reduction checkbox.
- Label visible lives in the Label subdialog; Units visible lives in the Units subdialog.

---

## Lock statement

Top-level Customize rows **and** the nested interiors photographed in `Dash Dialogs/` are **fixed**. Remaining gaps: Range interior, Target RPM with Enable on, Averaging off-scroll items, free-Label rows below Transparency, and the unknown INI keys listed in Capture gaps.

Inspector work should converge on this matrix (hide what the dash does not offer per type; show what it does). Painter locks are independent of this file.
