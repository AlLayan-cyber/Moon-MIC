"""Render a room impulse response through a measured microphone model (research reference).

render(...) returns the impulse response h(t) of: singer (omni point source, Q = 1) -> room (4 walls, horizontal
image sources + statistical diffuse tail) -> microphone model (measured, angle- and distance-dependent).
"""
from __future__ import annotations

import sys
from dataclasses import dataclass
from pathlib import Path as FsPath

import numpy as np

from .room import BANDS, C, Room, air_absorption_db_per_m, band_value, image_paths, min_phase_from_mag

ROOT = FsPath(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "export"))
import export_signature_models as ex  # noqa: E402


@dataclass
class MicModel:
    sid: str
    fg: np.ndarray
    m32: tuple

    @staticmethod
    def load(sid: str) -> "MicModel":
        fg = ex.log_grid()
        curves = ex.load_mfr_curves()
        for model, pattern, s, _ in ex.SETS:
            if s == sid:
                _, _, mag, ph, b, _ = ex.build_set(model, pattern, s, curves, fg)
                m32 = (mag.astype(np.float32).astype(float), ph.astype(np.float32).astype(float),
                       b.real.astype(np.float32) + 1j * b.imag.astype(np.float32))
                return MicModel(sid, fg, m32)
        raise KeyError(sid)

    def db(self, angle_deg: float, distance_m: float) -> np.ndarray:
        return ex.model_response(*self.m32, self.fg, angle_deg, distance_m)


def band_weights(f: np.ndarray) -> np.ndarray:
    """(n_bands, n_bins) triangular weights in log frequency around the octave centres; columns sum to 1."""
    lf = np.log2(np.maximum(f, 1.0))
    lb = np.log2(BANDS)
    w = np.zeros((BANDS.size, f.size))
    for i in range(BANDS.size):
        if i == 0:
            w[i] = np.clip((lb[1] - lf) / (lb[1] - lb[0]), 0, 1)
        elif i == BANDS.size - 1:
            w[i] = np.clip((lf - lb[i - 1]) / (lb[i] - lb[i - 1]), 0, 1)
        else:
            w[i] = np.clip(np.minimum((lf - lb[i - 1]) / (lb[i] - lb[i - 1]), (lb[i + 1] - lf) / (lb[i + 1] - lb[i])), 0, 1)
    return w / np.maximum(w.sum(axis=0, keepdims=True), 1e-12)


def render(room: Room, mic: MicModel, mic_xy, mic_axis_deg: float, singer_distance: float, singer_angle_deg: float,
           fs: float = 48000.0, max_order: int = 2, seed: int = 1, tail: bool = True, r_ref: float = 0.5,
           return_parts: bool = False):
    t60_max = max(room.rt60.values())
    n = 1 << int(np.ceil(np.log2((1.5 * t60_max + 0.1) * fs)))
    f = np.fft.rfftfreq(n, 1 / fs)
    fq = np.maximum(f, 1.0)
    T = band_value(room, fq)
    alpha_f = 1.0 - np.exp(-0.161 * room.volume / (room.surface * T))
    beta_f = np.sqrt(1.0 - alpha_f)
    air = 10 ** (-air_absorption_db_per_m(fq) / 20.0)

    a = np.radians(mic_axis_deg + singer_angle_deg)
    src_xy = (mic_xy[0] + singer_distance * np.sin(a), mic_xy[1] + singer_distance * np.cos(a))
    paths = image_paths(room, mic_xy, mic_axis_deg, src_xy, 1.0, max_order, r_ref)
    # Hybrid: discrete wall reflections up to max_order (clearly audible early pattern, horizontal plane) plus a
    # statistical diffuse field from the first reflection on. The diffuse part carries the statistical reverberant
    # energy minus what the discrete reflections already carry, so the total matches room acoustics.
    t_tail = min(p.delay for p in paths if p.order == 1)
    cache = {}
    H = np.zeros(f.size, complex)
    for p in paths:
        key = (round(p.angle_deg), round(p.distance, 1) if p.distance < 2.0 else 2.0)
        if key not in cache:
            mag = 10 ** (np.interp(np.log(fq), np.log(mic.fg), mic.db(key[0], max(p.distance, 0.05))) / 20.0)
            cache[key] = min_phase_from_mag(mag)
        H += cache[key] * p.gain * beta_f ** p.order * air ** p.distance * np.exp(-2j * np.pi * f * p.delay)
    h_early = np.fft.irfft(H, n)

    h_tail = np.zeros(n)
    if tail:
        rng = np.random.default_rng(seed)
        t = np.arange(n) / fs
        W = band_weights(fq)
        # statistical reverberant energy for an omni receiver, relative to the direct energy at r_ref (Q = 1):
        #   E_rev = (r_ref / r_c)^2, r_c^2 = A / (16 pi), A = -S ln(1 - alpha) (Eyring equivalent area)
        fade = np.clip((t - t_tail) / 0.010, 0.0, 1.0)
        # microphone diffuse-field response: power average over the 72 horizontal angles (far field)
        diff_db = 10 * np.log10(np.mean([10 ** (mic.db(ang, 3.0) / 10) for ang in range(0, 360, 5)], axis=0))
        diff = 10 ** (np.interp(np.log(fq), np.log(mic.fg), diff_db) / 20.0)
        for i, fb in enumerate(BANDS):
            Tb = float(band_value(room, np.array([fb]))[0])
            alpha_b = room.alpha(Tb)
            A = -room.surface * np.log(1.0 - alpha_b)
            e_rev = r_ref ** 2 * 16 * np.pi / A
            env = np.exp(-6.91 * t / Tb) * fade
            amp = np.sqrt(W[i])                       # energy-complementary bands (independent noise per band)
            noise = np.fft.rfft(rng.standard_normal(n))
            sig = np.fft.irfft(noise * amp * diff, n) * env
            sig_omni = np.fft.irfft(noise * amp, n) * env
            # target: the band's share of a unit impulse (energy e_imp) x statistical energy left after t_tail.
            # Energies are discrete sums; the direct impulse of gain 1 at r_ref has energy 1.
            e_imp = float(np.sum(np.fft.irfft(amp, n) ** 2))
            beta_b = np.sqrt(1.0 - alpha_b)
            e_discrete = sum((p.gain * beta_b ** p.order) ** 2 for p in paths if p.order > 0) * e_imp
            e_target = max(0.0, e_rev * e_imp * np.exp(-13.82 * t_tail / Tb) - e_discrete)
            e_omni = float(np.sum(sig_omni ** 2))
            if e_omni > 0:
                h_tail += sig * np.sqrt(e_target / e_omni)
    h = h_early + h_tail
    if return_parts:
        return h, h_early, h_tail, paths, t_tail
    return h
