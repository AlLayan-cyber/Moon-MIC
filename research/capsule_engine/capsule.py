"""Constant-charge condenser capsule: displacement -> open-circuit voltage at the first grid.

Physics (quasi-static, below the diaphragm resonance; the linear acoustic response is NOT modelled here,
it stays the measured response of the SIGNATURE path):

  The capsule is charged through a very large resistor (C-800G: R103 = 500 MOhm, schematic 3-2), so over an
  audio period the charge Q is constant. Local capacitance of an annular element: dC = eps dA / (d0 - x(r)).
  With a parallel stray / input capacitance Cp the grid voltage is

      V(t) = Q / (C(t) + Cp),     C(t) = integral_A eps / (d0 - w(r) * x(t)) dA

  where x(t) is the centre displacement and w(r) the normalised deflection shape (w(0) = 1).

  * Uniform (piston) deflection and Cp = 0  ->  V = Q (d0 - x) / (eps A): exactly linear in x.
  * Cp > 0 or a non-uniform shape          ->  second- and higher-order terms.

  Small-signal series for the piston case (u = x / d0, c = C0 / (C0 + Cp)):
      V / V0 = 1 - c u + (c^2 - c) u^2 + O(u^3)
  so for u = a sin(wt): H2 / H1 = (1 - c) a / 2. Used as an analytic check in tests.py.

  Electrostatic force (spring softening, pull-in) is not part of this quasi-static block: at constant charge
  the force on a piston is independent of x (F = Q^2 / (2 eps A)), so it only shifts the static rest position.
  For a non-uniform membrane the force does vary; that belongs to the modal model (later level).

Parameters are dimensionless so that unknown physical values are never needed explicitly:
  u_peak_94   centre displacement / gap at 94 dB SPL (1 Pa), peak       -> UNKNOWN for C-800G (range study)
  cp_ratio    Cp / C0                                                   -> UNKNOWN for C-800G (range study)
  shape       "piston" or "clamped" (w = 1 - (r/a)^2, ideal tensioned membrane fundamental, approx.)
"""
from __future__ import annotations

import numpy as np

# radial quadrature for the shape integral (normalised radius rho in [0, 1], area weight 2 rho)
_RHO = (np.arange(64) + 0.5) / 64.0
_WA = 2.0 * _RHO / 64.0


def shape_profile(shape: str) -> np.ndarray:
    if shape == "piston":
        return np.ones_like(_RHO)
    if shape == "clamped":
        return 1.0 - _RHO ** 2          # parabolic approximation of the fundamental mode of a tensioned membrane
    raise ValueError(shape)


def capacitance_ratio(u: np.ndarray, shape: str = "piston") -> np.ndarray:
    """C(u) / C0 for centre displacement u = x / d0 (u > 0 towards the backplate)."""
    w = shape_profile(shape)
    u = np.asarray(u, float)
    return (_WA[None, :] / (1.0 - u[..., None] * w[None, :])).sum(axis=-1) if u.ndim else float(
        (_WA / (1.0 - u * w)).sum())


class ConstantChargeCapsule:
    """Maps a pressure-proportional signal p (Pa) to a normalised grid voltage (V / V0 - 1)."""

    def __init__(self, u_peak_94: float, cp_ratio: float, shape: str = "piston"):
        if not (0.0 <= u_peak_94 < 0.05):
            raise ValueError("u_peak_94 outside the small-deflection validity range [0, 0.05)")
        self.k = u_peak_94 / np.sqrt(2.0)      # u per Pa (rms 1 Pa = 94 dB SPL -> peak sqrt2)
        self.cp = float(cp_ratio)
        self.shape = shape
        self.c_total0 = capacitance_ratio(np.array(0.0), shape) + self.cp

    def __call__(self, p: np.ndarray) -> np.ndarray:
        u = np.clip(self.k * np.asarray(p, float), -0.5, 0.5)   # hard guard far outside validity, never NaN
        c = capacitance_ratio(u, self.shape) + self.cp
        return self.c_total0 / c - 1.0                           # V/V0 - 1; negative for u > 0 (gap closes)

    def small_signal_gain(self) -> float:
        """d(V/V0)/dp at p = 0 (per Pa)."""
        h = 1e-6
        return float((self(np.array([h])) - self(np.array([-h])))[0] / (2 * h))
