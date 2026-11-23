"""Checks of the room & distance reference model against closed-form acoustics (no measured room data exists:
Surrey published only the windowed, quasi-anechoic responses).

Run: .venv/Scripts/python.exe -m research.room_model.tests
"""
from __future__ import annotations

import sys

import numpy as np

from .render import MicModel, band_weights, render
from .room import BANDS, C, Room, image_paths

RESULTS = []


def check(name, ok, info):
    RESULTS.append(bool(ok))
    print(f"[{'PASS' if ok else 'FAIL'}] {name}: {info}")


def band(h: np.ndarray, fs: float, band_hz: float) -> np.ndarray:
    """Zero-phase octave-band filter. The response is zero-padded on both sides first, otherwise the filter's
    pre-ringing around t = 0 wraps to the end of the buffer and fakes a non-decaying tail."""
    pad = int(0.1 * fs)
    x = np.concatenate([np.zeros(pad), h, np.zeros(pad)])
    f = np.fft.rfftfreq(x.size, 1 / fs)
    w = band_weights(np.maximum(f, 1.0))[int(np.argmin(np.abs(BANDS - band_hz)))]
    return np.fft.irfft(np.fft.rfft(x) * w, x.size)[pad: pad + h.size]


def schroeder_t60(h: np.ndarray, fs: float, band_hz: float, t_from: float) -> float:
    """Schroeder decay fitted on the late part only: from t_from down to 30 dB below the level there."""
    hb = band(h, fs, band_hz)
    edc = np.cumsum((hb ** 2)[::-1])[::-1]
    t = np.arange(h.size) / fs
    k0 = int(t_from * fs)
    edc_db = 10 * np.log10(edc / edc[k0] + 1e-30)
    m = (t >= t_from) & (edc_db >= -30)
    return -60.0 / np.polyfit(t[m], edc_db[m], 1)[0]


def main() -> int:
    fs = 48000.0
    # 1. image-source geometry: first reflection from the side wall, closed form
    room = Room(4.0, 6.0, 3.0, {1000: 0.4})
    paths = image_paths(room, (1.0, 2.0), 0.0, (1.0, 3.0), 1.0, 1, r_ref=0.5)
    side = [p for p in paths if p.order == 1 and abs(p.angle_deg + 63.4349) < 0.01]
    exp_d = np.hypot(2.0, 1.0)
    check("image source: wall x=0 reflection", len(side) == 1 and abs(side[0].distance - exp_d) < 1e-9
          and abs(side[0].delay - exp_d / C) < 1e-12,
          f"d = {side[0].distance:.4f} m (exact {exp_d:.4f}), arrival {side[0].angle_deg:.2f} deg")

    # 2. decay of the rendered response = the room's T (omni mic), fitted on the late part
    omni = MicModel.load("u87ai_omni")
    rooms = [Room(2.0, 2.5, 2.4, {125: 0.35, 250: 0.3, 500: 0.25, 1000: 0.25, 2000: 0.22, 4000: 0.2, 8000: 0.18}, "vocal booth"),
             Room(6.0, 8.0, 3.5, {125: 0.8, 250: 0.7, 500: 0.65, 1000: 0.6, 2000: 0.55, 4000: 0.5, 8000: 0.45}, "live room"),
             Room(20.0, 12.5, 6.4, {125: 1.1, 250: 1.1, 500: 1.1, 1000: 1.1, 2000: 1.1, 4000: 1.1, 8000: 1.1},
                  "Surrey Studio 1 (V 1600 m3, RT60 1.10 s VERIFIED; plan 20 x 12.5 m INFERRED)")]
    for r in rooms:
        h, he, ht, paths, t_tail = render(r, omni, (r.lx / 2, r.ly * 0.3), 0.0, 0.5, 0.0, fs, return_parts=True)
        for b in (250.0, 1000.0, 4000.0):
            est = schroeder_t60(h, fs, b, t_tail + 0.02)
            ref = r.rt60[int(b)]
            check(f"{r.name}: T60 at {b:g} Hz", abs(est / ref - 1) < 0.10, f"{est:.3f} s (target {ref:.3f} s)")

    # 3. direct-to-reverberant ratio vs distance, omni, 1 kHz: statistical theory (r_c / r)^2
    r = rooms[1]
    A = -r.surface * np.log(1 - r.alpha(r.rt60[1000]))
    rc = np.sqrt(A / (16 * np.pi))
    for dist in (0.1, 0.3, 1.0, 2.0):
        h, he, ht, paths, t_tail = render(r, omni, (r.lx / 2, r.ly * 0.3), 0.0, dist, 0.0, fs, return_parts=True)
        hb = band(h, fs, 1000.0)
        n0, n1 = int(round(paths[0].delay * fs)), int(round(paths[1].delay * fs))
        e_dir = np.sum(hb[max(0, n0 - 96): max(n0 + 24, n1 - 24)] ** 2)   # direct sound up to the first reflection
        drr = 10 * np.log10(e_dir / (np.sum(hb ** 2) - e_dir))
        theory = 20 * np.log10(rc / dist)
        check(f"live room DRR at {dist:g} m (1 kHz, omni)", abs(drr - theory) < 3.0,
              f"{drr:+.1f} dB (statistical theory {theory:+.1f} dB, r_c = {rc:.2f} m)")

    # 4. the measured pattern acts on each reflection: side-wall reflections (arrival 60-120 deg) are weighted by
    #    the figure-8's measured response at that angle (deep side null), the omni's barely
    fig8 = MicModel.load("u87ai_figure8")
    room = Room(3.0, 8.0, 3.0, {1000: 0.6})
    paths = image_paths(room, (1.5, 2.0), 0.0, (1.5, 2.5), 1.0, 1)
    side = [p for p in paths if 60 <= abs(p.angle_deg) <= 120]
    def at1k(mic, ang, d):
        return float(np.interp(1000.0, mic.fg, mic.db(ang, d)) - np.interp(1000.0, mic.fg, mic.db(0.0, d)))
    o = [at1k(omni, p.angle_deg, p.distance) for p in side]
    e = [at1k(fig8, p.angle_deg, p.distance) for p in side]
    check("figure-8 weights side-wall reflections by its measured side null", len(side) == 2 and max(e) < min(o) - 10,
          ", ".join(f"{p.angle_deg:+.1f} deg: omni {a:+.1f} dB, figure-8 {b:+.1f} dB" for p, a, b in zip(side, o, e)))

    print(f"\n{sum(RESULTS)}/{len(RESULTS)} checks passed")
    return 0 if all(RESULTS) else 1


if __name__ == "__main__":
    sys.exit(main())
