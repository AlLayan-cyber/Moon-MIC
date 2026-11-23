#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <vector>

namespace moonmic
{

/** Stage View (docs/architecture/13 §4.1): top-down view of the microphone and the singer.

    The microphone sits in the centre with its front (0 deg) pointing up. A singer marker is dragged with the
    mouse: its radius sets the distance (log scale, 5-125 cm), its bearing the angle (positive = to the right).
    In TRANSFORM a second, hollow marker shows the position of the source recording.
    The polar pattern drawn around the microphone is the model's response at the selected frequency and
    current distance (nothing is drawn "for looks", 13 §6). */
class StageView final : public juce::Component
{
public:
    struct Marker
    {
        float distanceCm = 50.0f, angleDeg = 0.0f;
        bool visible = false, hollow = false;
        juce::Colour colour;
        juce::String label;
    };

    static constexpr float minCm = 5.0f, maxCm = 125.0f;

    void setMarker (int index, const Marker& m);           // 0 = target / SIGNATURE singer, 1 = source
    void setPolar (std::vector<float> dbAt5DegSteps, juce::String freqLabel);

    /** Called while dragging marker `index`; begin/end bracket a drag for host automation gestures. */
    std::function<void (int index, float distanceCm, float angleDeg)> onMove;
    std::function<void (int index, bool begin)> onGesture;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    juce::Point<float> centre() const;
    float radius() const;
    float radiusFor (float cm) const;
    float cmFor (float r) const;
    juce::Point<float> positionOf (const Marker&) const;
    void moveTo (int index, juce::Point<float>);

    Marker markers[2];
    std::vector<float> polar;
    juce::String polarLabel;
    int dragging = -1;
};

} // namespace moonmic
