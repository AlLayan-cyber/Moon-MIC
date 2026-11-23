#include "PolarPlot.h"
#include "Theme.h"

#include <cmath>

namespace moonmic
{

void PolarPlot::setCurves (std::vector<Curve> c)
{
    bool same = c.size() == curves.size();
    for (size_t i = 0; same && i < c.size(); ++i)
        same = c[i].db == curves[i].db && c[i].label == curves[i].label;
    if (same)
        return;
    curves = std::move (c);
    repaint();
}

void PolarPlot::paint (juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();
    g.setColour (theme::panel);
    g.fillRoundedRectangle (area, 8.0f);

    auto legend = area.removeFromBottom (22.0f).reduced (10.0f, 2.0f);
    const auto c = area.getCentre();
    const float R = 0.5f * juce::jmin (area.getWidth(), area.getHeight()) - 16.0f;

    g.setFont (juce::FontOptions (9.5f));
    for (int ring = 0; ring <= 3; ++ring)
    {
        const float r = R * (1.0f - (float) ring / 3.0f);
        if (r <= 0.0f) continue;
        g.setColour (theme::grid);
        g.drawEllipse (c.x - r, c.y - r, 2 * r, 2 * r, ring == 0 ? 1.2f : 0.8f);
        g.setColour (theme::muted.withAlpha (0.8f));
        g.drawText (juce::String (-10 * ring) + " dB", juce::Rectangle<float> (c.x + 3, c.y - r, 44, 11),
                    juce::Justification::left);
    }
    for (int deg = 0; deg < 360; deg += 30)
    {
        const float a = juce::degreesToRadians ((float) deg);
        g.setColour (theme::grid);
        g.drawLine (juce::Line<float> (c, c + juce::Point<float> (R * std::sin (a), -R * std::cos (a))), 0.6f);
    }

    for (const auto& cv : curves)
    {
        if (cv.db.size() < 8) continue;
        const float peak = *std::max_element (cv.db.begin(), cv.db.end());
        const int n = (int) cv.db.size();
        juce::Path p;
        for (int i = 0; i <= n; ++i)
        {
            const float db = juce::jlimit (-30.0f, 0.0f, cv.db[(size_t) (i % n)] - peak);
            const float r = R * (1.0f + db / 30.0f);
            const float a = juce::degreesToRadians ((float) (i % n) * 360.0f / (float) n);
            const auto q = c + juce::Point<float> (r * std::sin (a), -r * std::cos (a));
            if (i == 0) p.startNewSubPath (q); else p.lineTo (q);
        }
        g.setColour (cv.colour);
        g.strokePath (p, juce::PathStrokeType (1.6f));
    }

    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    const float w = curves.empty() ? 0.0f : legend.getWidth() / (float) curves.size();
    for (const auto& cv : curves)
    {
        auto cell = legend.removeFromLeft (w);
        g.setColour (cv.colour);
        g.fillRoundedRectangle (cell.removeFromLeft (12.0f).withSizeKeepingCentre (10.0f, 3.0f), 1.5f);
        g.setColour (theme::muted);
        g.drawText (cv.label, cell.reduced (4.0f, 0.0f), juce::Justification::centredLeft);
    }
}

} // namespace moonmic
