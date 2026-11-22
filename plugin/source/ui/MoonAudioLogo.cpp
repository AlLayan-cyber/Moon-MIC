#include "MoonAudioLogo.h"

namespace moonmic
{

MoonAudioLogo::MoonAudioLogo (juce::Colour discColour) : colour (discColour)
{
    setInterceptsMouseClicks (false, false);
}

juce::Path MoonAudioLogo::createPath()
{
    juce::Path p;
    p.addEllipse (0.0f, 0.0f, 106.0f, 106.0f);
    p.addRoundedRectangle (29.5f, 25.5f, 10.0f, 55.0f, 5.0f);
    p.addRoundedRectangle (48.0f, 38.5f, 10.0f, 29.0f, 5.0f);
    p.addRoundedRectangle (66.5f, 11.5f, 10.0f, 83.0f, 5.0f);
    p.setUsingNonZeroWinding (false);   // even-odd: the bars become holes
    return p;
}

void MoonAudioLogo::paint (juce::Graphics& g)
{
    static const juce::Path path = createPath();
    const auto area = getLocalBounds().toFloat();
    const float side = juce::jmin (area.getWidth(), area.getHeight());
    g.setColour (colour);
    g.fillPath (path, path.getTransformToScaleToFit (area.withSizeKeepingCentre (side, side), true));
}

} // namespace moonmic
