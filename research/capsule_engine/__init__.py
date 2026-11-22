"""MOON MIC Physical Capsule Engine — offline Python reference (Stage 2).

Research code, not plugin code. Brief: docs/briefs/MOON_MIC_Physical_Capsule_Engine/.
Stage 1 report: docs/research/PHYSICAL_CAPSULE_ENGINE_STAGE1.md.

Modules
  capsule.py    constant-charge condenser capsule (electrostatic transduction non-linearity)
  triode.py     tube law (Koren) + common-cathode stage solved per sample (bilinear companions, Newton)
  oversample.py polyphase oversampling wrapper for the non-linear blocks
  analysis.py   THD / harmonic and aliasing measurement helpers
  tests.py      verification suite (VALIDATION_PLAN_RU.md §1-3); run: python -m research.capsule_engine.tests

No parameter in this package is a Sony / tube-maker specification unless it is listed with a source in
docs/research/c800g_capsule_parameters.csv. Test parameter sets are labelled TEST-ONLY.
"""
