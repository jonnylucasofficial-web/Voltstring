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
    std::atomic<float> preMeterDb { -90.0f }, postMeterDb { -90.0f };   // peak level into each gate
    std::atomic<bool> preGateOpen { true }, postGateOpen { true };

private:
    static constexpr int numVoices = 18;
    int currentPreset = 0;

    void analyse();                 // runs YIN on the analysis buffer
    bool runYin (int W, double& period, float& confidence);
    void acceptPitch (float midi);
    float processPitchInput (float monoIn);

    double sr = 44100.0;

    // --- pitch tracking (runs on a decimated copy of the input) ---
    int decim = 2, decimCount = 0;
    double anaRate = 22050.0;
    int yinW = 512, tauMin = 20, tauMax = 400, hop = 128, hopCount = 0;
    std::vector<float> anaBuf, yinD;
    float pre1 = 0.0f, pre2 = 0.0f, preCoef = 0.3f;  // anti-alias
    float trackedMidi = -1.0f;      // latest accepted detection
    float pendingMidi = -1.0f;      // octave jump waiting for confirmation
    int pendingCount = 0;
    double lastPeriod = 0.0;        // in analysis samples
    int yinWMax = 512, yinWMin = 128;
    float pre3 = 0.0f, preCoef2 = 0.3f;
    bool staleNote = true, snapPitch = true;   // new note: wait for its pitch, then jump straight to it
    int silenceSamples = 0;
    float validGain = 0.0f;

    // --- synth ---
    float curMidi = 52.0f;          // glided pitch
    std::array<double, numVoices> phase {};
    std::array<juce::SmoothedValue<float>, numVoices> voiceGain;
    double ringPhase = 0.0;
    double syncSlave = 0.0, fmMod = 0.0, ens2 = 0.0, vibPhase = 0.0, pwmPhase = 0.0;
    float vIc1[2] {}, vIc2[2] {}, vA1[2] { 1, 1 }, vA2[2] {}, vA3[2] {};   // vowel formants
    float bIc1 = 0, bIc2 = 0, bA1 = 1, bA2 = 0, bA3 = 0;                     // breath band-pass
    float brassLp1 = 0, brassLp2 = 0, brassCoef = 1.0f;
    int colourCount = 0;
    juce::Random noise;

    // envelope follower
    float rmsState = 0.0f, env = 0.0f, gate = 0.0f;

    // filter (TPT state variable)
    float ic1 = 0.0f, ic2 = 0.0f, fA1 = 1.0f, fA2 = 0.0f, fA3 = 0.0f;
    int filterCount = 0;

    // crush
    float crushHold = 0.0f, crushCounter = 0.0f;

    // ADSR section: triggered by picking when enabled
    juce::ADSR adsr;
    bool adsrNoteOn = false, adsrWasOn = false;
    float slowLvl = 0.0f;
    int retrigHold = 0;
    float lastAmp = 0.0f;

    // noise gates: pre (guitar input) and post (final output)
    struct Gate
    {
        float det = 0.0f, gain = 0.0f;
        int hold = 0;
        bool open = false;
        void reset() { det = gain = 0.0f; hold = 0; open = false; }
        float process (float level, float thr, float closeThr, float detRel, float atk, float rel, int holdLen)
        {
            det = juce::jmax (level, det * detRel);
            if (det > thr) { open = true; hold = holdLen; }
            else if (det < closeThr) { if (hold > 0) --hold; else open = false; }
            const float t = open ? 1.0f : 0.0f;
            gain += (t > gain ? atk : rel) * (t - gain);
            return gain;
        }
    };
    Gate preGate, postGate;
    std::vector<float> preGainBuf;

    juce::Reverb reverb;
    juce::AudioBuffer<float> wetBuf;
    std::vector<float> gateBuf, synthMono;

    juce::SmoothedValue<float> mixS, outS, driveS, verbS;
    int lastEngine = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoltstringProcessor)
};
