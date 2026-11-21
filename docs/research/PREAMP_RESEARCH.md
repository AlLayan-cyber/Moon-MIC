# MOON MIC — Preamplifier Research Overview

Audit date: 2026-11-21 · Scope: Neve 1073, Tube-Tech MP1A (Avalon VT-737sp excluded)

Tags: **VERIFIED** / **INFERRED** / **UNKNOWN**, as defined in the per-device reports:
- [NEVE_1073_ANALYSIS.md](NEVE_1073_ANALYSIS.md)
- [TUBE_TECH_MP1A_ANALYSIS.md](TUBE_TECH_MP1A_ANALYSIS.md)

## 1. Documentation inventory

| Expected file | Found? | Actual file | Nature |
|---|---|---|---|
| `Tube-Tech_MP1A_Manual.pdf` | Partially | `ManMP1A-1.pdf` (2 pp.) | Official Lydkraft text. Appears to be only the first 2 pages (INFERRED) |
| `Neve_1073_Full_Schematic.pdf` | Yes (different name) | `1073-fullpak.pdf` (11 pp.) + identical duplicate `1073-fullpak (1).pdf` | Scanned original Neve drawings, 1970–74 |
| `Tube-Tech_MP1A_Specifications.pdf` | **No** | — | — |
| (not expected) | — | `tubetech_mp1a.gif` | **Third-party Gyraf Audio redraw**, 06-2002. Not a manufacturer document |

## 2. Side-by-side architecture

| Aspect | Neve 1073 (EH 10023 iss. 3, 1974) | Tube-Tech MP1A (manual + Gyraf redraw) |
|---|---|---|
| Input transformer | T1 **10468**, Hi/Lo primary switching (VERIFIED) | TD5968.0 / TR110/1, +10 dB step-up (VERIFIED: mfr step-up; Gyraf part no.) |
| Pad | Within sensitivity switch network (INFERRED) | −20 dB resistive pad **before** the transformer (VERIFIED) |
| Gain device | Discrete Si BJT: BC184C (VERIFIED) | Triodes: ECC83 ×2, ECC82 ×1 per channel per Gyraf (VERIFIED-as-drawn) |
| Gain stages | BA284 input amp (18 dB base) → BA283AV pre-amp → BA283AV class-A output (TR3 2N3055/BDY61) | V1a/V1b (switched cathode feedback + passive divider) → V2a (OT feedback) → V2b phase splitter → ECC82 push-pull |
| Gain control | 22-way sensitivity switch: mic −80…−20 dBm, line +10…−20 dBm (VERIFIED) | 20–70 dB in 5 dB steps; DI 10–60 dB (VERIFIED) |
| Output stage | Single-ended class A into transformer (INFERRED from topology) | Push-pull (VERIFIED). Tube count disagrees between sources |
| Output transformer | **LO1166**, 1K5 + 0.01 µF Zobel (VERIFIED) | TD5731.0 / TR142/1, tertiary winding used for feedback (VERIFIED-as-drawn) |
| HPF | B182/C: 50/80/160/300 (VERIFIED values) | 20 Hz 12 dB/oct, 40 Hz 6 dB/oct (VERIFIED) |
| Phantom | Not in the module drawing (UNKNOWN) | +48 V via 6.81 kΩ (VERIFIED) |
| Input impedance | UNKNOWN | UNKNOWN |
| Frequency response | UNKNOWN (no data) | Mfr: UNKNOWN. Gyraf table with unknown units (see MP1A report §3.5) |
| Noise / THD | UNKNOWN | UNKNOWN |
| Supply | B+ / B− pins; voltage UNKNOWN | B+ voltage UNKNOWN; series heaters with 30 Ω; ~24 VDC grid-offset node (VERIFIED-as-drawn) |

## 3. Manufacturer data vs measured data vs inference

| Category | Neve 1073 | Tube-Tech MP1A |
|---|---|---|
| Manufacturer specifications | None (drawings only) | Gain range, pad, HPF, max input, crosstalk, channel matching (S1) |
| Manufacturer circuit | Original Neve drawings (high value) | None |
| Third-party circuit | — | Gyraf redraw (unknown revision) |
| Independent measurements | **None** | Only the Gyraf "FREQ.RESP" table (units and provenance unknown) |
| Inferred behaviour | Class-A output, per-position feedback, transformer colouration | Gain-dependent LF corner, OT-loop feedback, transformer-limited headroom |

## 4. Gaps blocking nonlinear modelling (both units)
1. Transformer data (all five transformers: 10468, 31267, LO1166, TD5968.0, TD5731.0).
2. Supply voltages and PSU schematics.
3. Independent measurements: frequency/phase response per gain step, THD/IMD vs level and frequency, clip behaviour, EIN, input/output impedance.
4. MP1A official schematic or revision confirmation. Full MP1A manual / spec sheet.
5. 1073: missing switch drawings (EK20065/66, EK20105), the missing BA283/BA284 pages, and the nominal output level.

## 5. Recommendation
Before any modelling, obtain **measurement data from real units**: swept-sine IRs with harmonic separation, using the same Farina/Novak method as the Surrey dataset, at several gain settings and drive levels. Also obtain the official MP1A spec sheet and full manual. The documents on hand support block-level and component-level circuit study. They do not support calibrated modelling.
