# Neve 1073 — Source Analysis

Audit date: 2026-11-21 · Stage: research audit (no DSP work)

Tags: **VERIFIED** = stated or drawn directly in a source · **INFERRED** = reasoned from a source but not stated · **UNKNOWN** = not available in the sources on hand.

## 1. Source on hand

| File | Notes |
|------|-------|
| `docs/preamp_schematics/1073-fullpak.pdf` | 11 pages, 5,126,918 bytes, MD5 `0ebf7665…cfaa1804`. Compiled by technicalaudio.com ("VERSION 0.1a"). PDF created 2002-04-12 (Acrobat PDFWriter 4.05, Mac). Pages 1 and 4 are typed text. The other 9 pages are scanned original Neve drawings (JPEG, 2475×1575, one at 1188×756). |
| `docs/preamp_schematics/1073-fullpak (1).pdf` | **Byte-identical duplicate** (same MD5). Left untouched. |

Expected file name `Neve_1073_Full_Schematic.pdf`: not present under that name. The fullpak above is the equivalent.

The scanned pages have no text layer. Their content was read visually from the embedded images, which were extracted to a temporary scratch folder. The source PDF was not modified. Page numbers for scanned pages follow the order of the embedded image streams. That order matches the cover's contents list and the text pages at p.1 and p.4 (INFERRED, high confidence).

| PDF p. | Content | Drawing no. |
|--------|---------|-------------|
| 1 | Cover / contents (text) | — |
| 2 | 1073 Channel Amp module schematic | **EH 10023** |
| 3 | Sensitivity switch | **EK 20033** |
| 4 | BA283 info sheet (text, technicalaudio.com) | — |
| 5 | BA283AV/AVA PCB description, layout, partial parts list, "gain boost connections" | (text refers to EX10283) |
| 6 | BA283 AM & AV circuit | **EX/10 283** |
| 7 | BA284 PCB layout, parts list, gain table (low-resolution scan, 1188×756) | — |
| 8 | BA284 circuit | **EX/10,284** |
| 9 | B182 C & D HPF board | **D/10,019 C&D** |
| 10 | B205 HF/LF EQ board | **D/10,042** |
| 11 | B211 presence EQ board | **D/10,048** |

The cover (p.1) states these are missing: module description, module component locations and parts list, LF switch EK20065, HPF switch EK20066, presence switch EK20105 (VERIFIED). The cover lists BA283 as "5 pages" and BA284 as "3 pages", but only 3 and 2 pages respectively are present (INFERRED from the page map). **No power-supply schematic is included.**

## 2. Revision identification

| Drawing | Title-block revision history (as read) | Tag |
|---------|----------------------------------------|-----|
| EH 10023 (1073 Channel Amp), Rupert Neve & Co., ©1973 | Issue 1: 6/8/73 · Issue 2: 30-11-73 (CN 10816) · **Issue 3: 17.5.74 (CN 10907)** | VERIFIED |
| EK 20033 (Sensitivity switch) | Issue 1: 19-10-73 · Issue 2: 19-3-74 (CN 10878) · **Issue 3: 27-6-74 (CN 10907)** | VERIFIED |
| EX/10 283 (BA283 AM & AV), Neve Group | Drawn 15/11/71 (CN 10,361) · A: 8-3-72 (10473) · B: 7/6/72 (10518) · C: 15/11/72 (10565) · **D: 23-11-72 (10547)** | VERIFIED. Rev D's CN number is lower than rev C's. This could be a scan/reading ambiguity or an original clerical oddity (UNKNOWN). |
| EX/10,284 (BA284) | Drawn 15/11/71 (CN 10,362) · A: 8-12-71 (10,402) · B: 8-3-72 (10473) · **C: 15/11/72 (10565)** | VERIFIED |
| D/10,019 C&D (B182) | Drawn 21/10/70 · **A: 19/7/71 (10,299)** | VERIFIED |
| D/10,042 (B205) | Drawn 24 Feb 71 · A: 12/2/71 (10,241). The revision date is earlier than the drawing date (oddity, UNKNOWN cause). | VERIFIED |
| D/10,048 (B211) | Drawn 21/12/70, no revisions shown | VERIFIED |

**Caveat:** these drawings describe the 1973–74 documentation set. They do **not** prove the circuit of any particular commercial unit. Production 1073s span many years, board variants (e.g. BA283AV vs AVA, B182 C vs D), transformer suppliers (Marinair, St Ives, Carnhill) and later reissues. The drawing set shows which variant was intended. Any specific unit needs to be checked on its own.

## 3. Signal chain (EH 10023, p.2)

```
MIC IN (A,B) → S1 HI/LO → T1 (10468) ─┐
                                       ├→ S2 Sensitivity (EK20033) → BA284 → BA283/AV → [EQ path: B182/C HPF, B205 LF/HF, B211 presence]
LINE IN (H,F) → T2 (31267) ───────────┘                                         → T3 (LO1166) → S6 Phase → Balanced output (P,R,T)
```

The order of HPF and EQ relative to the amp boards is drawn through switch and pin connections (S3, S4, S5, S7 "EQU." in/out, EXT. LINK). The exact insertion point should be confirmed when the missing switch drawings are found (INFERRED).

### 3.1 Input transformer
- Mic: **T1, part 10468**. Primary pins 2/4 and 3/5 are switched by **S1 HI/LO** (impedance selection). Secondary pins 7/10 are loaded by **C1 180 pF** (VERIFIED, p.2).
- The actual Hi/Lo impedances, turns ratio and inductance are not given (UNKNOWN). The S1 wiring re-configures the primary windings in series or parallel (INFERRED).
- Line: **T2, part 31267**. Secondary loaded by **C6 2n2** (VERIFIED, p.2).

### 3.2 Sensitivity / gain switching (EK 20033, p.3, and EH 10023 p.2)
- 3-wafer switch S2a/S2b/S2c, "STOPPED DOWN TO 22 WAY" (VERIFIED).
- Mic positions labelled −80 … −20 dBm. Line positions labelled +10 … −20 dBm, with OFF positions between groups (VERIFIED). The unlabelled detents between the 10 dB labels give 5 dB steps (INFERRED from detent count).
- S2a selects the transformer-secondary tap and pad. S2b selects the BA284/BA283 feedback-resistor ladder (3K9, 3K3, 2K7, 2K7, 1K5, 18K, 12K, 3K9, 1K8, 1K, 510, 220, 33, 27, 47, 82, 150, 270, 430, 2K2). S2c switches 120 Ω and 330 Ω (R29, R30) (VERIFIED values). The function of each wafer is INFERRED from the connections.
- The 4.3 kΩ / 3.9 kΩ / 3.3 kΩ / 2.2 kΩ resistors (R3–R6) at −20 to −40 dBm act as input attenuation on the mic path (INFERRED).
- Sensitivity labels give the input level for nominal output. The nominal output reference level itself is not stated (UNKNOWN). Absolute gain in dB therefore cannot be stated directly from these sources.

### 3.3 BA284: first amplifier (EX/10,284 p.8; parts list p.7)
- Three circuits on one card (VERIFIED, p.7 text): "an input amplifier similar to that on the BA283 board together with two similar operational amplifiers".
- Input amplifier T1–T3, all **BC184C**: T1 common-emitter with 1500 pF / 100 pF / 680 pF compensation, T2 second stage, T3 emitter-follower output. Bias network 33K/47K, feedback 68K, 120K, 2K2, 390 Ω, 1K8, 51K; C9 400 µF/4 V; C6 125 µF/4 V (VERIFIED as drawn).
- Two "operational amplifiers" T4–T6 and T7–T9 (BC184C), 3-transistor discrete stages (VERIFIED). Their use in the 1073 is probably as EQ gain/buffer stages (INFERRED; pins C/D/E/A and K/L/M/H tie to the EQ section on EH10023).
- **Gain table (p.7, VERIFIED):** base gain 18 dB. A resistor across pins T–V raises it in 5 dB steps: 23 dB/330 Ω, 28/120, 33/56, 38/27, 43/15, 48/8.2 Ω.

### 3.4 BA283/AV: second gain stage and output amplifier (EX/10 283 p.6; p.4–5)
- BA283AV = pre-amp stage (TR4–6, pins N–V, "BA283NV" section) + output stage (TR1–3, pins A–M, "BA283AM" section) (VERIFIED p.5). The 1073 uses **BA283AV** (VERIFIED p.1 cover and p.2 "BA283/AV").
- Pre-amp section: TR4, TR5, TR6 = BC184C. Topology mirrors the BA284 input amp (R9 120K, R10 68K, R11 33K, R12 47K, R13 5K1, R14 470, R15 1K5, R16 10K, R17 2K2, R18 390, R19 1K8, R20 51K; C8 10 µ, C9 100 p, C10 1500 p, C11 680 p, C12 22 µ, C13 125 µ/4, C14/C15 22 µ/16, C16 1000 p, C17 400 µ/4) (VERIFIED).
- Output section: TR1 BC184C, TR2 BC184C, **TR3 = "2N3055" on the schematic (p.6) but "BDY61" in the parts list (p.5)** (VERIFIED discrepancy; which part a given unit uses is UNKNOWN). Class-A single-ended output with TR3 driving the output transformer. Bias set by RV1 4K7 and R6 18K, R7 47 Ω emitter; C4/C6/C7 80 µ/25 V; local feedback R2 56K, R3 68K, R4 1K2, R5 3K3, C5 330 p, C2 220 p, C3 4700 p (VERIFIED values; class-A description INFERRED from topology).
- "A"-suffix boards (AVA, AMA) mount TR3 off-card on a heat sink for high-current use (VERIFIED p.4, p.5).
- "Gain boost connections" (p.5): R(F–E) plus 80 µF/25 V + R to N (VERIFIED as drawn; values not given, UNKNOWN).

### 3.5 Output stage and transformer
- **T3, part LO1166** (VERIFIED p.2, p.6). Secondary damping **R64 1K5 + C9 0.01 µF** (VERIFIED p.2). Phase switch S6 → balanced output pins P/R/T (VERIFIED).
- Turns ratio, impedances, inductance, gapping and core: UNKNOWN. LO1166 appears to be a gapped output transformer for the single-ended class-A stage (INFERRED from the topology; not stated).
- Supply decoupling on the module: R56 12 Ω, R57 120 Ω, R51 270 Ω with C7/C8/C10 470 µ/25 V, C4 470 µ/25 V, C5 100 µ/25 V (VERIFIED, p.2).

### 3.6 Filters / EQ (outside the core preamp, recorded for completeness)
- B182/C HPF (D/10,019): inductor **T1295** with tapped capacitor ladder 1.0/0.47/0.22/0.1/0.022 µF and 10K (VERIFIED). Frequencies per the EH10023 table: OFF/50/80/160/300 Hz (VERIFIED; units "Hz" INFERRED).
- LF (S3, RV2 50K): OFF/35/60/110/220 (VERIFIED table; Hz INFERRED). HF section on B205 (620 Ω, 22 nF, 6K8, capacitor ladders). The HF frequency is not in the table (UNKNOWN from this source).
- B211 presence: inductors **T1280** and **T1530**, amplifier block, 10K pot. Table: OFF/.36/.7/1.6/3.2/4.8/7.2 (VERIFIED; kHz INFERRED).

## 4. Power supply
- Module supply pins B+ (U), B− (E), V earth (VERIFIED p.2).
- **Supply voltage is not stated anywhere in this PDF (UNKNOWN).** No PSU schematic is present.

## 5. Frequency and phase characteristics
- No frequency-response, phase, noise or distortion data are documented in this PDF (UNKNOWN).
- Elements that will shape the response: T1/T2/T3 transformers; C1 180 pF and C6 2n2 secondary loading; C2 470 p, C3 180 p, R64/C9 Zobel; HF compensation capacitors on BA283/BA284; electrolytic coupling and decoupling capacitors (VERIFIED as components; their effect is INFERRED).

## 6. Relevance to nonlinear modelling (INFERRED)
- Nonlinear elements: three transformers (input, line, output), BC184C stages, the class-A TR3 output (2N3055/BDY61) driving LO1166.
- The sensitivity switch changes both input attenuation and amplifier feedback (S2b ladder). So drive level, closed-loop gain and therefore distortion vary per switch position. A model needs per-position behaviour.
- The 4 V-rated large emitter-bypass capacitors (125 µF/4 V, 400 µF/4 V) set LF behaviour and possibly level-dependent LF distortion.
- Without transformer data and supply voltage, headroom and clip points cannot be derived from these sources.

## 7. Missing information
1. PSU schematic and supply rail voltage.
2. T1 10468, T2 31267, T3 LO1166 datasheets (ratios, impedances, inductances, DCR, core).
3. The missing BA283 pages (2 of the 5 listed) and BA284 page (1 of the 3 listed); full BA283AV parts list (resistors and C1–C9 not visible).
4. EK20065 / EK20066 / EK20105 switch drawings, module description, and nominal output level reference.
5. Any manufacturer or independent measurements: frequency/phase response per gain, EIN, THD vs level and frequency, max output.
6. Confirmation of which revision or variant matches any reference unit used for measurement.

## 8. Confidence
| Area | Confidence |
|------|-----------|
| Block architecture and board allocation | High (original drawings) |
| Component values on EH10023 / EX10283 / EX10284 | High-medium (legible scans; a few values small) |
| Revision dates | High (one CN-ordering oddity noted) |
| Gain in absolute dB | Low (output reference level unknown); BA284 gain table High |
| Transformer, PSU, measured response, noise, THD | None in the sources on hand |
