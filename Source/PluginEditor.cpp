#include "PluginEditor.h"
#include "Presets.h"

namespace colours
{
    const juce::Colour bg     { 0xff101416 };
    const juce::Colour panel  { 0xff192024 };
    const juce::Colour track  { 0xff2a3439 };
    const juce::Colour accent { 0xff1fc2b2 };   // teal
    const juce::Colour accent2{ 0xfff2a33a };   // amber (FX row)
    const juce::Colour text   { 0xffe6ecec };
    const juce::Colour dim    { 0xff86959a };
}

static const char* paramIds[] = { "transpose", "glide", "detune", "sens", "thresh", "cutoff", "envamt", "reso",
                                  "drive", "crush", "ring", "ringhz", "verb", "vgate", "mix", "output" };
static constexpr int numKnobs = 16;

VoltstringLook::VoltstringLook()
{
    setColour (juce::Slider::textBoxTextColourId, colours::text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Label::textColourId, colours::dim);
    setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    setColour (juce::TextButton::buttonColourId, colours::panel);
    setColour (juce::TextButton::buttonOnColourId, colours::accent);
    setColour (juce::TextButton::textColourOffId, colours::dim);
    setColour (juce::TextButton::textColourOnId, colours::bg);
    setColour (juce::ComboBox::backgroundColourId, colours::panel);
    setColour (juce::ComboBox::outlineColourId, colours::track);
    setColour (juce::ComboBox::textColourId, colours::text);
    setColour (juce::ComboBox::arrowColourId, colours::accent);
    setColour (juce::PopupMenu::backgroundColourId, colours::panel);
    setColour (juce::PopupMenu::textColourId, colours::text);
    setColour (juce::PopupMenu::headerTextColourId, colours::accent);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, colours::accent.withAlpha (0.35f));
}

void VoltstringLook::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                       float start, float end, juce::Slider& s)
{
    auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (6.0f);
    const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const float angle = start + pos * (end - start);
    const auto col = s.getProperties().getWithDefault ("alt", false) ? colours::accent2 : colours::accent;

    juce::Path arcBg, arcVal;
    arcBg.addCentredArc (c.x, c.y, r - 3, r - 3, 0, start, end, true);
    g.setColour (colours::track);
    g.strokePath (arcBg, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const bool bipolar = s.getMinimum() < 0.0 && s.getMaximum() > 0.0;
    const float from = bipolar ? start + (end - start) * (float) s.valueToProportionOfLength (0.0) : start;
    arcVal.addCentredArc (c.x, c.y, r - 3, r - 3, 0, juce::jmin (from, angle), juce::jmax (from, angle), true);
    g.setColour (col);
    g.strokePath (arcVal, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const float kr = r - 10;
    g.setColour (colours::panel.brighter (0.15f));
    g.fillEllipse (c.x - kr, c.y - kr, kr * 2, kr * 2);
    g.setColour (colours::bg);
    g.drawEllipse (c.x - kr, c.y - kr, kr * 2, kr * 2, 1.5f);

    juce::Path tick;
    tick.addRoundedRectangle (-1.5f, -kr + 4, 3.0f, kr * 0.45f, 1.5f);
    g.setColour (colours::text);
    g.fillPath (tick, juce::AffineTransform::rotation (angle).translated (c.x, c.y));
}

void VoltstringLook::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&,
                                           bool highlighted, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    auto fill = b.getToggleState() ? colours::accent : colours::panel;
    if (highlighted && ! b.getToggleState()) fill = fill.brighter (0.1f);
    g.setColour (fill);
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (b.getToggleState() ? colours::accent : colours::track);
    g.drawRoundedRectangle (r, 6.0f, 1.0f);
}

VoltstringEditor::VoltstringEditor (VoltstringProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&look);

    for (int i = 0; i < numKnobs; ++i)
    {
        auto k = std::make_unique<Knob>();
        auto* param = proc.apvts.getParameter (paramIds[i]);
        k->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        k->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 90, 18);
        if (i >= 8) k->slider.getProperties().set ("alt", true);
        k->label.setText (param->getName (20).toUpperCase(), juce::dontSendNotification);
        k->label.setJustificationType (juce::Justification::centred);
        k->label.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        addAndMakeVisible (k->slider);
        addAndMakeVisible (k->label);
        k->attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, paramIds[i], k->slider);
        knobs.push_back (std::move (k));
    }

    for (int i = 0; i < 5; ++i)
    {
        auto& b = engineButtons[(size_t) i];
        b.setButtonText (VoltstringProcessor::engineNames[i].toUpperCase());
        b.setClickingTogglesState (false);
        b.onClick = [this, i]
        {
            if (auto* param = proc.apvts.getParameter ("engine"))
            {
                param->beginChangeGesture();
                param->setValueNotifyingHost (param->convertTo0to1 ((float) i));
                param->endChangeGesture();
            }
        };
        addAndMakeVisible (b);
    }

    // preset browser, grouped by category
    juce::String lastCat;
    for (int i = 0; i < numVoltstringPresets; ++i)
    {
        if (lastCat != voltstringPresets[i].category)
        {
            if (i > 0) presetBox.addSeparator();
            presetBox.addSectionHeading (voltstringPresets[i].category);
            lastCat = voltstringPresets[i].category;
        }
        presetBox.addItem (voltstringPresets[i].name, i + 1);
    }
    presetBox.setTextWhenNothingSelected ("Presets");
    presetBox.onChange = [this]
    {
        auto id = presetBox.getSelectedId();
        if (id > 0 && id - 1 != proc.getCurrentProgram())
            proc.setCurrentProgram (id - 1);
    };
    auto step = [this] (int d)
    {
        const int n = numVoltstringPresets;
        proc.setCurrentProgram ((proc.getCurrentProgram() + d + n) % n);
        refreshPresetBox();
    };
    prevButton.onClick = [step] { step (-1); };
    nextButton.onClick = [step] { step (1); };
    addAndMakeVisible (presetBox);
    addAndMakeVisible (prevButton);
    addAndMakeVisible (nextButton);
    refreshPresetBox();

    setSize (780, 440);
    startTimerHz (30);
}

void VoltstringEditor::refreshPresetBox()
{
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
}

VoltstringEditor::~VoltstringEditor() { setLookAndFeel (nullptr); }

void VoltstringEditor::timerCallback()
{
    if (presetBox.getSelectedId() != proc.getCurrentProgram() + 1)
        refreshPresetBox();
    const int engine = (int) proc.apvts.getRawParameterValue ("engine")->load();
    for (int i = 0; i < 5; ++i)
        engineButtons[(size_t) i].setToggleState (i == engine, juce::dontSendNotification);

    shownNote = proc.displayNote.load();
    shownLevel = proc.displayLevel.load();
    scope[(size_t) scopePos] = shownLevel;
    scopePos = (scopePos + 1) % (int) scope.size();
    repaint (0, 0, getWidth(), 120);
}

void VoltstringEditor::paint (juce::Graphics& g)
{
    g.fillAll (colours::bg);

    g.setColour (colours::text);
    g.setFont (juce::FontOptions (34.0f, juce::Font::bold));
    g.drawText ("VOLTSTRING", 24, 12, 300, 40, juce::Justification::left);
    g.setColour (colours::dim);
    g.setFont (juce::FontOptions (12.0f));
    g.drawText ("guitar synth  /  play single notes", 26, 48, 300, 16, juce::Justification::left);

    // tracked note readout + level trace
    auto readout = juce::Rectangle<float> ((float) getWidth() - 250.0f, 14.0f, 226.0f, 56.0f);
    g.setColour (colours::panel);
    g.fillRoundedRectangle (readout, 8.0f);

    juce::String noteText = "--";
    if (shownNote > 0.0f)
    {
        const int nn = juce::roundToInt (shownNote);
        const int cents = juce::roundToInt ((shownNote - (float) nn) * 100.0f);
        noteText = juce::MidiMessage::getMidiNoteName (nn, true, true, 3)
                 + (cents == 0 ? juce::String() : juce::String (cents > 0 ? "  +" : "  ") + juce::String (cents));
    }
    g.setColour (shownNote > 0.0f ? colours::accent : colours::dim);
    g.setFont (juce::FontOptions (24.0f, juce::Font::bold));
    g.drawText (noteText, readout.removeFromLeft (100.0f).toNearestInt(), juce::Justification::centred);

    auto trace = readout.reduced (8.0f, 10.0f);
    juce::Path pth;
    for (int i = 0; i < (int) scope.size(); ++i)
    {
        const float v = scope[(size_t) ((scopePos + i) % (int) scope.size())];
        const float xx = trace.getX() + trace.getWidth() * (float) i / (float) (scope.size() - 1);
        const float yy = trace.getBottom() - trace.getHeight() * juce::jlimit (0.0f, 1.0f, v);
        if (i == 0) pth.startNewSubPath (xx, yy); else pth.lineTo (xx, yy);
    }
    g.setColour (colours::accent.withAlpha (0.8f));
    g.strokePath (pth, juce::PathStrokeType (2.0f));

    // section panels
    auto body = getLocalBounds().withTrimmedTop (120).reduced (14, 8);
    auto row1 = body.removeFromTop (body.getHeight() / 2).reduced (0, 4);
    auto row2 = body.reduced (0, 4);
    g.setColour (colours::panel);
    g.fillRoundedRectangle (row1.toFloat(), 10.0f);
    g.fillRoundedRectangle (row2.toFloat(), 10.0f);
}

void VoltstringEditor::resized()
{
    auto bar = juce::Rectangle<int> (300, 28, getWidth() - 300 - 262, 28);
    nextButton.setBounds (bar.removeFromRight (28));
    bar.removeFromRight (4);
    prevButton.setBounds (bar.removeFromRight (28));
    bar.removeFromRight (6);
    presetBox.setBounds (bar);

    auto engineRow = juce::Rectangle<int> (24, 80, getWidth() - 48, 30);
    const int bw = engineRow.getWidth() / 5;
    for (auto& b : engineButtons)
        b.setBounds (engineRow.removeFromLeft (bw).reduced (3, 0));

    auto body = getLocalBounds().withTrimmedTop (120).reduced (14, 8);
    auto row1 = body.removeFromTop (body.getHeight() / 2).reduced (6, 10);
    auto row2 = body.reduced (6, 10);

    auto layoutRow = [this] (juce::Rectangle<int> row, int first)
    {
        const int w = row.getWidth() / 8;
        for (int i = 0; i < 8; ++i)
        {
            auto cell = row.removeFromLeft (w);
            auto& k = *knobs[(size_t) (first + i)];
            k.label.setBounds (cell.removeFromTop (16));
            k.slider.setBounds (cell);
        }
    };
    layoutRow (row1, 0);
    layoutRow (row2, 8);
}
