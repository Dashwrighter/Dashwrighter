# Dashwrighter

Windows editor for Holley EFI Pro Dash layout INI files (12.3" and 6.86").

Both dashes are 1280×480. A file saved here works on either unit.

**Warning:** A bad layout can misbehave on a real dash. Keep backups. Test on the unit before you rely on it.

## Download

Windows x64. One file, no installer:

[Dashwrighter.exe](https://github.com/Dashwrighter/Dashwrighter/releases/latest/download/Dashwrighter.exe)

The same file is on the [releases page](https://github.com/Dashwrighter/Dashwrighter/releases).

## Build and run

The solution is x64 only. Open `Dashwrighter.sln` in Visual Studio (toolset **v145** / VS 18) and build Debug or Release.

Release links the static C runtime (`/MT`). `x64\Release\Dashwrighter.exe` is the program: one file, and no support folder. Layout INIs and background images stay documents you open. Debug uses the dynamic debug runtime.

1. Build **Release | x64**.
2. Run `x64\Release\Dashwrighter.exe`.
3. **Load Layout**. Start with `Samples/placement/*.ini`. The gray background is `Samples/placement/internal/images/PlacementGray.jpg`.

Do not Save the files in `Samples/honor/` or `Samples/placement/` from the editor. They are reference layouts.

Headless check, no window:

```
Dashwrighter.exe --shot <layout.ini> <out.png>
```

That writes a 1280×480 PNG. The demo value is 12% of the way from min to max. Channel math is not applied, so a shot can read 12 while the dash photo of the same layout reads 11.

## What is in the EXE

A build embeds:

- Roboto Regular 1.200310 (2013), Apache 2.0 (`LICENSE-Roboto.txt`). Preview Latin uses this face. The inspector uses the system UI font.
- `Holley-Channel-Picker.csv` (`channel,name`).
- Bezel rings `blackbezel-blackface-lg.png` and `silverbezel-blackface-lg.png`. Grey uses the black ring.
- Symbol atlas `TX-all_symbols.png`, a dash capture. Symbol PUA glyphs come from this image.
- The 13 soft-switch sheets. Styles 0–16; some styles share a sheet. See `Switch-Style-Map.md`.

Status LEDs are drawn. They are not a PNG.

A loose file overrides the copy in the EXE. The program looks for `WidgetGraphics\<name>`, `Holley-Channel-Picker.csv`, and `Roboto-Regular-2013.ttf` next to the EXE and in parent directories. The JPG files in `WidgetGraphics/` are the drawings the PNG sheets were made from. The loader uses the PNGs.

## Layout

| Path | Role |
|------|------|
| `Dashwrighter/` | C++17 Win32 and GDI+ |
| `Samples/placement/` | Position and size INIs, flat gray background |
| `Samples/honor/` | Customize probes. Load only. |
| `Samples/dash-renders/` | Photos of the dash |
| `Samples/editor-renders/` | Editor shots of the same layouts |
| `WidgetGraphics/` | Switch sheets, bezel rings, symbol atlas, JPG masters |
| `Dash Dialogs/` | Customize screen photos |
| `GaugeType-Enum.md` | `gaugeType` ids |
| `Dash-Customize-Settings.md` | Customize rows per type |
| `Switch-Style-Map.md` | Soft-switch `style=` 0–16 |
| `Symbol-Codepoint-Map.md` | Glyph escapes, names, atlas |
| `Symbol-Glyph-Catalog.md` | Keyboard pages |
| `Holley-Channel-Picker.csv` | Names for the **Ch** button |
| `Holley-Channel-Map.csv` | Wider channel list |
| `Holley-Channel-Map.md` | Notes on that list |

```
Dashwrighter/
  main.cpp
  App/       Window, inspector, symbol picker, channel picker
  Model/     Layout document, gauge types, 1280×480 profile
  Io/        INI read/write, backgrounds, channel list
  Render/    Preview painters
```

Inspector control positions: `App/InspectorLayout.h`.

## Features

- Single- and multi-screen packs (`[Layout]` / `[LayoutN]` + `[Defaults]`)
- Types **0 Analog, 1 Bargraph, 2 Digital, 4 Thin Analog, 5 Status LED, 6 RPM Bar, 8 Symbol, 9 Graph, 11 Bargraph Style 2**. Types 3, 7, and 10 are unused.
- Soft switches, style 0–16. **Border** / **Border Style** (`crown` / `ct`) on analog faces. Border Style still selects the face when Border is off.
- Inspector under the 1280×480 preview. **More settings** for channel, math, rate, peaks, flags, and `hor`.
- **Preview** slider sets the demo value. Channel math and peak markers are stored and written. The preview does not draw them.
- **Sym** picker for Symbol gauges. **Ch** picker for gauge channels. Switches stay an integer. Find matches the channel name or the decimal id.
- **Screenshot** (no selection chrome). **Positions** (blue ring on every gauge).
- Click empty preview to clear selection. Inspector clicks do not.

Names like AFR Meter, TPI, GPS Map, and G Meter are ordinary gauges, usually Digital, with a label. They are not separate `gaugeType`s.

## License

Copyright (c) 2026 Tom. The program is [PolyForm Noncommercial 1.0.0](https://polyformproject.org/licenses/noncommercial/1.0.0). The text and the extra grant are in `LICENSE`.

You may use and change the program to make dash layouts, including for a customer. Those layout files are yours. You may keep them, give them away, or sell them. You may not sell the program or a modified program, charge for a copy, or fold the program into a commercial product. A customer receives the layout files only.

Roboto Regular 1.200310 is Apache 2.0. It is not covered by `LICENSE`. See `LICENSE-Roboto.txt`.

## Open

See `PENDING-CHANGES.md`.
