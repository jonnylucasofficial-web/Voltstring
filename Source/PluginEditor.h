#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class VoltstringLook : public juce::LookAndFeel_V4
{
public:
    VoltstringLook();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float start, float end, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&,
                               bool highlighted, bool down) override;
};

class VoltstringEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit VoltstringEditor (VoltstringProcessor&);
    ~VoltstringEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void refreshPresetBox();

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    VoltstringProcessor& proc;
    VoltstringLook look;
    juce::ComboBox presetBox;
    juce::TextButton prevButton { "<" }, nextButton { ">" };
    std::vector<std::unique_ptr<Knob>> knobs;
    std::array<juce::TextButton, 5> engineButtons;
    juce::TextButton envButton { "OFF" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> envAttachment;
    juce::TextButton preButton { "OFF" }, postButton { "OFF" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> preAttachment, postAttachment;
    float shownNote = -1.0f, shownLevel = 0.0f;
    float preMeter = -90.0f, postMeter = -90.0f;   // smoothed for display
    std::array<float, 64> scope {};
    int scopePos = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoltstringEditor)
};
