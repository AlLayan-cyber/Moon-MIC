"""Long-term average spectrum (LTAS) of the voiced part of a vocal take, on the model's log grid.

Why LTAS: the source-position estimator (docs/architecture/06, TRANSFORM) only needs the long-term spectral
balance. For a linear microphone model the recorded LTAS is exactly voice LTAS + |H_mic(f, r)|^2 (in dB), so
synthetic training data can be built on spectra directly, without rendering audio.

Voicing: 186 ms frames (hop 46 ms; long frames for ~5 Hz resolution at the low end), frame kept if its level is within 35 dB of the take's 95th percentile and the
normalised autocorrelation peak in 70-1100 Hz exceeds 0.5. The frame's f0 is that peak's lag.
"""
from __future__ import annotations

import numpy as np
import soundfile as sf


def analyse(path: str, fg: np.ndarray) -> dict | None:
    x, fs = sf.read(path, dtype="float64", always_2d=True)
    x = x.mean(axis=1)
    n, hop = 8192 if fs > 32000 else 4096, 2048 if fs > 32000 else 1024   # ~5 Hz bins: proximity lives below ~500 Hz
    if x.size < n * 4:
        return None
    win = np.hanning(n)
    frames = np.lib.stride_tricks.sliding_window_view(x, n)[::hop]
    lev = 10 * np.log10(np.mean(frames ** 2, axis=1) + 1e-20)
    loud = lev > np.percentile(lev, 95) - 35.0
    lag_lo, lag_hi = int(fs / 1100), int(fs / 70)
    y = frames[loud]
    y = y - y.mean(axis=1, keepdims=True)
    ac = np.fft.irfft(np.abs(np.fft.rfft(y, 2 * n, axis=1)) ** 2, axis=1)[:, : lag_hi + 1]
    ok = ac[:, 0] > 0
    y, ac = y[ok], ac[ok] / ac[ok, :1]
    k = lag_lo + np.argmax(ac[:, lag_lo: lag_hi + 1], axis=1)
    voiced = ac[np.arange(len(k)), k] >= 0.5
    used = int(voiced.sum())
    if used < 20:
        return None
    spec = np.sum(np.abs(np.fft.rfft(y[voiced] * win, axis=1)) ** 2, axis=0)
    f0s = fs / k[voiced]
    f = np.fft.rfftfreq(n, 1 / fs)
    db = 10 * np.log10(spec / used + 1e-30)
    # 1/3-octave power average (+-1/6 octave) evaluated on the 1/48-octave model grid
    out = np.full(fg.size, np.nan)
    for i, fc in enumerate(fg):
        m = (f >= fc * 2 ** (-1 / 6)) & (f <= fc * 2 ** (1 / 6))
        if m.any() and fc < fs / 2:
            out[i] = 10 * np.log10(np.mean(10 ** (db[m] / 10)))
    return {"ltas_db": out, "f0_median": float(np.median(f0s)), "voiced_frames": used, "fs": int(fs)}
