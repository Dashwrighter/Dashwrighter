#!/usr/bin/env python3
"""Generate honor/ignore INIs for Customize settings the dash may not expose.

Run from this folder:  python generate_honor.py
Load on the dash only. Do not Save from the editor.
"""

from __future__ import annotations

import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parent
PLACEMENT_BG = ROOT.parent / "placement" / "internal" / "images" / "PlacementGray.jpg"
IMG_DIR = ROOT / "internal" / "images"
CANVAS_W, CANVAS_H = 1280, 480
BG_NAME = "PlacementGray.jpg"
BG_PATH_INI = f"/internal/images/{BG_NAME}"


def argb(a: int, r: int, g: int, b: int) -> int:
    return ((a & 255) << 24) | ((r & 255) << 16) | ((g & 255) << 8) | (b & 255)


PLATE = argb(255, 0, 0, 0)
FG = argb(255, 255, 255, 255)
C0 = argb(255, 220, 0, 0)
C1 = argb(255, 220, 180, 0)
C2 = argb(255, 0, 180, 0)
C3 = argb(255, 220, 180, 0)
C4 = argb(255, 220, 0, 0)
CHANNEL = 41  # Battery


def bool_str(v: bool) -> str:
    return "true" if v else "false"


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
        "minThresholdEnabled": True,
        "maxThresholdEnabled": True,
        "minPeakEnabled": False,
        "maxPeakEnabled": False,
        "peakReductionEnabled": False,
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
        "displayLabel": True,
        "isLED": False,
        "valueMult2": 0,
        "valueMult": 1,
        "valueMultB": 0,
        "scaleValues": False,
        "orientation": False,
        "hor": False,
        "nnl": 5,
        "ncl": 5,
        "updateRate": 6,
        "av": 0,
        "avFlag": False,
        "wfr": False,
        "bm": False,
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


def write_gauge(lines: list[str], idx: int, g: dict) -> None:
    p = f"analogMeters\\{idx}\\"
    lines.append(f"{p}label={g['label']}")
    lines.append(f"{p}fontSize={g['fontSize']}")
    lines.append(f'{p}pos="({g["x"]},{g["y"]})"')
    lines.append(f'{p}size="({g["w"]},{g["h"]})"')
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
    lines.append(f"{p}minPeakEnabled={bool_str(g['minPeakEnabled'])}")
    lines.append(f"{p}maxPeakEnabled={bool_str(g['maxPeakEnabled'])}")
    lines.append(f"{p}numericIndicatorEnabled={bool_str(g['numericIndicatorEnabled'])}")
    lines.append(f"{p}peakReductionEnabled={bool_str(g['peakReductionEnabled'])}")
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
    lines.append(f"{p}isLED={bool_str(g['isLED'])}")
    lines.append(f"{p}valueMult2={g['valueMult2']}")
    lines.append(f"{p}valueMult={g['valueMult']}")
    lines.append(f"{p}valueMultB={g['valueMultB']}")
    lines.append(f"{p}scaleValues={bool_str(g['scaleValues'])}")
    lines.append(f"{p}orientation={bool_str(g['orientation'])}")
    lines.append(f"{p}updateRate={g['updateRate']}")
    lines.append(f"{p}c0={g['c0']}")
    lines.append(f"{p}c1={g['c1']}")
    lines.append(f"{p}c2={g['c2']}")
    lines.append(f"{p}c3={g['c3']}")
    lines.append(f"{p}c4={g['c4']}")
    lines.append(f"{p}wfr={bool_str(g['wfr'])}")
    lines.append(f"{p}bm={bool_str(g['bm'])}")
    lines.append(f"{p}hor={bool_str(g['hor'])}")
    lines.append(f"{p}nnl={g['nnl']}")
    lines.append(f"{p}ncl={g['ncl']}")
    lines.append(f"{p}av={g['av']}")
    lines.append(f"{p}avFlag={bool_str(g['avFlag'])}")
    lines.append(f"{p}ct={g['ct']}")


def write_ini(name: str, gauges: list[dict]) -> None:
    check_boxes(name, [(g["x"], g["y"], g["w"], g["h"]) for g in gauges])
    lines = ["[Layout]", f"Background={BG_PATH_INI}", "v=10"]
    for i, g in enumerate(gauges, 1):
        write_gauge(lines, i, g)
    lines.append(f"analogMeters\\size={len(gauges)}")
    lines.append("labels\\size=0")
    lines.append("outputs\\size=0")
    text = "\r\n".join(lines) + "\r\n"
    (ROOT / name).write_bytes(text.encode("ascii"))


def copy_background() -> None:
    IMG_DIR.mkdir(parents=True, exist_ok=True)
    dest = IMG_DIR / BG_NAME
    if not PLACEMENT_BG.is_file():
        raise SystemExit(f"missing placement background: {PLACEMENT_BG}")
    shutil.copy2(PLACEMENT_BG, dest)


def analog_units() -> list[dict]:
    # Does type 0 paint units next to the numeric value when displayUnits=true?
    # 2x2 of Value Display x Units. Digital is the positive control (type 2 does).
    # Battery ~12 in 0..20; units=psi so a suffix is obvious if it appears.
    side = 200
    y = 40
    xs = (16, 232, 448, 664)
    combos = (
        ("v1u1", True, True),
        ("v1u0", True, False),
        ("v0u1", False, True),
        ("v0u0", False, False),
    )
    rng = dict(
        minValue=0, maxValue=20, precision=1, units="psi",
        minCaution=8, minNormal=10, maxNormal=14, maxCaution=16,
        minThresholdEnabled=True, maxThresholdEnabled=True,
    )
    out = []
    for x, (lab, value_on, units_on) in zip(xs, combos):
        out.append(base_gauge(
            gaugeType=0, label=lab, fontSize=18,
            x=x, y=y, w=side, h=side,
            crown=False, ct=0, steps=8, mtics=1,
            scaleValues=False, displayLabel=True,
            numericIndicatorEnabled=value_on,
            displayUnits=units_on,
            **rng,
        ))
    out.append(base_gauge(
        gaugeType=2, label="Dig", fontSize=18,
        x=900, y=80, w=170, h=120,
        steps=0, mtics=0, scaleValues=False, displayLabel=True,
        numericIndicatorEnabled=True, displayUnits=True,
        **rng,
    ))
    return out


def analog_ct_crown() -> list[dict]:
    # Does analog ct still pick blit tick/arc geometry when crown=false?
    # Columns: off0 (P0 no-border) | off1 (THE TEST) | on0 (Original) | on1 (Black blit)
    # Rows: 255 and 180 (P0 rungs). Arcs on, opaque plate, ticks on.
    combos = (
        ("off0", False, 0),
        ("off1", False, 1),
        ("on0", True, 0),
        ("on1", True, 1),
    )
    large = 255
    gap = 8
    col_x = [8 + i * (large + gap) for i in range(len(combos))]
    out = []
    for side, y in ((255, 0), (180, 263)):
        for x, (lab, crown, ct) in zip(col_x, combos):
            out.append(base_gauge(
                gaugeType=0, label=lab, fontSize=18,
                x=x, y=y, w=side, h=side,
                crown=crown, ct=ct, units="V",
                steps=8, mtics=1,
                minThresholdEnabled=True, maxThresholdEnabled=True,
                scaleValues=False, numericIndicatorEnabled=True,
                displayLabel=True, displayUnits=False,
            ))
    return out


def thin_borderstyle() -> list[dict]:
    # Customize has Border, no Border Style. ct 0 Original / 1 Black / 2 Grey / 3 Silver.
    side = 200
    y = 140
    xs = [16, 232, 448, 664]
    labels = ("ct0", "ct1", "ct2", "ct3")
    out = []
    for i, (x, lab) in enumerate(zip(xs, labels)):
        out.append(base_gauge(
            gaugeType=4, label=lab, x=x, y=y, w=side, h=side,
            crown=True, ct=i, units="V", steps=6, mtics=0,
            scaleValues=False, numericIndicatorEnabled=True,
        ))
    return out


def thin_scale() -> list[dict]:
    side = 220
    y = 130
    return [
        base_gauge(
            gaugeType=4, label="off", x=16, y=y, w=side, h=side,
            units="V", steps=5, mtics=0, scaleValues=False, scalingDivisor=1,
            crown=False, ct=0,
        ),
        base_gauge(
            gaugeType=4, label="tics", x=252, y=y, w=side, h=side,
            units="V", steps=5, mtics=2, scaleValues=True, scalingDivisor=1,
            crown=False, ct=0,
        ),
        base_gauge(
            gaugeType=4, label="div10", x=488, y=y, w=side, h=side,
            units="V", steps=5, mtics=2, scaleValues=True, scalingDivisor=10,
            crown=False, ct=0,
        ),
    ]


def digital_valueoff() -> list[dict]:
    # Height 120; stored W from placement aspect (digits=3, no units, label on).
    h = 120
    w = 170
    y = 180
    return [
        base_gauge(
            gaugeType=2, label="on", x=80, y=y, w=w, h=h,
            units="V", precision=1, steps=0, mtics=0,
            numericIndicatorEnabled=True, displayUnits=False,
            minThresholdEnabled=False, maxThresholdEnabled=False,
            minValue=0, maxValue=20,
        ),
        base_gauge(
            gaugeType=2, label="off", x=400, y=y, w=w, h=h,
            units="V", precision=1, steps=0, mtics=0,
            numericIndicatorEnabled=False, displayUnits=False,
            minThresholdEnabled=False, maxThresholdEnabled=False,
            minValue=0, maxValue=20,
        ),
    ]


def digital_alpha() -> list[dict]:
    # Does the dash honor the A byte on Digital foreground / zone colors?
    # Customize only exposes Transparency on background.
    # In-car Battery is 12.0–13.5; trip ladders step 0.2 V so some faces
    # are in-range and some in high alarm (c4) at any voltage in that band.
    # Photo the unit (no dash screenshot). Judge the dash, not the editor
    # (preview forces text/warn-plate A>=200).
    h, w = 70, 148
    trips = (12.0, 12.2, 12.4, 12.6, 12.8, 13.0, 13.2, 13.4)
    xs = tuple(10 + i * 158 for i in range(8))
    y_ctrl, y_c4, y_brick, y_ghost = 8, 118, 228, 338
    quiet = dict(
        minValue=0, maxValue=20, precision=1, steps=0, mtics=0,
        minThresholdEnabled=False, maxThresholdEnabled=False,
    )
    gold = argb(255, 255, 200, 0)

    def high_at(trip: float) -> dict:
        return dict(
            minValue=0, maxValue=20, precision=1, steps=0, mtics=0,
            minThresholdEnabled=False, maxThresholdEnabled=True,
            minCaution=0, minNormal=0, maxNormal=trip, maxCaution=trip,
        )

    def trip_label(trip: float) -> str:
        return f"{trip:.1f}"

    gauges = [
        base_gauge(
            gaugeType=2, label="bg50", x=xs[0], y=y_ctrl, w=w, h=h,
            displayUnits=False, **quiet,
            background=argb(128, 0, 0, 0), foreground=argb(255, 255, 255, 255),
        ),
        base_gauge(
            gaugeType=2, label="fg50", x=xs[1], y=y_ctrl, w=w, h=h,
            displayUnits=False, **quiet,
            background=argb(255, 0, 0, 0), foreground=argb(128, 255, 255, 255),
        ),
        base_gauge(
            gaugeType=2, label="fg0", x=xs[2], y=y_ctrl, w=w, h=h,
            displayUnits=False, **quiet,
            background=argb(255, 0, 0, 0), foreground=argb(0, 255, 255, 255),
        ),
        base_gauge(
            gaugeType=2, label="both50", x=xs[3], y=y_ctrl, w=w, h=h,
            displayUnits=False, **quiet,
            background=argb(128, 0, 0, 0), foreground=argb(128, 255, 255, 255),
        ),
        base_gauge(
            gaugeType=2, label="same", x=xs[4], y=y_ctrl, w=w, h=h,
            displayUnits=False, **quiet,
            background=gold, foreground=gold,
        ),
    ]
    for i, trip in enumerate(trips):
        lab = trip_label(trip)
        kw = high_at(trip)
        gauges.append(base_gauge(
            gaugeType=2, label="c" + lab, x=xs[i], y=y_c4, w=w, h=h,
            displayUnits=False, **kw,
            background=argb(255, 0, 0, 0), foreground=argb(255, 255, 255, 255),
            c4=argb(128, 220, 0, 0),
        ))
        gauges.append(base_gauge(
            gaugeType=2, label="b" + lab, x=xs[i], y=y_brick, w=w, h=h,
            displayUnits=False, **kw,
            background=gold, foreground=argb(255, 255, 255, 255),
            c4=argb(0, 255, 200, 0),
        ))
        gauges.append(base_gauge(
            gaugeType=2, label="g" + lab, x=xs[i], y=y_ghost, w=w, h=h,
            displayUnits=False, **kw,
            background=argb(255, 255, 255, 255),
            foreground=argb(255, 255, 255, 255),
            c4=argb(0, 255, 255, 255),
        ))
    return gauges


def rpm_labelon() -> list[dict]:
    w, h = 750, 100
    x = 265
    return [
        base_gauge(
            gaugeType=6, label="RPM", x=x, y=80, w=w, h=h,
            fontSize=24, units="", minValue=0, maxValue=8000,
            minCaution=0, maxCaution=7000, minNormal=0, maxNormal=6500,
            minThresholdEnabled=False, maxThresholdEnabled=False,
            numericIndicatorEnabled=False, displayLabel=False,
            steps=15, nnl=5, ncl=5, rpmModeEnabled=False,
        ),
        base_gauge(
            gaugeType=6, label="RPM", x=x, y=260, w=w, h=h,
            fontSize=24, units="", minValue=0, maxValue=8000,
            minCaution=0, maxCaution=7000, minNormal=0, maxNormal=6500,
            minThresholdEnabled=False, maxThresholdEnabled=False,
            numericIndicatorEnabled=False, displayLabel=True,
            steps=15, nnl=5, ncl=5, rpmModeEnabled=False,
        ),
    ]


def style2_orient() -> list[dict]:
    w, h = 280, 120
    return [
        base_gauge(
            gaugeType=11, label="o0h0", x=40, y=40, w=w, h=h,
            orientation=False, hor=False, steps=8, mtics=1,
            numericIndicatorEnabled=True, scaleValues=False, units="V",
        ),
        base_gauge(
            gaugeType=11, label="o1h0", x=400, y=40, w=w, h=h,
            orientation=True, hor=False, steps=8, mtics=1,
            numericIndicatorEnabled=True, scaleValues=False, units="V",
        ),
        base_gauge(
            gaugeType=11, label="o0h1", x=40, y=220, w=w, h=h,
            orientation=False, hor=True, steps=8, mtics=1,
            numericIndicatorEnabled=True, scaleValues=False, units="V",
        ),
        base_gauge(
            gaugeType=11, label="o1h1", x=400, y=220, w=w, h=h,
            orientation=True, hor=True, steps=8, mtics=1,
            numericIndicatorEnabled=True, scaleValues=False, units="V",
        ),
    ]


def peaks() -> list[dict]:
    # Range 0..20 so Battery (~12 V) sits on-scale. Analog control pair + Digital + Bargraph.
    rng = dict(
        minValue=0, maxValue=20, precision=1,
        minCaution=8, minNormal=10, maxNormal=14, maxCaution=16,
        minThresholdEnabled=True, maxThresholdEnabled=True,
        units="V",
    )
    return [
        base_gauge(
            gaugeType=0, label="Aon", x=16, y=16, w=200, h=200,
            steps=8, mtics=1, scaleValues=True,
            minPeakEnabled=True, maxPeakEnabled=True, peakReductionEnabled=False,
            **rng,
        ),
        base_gauge(
            gaugeType=0, label="Aoff", x=232, y=16, w=200, h=200,
            steps=8, mtics=1, scaleValues=True,
            minPeakEnabled=False, maxPeakEnabled=False, peakReductionEnabled=False,
            **rng,
        ),
        base_gauge(
            gaugeType=2, label="Dig", x=448, y=16, w=170, h=120,
            steps=0, mtics=0, scaleValues=False,
            minPeakEnabled=True, maxPeakEnabled=True, peakReductionEnabled=False,
            displayUnits=True,
            **rng,
        ),
        base_gauge(
            gaugeType=1, label="BarOn", x=16, y=240, w=900, h=52,
            steps=4, mtics=1, scaleValues=True, orientation=False, hor=True,
            minPeakEnabled=True, maxPeakEnabled=True, peakReductionEnabled=False,
            **rng,
        ),
        base_gauge(
            gaugeType=1, label="BarOff", x=16, y=320, w=900, h=52,
            steps=4, mtics=1, scaleValues=True, orientation=False, hor=True,
            minPeakEnabled=False, maxPeakEnabled=False, peakReductionEnabled=False,
            **rng,
        ),
    ]


def led_fontsize() -> list[dict]:
    # Does Status LED Label Font Size change cap / drop? Editor currently ignores it
    # (fixed 13px cap). Same Y so a drop change is obvious. Load only; do not Save.
    w, h = 44, 63
    y = 80
    xs = [20, 140, 260, 380, 500, 620]
    rungs = ((8, "8"), (12, "12"), (18, "18"), (24, "24"), (36, "36"), (-1, "auto"))
    out = []
    for x, (fs, lab) in zip(xs, rungs):
        out.append(base_gauge(
            gaugeType=5, label=lab, fontSize=fs,
            x=x, y=y, w=w, h=h,
            orientation=False, displayLabel=True,
            minThresholdEnabled=True, maxThresholdEnabled=True,
        ))
    return out


def led_math() -> list[dict]:
    # LED Customize has no Channel Math. x2 should slam Battery into high alarm if honored.
    rng = dict(
        minValue=8, maxValue=20, precision=1, units="V",
        minCaution=9, minNormal=10, maxNormal=14, maxCaution=16,
        minThresholdEnabled=True, maxThresholdEnabled=True,
    )
    return [
        base_gauge(
            gaugeType=5, label="x1", x=80, y=160, w=90, h=109,
            orientation=True, valueMult=1, fontSize=18, **rng,
        ),
        base_gauge(
            gaugeType=5, label="x2", x=280, y=160, w=90, h=109,
            orientation=True, valueMult=2, fontSize=18, **rng,
        ),
    ]


def rpm_mode() -> list[dict]:
    w, h = 750, 100
    x = 265
    rpm = dict(
        gaugeType=6, label="RPM", fontSize=24, units="",
        minValue=0, maxValue=8000,
        minCaution=0, maxCaution=7000, minNormal=0, maxNormal=6500,
        numericIndicatorEnabled=False, displayLabel=False,
        steps=15, nnl=5, ncl=5,
    )
    return [
        base_gauge(
            x=x, y=80, w=w, h=h,
            rpmModeEnabled=False,
            minThresholdEnabled=False, maxThresholdEnabled=False,
            **rpm,
        ),
        base_gauge(
            x=x, y=260, w=w, h=h,
            rpmModeEnabled=True,
            minThresholdEnabled=False, maxThresholdEnabled=True,
            **rpm,
        ),
    ]


# Must match GaugeType.h BargraphCardThickness (2026-08-25).
BARGRAPH_TOP_PAD = 5
BARGRAPH_LABEL_BAR_PAD = 9
BARGRAPH_BAR_PX = 25
BARGRAPH_AUTO_PX = 12


def bargraph_thickness(font_size: int) -> int:
    label_px = font_size if font_size > 0 else BARGRAPH_AUTO_PX
    return BARGRAPH_TOP_PAD + label_px + BARGRAPH_LABEL_BAR_PAD + BARGRAPH_BAR_PX


# P1 used fs=18. Factory bars often 15. AUTO is 12 in the editor.
BARGRAPH_FS_LADDER = [
    (8, "8"),
    (12, "12"),
    (15, "15"),
    (18, "18"),
    (24, "24"),
    (36, "36"),
    (-1, "auto"),
]


def barh_fontsize(label_on: bool) -> list[dict]:
    out = []
    y = 8
    gap = 6
    for fs, lab in BARGRAPH_FS_LADDER:
        thick = bargraph_thickness(fs)
        out.append(base_gauge(
            gaugeType=1, label=lab, fontSize=fs,
            x=16, y=y, w=1100, h=thick,
            units="psi", steps=4, mtics=0,
            scaleValues=False, displayLabel=label_on,
            numericIndicatorEnabled=True,
            orientation=False, hor=True, crown=False,
        ))
        y += thick + gap
    return out


def barv_fontsize() -> list[dict]:
    out = []
    x = 16
    gap = 32
    for fs, lab in ((8, "8"), (18, "18"), (36, "36")):
        thick = bargraph_thickness(fs)
        out.append(base_gauge(
            gaugeType=1, label=lab, fontSize=fs,
            x=x, y=16, w=thick, h=400,
            units="psi", steps=4, mtics=0,
            scaleValues=False, displayLabel=True,
            numericIndicatorEnabled=True,
            orientation=True, hor=False, crown=False,
        ))
        x += thick + gap
    return out


def scale_on() -> list[dict]:
    # Placement never sets scaleValues=true. Photos show the row on these types.
    return [
        base_gauge(
            gaugeType=1, label="Bar", x=16, y=16, w=900, h=52,
            units="V", steps=4, mtics=1, scaleValues=True, scalingDivisor=1,
            orientation=False, hor=True,
        ),
        base_gauge(
            gaugeType=11, label="S2", x=16, y=88, w=500, h=140,
            units="V", steps=8, mtics=1, scaleValues=True, scalingDivisor=1,
            orientation=False, hor=False,
        ),
        base_gauge(
            gaugeType=9, label="Gr", x=540, y=88, w=400, h=280,
            units="V", steps=6, mtics=1, scaleValues=True, scalingDivisor=1,
        ),
    ]


def main() -> None:
    copy_background()
    packs = [
        ("H-analog-ct-crown.ini", analog_ct_crown),
        ("H-analog-units.ini", analog_units),
        ("H-thin-borderstyle.ini", thin_borderstyle),
        ("H-thin-scale.ini", thin_scale),
        ("H-digital-valueoff.ini", digital_valueoff),
        ("H-digital-alpha.ini", digital_alpha),
        ("H-rpm-labelon.ini", rpm_labelon),
        ("H-style2-orient.ini", style2_orient),
        ("H-peaks.ini", peaks),
        ("H-led-math.ini", led_math),
        ("H-led-fontsize.ini", led_fontsize),
        ("H-rpm-mode.ini", rpm_mode),
        ("H-scale-on.ini", scale_on),
        ("H-barh-fontsize-labelon.ini", lambda: barh_fontsize(True)),
        ("H-barh-fontsize-labeloff.ini", lambda: barh_fontsize(False)),
        ("H-barv-fontsize.ini", barv_fontsize),
    ]
    for name, fn in packs:
        write_ini(name, fn())
        print(name)


if __name__ == "__main__":
    main()
