#pragma once

#include "model/ModelLibrary.h"
#include "dsp/SignatureDesigner.h"
#include "dsp/ElectronicsStage.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include <atomic>

namespace moonmic
{

namespace ParamIDs
{
    inline constexpr auto mic      = "mic";
    inline constexpr auto distance = "distance"; // cm
    inline constexpr auto angle    = "angle";    // degrees, horizontal plane
    inline constexpr auto autoGain = "autoGain";
    inline constexpr auto output   = "output";   // dB
    inline constexpr auto level    = "level";    // dB SPL at the capsule for a full-scale sine
    inline constexpr auto transform   = "transform";   // false: SIGNATURE, true: TRANSFORM (v1, manual source)
    inline constexpr auto srcMic      = "srcMic";      // TRANSFORM source microphone
    inline constexpr auto srcDistance = "srcDistance"; // cm
    inline constexpr auto srcAngle    = "srcAngle";    // degrees
}

/** MOON MIC — SIGNATURE mode (deterministic, no AI; decision D-001) and TRANSFORM v1 (manual source,
    no AI analysis yet; docs/architecture/05).
    Audio thread: zero-latency convolution with a minimum-phase FIR + smoothed gains.
    Filter design runs on a background thread whenever mic/angle/distance change. */
class MoonMicProcessor final : public juce::AudioProcessor
{
public:
    MoonMicProcessor();
    ~MoonMicProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override                     { return true; }

    const juce::String getName() const override         { return "MOON MIC"; }
    bool acceptsMidi() const override                   { return false; }
    bool producesMidi() const override                  { return false; }
    bool isMidiEffect() const override                  { return false; }
    double getTailLengthSeconds() const override;

    int getNumPrograms() override                       { return 1; }
    int getCurrentProgram() override                    { return 0; }
    void setCurrentProgram (int) override               {}
    const juce::String getProgramName (int) override    { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getState() noexcept     { return apvts; }
    const ModelLibrary& getLibrary() const noexcept             { return library; }
    bool isLibraryValid() const noexcept                        { return libraryOk; }
    const juce::String& getLibraryError() const noexcept        { return libraryError; }

    /** True if the set has a physical electronics model (the Level control applies). */
    bool hasElectronics (int setIndex) const noexcept
    {
        return setIndex >= 0 && setIndex < (int) setElectronics.size()
               && setElectronics[(size_t) setIndex] != ElectronicsStage::Model::none;
    }

    /** Latest designed target magnitude (model grid) for the UI. Not for the audio thread. */
    std::vector<float> getDisplayCurve() const;

private:
    class DesignThread;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout (const ModelLibrary&);
    void designIfNeeded();   // background thread only

    // Order matters: libraryOk/libraryError are written while 'library' is constructed.
    bool libraryOk = false;
    juce::String libraryError;
    ModelLibrary library;

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float>* micParam = nullptr;
    std::atomic<float>* distanceParam = nullptr;
    std::atomic<float>* angleParam = nullptr;
    std::atomic<float>* autoGainParam = nullptr;
    std::atomic<float>* outputParam = nullptr;
    std::atomic<float>* levelParam = nullptr;
    std::atomic<float>* transformParam = nullptr;
    std::atomic<float>* srcMicParam = nullptr;
    std::atomic<float>* srcDistanceParam = nullptr;
    std::atomic<float>* srcAngleParam = nullptr;

    juce::dsp::Convolution convolution;   // zero latency, uniform partitioned
    juce::SmoothedValue<float> gain;
    ElectronicsStage electronics;
    std::vector<ElectronicsStage::Model> setElectronics;   // per model set, fixed after construction
    bool prepared = false;

    // Design thread -> audio thread hand-over (single slot).
    juce::SpinLock pendingLock;
    juce::AudioBuffer<float> pendingIR;
    double pendingRate = SignatureDesigner::designSampleRate;   // guarded by pendingLock
    std::atomic<bool> pendingAvailable { false };
    std::atomic<double> hostRate { SignatureDesigner::designSampleRate };
    std::atomic<float> designedAutoGainDb { 0.0f };

    // Design thread -> UI.
    mutable juce::SpinLock curveLock;
    std::vector<float> displayCurve;

    // Last designed parameters (design thread only).
    int lastMic = -1, lastSrcMic = -1;
    bool lastTransform = false;
    float lastDistance = -1.0f, lastAngle = 1.0e9f, lastSrcDistance = -1.0f, lastSrcAngle = 1.0e9f;
    std::atomic<bool> forceRedesign { true };

    std::unique_ptr<DesignThread> designThread;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MoonMicProcessor)
};

} // namespace moonmic
