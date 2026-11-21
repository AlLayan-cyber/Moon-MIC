# Tube-Tech MP1A — Source Analysis

Audit date: 2026-11-21 · Stage: research audit (no DSP work)

Tags: **VERIFIED** = stated directly in a source · **INFERRED** = reasoned from a source but not stated · **UNKNOWN** = not available in the sources on hand.

## 1. Sources on hand

| # | File | What it actually is | Pages | Authority |
|---|------|---------------------|-------|-----------|
| S1 | `docs/preamp_schematics/ManMP1A-1.pdf` | Lydkraft ApS (Tube-Tech) MP 1A manual extract. PDF metadata: created from "Manual MP 1A 040902.doc", 2005-06-10. Footer codes "040902" (p.1) and "040092" (p.2). | 2 | Manufacturer, but **partial**. The file name suffix "-1" and its 2 pages suggest it is the first part of a longer manual (INFERRED). |
| S2 | `docs/preamp_schematics/tubetech_mp1a.gif` | Schematic **redrawn by Gyraf Audio** (title block: "Gyraf Audio, www.gyraf.dk, Date 06-2002, Subject: TUBETECH MIC PREAMP MP 1A", Rev no. blank). 4173×1269 px, single image. | 1 | **Third party, not Tube-Tech.** Reverse-engineered or traced from an unknown unit. |

Expected but **not present**: `Tube-Tech_MP1A_Manual.pdf` (under that name), `Tube-Tech_MP1A_Specifications.pdf`. No official Tube-Tech schematic and no specification sheet (noise, THD, frequency response, impedances) were found.

MP2A check: neither S1 nor S2 mentions the MP2A. S1 is headed "TUBE-TECH MP 1A". No MP2A material is used in this report.

## 2. Manufacturer statements (S1, Lydkraft manual extract)

| Item | Finding | Tag | Ref |
|------|---------|-----|-----|
| Format | Two-channel combined mic and DI preamplifier | VERIFIED | S1 p.1 |
| Topology per channel | Mic input transformer (with static screen), +10 dB step-up → dual-tube preamplifier with stepped gain (5 dB/step, "passive and active") → output amplifier with "two dual tubes in push-pull" → output transformer (with static screen) | VERIFIED | S1 p.1 |
| Gain range (mic) | +20 dB to +70 dB, 5 dB steps | VERIFIED | S1 p.2 |
| Gain range (DI) | +10 dB to +60 dB | VERIFIED | S1 p.1 |
| DI input | High impedance, unbalanced, injected **after** the input transformer; mic input disabled when DI is in use | VERIFIED | S1 p.1 |
| Pad | Switchable −20 dB, placed **before** the mic transformer | VERIFIED | S1 p.1, p.2 |
| Max input level | +6 dBu (1.55 V) at 40 Hz without pad; +26 dBu (15.5 V) at 40 Hz with pad | VERIFIED | S1 p.1 |
| Phantom | +48 V via two 6.81 kΩ resistors, switchable | VERIFIED | S1 p.2 |
| HPF | Off / 20 Hz (12 dB/oct) / 40 Hz (6 dB/oct) | VERIFIED | S1 p.2 |
| Channel gain matching | < 0.5 dB at any gain setting | VERIFIED (mfr spec) | S1 p.1 |
| Crosstalk | Below −60 dB, 20 Hz–20 kHz | VERIFIED (mfr spec) | S1 p.1 |
| Input impedance | Not stated | UNKNOWN | — |
| Output impedance / max output level | Not stated | UNKNOWN | — |
| Frequency response | Not stated | UNKNOWN | — |
| Noise (EIN) | Not stated | UNKNOWN | — |
| THD / distortion | Not stated | UNKNOWN | — |
| Supply voltages (B+, heater) | Not stated | UNKNOWN | — |

Note: the "+10 dB step-up" input transformer plus the 20–70 dB range implies the tube stages provide roughly 10–60 dB (INFERRED, simple subtraction, ignores pad and loading losses).

## 3. Circuit content of the Gyraf redraw (S2): third-party, unverified against Tube-Tech

All values below are read from S2. They are **VERIFIED as what S2 shows** and **unverified as the circuit of production MP1A units**. The revision of the unit traced is UNKNOWN.

### 3.1 Input section
- Input transformer: "TR110/1 INPUT TRAFO, TRADANIA TD5968.0". Turns ratio and impedances are not given (UNKNOWN). Step-up of +10 dB is from S1 (≈1:3.16 voltage ratio if lossless, INFERRED).
- Phantom: 6k8 + 6k8 feed resistors (S1 says 6.81 kΩ; consistent within E-series rounding, INFERRED).
- Pad (SW2, "PAD-20"): resistor network 546 Ω / 133 Ω / 546 Ω in front of the transformer. Its source and load impedances depend on the transformer reflected impedance, which is UNKNOWN. Pad attenuation of 20 dB is from S1 (VERIFIED). The exact attenuation of the network in circuit is UNKNOWN.
- Transformer secondary load: series 1 kΩ + 2n7 (Zobel-type damping network; function INFERRED).
- Instrument input: jack "INST. IN" switching onto the secondary side, which matches S1 (VERIFIED, both sources).

### 3.2 Gain stages
- V1a (½ ECC83): grid via 470 nF and 1 MΩ; plate 51 kΩ; cathode network 1 kΩ / 750 Ω / 68 kΩ / 4u7 with feedback from stage 2 via 68 kΩ. Coupling 330 nF/250 V.
- V1b (½ ECC83): plate 51 kΩ, cathode 1 kΩ, grid 1 MΩ.
- Gain switch SW3A/SW3B (ganged, 20–70 dB): SW3A switches the stage-1 cathode feedback network (750 Ω, 1k4, 2k87, 6k19, 17k8, 140k); SW3B switches an inter-stage passive divider (13k3, 7k5, 4k2, 2k35, 3k09 (drawn "3K09"), …). This matches the manual's "passive and active" gain description (INFERRED match).
- HPF SW4A/SW4B: 330 nF and 15 nF switched, with 347 kΩ to ground. Labels Off/20 Hz/40 Hz. Slopes from S1.
- V2a (½ ECC83): grid 470 nF / 1 MΩ; plate 200 kΩ; cathode 2k2 bypassed by 10 µ/35 V; "GAIN TRIM" 470 Ω; **cathode feedback via 1 kΩ from a tertiary winding of the output transformer** ("CATODE FEEDBACK … PART OF OUTPUT TRAFO"). Global loop feedback through the output transformer is VERIFIED as drawn. Its exact loop gain is UNKNOWN.
- V2b (½ ECC83): split-load phase inverter (plate 100 kΩ, cathode 2k6 + 100 kΩ, grid bias 1 MΩ). The cathodyne description is INFERRED from the topology.

### 3.3 Output stage
- Push-pull **ECC82** (both triodes), each cathode 4k7 1 W bypassed by 47 µ/63 V; grid leaks 1 MΩ + 1 MΩ to an "OFFSET ~24 VDC" node (470 kΩ / 51 kΩ divider, 10 nF).
- Output transformer: "TR142/1 OUTPUT TRAFO, TRADANIA TD5731.0", centre-tapped primary to B+ (RED). Turns ratio and impedances are UNKNOWN.
- Heaters: V1, V2, V3 in series with 30 Ω. That is **three** dual triodes per channel (2× ECC83 + 1× ECC82) in this drawing.

### 3.4 Conflicts between S1 and S2
| Topic | S1 (Tube-Tech) | S2 (Gyraf) | Status |
|-------|----------------|------------|--------|
| Output amplifier | "two dual tubes in push-pull" | one ECC82 in push-pull, driven by ½ ECC83 phase splitter | **Unresolved.** S1 may count the ECC83 driver/splitter as one of the "two dual tubes" (INFERRED), or S2 may be a different revision. |
| Phantom resistors | 6.81 kΩ | 6k8 | Minor. Same within tolerance (INFERRED). |

### 3.5 "FREQ.RESP" table in S2
S2 contains a box labelled "FREQ.RESP:" with columns −3 dB and −6 dB:

| Gain | −3 dB | −6 dB |
|------|-------|-------|
| 70 dB | 115 | 60 |
| 65 dB | 105 | 36 |
| 60 dB | 45 | 20 |
| 55 dB | 24 | <10 |

- The table exists in S2 (VERIFIED). **Units, measurement conditions, source impedance, HPF setting and the unit measured are not stated (UNKNOWN).**
- Because −6 dB < −3 dB in every row, these are probably low-frequency corner frequencies in Hz, rising with gain (INFERRED). This would point to LF loss that depends on gain, through transformer/coupling interaction with the feedback network. Treat it as **third-party measured data of unknown provenance**. Do not treat it as a manufacturer spec.

## 4. Information relevant to nonlinear modelling (all INFERRED unless noted)
- Nonlinear elements in the path: input transformer core (TD5968.0), 4 ECC83 triode sections, 2 ECC82 triode sections, output transformer core (TD5731.0). The element list is VERIFIED from S2. Their contribution is INFERRED.
- Feedback: local cathode feedback on V1a (gain-switched) and an output-transformer tertiary winding feeding V2a's cathode. Distortion character will vary with gain setting (INFERRED).
- The pad is ahead of the transformer, so engaging it changes transformer drive level and source impedance (INFERRED from S1 placement).
- Max input of +6 dBu at 40 Hz without pad suggests the transformer's LF saturation sets the clip point (INFERRED; S1 specifies it at 40 Hz specifically).
- **No** B+ voltage, operating points, tube plate curves, transformer inductance/leakage/core data, THD or noise data are available (UNKNOWN).

## 5. Missing information (needed before modelling)
1. Complete official MP1A manual (pages after p.2), including the full specification table.
2. Official Tube-Tech schematic, or confirmation of which revision the Gyraf drawing represents.
3. Input impedance (with and without pad), output impedance, max output level.
4. Frequency response (manufacturer) and independent measurements per gain step.
5. EIN / noise, THD vs level and frequency.
6. B+ / heater voltages and the PSU schematic.
7. TD5968.0 and TD5731.0 transformer data (ratio, primary inductance, DCR, leakage, core material).
8. Independent bench measurements (sweeps, THD, IMD, step response) of a real unit.

## 6. Confidence
| Area | Confidence |
|------|-----------|
| Block architecture (transformer → tubes → push-pull → transformer) | High (two independent sources agree) |
| Gain range, pad, HPF, phantom, max input | High (manufacturer text) |
| Component values | Medium-low (single third-party drawing, unknown revision, some small text) |
| Frequency response / noise / THD | None (no manufacturer data; S2 table has unknown units and provenance) |
