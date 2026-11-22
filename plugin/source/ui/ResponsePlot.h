#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <vector>

namespace moonmic
{

/** Frequency-response display (log frequency 20 Hz - 20 kHz, dB). Draws exactly the curve the
    designer produced; nothing is drawn "for looks" (docs/architecture/13 §6). */
class ResponsePlot final : public juce::Component
{
public:
    void setGrid (std::vector<float> frequencies);
    void setCurve (std::vector<float> magDb);
    void setLfHoldBelow (float hz)  { lfHoldHz = hz; repaint(); }
    void paint (juce::Graphics&) override;

private:
    float xFor (float hz) const;
    float yFor (float db) const;

    std::vector<float> freqs, curve;
    float lfHoldHz = 0.0f;
    static constexpr float fMin = 20.0f, fMax = 20000.0f, dbRange = 18.0f;
};

} // namespace moonmic
