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

    def ip_partials(self, vgk: float, vpk: float) -> tuple[float, float, float]:
        """(Ip, dIp/dVgk, dIp/dVpk), analytic."""
        if vpk <= 0.0:
            return 0.0, 0.0, 0.0
        q = self.kvb + vpk * vpk
        sq = np.sqrt(q)
        z = self.kp * (1.0 / self.mu + vgk / sq)
        sp = float(np.logaddexp(0.0, z))
        sig = 0.5 * (1.0 + np.tanh(0.5 * z))
        e1 = vpk / self.kp * sp
        if e1 <= 0.0:
            return 0.0, 0.0, 0.0
        de_dg = vpk * sig / sq
        de_dp = sp / self.kp - sig * vgk * vpk * vpk / (q * sq)
        ip = e1 ** self.ex / self.kg1
        k = self.ex * ip / e1
        return float(ip), float(k * de_dg), float(k * de_dp)


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

    def __init__(self, tube: KorenTriode, circuit: StageCircuit, fs: float, grid_dc: float = 0.0):
        """grid_dc: DC grid potential (V). 0 = ideal grid leak; a negative value represents contact-potential
        self-bias through the 500 MOhm leak (not measured for the C-800G; used only as a sensitivity case)."""
        self.t, self.c, self.fs = tube, circuit, float(fs)
        self.grid_dc = float(grid_dc)
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

        def resid_d(ip):
            vp, vk = nodes(ip)
            f, fg, fp = self.t.ip_partials(vg - vk, vp - vk)
            return ip - f, 1.0 + fg / ak + fp * (1.0 / ap + 1.0 / ak)

        lo, hi = 0.0, c.b_plus / c.ra + abs(gb * vc_hist) + 1e-3
        self.iterations = 0
        if resid(lo) >= 0.0:                    # tube cut off
            vp, vk = nodes(0.0)
            return 0.0, vp, vk
        for _ in range(8):                      # widen (never needed with the C-800G values)
            if resid(hi) > 0.0:
                break
            hi *= 2.0
        x = self.ip_guess if lo < self.ip_guess < hi else 0.5 * (lo + hi)   # warm start: previous sample
        for _ in range(self.MAX_ITER):
            self.iterations += 1
            r, d = resid_d(x)
            if r > 0.0:
                hi = x
            else:
                lo = x
            xn = x - r / d if d > 0.0 else 0.5 * (lo + hi)
            if abs(xn - x) <= 1e-12 * max(x, 1e-6):    # converged (checked before the bracket guard:
                x = xn                                 # an exact root sits on a bracket end)
                break
            if not (lo < xn < hi):
                xn = 0.5 * (lo + hi)
            x = xn
        self.ip_guess = x
        vp, vk = nodes(x)
        return x, vp, vk

    def reset(self) -> None:
        """DC operating point (capacitors open)."""
        self.ip_guess = -1.0
        ip, vp, vk = self._solve(self.grid_dc, 0.0, 0.0, 0.0, 0.0)
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
            ip, vp, vk = self._solve(float(g) + self.grid_dc, jk, self.gk, vc_hist, gb)
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
        f, vg0, vpk0 = self.t.ip, self.grid_dc - self.vk0, self.vp0 - self.vk0      # operating point Vgk
        gm = (f(vg0 + h, vpk0) - f(vg0 - h, vpk0)) / (2 * h)
        gp = (f(vg0, vpk0 + h) - f(vg0, vpk0 - h)) / (2 * h)
        r_ac = 1.0 / (1.0 / self.c.ra + 1.0 / self.c.r_load)
        return -gm / (gp + 1.0 / r_ac) / self.c.turns
