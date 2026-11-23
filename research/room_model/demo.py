"""Listening demo: one dry VocalSet excerpt (CC BY 4.0) through microphone model + room at several distances.

Each file is level-matched (pink-weighted, like Auto gain) so the difference is the room/distance, not loudness.
Output: research/demos/room_<mic>_<room>_<distance>cm.wav (WAV files are not in git).
Run: .venv/Scripts/python.exe -m research.room_model.demo <path to a VocalSet wav>
"""
from __future__ import annotations

import sys
from pathlib import Path

import numpy as np
import soundfile as sf
from scipy.signal import fftconvolve, resample_poly

from .render import MicModel, render
from .room import Room

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "research" / "demos"


def main() -> int:
    x, fs = sf.read(sys.argv[1], dtype="float64", always_2d=True)
    x = x.mean(axis=1)
    if fs != 48000:
        x = resample_poly(x, 48000, fs); fs = 48000
    x = x[: int(12 * fs)]
    OUT.mkdir(parents=True, exist_ok=True)
    rooms = {"live_room": Room(6.0, 8.0, 3.5, {125: 0.8, 250: 0.7, 500: 0.65, 1000: 0.6, 2000: 0.55, 4000: 0.5, 8000: 0.45}),
             "vocal_booth": Room(2.0, 2.5, 2.4, {125: 0.35, 250: 0.3, 500: 0.25, 1000: 0.25, 2000: 0.22, 4000: 0.2, 8000: 0.18})}
    for sid in ("u87ai_cardioid",):
        mic = MicModel.load(sid)
        for rname, room in rooms.items():
            dists = (0.15, 0.5, 1.5, 3.0) if rname == "live_room" else (0.15, 0.5, 1.2)
            for d in dists:
                h = render(room, mic, (room.lx / 2, room.ly * 0.3), 0.0, d, 0.0, float(fs))
                y = fftconvolve(x, h)[: x.size + int(0.8 * fs)]
                y /= np.sqrt(np.mean(y ** 2)) / 0.05                 # level-matched (RMS -26 dBFS)
                y = np.clip(y, -1.0, 1.0)
                path = OUT / f"room_{sid}_{rname}_{int(round(d * 100))}cm.wav"
                sf.write(path, y.astype(np.float32), fs, subtype="FLOAT")
                print(path.relative_to(ROOT))
    return 0


if __name__ == "__main__":
    sys.exit(main())
