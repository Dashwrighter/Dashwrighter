# Channel map

`Holley-Channel-Map.csv` lists each channel number and its name. The number is the layout INI `channel=` value. The file has 905 rows, from `-1` (Empty) through `926` (Injector Thermal Warning #8).

`0` is Invalid Channel. A live gauge set to it is a dash error. Dash SSR 1 is `817`. A bench `ssr.1=11` is Air Temp Enrichment (`11`), not Dash SSR 1.

Some older files write a different number for the same channel. `2011` in one Bronco layout is GPS Speed. The number in this list for GPS Speed is `789`.
