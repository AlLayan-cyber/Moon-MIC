#pragma once

#include "CapsuleEngine.h"

#include <juce_dsp/juce_dsp.h>

#include <memory>
#include <vector>

namespace moonmic
{

/** Physical model of a microphone's electronics, placed after the measured linear response (SIGNATURE).

    Only the non-linear behaviour is added: the linear response of the measured model already contains the
    microphone's electronics, so the stage output is divided by its own small-signal gain. At low levels the
    block is transparent (unity gain, the coupling/cathode corners lie below 1 Hz).

    Level calibration: a full-scale sine (peak 1.0) after the linear model corresponds to `levelDbSpl` dB SPL
    (RMS) at the capsule. Grid drive = pressure x capsule sensitivity at the grid, derived from the published
    output sensitivity and the stage gain (research/reports/PHYSICAL_CAPSULE_ENGINE_STAGE3.md).

    Models: Sony C-800G (6AU6A common cathode, MA-55 board). Other microphones: no electronics model yet,
    the block is bypassed. Oversampling: 2x polyphase IIR (2x verified as good as 4x for aliasing, Stage 5). */
class ElectronicsStage
{
public:
    enum class Model { none, c800gUni, c800gOmni };

    static Model modelForSet (const std::string& setId) noexcept;

    void prepare (double sampleRate, int maxBlockSize, int numChannels);
    void reset() noexcept;

    /** Audio thread. Switching model resets the circuit state to its operating point. */
    void setModel (Model m) noexcept;
    Model getModel() const noexcept { return model; }

    void process (juce::dsp::AudioBlock<float>& block, float levelDbSpl) noexcept;

    /** Oversampling filter latency in samples (fractional, at the host rate). Not reported to the host:
        the IIR half-band filters are minimum-phase-like, see the report. */
    float getLatencySamples() const noexcept;

    static constexpr float defaultLevelDbSpl = 120.0f;

private:
    void configure() noexcept;

    Model model = Model::none;
    double hostRate = 48000.0;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
    std::vector<capsule::CommonCathodeStage> stages;
    double gridPerPa = 0.0;       // V at the grid per Pa at the capsule
    double stageGain = 1.0;       // signed small-signal gain grid -> secondary
    juce::SmoothedValue<double> pressurePerUnit;   // Pa per digital full-scale unit (peak)
};

} // namespace moonmic
