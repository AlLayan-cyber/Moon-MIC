# Surrey Microphone IR Dataset — Methodology Extract

Audit date: 2026-11-21 · Stage: research audit (no DSP work)

Tags: **VERIFIED** = stated in the article, or measured directly from files in the archive (marked "archive check") · **INFERRED** = reasoned, not stated · **UNKNOWN** = not available.

## 1. Sources (article and dataset are separate items)

| Item | Location | Notes |
|------|----------|-------|
| **Article** | `docs/research/Surrey_Multi_Angle_Multi_Distance_Microphone_IR_Dataset_2022.pdf` | J.C. Franco Hernández, B. Bacila, T. Brookes, E. De Sena, "A Multi-Angle, Multi-Distance Dataset of Microphone Impulse Responses", *J. Audio Eng. Soc.* 70(10):882–893, Oct 2022, DOI 10.17743/jaes.2022.0027. 12 PDF pages. **PDF page n = journal page 881+n.** |
| Duplicate of the article | `docs/papers/22014.pdf` | Byte-identical (MD5 `8207fab6…`). Left untouched. |
| **Dataset archive** (not the article) | `data/raw/surrey_madir/original_zip/Microphone_Impulse_Responses.zip` (1.49 GB) and `data/raw/surrey_madir/extracted/…` | Zenodo DOI 10.5281/zenodo.4633508, CC BY. `contents.txt` says "Archive last updated 23 Aug 2021". |

Page references below are PDF pages (journal pages in brackets).

## 2. Measurement environment
| Item | Finding | Tag | Ref |
|------|---------|-----|-----|
| Room | Studio 1, Institute of Sound Recording, Univ. of Surrey: approx. 250 m², RT60 1.10 s, V 1,600 m³, S 901 m² | VERIFIED | p.3–4 [884–885] |
| Noise | NR23, 23.1 dBA ± 0.1 dB | VERIFIED | p.3 [884] |
| Geometry | Mics 3 m above the floor, 3.7 m from the lighting gantry. Only one distance rigged at a time. Doors closed. | VERIFIED | p.3 [884] |
| Method | **Quasi-anechoic**: time-windowing removes everything from the first reflection onward | VERIFIED | p.3, p.7 |
| Control noise | Laptop placed beyond the critical distance (≤ 2.51 m estimated; actual ≥ 3.6 m); 5 m cable to the turntable PSU | VERIFIED | p.4 [885] |

## 3. Equipment and stimulus
| Item | Finding | Tag | Ref |
|------|---------|-----|-----|
| Source | Genelec 8040 (manufacturer figures quoted: 48 Hz–20 kHz ±2 dB, etc.) | VERIFIED | p.4 [885] |
| Interface | RME Fireface 802 | VERIFIED | p.4 |
| Mic preamplifier | Called the "microphone amplifier" only. **Make and model not stated.** | UNKNOWN | p.6 [887] |
| Stimulus | Synchronized exponential swept sine (Novak), 20 Hz–20 kHz, 7 s + 3 s silence; frequency-domain deconvolution with the analytic inverse | VERIFIED | p.4–5 [885–886] |
| Recording | 48 kHz, **16-bit** capture | VERIFIED | p.6 [887] |
| Turntable | Custom: NEMA 17 stepper, TMC2100 driver, Arduino Uno, HC-06 Bluetooth, MATLAB control; precision ±0.1°; capsule aligned to the rotation axis on a K&M stereo bar; 10 s settle time before each sweep | VERIFIED | p.5–6 [886–887] |

## 4. Angles, distances, microphones
- Angles: 0°–355° in 5° steps, 72 per rotation, horizontal plane only (VERIFIED p.2, p.8).
- Distances: 0.5 m (proximity effect), 1.25 m (mid-field / datasheet-like), 5 m (far field) (VERIFIED p.2–3).
- 25 microphones, 38 configurations per distance. The Røde NTG8 was not measured at 0.5 m, giving 113 measurements and 8,136 IRs (VERIFIED p.8 [889]).
- Microphone list: Table 1, p.5 [886] (VERIFIED). The table gives model names only. **Serial numbers, production year and individual-unit condition are not given (UNKNOWN).**

### MOON MIC overlap
| MOON MIC mic | In dataset? | Patterns available (archive check) | Notes |
|---|---|---|---|
| Neumann U87 Ai | **Yes** | Cardioid, Omni, Bidirect × 3 distances | Exact model match (VERIFIED) |
| Neumann U47 FET | **Yes** | Cardioid × 3 distances | Exact model match. Archive anomaly: `Normalised_IRs/NeumannU47FET_5m/24bit` is empty, and its 72 24-bit files sit in the `32bit` folder (144 files) (VERIFIED, archive check) |
| AKG C414 B-XLS | **Yes** | Cardioid, Omni, Bidirect, **Hyper** × 3 | The paper lists C, O, B only. The archive also has a Hyper set (VERIFIED discrepancy) |
| Sony C-800G | **Partial** | Cardioid, Omni × 3 | The paper says "Sony C800". Whether it is the C-800G variant is not stated (UNKNOWN) |
| RØDE K2 | **Yes** | Cardioid, Omni, Bidirect × 3 | VERIFIED |
| Neumann U67 | No | — | Missing |
| AKG C12 | No | — | Missing |
| Shure SM7B | No | — | Missing (dataset has SM57/SM58 only) |
| Audio-Technica AT5040 | No | — | Missing |
| RØDE NT1 5th Gen | No | — | Missing (dataset has NT2-A, not NT1) |
| Telefunken ELA M 251E | No | — | Missing |

Out-of-scope note: the Royer R-121 is in the dataset but excluded from MOON MIC scope.

## 5. IR acquisition and post-processing
| Item | Finding | Tag | Ref |
|------|---------|-----|-----|
| Deconvolution | Farina / Novak method. Harmonic distortion products land before the linear IR, so the dataset is usable for HD analysis | VERIFIED | p.4–5, p.7 |
| Window | Hann window, direct sound centred. First reflections observed at 16.6 / 14.7 / 8.00 ms. Window = 2× that: **33.2 / 29.4 / 16.0 ms** at 0.5 / 1.25 / 5 m | VERIFIED | p.7 [888] |
| LF validity limit | One full period before the first reflection only above **60.2 / 68.0 / 125 Hz** (0.5 / 1.25 / 5 m). Below that, accuracy reduces | VERIFIED | p.7, p.10 |
| Paper inconsistency | The summary (p.10 [891]) says "68.0 Hz at 1.5 m". Elsewhere the distance is 1.25 m | VERIFIED typo |
| File format (archive check) | U87Ai Cardioid samples: mono, 48 kHz. 24-bit = PCM int, 32-bit = IEEE float. 480,000 samples (10 s). At 1.25 m the non-zero samples are 0–1410 (1,411 samples = 29.4 ms) with the peak at sample 705–706. So the delivered files are the **windowed** IR, zero-padded to 10 s | VERIFIED (one set checked) |
| Delivered bit depth vs capture | Captured at 16-bit, rendered to 24/32-bit. The extra bits add no resolution | INFERRED |

## 6. Calibration and normalisation
| Item | Finding | Tag | Ref |
|------|---------|-----|-----|
| Acoustic calibration | Once per session: NTi XL2 + M2211, pink noise from the 8040; **83 dB(A)** at 0.5 m and 1.25 m, **75 dB(A)** at 5 m | VERIFIED | p.6 [887] |
| Digital gain | Preamp gain **re-adjusted for every mic and rotation** so the signal reached −15 dBFS ± 1 dB | VERIFIED | p.6 [887] |
| Consequence | Raw IRs do **not** preserve absolute mic sensitivity across mics. Within one rotation the gain is fixed, so the relative angle levels are preserved | INFERRED |
| "Normalised" set | `contents.txt`: normalised to −1 dBFS peak | VERIFIED |
| Normalisation scope (archive check) | U87Ai Cardioid 125 cm, 32-bit: Normalised 0° peak = −1.00 dBFS, 90° = −11.49, 180° = −10.53. Raw is −25.50 / −35.99 / −35.03. Constant 24.5 dB offset, so normalisation is **per rotation set, keyed to 0°**, not per file. Angle-relative levels are preserved | VERIFIED for this set only. Other sets need checking |
| Loudspeaker / chain compensation | The paper describes **no** correction for the Genelec 8040 response, the preamp or the interface. The IRs contain the mic **plus** the loudspeaker and electronics | INFERRED (absent from text) |
| Reference mic | NTi M2211 (Class-1) measured in the dataset "for possible future use as a reference" (p.4). It could be used to estimate the loudspeaker+room-window response for deconvolving it out | VERIFIED presence; use is INFERRED |

## 7. Dataset limitations
1. Horizontal plane only. No elevation data (VERIFIED).
2. LF accuracy is reduced below 60–125 Hz depending on distance, so proximity-effect data is limited at the lowest frequencies (VERIFIED).
3. The measurement chain (Genelec 8040, unidentified preamp, RME) is not compensated (INFERRED).
4. Per-mic gain normalisation removes absolute sensitivity (VERIFIED procedure; consequence INFERRED).
5. One sample unit per model. Unit-to-unit variation is not captured (INFERRED; no serials given).
6. 16-bit capture limits dynamic range for off-axis nulls and HD analysis (INFERRED).
7. A point source at 0.5 m does not model a real near-field voice source (INFERRED).
8. Archive anomalies (VERIFIED, archive check):
   - `Normalised_IRs/NeumannU47FET_5m/24bit` is empty, and `32bit` holds 144 files (72 × 24-bit + 72 × 32-bit).
   - `Raw_IRs/Coles4038_5m/24bit` is empty (out of scope).
   - Extra `IRs/` sub-folders exist in `ShureSM58_125cm` and `SonyECM670_125cm` (Normalised and Raw). They hold 288 files total with no bit-depth suffix.
   - 116 folders per variant (39 at 125 cm and 5 m, 38 at 50 cm), against the paper's 38 per distance. The Schoeps CMC6 with and without KA40 are separate folders, and the AKG C414 Hyper set is not in Table 1.
   - Totals: 16,848 Normalised and 16,776 Raw WAVs.

## 8. Relevance to MOON MIC
- 5 of 11 target mics have angle- and distance-resolved IRs suitable for linear (FIR) modelling of on- and off-axis response and proximity trends above about 60 Hz (INFERRED).
- The data is suitable for harmonic-distortion analysis per the article. Practical use is limited by the 16-bit capture and the moderate SPL (83 dB(A)) (INFERRED).
- Before use as "mic-only" responses, the chain response must be estimated and removed, for example using the M2211 measurements (INFERRED, requires validation).
- Six target mics need another data source.

## 9. Confidence
| Area | Confidence |
|------|-----------|
| Methodology, geometry, stimulus, windowing | High (peer-reviewed article) |
| Normalisation behaviour | Medium (verified on one set; generalisation unchecked) |
| Chain compensation status | Medium (inferred from absence) |
| Model identity (C800 vs C-800G) | Low |
