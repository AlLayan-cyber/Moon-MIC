"""Print plate-curve crossings of the GE 6AU6-A triode-connection chart at fixed plate currents.

Input : scale-5 render of docs/manufacturer_data/tube_6au6a/6au6a_general_electric.pdf page 4, lower half
        (pypdfium2: page.render(scale=5), crop rows from 0.5*height). Frame calibration in that crop:
        0 V at x=586, 500 V at x=2543.5; 0 mA at y=1491.5, 35 mA at y=121.5 (grid-line detection).
Method: vertical grid lines (dark fraction > 0.6 over the plot) are blanked, dark runs on the row are merged
        (gap <= 12 px) and their centres converted to volts. Rows are chosen between 0.5 mA grid lines.
        Only rows where the crossing count equals the number of curves present were kept; the values are
        recorded in research/capsule_engine/fit_6au6a.py (ROWS).
Usage : .venv/Scripts/python.exe tools/manufacturer/digitize_triode_curves.py <crop.png>
"""
from __future__ import annotations

import sys

import numpy as np
from PIL import Image

X0, X500, Y0, Y35 = 586.0, 2543.5, 1491.5, 121.5
LEVELS_MA = (0.75, 1.25, 2.25, 3.25, 4.75, 7.25, 9.75, 12.25, 15.25, 17.75, 20.25, 22.75, 25.25, 27.75, 30.25)


def main() -> int:
    g = np.asarray(Image.open(sys.argv[1]).convert("L")).astype(float)
    d = g < 128
    plot = d[125:1488, 590:2540]
    gc = np.flatnonzero(plot.mean(axis=0) > 0.6) + 590
    m = d.copy()
    for c in gc:
        m[:, c - 1:c + 2] = False
    for i_ma in LEVELS_MA:
        y = int(round(Y0 - i_ma * (Y0 - Y35) / 35))
        idx = np.flatnonzero(m[y - 1:y + 2, 590:2540].any(axis=0)) + 590
        runs = np.split(idx, np.flatnonzero(np.diff(idx) > 12) + 1)
        print(i_ma, [round((r.mean() - X0) * 500 / (X500 - X0), 1) for r in runs if r.size >= 2])
    return 0


if __name__ == "__main__":
    sys.exit(main())
