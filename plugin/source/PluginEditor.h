#pragma once

#include "PluginProcessor.h"
#include "ui/MoonAudioLogo.h"
#include "ui/PolarPlot.h"
#include "ui/ResponsePlot.h"
#include "ui/StageView.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>
#include <functional>
#include <memory>
#include <vector>

namespace moonmic
{

class MoonLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    MoonLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
};

/** Microphone model + polar-pattern buttons (only the patterns that exist for that model). */
class MicSelector final : public juce::Component
{
public:
    MicSelector (const ModelLibrary&, juce::String title);
    void setSelectedSet (int setIndex);
    void setTitle (const juce::String&);
    std::function<void (int setIndex)> onSelect;
    void resized() override;

private:
    struct Group { juce::String model; std::vector<std::pair<juce::String, int>> patterns; };
    void showGroup (int g, int setIndex);
    std::vector<Group> groups;
    juce::Label title;
    juce::ComboBox modelBox;
    std::array<juce::TextButton, 4> patternButtons;
    int currentSet = -1;
};

/** SIGNATURE / TRANSFORM editor (docs/architecture/13): Stage View, polar display, response, A/B, bypass. */
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
    void setParam (const char* id, float plainValue);
    void gesture (const char* id, bool begin);
    float param (const char* id) const;
    bool isTransform() const { return param (ParamIDs::transform) >= 0.5f; }
    void updatePolar();

    MoonMicProcessor& processor;
    MoonLookAndFeel lnf;

    juce::TextButton signatureButton { "SIGNATURE" }, transformButton { "TRANSFORM" };
    juce::TextButton slotA { "A" }, slotB { "B" }, copyButton, bypassButton { "BYPASS" };
    MicSelector sourceSelector, targetSelector;

    juce::Slider distanceKnob, angleKnob, levelKnob, srcDistanceKnob, srcAngleKnob, outputKnob;
    juce::Label distanceLabel, angleLabel, levelLabel, srcDistanceLabel, srcAngleLabel, outputLabel, statusLabel;
    juce::ToggleButton autoGainButton { "Auto gain" };

    StageView stage;
    PolarPlot polarPlot;
    std::array<juce::TextButton, 8> freqButtons;
    std::array<bool, 8> freqOn { false, true, false, true, false, true, true, false };
    ResponsePlot plot;
    MoonAudioLogo logo;

    using APVTS = juce::AudioProcessorValueTreeState;
    std::unique_ptr<APVTS::SliderAttachment> distanceAttachment, angleAttachment, levelAttachment,
                                             srcDistanceAttachment, srcAngleAttachment, outputAttachment;
    std::unique_ptr<APVTS::ButtonAttachment> autoGainAttachment, bypassAttachment;

    bool shownTransform = false;
    std::vector<float> lastCurve;
    int polarSet = -1;
    float polarDist = -1.0f;
    std::array<bool, 8> polarFreqs {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MoonMicEditor)
};

} // namespace moonmic
