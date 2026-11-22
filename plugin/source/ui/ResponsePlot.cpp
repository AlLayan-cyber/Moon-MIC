#include "ResponsePlot.h"
#include "Theme.h"

#include <cmath>

namespace moonmic
{

void ResponsePlot::setGrid (std::vector<float> frequencies) { freqs = std::move (frequencies); repaint(); }
void ResponsePlot::setCurve (std::vector<float> magDb)      { curve = std::move (magDb); repaint(); }

float ResponsePlot::xFor (float hz) const
{
    const auto w = (float) getWidth();
    return w * std::log (hz / fMin) / std::log (fMax / fMin);
}

float ResponsePlot::yFor (float db) const
{
    const auto h = (float) getHeight();
    return h * 0.5f - (db / dbRange) * h * 0.5f;
}

void ResponsePlot::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    g.setColour (theme::panel);
    g.fillRoundedRectangle (bounds, 6.0f);

    g.setFont (juce::FontOptions (11.0f));
    for (float f : { 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f })
    {
        const float x = xFor (f);
        g.setColour (theme::grid);
        g.drawVerticalLine ((int) x, 0.0f, bounds.getHeight());
        g.setColour (theme::muted);
        const auto label = f >= 1000.0f ? juce::String ((int) (f / 1000.0f)) + "k" : juce::String ((int) f);
        g.drawText (label, juce::Rectangle<float> (x + 3.0f, bounds.getBottom() - 16.0f, 40.0f, 14.0f),
                    juce::Justification::left);
    }
    for (float db = -12.0f; db <= 12.0f; db += 6.0f)
    {
        const float y = yFor (db);
        g.setColour (db == 0.0f ? theme::grid.brighter (0.4f) : theme::grid);
        g.drawHorizontalLine ((int) y, 0.0f, bounds.getWidth());
        g.setColour (theme::muted);
        g.drawText (juce::String ((int) db) + " dB", juce::Rectangle<float> (4.0f, y - 14.0f, 50.0f, 12.0f),
                    juce::Justification::left);
    }

    if (freqs.size() == curve.size() && freqs.size() > 1)
    {
        juce::Path p;
        bool started = false;
        for (size_t i = 0; i < freqs.size(); ++i)
        {
            if (freqs[i] < fMin || freqs[i] > fMax) continue;
            const float db = juce::jlimit (-dbRange, dbRange, curve[i]);
            const juce::Point<float> pt (xFor (freqs[i]), yFor (db));
            if (! started) { p.startNewSubPath (pt); started = true; }
            else           p.lineTo (pt);
        }
        g.setColour (theme::accent);
        g.strokePath (p, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved));
    }

    g.setColour (theme::grid.brighter (0.2f));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 6.0f, 1.0f);
}

} // namespace moonmic
