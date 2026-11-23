#include "ElectronicsStage.h"

namespace moonmic
{

namespace
{
    // Sony sell sheet A-1164-A p.3 (VERIFIED): sensitivity at 1 kHz, Uni -32 dB re 1 V/Pa, Omni -35 dB re 1 V/Pa.
    constexpr double c800gUniSensitivity  = 0.025118864315095794;   // 10^(-32/20)
    constexpr double c800gOmniSensitivity = 0.017782794100389229;   // 10^(-35/20)
}

ElectronicsStage::Model ElectronicsStage::modelForSet (const std::string& setId) noexcept
{
    if (setId == "c800_cardioid") return Model::c800gUni;
    if (setId == "c800_omni")     return Model::c800gOmni;
    return Model::none;
}

void ElectronicsStage::prepare (double sampleRate, int maxBlockSize, int numChannels)
{
    hostRate = sampleRate;
    oversampler = std::make_unique<juce::dsp::Oversampling<float>> (
        (size_t) juce::jmax (1, numChannels), 1,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false);
    oversampler->initProcessing ((size_t) juce::jmax (1, maxBlockSize));
    stages.assign ((size_t) juce::jmax (1, numChannels), {});
    pressurePerUnit.reset (sampleRate * 2.0, 0.05);
    pressurePerUnit.setCurrentAndTargetValue (std::sqrt (2.0) * std::pow (10.0, (defaultLevelDbSpl - 94.0) / 20.0));
    configure();
    reset();
}

void ElectronicsStage::configure() noexcept
{
    const auto circuit = capsule::StageCircuit {};       // R_L 100 kOhm (Stage 3 default)
    for (auto& s : stages)
        s.prepare (capsule::tube6AU6A(), circuit, hostRate * 2.0);
    stageGain = stages.empty() ? 1.0 : stages.front().smallSignalGain();
    const double sens = model == Model::c800gOmni ? c800gOmniSensitivity : c800gUniSensitivity;
    gridPerPa = sens / std::abs (stageGain);
}

void ElectronicsStage::reset() noexcept
{
    if (oversampler != nullptr)
        oversampler->reset();
    for (auto& s : stages)
        s.reset();
}

void ElectronicsStage::setModel (Model m) noexcept
{
    if (m == model)
        return;
    model = m;
    if (model != Model::none)
    {
        configure();   // bounded: one DC operating-point solve per channel
        reset();
    }
}

float ElectronicsStage::getLatencySamples() const noexcept
{
    return oversampler != nullptr ? oversampler->getLatencyInSamples() : 0.0f;
}

void ElectronicsStage::process (juce::dsp::AudioBlock<float>& block, float levelDbSpl) noexcept
{
    if (model == Model::none || oversampler == nullptr)
        return;

    // full-scale sine (peak 1) = levelDbSpl RMS -> peak pressure sqrt(2) * 10^((L - 94) / 20) Pa
    pressurePerUnit.setTargetValue (std::sqrt (2.0) * std::pow (10.0, (levelDbSpl - 94.0) / 20.0));

    auto up = oversampler->processSamplesUp (block);
    const size_t numCh = juce::jmin (up.getNumChannels(), stages.size());
    const size_t n = up.getNumSamples();
    for (size_t i = 0; i < n; ++i)
    {
        const double ppu = pressurePerUnit.getNextValue();
        const double toGrid = ppu * gridPerPa;
        const double fromSecondary = 1.0 / (toGrid * stageGain);   // small-signal unity, sign restored
        for (size_t ch = 0; ch < numCh; ++ch)
        {
            float* d = up.getChannelPointer (ch);
            d[i] = (float) (stages[ch].processSample ((double) d[i] * toGrid) * fromSecondary);
        }
    }
    oversampler->processSamplesDown (block);
}

} // namespace moonmic
