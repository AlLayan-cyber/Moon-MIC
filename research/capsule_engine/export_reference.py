"""Export reference vectors from the Python model for the C++ test bench (Stage 5).

Writes <out>/manifest.json and raw little-endian float64 files <case>_in.f64 / <case>_out.f64.
Run: .venv/Scripts/python.exe -m research.capsule_engine.export_reference <out_dir>
"""
from __future__ import annotations

import json
import sys
from pathlib import Path

import numpy as np

from .analysis import sine
from .capsule import ConstantChargeCapsule
from .triode import CommonCathodeStage, KorenTriode, StageCircuit

ROOT = Path(__file__).resolve().parents[2]


def tube_6au6a() -> KorenTriode:
    k = json.loads((ROOT / "data" / "manufacturer" / "tube_6au6a_triode.json").read_text(encoding="utf-8"))["koren"]
    return KorenTriode(k["mu"], k["ex"], k["kg1"], k["kp"], k["kvb"], "6AU6A fit")


def main() -> int:
    out = Path(sys.argv[1]); out.mkdir(parents=True, exist_ok=True)
    t = tube_6au6a()
    cases = []

    def stage_case(name, x, fs, r_load=100e3, grid_dc=0.0):
        st = CommonCathodeStage(t, StageCircuit(r_load=r_load), fs, grid_dc)
        op = st.operating_point()
        y = st.process(x)
        x.astype("<f8").tofile(out / f"{name}_in.f64"); y.astype("<f8").tofile(out / f"{name}_out.f64")
        cases.append({"name": name, "kind": "stage", "fs": fs, "r_load": r_load, "grid_dc": grid_dc, "n": int(x.size),
                      "op": {"ip": op["Ip_mA"] * 1e-3, "vp": op["Vp_V"], "vk": op["Vk_V"]}})
        print(name, x.size, "samples, peak out", float(np.max(np.abs(y))))

    def capsule_case(name, p, u, cp, shape):
        cap = ConstantChargeCapsule(u, cp, shape)
        y = cap(p)
        p.astype("<f8").tofile(out / f"{name}_in.f64"); y.astype("<f8").tofile(out / f"{name}_out.f64")
        cases.append({"name": name, "kind": "capsule", "u_peak_94": u, "cp_ratio": cp, "shape": shape, "n": int(p.size)})
        print(name, p.size)

    fs = 48000.0
    stage_case("sine1k_small", sine(1000.0, fs, 0.25, 0.01), fs)
    stage_case("sine100_large", sine(100.0, fs, 0.25, 0.6), fs)
    imp = np.zeros(12000); imp[100] = 0.5
    stage_case("impulse", imp, fs)
    tt = np.arange(int(0.25 * fs)) / fs
    stage_case("multitone", 0.2 * (np.sin(2 * np.pi * 100 * tt) + np.sin(2 * np.pi * 1000 * tt) + np.sin(2 * np.pi * 5000 * tt)), fs)
    stage_case("overload", sine(100.0, fs, 0.1, 500.0), fs)
    stage_case("sine1k_96k_rl49k_bias", sine(1000.0, 96000.0, 0.25, 0.3), 96000.0, r_load=49e3, grid_dc=-0.61)
    p = sine(1000.0, fs, 0.05, np.sqrt(2) * 100.0)
    capsule_case("capsule_piston_cp1", p, 1e-3, 1.0, "piston")
    capsule_case("capsule_clamped_cp0", p, 1e-3, 0.0, "clamped")
    lines = []
    for c in cases:
        if c["kind"] == "stage":
            lines.append(f"stage {c['name']} {c['fs']!r} {c['r_load']!r} {c['grid_dc']!r} {c['n']} "
                         f"{c['op']['ip']!r} {c['op']['vp']!r} {c['op']['vk']!r}")
        else:
            lines.append(f"capsule {c['name']} {c['u_peak_94']!r} {c['cp_ratio']!r} {c['shape']} {c['n']}")
    (out / "manifest.txt").write_text("\n".join(lines) + "\n", encoding="ascii")
    (out / "manifest.json").write_text(json.dumps({"tube": {"mu": t.mu, "ex": t.ex, "kg1": t.kg1, "kp": t.kp, "kvb": t.kvb},
                                                   "cases": cases}, indent=1), encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
