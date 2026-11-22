#pragma once

#include <juce_graphics/juce_graphics.h>

namespace moonmic::theme
{
    // Dark, restrained palette (docs/architecture/13 §5).
    inline const juce::Colour background { 0xff0e1014 };
    inline const juce::Colour panel      { 0xff161920 };
    inline const juce::Colour grid       { 0xff252a34 };
    inline const juce::Colour text       { 0xffe7e9ef };
    inline const juce::Colour muted      { 0xff8a91a1 };
    inline const juce::Colour accent     { 0xff8fb3ff };
    inline const juce::Colour warning    { 0xffe0b25a };
}
