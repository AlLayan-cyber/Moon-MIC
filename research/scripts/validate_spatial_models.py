"""Phase 1 validation of the spatial model (docs/architecture/03) on Surrey data.

Test A - dual-diaphragm consistency: can the measured Cardioid be built from measured
         Omni and Figure-8 of the same mic and distance?  C ~= gO*O + gB*B
         (gO, gB: two complex scalars per set, because Surrey levels are relative).
Test B - distance / proximity prediction: fit first-order decomposition H = a(f) + b(f) cos(theta)
         at 1.25 m, move the gradient part to 0.5 m with G(kr) = 1 + 1/(jkr), compare with the
         measured 0.5 m set. Baseline = "no proximity model" (shape of 1.25 m unchanged).
         One broadband real gain per set is fitted on 0.5-2 kHz on axis (levels are relative).

Outputs: research/reports/phase1_spatial_validation.json, research/plots/*.png
Usage  : .venv/Scripts/python.exe research/scripts/validate_spatial_models.py
"""
from __future__ import annotations

import json
import sys
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402
import numpy as np  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
FR = ROOT / "data" / "processed" / "frequency_responses"
REPORTS = ROOT / "research" / "reports"
PLOTS = ROOT / "research" / "plots"
C_SOUND = 343.0
THETA = np.deg2rad(np.arange(0, 360, 5))

DUAL = ["NeumannU87Ai", "AKGC414", "RodeK2", "RodeNT2A"]
MICS = {  # model -> patterns
    "NeumannU87Ai": ["Omni", "Cardioid", "Bidirect"], "AKGC414": ["Omni", "Cardioid", "Hyper", "Bidirect"],
    "RodeK2": ["Omni", "Cardioid", "Bidirect"], "RodeNT2A": ["Omni", "Cardioid", "Bidirect"],
    "SonyC800": ["Omni", "Cardioid"], "NeumannU47FET": ["fixed"], "RoyerR121": ["fixed"],
    "ShureSM58": ["fixed"], "ElectroVoiceRE20": ["fixed"],
}


def load(model: str, pattern: str, dist: float) -> dict:
    d = np.load(FR / f"{model}_{pattern}_{dist:g}m.npz")
    return {k: d[k] for k in d.files}


def band_centres(lo: float, hi: float, frac: int = 6) -> np.ndarray:
    n = np.arange(np.floor(frac * np.log2(lo / 1000)), np.ceil(frac * np.log2(hi / 1000)) + 1)
    return 1000 * 2 ** (n / frac)


def band_rms_db(f, err_db, mask, centres, frac=6):
    """RMS of err_db (angles x F) inside each fractional-octave band, over masked bins."""
    out = []
    for fc in centres:
        sel = (f >= fc * 2 ** (-0.5 / frac)) & (f < fc * 2 ** (0.5 / frac))
        e = err_db[:, sel][mask[:, sel]]
        out.append(float(np.sqrt(np.mean(e ** 2))) if e.size else np.nan)
    return np.array(out)


def significant(H, floor_db=-15.0):
    """Angles/bins whose level is within floor_db of the on-axis level (avoid nulls)."""
    lvl = 20 * np.log10(np.abs(H) + 1e-12)
    return lvl > (lvl[0][None, :] + floor_db)


# ---------------- Test A ----------------
def test_a() -> list[dict]:
    res = []
    for m in DUAL:
        for dist in (0.5, 1.25, 5.0):
            O, C, B = (load(m, p, dist)["H"] for p in ("Omni", "Cardioid", "Bidirect"))
            f = load(m, "Cardioid", dist)["f"]
            lf = float(load(m, "Cardioid", dist)["lf_limit"])
            sel = (f >= max(lf, 100)) & (f <= 16000)
            w = 1 / (np.abs(C[0, sel]) + 1e-12)  # per-frequency normalisation to on-axis level
            # Each Surrey IR is centred on its own window, so relative O/B/C timing is lost:
            # search a relative delay (samples) per component in addition to the complex gains.
            best = None
            for tO in np.arange(-3, 3.01, 0.25):
                for tB in np.arange(-3, 3.01, 0.25):
                    Os = O * np.exp(-2j * np.pi * f * tO / 48000)
                    Bs = B * np.exp(-2j * np.pi * f * tB / 48000)
                    A = np.stack([(Os[:, sel] * w).ravel(), (Bs[:, sel] * w).ravel()], axis=1)
                    y = (C[:, sel] * w).ravel()
                    g, *_ = np.linalg.lstsq(A, y, rcond=None)
                    r = float(np.sum(np.abs(A @ g - y) ** 2))
                    if best is None or r < best[0]:
                        best = (r, tO, tB, g, Os, Bs)
            _, tO, tB, g, Os, Bs = best
            Cp = g[0] * Os + g[1] * Bs
            err = 20 * np.log10(np.abs(Cp) + 1e-12) - 20 * np.log10(np.abs(C) + 1e-12)
            mask = significant(C)
            bands = {"100-500": (100, 500), "500-4k": (500, 4000), "4k-10k": (4000, 10000), "10k-16k": (10000, 16000)}
            row = {"model": m, "distance_m": dist}
            for name, (lo, hi) in bands.items():
                s = (f >= max(lo, lf)) & (f < hi)
                e = err[:, s][mask[:, s]]
                row[f"rms_db_{name}"] = round(float(np.sqrt(np.mean(e ** 2))), 2)
                row[f"onaxis_rms_db_{name}"] = round(float(np.sqrt(np.mean(err[0, s] ** 2))), 2)
            row["gO_over_gB_db"] = round(float(20 * np.log10(abs(g[0] / g[1]))), 2)
            row["delay_O_B_samples"] = [float(tO), float(tB)]
            res.append(row)
            if dist == 1.25:
                c = band_centres(100, 16000)
                plt.semilogx(c, band_rms_db(f, err, mask, c), label=m)
    plt.xlabel("Hz"); plt.ylabel("RMS error, dB (all significant angles)")
    plt.title("Test A @1.25 m: Cardioid built from Omni + Figure-8"); plt.grid(True, which="both", alpha=.3)
    plt.legend(); plt.savefig(PLOTS / "testA_dual_diaphragm_1.25m.png", dpi=120); plt.close()
    return res


# ---------------- Test B ----------------
def G(f, r):
    k = 2 * np.pi * np.maximum(f, 1.0) / C_SOUND
    return 1 + 1 / (1j * k * r)


def predict_050(src, f, dr):
    """First-order fit at 1.25 m, gradient part moved to 0.5 m. dr: acoustic-centre offset (m)."""
    X = np.stack([np.ones_like(THETA), np.cos(THETA)], axis=1)          # 72 x 2
    coef, *_ = np.linalg.lstsq(X, src, rcond=None)                       # 2 x F
    _, b = coef
    # Residual-preserving form: keep the measured 1.25 m response (incl. higher-order terms)
    # and only add the change of the first-order gradient part: b*cos(theta)*(G(0.5)/G(1.25) - 1).
    delta = b * (G(f, 0.5 + dr) / G(f, 1.25 + dr) - 1)
    return src + np.outer(np.cos(THETA), delta)


def level_match(pred, meas, f):
    s = (f >= 500) & (f <= 2000)
    g = np.mean(np.abs(meas[0, s])) / np.mean(np.abs(pred[0, s]))
    return pred * g


def test_b(dr_grid=np.arange(-0.10, 0.31, 0.02)):
    sets = [(m, p) for m, ps in MICS.items() for p in ps]
    data = {}
    for m, p in sets:
        a, b = load(m, p, 1.25), load(m, p, 0.5)
        data[(m, p)] = (a["f"], a["H_comp"], b["H_comp"])
    f = data[sets[0]][0]
    band = (f >= 70) & (f <= 500)

    def errs(dr):
        out = {}
        for key, (f_, H125, H050) in data.items():
            mask = significant(H050)[:, band]
            meas = 20 * np.log10(np.abs(H050[:, band]) + 1e-12)
            pm = level_match(predict_050(H125, f_, dr), H050, f_)
            pb = level_match(H125, H050, f_)
            em = (20 * np.log10(np.abs(pm[:, band]) + 1e-12) - meas)
            eb = (20 * np.log10(np.abs(pb[:, band]) + 1e-12) - meas)
            out[key] = (float(np.sqrt(np.mean(em[mask] ** 2))), float(np.sqrt(np.mean(eb[mask] ** 2))),
                        float(np.sqrt(np.mean(em[0] ** 2))), float(np.sqrt(np.mean(eb[0] ** 2))))
        return out

    scan = {round(float(dr), 2): float(np.mean([v[0] for v in errs(dr).values()])) for dr in dr_grid}
    best = min(scan, key=scan.get)
    rows = []
    for dr in (0.0, best):
        for (m, p), (em, eb, em0, eb0) in errs(dr).items():
            rows.append({"model": m, "pattern": p, "dr_m": dr, "model_rms_db_70_500": round(em, 2),
                         "baseline_rms_db_70_500": round(eb, 2), "model_onaxis_rms_db": round(em0, 2),
                         "baseline_onaxis_rms_db": round(eb0, 2)})

    # Plots: on-axis, measured 1.25 vs 0.5 vs predicted, for a few characteristic mics
    for m, p in [("RoyerR121", "fixed"), ("NeumannU87Ai", "Cardioid"), ("NeumannU87Ai", "Omni"),
                 ("ElectroVoiceRE20", "fixed"), ("ShureSM58", "fixed"), ("NeumannU87Ai", "Bidirect")]:
        f_, H125, H050 = data[(m, p)]
        pm = level_match(predict_050(H125, f_, best), H050, f_)
        pb = level_match(H125, H050, f_)
        s = (f_ >= 40) & (f_ <= 4000)
        for y, lab, st in [(H050, "measured 0.5 m", "-"), (pb, "1.25 m shape (no model)", ":"),
                           (pm, f"model -> 0.5 m (dr={best:+.2f} m)", "--")]:
            plt.semilogx(f_[s], 20 * np.log10(np.abs(y[0, s])), st, label=lab)
        plt.axvspan(40, 68, color="grey", alpha=.15, label="below LF validity")
        plt.title(f"{m} {p}: on-axis, proximity prediction"); plt.xlabel("Hz"); plt.ylabel("dB (relative)")
        plt.grid(True, which="both", alpha=.3); plt.legend(fontsize=8)
        plt.savefig(PLOTS / f"testB_{m}_{p}.png", dpi=120); plt.close()
    return {"dr_scan_mean_rms_db": scan, "best_dr_m": best, "rows": rows}


def main() -> int:
    REPORTS.mkdir(parents=True, exist_ok=True); PLOTS.mkdir(parents=True, exist_ok=True)
    a = test_a()
    b = test_b()
    (REPORTS / "phase1_spatial_validation.json").write_text(json.dumps({"testA": a, "testB": b}, indent=2))
    print("TEST A"); [print(r) for r in a]
    print("\nTEST B dr scan:", b["dr_scan_mean_rms_db"], "best", b["best_dr_m"])
    for r in b["rows"]:
        print(r)
    return 0


if __name__ == "__main__":
    sys.exit(main())
