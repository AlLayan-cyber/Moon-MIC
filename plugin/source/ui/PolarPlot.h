#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <vector>

namespace moonmic
{

/** Polar display (docs/architecture/13 §4.1): model response vs angle at several frequencies, overlaid.
    Each curve is normalised to its own maximum; rings at 0 / -10 / -20 / -30 dB. Curves come straight from
    the spatial model at the current distance (SignatureDesigner::polarDb). */
class PolarPlot final : public juce::Component
{
public:
    struct Curve
    {
        std::vector<float> db;      // 72 values, 0..355 deg
        juce::Colour colour;
        juce::String label;
    };

    void setCurves (std::vector<Curve> c);
    void paint (juce::Graphics&) override;

private:
    std::vector<Curve> curves;
};

} // namespace moonmic
