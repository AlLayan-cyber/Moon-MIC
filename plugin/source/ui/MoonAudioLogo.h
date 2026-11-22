#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace moonmic
{

/** MOON AUDIO logo drawn as a vector path (same geometry as
    plugin/resources/branding/moon_audio_logo.svg): a disc with three rounded bars cut out. */
class MoonAudioLogo final : public juce::Component
{
public:
    explicit MoonAudioLogo (juce::Colour discColour = juce::Colours::white);
    void setDiscColour (juce::Colour c) { colour = c; repaint(); }
    void paint (juce::Graphics& g) override;

    /** Logo path in a 106 x 106 unit box. */
    static juce::Path createPath();

private:
    juce::Colour colour;
};

} // namespace moonmic
