"""Fit Koren constants to the GE 6AU6-A triode-connection plate characteristics.

Source: docs/manufacturer_data/tube_6au6a/6au6a_general_electric.pdf (GE ET-T916A, 4-60), page 4,
"Average Plate Characteristics — Triode Connection" (screen and suppressor tied to plate).
Points: curve crossings at fixed plate currents, read from a scale-5 render (grid columns removed,
run centres), only rows where the number of crossings equals the number of curves present.
Reading accuracy about +-2 V (INFERRED). Output: data/manufacturer/tube_6au6a_triode_points.json + fit.
Run: .venv/Scripts/python.exe -m research.capsule_engine.fit_6au6a
"""
from __future__ import annotations

import json
from pathlib import Path

import numpy as np
from scipy.optimize import least_squares

from .triode import KorenTriode

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "data" / "manufacturer" / "tube_6au6a_triode.json"

# plate current (mA) -> plate voltages (V) of the curves Ec1 = 0, -1, -2, ... (left to right)
ROWS = {
    0.75: [8.8, 45.1, 75.6, 108.8, 144.7, 172.2, 198.7, 232.4, 257.6, 287.9, 317.4, 343.8, 372.2],
    1.25: [13.5, 53.4, 84.9, 118.6, 154.2, 182.1, 211.6, 242.9, 269.9, 301.5, 332.0, 357.4, 387.1],
    2.25: [25.8, 66.4, 99.6, 133.6, 169.0, 197.4, 228.1, 261.4, 289.2, 320.8, 351.9, 377.9, 407.4],
    3.25: [34.1, 76.2, 111.2, 143.9, 180.7, 209.6, 241.6, 273.1, 303.4, 335.8, 366.8, 393.7, 423.4],
    4.75: [47.3, 88.9, 125.3, 159.1, 195.3, 226.3, 258.2, 290.9, 321.2, 353.0, 385.2, 413.0],
    7.25: [66.0, 108.4, 146.0, 179.1, 216.2, 248.5, 281.7, 313.7, 345.0, 378.2, 410.1],
    12.25: [96.3, 139.3, 178.0, 212.9, 251.1, 284.0, 318.1, 351.0, 383.0],
    15.25: [112.8, 156.3, 194.9, 231.0, 269.0, 302.7, 336.9, 370.4, 400.2],
    20.25: [138.1, 182.1, 221.5, 258.6, 297.2, 331.0, 365.9],
    25.25: [161.9, 206.1, 246.0, 284.0, 323.8, 353.9],
    27.75: [169.9, 217.4, 257.2, 296.6, 330.8],
}
# GE ET-T916A p.2, triode connection, typical operation (VERIFIED): Eb 250 V, Rk 330 Ohm,
# mu 36, gm 4800 umho, Ib 12.2 mA
TYPICAL = {"eb": 250.0, "rk": 330.0, "mu": 36.0, "gm": 4.8e-3, "ib": 12.2e-3}


def points():
    vg, vp, ip = [], [], []
    for i_ma, vs in ROWS.items():
        for k, v in enumerate(vs):
            vg.append(-float(k)); vp.append(v); ip.append(i_ma * 1e-3)
    return np.array(vg), np.array(vp), np.array(ip)


def model(p, vg, vp):
    mu, ex, kg1, kp, kvb = p
    t = KorenTriode(mu, ex, kg1, kp, kvb, "fit")
    return np.array([t.ip(a, b) for a, b in zip(vg, vp)])


def main() -> int:
    vg, vp, ip = points()

    def res(p):
        return np.log(model(p, vg, vp) + 1e-7) - np.log(ip)   # relative error, all currents weighted alike

    p0 = [36.0, 1.4, 1000.0, 300.0, 300.0]
    fit = least_squares(res, p0, bounds=([5, 1.0, 10, 10, 1], [100, 2.0, 1e5, 5e3, 1e5]), x_scale="jac")
    mu, ex, kg1, kp, kvb = fit.x
    t = KorenTriode(mu, ex, kg1, kp, kvb, "6AU6A triode, fit to GE ET-T916A p.4")
    pred = model(fit.x, vg, vp)
    rel = (pred - ip) / ip
    # voltage error: for each point, plate voltage at which the model gives the target current
    verr = []
    for a, b, i in zip(vg, vp, ip):
        lo, hi = 0.0, 600.0
        for _ in range(60):
            mid = 0.5 * (lo + hi)
            lo, hi = (mid, hi) if t.ip(a, mid) < i else (lo, mid)
        verr.append(0.5 * (lo + hi) - b)
    verr = np.array(verr)
    # typical-operation check (not used in the fit): find Ib with Rk = 330 at Eb = 250
    lo, hi = 0.0, 0.05
    for _ in range(80):
        mid = 0.5 * (lo + hi)
        f = t.ip(-mid * TYPICAL["rk"], TYPICAL["eb"] - mid * TYPICAL["rk"]) - mid
        lo, hi = (mid, hi) if f > 0 else (lo, mid)
    ib = 0.5 * (lo + hi)
    vgk, vpk, h = -ib * TYPICAL["rk"], TYPICAL["eb"] - ib * TYPICAL["rk"], 1e-4
    gm = (t.ip(vgk + h, vpk) - t.ip(vgk - h, vpk)) / (2 * h)
    gp = (t.ip(vgk, vpk + h) - t.ip(vgk, vpk - h)) / (2 * h)
    # C-800G operating point from the schematic: what grid-cathode voltage gives Ia = 1.4 mA at Vpk = 88.6 V?
    lo, hi = -10.0, 0.0
    for _ in range(80):
        mid = 0.5 * (lo + hi)
        lo, hi = (lo, mid) if t.ip(mid, 88.6) > 1.4e-3 else (mid, hi)
    vgk_needed = 0.5 * (lo + hi)
    ia_at_schematic_bias = t.ip(-1.4, 88.6)
    out = {
        "source": "docs/manufacturer_data/tube_6au6a/6au6a_general_electric.pdf p.4 (triode connection), digitised",
        "status": "INFERRED (fit to digitised average curves, reading +-2 V)",
        "koren": {"mu": mu, "ex": ex, "kg1": kg1, "kp": kp, "kvb": kvb},
        "n_points": int(ip.size),
        "fit_error": {"current_rel_rms": float(np.sqrt(np.mean(rel ** 2))), "current_rel_max": float(np.max(np.abs(rel))),
                      "plate_voltage_rms_V": float(np.sqrt(np.mean(verr ** 2))), "plate_voltage_max_V": float(np.max(np.abs(verr)))},
        "check_typical_operation_GE_p2": {"published": {"ib_mA": 12.2, "mu": 36, "gm_umho": 4800},
                                          "model": {"ib_mA": ib * 1e3, "mu": gm / gp, "gm_umho": gm * 1e6}},
        "c800g_schematic": {"Ia_at_Vgk_-1.4_Vpk_88.6_mA": ia_at_schematic_bias * 1e3,
                            "Vgk_needed_for_1.4mA_V": vgk_needed,
                            "implied_grid_dc_V": vgk_needed + 1.4},
        "points": {"vg": vg.tolist(), "vp": vp.tolist(), "ip_mA": (ip * 1e3).tolist()},
    }
    OUT.write_text(json.dumps(out, indent=1), encoding="utf-8")
    print(json.dumps({k: v for k, v in out.items() if k != "points"}, indent=1))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
