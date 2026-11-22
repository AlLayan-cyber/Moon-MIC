"""Semi-automatic digitiser for manufacturer frequency-response graphs (raster crops).

Calibration is given explicitly per graph (pixel x of two frequencies, pixel y of two dB values);
y positions are snapped to the nearest strong horizontal grid line within +/-8 px.
The curve is traced column by column as the TOPMOST run of curve-coloured pixels
(the bass-cut variant drawn below the main curve is therefore ignored).

Output: CSV (freq_hz, level_db) on a log grid, + an overlay PNG for visual verification.
Usage : called from digitize_manufacturer.py
"""
from __future__ import annotations

import numpy as np
from PIL import Image, ImageDraw


def snap_row(gray: np.ndarray, y_guess: int, x0: int, x1: int, win: int = 8) -> int:
    rows = range(max(0, y_guess - win), min(gray.shape[0], y_guess + win + 1))
    score = [(np.mean(gray[r, x0:x1] < 235), r) for r in rows]
    return max(score)[1]


def trace(png: str, fx: tuple[float, float], px: tuple[int, int], dby: tuple[float, float], py: tuple[int, int],
          colour: str = "dark", y_limits: tuple[int, int] | None = None, overlay: str | None = None,
          f_range: tuple[float, float] = (20.0, 20000.0)) -> tuple[np.ndarray, np.ndarray]:
    im = Image.open(png).convert("RGB")
    rgb = np.asarray(im).astype(int)
    gray = rgb.mean(axis=2)
    x0, x1 = min(px), max(px)
    py = (snap_row(gray, py[0], x0, x1), snap_row(gray, py[1], x0, x1))
    if colour == "dark":
        mask = gray < 90
    elif colour == "red":
        mask = (rgb[:, :, 0] > 150) & (rgb[:, :, 1] < 110) & (rgb[:, :, 2] < 110)
    else:
        raise ValueError(colour)
    ylo, yhi = y_limits if y_limits else (0, mask.shape[0])

    lf = np.log10(fx)
    def x_to_f(x):
        return 10 ** (lf[0] + (x - px[0]) * (lf[1] - lf[0]) / (px[1] - px[0]))
    def y_to_db(y):
        return dby[0] + (y - py[0]) * (dby[1] - dby[0]) / (py[1] - py[0])

    fs, ds = [], []
    for x in range(x0, x1 + 1):
        col = np.flatnonzero(mask[ylo:yhi, x])
        if col.size == 0:
            continue
        top = col[0]
        run = top
        while run + 1 < col.size + top and (run + 1 - top) < col.size and col[run + 1 - top] == run + 1:
            run += 1
        y = ylo + 0.5 * (top + run)      # centre of the topmost run
        f = x_to_f(x)
        if f_range[0] <= f <= f_range[1]:
            fs.append(f); ds.append(y_to_db(y))
    fs, ds = np.array(fs), np.array(ds)

    if overlay:
        ov = im.copy(); d = ImageDraw.Draw(ov)
        for f, db in zip(fs, ds):
            x = px[0] + (np.log10(f) - lf[0]) * (px[1] - px[0]) / (lf[1] - lf[0])
            y = py[0] + (db - dby[0]) * (py[1] - py[0]) / (dby[1] - dby[0])
            d.point((x, y), fill=(0, 200, 255))
        for y in py:
            d.line((x0, y, x1, y), fill=(0, 255, 0))
        ov.save(overlay)
    return fs, ds
