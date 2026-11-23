#include "StageView.h"
#include "Theme.h"

#include <cmath>

namespace moonmic
{

void StageView::setMarker (int index, const Marker& m)
{
    auto& cur = markers[index & 1];
    if (cur.distanceCm == m.distanceCm && cur.angleDeg == m.angleDeg && cur.visible == m.visible
        && cur.hollow == m.hollow && cur.label == m.label && cur.colour == m.colour)
        return;
    cur = m;
    repaint();
}

void StageView::setPolar (std::vector<float> db, juce::String freqLabel)
{
    if (db == polar && freqLabel == polarLabel)
        return;
    polar = std::move (db);
    polarLabel = std::move (freqLabel);
    repaint();
}

juce::Point<float> StageView::centre() const   { return getLocalBounds().toFloat().getCentre(); }
float StageView::radius() const                 { return 0.5f * (float) juce::jmin (getWidth(), getHeight()) - 26.0f; }

// log distance scale; the inner 12 % of the radius is the microphone itself
float StageView::radiusFor (float cm) const
{
    const float t = std::log (juce::jlimit (minCm, maxCm, cm) / minCm) / std::log (maxCm / minCm);
    return radius() * (0.12f + 0.88f * t);
}

float StageView::cmFor (float r) const
{
    const float t = juce::jlimit (0.0f, 1.0f, (r / radius() - 0.12f) / 0.88f);
    return minCm * std::exp (t * std::log (maxCm / minCm));
}

juce::Point<float> StageView::positionOf (const Marker& m) const
{
    const float a = juce::degreesToRadians (m.angleDeg), r = radiusFor (m.distanceCm);
    return centre() + juce::Point<float> (r * std::sin (a), -r * std::cos (a));
}

void StageView::paint (juce::Graphics& g)
{
    const auto c = centre();
    const float R = radius();
    g.setColour (theme::panel);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 8.0f);

    // distance rings (log) and bearing spokes
    g.setFont (juce::FontOptions (10.0f));
    for (float cm : { 10.0f, 20.0f, 30.0f, 50.0f, 80.0f, 125.0f })
    {
        const float r = radiusFor (cm);
        g.setColour (theme::grid);
        g.drawEllipse (c.x - r, c.y - r, 2 * r, 2 * r, cm == 50.0f ? 1.2f : 0.8f);
        g.setColour (theme::muted.withAlpha (0.8f));
        g.drawText (juce::String ((int) cm) + " cm", juce::Rectangle<float> (c.x + 4, c.y - r - 13, 50, 12),
                    juce::Justification::left);
    }
    for (int deg = 0; deg < 360; deg += 30)
    {
        const float a = juce::degreesToRadians ((float) deg);
        const auto dir = juce::Point<float> (std::sin (a), -std::cos (a));
        g.setColour (theme::grid);
        g.drawLine (juce::Line<float> (c + dir * radiusFor (minCm), c + dir * R), 0.6f);
        g.setColour (theme::muted.withAlpha (0.7f));
        const auto p = c + dir * (R + 13.0f);
        g.drawText (juce::String (deg <= 180 ? deg : deg - 360) + juce::String::fromUTF8 ("\xc2\xb0"),
                    juce::Rectangle<float> (p.x - 20, p.y - 6, 40, 12), juce::Justification::centred);
    }

    // model polar pattern at the selected frequency (0 dB = maximum, -30 dB = centre)
    if (polar.size() >= 8)
    {
        const float peak = *std::max_element (polar.begin(), polar.end());
        juce::Path p;
        const int n = (int) polar.size();
        for (int i = 0; i <= n; ++i)
        {
            const float db = juce::jlimit (-30.0f, 0.0f, polar[(size_t) (i % n)] - peak);
            const float r = R * (1.0f + db / 30.0f);
            const float a = juce::degreesToRadians ((float) (i % n) * 360.0f / (float) n);
            const auto q = c + juce::Point<float> (r * std::sin (a), -r * std::cos (a));
            if (i == 0) p.startNewSubPath (q); else p.lineTo (q);
        }
        p.closeSubPath();
        g.setColour (theme::accent.withAlpha (0.07f));
        g.fillPath (p);
        g.setColour (theme::accent.withAlpha (0.45f));
        g.strokePath (p, juce::PathStrokeType (1.4f));
        g.setColour (theme::muted);
        g.drawText (polarLabel, getLocalBounds().reduced (10).removeFromBottom (14), juce::Justification::bottomLeft);
    }

    // microphone (front up)
    const float mr = radius() * 0.07f;
    g.setColour (theme::text.withAlpha (0.9f));
    g.fillEllipse (c.x - mr, c.y - mr, 2 * mr, 2 * mr);
    g.setColour (theme::background);
    g.fillEllipse (c.x - mr * 0.55f, c.y - mr * 0.55f, mr * 1.1f, mr * 1.1f);
    juce::Path front;
    front.addTriangle (c.x - mr * 0.45f, c.y - mr * 1.15f, c.x + mr * 0.45f, c.y - mr * 1.15f, c.x, c.y - mr * 1.7f);
    g.setColour (theme::text.withAlpha (0.9f));
    g.fillPath (front);

    // markers: source first so the target is on top
    for (int i : { 1, 0 })
    {
        const auto& m = markers[i];
        if (! m.visible) continue;
        const auto p = positionOf (m);
        g.setColour (m.colour.withAlpha (0.35f));
        g.drawLine (juce::Line<float> (c, p), 1.0f);
        const float rr = 9.0f;
        if (m.hollow)
        {
            g.setColour (m.colour);
            g.drawEllipse (p.x - rr, p.y - rr, 2 * rr, 2 * rr, 2.2f);
        }
        else
        {
            g.setColour (m.colour.withAlpha (dragging == i ? 1.0f : 0.92f));
            g.fillEllipse (p.x - rr, p.y - rr, 2 * rr, 2 * rr);
        }
        // when both markers are close, the source label goes below the target label
        const bool stacked = i == 1 && markers[0].visible
                             && positionOf (markers[0]).getDistanceFrom (p) < 30.0f;
        g.setColour (i == 0 ? theme::text : m.colour);
        g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        g.drawText (m.label + "  " + juce::String (m.distanceCm, 1) + " cm  " + juce::String (m.angleDeg, 1)
                        + juce::String::fromUTF8 ("\xc2\xb0"),
                    juce::Rectangle<float> (p.x + 13, p.y - 8 + (stacked ? 16.0f : 0.0f), 190, 16),
                    juce::Justification::left);
    }
}

void StageView::mouseDown (const juce::MouseEvent& e)
{
    dragging = -1;
    float best = 22.0f;
    for (int i : { 0, 1 })
        if (markers[i].visible)
        {
            const float d = positionOf (markers[i]).getDistanceFrom (e.position);
            if (d < best) { best = d; dragging = i; }
        }
    if (dragging < 0 && markers[0].visible)
        dragging = 0;                 // click anywhere moves the main singer
    if (dragging >= 0)
    {
        if (onGesture) onGesture (dragging, true);
        moveTo (dragging, e.position);
    }
}

void StageView::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging >= 0)
        moveTo (dragging, e.position);
}

void StageView::mouseUp (const juce::MouseEvent&)
{
    if (dragging >= 0 && onGesture)
        onGesture (dragging, false);
    dragging = -1;
    repaint();
}

void StageView::mouseDoubleClick (const juce::MouseEvent&)
{
    // double click: back to the reference position (50 cm on axis)
    if (onGesture) onGesture (0, true);
    if (onMove) onMove (0, 50.0f, 0.0f);
    if (onGesture) onGesture (0, false);
}

void StageView::moveTo (int index, juce::Point<float> p)
{
    const auto v = p - centre();
    float ang = juce::radiansToDegrees (std::atan2 (v.x, -v.y));
    ang = std::round (ang * 2.0f) / 2.0f;                              // 0.5 deg steps, like the parameter
    const float cm = std::round (cmFor (v.getDistanceFromOrigin()) * 2.0f) / 2.0f;
    if (onMove) onMove (index, cm, ang);
}

} // namespace moonmic
