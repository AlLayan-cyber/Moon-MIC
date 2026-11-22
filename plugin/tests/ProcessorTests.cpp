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
    std::vector<float> impulseResponse (double fs, const char* micId, float distCm, float angle)
    {
        MoonMicProcessor p;
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
            if (done == 0) { buf.setSample (0, 0, 1.0f); buf.setSample (1, 0, 1.0f); }
            p.processBlock (buf, midi);
            out.insert (out.end(), buf.getReadPointer (0), buf.getReadPointer (0) + block);
        }
        p.releaseResources();
        return out;
    }
}

/** Renders the editor to a PNG (visual check of the UI without a host). */
static void snapshotEditor (const juce::File& png, float distCm)
{
    MoonMicProcessor p;
    setParam (p, ParamIDs::distance, distCm);
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

    std::printf ("\n%s (%d failure%s)\n", failures == 0 ? "ALL TESTS PASSED" : "TESTS FAILED",
                 failures, failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}
