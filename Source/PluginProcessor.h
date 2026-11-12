#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <atomic>
#include <vector>

// Voltstring - guitar to synth.
// A YIN pitch tracker follows the (monophonic) guitar, a bank of oscillators plays that pitch,
// and an envelope follower makes the synth breathe with your picking. After that: an envelope
// filter, drive, bitcrush, ring mod and a gated reverb.
class VoltstringProcessor : public juce::AudioProcessor
{
public:
    VoltstringProcessor();
    ~VoltstringProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Voltstring"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentPreset; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    static const juce::StringArray engineNames;

    // for the editor
    std::atomic<float> displayNote { -1.0f };   // MIDI note being played, -1 when nothing tracked
    std::atomic<float> displayLevel { 0.0f };   // 0..1 envelope

private:
    static constexpr int numVoices = 8;
    int currentPreset = 0;

    void analyse();                 // runs YIN on the analysis buffer
    float processPitchInput (float monoIn);

    double sr = 44100.0;

    // --- pitch tracking (runs on a decimated copy of the input) ---
    int decim = 2, decimCount = 0;
    double anaRate = 22050.0;
    int yinW = 512, tauMin = 20, tauMax = 400, hop = 128, hopCount = 0;
    std::vector<float> anaBuf, yinD;
    float pre1 = 0.0f, pre2 = 0.0f, preCoef = 0.3f;
    float trackedMidi = -1.0f;      // latest confident detection (median of last 3)
    std::array<float, 3> hist {};
    int histN = 0;

    // --- synth ---
    float curMidi = 52.0f;          // glided pitch
    std::array<double, numVoices> phase {};
    std::array<juce::SmoothedValue<float>, numVoices> voiceGain;
    double ringPhase = 0.0;

    // envelope follower
    float rmsState = 0.0f, env = 0.0f, gate = 0.0f;

    // filter (TPT state variable)
    float ic1 = 0.0f, ic2 = 0.0f, fA1 = 1.0f, fA2 = 0.0f, fA3 = 0.0f;
    int filterCount = 0;

    // crush
    float crushHold = 0.0f, crushCounter = 0.0f;

    juce::Reverb reverb;
    juce::AudioBuffer<float> wetBuf;
    std::vector<float> gateBuf, synthMono;

    juce::SmoothedValue<float> mixS, outS, driveS, verbS;
    int lastEngine = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoltstringProcessor)
};
