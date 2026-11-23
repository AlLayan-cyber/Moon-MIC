#include "PluginEditor.h"
#include "ui/Theme.h"

namespace moonmic
{

namespace
{
    constexpr int headerH = 58, footerH = 30, margin = 16;
    constexpr float kPolarHz[8] = { 125.0f, 250.0f, 500.0f, 1000.0f, 2000.0f, 4000.0f, 8000.0f, 16000.0f };
    const char* const polarNames[8] = { "125", "250", "500", "1k", "2k", "4k", "8k", "16k" };
    const juce::Colour polarColours[8] = { juce::Colour (0xffe0b25a), juce::Colour (0xffd98a5a), juce::Colour (0xffc76f8f),
                                           juce::Colour (0xff8fb3ff), juce::Colour (0xff6fd0c0), juce::Colour (0xff9be07a),
                                           juce::Colour (0xffd7e36a), juce::Colour (0xffb88cf0) };

    // tooltips (UTF-8, Russian): "made recording" = source, "new recording" = target
    const char* const tipSrcDistance = "\xd0\xa1\xd0\xb4\xd0\xb5\xd0\xbb\xd0\xb0\xd0\xbd\xd0\xbd\xd0\xb0\xd1\x8f \xd0\xb7\xd0\xb0\xd0\xbf\xd0\xb8\xd1\x81\xd1\x8c: \xd0\xbd\xd0\xb0 \xd0\xba\xd0\xb0\xd0\xba\xd0\xbe\xd0\xbc \xd1\x80\xd0\xb0\xd1\x81\xd1\x81\xd1\x82\xd0\xbe\xd1\x8f\xd0\xbd\xd0\xb8\xd0\xb8 \xd0\xbe\xd1\x82 \xd0\xbc\xd0\xb8\xd0\xba\xd1\x80\xd0\xbe\xd1\x84\xd0\xbe\xd0\xbd\xd0\xb0 \xd0\xbf\xd0\xb5\xd0\xbb\xd0\xb8";
    const char* const tipSrcAngle = "\xd0\xa1\xd0\xb4\xd0\xb5\xd0\xbb\xd0\xb0\xd0\xbd\xd0\xbd\xd0\xb0\xd1\x8f \xd0\xb7\xd0\xb0\xd0\xbf\xd0\xb8\xd1\x81\xd1\x8c: \xd0\xbf\xd0\xbe\xd0\xb4 \xd0\xba\xd0\xb0\xd0\xba\xd0\xb8\xd0\xbc \xd1\x83\xd0\xb3\xd0\xbb\xd0\xbe\xd0\xbc \xd0\xba \xd0\xbc\xd0\xb8\xd0\xba\xd1\x80\xd0\xbe\xd1\x84\xd0\xbe\xd0\xbd\xd1\x83 \xd0\xbf\xd0\xb5\xd0\xbb\xd0\xb8";
    const char* const tipSrcMic = "\xd0\xa1\xd0\xb4\xd0\xb5\xd0\xbb\xd0\xb0\xd0\xbd\xd0\xbd\xd0\xb0\xd1\x8f \xd0\xb7\xd0\xb0\xd0\xbf\xd0\xb8\xd1\x81\xd1\x8c: \xd0\xbd\xd0\xb0 \xd0\xba\xd0\xb0\xd0\xba\xd0\xbe\xd0\xb9 \xd0\xbc\xd0\xb8\xd0\xba\xd1\x80\xd0\xbe\xd1\x84\xd0\xbe\xd0\xbd \xd0\xb7\xd0\xb0\xd0\xbf\xd0\xb8\xd1\x81\xd0\xb0\xd0\xbd \xd0\xb2\xd0\xbe\xd0\xba\xd0\xb0\xd0\xbb";
    const char* const tipDistanceTf = "\xd0\x9d\xd0\xbe\xd0\xb2\xd0\xb0\xd1\x8f \xd0\xb7\xd0\xb0\xd0\xbf\xd0\xb8\xd1\x81\xd1\x8c: \xd0\xbd\xd0\xb0 \xd0\xba\xd0\xb0\xd0\xba\xd0\xbe\xd0\xbc \xd1\x80\xd0\xb0\xd1\x81\xd1\x81\xd1\x82\xd0\xbe\xd1\x8f\xd0\xbd\xd0\xb8\xd0\xb8 \xd0\xbe\xd1\x82 \xd0\xbd\xd0\xbe\xd0\xb2\xd0\xbe\xd0\xb3\xd0\xbe \xd0\xbc\xd0\xb8\xd0\xba\xd1\x80\xd0\xbe\xd1\x84\xd0\xbe\xd0\xbd\xd0\xb0";
    const char* const tipAngleTf = "\xd0\x9d\xd0\xbe\xd0\xb2\xd0\xb0\xd1\x8f \xd0\xb7\xd0\xb0\xd0\xbf\xd0\xb8\xd1\x81\xd1\x8c: \xd0\xbf\xd0\xbe\xd0\xb4 \xd0\xba\xd0\xb0\xd0\xba\xd0\xb8\xd0\xbc \xd1\x83\xd0\xb3\xd0\xbb\xd0\xbe\xd0\xbc \xd0\xba \xd0\xbd\xd0\xbe\xd0\xb2\xd0\xbe\xd0\xbc\xd1\x83 \xd0\xbc\xd0\xb8\xd0\xba\xd1\x80\xd0\xbe\xd1\x84\xd0\xbe\xd0\xbd\xd1\x83";
    const char* const tipMicTf = "\xd0\x9d\xd0\xbe\xd0\xb2\xd0\xb0\xd1\x8f \xd0\xb7\xd0\xb0\xd0\xbf\xd0\xb8\xd1\x81\xd1\x8c: \xd0\xba\xd0\xb0\xd0\xba\xd0\xbe\xd0\xb9 \xd0\xbc\xd0\xb8\xd0\xba\xd1\x80\xd0\xbe\xd1\x84\xd0\xbe\xd0\xbd \xd0\xb4\xd0\xbe\xd0\xbb\xd0\xb6\xd0\xb5\xd0\xbd \xd0\xbf\xd0\xbe\xd0\xbb\xd1\x83\xd1\x87\xd0\xb8\xd1\x82\xd1\x8c\xd1\x81\xd1\x8f";
    const char* const tipDistance = "\xd0\xa0\xd0\xb0\xd1\x81\xd1\x81\xd1\x82\xd0\xbe\xd1\x8f\xd0\xbd\xd0\xb8\xd0\xb5 \xd0\xbe\xd1\x82 \xd0\xbf\xd0\xb5\xd0\xb2\xd1\x86\xd0\xb0 \xd0\xb4\xd0\xbe \xd0\xbc\xd0\xb8\xd0\xba\xd1\x80\xd0\xbe\xd1\x84\xd0\xbe\xd0\xbd\xd0\xb0";
    const char* const tipAngle = "\xd0\xa3\xd0\xb3\xd0\xbe\xd0\xbb \xd0\xbc\xd0\xb5\xd0\xb6\xd0\xb4\xd1\x83 \xd0\xbf\xd0\xb5\xd0\xb2\xd1\x86\xd0\xbe\xd0\xbc \xd0\xb8 \xd0\xbe\xd1\x81\xd1\x8c\xd1\x8e \xd0\xbc\xd0\xb8\xd0\xba\xd1\x80\xd0\xbe\xd1\x84\xd0\xbe\xd0\xbd\xd0\xb0";

    juce::String patternLabel (const std::string& p)
    {
        if (p == "Bidirect") return "Figure-8";
        if (p == "Hyper")    return "Hyper";
        if (p == "Omni")     return "Omni";
        if (p == "Cardioid") return "Cardioid";
        return {};
    }

    void styleToggle (juce::TextButton& b)
    {
        b.setColour (juce::TextButton::buttonColourId, theme::panel);
        b.setColour (juce::TextButton::buttonOnColourId, theme::accent.withAlpha (0.25f));
        b.setColour (juce::TextButton::textColourOffId, theme::muted);
        b.setColour (juce::TextButton::textColourOnId, theme::accent);
    }
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
    setColour (juce::TextButton::buttonColourId, theme::panel);
    setColour (juce::TextButton::textColourOffId, theme::muted);
    setColour (juce::TooltipWindow::backgroundColourId, theme::panel.brighter (0.12f));
    setColour (juce::TooltipWindow::textColourId, theme::text);
    setColour (juce::TooltipWindow::outlineColourId, theme::grid.brighter (0.3f));
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
MicSelector::MicSelector (const ModelLibrary& lib, juce::String titleText)
{
    // group the model sets by microphone ("U87 Ai - Cardioid" -> model "U87 Ai", pattern "Cardioid")
    const auto dash = juce::String::fromUTF8 (" \xe2\x80\x94 ");
    for (int i = 0; i < lib.numSets(); ++i)
    {
        const auto& s = lib.set (i);
        const auto name = juce::String::fromUTF8 (s.displayName.c_str());
        const auto model = name.contains (dash) ? name.upToFirstOccurrenceOf (dash, false, false) : name;
        auto it = std::find_if (groups.begin(), groups.end(), [&] (const Group& g) { return g.model == model; });
        if (it == groups.end()) { groups.push_back ({ model, {} }); it = groups.end() - 1; }
        it->patterns.push_back ({ patternLabel (s.pattern), i });
    }
    for (size_t g = 0; g < groups.size(); ++g)
        modelBox.addItem (groups[g].model, (int) g + 1);

    title.setText (titleText, juce::dontSendNotification);
    title.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    title.setColour (juce::Label::textColourId, theme::muted);
    addAndMakeVisible (title);
    addAndMakeVisible (modelBox);
    modelBox.onChange = [this]
    {
        const int g = modelBox.getSelectedId() - 1;
        if (g < 0 || g >= (int) groups.size() || ! onSelect) return;
        // keep the current pattern if the new microphone has it
        juce::String want;
        for (const auto& grp : groups)
            for (const auto& [label, idx] : grp.patterns)
                if (idx == currentSet) want = label;
        int pick = groups[(size_t) g].patterns.front().second;
        for (const auto& [label, idx] : groups[(size_t) g].patterns)
            if (label == want) pick = idx;
        onSelect (pick);
    };
    for (auto& b : patternButtons)
    {
        styleToggle (b);
        b.setClickingTogglesState (false);
        addChildComponent (b);
    }
}

void MicSelector::setTitle (const juce::String& t)  { title.setText (t, juce::dontSendNotification); }

void MicSelector::setTip (const juce::String& tip)
{
    modelBox.setTooltip (tip);
    for (auto& b : patternButtons) b.setTooltip (tip);
}

void MicSelector::setSelectedSet (int setIndex)
{
    if (setIndex == currentSet)
        return;
    currentSet = setIndex;
    for (size_t g = 0; g < groups.size(); ++g)
        for (const auto& p : groups[g].patterns)
            if (p.second == setIndex)
            {
                modelBox.setSelectedId ((int) g + 1, juce::dontSendNotification);
                showGroup ((int) g, setIndex);
                return;
            }
}

void MicSelector::showGroup (int g, int setIndex)
{
    const auto& grp = groups[(size_t) g];
    const bool fixed = grp.patterns.size() == 1;
    for (size_t i = 0; i < patternButtons.size(); ++i)
    {
        auto& b = patternButtons[i];
        if (fixed || i >= grp.patterns.size()) { b.setVisible (false); continue; }
        const auto [label, idx] = grp.patterns[i];
        b.setButtonText (label);
        b.setToggleState (idx == setIndex, juce::dontSendNotification);
        b.onClick = [this, idx = idx] { if (onSelect) onSelect (idx); };
        b.setVisible (true);
    }
    resized();
}

void MicSelector::resized()
{
    auto r = getLocalBounds();
    title.setBounds (r.removeFromTop (16));
    modelBox.setBounds (r.removeFromTop (30));
    r.removeFromTop (6);
    auto row = r.removeFromTop (26);
    int visible = 0;
    for (auto& b : patternButtons) visible += b.isVisible() ? 1 : 0;
    if (visible == 0) return;
    const int w = row.getWidth() / visible;
    for (auto& b : patternButtons)
        if (b.isVisible())
            b.setBounds (row.removeFromLeft (w).reduced (2, 0));
}

//==============================================================================
MoonMicEditor::MoonMicEditor (MoonMicProcessor& p)
    : AudioProcessorEditor (p), processor (p),
      sourceSelector (p.getLibrary(), "SOURCE (RECORDED WITH)"),
      targetSelector (p.getLibrary(), "MICROPHONE")
{
    setLookAndFeel (&lnf);
    auto& state = processor.getState();

    // header: mode, A/B, bypass
    for (auto* b : { &signatureButton, &transformButton, &slotA, &slotB })
    {
        styleToggle (*b);
        b->setClickingTogglesState (false);
        addAndMakeVisible (b);
    }
    signatureButton.onClick = [this] { setParam (ParamIDs::transform, 0.0f); };
    transformButton.onClick = [this] { setParam (ParamIDs::transform, 1.0f); };
    slotA.onClick = [this] { processor.selectSlot (0); };
    slotB.onClick = [this] { processor.selectSlot (1); };
    copyButton.setButtonText (juce::String::fromUTF8 ("COPY \xe2\x86\x92"));
    copyButton.onClick = [this] { processor.copyActiveToOther(); };
    styleToggle (copyButton);
    addAndMakeVisible (copyButton);
    styleToggle (bypassButton);
    bypassButton.setColour (juce::TextButton::buttonOnColourId, theme::warning.withAlpha (0.3f));
    bypassButton.setColour (juce::TextButton::textColourOnId, theme::warning);
    bypassButton.setClickingTogglesState (true);
    addAndMakeVisible (bypassButton);
    bypassAttachment = std::make_unique<APVTS::ButtonAttachment> (state, ParamIDs::bypass, bypassButton);

    // microphones
    addChildComponent (sourceSelector);
    addAndMakeVisible (targetSelector);
    sourceSelector.onSelect = [this] (int idx) { setParam (ParamIDs::srcMic, (float) idx); };
    targetSelector.onSelect = [this] (int idx) { setParam (ParamIDs::mic, (float) idx); };

    // knobs
    setupKnob (distanceKnob, distanceLabel, "DISTANCE");
    setupKnob (angleKnob, angleLabel, "ANGLE");
    setupKnob (levelKnob, levelLabel, "LEVEL");
    setupKnob (srcDistanceKnob, srcDistanceLabel, "SRC DISTANCE");
    setupKnob (srcAngleKnob, srcAngleLabel, "SRC ANGLE");
    setupKnob (outputKnob, outputLabel, "OUTPUT");
    distanceAttachment    = std::make_unique<APVTS::SliderAttachment> (state, ParamIDs::distance, distanceKnob);
    angleAttachment       = std::make_unique<APVTS::SliderAttachment> (state, ParamIDs::angle, angleKnob);
    levelAttachment       = std::make_unique<APVTS::SliderAttachment> (state, ParamIDs::level, levelKnob);
    srcDistanceAttachment = std::make_unique<APVTS::SliderAttachment> (state, ParamIDs::srcDistance, srcDistanceKnob);
    srcAngleAttachment    = std::make_unique<APVTS::SliderAttachment> (state, ParamIDs::srcAngle, srcAngleKnob);
    outputAttachment      = std::make_unique<APVTS::SliderAttachment> (state, ParamIDs::output, outputKnob);
    const auto deg = juce::String::fromUTF8 ("\xc2\xb0");
    distanceKnob.setTextValueSuffix (" cm");
    srcDistanceKnob.setTextValueSuffix (" cm");
    angleKnob.setTextValueSuffix (deg);
    srcAngleKnob.setTextValueSuffix (deg);
    levelKnob.setTextValueSuffix (" dB SPL");
    outputKnob.setTextValueSuffix (" dB");
    for (auto* c : { (juce::Component*) &srcDistanceKnob, (juce::Component*) &srcDistanceLabel,
                     (juce::Component*) &srcAngleKnob, (juce::Component*) &srcAngleLabel })
        c->setVisible (false);

    addAndMakeVisible (autoGainButton);
    autoGainAttachment = std::make_unique<APVTS::ButtonAttachment> (state, ParamIDs::autoGain, autoGainButton);

    statusLabel.setFont (juce::FontOptions (12.0f));
    statusLabel.setJustificationType (juce::Justification::topLeft);
    addAndMakeVisible (statusLabel);

    // stage view: drag the singer (and the source position in TRANSFORM)
    stage.onMove = [this] (int i, float cm, float ang)
    {
        setParam (i == 0 ? ParamIDs::distance : ParamIDs::srcDistance, cm);
        setParam (i == 0 ? ParamIDs::angle : ParamIDs::srcAngle, ang);
    };
    stage.onGesture = [this] (int i, bool begin)
    {
        gesture (i == 0 ? ParamIDs::distance : ParamIDs::srcDistance, begin);
        gesture (i == 0 ? ParamIDs::angle : ParamIDs::srcAngle, begin);
    };
    addAndMakeVisible (stage);

    // polar display + frequency selection (up to 4 at once)
    addAndMakeVisible (polarPlot);
    for (size_t i = 0; i < freqButtons.size(); ++i)
    {
        auto& b = freqButtons[i];
        b.setButtonText (polarNames[i]);
        styleToggle (b);
        b.setColour (juce::TextButton::textColourOnId, polarColours[i]);
        b.setClickingTogglesState (false);
        b.setToggleState (freqOn[i], juce::dontSendNotification);
        b.onClick = [this, i]
        {
            int count = 0;
            for (bool on : freqOn) count += on ? 1 : 0;
            if (freqOn[i] && count == 1) return;           // keep at least one
            if (! freqOn[i] && count >= 4) return;          // at most four
            freqOn[i] = ! freqOn[i];
            freqButtons[i].setToggleState (freqOn[i], juce::dontSendNotification);
            updatePolar();
        };
        addAndMakeVisible (b);
    }

    plot.setGrid (processor.getLibrary().frequencies());
    addAndMakeVisible (plot);
    addAndMakeVisible (logo);

    setResizable (true, true);
    setResizeLimits (980, 620, 1900, 1200);
    setSize (1180, 720);
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
    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 86, 18);
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

float MoonMicEditor::param (const char* id) const
{
    return processor.getState().getRawParameterValue (id)->load();
}

void MoonMicEditor::setParam (const char* id, float plainValue)
{
    if (auto* p = processor.getState().getParameter (id))
    {
        const float v = p->convertTo0to1 (plainValue);
        if (std::abs (p->getValue() - v) < 1.0e-7f) return;
        p->setValueNotifyingHost (v);
    }
}

void MoonMicEditor::gesture (const char* id, bool begin)
{
    if (auto* p = processor.getState().getParameter (id))
    {
        if (begin) p->beginChangeGesture(); else p->endChangeGesture();
    }
}

void MoonMicEditor::updatePolar()
{
    const int set = juce::jlimit (0, processor.getLibrary().numSets() - 1, (int) param (ParamIDs::mic));
    const float distM = param (ParamIDs::distance) / 100.0f;
    if (set == polarSet && std::abs (distM - polarDist) < 1.0e-4f && freqOn == polarFreqs)
        return;
    polarSet = set; polarDist = distM; polarFreqs = freqOn;

    std::vector<PolarPlot::Curve> curves;
    std::vector<float> stagePolar;
    juce::String stageLabel;
    for (size_t i = 0; i < 8; ++i)
        if (freqOn[i])
        {
            auto db = SignatureDesigner::polarDb (processor.getLibrary(), set, kPolarHz[i], distM);
            if (stagePolar.empty() || kPolarHz[i] == 1000.0f)
            {
                stagePolar = db;
                stageLabel = juce::String ("Pattern at ") + polarNames[i] + " Hz";
            }
            curves.push_back ({ std::move (db), polarColours[i], juce::String (polarNames[i]) });
        }
    polarPlot.setCurves (std::move (curves));
    stage.setPolar (std::move (stagePolar), stageLabel);
}

void MoonMicEditor::timerCallback()
{
    const bool tf = isTransform();
    if (tf != shownTransform || signatureButton.getToggleState() == transformButton.getToggleState())
    {
        shownTransform = tf;
        signatureButton.setToggleState (! tf, juce::dontSendNotification);
        transformButton.setToggleState (tf, juce::dontSendNotification);
        for (juce::Component* c : { (juce::Component*) &sourceSelector,
                                    (juce::Component*) &srcDistanceKnob, (juce::Component*) &srcDistanceLabel,
                                    (juce::Component*) &srcAngleKnob, (juce::Component*) &srcAngleLabel })
            c->setVisible (tf);
        targetSelector.setTitle (tf ? "TARGET MICROPHONE" : "MICROPHONE");
        auto u8 = [] (const char* s) { return juce::String::fromUTF8 (s); };
        sourceSelector.setTip (u8 (tipSrcMic));
        srcDistanceKnob.setTooltip (u8 (tipSrcDistance));
        srcAngleKnob.setTooltip (u8 (tipSrcAngle));
        targetSelector.setTip (tf ? u8 (tipMicTf) : juce::String());
        distanceKnob.setTooltip (u8 (tf ? tipDistanceTf : tipDistance));
        angleKnob.setTooltip (u8 (tf ? tipAngleTf : tipAngle));
        resized();
    }
    slotA.setToggleState (processor.activeSlot() == 0, juce::dontSendNotification);
    slotB.setToggleState (processor.activeSlot() == 1, juce::dontSendNotification);
    copyButton.setButtonText (juce::String::fromUTF8 (processor.activeSlot() == 0 ? "COPY A \xe2\x86\x92 B" : "COPY B \xe2\x86\x92 A"));

    const int mic = (int) param (ParamIDs::mic);
    targetSelector.setSelectedSet (mic);
    sourceSelector.setSelectedSet ((int) param (ParamIDs::srcMic));

    StageView::Marker target;
    target.distanceCm = param (ParamIDs::distance);
    target.angleDeg = param (ParamIDs::angle);
    target.visible = true;
    target.colour = theme::accent;
    target.label = tf ? "TARGET" : "SINGER";
    stage.setMarker (0, target);
    StageView::Marker src;
    src.distanceCm = param (ParamIDs::srcDistance);
    src.angleDeg = param (ParamIDs::srcAngle);
    src.visible = tf;
    src.hollow = true;
    src.colour = theme::warning;
    src.label = "SOURCE";
    stage.setMarker (1, src);
    updatePolar();

    auto curve = processor.getDisplayCurve();
    if (curve != lastCurve)
    {
        lastCurve = curve;
        plot.setCurve (std::move (curve));
    }

    const bool electronics = processor.hasElectronics (mic);
    if (levelKnob.isEnabled() != electronics)
    {
        levelKnob.setEnabled (electronics);
        levelKnob.setAlpha (electronics ? 1.0f : 0.35f);
        levelLabel.setAlpha (electronics ? 1.0f : 0.35f);
    }

    // Decision D-012: no explanatory texts about measurement limits in the UI. Only a genuine failure is shown.
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
    // header
    {
        auto h = getLocalBounds().removeFromTop (headerH).reduced (margin, 0);
        h.removeFromLeft (180);
        signatureButton.setBounds (h.removeFromLeft (120).withSizeKeepingCentre (114, 30));
        transformButton.setBounds (h.removeFromLeft (120).withSizeKeepingCentre (114, 30));
        bypassButton.setBounds (h.removeFromRight (96).withSizeKeepingCentre (88, 28));
        h.removeFromRight (18);
        copyButton.setBounds (h.removeFromRight (120).withSizeKeepingCentre (112, 28));
        slotB.setBounds (h.removeFromRight (40).withSizeKeepingCentre (34, 28));
        slotA.setBounds (h.removeFromRight (40).withSizeKeepingCentre (34, 28));
    }

    auto area = getLocalBounds();
    area.removeFromTop (headerH);
    auto footer = area.removeFromBottom (footerH).reduced (margin, 0);
    logo.setBounds (footer.removeFromLeft (16).withSizeKeepingCentre (16, 16));
    area.reduce (margin, margin);

    // left column: microphones and precise controls
    auto left = area.removeFromLeft (300);
    area.removeFromLeft (margin);
    if (shownTransform)
    {
        sourceSelector.setBounds (left.removeFromTop (80));
        left.removeFromTop (10);
    }
    targetSelector.setBounds (left.removeFromTop (80));
    left.removeFromTop (14);

    const int kw = left.getWidth() / 3;
    auto placeRow = [&] (std::initializer_list<std::pair<juce::Slider*, juce::Label*>> knobs, int h)
    {
        auto row = left.removeFromTop (h);
        for (auto [knob, label] : knobs)
        {
            auto col = row.removeFromLeft (kw);
            label->setBounds (col.removeFromTop (16));
            knob->setBounds (col);
        }
        left.removeFromTop (6);
    };
    placeRow ({ { &distanceKnob, &distanceLabel }, { &angleKnob, &angleLabel }, { &levelKnob, &levelLabel } }, 118);
    if (shownTransform)
        placeRow ({ { &srcDistanceKnob, &srcDistanceLabel }, { &srcAngleKnob, &srcAngleLabel } }, 118);
    placeRow ({ { &outputKnob, &outputLabel } }, 118);
    autoGainButton.setBounds (left.removeFromTop (26));
    statusLabel.setBounds (left);

    // right column: polar display and response
    auto right = area.removeFromRight (juce::jmin (340, area.getWidth() / 3));
    area.removeFromRight (margin);
    const int ps = right.getWidth();
    polarPlot.setBounds (right.removeFromTop (ps));
    right.removeFromTop (6);
    auto fb = right.removeFromTop (24);
    const int bw = fb.getWidth() / (int) freqButtons.size();
    for (auto& b : freqButtons)
        b.setBounds (fb.removeFromLeft (bw).reduced (1, 0));
    right.removeFromTop (10);
    plot.setBounds (right);

    // centre: stage view
    stage.setBounds (area);
}

} // namespace moonmic
