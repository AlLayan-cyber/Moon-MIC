"""Synthetic leave-one-singer-out evaluation of the source-distance estimator v1.

1. LTAS of every VocalSet take (cached in data/processed/vocalset_ltas.npz; data/processed is not in git).
2. For each held-out singer: prior from the other 19 singers; up to TAKES_PER_SINGER takes of the held-out
   singer are rendered (on spectra) through every source-mic model of the plugin at R_TRUE, angle 0, and the
   distance is estimated.
3. Report: median |log2(r_hat / r_true)| (octaves), share within +-30 %, 68 % interval coverage, per pattern.

Limits (stated in the report): synthetic only. VocalSet was recorded with an AT2020 cardioid "close" (distance
not stated, ISMIR 2018 paper §3.2), so the prior is "voice as captured by that setup"; on real recordings the
estimate inherits that unknown offset. Room reflections, singer movement and other mics are not simulated.

Run: .venv/Scripts/python.exe -m research.position_estimator.experiment <VocalSet root folder>
"""
from __future__ import annotations

import json
import sys
from pathlib import Path

import numpy as np

from .estimator import R_GRID, Prior, estimate
from .ltas import analyse

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "export"))
import export_signature_models as ex  # noqa: E402

CACHE = ROOT / "data" / "processed" / "vocalset_ltas.npz"
OUT = ROOT / "research" / "reports" / "position_estimator_v1.json"
R_TRUE = (0.05, 0.10, 0.15, 0.20, 0.30, 0.50, 0.80)
TAKES_PER_SINGER = 20


def singer_of(path: Path) -> str | None:
    for part in path.parts:
        p = part.lower()
        for prefix in ("female", "male"):
            if p.startswith(prefix) and p[len(prefix):].isdigit():
                return p
    return None


def build_cache(root: Path, fg: np.ndarray) -> dict:
    if CACHE.exists():
        d = np.load(CACHE, allow_pickle=False)
        return {k: d[k] for k in d.files}
    files = sorted(root.rglob("*.wav"))
    rows, singers, f0, paths = [], [], [], []
    for i, p in enumerate(files):
        s = singer_of(p.relative_to(root))
        if s is None:
            continue
        r = analyse(str(p), fg)
        if r is None:
            continue
        rows.append(r["ltas_db"]); singers.append(s); f0.append(r["f0_median"]); paths.append(str(p.relative_to(root)))
        if i % 200 == 0:
            print(f"  {i}/{len(files)}", flush=True)
    CACHE.parent.mkdir(parents=True, exist_ok=True)
    out = {"ltas": np.array(rows), "singer": np.array(singers), "f0": np.array(f0), "path": np.array(paths)}
    np.savez_compressed(CACHE, **out)
    return out


def main() -> int:
    root = Path(sys.argv[1])
    fg = ex.log_grid()
    data = build_cache(root, fg)
    print("takes:", len(data["singer"]), "singers:", len(set(data["singer"])))
    curves = ex.load_mfr_curves()
    models = {}
    for model, pattern, sid, _ in ex.SETS:
        _, _, mag, ph, b, _ = ex.build_set(model, pattern, sid, curves, fg)
        m32 = (mag.astype(np.float32).astype(float), ph.astype(np.float32).astype(float),
               b.real.astype(np.float32) + 1j * b.imag.astype(np.float32))
        models[sid] = (pattern, np.array([ex.model_response(*m32, fg, 0.0, r) for r in R_GRID]))
    rng = np.random.default_rng(1)
    results = []
    singers = sorted(set(data["singer"]))
    for s in singers:
        train = [{"ltas_db": l, "f0_median": f} for l, f, g in zip(data["ltas"], data["f0"], data["singer"]) if g != s]
        prior = Prior(train, fg)
        idx = np.flatnonzero(data["singer"] == s)
        idx = rng.choice(idx, size=min(TAKES_PER_SINGER, idx.size), replace=False)
        for sid, (pattern, mdb) in models.items():
            for r_true in R_TRUE:
                k_true = int(np.argmin(np.abs(np.log(R_GRID / r_true))))
                for i in idx:
                    L = data["ltas"][i] + mdb[k_true]
                    e = estimate(L, float(data["f0"][i]), prior, mdb)
                    f0 = float(data["f0"][i])
                    reg = "low <180 Hz" if f0 < 180 else ("mid 180-330 Hz" if f0 < 330 else "high >330 Hz")
                    results.append((s, sid, pattern, r_true, float(R_GRID[k_true]), e["r_hat"], e["lo68"], e["hi68"], e["width_oct"], reg))
        print(" done", s, flush=True)
    arr = np.array([(r[4], r[5], r[6], r[7], r[8]) for r in results], float)
    pat = np.array([r[2] for r in results]); sids = np.array([r[1] for r in results])
    def summary(mask):
        rt, rh, lo, hi, w = arr[mask].T
        ok = np.isfinite(rh)
        err = np.abs(np.log2(rh[ok] / rt[ok]))
        return {"n": int(mask.sum()), "median_abs_err_oct": round(float(np.median(err)), 3),
                "p90_abs_err_oct": round(float(np.percentile(err, 90)), 3),
                "within_30pct": round(float(np.mean(err <= np.log2(1.3))), 3),
                "coverage_68": round(float(np.mean((rt[ok] >= lo[ok]) & (rt[ok] <= hi[ok]))), 3),
                "median_interval_oct": round(float(np.median(w[ok])), 2)}
    reg = np.array([r[9] for r in results])
    out = {"by_pattern": {p: summary(pat == p) for p in sorted(set(pat))},
           "by_register": {g: summary(reg == g) for g in ("low <180 Hz", "mid 180-330 Hz", "high >330 Hz")},
           "by_set": {s: summary(sids == s) for s in sorted(set(sids))},
           "by_distance": {f"{r:g} m": summary(np.isclose(arr[:, 0], R_GRID[int(np.argmin(np.abs(np.log(R_GRID / r))))])) for r in R_TRUE}}
    OUT.write_text(json.dumps(out, indent=1), encoding="utf-8")
    print(json.dumps(out["by_pattern"], indent=1))
    print(json.dumps(out["by_distance"], indent=1))
    print(json.dumps(out["by_register"], indent=1))
    return 0


if __name__ == "__main__":
    sys.exit(main())
