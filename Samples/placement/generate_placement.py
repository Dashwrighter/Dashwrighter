#!/usr/bin/env python3
"""Generate the placement/size test INI pack + flat gray JPG.

Run from this folder:  python generate_placement.py
Do not round-trip the INIs through the editor Save (writer may rewrite W / flags).
"""

from __future__ import annotations

from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parent
IMG_DIR = ROOT / "internal" / "images"
CANVAS_W, CANVAS_H = 1280, 480
GAP = 8
BG_NAME = "PlacementGray.jpg"
BG_PATH_INI = f"/internal/images/{BG_NAME}"
BG_RGB = (196, 196, 196)  # flat light gray

# Holley AARRGGBB as unsigned decimal
def argb(a: int, r: int, g: int, b: int) -> int:
    return ((a & 255) << 24) | ((r & 255) << 16) | ((g & 255) << 8) | (b & 255)


PLATE = argb(255, 0, 0, 0)          # opaque black — render area
FG = argb(255, 255, 255, 255)
C0 = argb(255, 220, 0, 0)           # low alarm
C1 = argb(255, 220, 180, 0)         # low warn
C2 = argb(255, 0, 180, 0)           # normal
C3 = argb(255, 220, 180, 0)         # high warn
C4 = argb(255, 220, 0, 0)           # high alarm
SW_INACTIVE = argb(255, 160, 160, 160)
SW_ACTIVE = argb(255, 255, 255, 255)

CHANNEL = 41  # Battery — live enough for needles/arcs; switches will not enable


def bool_str(v: bool) -> str:
    return "true" if v else "false"


def write_jpg() -> None:
    IMG_DIR.mkdir(parents=True, exist_ok=True)
    img = Image.new("RGB", (CANVAS_W, CANVAS_H), BG_RGB)
    img.save(IMG_DIR / BG_NAME, "JPEG", quality=92, optimize=True)


def overlap(a, b) -> bool:
    ax, ay, aw, ah = a
    bx, by, bw, bh = b
    return ax < bx + bw and ax + aw > bx and ay < by + bh and ay + ah > by


def check_boxes(name: str, boxes: list[tuple[int, int, int, int]]) -> None:
    for i, (x, y, w, h) in enumerate(boxes):
        if w < 1 or h < 1:
            raise SystemExit(f"{name}: box {i} empty {x,y,w,h}")
        if x < 0 or y < 0 or x + w > CANVAS_W or y + h > CANVAS_H:
            raise SystemExit(f"{name}: box {i} off-canvas {x,y,w,h}")
        for j in range(i + 1, len(boxes)):
            if overlap((x, y, w, h), boxes[j]):
                raise SystemExit(f"{name}: overlap {i} {x,y,w,h} vs {j} {boxes[j]}")


def write_gauge(lines: list[str], idx: int, g: dict) -> None:
    p = f"analogMeters\\{idx}\\"
    x, y = g["x"], g["y"]
    w, h = g["w"], g["h"]
    lines.append(f"{p}label={g['label']}")
    lines.append(f"{p}fontSize={g['fontSize']}")
    lines.append(f'{p}pos="({x},{y})"')
    lines.append(f'{p}size="({w},{h})"')
    lines.append(f"{p}units={g['units']}")
    lines.append(f"{p}precision={g['precision']}")
    lines.append(f"{p}minValue={g['minValue']}")
    lines.append(f"{p}maxValue={g['maxValue']}")
    lines.append(f"{p}scalingDivisor={g['scalingDivisor']}")
    lines.append(f"{p}minCaution={g['minCaution']}")
    lines.append(f"{p}maxCaution={g['maxCaution']}")
    lines.append(f"{p}minNormal={g['minNormal']}")
    lines.append(f"{p}maxNormal={g['maxNormal']}")
    lines.append(f"{p}minThresholdEnabled={bool_str(g['minThresholdEnabled'])}")
    lines.append(f"{p}maxThresholdEnabled={bool_str(g['maxThresholdEnabled'])}")
    lines.append(f"{p}minPeakEnabled=false")
    lines.append(f"{p}maxPeakEnabled=false")
    lines.append(f"{p}numericIndicatorEnabled={bool_str(g['numericIndicatorEnabled'])}")
    lines.append(f"{p}peakReductionEnabled=false")
    lines.append(f"{p}channel={g['channel']}")
    lines.append(f"{p}foreground={g['foreground']}")
    lines.append(f"{p}background={g['background']}")
    lines.append(f"{p}startAngle={g['startAngle']}")
    lines.append(f"{p}endAngle={g['endAngle']}")
    lines.append(f"{p}steps={g['steps']}")
    lines.append(f"{p}mtics={g['mtics']}")
    lines.append(f"{p}crown={bool_str(g['crown'])}")
    lines.append(f"{p}rpmModeEnabled={bool_str(g['rpmModeEnabled'])}")
    lines.append(f"{p}gaugeType={g['gaugeType']}")
    lines.append(f"{p}displayUnits={bool_str(g['displayUnits'])}")
    lines.append(f"{p}displayLabel={bool_str(g['displayLabel'])}")
    lines.append(f"{p}isLED=false")
    lines.append(f"{p}valueMult2=0")
    lines.append(f"{p}valueMult=1")
    lines.append(f"{p}valueMultB=0")
    lines.append(f"{p}scaleValues={bool_str(g['scaleValues'])}")
    lines.append(f"{p}orientation={bool_str(g['orientation'])}")
    lines.append(f"{p}updateRate=6")
    lines.append(f"{p}c0={g['c0']}")
    lines.append(f"{p}c1={g['c1']}")
    lines.append(f"{p}c2={g['c2']}")
    lines.append(f"{p}c3={g['c3']}")
    lines.append(f"{p}c4={g['c4']}")
    lines.append(f"{p}wfr=false")
    lines.append(f"{p}bm=false")
    lines.append(f"{p}hor={bool_str(g['hor'])}")
    lines.append(f"{p}nnl={g['nnl']}")
    lines.append(f"{p}ncl={g['ncl']}")
    lines.append(f"{p}av=0")
    lines.append(f"{p}avFlag=false")
    lines.append(f"{p}ct={g['ct']}")


def write_output(lines: list[str], idx: int, o: dict) -> None:
    p = f"outputs\\{idx}\\"
    lines.append(f"{p}text={o['text']}")
    lines.append(f'{p}pos="({o["x"]},{o["y"]})"')
    lines.append(f"{p}channel={o['channel']}")
    lines.append(f"{p}style={o['style']}")
    lines.append(f"{p}inactiveColor={o['inactiveColor']}")
    lines.append(f"{p}activeColor={o['activeColor']}")
    lines.append(f"{p}background={o['background']}")
    lines.append(f"{p}fontSize={o['fontSize']}")


def write_ini(name: str, gauges: list[dict], outputs: list[dict] | None = None) -> None:
    outputs = outputs or []
    boxes = [(g["x"], g["y"], g["w"], g["h"]) for g in gauges]
    boxes += [(o["x"], o["y"], o["w"], o["h"]) for o in outputs]
    check_boxes(name, boxes)

    lines = [
        "[Layout]",
        f"Background={BG_PATH_INI}",
        "v=10",
    ]
    for i, g in enumerate(gauges, 1):
        write_gauge(lines, i, g)
    lines.append(f"analogMeters\\size={len(gauges)}")
    lines.append("labels\\size=0")
    for i, o in enumerate(outputs, 1):
        write_output(lines, i, o)
    lines.append(f"outputs\\size={len(outputs)}")
    text = "\r\n".join(lines) + "\r\n"
    (ROOT / name).write_bytes(text.encode("ascii"))


def base_gauge(**kw) -> dict:
    g = {
        "label": "A",
        "fontSize": 18,
        "units": "",
        "precision": 0,
        "minValue": 0,
        "maxValue": 100,
        "scalingDivisor": 1,
        "minCaution": 10,
        "maxCaution": 90,
        "minNormal": 25,
        "maxNormal": 75,
        "minThresholdEnabled": False,
        "maxThresholdEnabled": False,
        "numericIndicatorEnabled": True,
        "channel": CHANNEL,
        "foreground": FG,
        "background": PLATE,
        "startAngle": -120,
        "endAngle": 120,
        "steps": 8,
        "mtics": 0,
        "crown": False,
        "rpmModeEnabled": False,
        "gaugeType": 0,
        "displayUnits": False,
        "displayLabel": False,
        "scaleValues": False,
        "orientation": False,
        "hor": False,
        "nnl": 5,
        "ncl": 5,
        "ct": 0,
        "c0": C0,
        "c1": C1,
        "c2": C2,
        "c3": C3,
        "c4": C4,
        "x": 0,
        "y": 0,
        "w": 100,
        "h": 100,
    }
    g.update(kw)
    return g


def letter(i: int) -> str:
    return chr(ord("A") + i)


def digital_width(height: int, units: bool, label: bool, crown: bool) -> int:
    # Editor fallback when GDI+ measure is unavailable (DigitalLayout.cpp).
    # Sweep is height; W is a side-effect so the INI looks like a dash export.
    if height < 20:
        height = 20
    digits = 3  # 0..100
    aspect = 0.40 + 0.34 * digits
    if units:
        aspect += 0.12 + 0.06 * 3  # "psi"
    aspect = max(1.05, min(3.8, aspect))
    w = int(round(height * aspect))
    if label:
        hooks = 12 if crown else 0
        w = max(w, 8 + hooks + 12 + 24)
    return max(40, w)


# ---------------------------------------------------------------------------
# Layouts (positions identical across sibling flag files)
# ---------------------------------------------------------------------------

ANALOG = [
    # size, x, y  — 480 origin/flush-bottom; 121 flush-right
    (480, 0, 0),
    (255, 488, 0),
    (180, 751, 0),
    (127, 751, 188),
    (121, 1159, 0),
]

THIN = [
    (480, 0, 0),
    (255, 488, 0),
    (180, 751, 0),
    (127, 939, 0),
    (80, 1200, 400),   # flush bottom-right
    (40, 1240, 0),     # flush right
]

DIGITAL_H = [480, 255, 160, 89, 50, 28]


def digital_boxes(units: bool, label: bool, crown: bool) -> list[tuple[int, int, int, int]]:
    """Fixed positions (packed for units-on widths). Per-file W still follows flags."""
    # Positions chosen from the widest (units on, no label) so siblings never collide.
    w480 = digital_width(480, True, False, False)
    w255 = digital_width(255, True, False, False)
    w160 = digital_width(160, True, False, False)
    w89 = digital_width(89, True, False, False)
    w50 = digital_width(50, True, False, False)
    w28 = digital_width(28, True, False, False)
    # Recompute actual W for this flag combo; keep x,y from the wide pack.
    pack = [
        (0, 0, 480),
        (CANVAS_W - w255, 0, 255),
        (CANVAS_W - w255, 255 + GAP, 160),
        (CANVAS_W - w255 + w160 + GAP, 255 + GAP, 89),
        (CANVAS_W - w255, CANVAS_H - 50, 50),
        (CANVAS_W - w28, 255 + GAP + 89 + GAP, 28),
    ]
    out = []
    for x, y, h in pack:
        w = digital_width(h, units, label, crown)
        # Keep left/top; allow W to shrink. If W grew past pack slot, still check later.
        out.append((x, y, w, h))
    return out


BAR_H = [
    # w, h, x, y
    (1200, 52, 0, 0),
    (80, 52, 1200, 0),
    (750, 52, 0, 60),
    (400, 52, 0, 120),
    (255, 52, 0, 180),
    (127, 52, 0, CANVAS_H - 52),  # flush bottom
]

BAR_V = [
    # w, h, x, y  — length in height; card W factory-like 101
    (101, 400, 0, 0),
    (101, 255, 109, 0),
    (101, 180, 218, 0),
    (101, 121, 327, 0),
    (101, 400, CANVAS_W - 101, CANVAS_H - 400),  # flush right+bottom
    (101, 121, 436, CANVAS_H - 121),             # flush bottom
]

STYLE2 = [
    # w, h, x, y
    (1192, 60, 0, 0),                                 # wide-short
    (80, 400, CANVAS_W - 80, 0),                      # tall-narrow, flush right
    (255, 255, 0, 68),
    (180, 127, 263, 68),
    (64, 64, 451, 68),
    (40, 121, 523, 68),                               # odd
    (400, 149, 0, CANVAS_H - 149),                    # flush bottom
    (121, 72, CANVAS_W - 121, CANVAS_H - 72),         # flush right+bottom
]

GRAPH = [
    (600, 200, 0, 0),
    (400, 120, 608, 0),
    (255, 80, 608, 128),
    (127, 255, 0, 208),
    (264, 264, CANVAS_W - 264, 0),
    (121, 127, CANVAS_W - 121, CANVAS_H - 127),
]

LED = [
    # w, h, x, y, large
    (44, 63, 0, 0, False),
    (44, 63, CANVAS_W - 44, 0, False),
    (44, 63, 0, CANVAS_H - 63, False),
    (44, 63, 200, 200, False),
    (90, 109, 60, 0, True),
    (90, 109, CANVAS_W - 90, CANVAS_H - 109, True),
]

RPM = [
    (750, 100, 0, 0),
    (750, 100, CANVAS_W - 750, 108),          # flush right
    (750, 100, 0, CANVAS_H - 100),            # flush bottom
]

SYMBOL = [
    (480, 0, 0),
    (255, 488, 0),
    (180, 751, 0),
    (127, 751, 188),
    (80, 886, 188),
    (40, 886, 276),
    (24, CANVAS_W - 24, CANVAS_H - 24),
]

SWITCHES = [
    # style, w, h, x, y
    (4, 80, 80, 0, 0),
    (4, 80, 80, CANVAS_W - 80, 0),
    (4, 80, 80, 0, CANVAS_H - 80),
    (3, 150, 150, 100, 0),
    (3, 150, 150, CANVAS_W - 150, CANVAS_H - 150),
]


def analog_gauges(crown: bool, ct: int, label_on: bool) -> list[dict]:
    out = []
    for i, (side, x, y) in enumerate(ANALOG):
        out.append(base_gauge(
            gaugeType=0,
            label=letter(i),
            fontSize=18,
            units="V",
            x=x, y=y, w=side, h=side,
            crown=crown, ct=ct,
            displayLabel=label_on,
            displayUnits=False,
            scaleValues=False,
            steps=8, mtics=1,
            minThresholdEnabled=True,
            maxThresholdEnabled=True,
            numericIndicatorEnabled=True,
        ))
    return out


def thin_gauges(crown: bool, label_on: bool) -> list[dict]:
    out = []
    for i, (side, x, y) in enumerate(THIN):
        out.append(base_gauge(
            gaugeType=4,
            label=letter(i),
            fontSize=18,
            units="V",
            x=x, y=y, w=side, h=side,
            crown=crown, ct=0,
            displayLabel=label_on,
            displayUnits=False,
            scaleValues=False,
            steps=6, mtics=0,
            minThresholdEnabled=True,
            maxThresholdEnabled=True,
        ))
    return out


def digital_gauges(crown: bool, label_on: bool, units_on: bool) -> list[dict]:
    boxes = digital_boxes(units_on, label_on, crown)
    out = []
    for i, (x, y, w, h) in enumerate(boxes):
        out.append(base_gauge(
            gaugeType=2,
            label=letter(i),
            fontSize=18,
            units="psi",
            x=x, y=y, w=w, h=h,
            crown=crown, ct=0,
            displayLabel=label_on,
            displayUnits=units_on,
            scaleValues=False,
            steps=0, mtics=0,
            startAngle=-120, endAngle=120,
            minThresholdEnabled=False,
            maxThresholdEnabled=False,
        ))
    return out


def barh_gauges(crown: bool, label_on: bool) -> list[dict]:
    out = []
    for i, (w, h, x, y) in enumerate(BAR_H):
        out.append(base_gauge(
            gaugeType=1,
            label=letter(i),
            fontSize=18,
            units="psi",
            x=x, y=y, w=w, h=h,
            crown=crown, ct=0,
            displayLabel=label_on,
            displayUnits=False,
            scaleValues=False,
            orientation=False, hor=True,
            steps=4, mtics=0,
            minThresholdEnabled=True,
            maxThresholdEnabled=True,
        ))
    return out


def barv_gauges(label_on: bool) -> list[dict]:
    out = []
    for i, (w, h, x, y) in enumerate(BAR_V):
        out.append(base_gauge(
            gaugeType=1,
            label=letter(i),
            fontSize=18,
            units="psi",
            x=x, y=y, w=w, h=h,
            crown=False, ct=0,
            displayLabel=label_on,
            displayUnits=False,
            scaleValues=False,
            orientation=True, hor=False,
            steps=4, mtics=0,
            minThresholdEnabled=True,
            maxThresholdEnabled=True,
        ))
    return out


def style2_gauges(crown: bool, label_on: bool) -> list[dict]:
    out = []
    for i, (w, h, x, y) in enumerate(STYLE2):
        out.append(base_gauge(
            gaugeType=11,
            label=letter(i),
            fontSize=18,
            units="",
            x=x, y=y, w=w, h=h,
            crown=crown, ct=0,
            displayLabel=label_on,
            displayUnits=False,
            scaleValues=False,
            orientation=False, hor=False,
            steps=8, mtics=1,
            minThresholdEnabled=True,
            maxThresholdEnabled=True,
        ))
    return out


def graph_gauges(crown: bool, label_on: bool) -> list[dict]:
    out = []
    for i, (w, h, x, y) in enumerate(GRAPH):
        out.append(base_gauge(
            gaugeType=9,
            label=letter(i),
            fontSize=18,
            units="",
            x=x, y=y, w=w, h=h,
            crown=crown, ct=0,
            displayLabel=label_on,
            displayUnits=False,
            scaleValues=False,
            steps=6, mtics=0,
            minThresholdEnabled=True,
            maxThresholdEnabled=True,
        ))
    return out


def led_gauges(label_on: bool) -> list[dict]:
    out = []
    for i, (w, h, x, y, large) in enumerate(LED):
        out.append(base_gauge(
            gaugeType=5,
            label=letter(i),
            fontSize=18,
            units="",
            x=x, y=y, w=w, h=h,
            crown=False, ct=0,
            displayLabel=label_on,
            orientation=large, hor=False,
            steps=0, mtics=0,
            minThresholdEnabled=True,
            maxThresholdEnabled=True,
        ))
    return out


def led_fontsize_gauges() -> list[dict]:
    # Labels on. Same Y per row so a cap/drop change is obvious.
    # Editor ignores LED fontSize (fixed 13 cap). Dash unknown.
    out = []
    y_n, y_l = 80, 220
    normal = ((8, "8", 20), (12, "12", 140), (18, "18", 260),
              (24, "24", 380), (36, "36", 500), (-1, "auto", 620))
    for fs, lab, x in normal:
        out.append(base_gauge(
            gaugeType=5, label=lab, fontSize=fs,
            x=x, y=y_n, w=44, h=63,
            crown=False, ct=0, displayLabel=True,
            orientation=False, hor=False, steps=0, mtics=0,
            minThresholdEnabled=True, maxThresholdEnabled=True,
        ))
    large = ((8, "L8", 20), (18, "L18", 160), (36, "L36", 300))
    for fs, lab, x in large:
        out.append(base_gauge(
            gaugeType=5, label=lab, fontSize=fs,
            x=x, y=y_l, w=90, h=109,
            crown=False, ct=0, displayLabel=True,
            orientation=True, hor=False, steps=0, mtics=0,
            minThresholdEnabled=True, maxThresholdEnabled=True,
        ))
    return out


def rpm_gauges() -> list[dict]:
    out = []
    for i, (w, h, x, y) in enumerate(RPM):
        out.append(base_gauge(
            gaugeType=6,
            label=letter(i),
            fontSize=18,
            units="",
            x=x, y=y, w=w, h=h,
            crown=False, ct=0,
            displayLabel=False,
            rpmModeEnabled=False,
            nnl=5, ncl=5,
            steps=15, mtics=0,
            minValue=0, maxValue=8000,
            minCaution=0, maxCaution=7000,
            minNormal=0, maxNormal=6500,
            minThresholdEnabled=False,
            maxThresholdEnabled=False,
            numericIndicatorEnabled=False,
        ))
    return out


def rpm_mode_gauges() -> list[dict]:
    # Same pos/size as P6-rpm so they overlay.
    # A: both off (should match P6 A).
    # B: rpmModeEnabled + max thresh (skull1 / live Customize Enable).
    # C: max thresh only (editor's current RpmBarIsShiftLight trap).
    flags = (
        (False, False),
        (True, True),
        (False, True),
    )
    out = []
    for i, ((w, h, x, y), (mode, thresh)) in enumerate(zip(RPM, flags)):
        out.append(base_gauge(
            gaugeType=6,
            label=letter(i),
            fontSize=18,
            units="",
            x=x, y=y, w=w, h=h,
            crown=False, ct=0,
            displayLabel=False,
            rpmModeEnabled=mode,
            nnl=5, ncl=5,
            steps=15, mtics=0,
            minValue=0, maxValue=8000,
            minCaution=0, maxCaution=7000,
            minNormal=0, maxNormal=6500,
            minThresholdEnabled=False,
            maxThresholdEnabled=thresh,
            numericIndicatorEnabled=False,
        ))
    return out


def symbol_gauges() -> list[dict]:
    out = []
    for i, (side, x, y) in enumerate(SYMBOL):
        out.append(base_gauge(
            gaugeType=8,
            label="\\xf095",
            fontSize=35,  # fixed; size= is the hit box
            units="",
            x=x, y=y, w=side, h=side,
            crown=False, ct=0,
            displayLabel=False,
            displayUnits=False,
            scaleValues=False,
            steps=0, mtics=0,
            minThresholdEnabled=True,
            maxThresholdEnabled=True,
            numericIndicatorEnabled=False,
        ))
    return out


def switch_outputs() -> list[dict]:
    out = []
    for i, (style, w, h, x, y) in enumerate(SWITCHES):
        out.append({
            "text": letter(i),
            "x": x, "y": y, "w": w, "h": h,
            "channel": 11,  # local ssr.1 on this bench; not universal
            "style": style,
            "inactiveColor": SW_INACTIVE,
            "activeColor": SW_ACTIVE,
            "background": PLATE,
            "fontSize": 18,
        })
    return out


def main() -> None:
    write_jpg()

    write_ini("P0-analog-noborder-labeloff.ini", analog_gauges(False, 0, False))
    write_ini("P0-analog-noborder-labelon.ini", analog_gauges(False, 0, True))
    write_ini("P0-analog-original-labeloff.ini", analog_gauges(True, 0, False))
    write_ini("P0-analog-original-labelon.ini", analog_gauges(True, 0, True))
    write_ini("P0-analog-blit-labeloff.ini", analog_gauges(True, 1, False))
    write_ini("P0-analog-blit-labelon.ini", analog_gauges(True, 1, True))

    write_ini("P4-thin-noborder-labeloff.ini", thin_gauges(False, False))
    write_ini("P4-thin-noborder-labelon.ini", thin_gauges(False, True))
    write_ini("P4-thin-border-labeloff.ini", thin_gauges(True, False))
    write_ini("P4-thin-border-labelon.ini", thin_gauges(True, True))

    write_ini("P2-digital-noborder-labeloff-unitsoff.ini",
              digital_gauges(False, False, False))
    write_ini("P2-digital-border-labeloff-unitsoff.ini",
              digital_gauges(True, False, False))
    write_ini("P2-digital-noborder-labelon-unitsoff.ini",
              digital_gauges(False, True, False))
    write_ini("P2-digital-noborder-labeloff-unitson.ini",
              digital_gauges(False, False, True))

    write_ini("P1-barh-noborder-labeloff.ini", barh_gauges(False, False))
    write_ini("P1-barh-noborder-labelon.ini", barh_gauges(False, True))
    write_ini("P1-barh-border-labeloff.ini", barh_gauges(True, False))
    write_ini("P1-barh-border-labelon.ini", barh_gauges(True, True))

    write_ini("P1-barv-noborder-labeloff.ini", barv_gauges(False))
    write_ini("P1-barv-noborder-labelon.ini", barv_gauges(True))

    write_ini("P11-style2-noborder-labeloff.ini", style2_gauges(False, False))
    write_ini("P11-style2-noborder-labelon.ini", style2_gauges(False, True))
    write_ini("P11-style2-border-labeloff.ini", style2_gauges(True, False))
    write_ini("P11-style2-border-labelon.ini", style2_gauges(True, True))

    write_ini("P9-graph-noborder-labeloff.ini", graph_gauges(False, False))
    write_ini("P9-graph-noborder-labelon.ini", graph_gauges(False, True))
    write_ini("P9-graph-border-labeloff.ini", graph_gauges(True, False))
    write_ini("P9-graph-border-labelon.ini", graph_gauges(True, True))

    write_ini("P5-led-labeloff.ini", led_gauges(False))
    write_ini("P5-led-labelon.ini", led_gauges(True))
    write_ini("P5-led-fontsize.ini", led_fontsize_gauges())

    write_ini("P6-rpm.ini", rpm_gauges())
    write_ini("P6-rpm-mode.ini", rpm_mode_gauges())
    write_ini("P8-symbol.ini", symbol_gauges())
    write_ini("Psw-switches.ini", [], switch_outputs())

    n = len(list(ROOT.glob("P*.ini")))
    print(f"Wrote {n} INIs + {BG_NAME} in {ROOT}")


if __name__ == "__main__":
    main()
