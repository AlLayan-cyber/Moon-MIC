#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace moonmic
{

namespace
{
    ModelLibrary loadLibrary (bool& ok, juce::String& error)
    {
        ModelLibrary lib;
        std::string err;
        ok = lib.loadEmbedded (err);
        error = juce::String (err);
        return lib;
    }
}

//==============================================================================
class MoonMicProcessor::DesignThread final : public juce::Thread
{
public:
    explicit DesignThread (MoonMicProcessor& p) : juce::Thread ("MOON MIC designer"), owner (p) {}

    void run() override
    {
        while (! threadShouldExit())
        {
            owner.designIfNeeded();
            wait (15);
        }
    }

private:
    MoonMicProcessor& owner;
};

//==============================================================================
MoonMicProcessor::MoonMicProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      library (loadLibrary (libraryOk, libraryError)),
      apvts (*this, nullptr, "MOONMIC_STATE", createLayout (library))
{
    micParam      = apvts.getRawParameterValue (ParamIDs::mic);
    distanceParam = apvts.getRawParameterValue (ParamIDs::distance);
    angleParam    = apvts.getRawParameterValue (ParamIDs::angle);
    autoGainParam = apvts.getRawParameterValue (ParamIDs::autoGain);
    outputParam   = apvts.getRawParameterValue (ParamIDs::output);
    levelParam    = apvts.getRawParameterValue (ParamIDs::level);
    transformParam   = apvts.getRawParameterValue (ParamIDs::transform);
    srcMicParam      = apvts.getRawParameterValue (ParamIDs::srcMic);
    srcDistanceParam = apvts.getRawParameterValue (ParamIDs::srcDistance);
    srcAngleParam    = apvts.getRawParameterValue (ParamIDs::srcAngle);

    for (int i = 0; i < library.numSets(); ++i)
        setElectronics.push_back (ElectronicsStage::modelForSet (library.set (i).id));

    designThread = std::make_unique<DesignThread> (*this);
    if (libraryOk)
        designThread->startThread (juce::Thread::Priority::low);
}

MoonMicProcessor::~MoonMicProcessor()
{
    designThread->stopThread (2000);
}

juce::AudioProcessorValueTreeState::ParameterLayout MoonMicProcessor::createLayout (const ModelLibrary& lib)
{
    juce::StringArray names;
    for (int i = 0; i < lib.numSets(); ++i)
        names.add (juce::String::fromUTF8 (lib.set (i).displayName.c_str()));
    if (names.isEmpty())
        names.add ("(model data missing)");

    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ParamIDs::mic, 1 }, "Microphone", names, 0));

    juce::NormalisableRange<float> distRange (10.0f, 125.0f, 0.5f);
    distRange.setSkewForCentre (40.0f);
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::distance, 1 }, "Distance", distRange, 50.0f,
        juce::AudioParameterFloatAttributes().withLabel ("cm")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::angle, 1 }, "Angle",
        juce::NormalisableRange<float> (-180.0f, 180.0f, 0.5f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel (juce::String::fromUTF8 ("\xc2\xb0"))));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParamIDs::autoGain, 1 }, "Auto Gain", true));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::output, 1 }, "Output",
        juce::NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    // Level calibration for the physical electronics model (tube stage): which SPL at the capsule a
    // full-scale sine corresponds to. Only affects microphones with an electronics model.
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::level, 1 }, "Level",
        juce::NormalisableRange<float> (94.0f, 140.0f, 0.5f), ElectronicsStage::defaultLevelDbSpl,
        juce::AudioParameterFloatAttributes().withLabel ("dB SPL")));

    // TRANSFORM v1: the source microphone and its position are chosen by the user (no AI analysis yet).
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParamIDs::transform, 1 }, "Transform", false));
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ParamIDs::srcMic, 1 }, "Source Microphone", names, 0));
    juce::NormalisableRange<float> srcRange (5.0f, 125.0f, 0.5f);
    srcRange.setSkewForCentre (30.0f);
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::srcDistance, 1 }, "Source Distance", srcRange, 15.0f,
        juce::AudioParameterFloatAttributes().withLabel ("cm")));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::srcAngle, 1 }, "Source Angle",
        juce::NormalisableRange<float> (-180.0f, 180.0f, 0.5f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel (juce::String::fromUTF8 ("\xc2\xb0"))));

    return layout;
}

//==============================================================================
void MoonMicProcessor::designIfNeeded()
{
    const int mic = juce::jlimit (0, library.numSets() - 1, (int) micParam->load());
    const float distM = distanceParam->load() / 100.0f;
    const float ang = angleParam->load();
    const bool tf = transformParam->load() >= 0.5f;
    const int srcMic = juce::jlimit (0, library.numSets() - 1, (int) srcMicParam->load());
    const float srcDistM = srcDistanceParam->load() / 100.0f;
    const float srcAng = srcAngleParam->load();

    const bool srcSame = ! tf || (srcMic == lastSrcMic && std::abs (srcDistM - lastSrcDistance) < 1.0e-4f
                                  && std::abs (srcAng - lastSrcAngle) < 1.0e-3f);
    if (! forceRedesign.exchange (false) && mic == lastMic && tf == lastTransform && srcSame
        && std::abs (distM - lastDistance) < 1.0e-4f && std::abs (ang - lastAngle) < 1.0e-3f)
        return;

    lastMic = mic; lastDistance = distM; lastAngle = ang; lastTransform = tf;
    lastSrcMic = srcMic; lastSrcDistance = srcDistM; lastSrcAngle = srcAng;

    // Designed at the host rate, so the convolution engine never resamples the IR.
    const double rate = hostRate.load();
    auto result = tf ? SignatureDesigner::designTransform (library, srcMic, srcAng, srcDistM, mic, ang, distM, rate)
                     : SignatureDesigner::design (library, mic, ang, distM, rate);

    juce::AudioBuffer<float> ir (1, (int) result.fir.size());
    ir.copyFrom (0, 0, result.fir.data(), (int) result.fir.size());

    {
        const juce::SpinLock::ScopedLockType sl (pendingLock);
        pendingIR = std::move (ir);
        pendingRate = rate;
        pendingAvailable = true;
    }
    designedAutoGainDb = result.autoGainDb;

    const juce::SpinLock::ScopedLockType sl (curveLock);
    displayCurve = std::move (result.targetMagDb);
}

std::vector<float> MoonMicProcessor::getDisplayCurve() const
{
    const juce::SpinLock::ScopedLockType sl (curveLock);
    return displayCurve;
}

//==============================================================================
void MoonMicProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock,
                                        (juce::uint32) juce::jmax (1, getTotalNumOutputChannels()) };
    convolution.prepare (spec);
    convolution.reset();
    gain.reset (sampleRate, 0.05);
    gain.setCurrentAndTargetValue (1.0f);
    electronics.prepare (sampleRate, samplesPerBlock, (int) spec.numChannels);
    prepared = true;
    hostRate = sampleRate;
    forceRedesign = true;   // re-design at the new rate and re-deliver the IR after prepare()
    setLatencySamples (convolution.getLatency());
}

void MoonMicProcessor::releaseResources()
{
    prepared = false;
}

bool MoonMicProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return out == layouts.getMainInputChannelSet();
}

double MoonMicProcessor::getTailLengthSeconds() const
{
    return SignatureDesigner::firSeconds;
}

void MoonMicProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (auto ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    if (! libraryOk || ! prepared)
        return;

    if (pendingAvailable.load())
    {
        const juce::SpinLock::ScopedTryLockType tl (pendingLock);
        if (tl.isLocked() && pendingAvailable.exchange (false))
            convolution.loadImpulseResponse (std::move (pendingIR), pendingRate,
                                             juce::dsp::Convolution::Stereo::no,
                                             juce::dsp::Convolution::Trim::no,
                                             juce::dsp::Convolution::Normalise::no);
    }

    juce::dsp::AudioBlock<float> block (buffer);
    convolution.process (juce::dsp::ProcessContextReplacing<float> (block));

    // physical electronics model after the measured linear response (C-800G tube stage)
    const int micIndex = juce::jlimit (0, (int) setElectronics.size() - 1, (int) micParam->load());
    electronics.setModel (setElectronics.empty() ? ElectronicsStage::Model::none : setElectronics[(size_t) micIndex]);
    electronics.process (block, levelParam->load());

    float targetDb = outputParam->load();
    if (autoGainParam->load() >= 0.5f)
        targetDb += designedAutoGainDb.load();
    gain.setTargetValue (juce::Decibels::decibelsToGain (targetDb));

    const int n = buffer.getNumSamples();
    if (gain.isSmoothing())
    {
        for (int i = 0; i < n; ++i)
        {
            const float g = gain.getNextValue();
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                buffer.getWritePointer (ch)[i] *= g;
        }
    }
    else
    {
        buffer.applyGain (gain.getTargetValue());
    }
}

//==============================================================================
juce::AudioProcessorEditor* MoonMicProcessor::createEditor()
{
    return new MoonMicEditor (*this);
}

void MoonMicProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void MoonMicProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
    forceRedesign = true;
}

} // namespace moonmic

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new moonmic::MoonMicProcessor();
}
