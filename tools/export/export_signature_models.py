"""Export SIGNATURE model data for the plugin (format MMIC v1) + reference test vectors.

Source : data/processed/frequency_responses/<set>_0.5m.npz (chain-compensated, 32-bit Surrey data)
Output : plugin/resources/models/moonmic_signature_v1.bin
         plugin/resources/models/moonmic_signature_v1.json   (provenance, set list)
         plugin/tests/reference/signature_reference_v1.json  (expected magnitudes for C++ tests)

Model per set (reference distance r0 = 0.5 m, the closest Surrey distance):
  H(f, theta)  complex response, 72 angles, on a log-frequency grid (1/48 octave, 20 Hz - 24 kHz)
  b(f)         first-order gradient component (least squares over 72 angles: H = a + b cos(theta))
Runtime (docs/architecture/03 §3, residual-preserving form, VERIFIED 1.25 -> 0.5 m in Phase 1):
  H(f, theta, r) = H(f, theta) + b(f) cos(theta) [G(f, r)/G(f, r0) - 1],  G = 1 + 1/(j k r)

Data handling (documented limits):
  * below the LF validity limit (60.2 Hz at 0.5 m) magnitude/phase are HELD at the limit value
    -> real LF roll-off of each mic below ~60 Hz is UNKNOWN in this model.
  * above 18 kHz values are held at 18 kHz (reference mic falls above 18 kHz; division unreliable).
  * levels are relative: 0 dB at 1 kHz on axis (absolute sensitivity UNKNOWN).

Binary layout (little endian):
  char[4] 'MMIC' | u32 version=1 | u32 nSets | u32 nFreq | u32 nAngles | f32 r0 | f32 c_sound
  f32 freq[nFreq] | f32 angles_deg[nAngles]
  per set: u32 len + utf8 id | u32 len + utf8 display | u32 len + utf8 pattern | f32 lfLimitHz
           f32 magDb[nAngles][nFreq] | f32 phaseRad[nAngles][nFreq] (unwrapped along f)
           f32 bRe[nFreq] | f32 bIm[nFreq]

Usage: .venv/Scripts/python.exe tools/export/export_signature_models.py
"""
from __future__ import annotations

import hashlib
import json
import struct
import sys
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parents[2]
FR = ROOT / "data" / "processed" / "frequency_responses"
OUT_MODELS = ROOT / "plugin" / "resources" / "models"
OUT_TESTS = ROOT / "plugin" / "tests" / "reference"

R0 = 0.5
C_SOUND = 343.0
F_LO, F_HI, OCT_FRAC = 20.0, 24000.0, 48
# Above 18 kHz the reference mic itself falls (-7..-9 dB at 20 kHz) and the compensated data is
# noisy (U87 Ai Cardioid 0.5 m: -10.4 dB at 18 kHz but +3.6 dB at 20 kHz). Hold from 18 kHz.
F_HOLD_HI = 18000.0
THETA = np.deg2rad(np.arange(0, 360, 5))

# (surrey model, surrey pattern) -> (id, display name). Display names: dev build only,
# trademark use pending legal review (docs/architecture/11 §4).
SETS = [
    ("NeumannU87Ai", "Cardioid", "u87ai_cardioid", "U87 Ai — Cardioid"),
    ("NeumannU87Ai", "Omni", "u87ai_omni", "U87 Ai — Omni"),
    ("NeumannU87Ai", "Bidirect", "u87ai_figure8", "U87 Ai — Figure-8"),
    ("NeumannU47FET", "fixed", "u47fet", "U47 FET"),
    ("AKGC414", "Cardioid", "c414bxls_cardioid", "C414 B-XLS — Cardioid"),
    ("AKGC414", "Hyper", "c414bxls_hyper", "C414 B-XLS — Hypercardioid"),
    ("AKGC414", "Omni", "c414bxls_omni", "C414 B-XLS — Omni"),
    ("AKGC414", "Bidirect", "c414bxls_figure8", "C414 B-XLS — Figure-8"),
    ("SonyC800", "Cardioid", "c800_cardioid", "C-800 — Cardioid"),
    ("SonyC800", "Omni", "c800_omni", "C-800 — Omni"),
    ("RodeK2", "Cardioid", "k2_cardioid", "K2 — Cardioid"),
    ("RodeK2", "Omni", "k2_omni", "K2 — Omni"),
    ("RodeK2", "Bidirect", "k2_figure8", "K2 — Figure-8"),
    ("RoyerR121", "fixed", "r121", "R-121"),
    ("ShureSM58", "fixed", "sm58", "SM58"),
    ("ElectroVoiceRE20", "fixed", "re20", "RE20"),
    ("RodeNT2A", "Cardioid", "nt2a_cardioid", "NT2-A — Cardioid"),
    ("RodeNT2A", "Omni", "nt2a_omni", "NT2-A — Omni"),
    ("RodeNT2A", "Bidirect", "nt2a_figure8", "NT2-A — Figure-8"),
]


def log_grid() -> np.ndarray:
    n = int(np.floor(OCT_FRAC * np.log2(F_HI / F_LO))) + 1
    return (F_LO * 2 ** (np.arange(n) / OCT_FRAC)).astype(np.float64)


def G(f, r):
    return 1 + 1 / (1j * (2 * np.pi * f / C_SOUND) * r)


def to_grid(f_src, H, fg, lf_limit):
    """Complex spectrum (angles x F) -> magnitude dB / unwrapped phase on fg, with holds."""
    mag = 20 * np.log10(np.abs(H) + 1e-12)
    ph = np.unwrap(np.angle(H), axis=-1)
    fq = np.clip(fg, lf_limit, F_HOLD_HI)
    m = np.stack([np.interp(fq, f_src, row) for row in mag])
    p = np.stack([np.interp(fq, f_src, row) for row in ph])
    return m, p


def model_response(mag_db, ph, b, fg, angle_deg, r):
    """Reference implementation of the runtime synthesis (target magnitude, before min-phase)."""
    H = 10 ** (mag_db / 20) * np.exp(1j * ph)
    a = (angle_deg % 360) / 5.0
    i0 = int(np.floor(a)) % 72
    i1 = (i0 + 1) % 72
    t = a - np.floor(a)
    Ht = (1 - t) * H[i0] + t * H[i1]
    fq = np.maximum(fg, 1.0)
    Ht = Ht + b * np.cos(np.deg2rad(angle_deg)) * (G(fq, r) / G(fq, R0) - 1)
    return 20 * np.log10(np.abs(Ht) + 1e-12)


def main() -> int:
    OUT_MODELS.mkdir(parents=True, exist_ok=True)
    OUT_TESTS.mkdir(parents=True, exist_ok=True)
    fg = log_grid()
    X = np.stack([np.ones_like(THETA), np.cos(THETA)], axis=1)
    blob = bytearray()
    blob += b"MMIC" + struct.pack("<IIIIff", 1, len(SETS), len(fg), 72, R0, C_SOUND)
    blob += fg.astype("<f4").tobytes() + np.arange(0, 360, 5).astype("<f4").tobytes()
    meta, refs = [], []
    for model, pattern, sid, disp in SETS:
        src = FR / f"{model}_{pattern}_0.5m.npz"
        d = np.load(src)
        f, H, lf = d["f"], d["H_comp"], float(d["lf_limit"])
        coef, *_ = np.linalg.lstsq(X, H, rcond=None)
        b = coef[1]
        mag, ph = to_grid(f, H, fg, lf)
        fq = np.clip(fg, lf, F_HOLD_HI)
        b_g = np.interp(fq, f, b.real) + 1j * np.interp(fq, f, b.imag)
        for s in (sid, disp, pattern):
            e = s.encode("utf-8")
            blob += struct.pack("<I", len(e)) + e
        blob += struct.pack("<f", lf)
        blob += mag.astype("<f4").tobytes() + ph.astype("<f4").tobytes()
        blob += b_g.real.astype("<f4").tobytes() + b_g.imag.astype("<f4").tobytes()
        meta.append({"id": sid, "display": disp, "surrey_model": model, "pattern": pattern,
                     "r0_m": R0, "lf_limit_hz": lf, "source_file": src.relative_to(ROOT).as_posix(),
                     "source_sha256": hashlib.sha256(src.read_bytes()).hexdigest()})
        # reference vectors (float32-rounded inputs, same as the plugin will see)
        m32, p32 = mag.astype(np.float32).astype(float), ph.astype(np.float32).astype(float)
        b32 = b_g.real.astype(np.float32) + 1j * b_g.imag.astype(np.float32)
        for ang, r in [(0, 0.5), (0, 0.1), (0, 1.25), (37.5, 0.25), (90, 0.5), (180, 0.3)]:
            refs.append({"id": sid, "angle_deg": ang, "distance_m": r,
                         "mag_db": [round(float(v), 4) for v in model_response(m32, p32, b32, fg, ang, r)]})
    out_bin = OUT_MODELS / "moonmic_signature_v1.bin"
    out_bin.write_bytes(bytes(blob))
    info = {"format": "MMIC", "version": 1, "r0_m": R0, "c_sound": C_SOUND, "n_freq": len(fg),
            "grid": f"log, 1/{OCT_FRAC} octave, {F_LO:g}-{F_HI:g} Hz", "hold_hi_hz": F_HOLD_HI,
            "level": "0 dB at 1 kHz on axis (relative; absolute sensitivity UNKNOWN)",
            "source": "Surrey MADIR (CC BY), Zenodo 10.5281/zenodo.4633508, Normalised 32-bit, "
                      "chain-compensated by NTi M2211 at same distance",
            "bin_sha256": hashlib.sha256(bytes(blob)).hexdigest(), "sets": meta}
    (OUT_MODELS / "moonmic_signature_v1.json").write_text(json.dumps(info, indent=2, ensure_ascii=False),
                                                          encoding="utf-8")
    (OUT_TESTS / "signature_reference_v1.json").write_text(
        json.dumps({"freq": [float(v) for v in fg], "r0_m": R0, "cases": refs}), encoding="utf-8")
    print(f"{len(SETS)} sets, {len(fg)} freqs, {len(blob)/1e6:.2f} MB -> {out_bin.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
