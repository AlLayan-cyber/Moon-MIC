// MOON MIC — processor-level test: the audio path really applies the designed filter.
// Instantiates the plugin processor (no host, no editor), lets the design thread deliver the IR,
// sends an impulse, and compares the measured output magnitude with the designer's target,
// at 48 kHz and 96 kHz (the convolution engine resamples the 48 kHz IR).

#include "PluginProcessor.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <cmath>
#include <complex>
#include <cstdio>
#include <thread>

using namespace moonmic;

namespace
{
    int failures = 0;

    void check (bool ok, const juce::String& what)
    {
        std::printf ("[%s] %s\n", ok ? "PASS" : "FAIL", what.toRawUTF8());
        if (! ok) ++failures;
    }

    double magDbAt (const std::vector<float>& h, double f, double fs)
    {
        std::complex<double> acc;
        const double w = -2.0 * juce::MathConstants<double>::pi * f / fs;
        for (size_t n = 0; n < h.size(); ++n)
            acc += (double) h[n] * std::polar (1.0, w * (double) n);
        return 20.0 * std::log10 (std::abs (acc) + 1e-12);
    }

    void setParam (MoonMicProcessor& p, const char* id, float plainValue)
    {
        auto* param = p.getState().getParameter (id);
        param->setValueNotifyingHost (param->convertTo0to1 (plainValue));
    }

    /** Run the processor at fs; returns the response to a unit impulse (left channel). */
    struct Source { const char* id = nullptr; float distCm = 15.0f, angle = 0.0f; };

    std::vector<float> impulseResponse (double fs, const char* micId, float distCm, float angle,
                                        float levelDbSpl = 120.0f, float impulse = 1.0f, Source src = {})
    {
        MoonMicProcessor p;
        setParam (p, ParamIDs::level, levelDbSpl);
        if (src.id != nullptr)
        {
            setParam (p, ParamIDs::transform, 1.0f);
            setParam (p, ParamIDs::srcMic, (float) p.getLibrary().indexOf (src.id));
            setParam (p, ParamIDs::srcDistance, src.distCm);
            setParam (p, ParamIDs::srcAngle, src.angle);
        }
        const int mic = p.getLibrary().indexOf (micId);
        setParam (p, ParamIDs::mic, (float) mic);
        setParam (p, ParamIDs::distance, distCm);
        setParam (p, ParamIDs::angle, angle);
        setParam (p, ParamIDs::autoGain, 0.0f);
        setParam (p, ParamIDs::output, 0.0f);

        const int block = 512;
        p.setPlayConfigDetails (2, 2, fs, block);
        p.prepareToPlay (fs, block);

        juce::AudioBuffer<float> buf (2, block);
        juce::MidiBuffer midi;
        // Let the design thread deliver the IR and the convolution engine finish its crossfade (~2 s).
        for (int i = 0; i < (int) (2.0 * fs / block); ++i)
        {
            buf.clear();
            p.processBlock (buf, midi);
            if (i % 8 == 0) std::this_thread::sleep_for (std::chrono::milliseconds (5));
        }

        std::vector<float> out;
        const int total = (int) (0.2 * fs);
        for (int done = 0; done < total; done += block)
        {
            buf.clear();
            if (done == 0) { buf.setSample (0, 0, impulse); buf.setSample (1, 0, impulse); }
            p.processBlock (buf, midi);
            out.insert (out.end(), buf.getReadPointer (0), buf.getReadPointer (0) + block);
        }
        p.releaseResources();
        for (auto& v : out) v /= impulse;
        return out;
    }

    /** Steady 1 kHz sine through the processor; returns 1 s of the left output after 0.6 s of settling. */
    std::vector<float> sineResponse (double fs, const char* micId, float levelDbSpl, float amplitude)
    {
        MoonMicProcessor p;
        setParam (p, ParamIDs::mic, (float) p.getLibrary().indexOf (micId));
        setParam (p, ParamIDs::distance, 50.0f);
        setParam (p, ParamIDs::angle, 0.0f);
        setParam (p, ParamIDs::autoGain, 0.0f);
        setParam (p, ParamIDs::output, 0.0f);
        setParam (p, ParamIDs::level, levelDbSpl);
        const int block = 480;
        p.setPlayConfigDetails (2, 2, fs, block);
        p.prepareToPlay (fs, block);
        juce::AudioBuffer<float> buf (2, block);
        juce::MidiBuffer midi;
        for (int i = 0; i < (int) (2.0 * fs / block); ++i)   // IR delivery + crossfade
        {
            buf.clear();
            p.processBlock (buf, midi);
            if (i % 8 == 0) std::this_thread::sleep_for (std::chrono::milliseconds (5));
        }
        std::vector<float> out;
        long long n = 0;
        const long long settle = (long long) (0.6 * fs), total = settle + (long long) fs;
        while (n < total)
        {
            for (int i = 0; i < block; ++i)
            {
                const float s = amplitude * (float) std::sin (2.0 * juce::MathConstants<double>::pi * 1000.0 * (double) (n + i) / fs);
                buf.setSample (0, i, s); buf.setSample (1, i, s);
            }
            p.processBlock (buf, midi);
            for (int i = 0; i < block; ++i)
                if (n + i >= settle && n + i < total) out.push_back (buf.getSample (0, i));
            n += block;
        }
        p.releaseResources();
        return out;
    }

    /** THD of a 1 kHz sine from exactly 1 s of samples (integer cycles, rectangular DFT bins). */
    double thd1k (const std::vector<float>& y, double fs)
    {
        auto amp = [&] (double f)
        {
            std::complex<double> acc;
            for (size_t i = 0; i < y.size(); ++i)
                acc += (double) y[i] * std::polar (1.0, -2.0 * juce::MathConstants<double>::pi * f * (double) i / fs);
            return std::abs (acc);
        };
        const double h1 = amp (1000.0);
        double hs = 0.0;
        for (int h = 2; h <= 9; ++h) hs += std::pow (amp (1000.0 * h), 2.0);
        return std::sqrt (hs) / h1;
    }
}

/** Renders the editor to a PNG (visual check of the UI without a host). */
static void snapshotEditor (const juce::File& png, float distCm, bool transform = false)
{
    MoonMicProcessor p;
    setParam (p, ParamIDs::distance, distCm);
    if (transform)
    {
        setParam (p, ParamIDs::transform, 1.0f);
        setParam (p, ParamIDs::srcMic, (float) p.getLibrary().indexOf ("sm58"));
        setParam (p, ParamIDs::mic, (float) p.getLibrary().indexOf ("c800_cardioid"));
    }
    p.setPlayConfigDetails (2, 2, 48000.0, 512);
    p.prepareToPlay (48000.0, 512);
    std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
    juce::MessageManager::getInstance()->runDispatchLoopUntil (600);  // let the editor timer pick up the curve
    const auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 2.0f);
    png.deleteFile();
    juce::FileOutputStream os (png);
    juce::PNGImageFormat().writeImageToStream (img, os);
    std::printf ("snapshot written: %s\n", png.getFullPathName().toRawUTF8());
    ed.reset();
    p.releaseResources();
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI juceInit;   // message manager for the processor's internals

    if (argc > 1)
    {
        snapshotEditor (juce::File (juce::String (argv[1]) + "_50cm.png"), 50.0f);
        snapshotEditor (juce::File (juce::String (argv[1]) + "_15cm.png"), 15.0f);
        snapshotEditor (juce::File (juce::String (argv[1]) + "_transform.png"), 15.0f, true);
        return 0;
    }

    ModelLibrary lib;
    std::string err;
    check (lib.loadEmbedded (err), "model data loads");

    struct Case { const char* id; float distCm, angle; };
    for (double fs : { 44100.0, 48000.0, 96000.0, 192000.0 })
        for (auto cs : { Case { "u87ai_cardioid", 50.0f, 0.0f }, Case { "r121", 15.0f, 20.0f } })
        {
            const auto h = impulseResponse (fs, cs.id, cs.distCm, cs.angle);
            const auto target = SignatureDesigner::targetMagnitudeDb (lib, lib.indexOf (cs.id), cs.angle,
                                                                      cs.distCm / 100.0f);
            const auto& f = lib.frequencies();
            double worst = 0.0, worstF = 0.0;
            for (size_t k = 0; k < f.size(); k += 6)
            {
                if (f[k] < 70.0f || f[k] > 18000.0f) continue;
                const double e = std::abs (magDbAt (h, f[k], fs) - target[k]);
                if (e > worst) { worst = e; worstF = f[k]; }
            }
            check (worst < 0.5, juce::String (cs.id) + " @ " + juce::String ((int) fs) + " Hz: audio path vs target, "
                                    "max error " + juce::String (worst, 3) + " dB at " + juce::String (worstF, 0) + " Hz");
        }

    // ---- physical electronics model (C-800G tube stage) ----------------------------------------------
    // 1. Linear at low level: same audio-path-vs-target check, Level 94 dB SPL, impulse 0.01.
    for (double fs : { 48000.0, 96000.0 })
        for (const char* id : { "c800_cardioid", "c800_omni" })
        {
            const auto h = impulseResponse (fs, id, 50.0f, 0.0f, 94.0f, 0.01f);
            const auto target = SignatureDesigner::targetMagnitudeDb (lib, lib.indexOf (id), 0.0f, 0.5f);
            const auto& f = lib.frequencies();
            double worst = 0.0, worstF = 0.0;
            for (size_t k = 0; k < f.size(); k += 6)
            {
                if (f[k] < 70.0f || f[k] > 18000.0f) continue;
                const double e = std::abs (magDbAt (h, f[k], fs) - target[k]);
                if (e > worst) { worst = e; worstF = f[k]; }
            }
            check (worst < 0.5, juce::String (id) + " @ " + juce::String ((int) fs) + " Hz, tube stage at low level: "
                                    "audio path vs target, max error " + juce::String (worst, 3) + " dB at "
                                    + juce::String (worstF, 0) + " Hz");
        }

    // 2. Distortion vs the Python reference (research/reports/stage3_maxspl.json, R_L 100 kOhm, grid 0 V):
    //    full-scale 1 kHz sine at Level 114 / 131 dB SPL -> THD 0.258 % / 1.816 %.
    for (auto [lvl, ref] : { std::pair { 114.0f, 0.00258 }, std::pair { 131.0f, 0.01816 } })
    {
        const double d = thd1k (sineResponse (48000.0, "c800_cardioid", lvl, 1.0f), 48000.0);
        check (std::abs (d / ref - 1.0) < 0.10, "c800_cardioid THD at Level " + juce::String (lvl, 0) + " dB SPL: "
                                                + juce::String (d * 100.0, 3) + " % (Python " + juce::String (ref * 100.0, 3) + " %)");
    }

    // ---- TRANSFORM v1 ------------------------------------------------------------------------------------
    // 4. Audio path == designed correction (sm58 @ 10 cm -> u87ai_cardioid @ 10 cm; k2_omni @ 1.25 m -> r121 @ 25 cm, 30 deg)
    struct TfCase { const char* src; float srcCm, srcAng; const char* tgt; float tgtCm, tgtAng; };
    for (auto tc : { TfCase { "sm58", 10.0f, 0.0f, "u87ai_cardioid", 10.0f, 0.0f },
                     TfCase { "k2_omni", 125.0f, 0.0f, "r121", 25.0f, 30.0f } })
        for (double fs : { 48000.0, 96000.0 })
        {
            const auto h = impulseResponse (fs, tc.tgt, tc.tgtCm, tc.tgtAng, 120.0f, 1.0f, Source { tc.src, tc.srcCm, tc.srcAng });
            const auto corr = SignatureDesigner::transformCorrectionDb (lib, lib.indexOf (tc.src), tc.srcAng, tc.srcCm / 100.0f,
                                                                        lib.indexOf (tc.tgt), tc.tgtAng, tc.tgtCm / 100.0f);
            const auto& f = lib.frequencies();
            double worst = 0.0, worstF = 0.0;
            for (size_t k = 0; k < f.size(); k += 6)
            {
                if (f[k] < 70.0f || f[k] > 18000.0f) continue;
                const double e = std::abs (magDbAt (h, f[k], fs) - corr[k]);
                if (e > worst) { worst = e; worstF = f[k]; }
            }
            check (worst < 0.5, juce::String ("TRANSFORM ") + tc.src + " -> " + tc.tgt + " @ " + juce::String ((int) fs)
                                    + " Hz: audio path vs correction, max error " + juce::String (worst, 3) + " dB at "
                                    + juce::String (worstF, 0) + " Hz");
        }

    // 5. Null test (05 §7): source == target at the same position -> output == input
    {
        const auto h = impulseResponse (48000.0, "u87ai_cardioid", 30.0f, 10.0f, 120.0f, 1.0f, Source { "u87ai_cardioid", 30.0f, 10.0f });
        double worst = 0.0;
        for (double fq = 30.0; fq < 20000.0; fq *= 1.05)
            worst = std::max (worst, std::abs (magDbAt (h, fq, 48000.0)));
        check (worst < 0.05, "TRANSFORM null test (u87ai_cardioid -> itself, 30 cm, 10 deg): |H| within "
                                 + juce::String (worst, 4) + " dB of 0 dB, 30 Hz-20 kHz");
    }

    // 3. Microphones without an electronics model ignore Level (bit-identical output).
    {
        const auto a = sineResponse (48000.0, "u87ai_cardioid", 94.0f, 1.0f);
        const auto b = sineResponse (48000.0, "u87ai_cardioid", 140.0f, 1.0f);
        check (a == b, "u87ai_cardioid: Level has no effect (no electronics model)");
    }

    std::printf ("\n%s (%d failure%s)\n", failures == 0 ? "ALL TESTS PASSED" : "TESTS FAILED",
                 failures, failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}
