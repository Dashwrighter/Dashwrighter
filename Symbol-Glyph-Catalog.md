# Holley Pro Dash — Symbol keyboard glyph catalog

**Source (keyboard photos):** project root  
- `Holley Virtual Symbol Keyboard 1 Photo.jpg` — Sym page 1  
- `Holley Virtual Symbol Keyboard 2 Photo.jpg` — Sym page 2 (Shift)  
Originally photographed 2026-08-04; copies in-repo 2026-08-06.  

**Atlas export (full codepoints):** `TestLayouts/TX-all_symbols.ini` + `TestLayoutDashRenders/TX-all_symbols.png`  
Four Symbol gauges with multi-glyph `label=` strings — **91 unique** PUA escapes (`\xf0xx`), 92 total (one duplicate `\xf051`).  

**How accessed:** Gauge label edit → **Sym** → page 1; **Shift** toggles page 2 (two symbol pages).  
**Nature:** Font glyphs (not PNGs). Appearance of a Symbol-type gauge = characters placed in `label`.  
**Codepoints:** Known as a full set from `TX-all_symbols.ini` (ordered rows). Catalog names below remain by **page + row + column** until fully joined to escapes.  
**Dash settings (sparse):** Customize list is only **Label**, **Range**, **Warnings**, **Label Transparency**, **Channel Math** — see `GaugeType-Enum.md`.  
**Color:** from **channel vs warning/alarm zones** (same pattern as Digital), not a free static paint — binary idiot lights or analog channels.

Naming follows common automotive dashboard / ISO-style idiot-light conventions (oil can, battery, MIL, TPMS, etc.). Names in *italics* are best-effort where the icon is ambiguous or nonstandard.

Chrome keys (Esc, Clear, Caps, Shift, Enter, Space, Ok, arrows, Backspace, Sym) are **not** gauge symbols.

---

## Page 1 — primary symbols (Sym)

### Row 1 (top)

| # | Proposed name | Visual | Typical dash meaning |
|---|---------------|--------|----------------------|
| 1 | **General alert** | `!` | Generic attention / master warning |
| 2 | **Low beam** | Sideways headlamp, short rays | Headlights dipped / low beam on |
| 3 | **High beam** | Sideways headlamp, long rays | High beams on |
| 4 | **Front fog lamp** | Lamp + wave lines | Front fog lights |
| 5 | **Hazard / master warning** | Triangle | Hazard flashers or general warning |
| 6 | **Rear fog lamp** | Lamp in circle + waves | Rear fog light |
| 7 | **Information** | `i` in circle | Info message / service info |
| 8 | **Parking brake** | `P` in circle | Park brake applied |
| 9 | **Brake system** | Circle with `!` | Brake fault / low fluid / ABS-related brake |
| 10 | **Washer fluid** | Spray / reservoir icon | Low windshield washer fluid |
| 11 | **Battery** | Battery outline | Charging system / battery |
| 12 | **Battery charge** | Battery with `+` | Charging / battery positive / overcharge cue |

### Row 2

| # | Proposed name | Visual | Typical dash meaning |
|---|---------------|--------|----------------------|
| 1 | **Coolant temperature** | Thermometer in waves | Engine coolant over-temp |
| 2 | **Oil pressure** | Oil can | Low oil pressure |
| 3 | **Oil level** | Oil can + drop | Low oil level |
| 4 | **Washer fluid (alt)** | Bottle / reservoir with liquid | Washer fluid low (alt glyph) |
| 5 | ***Service cart / trolley*** | Wheeled cart | *Uncertain* — not a standard ISO idiot light; possible service / load / shop icon |
| 6 | **Battery (alt)** | Battery block | Battery / electrical (duplicate family) |
| 7 | **Battery disconnect / charge** | Battery with cable/plug | Battery connection / isolation |
| 8 | **Battery fail** | Battery with `X` | No charge / battery fault |
| 9 | **Warning triangle** | Triangle with `!` | General fault / caution |
| 10 | **Up / increase** | Up arrow | Directional / high / rise (generic) |
| 11 | ***Fluid bottle / spray*** | Upright bottle | *Uncertain* — chemical, additive, or fluid fill |
| 12 | **Bluetooth** | Bluetooth rune | Wireless / phone link |
| 13 | **Trailer** | Truck / trailer outline | Trailer connected / tow |

### Row 3 (after Caps)

| # | Proposed name | Visual | Typical dash meaning |
|---|---------------|--------|----------------------|
| 1 | **Cabin fan / HVAC blower** | Fan blades | Climate fan / blower |
| 2 | **Coolant circulation / A/C loop** | Circular arrows | Coolant flow or recirculation |
| 3 | **A/C / cold** | Snowflake | Air conditioning / max cold |
| 4 | **Transmission fault** | Gear with `!` | Gearbox / transmission warning |
| 5 | **Transmission fluid** | Gear with oil drop | ATF / gearbox oil |
| 6 | **Key / ignition** | Key | Key present / ignition |
| 7 | **Immobilizer / key fault** | Key with slash or `X` | Security / invalid key |
| 8 | **Fuel** | Fuel pump nozzle | Fuel system / level |
| 9 | **Fuel empty / fault** | Fuel pump with `X` | Empty or fuel system fault |
| 10 | ***Smart key / key fob*** | Key with pad/remote cue | Keyless entry / fob |

### Row 4 (between Shifts)

| # | Proposed name | Visual | Typical dash meaning |
|---|---------------|--------|----------------------|
| 1 | **Fuel pump** | Fuel pump | Fuel |
| 2 | **Fuel warning** | Fuel pump with `!` | Low fuel / fuel fault |
| 3 | **Fuel (alt)** | Fuel pump variant | Fuel (second drawing) |
| 4 | **Low fuel** | Fuel pump low/empty style | Low fuel |
| 5 | **Seat belt** | Occupant + belt | Fasten seat belt |
| 6 | **Airbag** | Seated figure + balloon | SRS / airbag system |
| 7 | **Airbag off** | Airbag with slash / off | Passenger airbag disabled |
| 8 | **Left** | Left arrow | Directional |
| 9 | **Right** | Right arrow | Directional |
| 10 | **Lane keep / lane marks** | Parallel lane lines | Lane departure / LKA cue |

**Page 1 symbol count (approx.):** 12 + 13 + 10 + 10 = **45** glyphs (excluding chrome).

---

## Page 2 — more symbols (Sym → Shift)

Left **Shift** is highlighted on the photo (page-2 mode).

### Row 1 (top)

| # | Proposed name | Visual | Typical dash meaning |
|---|---------------|--------|----------------------|
| 1 | **Check engine (MIL)** | Engine outline | Malfunction indicator lamp |
| 2 | **Engine fault** | Engine with `!` | Engine fault (emphasized) |
| 3 | **Engine** | Engine block | Engine / powertrain |
| 4 | ***Engine coolant / overheat (alt)*** | Engine with heat/water cue | *Engine thermal* family |
| 5 | **Glow plug / diesel preheat** | Coil / glow plugs | Diesel wait-to-start |
| 6 | **Emissions / exhaust** | Tailpipe + waves | Emissions / DPF / catalytic |
| 7 | **Power steering fault** | Steering wheel with `!` | EPS / hydraulic steering fault |
| 8 | **Steering** | Steering wheel | Steering system |
| 9 | **Electric power steering / assist** | Wheel with circular arrows | EPS active / assist |
| 10 | **Auto lights / light sensor** | Sun / rays | Automatic headlamps |
| 11 | **Exterior lights on** | Lamp with rays | Lights on reminder |
| 12 | **Heated rear window / defrost** | Grid or heat lines | Rear defroster |
| 13 | ***Sparkle / frost / wash*** | Burst / sparkles | *Uncertain* — ice, washer, or specialty lamp |

### Row 2

| # | Proposed name | Visual | Typical dash meaning |
|---|---------------|--------|----------------------|
| 1 | **Traction control (TC)** | `TC` badge | Traction control |
| 2 | **Traction control off** | `TC` with slash | TC disabled |
| 3 | **Traction control fault** | `TC` with `!` | TC fault |
| 4 | **Tire pressure (TPMS)** | Tire cross-section with `!` | Low tire pressure |
| 5 | **Tire pressure (alt / multi)** | Dual tire or TPMS variant | TPMS / tire set |
| 6 | **Oil pressure fault** | Oil drop / can with `!` | Oil pressure warning |
| 7 | **Washer spray** | Spray onto glass | Washer / wiper fluid action |
| 8 | **Stability control (ESC/ESP)** | Car with squiggle | Electronic stability control |
| 9 | **Stability control off** | ESC with slash / off | ESC disabled |
| 10 | **Stability / ESP (alt)** | Round ESP-style mark | ESC/ESP indicator |
| 11 | **Door ajar / central lock** | Door or lock cue | Door open / lock status |
| 12 | **Lane assist left** | Car + left lane mark | LDW / LKA left |
| 13 | **Lane assist right** | Car + right lane mark | LDW / LKA right |

### Row 3 (after Caps)

| # | Proposed name | Visual | Typical dash meaning |
|---|---------------|--------|----------------------|
| 1 | **Vehicle / car** | Side car silhouette | General vehicle |
| 2 | **Vehicle (alt)** | Car silhouette variant | Vehicle / body |
| 3 | **Door open (car)** | Car with open door | Door ajar |
| 4 | **Door open** | Door leaf | Door ajar (simple) |
| 5 | **Locked** | Padlock closed | Security locked |
| 6 | **Unlocked** | Padlock open | Unlocked |
| 7 | **Lock fault / security alert** | Padlock with `!` | Lock system fault |
| 8 | **Vehicle locked / alarm** | Car + lock | Anti-theft armed |
| 9 | **Vehicle unlocked / open** | Car open style | Unlocked / open |
| 10 | **Passenger airbag / child seat** | Child seat or airbag-off figure | Passenger airbag / child seat |
| 11 | **AUTO** | Text `AUTO` | Automatic mode (lights, climate, etc.) |

### Row 4 (between Shifts)

| # | Proposed name | Visual | Typical dash meaning |
|---|---------------|--------|----------------------|
| 1 | **Restart / cycle** | Circular arrows | Restart, refresh, or mode cycle |
| 2 | **Climate / blower off** | Fan with `X` or slash | HVAC off |
| 3 | **Manual / handbook** | Open book | Owner’s manual / info |
| 4 | **Power** | Power button | System power / ignition power |
| 5 | **Audio / speaker** | Speaker with waves | Sound / volume / media |
| 6 | **Phone** | Handset | Incoming / phone connected |
| 7 | **End call / phone alt** | Handset variant | Call end or second phone glyph |
| 8 | **Phone link / data** | Phone with arrows | Hands-free / data transfer |
| 9 | **Key fob** | Remote fob | Keyless remote |
| 10 | **Key fob fault / disabled** | Fob with `X` | Fob not detected / disabled |

**Page 2 symbol count (approx.):** 13 + 13 + 11 + 10 = **47** glyphs (excluding chrome).

**Combined unique-looking set:** ~**90+** keys; some families are intentional variants (fuel ×4, battery ×4, engine ×3, lock ×3, TC ×3).

---

## Grouped by function (for editor pickers later)

| Family | Page 1 | Page 2 |
|--------|--------|--------|
| Lighting | Low/high beam, fogs, lights | Auto lights, lights on, defrost |
| Brakes / park | Park `P`, brake `!` | — |
| Engine / oil / coolant | Coolant, oil, oil level | MIL, engine, glow, emissions, oil! |
| Electrical | Battery variants, Bluetooth | Power |
| Fuel | Fuel pumps (several) | — |
| Drivetrain | Gears, trailer | TC, ESC, TPMS |
| Safety | Seat belt, airbag | Airbag/child, doors, locks, lane |
| Climate | Fan, snowflake, circulation | Fan off |
| Steering | — | Steering / EPS |
| Cabin / UI | Info, arrows, `!` | Phone, speaker, book, AUTO, key fob |

---

## Confidence notes

| Item | Note |
|------|------|
| Classic ISO-style icons | High confidence names (oil can, battery, MIL, high beam, TPMS, seat belt, etc.) |
| Fuel / battery multiples | Same family, slight drawing differences — keep distinct catalog IDs until codepoints known |
| *Service cart*, *fluid bottle*, *sparkle* | Low confidence — confirm against manual or by inserting and exporting |
| Codepoints / UTF-8 in INI | ASCII `\xHHHH` PUA escapes. **Full set:** `TX-all_symbols.ini` (91 unique). Examples: `\xf095`→U+F095, `\xf07a`→U+F07A, `\xf096`→U+F096. Visual name ↔ codepoint join still open. |
| Holley vs road-car set | Set is OEM-dash flavored (TC, ESP, TPMS, glow plug) plus race/aftermarket extras (Bluetooth, trailer, generic arrows) |

---

## Priority

**Low / parked.** Catalog is reference only. Do **not** refine names or design a full Symbol picker UI yet — accurate editor render/selection of dash font glyphs may be hard or incomplete; placeholder + raw `label` is fine for now.

## Next steps

**Field TL16 — done** (`TestLayouts/TL16-Symbols.ini` + dash PNG):

1. ~~Create Symbol → export `gaugeType` + `label=`~~ → **`gaugeType=8`**, `label=\xf095` / `\xf07a` / `\xf096` (PUA).
2. ~~Vary **fontSize**~~ → 35 / 45 / 55; glyph size scales; **`size=`** box grows (~50 / 64 / 77 tall).
3. Optional later: full Sym-keyboard → codepoint map; PC-generated ladders with escape write path.

**Editor:** enum **8** + decode/encode `\x….` + zone-colored glyph preview (needed for fidelity). Faithful Holley-font picker/render remains **low priority**.

---

## Evidence files

| File | Content |
|------|---------|
| `Holley Virtual Symbol Keyboard 1 Photo.jpg` | Sym page 1 photograph (project root) |
| `Holley Virtual Symbol Keyboard 2 Photo.jpg` | Sym page 2 / Shift photograph (project root) |
| `TestLayouts/TX-all_symbols.ini` | All symbols as multi-glyph Symbol gauges (export) |
| `TestLayoutDashRenders/TX-all_symbols.png` | On-dash atlas screenshot |
| `GaugeType-Enum.md` | Symbol type **8**, encoding, size, color |
| `TestLayouts/TL16-Symbols.ini` | Early ladder (3 glyphs × 3 sizes) |
