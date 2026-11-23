#pragma once

#include "model/ModelLibrary.h"

#include <vector>

namespace moonmic
{

/** Designs the SIGNATURE filter for one microphone set at a given angle and distance.

    Spatial model (docs/architecture/03 §3, residual-preserving form, VERIFIED 1.25 -> 0.5 m):
        H(f, theta, r) = H_meas(f, theta, r0) + b(f) cos(theta) [G(f, r)/G(f, r0) - 1],
        G(f, r) = 1 + 1/(j k r)
    The FIR is minimum phase (zero added latency). The measured excess phase is NOT reproduced
    in this version (planned "high accuracy" mode, docs/architecture/04 §5).

    Runs on a background thread; never call from the audio thread. Deterministic. */
class SignatureDesigner
{
public:
    static constexpr double designSampleRate = 48000.0; // reference rate (model data is 48 kHz)
    static constexpr double firSeconds = 4096.0 / 48000.0; // FIR span, ~85 ms at any sample rate

    /** Polar response (dB) at one frequency (nearest model grid point) and distance, for all model
        angles (0, 5, ..., 355 deg). Same spatial model as targetMagnitudeDb. UI use. */
    static std::vector<float> polarDb (const ModelLibrary& lib, int setIndex, float freqHz, float distanceM);

    /** FIR length at a given sample rate (power of two, >= 85 ms). */
    static int firLengthFor (double sampleRate);

    struct Result
    {
        std::vector<float> fir;         // minimum-phase impulse response at the requested sample rate
        std::vector<float> targetMagDb; // target magnitude on the model's log-frequency grid
        float autoGainDb = 0.0f;        // gain that makes the pink-weighted (100 Hz-10 kHz) level 0 dB
    };

    /** Target magnitude (dB) on the model grid; identical algorithm to the Python reference. */
    static std::vector<float> targetMagnitudeDb (const ModelLibrary& lib, int setIndex,
                                                 float angleDeg, float distanceM);

    /** Designs the FIR directly at the host sample rate (no IR resampling: JUCE's resampled IR
        deviated by up to 1.8 dB at 96 kHz in testing). Above the data limit (20 kHz hold, 24 kHz grid
        end) the magnitude is held at its last value; the true response there is UNKNOWN. */
    static Result design (const ModelLibrary& lib, int setIndex, float angleDeg, float distanceM,
                          double sampleRate = designSampleRate);

    /** Minimum-phase FIR + auto gain from any magnitude curve on the model grid. */
    static Result designFromMagnitude (const ModelLibrary& lib, std::vector<float> magDb, double sampleRate);

    // ---- TRANSFORM v1 (docs/architecture/05): source mic/position -> target mic/position --------------
    // Provisional limits, identical to tools/export/export_signature_models.py (INFERRED, set after a pilot).
    static constexpr double transformMaxBoostDb = 12.0;
    static constexpr double transformFloorBelowDb = 30.0;
    static constexpr double transformFloorRampDb = 10.0;

    /** Correction (dB) = target - source with the boost limited where the source has little energy;
        cuts are never limited; source == target gives exactly 0 dB. Same algorithm as the Python reference. */
    static std::vector<float> transformCorrectionDb (const ModelLibrary& lib,
                                                     int sourceSet, float sourceAngleDeg, float sourceDistanceM,
                                                     int targetSet, float targetAngleDeg, float targetDistanceM);

    static Result designTransform (const ModelLibrary& lib,
                                   int sourceSet, float sourceAngleDeg, float sourceDistanceM,
                                   int targetSet, float targetAngleDeg, float targetDistanceM,
                                   double sampleRate = designSampleRate);
};

} // namespace moonmic
