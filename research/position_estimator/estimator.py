"""Source-distance estimator v1 for TRANSFORM (docs/architecture/06): classical, interpretable, no network.

Model of a recorded take (dB, power, long-term):   L(f) = V(f) + M_src(f, r)
  V      voice LTAS, unknown -> statistical prior (mean and per-frequency spread over training takes)
  M_src  magnitude of the KNOWN source-mic model at distance r (same model the plugin uses), angle 0
Estimate: r_hat = argmin_r sum_f [ (L - M_src(r)) - mu(f) ]^2 / s(f)^2, after removing the mean over a
reference band from both sides, so absolute level and the broad voice tilt above it do not matter.

v1 choices (from the first experiment, research/reports/POSITION_ESTIMATOR_V1_REPORT.md):
  * reference band 700-1400 Hz (a 2-5 kHz reference let the voice's spectral tilt, +-10 dB, swamp the signal);
  * evaluation band from max(80 Hz, 1.2 x median f0) to 700 Hz (where proximity acts);
  * prior conditioned on f0: only training takes within +-1/4 octave of the take's median f0.
Confidence: posterior ~ exp(-J/2) over log r -> 68 % interval. Omni sources give a flat posterior.
"""
from __future__ import annotations

import numpy as np

R_GRID = np.exp(np.linspace(np.log(0.03), np.log(1.5), 81))     # 3 cm .. 1.5 m
REF_BAND = (700.0, 1400.0)
EVAL_HI = 700.0
F0_HALF_WIDTH_OCT = 0.25
MIN_PRIOR_TAKES = 30


def normalise(db: np.ndarray, fg: np.ndarray) -> np.ndarray:
    m = (fg >= REF_BAND[0]) & (fg <= REF_BAND[1])
    return db - np.nanmean(db[..., m], axis=-1, keepdims=True)


class Prior:
    """f0-local mean and spread of normalised voice LTAS."""

    def __init__(self, takes: list[dict], fg: np.ndarray):
        self.fg = fg
        self.ltas = np.array([normalise(t["ltas_db"], fg) for t in takes])
        self.logf0 = np.log2(np.array([t["f0_median"] for t in takes]))
        self.cache: dict[int, tuple[np.ndarray, np.ndarray]] = {}

    def at(self, f0: float) -> tuple[np.ndarray, np.ndarray]:
        key = int(round(np.log2(f0) * 24))
        if key not in self.cache:
            d = np.abs(self.logf0 - np.log2(f0))
            idx = np.flatnonzero(d <= F0_HALF_WIDTH_OCT)
            if idx.size < MIN_PRIOR_TAKES:
                idx = np.argsort(d)[:MIN_PRIOR_TAKES]
            a = self.ltas[idx]
            self.cache[key] = (np.nanmean(a, axis=0), np.maximum(np.nanstd(a, axis=0), 1.0))
        return self.cache[key]


def estimate(ltas_db: np.ndarray, f0: float, prior: Prior, model_db: np.ndarray) -> dict:
    """model_db: (len(R_GRID), n_freq) source-mic magnitude in dB at each grid distance."""
    fg = prior.fg
    mu, sd = prior.at(f0)
    lo = max(80.0, 1.2 * f0)
    band = (fg >= lo) & (fg <= EVAL_HI) & np.isfinite(ltas_db) & np.isfinite(mu)
    if band.sum() < 10:
        return {"r_hat": np.nan, "lo68": np.nan, "hi68": np.nan, "width_oct": np.inf}
    resid = normalise(ltas_db[None, :] - model_db, fg)
    J = np.nansum(((resid[:, band] - mu[band]) / sd[band]) ** 2, axis=1)
    J = J / 16.0          # 1/48-octave points -> ~1/3-octave independent bands
    post = np.exp(-(J - J.min()) / 2)
    post /= post.sum()
    cdf = np.cumsum(post)
    r_hat = float(R_GRID[int(np.argmin(J))])
    lo68 = float(np.interp(0.16, cdf, R_GRID)); hi68 = float(np.interp(0.84, cdf, R_GRID))
    return {"r_hat": r_hat, "lo68": lo68, "hi68": hi68, "width_oct": float(np.log2(hi68 / lo68))}
