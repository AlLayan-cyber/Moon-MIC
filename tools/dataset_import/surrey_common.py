"""Shared definitions for the Surrey multi-angle, multi-distance microphone IR dataset.

Source: Franco Hernandez et al., JAES 70(10), 2022; Zenodo 10.5281/zenodo.4633508.
Read-only access: nothing in data/raw is ever modified.
"""
from __future__ import annotations

import re
from dataclasses import dataclass
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parents[2]
SURREY_ROOT = (PROJECT_ROOT / "data" / "raw" / "surrey_madir" / "extracted"
               / "Microphone_Impulse_Responses" / "Microphone_Impulse_Responses")
SURREY_ZIP = PROJECT_ROOT / "data" / "raw" / "surrey_madir" / "original_zip" / "Microphone_Impulse_Responses.zip"

SAMPLE_RATE = 48_000
DISTANCES_M = {"50cm": 0.5, "125cm": 1.25, "5m": 5.0}
# Paper p.7 [888]: Hann window lengths (s) and LF validity limits (Hz) per distance.
WINDOW_S = {0.5: 0.0332, 1.25: 0.0294, 5.0: 0.0160}
LF_LIMIT_HZ = {0.5: 60.2, 1.25: 68.0, 5.0: 125.0}

PATTERNS = ("Bidirect", "Cardioid", "Omni", "Hyper")

# Surrey model string -> MOON MIC canonical id (only microphones in MOON MIC scope).
IN_SCOPE = {
    "NeumannU87Ai": "neumann_u87ai",
    "NeumannU47FET": "neumann_u47fet",
    "AKGC414": "akg_c414bxls",
    "SonyC800": "sony_c800",          # C-800G variant UNCONFIRMED
    "RodeK2": "rode_k2",
    "RoyerR121": "royer_r121",
    "ShureSM58": "shure_sm58",
    "ElectroVoiceRE20": "ev_re20",
    "RodeNT2A": "rode_nt2a",
}
REFERENCE_MIC = "NTiM2211"  # Class-1 measurement mic, used for chain compensation

_FOLDER_RE = re.compile(r"^(?P<model>.+?)_(?P<dist>50cm|125cm|5m)$")
_FILE_RE = re.compile(r"^(?P<stem>.+?)_(?P<dist>50cm|125cm|5m)_(?P<angle>\d+)Deg(?:_(?P<bits>24bit|32bit))?\.wav$")


@dataclass(frozen=True)
class SetKey:
    """One measurement set: model + pattern + distance (72 angles)."""
    model: str          # Surrey model string without pattern, e.g. NeumannU87Ai
    pattern: str        # Cardioid / Omni / Bidirect / Hyper / fixed
    distance_m: float

    @property
    def label(self) -> str:
        return f"{self.model}_{self.pattern}_{self.distance_m:g}m"


def split_model_pattern(name: str) -> tuple[str, str]:
    """'NeumannU87AiCardioid' -> ('NeumannU87Ai', 'Cardioid'); fixed-pattern mics -> 'fixed'."""
    for p in PATTERNS:
        if name.endswith(p):
            return name[: -len(p)], p
    return name, "fixed"


def parse_folder(folder: str) -> tuple[str, str, float] | None:
    m = _FOLDER_RE.match(folder)
    if not m:
        return None
    model, pattern = split_model_pattern(m.group("model"))
    return model, pattern, DISTANCES_M[m.group("dist")]


def parse_file(name: str) -> dict | None:
    m = _FILE_RE.match(name)
    if not m:
        return None
    return {"angle_deg": int(m.group("angle")), "file_bits": m.group("bits") or ""}
