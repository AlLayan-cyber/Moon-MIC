"""Room & distance model (research reference, Python). Horizontal plane only (decision 2026-11-23).

Why: in a real room "further away" is heard mostly through the room: the direct sound falls 6 dB per distance
doubling while the reflected/reverberant sound stays almost constant, and early wall reflections colour the sound.
The SIGNATURE model is quasi-anechoic by construction (Surrey windowed out everything after the first reflection).

Physics (no data invented):
  * Direct path and early reflections: image-source method for the four WALLS of a rectangular room (plan view
    only; floor and ceiling are not modelled, by decision). Each path k has length d_k, delay d_k / c, arrival
    angle theta_k relative to the microphone axis, wall reflection factor beta^n_k and spherical spreading 1/d_k.
    It is filtered by the MEASURED microphone response at that angle and distance (same spatial model as the
    plugin: Surrey 72 x 5 deg + proximity + manufacturer band edges) and by air absorption (ISO 9613-1).
  * Wall absorption is not taken from material tables: a room is defined by its size and its reverberation time
    T (per octave band if wanted). Mean absorption from Eyring: alpha = 1 - exp(-0.161 V / (S T)), beta = sqrt(1 - alpha).
  * Late tail (after the image orders run out): Gaussian noise with exponential decay exp(-6.91 t / T) per octave band,
    level from the statistical reverberant energy, coloured by the microphone's diffuse-field response, here the
    power average of the measured response over the 72 HORIZONTAL angles (horizontal-only approximation).
  * Level reference: the direct path at r_ref = 0.5 m on axis has gain 1 (the SIGNATURE reference).
"""
from __future__ import annotations

from dataclasses import dataclass, field

import numpy as np

C = 343.0


@dataclass
class Room:
    lx: float                 # m, width (x)
    ly: float                 # m, depth (y)
    height: float             # m, only for volume / surface in the statistical formulas
    rt60: dict = field(default_factory=dict)   # octave centre Hz -> T (s)
    name: str = ""

    @property
    def volume(self) -> float:
        return self.lx * self.ly * self.height

    @property
    def surface(self) -> float:
        return 2 * (self.lx * self.ly + self.lx * self.height + self.ly * self.height)

    def alpha(self, t60: float) -> float:
        """Eyring mean absorption that gives reverberation time t60 in this room."""
        return float(1.0 - np.exp(-0.161 * self.volume / (self.surface * t60)))


BANDS = np.array([125.0, 250.0, 500.0, 1000.0, 2000.0, 4000.0, 8000.0])


def band_value(room: Room, f: np.ndarray) -> np.ndarray:
    """T60(f): interpolated in log frequency between the given octave values, held at the ends."""
    fb = np.array(sorted(room.rt60))
    tb = np.array([room.rt60[k] for k in fb])
    return np.interp(np.log(np.maximum(f, 1.0)), np.log(fb), tb)


def air_absorption_db_per_m(f: np.ndarray, temp_c: float = 20.0, rh: float = 50.0, pa_kpa: float = 101.325) -> np.ndarray:
    """ISO 9613-1:1993 pure-tone atmospheric attenuation (dB/m)."""
    T, T0, T01 = temp_c + 273.15, 293.15, 273.16
    pr = pa_kpa / 101.325
    psat = 10 ** (-6.8346 * (T01 / T) ** 1.261 + 4.6151)
    h = rh * psat / pr
    fro = pr * (24 + 4.04e4 * h * (0.02 + h) / (0.391 + h))
    frn = pr * (T / T0) ** -0.5 * (9 + 280 * h * np.exp(-4.170 * ((T / T0) ** (-1 / 3) - 1)))
    f2 = f ** 2
    return 8.686 * f2 * (1.84e-11 / pr * (T / T0) ** 0.5 + (T / T0) ** -2.5 * (
        0.01275 * np.exp(-2239.1 / T) / (fro + f2 / fro) + 0.1068 * np.exp(-3352.0 / T) / (frn + f2 / frn)))


@dataclass
class Path:
    order: int
    distance: float
    delay: float
    angle_deg: float          # arrival angle at the mic, relative to its axis (+ = from the right)
    gain: float               # beta^n * r_ref / d (frequency-independent part)


def image_paths(room: Room, mic_xy, mic_axis_deg: float, src_xy, beta: float, max_order: int, r_ref: float = 0.5) -> list[Path]:
    """2-D image sources of the four walls (x in [0, lx], y in [0, ly]). Axis angle 0 = +y direction."""
    mx, my = mic_xy
    sx, sy = src_xy
    paths = []
    for nx in range(-max_order, max_order + 1):
        for ny in range(-max_order, max_order + 1):
            if abs(nx) + abs(ny) > max_order:
                continue
            ix = nx * room.lx + (sx if nx % 2 == 0 else room.lx - sx)
            iy = ny * room.ly + (sy if ny % 2 == 0 else room.ly - sy)
            n = abs(nx) + abs(ny)
            dx, dy = ix - mx, iy - my
            d = float(np.hypot(dx, dy))
            bearing = np.degrees(np.arctan2(dx, dy))                 # 0 = +y, positive towards +x
            ang = (bearing - mic_axis_deg + 180.0) % 360.0 - 180.0
            paths.append(Path(n, d, d / C, float(ang), beta ** n * r_ref / d))
    return sorted(paths, key=lambda p: p.delay)


def min_phase_from_mag(mag: np.ndarray) -> np.ndarray:
    """Minimum-phase spectrum (rfft layout) from a magnitude on the same bins (real cepstrum)."""
    n = 2 * (mag.size - 1)
    logm = np.log(np.maximum(mag, 1e-9))
    cep = np.fft.irfft(logm, n)
    cep[1:n // 2] *= 2.0
    cep[n // 2 + 1:] = 0.0
    return np.exp(np.fft.rfft(cep, n))
