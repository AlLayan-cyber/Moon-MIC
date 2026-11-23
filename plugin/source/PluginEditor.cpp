#include "PluginEditor.h"
#include "ui/Theme.h"

namespace moonmic
{

namespace
{
    constexpr int headerH = 58, footerH = 30, margin = 18;
}

//==============================================================================
MoonLookAndFeel::MoonLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, theme::background);
    setColour (juce::ComboBox::backgroundColourId, theme::panel);
    setColour (juce::ComboBox::outlineColourId, theme::grid.brighter (0.2f));
    setColour (juce::ComboBox::textColourId, theme::text);
    setColour (juce::ComboBox::arrowColourId, theme::muted);
    setColour (juce::PopupMenu::backgroundColourId, theme::panel);
    setColour (juce::PopupMenu::textColourId, theme::text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, theme::accent.withAlpha (0.25f));
    setColour (juce::PopupMenu::highlightedTextColourId, theme::text);
    setColour (juce::Label::textColourId, theme::text);
    setColour (juce::Slider::textBoxTextColourId, theme::text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::ToggleButton::textColourId, theme::text);
    setColour (juce::ToggleButton::tickColourId, theme::accent);
    setColour (juce::ToggleButton::tickDisabledColourId, theme::muted);
}

void MoonLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                        float startAngle, float endAngle, juce::Slider&)
{
    const auto area = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (6.0f);
    const float radius = juce::jmin (area.getWidth(), area.getHeight()) * 0.5f;
    const auto centre = area.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);
    const float track = 3.0f;

    juce::Path bg;
    bg.addCentredArc (centre.x, centre.y, radius - track, radius - track, 0.0f, startAngle, endAngle, true);
    g.setColour (theme::grid.brighter (0.15f));
    g.strokePath (bg, juce::PathStrokeType (track, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path val;
    val.addCentredArc (centre.x, centre.y, radius - track, radius - track, 0.0f, startAngle, angle, true);
    g.setColour (theme::accent);
    g.strokePath (val, juce::PathStrokeType (track, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const float inner = radius * 0.62f;
    g.setColour (theme::panel.brighter (0.08f));
    g.fillEllipse (juce::Rectangle<float> (inner * 2.0f, inner * 2.0f).withCentre (centre));
    const auto tip = centre.getPointOnCircumference (inner * 0.8f, angle);
    g.setColour (theme::text);
    g.drawLine ({ centre.getPointOnCircumference (inner * 0.3f, angle), tip }, 2.0f);
}

//==============================================================================
MoonMicEditor::MoonMicEditor (MoonMicProcessor& p)
    : AudioProcessorEditor (p), processor (p)
{
    setLookAndFeel (&lnf);

    auto& state = processor.getState();
    if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (state.getParameter (ParamIDs::mic)))
        micBox.addItemList (choice->choices, 1);
    addAndMakeVisible (micBox);
    micAttachment = std::make_unique<APVTS::ComboBoxAttachment> (state, ParamIDs::mic, micBox);

    if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (state.getParameter (ParamIDs::srcMic)))
        srcBox.addItemList (choice->choices, 1);
    addChildComponent (srcBox);
    srcAttachment = std::make_unique<APVTS::ComboBoxAttachment> (state, ParamIDs::srcMic, srcBox);
    srcLabel.setText ("SOURCE (RECORDED WITH)", juce::dontSendNotification);
    srcLabel.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    srcLabel.setColour (juce::Label::textColourId, theme::muted);
    addChildComponent (srcLabel);

    for (auto* b : { &signatureButton, &transformButton })
    {
        b->setClickingTogglesState (false);
        b->setColour (juce::TextButton::buttonColourId, theme::panel);
        b->setColour (juce::TextButton::buttonOnColourId, theme::accent.withAlpha (0.25f));
        b->setColour (juce::TextButton::textColourOffId, theme::muted);
        b->setColour (juce::TextButton::textColourOnId, theme::accent);
        addAndMakeVisible (b);
    }
    signatureButton.onClick = [this] { setMode (false); };
    transformButton.onClick = [this] { setMode (true); };

    micLabel.setText ("MICROPHONE", juce::dontSendNotification);
    micLabel.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    micLabel.setColour (juce::Label::textColourId, theme::muted);
    addAndMakeVisible (micLabel);

    setupKnob (distanceKnob, distanceLabel, "DISTANCE");
    setupKnob (angleKnob, angleLabel, "ANGLE");
    setupKnob (outputKnob, outputLabel, "OUTPUT");
    distanceAttachment = std::make_unique<APVTS::SliderAttachment> (state, ParamIDs::distance, distanceKnob);
    angleAttachment    = std::make_unique<APVTS::SliderAttachment> (state, ParamIDs::angle, angleKnob);
    outputAttachment   = std::make_unique<APVTS::SliderAttachment> (state, ParamIDs::output, outputKnob);
    distanceKnob.setTextValueSuffix (" cm");
    angleKnob.setTextValueSuffix (juce::String::fromUTF8 ("\xc2\xb0"));
    outputKnob.setTextValueSuffix (" dB");

    // Level: SPL at the capsule for a full-scale sine. Drives the physical electronics model
    // (C-800G tube stage); disabled for microphones without one.
    setupKnob (levelKnob, levelLabel, "LEVEL");
    levelAttachment = std::make_unique<APVTS::SliderAttachment> (state, ParamIDs::level, levelKnob);
    levelKnob.setTextValueSuffix (" dB SPL");

    setupKnob (srcDistanceKnob, srcDistanceLabel, "SRC DISTANCE");
    setupKnob (srcAngleKnob, srcAngleLabel, "SRC ANGLE");
    srcDistanceAttachment = std::make_unique<APVTS::SliderAttachment> (state, ParamIDs::srcDistance, srcDistanceKnob);
    srcAngleAttachment    = std::make_unique<APVTS::SliderAttachment> (state, ParamIDs::srcAngle, srcAngleKnob);
    srcDistanceKnob.setTextValueSuffix (" cm");
    srcAngleKnob.setTextValueSuffix (juce::String::fromUTF8 ("\xc2\xb0"));

    addAndMakeVisible (autoGainButton);
    autoGainAttachment = std::make_unique<APVTS::ButtonAttachment> (state, ParamIDs::autoGain, autoGainButton);

    statusLabel.setFont (juce::FontOptions (12.0f));
    statusLabel.setJustificationType (juce::Justification::topLeft);
    addAndMakeVisible (statusLabel);

    plot.setGrid (processor.getLibrary().frequencies());
    addAndMakeVisible (plot);
    addAndMakeVisible (logo);

    setSize (860, 600);
    startTimerHz (30);
}

MoonMicEditor::~MoonMicEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void MoonMicEditor::setupKnob (juce::Slider& s, juce::Label& l, const juce::String& name)
{
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 18);
    s.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    s.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    s.setColour (juce::Slider::textBoxBackgroundColourId, theme::panel);
    addAndMakeVisible (s);
    l.setText (name, juce::dontSendNotification);
    l.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    l.setColour (juce::Label::textColourId, theme::muted);
    l.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (l);
}

bool MoonMicEditor::isTransform() const
{
    return processor.getState().getRawParameterValue (ParamIDs::transform)->load() >= 0.5f;
}

void MoonMicEditor::setMode (bool transform)
{
    if (auto* p = processor.getState().getParameter (ParamIDs::transform))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (transform ? 1.0f : 0.0f);
        p->endChangeGesture();
    }
}

void MoonMicEditor::timerCallback()
{
    const bool tf = isTransform();
    if (tf != shownTransform || signatureButton.getToggleState() == transformButton.getToggleState())
    {
        shownTransform = tf;
        signatureButton.setToggleState (! tf, juce::dontSendNotification);
        transformButton.setToggleState (tf, juce::dontSendNotification);
        for (juce::Component* c : { (juce::Component*) &srcBox, (juce::Component*) &srcLabel,
                                    (juce::Component*) &srcDistanceKnob, (juce::Component*) &srcDistanceLabel,
                                    (juce::Component*) &srcAngleKnob, (juce::Component*) &srcAngleLabel })
            c->setVisible (tf);
        micLabel.setText (tf ? "TARGET MICROPHONE" : "MICROPHONE", juce::dontSendNotification);
        resized();
    }

    auto curve = processor.getDisplayCurve();
    if (curve != lastCurve)
    {
        lastCurve = curve;
        plot.setCurve (std::move (curve));
    }

    const bool electronics = processor.hasElectronics (micBox.getSelectedItemIndex());
    if (levelKnob.isEnabled() != electronics)
    {
        levelKnob.setEnabled (electronics);
        levelKnob.setAlpha (electronics ? 1.0f : 0.35f);
        levelLabel.setAlpha (electronics ? 1.0f : 0.35f);
    }

    // Decision D-012: no explanatory texts about measurement limits in the UI (they live in the docs).
    // Only a genuine failure is shown.
    if (! processor.isLibraryValid())
    {
        statusLabel.setText ("Model data failed to load: " + processor.getLibraryError(), juce::dontSendNotification);
        statusLabel.setColour (juce::Label::textColourId, theme::warning);
    }
}

void MoonMicEditor::paint (juce::Graphics& g)
{
    g.fillAll (theme::background);

    auto header = getLocalBounds().removeFromTop (headerH).reduced (margin, 0);
    g.setColour (theme::text);
    g.setFont (juce::FontOptions (24.0f, juce::Font::bold));
    g.drawText ("MOON MIC", header.removeFromLeft (170), juce::Justification::centredLeft);


    g.setColour (theme::grid);
    g.drawHorizontalLine (headerH, 0.0f, (float) getWidth());
    g.drawHorizontalLine (getHeight() - footerH, 0.0f, (float) getWidth());

    auto footer = getLocalBounds().removeFromBottom (footerH).reduced (margin, 0);
    footer.removeFromLeft (22); // logo
    g.setColour (theme::muted);
    g.setFont (juce::FontOptions (10.0f, juce::Font::bold).withKerningFactor (0.18f));
    g.drawText ("MOON AUDIO ENGINEERING", footer.removeFromLeft (220), juce::Justification::centredLeft);
    g.setFont (juce::FontOptions (10.0f));
    g.drawText (juce::String::fromUTF8 ("Measured data: Surrey MADIR (CC BY) \xc2\xb7 reference 0.5 m \xc2\xb7 dev build"),
                footer, juce::Justification::centredRight);
}

void MoonMicEditor::resized()
{
    auto area = getLocalBounds();
    area.removeFromTop (headerH);
    auto footer = area.removeFromBottom (footerH).reduced (margin, 0);
    logo.setBounds (footer.removeFromLeft (16).withSizeKeepingCentre (16, 16));

    area.reduce (margin, margin);
    auto left = area.removeFromLeft (320);
    area.removeFromLeft (margin);
    plot.setBounds (area);

    {
        auto header = getLocalBounds().removeFromTop (headerH).reduced (margin, 0);
        header.removeFromLeft (180);
        signatureButton.setBounds (header.removeFromLeft (110).withSizeKeepingCentre (104, 26));
        header.removeFromLeft (6);
        transformButton.setBounds (header.removeFromLeft (110).withSizeKeepingCentre (104, 26));
    }

    if (shownTransform)
    {
        srcLabel.setBounds (left.removeFromTop (16));
        srcBox.setBounds (left.removeFromTop (30));
        left.removeFromTop (10);
    }
    micLabel.setBounds (left.removeFromTop (16));
    micBox.setBounds (left.removeFromTop (30));
    left.removeFromTop (18);

    auto knobs = left.removeFromTop (150);
    const int kw = knobs.getWidth() / 3;
    for (auto [knob, label] : { std::pair { &distanceKnob, &distanceLabel },
                                std::pair { &angleKnob, &angleLabel },
                                std::pair { &outputKnob, &outputLabel } })
    {
        auto col = knobs.removeFromLeft (kw);
        label->setBounds (col.removeFromTop (16));
        knob->setBounds (col);
    }

    left.removeFromTop (8);
    auto row2 = left.removeFromTop (120);
    for (auto [knob, label] : { std::pair { &levelKnob, &levelLabel },
                                std::pair { &srcDistanceKnob, &srcDistanceLabel },
                                std::pair { &srcAngleKnob, &srcAngleLabel } })
    {
        auto col = row2.removeFromLeft (kw);
        label->setBounds (col.removeFromTop (16));
        knob->setBounds (col);
    }

    left.removeFromTop (8);
    autoGainButton.setBounds (left.removeFromTop (26));
    left.removeFromTop (10);
    statusLabel.setBounds (left);
}

} // namespace moonmic
