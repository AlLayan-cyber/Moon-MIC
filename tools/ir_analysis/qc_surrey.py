"""Quality control of Surrey IR sets used by MOON MIC (in-scope mics + NTi M2211 reference).

For every set (model, pattern, distance) it checks, on the 32-bit float renders:
  * window extent (first/last non-zero sample) vs. the paper's window lengths
  * peak position spread across the 72 angles (time alignment)
  * normalisation scope: Normalised/Raw ratio constant across angles? 0 deg at -1 dBFS?
  * 24-bit vs 32-bit agreement (one angle)
It also writes cropped Normalised IR arrays to data/processed/aligned/<label>.npz
(read-only use of data/raw).

Usage: .venv/Scripts/python.exe tools/ir_analysis/qc_surrey.py
"""
from __future__ import annotations

import csv
import json
import sys
from collections import defaultdict
from pathlib import Path

import numpy as np
import soundfile as sf

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "dataset_import"))
from surrey_common import (IN_SCOPE, PROJECT_ROOT, REFERENCE_MIC, SAMPLE_RATE,  # noqa: E402
                           SURREY_ROOT, WINDOW_S, SetKey)

MANIFEST = PROJECT_ROOT / "data" / "manifests" / "surrey_madir_manifest.csv"
ALIGNED_DIR = PROJECT_ROOT / "data" / "processed" / "aligned"
QC_JSON = PROJECT_ROOT / "data" / "manifests" / "surrey_qc.json"
CROP = 2048  # samples kept per IR (> longest window, 33.2 ms = 1594 samples)
ANGLES = np.arange(0, 360, 5)


def load(rel: str) -> np.ndarray:
    x, sr = sf.read(str(SURREY_ROOT / rel), dtype="float64", always_2d=False)
    assert sr == SAMPLE_RATE
    return x


def db(x: float) -> float:
    return float(20 * np.log10(max(abs(x), 1e-30)))


def main() -> int:
    rows = list(csv.DictReader(MANIFEST.open(encoding="utf-8")))
    wanted = set(IN_SCOPE) | {REFERENCE_MIC}
    index = defaultdict(dict)  # (variant, SetKey, bits) -> {angle: relpath}
    for r in rows:
        if r["model"] not in wanted or "extra_dir" in r["flags"]:
            continue
        key = SetKey(r["model"], r["pattern"], float(r["distance_m"]))
        bits = r["file_bits"] or r["dir_bits"]
        index[(r["variant"], key, bits)][int(r["angle_deg"])] = r["relpath"]

    keys = sorted({k[1] for k in index}, key=lambda k: (k.model, k.pattern, k.distance_m))
    ALIGNED_DIR.mkdir(parents=True, exist_ok=True)
    results = []
    for key in keys:
        norm = index[("Normalised_IRs", key, "32bit")]
        raw = index[("Raw_IRs", key, "32bit")]
        n24 = index[("Normalised_IRs", key, "24bit")]
        irs = np.zeros((72, CROP))
        first_nz, last_nz, peak_idx, ratio_db = [], [], [], []
        for i, a in enumerate(ANGLES):
            x = load(norm[a])
            nz = np.flatnonzero(x)
            first_nz.append(int(nz[0]))
            last_nz.append(int(nz[-1]))
            peak_idx.append(int(np.argmax(np.abs(x))))
            irs[i] = x[:CROP]
            if a in raw:
                y = load(raw[a])
                ratio_db.append(db(np.max(np.abs(x))) - db(np.max(np.abs(y))))
        x24 = load(n24[0])[:CROP]
        diff24 = db(np.max(np.abs(x24 - irs[0]))) - db(np.max(np.abs(irs[0])))
        win_expected = WINDOW_S[key.distance_m] * SAMPLE_RATE
        res = {
            "label": key.label, "model": key.model, "pattern": key.pattern,
            "distance_m": key.distance_m, "moonmic_id": IN_SCOPE.get(key.model, "reference"),
            "window_samples_expected": round(win_expected, 1),
            "window_samples_observed": [min(last_nz) - max(first_nz) + 1, max(last_nz) - min(first_nz) + 1],
            "first_nonzero_range": [min(first_nz), max(first_nz)],
            "last_nonzero_range": [min(last_nz), max(last_nz)],
            "data_beyond_crop": bool(max(last_nz) >= CROP),
            "peak_index_range": [min(peak_idx), max(peak_idx)],
            "peak_0deg_dbfs": round(db(np.max(np.abs(irs[0]))), 3),
            "peak_max_dbfs": round(max(db(np.max(np.abs(r))) for r in irs), 3),
            "norm_minus_raw_db": [round(min(ratio_db), 3), round(max(ratio_db), 3)] if ratio_db else None,
            "raw_angles_available": len(raw),
            "err_24_vs_32_db": round(diff24, 1),
        }
        results.append(res)
        np.savez_compressed(ALIGNED_DIR / f"{key.label}.npz", ir=irs, angles_deg=ANGLES,
                            fs=SAMPLE_RATE, distance_m=key.distance_m, model=key.model,
                            pattern=key.pattern, first_nonzero=np.array(first_nz),
                            last_nonzero=np.array(last_nz), source="Surrey Normalised_IRs 32bit")
        print(f"{key.label:38s} win {res['window_samples_observed']} exp {res['window_samples_expected']:7.1f} "
              f"peak {res['peak_index_range']} 0deg {res['peak_0deg_dbfs']:6.2f} max {res['peak_max_dbfs']:6.2f} "
              f"N-R {res['norm_minus_raw_db']} 24v32 {res['err_24_vs_32_db']}")

    QC_JSON.write_text(json.dumps(results, indent=2), encoding="utf-8")
    print(f"\n{len(results)} sets written to {QC_JSON.relative_to(PROJECT_ROOT)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
