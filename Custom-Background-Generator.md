# Custom background generator

Planning only. Not built. Decided 2026-09-20. Wells dropped 2026-09-25.

Holley receives a normal layout INI plus one flattened 1280×480 PNG. Decorations are not dash keys. They live in an editor-only sidecar next to the layout. **Generate background** stamps that PNG and sets `Background=`. Moving a gauge makes the PNG stale until you generate again.

Development is a second program, not a mode in the shipping editor, so beta builds stay free of it. It loads an INI, previews with the existing painter, and writes the PNG. It does not have to save a Holley INI.

## What v1 bakes

Ticks, colored arc bars, and scale values, taken from the gauge as the user already configured it. Use the locked analog painter's geometry. Do not invent a second tick engine. Mostly analog. Another gauge type only if its painter already draws static marks that can sit behind the live part.

The needle, bar fill, digital value, and warn/alarm color stay live. Baked arc colors are frozen at generate time. Live ticks, arc, and scale must be turned off or the dash draws them twice.

## Out of reach

Wells. A well has to blend into the texture of the background the user already has. Do not draw wells, recessed plates, or chrome boxes for v1.

## Still standing from 2026-09-20

- Design-time overlay so the marks are visible before flatten. Generate is a command, not a mode.
- Layout mode places gauges. Decorate mode replaces the inspector for the selected gauge. The preview stays.
- Sidecar is per `analogMeters` index, with a type tag. Changing gauge type drops a decoration that no longer fits.
- PNG is always behind the live gauge. Each type has a mask. Warn when a baked mark sits entirely under opaque paint.
- Custom fonts are legal only at generate time (pixels in the PNG). Live gauges stay Roboto 2013.
