#pragma once

#include "PluginProcessor.h"
#include "ui/MoonAudioLogo.h"
#include "ui/ResponsePlot.h"

#include <juce_audio_processors/juce_audio_processors.h>

namespace moonmic
{

class MoonLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    MoonLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
};

/** Stage 2 skeleton UI for SIGNATURE. The final design (Stage View, polar display) follows
    docs/architecture/13; this editor only exposes the verified controls. */
class MoonMicEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit MoonMicEditor (MoonMicProcessor&);
    ~MoonMicEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void setupKnob (juce::Slider&, juce::Label&, const juce::String& name);

    MoonMicProcessor& processor;
    MoonLookAndFeel lnf;

    juce::ComboBox micBox;
    juce::Slider distanceKnob, angleKnob, outputKnob;
    juce::Label distanceLabel, angleLabel, outputLabel, micLabel, statusLabel;
    juce::ToggleButton autoGainButton { "Auto gain" };
    ResponsePlot plot;
    MoonAudioLogo logo;

    using APVTS = juce::AudioProcessorValueTreeState;
    std::unique_ptr<APVTS::ComboBoxAttachment> micAttachment;
    std::unique_ptr<APVTS::SliderAttachment> distanceAttachment, angleAttachment, outputAttachment;
    std::unique_ptr<APVTS::ButtonAttachment> autoGainAttachment;

    std::vector<float> lastCurve;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MoonMicEditor)
};

} // namespace moonmic
