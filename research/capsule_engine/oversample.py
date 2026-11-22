"""Oversampling wrapper for memoryless or stateful non-linear blocks (offline reference).

Uses polyphase FIR resampling (scipy.signal.resample_poly, Kaiser window, beta 8). The realtime C++ version
will use fixed half-band cascades; this reference only has to be accurate, not fast.
"""
from __future__ import annotations

from typing import Callable

import numpy as np
from scipy.signal import resample_poly


def oversampled(process: Callable[[np.ndarray], np.ndarray], x: np.ndarray, factor: int) -> np.ndarray:
    if factor == 1:
        return process(np.asarray(x, float))
    up = resample_poly(np.asarray(x, float), factor, 1, window=("kaiser", 8.0))
    y = process(up)
    return resample_poly(y, 1, factor, window=("kaiser", 8.0))
