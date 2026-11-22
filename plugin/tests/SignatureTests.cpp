// MOON MIC — SIGNATURE test runner (docs/architecture/10, 11).
// 1. Model data loads and has the expected shape.
// 2. FFT round trip.
// 3. C++ target magnitude == Python reference (tools/export/export_signature_models.py).
// 4. Minimum-phase FIR reproduces the target magnitude (70 Hz - 18 kHz).
// 5. FIR is minimum phase (energy at the start) and design is deterministic.

#include "model/ModelLibrary.h"
#include "dsp/SignatureDesigner.h"
#include "dsp/SimpleFFT.h"

#include <juce_core/juce_core.h>

#include <cmath>
#include <complex>
#include <cstdio>

using namespace moonmic;

namespace
{
    int failures = 0;

    void check (bool ok, const juce::String& what)
    {
        std::printf ("[%s] %s\n", ok ? "PASS" : "FAIL", what.toRawUTF8());
        if (! ok) ++failures;
    }

    /** Magnitude (dB) of an FIR at frequency f (Hz), evaluated by direct DTFT. */
    double firMagDb (const std::vector<float>& h, double f, double fs)
    {
        std::complex<double> acc;
        const double w = -2.0 * juce::MathConstants<double>::pi * f / fs;
        for (size_t n = 0; n < h.size(); ++n)
            acc += (double) h[n] * std::polar (1.0, w * (double) n);
        return 20.0 * std::log10 (std::abs (acc) + 1e-12);
    }
}

int main()
{
    ModelLibrary lib;
    std::string err;
    check (lib.loadEmbedded (err), "model data loads (" + juce::String (err) + ")");
    if (lib.numSets() == 0) return 1;
    check (lib.numSets() == 19, "19 microphone sets, got " + juce::String (lib.numSets()));
    check (lib.numAngles() == 72, "72 angles");

    // FFT round trip
    {
        SimpleFFT fft (10);
        std::vector<std::complex<double>> x ((size_t) fft.size()), y;
        juce::Random rnd (1234);
        for (auto& v : x) v = { rnd.nextDouble() - 0.5, rnd.nextDouble() - 0.5 };
        y = x;
        fft.forward (y); fft.inverse (y);
        double e = 0.0;
        for (size_t i = 0; i < x.size(); ++i) e = std::max (e, std::abs (x[i] - y[i]));
        check (e < 1e-12, "FFT round trip error " + juce::String (e));
    }

    // Python reference
    const juce::File refFile (MOONMIC_REFERENCE_JSON);
    const auto ref = juce::JSON::parse (refFile);
    const auto* cases = ref["cases"].getArray();
    check (cases != nullptr && ! cases->isEmpty(), "reference file " + refFile.getFullPathName());
    if (cases == nullptr) return 1;

    double worstRef = 0.0;
    for (const auto& c : *cases)
    {
        const int idx = lib.indexOf (c["id"].toString().toStdString());
        if (idx < 0) { check (false, "unknown id " + c["id"].toString()); continue; }
        const auto mine = SignatureDesigner::targetMagnitudeDb (lib, idx, (float) (double) c["angle_deg"],
                                                                (float) (double) c["distance_m"]);
        const auto* expected = c["mag_db"].getArray();
        for (int k = 0; k < expected->size(); ++k)
            worstRef = std::max (worstRef, std::abs ((double) mine[(size_t) k] - (double) (*expected)[k]));
    }
    check (worstRef < 0.01, "C++ target == Python reference over " + juce::String (cases->size())
                                + " cases, max |diff| = " + juce::String (worstRef, 5) + " dB");

    // FIR accuracy, minimum phase, determinism
    const auto& f = lib.frequencies();
    struct Case { const char* id; float angle, dist; };
    double worstFir = 0.0;
    juce::String worstWhere;
    for (auto cs : { Case { "u87ai_cardioid", 0.0f, 0.5f }, Case { "u87ai_figure8", 0.0f, 0.1f },
                     Case { "r121", 30.0f, 0.15f }, Case { "sm58", 0.0f, 1.25f },
                     Case { "c414bxls_omni", 90.0f, 0.5f }, Case { "re20", 180.0f, 0.3f } })
    {
        const int idx = lib.indexOf (cs.id);
        const auto r = SignatureDesigner::design (lib, idx, cs.angle, cs.dist);
        for (size_t k = 0; k < f.size(); k += 4)
        {
            if (f[k] < 70.0f || f[k] > 18000.0f) continue;
            const double e = std::abs (firMagDb (r.fir, f[k], SignatureDesigner::designSampleRate) - r.targetMagDb[k]);
            if (e > worstFir) { worstFir = e; worstWhere = juce::String (cs.id) + " @ " + juce::String (f[k], 0) + " Hz"; }
        }

        double eTotal = 0.0, eHead = 0.0;
        for (size_t n = 0; n < r.fir.size(); ++n)
        {
            const double e2 = (double) r.fir[n] * r.fir[n];
            eTotal += e2;
            if (n < 256) eHead += e2;
        }
        check (eHead / eTotal > 0.95, juce::String (cs.id) + ": >95% energy in first 256 samples ("
                                          + juce::String (100.0 * eHead / eTotal, 2) + "%)");

        const auto again = SignatureDesigner::design (lib, idx, cs.angle, cs.dist);
        check (again.fir == r.fir, juce::String (cs.id) + ": deterministic design");
    }
    check (worstFir < 0.5, "FIR magnitude vs target (70 Hz-18 kHz) max error "
                               + juce::String (worstFir, 3) + " dB at " + worstWhere);

    std::printf ("\n%s (%d failure%s)\n", failures == 0 ? "ALL TESTS PASSED" : "TESTS FAILED",
                 failures, failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}
