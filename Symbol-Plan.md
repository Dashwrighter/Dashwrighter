# Symbol atlas / naming plan

Symbol **painter is locked**. Do not change `DrawSymbol`, scale (`fontSize/35`), zone tint, or hit box. This plan is atlas *content* and picker names only.

ISO 2575 PDF is a local reference. Do not copy its figures into `WidgetGraphics` or ship them. IHS/ISO: no reproduction without permission.

Do not incorporate `RobotoPlus-Regular.ttf` (no embed, no subset, no outline dump, no auto-trace of that font into shipped paths). Research copy only.

---

## 1. ISO naming — names from user review 2026-08-30

Picker / atlas names applied from `WidgetGraphics/symbols/Glyph compare comments.txt`. Dash-capture atlas restored to `TX-all_symbols.png`. Rows with no comment kept.

Rows 08 / 20 / 23 / 69 renamed 2026-08-30 (Parking brake, Battery shut-off, Fire Extinguisher, Emissions). The PDF is not a build input.

---

## 2. Own glyphs — not shipped

Original drawings remain in `WidgetGraphics/symbols/` for later. User preferred the dash-capture atlas; `TX-all_symbols.png` is that original again (copy also at `TX-all_symbols-dash-capture.png`).

---

## Not this plan

- Reopening Symbol paint, scale, or tint.
- Building an atlas from ISO PDF rasters.
- Embedding RobotoPlus.
- Analog silver blit and other parked gauge items.
