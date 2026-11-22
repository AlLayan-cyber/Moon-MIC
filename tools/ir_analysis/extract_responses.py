"""Complex frequency responses per Surrey set, with optional chain compensation.

Input : data/processed/aligned/<label>.npz   (from qc_surrey.py)
Output: data/processed/frequency_responses/<label>.npz with
        f        : frequency grid (Hz)
        H        : complex response, shape (72 angles, F); window-centre delay removed
        H_comp   : H / H_ref(0 deg, same distance)  (NTi M2211 Class-1 reference; chain compensation)
        lf_limit : LF validity limit for this distance (paper p.7)
Levels are RELATIVE: Surrey re-set preamp gain per measurement, so absolute sensitivity is unknown.
H_comp is scaled to 0 dB at 1 kHz on axis.

Usage: .venv/Scripts/python.exe tools/ir_analysis/extract_responses.py
"""
from __future__ import annotations

import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "dataset_import"))
from surrey_common import LF_LIMIT_HZ, PROJECT_ROOT, REFERENCE_MIC, SAMPLE_RATE, WINDOW_S  # noqa: E402

ALIGNED = PROJECT_ROOT / "data" / "processed" / "aligned"
OUT = PROJECT_ROOT / "data" / "processed" / "frequency_responses"
NFFT = 8192


def spectrum(ir: np.ndarray, distance_m: float) -> tuple[np.ndarray, np.ndarray]:
    f = np.fft.rfftfreq(NFFT, 1 / SAMPLE_RATE)
    H = np.fft.rfft(ir, NFFT, axis=-1)
    centre = WINDOW_S[distance_m] * SAMPLE_RATE / 2.0  # direct sound is centred in the window
    H *= np.exp(2j * np.pi * f * centre / SAMPLE_RATE)
    return f, H


def main() -> int:
    OUT.mkdir(parents=True, exist_ok=True)
    refs = {}
    for p in ALIGNED.glob(f"{REFERENCE_MIC}_*.npz"):
        d = np.load(p)
        refs[float(d["distance_m"])] = spectrum(d["ir"][0], float(d["distance_m"]))[1]
    for p in sorted(ALIGNED.glob("*.npz")):
        d = np.load(p)
        dist = float(d["distance_m"])
        f, H = spectrum(d["ir"], dist)
        Hc = H / refs[dist][None, :]
        k1 = np.argmin(abs(f - 1000))
        Hc /= np.abs(Hc[0, k1])
        np.savez_compressed(OUT / p.name, f=f, H=H, H_comp=Hc, angles_deg=d["angles_deg"],
                            distance_m=dist, model=d["model"], pattern=d["pattern"],
                            lf_limit=LF_LIMIT_HZ[dist], reference=REFERENCE_MIC)
        print(p.stem)
    return 0


if __name__ == "__main__":
    sys.exit(main())
