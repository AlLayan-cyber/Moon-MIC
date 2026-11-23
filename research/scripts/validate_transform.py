"""TRANSFORM v1 validation on independent Surrey measurements (docs/architecture/05 §7, 10).

Reference pairs without own recordings: Surrey measured every microphone with the same source at the same
positions. For a source mic measured at r_s (1.25 m or 5 m, NOT used to build any model) and a target mic
measured at 0.5 m, TRANSFORM applied to the measured source must give the measured target:

    error(f) = [H_src,meas(r_s) + C(src @ r_s -> tgt @ 0.5 m)] - H_tgt,meas(0.5 m)      (dB, on axis)

Baseline ("do nothing"): H_src,meas(r_s) - H_tgt,meas(0.5 m). Both curves are 1/6-octave smoothed.
Band: from the higher of the two Surrey LF limits up to 16 kHz. All 18 x 18 pairs (incl. same mic).
Output: research/reports/transform_validation_v1.json + console summary.
Run: .venv/Scripts/python.exe research/scripts/validate_transform.py
"""
from __future__ import annotations

import json
import sys
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "export"))
import export_signature_models as ex  # noqa: E402

FR = ROOT / "data" / "processed" / "frequency_responses"
OUT = ROOT / "research" / "reports" / "transform_validation_v1.json"


def smooth_db(f: np.ndarray, db: np.ndarray, frac: float = 6.0) -> np.ndarray:
    p = 10 ** (db / 10)
    out = np.empty_like(db)
    for i, fc in enumerate(f):
        m = (f >= fc * 2 ** (-0.5 / frac)) & (f <= fc * 2 ** (0.5 / frac))
        out[i] = 10 * np.log10(np.mean(p[m]))
    return out


def measured_on_grid(model: str, pattern: str, r: float, fg: np.ndarray):
    d = np.load(FR / f"{model}_{pattern}_{r:g}m.npz")
    f, H, lf = d["f"], d["H_comp"], float(d["lf_limit"])
    db = 20 * np.log10(np.abs(H[0]) + 1e-12)
    keep = f > 10
    return np.interp(np.log(fg), np.log(f[keep]), smooth_db(f[keep], db[keep])), lf


def main() -> int:
    fg = ex.log_grid()
    curves = ex.load_mfr_curves()
    models = {}
    for model, pattern, sid, _ in ex.SETS:
        _, lf, mag, ph, b, _ = ex.build_set(model, pattern, sid, curves, fg)
        models[sid] = (model, pattern, mag.astype(np.float32).astype(float), ph.astype(np.float32).astype(float),
                       (b.real.astype(np.float32) + 1j * b.imag.astype(np.float32)))
    rows = []
    for r_s in (1.25, 5.0):
        for sid_s, (ms, ps, m_s, p_s, b_s) in models.items():
            src_meas, lf_s = measured_on_grid(ms, ps, r_s, fg)
            S = ex.model_response(m_s, p_s, b_s, fg, 0.0, r_s)
            for sid_t, (mt, pt, m_t, p_t, b_t) in models.items():
                tgt_meas, lf_t = measured_on_grid(mt, pt, 0.5, fg)
                T = ex.model_response(m_t, p_t, b_t, fg, 0.0, 0.5)
                C = ex.transform_correction(S, T, fg)
                band = (fg >= max(lf_s, lf_t)) & (fg <= 16000.0)
                def stats(e):
                    e = e[band] - np.mean(e[(fg >= 500) & (fg <= 2000)])     # level-aligned at mid band
                    return float(np.sqrt(np.mean(e ** 2))), float(np.max(np.abs(e)))
                rms0, max0 = stats(src_meas - tgt_meas)
                rms1, max1 = stats(src_meas + C - tgt_meas)
                rows.append({"source": sid_s, "source_distance_m": r_s, "target": sid_t, "band_lo_hz": round(max(lf_s, lf_t), 1),
                             "baseline_rms_db": round(rms0, 2), "baseline_max_db": round(max0, 2),
                             "transform_rms_db": round(rms1, 2), "transform_max_db": round(max1, 2)})
    summary = {}
    for r_s in (1.25, 5.0):
        sel = [x for x in rows if x["source_distance_m"] == r_s]
        b = np.array([x["baseline_rms_db"] for x in sel]); t = np.array([x["transform_rms_db"] for x in sel])
        tm = np.array([x["transform_max_db"] for x in sel])
        same = [x for x in sel if x["source"] == x["target"]]
        summary[f"{r_s:g} m"] = {
            "pairs": len(sel),
            "baseline_rms_db_median": round(float(np.median(b)), 2),
            "transform_rms_db_median": round(float(np.median(t)), 2),
            "transform_rms_db_p90": round(float(np.percentile(t, 90)), 2),
            "transform_max_db_median": round(float(np.median(tm)), 2),
            "improved_pairs": int(np.sum(t < b)),
            "same_mic_distance_only_rms_db_median": round(float(np.median([x["transform_rms_db"] for x in same])), 2),
        }
    worst = sorted(rows, key=lambda x: -x["transform_rms_db"])[:8]
    OUT.write_text(json.dumps({"method": __doc__.strip().splitlines()[0], "summary": summary, "worst": worst,
                               "pairs": rows}, indent=1), encoding="utf-8")
    print(json.dumps(summary, indent=1))
    print("worst transform RMS:")
    for w in worst:
        print(" ", w)
    return 0


if __name__ == "__main__":
    sys.exit(main())
