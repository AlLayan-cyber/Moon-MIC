"""Digitise manufacturer on-axis frequency responses from rendered datasheet crops.

Crops are rendered from the PDFs in docs/manufacturer_data (pypdfium2) into a working folder.
Calibration pixels were read from the crops and are recorded here for traceability;
each trace writes an overlay PNG that must be checked visually.

Output: data/manufacturer/frequency_responses.json
        {curve_id: {"source": ..., "page": ..., "freq_hz": [...], "level_db": [...], "status": "INFERRED"}}
Usage : .venv/Scripts/python.exe tools/manufacturer/digitize_manufacturer.py <crop_dir>
"""
from __future__ import annotations

import json
import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
from trace_curve import trace  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "data" / "manufacturer" / "frequency_responses.json"

U87 = "docs/manufacturer_data/neumann_u87ai/u87ai_operating_manual_opin0134.pdf, p.31 (PDF page 16)"
K2 = "docs/manufacturer_data/rode_k2/k2_datasheet_v4.pdf, p.2"
NT2A = "docs/manufacturer_data/rode_nt2a/nt2a_datasheet.pdf, p.2"
C414 = "docs/manufacturer_data/akg_c414bxls/c414bxls_manual.pdf, p.29 (C 414 B-XLS column)"
U47 = "docs/manufacturer_data/neumann_u47fet/u47fet_operating_manual.pdf, p.14 (free field, IEC 60268-4)"
C800 = "docs/manufacturer_data/sony_c800g/c800g_service_manual_manualslib.pdf, p.6 (service manual p.1-3, 0 deg curve)"

# Full-page renders (crop_dir): c414_p29_s8.png = C414 manual p.29 at scale 8; u47man_p14_s8.png = U47 fet manual
# p.14 at scale 8; c800_p6_s5.png = C-800G service manual p.6 at scale 5. Grid-line pixels were detected
# programmatically (dark-column / dark-row profiles) and are recorded below.
# C414 graphs: 2 dB per grid row (12.27 px), 0 dB row of each graph = grid top (+20 dB) + 122.7 px.
_C414 = {"min_run": 5, "snap": False, "skip_vertical": True, "despike": (41, 1.5)}
_C800 = {"min_run": 5, "snap": False, "skip_vertical": True, "despike": (41, 1.5)}
_U47 = {"min_run": 6, "snap": False}

# curve id: (crop, source, f-calibration (Hz), x-pixels, dB-calibration, y-pixels, colour, y-limits, f-range[, options])
# options: trace() keywords (min_run, snap, skip_vertical) + "despike": (window, dB) + "bands": ("lf",) / ("lf", "hf")
CURVES_EXTRA = {
    "u47fet":           ("u47man_p14_s8.png", U47, (20, 20000), (263.5, 2144), (0, -10), (996.5, 1112), "dark", (800, 1240), (20, 20000), _U47),
    "c414bxls_omni":    ("c414_p29_s8.png", C414, (20, 20000), (380.5, 1329.5), (0, -10), (1115.5, 1176.8), "dark", (987, 1304), (31, 20000), _C414),
    "c414bxls_cardioid":("c414_p29_s8.png", C414, (20, 20000), (380.5, 1329.5), (0, -10), (2650.5, 2711.8), "dark", (2522, 2839), (31, 20000), _C414),
    "c414bxls_hyper":   ("c414_p29_s8.png", C414, (20, 20000), (380.5, 1329.5), (0, -10), (3434.5, 3495.8), "dark", (3306, 3623), (31, 20000), _C414),
    "c414bxls_figure8": ("c414_p29_s8.png", C414, (20, 20000), (380.5, 1329.5), (0, -10), (4200.5, 4261.8), "dark", (4072, 4389), (31, 20000), _C414),
    "c800g_cardioid":   ("c800_p6_s5.png", C800, (20, 20000), (490.5, 1355.5), (0, -10), (1311, 1424.5), "dark", (1192, 1770), (20, 20000), _C800),
    "c800g_omni":       ("c800_p6_s5.png", C800, (20, 20000), (1800, 2660), (0, -10), (1312, 1425.5), "dark", (1192, 1770), (20, 3000), {**_C800, "bands": ("lf",)}),  # above ~3.5 kHz the 0/90/180 deg curves cross and the 0 deg curve cannot be identified reliably
}

CURVES = {
    "u87ai_omni":     ("u87_omni.png", U87, (20, 20000), (105, 1453), (0, -10), (201, 284), "dark", (60, 380), (20, 20000)),
    "u87ai_cardioid": ("u87_card.png", U87, (20, 20000), (105, 1453), (0, -10), (209, 291), "dark", (60, 390), (20, 20000)),
    "u87ai_figure8":  ("u87_fig8.png", U87, (20, 20000), (105, 1453), (0, -10), (200, 283), "dark", (60, 380), (20, 20000)),
    "k2_cardioid":    ("k2_card.png", K2, (20, 20000), (160, 1053), (0, -10), (225, 279), "red", (100, 400), (20, 20000)),
    "k2_omni":        ("k2_omni.png", K2, (20, 20000), (159, 1052), (0, -10), (226, 280), "red", (100, 400), (20, 20000)),
    "nt2a_cardioid":  ("nt2a_card.png", NT2A, (20, 20000), (136, 1074), (0, -10), (173, 231), "red", (40, 330), (20, 20000)),
    "nt2a_omni":      ("nt2a_omni.png", NT2A, (20, 20000), (138, 1076), (0, -10), (181, 239), "red", (40, 330), (20, 20000)),
}


def main() -> int:
    crop_dir = Path(sys.argv[1])
    out = {}
    only = set(sys.argv[2:])
    for cid, (png, src, fx, px, dby, py, col, ylim, fr, *rest) in {**CURVES, **CURVES_EXTRA}.items():
        if only and cid not in only:
            continue
        opts = dict(rest[0]) if rest else {}
        despike = opts.pop("despike", None)
        bands = opts.pop("bands", None)
        f, d = trace(str(crop_dir / png), fx, px, dby, py, col, ylim,
                     overlay=str(crop_dir / f"overlay_{cid}.png"), f_range=fr, **opts)
        if despike:
            # drop label glyphs / leader lines: points that deviate from a running median
            w, thr = despike
            med = np.array([np.median(d[max(0, i - w // 2): i + w // 2 + 1]) for i in range(d.size)])
            keep = np.abs(d - med) <= thr
            f, d = f[keep], d[keep]
        # resample to 1/12 octave for compact storage
        g = 20 * 2 ** (np.arange(0, 120) / 12)
        g = g[(g >= f.min()) & (g <= f.max())]
        lev = np.interp(np.log(g), np.log(f), d)
        out[cid] = {"source": src, "status": "INFERRED (digitised manufacturer graph, ~±0.5-1 dB)",
                    "freq_hz": [round(float(v), 2) for v in g], "level_db": [round(float(v), 2) for v in lev]}
        if bands:
            out[cid]["bands"] = list(bands)
        pick = {hz: round(float(np.interp(np.log(hz), np.log(g), lev)), 1) for hz in (20, 30, 50, 100, 1000, 10000, 16000, 18000, 20000)
                if g.min() <= hz <= g.max()}
        print(cid, pick)
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(json.dumps(out, indent=1), encoding="utf-8")
    print("->", OUT.relative_to(ROOT))
    return 0


if __name__ == "__main__":
    sys.exit(main())
