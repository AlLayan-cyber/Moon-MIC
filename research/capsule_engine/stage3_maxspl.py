"""Stage 3: why does the model reach 1 % THD below the published 131 dB SPL?

Sensitivity study, no tuning: each case changes one uncertain input and reports the SPL at which the
tube stage reaches 1 % THD (1 kHz, 4x oversampling, Uni sensitivity -32 dBV/Pa from the sell sheet).
Cases
  tube law  : full fit (all digitised points) | low-current fit (Ip <= 7.25 mA, Ec1 >= -5 V, the region the
              C-800G actually uses)
  grid bias : 0 V (ideal leak) | -0.61 V (value that reproduces the schematic operating point exactly)
  R_L       : 49 kOhm (1 kOhm preamp x 7^2) | 100 kOhm | 1 MOhm (unloaded secondary)
Output: research/reports/stage3_maxspl.json
Run: .venv/Scripts/python.exe -m research.capsule_engine.stage3_maxspl
"""
from __future__ import annotations

import json
from pathlib import Path

import numpy as np
from scipy.optimize import least_squares

from .analysis import sine, thd
from .fit_6au6a import model, points
from .oversample import oversampled
from .triode import CommonCathodeStage, KorenTriode, StageCircuit

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "research" / "reports" / "stage3_maxspl.json"
SENS_V_PER_PA = 10 ** (-32 / 20)     # sell sheet A-1164-A, Uni


def fit(mask=None) -> KorenTriode:
    vg, vp, ip = points()
    if mask is not None:
        m = mask(vg, vp, ip)
        vg, vp, ip = vg[m], vp[m], ip[m]
    r = least_squares(lambda p: np.log(model(p, vg, vp) + 1e-7) - np.log(ip), [36.0, 1.4, 1000.0, 300.0, 300.0],
                      bounds=([5, 1.0, 10, 10, 1], [100, 2.0, 1e5, 5e3, 1e5]), x_scale="jac")
    return KorenTriode(*r.x, label=f"fit n={ip.size}")


def spl_for_1pct(tube: KorenTriode, circ: StageCircuit, grid_dc: float, fs: float = 48000.0) -> dict:
    st = CommonCathodeStage(tube, circ, fs, grid_dc)
    g = abs(st.small_signal_gain())
    grid_per_pa = SENS_V_PER_PA / g
    rows = []
    for spl in (118.0, 124.0, 131.0):
        amp = np.sqrt(2) * 10 ** ((spl - 94) / 20) * grid_per_pa
        s = CommonCathodeStage(tube, circ, fs * 4, grid_dc)
        y = oversampled(s.process, sine(1000.0, fs, 0.5, amp), 4)
        rows.append((spl, thd(y, fs, 1000.0, skip=0.25)))
    spl_arr = np.array([r[0] for r in rows]); t = np.log10([r[1] for r in rows])
    spl_1 = float(np.interp(-2.0, t, spl_arr, left=np.nan, right=np.nan)) if t[0] < -2 < t[-1] else (
        float(spl_arr[-1] + 20 * (-2 - t[-1])) if t[-1] < -2 else float(spl_arr[0] - 20 * (t[0] + 2)))
    op = st.operating_point()
    return {"Ip_mA": round(op["Ip_mA"], 3), "Vp_V": round(op["Vp_V"], 1), "Vk_V": round(op["Vk_V"], 3),
            "gain_grid_to_xlr_dB": round(20 * np.log10(g), 2), "grid_mV_per_Pa": round(grid_per_pa * 1e3, 2),
            "thd_pct": {int(s): round(v * 100, 3) for s, v in rows}, "spl_at_1pct_dB": round(spl_1, 1)}


def main() -> int:
    tubes = {"full_fit": fit(), "low_current_fit": fit(lambda vg, vp, ip: (ip <= 7.3e-3) & (vg >= -5))}
    out = {"published_max_spl_dB": 131.0, "cases": []}
    for tn, tube in tubes.items():
        for gdc in (0.0, -0.61):
            for rl in (49e3, 100e3, 1e6):
                r = spl_for_1pct(tube, StageCircuit(r_load=rl), gdc)
                case = {"tube": tn, "grid_dc_V": gdc, "R_L_kOhm": rl / 1e3, **r}
                out["cases"].append(case)
                print(case)
        out[tn] = {"mu": tube.mu, "ex": tube.ex, "kg1": tube.kg1, "kp": tube.kp, "kvb": tube.kvb}
    OUT.write_text(json.dumps(out, indent=1), encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
