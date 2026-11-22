"""Triode law + common-cathode stage, solved sample by sample.

Topology = Sony C-800G MA-55 board (service manual schematic 3-2, VERIFIED values):
    B+ ---- Ra (R104 100k) ---- plate ---- C104 (1 uF) ---- transformer primary (modelled as R_L) ---- gnd
    cathode ---- Rk (R105 1k) || Ck (C102 330 uF) ---- gnd
    grid    ---- capsule signal (DC 0 V through R103 500 MOhm)
B+ is ideal (C103 decoupling and the external supply impedance are not modelled).

NOT known and therefore parameters, never defaults pretending to be data:
    tube law constants for a triode-connected 6AU6A  (fit to a manufacturer datasheet: pending)
    R_L, the primary-side load (depends on the transformer and on the preamp input impedance)
    grid current (not modelled at Stage 2a; required before the 131 dB SPL max-SPL check)

Discretisation: trapezoidal (bilinear) companion models for C104 and Ck, so the linear part maps to the
analogue prototype exactly up to frequency warping. The only non-linear unknown per sample is the plate
current Ip; the residual r(Ip) = Ip - f(Vgk(Ip), Vpk(Ip)) is strictly increasing, so a bracketed Newton
iteration with a fixed maximum iteration count has a bounded worst case (realtime requirement).
"""
from __future__ import annotations

from dataclasses import dataclass

import numpy as np


@dataclass(frozen=True)
class KorenTriode:
    """Norman Koren's phenomenological triode model.
    E1 = Vpk/kp * ln(1 + exp(kp * (1/mu + Vgk / sqrt(kvb + Vpk^2)))),  Ip = E1^ex / kg1  (E1 > 0)."""
    mu: float
    ex: float
    kg1: float
    kp: float
    kvb: float
    label: str

    def ip(self, vgk: float, vpk: float) -> float:
        if vpk <= 0.0:
            return 0.0
        z = self.kp * (1.0 / self.mu + vgk / np.sqrt(self.kvb + vpk * vpk))
        e1 = vpk / self.kp * (np.logaddexp(0.0, z))      # ln(1 + e^z), overflow-safe
        return float(e1 ** self.ex / self.kg1) if e1 > 0.0 else 0.0


# TEST-ONLY: Koren's published 12AX7 constants (a different tube). Used solely to verify numerics.
TEST_ONLY_12AX7 = KorenTriode(mu=100.0, ex=1.4, kg1=1060.0, kp=600.0, kvb=300.0,
                              label="TEST-ONLY Koren 12AX7 (not 6AU6A)")


@dataclass(frozen=True)
class StageCircuit:
    b_plus: float = 230.0      # V    schematic 3-2 ("230V")
    ra: float = 100e3          # Ohm  R104
    rk: float = 1e3            # Ohm  R105
    ck: float = 330e-6         # F    C102
    cc: float = 1e-6           # F    C104
    r_load: float = 100e3      # Ohm  primary-side load: UNKNOWN, parameter
    turns: float = 7.0         # T101 ratio 7:1 (JAS p.34); step-down orientation assumed


class CommonCathodeStage:
    MAX_ITER = 40

    def __init__(self, tube: KorenTriode, circuit: StageCircuit, fs: float):
        self.t, self.c, self.fs = tube, circuit, float(fs)
        T = 1.0 / self.fs
        self.gk = 2.0 * circuit.ck / T          # cathode capacitor companion conductance
        self.rc = T / (2.0 * circuit.cc)        # coupling capacitor companion resistance
        self.reset()

    # ---- operating point -------------------------------------------------------------------------
    def _solve(self, vg: float, jk: float, gk: float, vc_hist: float, gb: float) -> tuple[float, float, float]:
        """Return (Ip, Vp, Vk). Plate node:  (B - Vp)/Ra = Ip + gb*(Vp - vc_hist)
                                   cathode:  Ip = Vk/Rk + gk*Vk - jk"""
        c = self.c
        ak = 1.0 / c.rk + gk
        ap = 1.0 / c.ra + gb

        def nodes(ip):
            vk = (ip + jk) / ak
            vp = (c.b_plus / c.ra - ip + gb * vc_hist) / ap
            return vp, vk

        def resid(ip):
            vp, vk = nodes(ip)
            return ip - self.t.ip(vg - vk, vp - vk)

        lo, hi = 0.0, c.b_plus / c.ra + abs(gb * vc_hist) / 1.0 + 1e-3
        if resid(lo) >= 0.0:                    # tube cut off
            vp, vk = nodes(0.0)
            return 0.0, vp, vk
        while resid(hi) <= 0.0:                 # widen (never more than a few steps in practice)
            hi *= 2.0
        x = 0.5 * (lo + hi)
        for _ in range(self.MAX_ITER):
            r = resid(x)
            if r > 0.0:
                hi = x
            else:
                lo = x
            h = 1e-9 * max(1e-6, x)
            d = (resid(x + h) - r) / h
            xn = x - r / d if d > 0.0 else 0.5 * (lo + hi)
            if not (lo < xn < hi):
                xn = 0.5 * (lo + hi)
            if abs(xn - x) < 1e-13:
                x = xn
                break
            x = xn
        vp, vk = nodes(x)
        return x, vp, vk

    def reset(self) -> None:
        """DC operating point (capacitors open)."""
        ip, vp, vk = self._solve(0.0, 0.0, 0.0, 0.0, 0.0)
        self.ip0, self.vp0, self.vk0 = ip, vp, vk
        self.vk, self.ick = vk, 0.0             # cathode capacitor: voltage, current
        self.vc, self.ib = vp, 0.0              # coupling capacitor voltage (plate side minus load side), branch current

    def operating_point(self) -> dict:
        return {"Ip_mA": self.ip0 * 1e3, "Vp_V": self.vp0, "Vk_V": self.vk0}

    # ---- processing ------------------------------------------------------------------------------
    def process(self, vg: np.ndarray) -> np.ndarray:
        """Grid voltage (V, AC) -> secondary voltage (V)."""
        c = self.c
        out = np.empty(len(vg))
        gb = 1.0 / (c.r_load + self.rc)
        for n, g in enumerate(vg):
            jk = self.gk * self.vk + self.ick                  # trapezoid history, cathode capacitor
            vc_hist = self.vc + self.rc * self.ib              # coupling capacitor history
            ip, vp, vk = self._solve(float(g), jk, self.gk, vc_hist, gb)
            ib = gb * (vp - vc_hist)
            self.ick = self.gk * vk - jk
            self.vk = vk
            self.vc = vc_hist + self.rc * ib
            self.ib = ib
            out[n] = ib * c.r_load / c.turns
        return out

    def small_signal_gain(self) -> float:
        """Analytic mid-band gain (bypassed cathode, coupling C short): -gm (ra_ || rp)."""
        h = 1e-6
        f, vg0, vpk0 = self.t.ip, -self.vk0, self.vp0 - self.vk0      # operating point: Vgk = -Vk (grid at 0 V DC)
        gm = (f(vg0 + h, vpk0) - f(vg0 - h, vpk0)) / (2 * h)
        gp = (f(vg0, vpk0 + h) - f(vg0, vpk0 - h)) / (2 * h)
        r_ac = 1.0 / (1.0 / self.c.ra + 1.0 / self.c.r_load)
        return -gm / (gp + 1.0 / r_ac) / self.c.turns
