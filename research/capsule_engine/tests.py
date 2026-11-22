"""Verification suite for the Stage 2 reference (VALIDATION_PLAN_RU.md §1-3).

Every check compares against an analytic result or a property that must hold; no check is tuned to
make a model "sound like" anything. Tube constants are TEST-ONLY (see triode.py).

Run from the project root:  .venv/Scripts/python.exe -m research.capsule_engine.tests
"""
from __future__ import annotations

import sys

import numpy as np

from .analysis import alias_energy_db, harmonic_amps, sine, thd
from .capsule import ConstantChargeCapsule
from .oversample import oversampled
from .triode import TEST_ONLY_12AX7, CommonCathodeStage, StageCircuit

RESULTS: list[tuple[str, bool, str]] = []


def check(name: str, ok: bool, info: str) -> None:
    RESULTS.append((name, bool(ok), info))
    print(f"[{'PASS' if ok else 'FAIL'}] {name}: {info}")


def capsule_tests() -> None:
    fs, f0 = 48000.0, 1000.0
    p = sine(f0, fs, 1.0, amp=np.sqrt(2) * 100.0)          # 100 Pa rms = 134 dB SPL
    # 1. piston, no stray capacitance: exactly linear
    cap = ConstantChargeCapsule(u_peak_94=1e-3, cp_ratio=0.0, shape="piston")   # u = 0.1 at 134 dB SPL
    y = cap(p)
    check("capsule piston Cp=0 linear", thd(y, fs, f0) < 1e-9, f"THD={thd(y, fs, f0):.2e}")
    # 2. piston with Cp = C0: H2/H1 = (1-c) a / 2, c = 0.5
    cap = ConstantChargeCapsule(u_peak_94=1e-4, cp_ratio=1.0, shape="piston")
    a = cap.k * np.sqrt(2) * 100.0
    h = harmonic_amps(cap(p), fs, f0)
    want = 0.5 * a / 2
    check("capsule piston Cp=C0 H2/H1 analytic", abs(h[1] / h[0] / want - 1) < 0.01,
          f"H2/H1={h[1] / h[0]:.4e} analytic={want:.4e}")
    # 3. clamped (parabolic) shape, Cp=0: C/C0 = -ln(1-u)/u  ->  H2/H1 = a/12
    cap = ConstantChargeCapsule(u_peak_94=1e-4, cp_ratio=0.0, shape="clamped")
    a = cap.k * np.sqrt(2) * 100.0
    h = harmonic_amps(cap(p), fs, f0)
    check("capsule clamped shape H2/H1 analytic", abs(h[1] / h[0] / (a / 12) - 1) < 0.02,
          f"H2/H1={h[1] / h[0]:.4e} analytic={a / 12:.4e}")
    # 4. guard: extreme input stays finite
    cap = ConstantChargeCapsule(u_peak_94=0.04, cp_ratio=0.3, shape="clamped")
    y = cap(np.array([1e9, -1e9, 1e6]))
    check("capsule extreme input finite", np.all(np.isfinite(y)), f"y={y}")


def analog_gain(stage: CommonCathodeStage, f: float) -> complex:
    """Small-signal analogue transfer grid -> secondary for the same circuit (Laplace domain)."""
    t, c = stage.t, stage.c
    vpk0, vg0 = stage.vp0 - stage.vk0, -stage.vk0
    h = 1e-6
    gm = (t.ip(vg0 + h, vpk0) - t.ip(vg0 - h, vpk0)) / (2 * h)
    gp = (t.ip(vg0, vpk0 + h) - t.ip(vg0, vpk0 - h)) / (2 * h)
    s = 2j * np.pi * f
    zk = 1.0 / (1.0 / c.rk + s * c.ck)
    zb = c.r_load + 1.0 / (s * c.cc)
    zl = 1.0 / (1.0 / c.ra + 1.0 / zb)
    ip = gm / (1.0 + gm * zk + gp * (zl + zk))
    vp = -ip * zl
    return vp * c.r_load / zb / c.turns


def measured_gain(stage: CommonCathodeStage, f: float, fs: float, amp: float = 1e-4) -> complex:
    stage.reset()
    secs = max(2.0, 12.0 / f)
    x = sine(f, fs, secs, amp)
    y = stage.process(x)
    n = int(len(x) / 2)
    tt = np.arange(len(x))[n:] / fs
    ref = np.exp(-2j * np.pi * f * tt)
    # projection on exp(j w t) of the steady second half; input is amp*sin -> complex amp -j*amp/... use ratio
    X = np.sum(x[n:] * ref)
    Y = np.sum(y[n:] * ref)
    return Y / X


def triode_tests() -> None:
    circ = StageCircuit()
    for fs in (44100.0, 48000.0, 96000.0, 192000.0):
        st = CommonCathodeStage(TEST_ONLY_12AX7, circ, fs)
        for f in (5.0, 50.0, 1000.0):
            g_sim = measured_gain(st, f, fs)
            g_ana = analog_gain(st, f)
            err = 20 * np.log10(abs(g_sim) / abs(g_ana))
            ph = np.degrees(np.angle(g_sim / g_ana))
            check(f"stage small-signal vs analogue fs={fs:.0f} f={f:g}", abs(err) < 0.02 and abs(ph) < 0.5,
                  f"{err:+.4f} dB, {ph:+.3f} deg")
    st = CommonCathodeStage(TEST_ONLY_12AX7, circ, 48000.0)
    op = st.operating_point()
    vpk = op["Vp_V"] - op["Vk_V"]
    kcl = abs((circ.b_plus - op["Vp_V"]) / circ.ra - op["Ip_mA"] * 1e-3) + abs(op["Vk_V"] / circ.rk - op["Ip_mA"] * 1e-3)
    check("stage DC operating point KCL", kcl < 1e-12, f"{op}, residual {kcl:.1e} A (Vpk={vpk:.2f} V)")
    # impulse: decays, finite
    st.reset()
    x = np.zeros(int(4 * 48000)); x[10] = 1e-2
    y = st.process(x)
    tail = np.max(np.abs(y[-4800:]))
    check("stage impulse response decays", np.all(np.isfinite(y)) and tail < 1e-3 * np.max(np.abs(y)),
          f"peak {np.max(np.abs(y)):.3e}, last-100ms max {tail:.3e}")
    # extreme drive stays finite and bounded by the supply
    st.reset()
    y = st.process(sine(100.0, 48000.0, 0.5, amp=500.0))
    check("stage extreme drive finite", np.all(np.isfinite(y)) and np.max(np.abs(y)) < circ.b_plus,
          f"max |out| {np.max(np.abs(y)):.2f} V")
    # level series: THD rises with level (6 dB steps), measured at 1 kHz with 4x oversampling
    fs = 48000.0
    rows = []
    for lv_db in range(-36, 1, 6):
        amp = 1.0 * 10 ** (lv_db / 20)
        st = CommonCathodeStage(TEST_ONLY_12AX7, circ, fs * 4)
        y = oversampled(st.process, sine(1000.0, fs, 1.0, amp), 4)
        rows.append((lv_db, thd(y, fs, 1000.0)))
    mono = all(b[1] > a[1] for a, b in zip(rows, rows[1:]))
    check("stage THD monotonic with level", mono, ", ".join(f"{l:+d}dB:{t * 100:.3f}%" for l, t in rows))


def aliasing_tests() -> None:
    fs, f0 = 48000.0, 4567.0
    circ = StageCircuit()
    out = {}
    for k in (1, 2, 4, 8):
        st = CommonCathodeStage(TEST_ONLY_12AX7, circ, fs * k)
        y = oversampled(st.process, sine(f0, fs, 1.0, amp=1.5), k)
        out[k] = alias_energy_db(y, fs, f0)
    dec = out[1] > out[2] > out[4]
    check("aliasing falls with oversampling", dec, ", ".join(f"{k}x:{v:.1f} dB" for k, v in out.items()))


def main() -> int:
    capsule_tests()
    triode_tests()
    aliasing_tests()
    n_fail = sum(not ok for _, ok, _ in RESULTS)
    print(f"\n{len(RESULTS) - n_fail}/{len(RESULTS)} checks passed")
    return 1 if n_fail else 0


if __name__ == "__main__":
    sys.exit(main())
