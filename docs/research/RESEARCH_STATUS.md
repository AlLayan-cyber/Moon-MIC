# MOON MIC — Research Status

Audit date: 2026-11-21 · Phase: **Initialization and research audit** (no DSP, no plugin code)

## 1. Project structure (as found; nothing moved, renamed or deleted)

```
MOON_MIC/
├─ build/                                   (empty)
├─ data/
│  ├─ calibration/{measurement_chain,reference_mics}/        (empty)
│  ├─ manifests/                            (empty)
│  ├─ processed/{aligned,derived_ir,frequency_responses,normalized,phase_responses,polar_responses}/ (empty)
│  └─ raw/
│     ├─ manufacturer_data/                 (empty)
│     ├─ micirp/{extracted,metadata,original_zip}/          (empty)
│     └─ surrey_madir/
│        ├─ original_zip/Microphone_Impulse_Responses.zip   (1,485,159,533 B)
│        ├─ extracted/Microphone_Impulse_Responses/Microphone_Impulse_Responses/
│        │     contents.txt, Normalised_IRs/ (116 dirs), Raw_IRs/ (116 dirs) — 33,624 WAV
│        └─ metadata/                       (empty)
├─ docs/
│  ├─ architecture/, decisions/             (empty)
│  ├─ papers/22014.pdf                      (duplicate of the Surrey article)
│  ├─ preamp_schematics/
│  │     1073-fullpak.pdf, 1073-fullpak (1).pdf (identical), ManMP1A-1.pdf, tubetech_mp1a.gif
│  └─ research/
│        Surrey_Multi_Angle_Multi_Distance_Microphone_IR_Dataset_2022.pdf
│        + the 5 reports created in this audit
├─ dsp/{convolution,nonlinear_models,physical_models,preamp_models,transfer_functions}/ (empty)
├─ plugin/{presets,resources,source,tests}/ (empty)
├─ research/{notebooks,plots,reports,scripts}/ (empty)
├─ tools/{dataset_import,export,ir_analysis,measurement}/ (empty)
└─ validation/{frequency,listening,nonlinear,null,phase,reference}_tests/ (empty)
```
No source code exists yet. No git repository.

### Duplicates (kept untouched; flagged for owner decision)
- `docs/preamp_schematics/1073-fullpak (1).pdf` is identical to `1073-fullpak.pdf` (MD5 `0ebf7665…`).
- `docs/papers/22014.pdf` is identical to `docs/research/Surrey_…_2022.pdf` (MD5 `8207fab6…`).

### Directories suggested for the next research stage (not created)
- `docs/preamp_schematics/neve_1073/` and `…/tube_tech_mp1a/`, to separate devices and keep manufacturer and third-party sources apart.
- `docs/microphones/<model>/`: datasheets and manuals for each of the 11 mics.
- `data/raw/preamp_measurements/{neve_1073,tube_tech_mp1a}/`: bench measurements.
- `data/raw/manufacturer_data/<model>/`: already exists but is empty; needs per-mic subfolders.
- `data/raw/<additional_ir_source>/`: for the six mics not covered by Surrey.
- `docs/research/sources.md`: a bibliography with file hashes.

## 2. Analysis status

| Source | Status | Report |
|---|---|---|
| Neve 1073 fullpak (11 pp.) | Fully read (text + 9 scanned drawings read visually) | [NEVE_1073_ANALYSIS.md](NEVE_1073_ANALYSIS.md) |
| MP1A manual extract (2 pp.) | Fully read | [TUBE_TECH_MP1A_ANALYSIS.md](TUBE_TECH_MP1A_ANALYSIS.md) |
| MP1A Gyraf schematic GIF | Fully read (third-party) | same |
| Surrey JAES 2022 article (12 pp.) | Fully read | [SURREY_DATASET_METHODology.md](SURREY_DATASET_METHODology.md) |
| Surrey dataset archive | Structure audited; one IR set spot-checked (format, window, normalisation) | same |
| MicIRP folder | Empty, nothing to analyse | — |
| Combined preamp overview | Done | [PREAMP_RESEARCH.md](PREAMP_RESEARCH.md) |

## 3. Coverage vs scope

### Microphones (11)
| Mic | Linear IR data | Manufacturer data on disk |
|---|---|---|
| Neumann U87 Ai | Surrey (C/O/B) | None |
| Neumann U47 FET | Surrey (C); 5 m 24-bit folder anomaly | None |
| AKG C414 B-XLS | Surrey (C/O/B/Hyper) | None |
| Sony C-800G | Surrey "C800" (C/O); variant unconfirmed | None |
| RØDE K2 | Surrey (C/O/B) | None |
| Neumann U67 | **None** | None |
| AKG C12 | **None** | None |
| Shure SM7B | **None** | None |
| Audio-Technica AT5040 | **None** | None |
| RØDE NT1 5th Gen | **None** | None |
| Telefunken ELA M 251E | **None** | None |

### Preamps (2)
| Preamp | Circuit docs | Manufacturer specs | Measurements |
|---|---|---|---|
| Neve 1073 | Good (original 1970–74 drawings, partial) | None | None |
| Tube-Tech MP1A | Third-party only | Partial (2 pp.) | None (one third-party table of unknown units) |

## 4. Key issues found
1. No official MP1A schematic or spec sheet. The schematic on hand is a Gyraf Audio redraw. Sources disagree on the output tube count.
2. The 1073 docs lack the PSU, transformer data, three switch drawings and some board pages. The output transistor is 2N3055 on the schematic but BDY61 in the parts list.
3. Surrey IRs include an uncompensated measurement chain (loudspeaker + unidentified preamp). Absolute sensitivity is removed by per-rotation gain setting.
4. Surrey archive anomalies: U47 FET 5 m 24-bit files misplaced; stray `IRs/` folders; Coles 4038 raw 5 m 24-bit missing.
5. Six of the 11 target mics have no IR data.
6. No measured nonlinear data exists for either preamp.

## 5. What to obtain next (no downloads were made in this audit)
**Priority 1**
- The full official Tube-Tech MP1A manual and specification sheet (Tube-Tech / Lydkraft).
- Neve 1073 PSU schematic, transformer data (Marinair/Carnhill/St Ives 10468, 31267, LO1166), and the missing EK20065/EK20066/EK20105 and BA283/BA284 pages.
- An IR or measurement source for U67, C12, SM7B, AT5040, NT1 5th Gen and ELA M 251E. The MicIRP folder is prepared but empty.

**Priority 2**
- Manufacturer datasheets (frequency response, polar plots, sensitivity, self-noise, max SPL) for all 11 mics into `data/raw/manufacturer_data/`.
- A plan for bench measurements of real 1073 and MP1A units (swept-sine with harmonic separation, several gain settings and drive levels, impedance, EIN).
- Confirmation from the Surrey authors / Zenodo of the measurement preamp model and whether the Sony mic is a C-800G.

**Priority 3**
- Check Surrey normalisation on all sets (only one set was checked).
- A chain-compensation strategy using the NTi M2211 reference measurements.

## 6. Readiness for the next research phase
**Partially ready.**
- **Ready:** linear mic-response research (on- and off-axis, distance) for 5 mics using the Surrey dataset, and circuit-level study of the Neve 1073.
- **Not ready:** MP1A circuit modelling (no authoritative schematic or specs), nonlinear modelling of either preamp (no measurements, transformer data or supply voltages), and any work on the 6 uncovered microphones.
