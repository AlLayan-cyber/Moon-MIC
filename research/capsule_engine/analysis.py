"""Measurement helpers: harmonic levels / THD of a steady sine, aliasing energy."""
from __future__ import annotations

import numpy as np


def sine(f0: float, fs: float, seconds: float, amp: float = 1.0) -> np.ndarray:
    n = int(round(seconds * fs))
    return amp * np.sin(2 * np.pi * f0 * np.arange(n) / fs)


def spectrum(y: np.ndarray, fs: float, skip: float = 0.5) -> tuple[np.ndarray, np.ndarray]:
    """Blackman-Harris windowed amplitude spectrum of the steady part (first `skip` seconds dropped)."""
    y = np.asarray(y, float)[int(skip * fs):]
    n = len(y)
    a = [0.35875, 0.48829, 0.14128, 0.01168]
    k = np.arange(n)
    w = a[0] - a[1] * np.cos(2 * np.pi * k / n) + a[2] * np.cos(4 * np.pi * k / n) - a[3] * np.cos(6 * np.pi * k / n)
    Y = np.abs(np.fft.rfft((y - y.mean()) * w)) * 2 / w.sum()
    return np.fft.rfftfreq(n, 1 / fs), Y


def harmonic_amps(y: np.ndarray, fs: float, f0: float, n_harm: int = 9, skip: float = 0.5) -> np.ndarray:
    """Peak amplitudes of harmonics 1..n_harm below Nyquist (max over +-3 bins)."""
    f, Y = spectrum(y, fs, skip)
    df = f[1]
    out = []
    for h in range(1, n_harm + 1):
        fh = h * f0
        if fh >= fs / 2:
            break
        i = int(round(fh / df))
        out.append(Y[max(0, i - 3): i + 4].max())
    return np.array(out)


def thd(y: np.ndarray, fs: float, f0: float, n_harm: int = 9, skip: float = 0.5) -> float:
    h = harmonic_amps(y, fs, f0, n_harm, skip)
    return float(np.sqrt(np.sum(h[1:] ** 2)) / h[0])


def alias_energy_db(y: np.ndarray, fs: float, f0: float, skip: float = 0.5) -> float:
    """Energy of non-harmonic components (aliases), dB relative to the fundamental."""
    f, Y = spectrum(y, fs, skip)
    df = f[1]
    mask = np.ones_like(Y, bool)
    mask[: int(20 / df)] = False                       # ignore DC region
    h = 1
    while h * f0 < fs / 2:
        i = int(round(h * f0 / df))
        mask[max(0, i - 6): i + 7] = False
        h += 1
    fund = Y[int(round(f0 / df)) - 3: int(round(f0 / df)) + 4].max()
    return float(10 * np.log10(np.sum(Y[mask] ** 2) / fund ** 2 + 1e-30))
