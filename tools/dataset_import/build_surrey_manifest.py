"""Build a read-only manifest of the Surrey IR archive.

Output: data/manifests/surrey_madir_manifest.csv (one row per WAV)
        data/manifests/surrey_madir_summary.json (archive hash, counts, anomalies)

Usage:  .venv/Scripts/python.exe tools/dataset_import/build_surrey_manifest.py
"""
from __future__ import annotations

import csv
import hashlib
import json
import sys
from collections import Counter, defaultdict
from pathlib import Path

import soundfile as sf

sys.path.insert(0, str(Path(__file__).resolve().parent))
from surrey_common import (IN_SCOPE, PROJECT_ROOT, SURREY_ROOT, SURREY_ZIP,  # noqa: E402
                           parse_file, parse_folder)

OUT_DIR = PROJECT_ROOT / "data" / "manifests"


def sha256(path: Path, chunk: int = 1 << 24) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        while block := f.read(chunk):
            h.update(block)
    return h.hexdigest()


def main() -> int:
    if not SURREY_ROOT.is_dir():
        print(f"Surrey root not found: {SURREY_ROOT}", file=sys.stderr)
        return 1
    OUT_DIR.mkdir(parents=True, exist_ok=True)

    rows, anomalies = [], []
    for wav in sorted(SURREY_ROOT.rglob("*.wav")):
        rel = wav.relative_to(SURREY_ROOT)
        parts = rel.parts  # variant / set_folder / bitdir / file
        if len(parts) != 4:
            anomalies.append({"type": "unexpected_depth", "path": rel.as_posix()})
            continue
        variant, set_folder, bitdir, fname = parts
        pf, pfile = parse_folder(set_folder), parse_file(fname)
        if pf is None or pfile is None:
            anomalies.append({"type": "unparsed_name", "path": rel.as_posix()})
            continue
        model, pattern, dist = pf
        info = sf.info(str(wav))
        flags = []
        if bitdir in ("24bps", "32bps"):  # AKG D12 0.5 m uses 'bps' folder names
            flags.append("dir_name_variant")
            bitdir = bitdir.replace("bps", "bit")
        if bitdir not in ("24bit", "32bit"):
            flags.append("extra_dir")
        elif pfile["file_bits"] and pfile["file_bits"] != bitdir:
            flags.append("misfiled_bitdepth")
        rows.append({
            "variant": variant, "model": model, "pattern": pattern, "distance_m": dist,
            "angle_deg": pfile["angle_deg"], "dir_bits": bitdir, "file_bits": pfile["file_bits"],
            "moonmic_id": IN_SCOPE.get(model, ""), "samplerate": info.samplerate,
            "channels": info.channels, "frames": info.frames, "subtype": info.subtype,
            "size_bytes": wav.stat().st_size, "flags": ";".join(flags), "relpath": rel.as_posix(),
        })

    with (OUT_DIR / "surrey_madir_manifest.csv").open("w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
        w.writeheader()
        w.writerows(rows)

    # Completeness per (variant, set, effective bit depth): expect 72 angles each.
    # Every (variant, set) is expected in both 24-bit and 32-bit renders.
    sets = defaultdict(set)
    for r in rows:
        if "extra_dir" in r["flags"]:
            continue
        bits = r["file_bits"] or r["dir_bits"]
        sets[(r["variant"], r["model"], r["pattern"], r["distance_m"], bits)].add(r["angle_deg"])
    base_sets = {(k[0], k[1], k[2], k[3]) for k in sets}
    incomplete = []
    for b in sorted(base_sets):
        for bits in ("24bit", "32bit"):
            n = len(sets.get(b + (bits,), ()))
            if n != 72:
                incomplete.append({"variant": b[0], "model": b[1], "pattern": b[2],
                                   "distance_m": b[3], "bits": bits, "angles": n})

    summary = {
        "source": "Surrey MADIR, Zenodo 10.5281/zenodo.4633508 (CC BY)",
        "archive": {"path": SURREY_ZIP.relative_to(PROJECT_ROOT).as_posix(),
                    "size_bytes": SURREY_ZIP.stat().st_size, "sha256": sha256(SURREY_ZIP)},
        "wav_count": len(rows),
        "by_variant": Counter(r["variant"] for r in rows),
        "by_flag": Counter(r["flags"] or "ok" for r in rows),
        "samplerates": Counter(r["samplerate"] for r in rows),
        "subtypes": Counter(r["subtype"] for r in rows),
        "frames": Counter(r["frames"] for r in rows),
        "in_scope_models": sorted({r["model"] for r in rows if r["moonmic_id"]}),
        "incomplete_sets": incomplete,
        "anomalies": anomalies,
    }
    (OUT_DIR / "surrey_madir_summary.json").write_text(json.dumps(summary, indent=2), encoding="utf-8")
    print(json.dumps({k: v for k, v in summary.items() if k not in ("anomalies",)}, indent=2))
    print(f"anomalies: {len(anomalies)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
