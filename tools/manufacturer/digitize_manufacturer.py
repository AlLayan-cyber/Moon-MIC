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

# curve id: (crop, source, f-calibration (Hz), x-pixels, dB-calibration, y-pixels, colour, y-limits, f-range)
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
    for cid, (png, src, fx, px, dby, py, col, ylim, fr) in CURVES.items():
        f, d = trace(str(crop_dir / png), fx, px, dby, py, col, ylim,
                     overlay=str(crop_dir / f"overlay_{cid}.png"), f_range=fr)
        # resample to 1/12 octave for compact storage
        g = 20 * 2 ** (np.arange(0, 120) / 12)
        g = g[(g >= f.min()) & (g <= f.max())]
        lev = np.interp(np.log(g), np.log(f), d)
        out[cid] = {"source": src, "status": "INFERRED (digitised manufacturer graph, ~±0.5-1 dB)",
                    "freq_hz": [round(float(v), 2) for v in g], "level_db": [round(float(v), 2) for v in lev]}
        pick = {hz: round(float(np.interp(np.log(hz), np.log(g), lev)), 1) for hz in (20, 30, 50, 100, 1000, 10000, 16000, 18000, 20000)
                if g.min() <= hz <= g.max()}
        print(cid, pick)
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(json.dumps(out, indent=1), encoding="utf-8")
    print("->", OUT.relative_to(ROOT))
    return 0


if __name__ == "__main__":
    sys.exit(main())
