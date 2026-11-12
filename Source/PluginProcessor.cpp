#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"
#include <cmath>
#include <cstring>

namespace
{
    constexpr double twoPi = juce::MathConstants<double>::twoPi;

    float polyBlepFwd (double t, double dt)
    {
        if (t < dt)       { t /= dt;           return (float) (t + t - t * t - 1.0); }
        if (t > 1.0 - dt) { t = (t - 1.0) / dt; return (float) (t * t + t + t + 1.0); }
        return 0.0f;
    }

    // Voice slots:
    //  0 saw   1 saw+detune   2 saw-detune   3 square   4 sub sine (-1 oct)   5 sub square (-1 oct)
    //  6 fifth (saw)   7 octave-up tri   8 PWM pulse   9 hard-sync saw   10 FM   11 wavefolder
    // 12 vowel (formant saw)   13 breath noise   14 sine   15 ensemble saws   16 narrow pulse   17 triangle
    // Each engine is a mix of voices plus a few character settings.
    struct Voicing
    {
        float g[18];
        float atkMs, relMs;
        float vibCents, vibHz;   // vibrato that grows as the note sustains
        float brass;             // brightness that opens with playing level (brass / reed)
        float syncMax;           // hard-sync sweep amount
        float fmRatio, fmIndex;  // FM modulator ratio and depth
        float pwm;               // pulse width modulation depth
        float narrowW;           // narrow pulse width
    };
    //                       0     1     2     3     4     5     6     7     8     9    10    11    12    13    14    15    16    17
    const Voicing voicings[] = {
        { { .15f,   0,    0, .45f, .95f, .40f,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 },   2,   80,  0, 0,  0,   0, 1, 0,   0, .125f }, // SUB
        { { .55f, .45f, .45f, .20f, .30f,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 },   2,  100,  0, 0,  0,   0, 1, 0,   0, .125f }, // ANALOG
        { { .80f, .30f,    0, .30f, .20f,    0,    0, .25f,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 },   1,   70,  0, 0,  0,   0, 1, 0,   0, .125f }, // LEAD
        { { .35f, .50f, .50f,    0, .20f,    0,    0, .30f,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 }, 600,  900,  0, 0,  0,   0, 1, 0,   0, .125f }, // PAD
        { { .55f, .20f,    0, .20f, .25f,    0, .55f, .25f,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 },   2,  100,  0, 0,  0,   0, 1, 0,   0, .125f }, // FIFTHS
        { {    0,    0,    0,    0, .15f,    0,    0,    0,    0,    0,    0,    0,    0,    0, .90f,    0,    0,    0 },   2,  100,  0, 0,  0,   0, 1, 0,   0, .125f }, // SINE
        { {    0,    0,    0, .75f,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 },   2,  100,  0, 0,  0,   0, 1, 0,   0, .125f }, // SQUARE
        { {    0,    0,    0,    0, .10f,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, 1.0f },   2,  100,  0, 0,  0,   0, 1, 0,   0, .125f }, // TRIANGLE
        { { .85f,   0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0 },   2,  100,  0, 0,  0,   0, 1, 0,   0, .125f }, // SAW
        { {    0,    0,    0,    0, .20f,    0,    0,    0, .80f,    0,    0,    0,    0,    0,    0,    0,    0,    0 },   2,  120,  0, 0,  0,   0, 1, 0, .4f, .125f }, // PWM
        { {    0,    0,    0,    0, .20f,    0,    0,    0,    0, .80f,    0,    0,    0,    0,    0,    0,    0,    0 },   2,  100,  0, 0,  0,   4, 1, 0,   0, .125f }, // SYNC
        { {    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, .85f,    0,    0,    0,    0,    0,    0,    0 },   1,  700,  0, 0,  0,   0, 3.5f, 4,  0, .125f }, // FM BELL
        { {    0,    0,    0,    0, .15f,    0,    0,    0,    0,    0,    0, .85f,    0,    0,    0,    0,    0,    0 },   2,  100,  0, 0,  0,   0, 1, 0,   0, .125f }, // FOLD
        { {    0,    0,    0,    0, .10f,    0,    0,    0,    0,    0,    0,    0, 1.2f,    0,    0,    0,    0,    0 },   5,  120,  6, 5.5f, 0, 0, 1, 0,   0, .125f }, // VOWEL
        { { .55f, .40f, .35f,    0,    0,    0,    0,    0, .25f,    0,    0,    0,    0,    0,    0,    0,    0,    0 },  35,  140,  7, 5.2f, 1, 0, 1, 0, .1f, .125f }, // BRASS
        { { .30f, .35f, .35f,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, .55f,    0,    0 }, 220,  650,  9, 5.6f, 0, 0, 1, 0,   0, .125f }, // STRINGS
        { {    0,    0,    0,    0,    0,    0,    0, .08f,    0,    0,    0,    0,    0, .90f, .70f,    0,    0,    0 },  70,  220, 14, 5.0f, 0, 0, 1, 0,   0, .125f }, // WIND
        { {    0,    0,    0, .20f, .10f,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, .65f,    0 },  15,  120,  8, 5.5f, .6f, 0, 1, 0,  0, .2f   }, // REED
        { {    0,    0,    0, .45f,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, .45f,    0 },   1,   40,  0, 0,  0,   0, 1, 0,   0, .125f }, // CHIP
        { {    0,    0,    0,    0,    0,    0,    0,    0,    0, .45f, .50f, .30f,    0,    0,    0,    0,    0,    0 },   1,  150,  0, 0,  0,   6, 1.414f, 7, 0, .125f }, // GLITCH
        { {    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, .90f,    0, .35f,    0,    0 }, 400, 1300,  0, 0,  0,   0, 1, 0,   0, .125f }, // NOISE WASH
        { {    0,    0,    0,    0, .45f,    0,    0, .35f,    0,    0,    0,    0,    0,    0, .55f,    0,    0,    0 },   3,   60,  0, 0,  0,   0, 1, 0,   0, .125f }, // ORGAN
    };
    constexpr int numEngines = (int) (sizeof (voicings) / sizeof (voicings[0]));

    // small state-variable filter used for formants, breath and the brass bloom
    inline void svfSet (float& a1, float& a2, float& a3, float k, double fc, double rate)
    {
        const float g = (float) std::tan (juce::MathConstants<double>::pi * juce::jlimit (20.0, rate * 0.45, fc) / rate);
        a1 = 1.0f / (1.0f + g * (g + k)); a2 = g * a1; a3 = g * a2;
    }
    inline float svfBand (float x, float& ic1, float& ic2, float a1, float a2, float a3, float k)
    {
        const float v3 = x - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0f * v1 - ic1; ic2 = 2.0f * v2 - ic2;
        return v1 * k;   // normalised band-pass
    }

    inline float pulseOsc (double p, double dt, double w)
    {
        float v = p < w ? 1.0f : -1.0f;
        double p2 = p - w; if (p2 < 0.0) p2 += 1.0;
        v += polyBlepFwd (p, dt) - polyBlepFwd (p2, dt);
        return v - (float) (2.0 * w - 1.0);
    }

    inline float polyBlep (double t, double dt)
    {
        if (t < dt)       { t /= dt;           return (float) (t + t - t * t - 1.0); }
        if (t > 1.0 - dt) { t = (t - 1.0) / dt; return (float) (t * t + t + t + 1.0); }
        return 0.0f;
    }

    inline float sawOsc (double p, double dt)    { return (float) (2.0 * p - 1.0) - polyBlep (p, dt); }
    inline float squareOsc (double p, double dt)
    {
        float v = p < 0.5 ? 1.0f : -1.0f;
        double p2 = p + 0.5; if (p2 >= 1.0) p2 -= 1.0;
        return v + polyBlep (p, dt) - polyBlep (p2, dt);
    }
    inline float sineOsc (double p) { return (float) std::sin (twoPi * p); }
    inline float triOsc (double p)  { return (float) (1.0 - 4.0 * std::abs (p - 0.5)); }

    inline float coefFromMs (float ms, double rate)
    {
        return ms <= 0.0f ? 1.0f : (float) (1.0 - std::exp (-1.0 / (ms * 0.001 * rate)));
    }

}

const juce::StringArray VoltstringProcessor::engineNames { "Sub", "Analog", "Lead", "Pad", "Fifths", "Sine", "Square", "Triangle",
    "Saw", "PWM", "Sync", "FM Bell", "Fold", "Vowel", "Brass", "Strings", "Wind", "Reed", "Chip", "Glitch", "Noise Wash", "Organ" };

VoltstringProcessor::VoltstringProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "VOLTSTRING", createLayout())
{
    setCurrentProgram (0);
}

juce::AudioProcessorValueTreeState::ParameterLayout VoltstringProcessor::createLayout()
{
    using P = juce::AudioParameterFloat;
    using R = juce::NormalisableRange<float>;
    using A = juce::AudioParameterFloatAttributes;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    auto pct = A().withStringFromValueFunction ([] (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + " %"; });
    auto unit = [] (const char* u, int dec)
    {
        return A().withLabel (u).withStringFromValueFunction ([u, dec] (float v, int) { return juce::String (v, dec) + " " + u; });
    };
    auto hz = A().withLabel ("Hz").withStringFromValueFunction ([] (float v, int)
        { return v >= 1000.0f ? juce::String (v / 1000.0f, 2) + " kHz" : juce::String (juce::roundToInt (v)) + " Hz"; });

    R glideR (0.0f, 1000.0f, 0.1f);   glideR.setSkewForCentre (80.0f);
    R cutR (40.0f, 12000.0f, 1.0f);   cutR.setSkewForCentre (900.0f);
    R envR (0.0f, 10000.0f, 1.0f);    envR.setSkewForCentre (2000.0f);
    R ringR (20.0f, 2000.0f, 0.1f);   ringR.setSkewForCentre (220.0f);
    R gateR (30.0f, 1500.0f, 1.0f);   gateR.setSkewForCentre (250.0f);

    p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "engine", 1 }, "Engine", engineNames, 1));
    p.push_back (std::make_unique<P> (juce::ParameterID { "detune", 1 }, "Detune", R (0.0f, 50.0f, 0.1f), 12.0f, unit ("ct", 1)));
    p.push_back (std::make_unique<juce::AudioParameterInt> (juce::ParameterID { "transpose", 1 }, "Transpose", -24, 24, 0,
        juce::AudioParameterIntAttributes().withStringFromValueFunction ([] (int v, int) { return (v > 0 ? "+" : "") + juce::String (v) + " st"; })));
    p.push_back (std::make_unique<P> (juce::ParameterID { "glide", 1 }, "Glide", glideR, 25.0f, unit ("ms", 0)));
    p.push_back (std::make_unique<P> (juce::ParameterID { "sens", 1 }, "Sens", R (1.0f, 30.0f, 0.1f), 8.0f, unit ("x", 1)));
    p.push_back (std::make_unique<P> (juce::ParameterID { "thresh", 1 }, "Thresh", R (0.0f, 0.3f, 0.001f), 0.02f, pct));
    p.push_back (std::make_unique<P> (juce::ParameterID { "cutoff", 1 }, "Cutoff", cutR, 900.0f, hz));
    p.push_back (std::make_unique<P> (juce::ParameterID { "envamt", 1 }, "Env Amt", envR, 3500.0f, hz));
    p.push_back (std::make_unique<P> (juce::ParameterID { "reso", 1 }, "Reso", R (0.0f, 0.95f), 0.4f, pct));
    p.push_back (std::make_unique<P> (juce::ParameterID { "drive", 1 }, "Drive", R (1.0f, 20.0f, 0.01f, 0.5f), 2.0f, unit ("x", 1)));
    p.push_back (std::make_unique<P> (juce::ParameterID { "crush", 1 }, "Crush", R (0.0f, 1.0f), 0.0f, pct));
    p.push_back (std::make_unique<P> (juce::ParameterID { "ring", 1 }, "Ring", R (0.0f, 1.0f), 0.0f, pct));
    p.push_back (std::make_unique<P> (juce::ParameterID { "ringhz", 1 }, "Ring Hz", ringR, 220.0f, hz));
    p.push_back (std::make_unique<P> (juce::ParameterID { "verb", 1 }, "Verb", R (0.0f, 1.0f), 0.25f, pct));
    p.push_back (std::make_unique<P> (juce::ParameterID { "vgate", 1 }, "Verb Gate", gateR, 250.0f, unit ("ms", 0)));
    p.push_back (std::make_unique<P> (juce::ParameterID { "mix", 1 }, "Mix", R (0.0f, 1.0f), 0.9f, pct));
    R atkR (0.5f, 5000.0f, 0.1f);  atkR.setSkewForCentre (60.0f);
    R decR (1.0f, 5000.0f, 0.1f);  decR.setSkewForCentre (300.0f);
    R relR (5.0f, 8000.0f, 0.1f);  relR.setSkewForCentre (400.0f);
    auto ms = A().withLabel ("ms").withStringFromValueFunction ([] (float v, int)
        { return v >= 1000.0f ? juce::String (v / 1000.0f, 2) + " s" : juce::String (v, v < 10.0f ? 1 : 0) + " ms"; });
    p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "adsr", 1 }, "Envelope", false));
    p.push_back (std::make_unique<P> (juce::ParameterID { "attack", 1 }, "Attack", atkR, 5.0f, ms));
    p.push_back (std::make_unique<P> (juce::ParameterID { "decay", 1 }, "Decay", decR, 300.0f, ms));
    p.push_back (std::make_unique<P> (juce::ParameterID { "sustain", 1 }, "Sustain", R (0.0f, 1.0f), 0.7f, pct));
    p.push_back (std::make_unique<P> (juce::ParameterID { "release", 1 }, "Release", relR, 400.0f, ms));
    R gRelR (5.0f, 1000.0f, 0.1f);  gRelR.setSkewForCentre (100.0f);
    auto db = A().withLabel ("dB").withStringFromValueFunction ([] (float v, int) { return juce::String (v, 0) + " dB"; });
    p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "pregate", 1 }, "Pre Gate", false));
    p.push_back (std::make_unique<P> (juce::ParameterID { "pre_thresh", 1 }, "Pre Thresh", R (-80.0f, 0.0f, 0.1f), -50.0f, db));
    p.push_back (std::make_unique<P> (juce::ParameterID { "pre_release", 1 }, "Pre Release", gRelR, 60.0f, ms));
    p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "postgate", 1 }, "Post Gate", false));
    p.push_back (std::make_unique<P> (juce::ParameterID { "post_thresh", 1 }, "Post Thresh", R (-80.0f, 0.0f, 0.1f), -55.0f, db));
    p.push_back (std::make_unique<P> (juce::ParameterID { "post_release", 1 }, "Post Release", gRelR, 120.0f, ms));
    p.push_back (std::make_unique<P> (juce::ParameterID { "output", 1 }, "Output", R (-24.0f, 12.0f, 0.01f), 0.0f, unit ("dB", 1)));

    return { p.begin(), p.end() };
}

bool VoltstringProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    auto in = layouts.getMainInputChannelSet();
    return in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
}

void VoltstringProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;

    // pitch analysis at ~22-24 kHz
    decim = juce::jmax (1, (int) std::round (sr / 22050.0));
    anaRate = sr / decim;
    tauMin = (int) (anaRate / 1500.0);
    tauMax = (int) (anaRate / 55.0);          // down to A1 (covers drop tunings)
    yinWMax = (int) (anaRate * 0.020);        // long window for low notes
    yinWMin = (int) (anaRate * 0.005);        // short window floor for high notes
    yinW = yinWMax;
    hop = 64;                                 // re-analyse every ~3 ms
    anaBuf.assign ((size_t) (yinWMax + tauMax + 2), 0.0f);
    yinD.assign ((size_t) (tauMax + 2), 0.0f);
    preCoef = coefFromMs (1000.0f / (float) (twoPi * 4000.0), sr);   // ~4 kHz anti-alias
    preCoef2 = coefFromMs (1000.0f / (float) (twoPi * 1300.0), sr);  // tame upper harmonics (fewer octave errors)
    pre1 = pre2 = pre3 = 0.0f;
    decimCount = hopCount = 0;
    trackedMidi = pendingMidi = -1.0f;
    pendingCount = 0;
    lastPeriod = 0.0;
    staleNote = snapPitch = true;
    silenceSamples = 0;
    validGain = 0.0f;

    phase = {};
    ringPhase = 0.0;
    syncSlave = fmMod = ens2 = vibPhase = pwmPhase = 0.0;
    for (int i = 0; i < 2; ++i) vIc1[i] = vIc2[i] = 0.0f;
    bIc1 = bIc2 = brassLp1 = brassLp2 = 0.0f;
    colourCount = 0;
    rmsState = env = gate = 0.0f;
    ic1 = ic2 = 0.0f;
    filterCount = 0;
    crushHold = crushCounter = 0.0f;

    for (auto& g : voiceGain) g.reset (sr, 0.04);
    lastEngine = -1;

    for (auto* s : { &mixS, &outS, &driveS, &verbS }) s->reset (sr, 0.03);
    mixS.setCurrentAndTargetValue (apvts.getRawParameterValue ("mix")->load());
    outS.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (apvts.getRawParameterValue ("output")->load()));
    driveS.setCurrentAndTargetValue (apvts.getRawParameterValue ("drive")->load());
    verbS.setCurrentAndTargetValue (apvts.getRawParameterValue ("verb")->load());

    preGate.reset();
    postGate.reset();
    preGainBuf.assign ((size_t) juce::jmax (1, samplesPerBlock), 1.0f);

    adsr.setSampleRate (sr);
    adsr.reset();
    adsrNoteOn = adsrWasOn = false;
    slowLvl = 0.0f;
    retrigHold = 0;

    reverb.setSampleRate (sr);
    juce::Reverb::Parameters rp;
    rp.roomSize = 0.82f; rp.damping = 0.35f; rp.wetLevel = 0.5f; rp.dryLevel = 0.0f; rp.width = 1.0f; rp.freezeMode = 0.0f;
    reverb.setParameters (rp);
    reverb.reset();

    wetBuf.setSize (2, juce::jmax (1, samplesPerBlock), false, true, true);
    gateBuf.assign ((size_t) juce::jmax (1, samplesPerBlock), 0.0f);
    synthMono.assign ((size_t) juce::jmax (1, samplesPerBlock), 0.0f);
}

float VoltstringProcessor::processPitchInput (float x)
{
    // two one-pole lowpasses as an anti-alias filter, then decimate
    pre1 += preCoef * (x - pre1);
    pre2 += preCoef * (pre1 - pre2);
    pre3 += preCoef2 * (pre2 - pre3);
    if (++decimCount < decim) return trackedMidi;
    decimCount = 0;

    const int N = (int) anaBuf.size();
    if (hopCount == 0)
        std::memmove (anaBuf.data(), anaBuf.data() + hop, sizeof (float) * (size_t) (N - hop));
    anaBuf[(size_t) (N - hop + hopCount)] = pre3;
    if (++hopCount >= hop)
    {
        hopCount = 0;
        analyse();
    }
    return trackedMidi;
}

// YIN on the NEWEST W samples (compared against older ones), so a new note is heard as soon as possible.
bool VoltstringProcessor::runYin (int W, double& period, float& confidence)
{
    const int N = (int) anaBuf.size();
    const int maxTau = juce::jmin (tauMax, N - W - 1);
    const float* y = anaBuf.data() + (N - W);

    double energy = 0.0;
    for (int j = 0; j < W; ++j) energy += (double) y[j] * y[j];
    if (energy < 1.0e-7 * W) return false;

    yinD[0] = 1.0f;
    double running = 0.0;
    for (int tau = 1; tau <= maxTau; ++tau)
    {
        double d = 0.0;
        const float* z = y - tau;
        for (int j = 0; j < W; ++j)
        {
            const float diff = y[j] - z[j];
            d += (double) diff * diff;
        }
        running += d;
        yinD[(size_t) tau] = running > 0.0 ? (float) (d * tau / running) : 1.0f;
    }

    // first dip under the threshold (the shortest plausible period)
    int best = -1;
    for (int tau = juce::jmax (2, tauMin); tau < maxTau; ++tau)
    {
        if (yinD[(size_t) tau] < 0.12f)
        {
            while (tau + 1 < maxTau && yinD[(size_t) tau + 1] < yinD[(size_t) tau]) ++tau;
            best = tau;
            break;
        }
    }
    if (best < 0) return false;

    const float a = yinD[(size_t) best - 1], b = yinD[(size_t) best], c = yinD[(size_t) best + 1];
    const float denom = a - 2.0f * b + c;
    const float shift = std::abs (denom) > 1.0e-9f ? 0.5f * (a - c) / denom : 0.0f;
    period = best + juce::jlimit (-1.0f, 1.0f, shift);
    confidence = b;
    return true;
}

void VoltstringProcessor::analyse()
{
    double period = 0.0;
    float conf = 1.0f;
    bool ok = false;

    // fast path: a short window sized to the current note (high notes respond very quickly)
    if (lastPeriod > 0.0)
    {
        const int W = juce::jlimit (yinWMin, yinWMax, (int) (lastPeriod * 2.2));
        ok = runYin (W, period, conf) && period < W * 0.8;
    }
    // fall back to the long window (low notes, or a big jump down)
    if (! ok)
        ok = runYin (yinWMax, period, conf);
    if (! ok) return;   // not confident: keep the last note

    const double f = anaRate / period;
    if (f < 50.0 || f > 1600.0) return;
    lastPeriod = period;
    acceptPitch ((float) (69.0 + 12.0 * std::log2 (f / 440.0)));
}

// Octave jumps must be confirmed by consecutive analyses; any other note change is taken at once.
void VoltstringProcessor::acceptPitch (float midi)
{
    if (trackedMidi < 0.0f || staleNote)
    {
        trackedMidi = midi;
        pendingCount = 0;
        staleNote = false;
        return;
    }
    const float jump = std::abs (midi - trackedMidi);
    const bool octaveJump = std::abs (jump - 12.0f) < 0.7f || std::abs (jump - 24.0f) < 0.7f;
    if (! octaveJump)
    {
        trackedMidi = midi;
        pendingCount = 0;
        return;
    }
    if (pendingCount > 0 && std::abs (midi - pendingMidi) < 0.7f)
    {
        if (++pendingCount >= 3) { trackedMidi = midi; pendingCount = 0; }
    }
    else
    {
        pendingMidi = midi;
        pendingCount = 1;
    }
}

void VoltstringProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int inCh = getTotalNumInputChannels();
    const int outCh = getTotalNumOutputChannels();
    if (n == 0) return;

    if (wetBuf.getNumSamples() < n || (int) synthMono.size() < n)
    {
        wetBuf.setSize (2, n, false, true, true);
        gateBuf.assign ((size_t) n, 0.0f);
        synthMono.assign ((size_t) n, 0.0f);
    }
    if ((int) preGainBuf.size() < n) preGainBuf.assign ((size_t) n, 1.0f);

    // mono input for a stereo output: mirror it
    if (inCh == 1 && outCh == 2) buffer.copyFrom (1, 0, buffer, 0, 0, n);

    auto param = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    const int engine = juce::jlimit (0, numEngines - 1, (int) param ("engine"));
    const auto& V = voicings[engine];
    if (engine != lastEngine)
    {
        for (int k = 0; k < numVoices; ++k)
        {
            if (lastEngine < 0) voiceGain[(size_t) k].setCurrentAndTargetValue (V.g[k]);
            else                voiceGain[(size_t) k].setTargetValue (V.g[k]);
        }
        lastEngine = engine;
    }

    const float detRatio = std::exp2 (param ("detune") / 1200.0f);
    const float transpose = param ("transpose");
    const float glideCoef = coefFromMs (param ("glide"), sr);
    const float sens = param ("sens"), thresh = param ("thresh");
    const float cutoff = param ("cutoff"), envAmt = param ("envamt");
    const float k = 2.0f - 2.0f * param ("reso");
    const float crush = param ("crush"), ring = param ("ring");
    const double ringInc = param ("ringhz") / sr;
    const float atk = coefFromMs (V.atkMs, sr), rel = coefFromMs (V.relMs, sr);
    const float rmsCoef = coefFromMs (5.0f, sr);
    const int silenceLimit = (int) (0.04 * sr);
    const float validCoef = coefFromMs (1.5f, sr);
    const float gAtk = coefFromMs (1.0f, sr), gRel = coefFromMs (param ("vgate"), sr);
    const float crushHoldLen = 1.0f + crush * crush * 20.0f;
    const float crushSteps = std::exp2 (15.0f - crush * 12.0f);

    const bool adsrOn = param ("adsr") > 0.5f;
    if (adsrOn != adsrWasOn) { adsr.reset(); adsrNoteOn = false; adsrWasOn = adsrOn; }
    adsr.setParameters ({ param ("attack") * 0.001f, param ("decay") * 0.001f,
                          param ("sustain"), param ("release") * 0.001f });
    const float slowCoef = coefFromMs (60.0f, sr);
    const int retrigSamples = (int) (0.06 * sr);

    // gates
    const bool preOn = param ("pregate") > 0.5f, postOn = param ("postgate") > 0.5f;
    const float preThr = juce::Decibels::decibelsToGain (param ("pre_thresh"));
    const float postThr = juce::Decibels::decibelsToGain (param ("post_thresh"));
    const float hyst = juce::Decibels::decibelsToGain (-4.0f);           // close 4 dB below open
    const float gateDetRel = std::exp (-1.0f / (0.010f * (float) sr));    // 10 ms peak detector
    const float gateAtk = coefFromMs (0.5f, sr);
    const float preRel = coefFromMs (param ("pre_release"), sr), postRel = coefFromMs (param ("post_release"), sr);
    const int gateHold = (int) (0.02 * sr);

    mixS.setTargetValue (param ("mix"));
    outS.setTargetValue (juce::Decibels::decibelsToGain (param ("output")));
    driveS.setTargetValue (param ("drive"));
    verbS.setTargetValue (param ("verb"));

    const double ensRatio = std::exp2 (juce::jmax (8.0f, param ("detune")) * 2.0f / 1200.0f);

    const float* inL = buffer.getReadPointer (0);
    const float* inR = buffer.getReadPointer (outCh > 1 ? 1 : 0);

    float prePeak = 0.0f, postPeak = 0.0f;
    for (int i = 0; i < n; ++i)
    {
        float mono = 0.5f * (inL[i] + inR[i]);
        prePeak = juce::jmax (prePeak, std::abs (inL[i]), std::abs (inR[i]));

        // pre gate on the guitar input (affects tracking, dynamics and the dry signal)
        float preG = 1.0f;
        if (preOn)
            preG = preGate.process (juce::jmax (std::abs (inL[i]), std::abs (inR[i])), preThr, preThr * hyst,
                                    gateDetRel, gateAtk, preRel, gateHold);
        preGainBuf[(size_t) i] = preG;
        mono *= preG;

        // pitch
        const float tracked = processPitchInput (mono);
        if (tracked > 0.0f && ! staleNote)
        {
            if (snapPitch) { curMidi = tracked + transpose; snapPitch = false; }   // new note: no swoop from the last one
            else           curMidi += glideCoef * (tracked + transpose - curMidi);
        }

        // dynamics
        rmsState += rmsCoef * (mono * mono - rmsState);
        const float lvl = juce::jlimit (0.0f, 1.0f, std::sqrt (rmsState) * sens - thresh);
        env += (lvl > env ? atk : rel) * (lvl - env);

        // after ~40 ms of silence the next note is a fresh note: hold the synth until its pitch is known
        if (lvl <= 0.0f && env < 0.01f)
        {
            if (++silenceSamples > silenceLimit && ! staleNote) { staleNote = true; snapPitch = true; }
        }
        else silenceSamples = 0;
        validGain += validCoef * ((staleNote ? 0.0f : 1.0f) - validGain);

        // amplitude: either the picking follower, or the ADSR triggered by picking
        float amp = env;
        if (adsrOn)
        {
            const float raw = std::sqrt (rmsState) * sens;
            slowLvl += slowCoef * (raw - slowLvl);
            if (retrigHold > 0) --retrigHold;
            if (! adsrNoteOn && raw > thresh + 0.04f)
            {
                adsr.noteOn(); adsrNoteOn = true; retrigHold = retrigSamples;
            }
            else if (adsrNoteOn && retrigHold == 0 && raw > slowLvl * 1.8f + 0.03f)
            {
                adsr.noteOn(); retrigHold = retrigSamples;          // new pick: retrigger
            }
            else if (adsrNoteOn && raw < thresh + 0.015f)
            {
                adsr.noteOff(); adsrNoteOn = false;
            }
            amp = adsr.getNextSample();
        }

        amp *= validGain;
        const float gTarget = amp > 0.02f ? 1.0f : 0.0f;
        gate += (gTarget > gate ? gAtk : gRel) * (gTarget - gate);

        // oscillators
        vibPhase += V.vibHz / sr; if (vibPhase >= 1.0) vibPhase -= 1.0;
        pwmPhase += 0.6 / sr;     if (pwmPhase >= 1.0) pwmPhase -= 1.0;
        const double vib = V.vibCents > 0.0f ? std::exp2 (V.vibCents * amp / 1200.0 * std::sin (twoPi * vibPhase)) : 1.0;
        const double f = 440.0 * std::exp2 ((curMidi - 69.0) / 12.0) * vib;
        const double dt0 = juce::jmin (0.45, f / sr);

        if (colourCount-- <= 0)
        {
            colourCount = 15;
            svfSet (vA1[0], vA2[0], vA3[0], 0.15f, 300.0 + 430.0 * amp, sr);      // vowel F1: oo -> ah
            svfSet (vA1[1], vA2[1], vA3[1], 0.12f, 870.0 + 400.0 * amp, sr);      // vowel F2
            svfSet (bA1, bA2, bA3, 0.18f, f, sr);                                  // breath, tuned to the note
            brassCoef = coefFromMs (1000.0f / (float) (twoPi * juce::jmin (sr * 0.45, f * (1.2 + 10.0 * amp * V.brass))), sr);
        }

        float osc = 0.0f;
        for (int v = 0; v < numVoices; ++v)
        {
            const float g = voiceGain[(size_t) v].getNextValue();
            double& p = phase[(size_t) v];
            double dt = dt0;
            switch (v)
            {
                case 1:  dt = juce::jmin (0.45, f * detRatio / sr); break;
                case 2:  dt = f / detRatio / sr; break;
                case 4: case 5: dt = dt0 * 0.5; break;
                case 6:  dt = juce::jmin (0.45, dt0 * 1.4983070768766815); break;
                case 7:  dt = juce::jmin (0.45, dt0 * 2.0); break;
                case 15: dt = juce::jmin (0.45, f * ensRatio / sr); break;
                default: break;
            }

            if (g > 1.0e-4f)
            {
                float sm = 0.0f;
                switch (v)
                {
                    case 0: case 1: case 2: case 6: sm = sawOsc (p, dt); break;
                    case 3: case 5:   sm = squareOsc (p, dt); break;
                    case 4: case 14:  sm = sineOsc (p); break;
                    case 7: case 17:  sm = triOsc (p); break;
                    case 8:  sm = pulseOsc (p, dt, 0.5 + V.pwm * 0.45 * std::sin (twoPi * pwmPhase)); break;
                    case 9:  sm = (float) (2.0 * syncSlave - 1.0); break;                     // hard sync
                    case 10:                                                                      // FM
                    {
                        const double idx = V.fmIndex * (0.3 + 0.7 * amp);
                        sm = (float) std::sin (twoPi * p + idx * std::sin (twoPi * fmMod));
                        break;
                    }
                    case 11: sm = (float) std::sin (std::sin (twoPi * p) * (1.0 + 5.0 * amp)); break;   // wavefolder
                    case 12:                                                                      // vowel
                    {
                        const float src = sawOsc (p, dt);
                        sm = 2.5f * (svfBand (src, vIc1[0], vIc2[0], vA1[0], vA2[0], vA3[0], 0.15f)
                                   + 0.7f * svfBand (src, vIc1[1], vIc2[1], vA1[1], vA2[1], vA3[1], 0.12f));
                        break;
                    }
                    case 13:                                                                      // breath
                    {
                        const float w = noise.nextFloat() * 2.0f - 1.0f;
                        sm = 2.2f * svfBand (w, bIc1, bIc2, bA1, bA2, bA3, 0.18f) + 0.08f * w;
                        break;
                    }
                    case 15: sm = 0.6f * (sawOsc (p, dt) + sawOsc (ens2, juce::jmin (0.45, f / ensRatio / sr))); break;
                    case 16: sm = pulseOsc (p, dt, V.narrowW); break;
                    default: break;
                }
                osc += g * sm;
            }

            // advance phases (sync slave, FM modulator and 2nd ensemble saw ride along)
            if (v == 9)
            {
                syncSlave += dt * (1.0 + (V.syncMax - 1.0) * amp);
                if (syncSlave >= 1.0) syncSlave -= std::floor (syncSlave);
            }
            if (v == 10) { fmMod += dt * V.fmRatio; if (fmMod >= 1.0) fmMod -= std::floor (fmMod); }
            if (v == 15) { ens2 += juce::jmin (0.45, f / ensRatio / sr); if (ens2 >= 1.0) ens2 -= 1.0; }
            p += dt;
            if (p >= 1.0)
            {
                p -= 1.0;
                if (v == 9) syncSlave = p * (1.0 + (V.syncMax - 1.0) * amp);   // master wrapped: reset slave
            }
        }

        // brass / reed bloom: brighter the harder you play
        if (V.brass > 0.0f)
        {
            brassLp1 += brassCoef * (osc - brassLp1);
            brassLp2 += brassCoef * (brassLp1 - brassLp2);
            osc = osc * (1.0f - V.brass) + brassLp2 * V.brass * 1.4f;
        }
        float y = osc * 0.4f;

        // envelope filter (TPT state-variable lowpass), coefficients every 16 samples
        if (filterCount-- <= 0)
        {
            filterCount = 15;
            const double fc = juce::jlimit (30.0, sr * 0.45, (double) (cutoff + amp * envAmt));
            const float g = (float) std::tan (juce::MathConstants<double>::pi * fc / sr);
            fA1 = 1.0f / (1.0f + g * (g + k));
            fA2 = g * fA1;
            fA3 = g * fA2;
        }
        const float v3 = y - ic2;
        const float v1 = fA1 * ic1 + fA2 * v3;
        const float v2 = ic2 + fA2 * ic1 + fA3 * v3;
        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;
        y = v2 * amp;

        // drive
        y = std::tanh (y * driveS.getNextValue()) * 0.7f;

        // bitcrush: sample-and-hold + quantise
        if (crush > 0.001f)
        {
            crushCounter += 1.0f;
            if (crushCounter >= crushHoldLen)
            {
                crushCounter -= crushHoldLen;
                crushHold = std::round (y * crushSteps) / crushSteps;
            }
            y = crushHold;
        }

        // ring mod
        if (ring > 0.001f)
            y = y * (1.0f - ring) + y * (float) std::sin (twoPi * ringPhase) * ring;
        ringPhase += ringInc;
        if (ringPhase >= 1.0) ringPhase -= 1.0;

        synthMono[(size_t) i] = y;
        lastAmp = amp;
        gateBuf[(size_t) i] = gate;
        wetBuf.setSample (0, i, y);
        wetBuf.setSample (1, i, y);
    }

    reverb.processStereo (wetBuf.getWritePointer (0), wetBuf.getWritePointer (1), n);

    for (int i = 0; i < n; ++i)
    {
        const float mix = mixS.getNextValue(), outG = outS.getNextValue();
        const float vAmt = verbS.getNextValue() * gateBuf[(size_t) i];
        float outs[2] = { 0.0f, 0.0f };
        float peak = 0.0f;
        for (int c = 0; c < outCh; ++c)
        {
            const float verb = wetBuf.getSample (c, i) * vAmt;
            const float wet = synthMono[(size_t) i] + verb;
            const float dry = buffer.getSample (c, i) * preGainBuf[(size_t) i];
            outs[c] = (dry * (1.0f - mix) + wet * mix) * outG;
            peak = juce::jmax (peak, std::abs (outs[c]));
        }
        postPeak = juce::jmax (postPeak, peak);
        // post gate on the final output
        const float postG = postOn ? postGate.process (peak, postThr, postThr * hyst, gateDetRel, gateAtk, postRel, gateHold)
                                   : 1.0f;
        for (int c = 0; c < outCh; ++c)
            buffer.setSample (c, i, outs[c] * postG);
    }

    displayNote.store (lastAmp > 0.02f && trackedMidi > 0.0f ? trackedMidi + transpose : -1.0f);
    displayLevel.store (lastAmp);
    preMeterDb.store (juce::Decibels::gainToDecibels (prePeak, -90.0f));
    postMeterDb.store (juce::Decibels::gainToDecibels (postPeak, -90.0f));
    preGateOpen.store (! preOn || preGate.open);
    postGateOpen.store (! postOn || postGate.open);
}

int VoltstringProcessor::getNumPrograms() { return numVoltstringPresets; }

const juce::String VoltstringProcessor::getProgramName (int index)
{
    if (! juce::isPositiveAndBelow (index, numVoltstringPresets)) return {};
    return juce::String (voltstringPresets[index].category) + " - " + voltstringPresets[index].name;
}

void VoltstringProcessor::setCurrentProgram (int index)
{
    if (! juce::isPositiveAndBelow (index, numVoltstringPresets)) return;
    currentPreset = index;
    const auto& pr = voltstringPresets[index];
    for (int i = 0; i < numPresetParams; ++i)
        if (auto* p = apvts.getParameter (presetParamIds[i]))
            p->setValueNotifyingHost (p->convertTo0to1 (pr.v[i]));

    // envelope: presets without one switch it off and leave the ADSR knobs where they are
    static const char* envIds[] = { "adsr", "attack", "decay", "sustain", "release" };
    const int envCount = pr.e[0] > 0.5f ? 5 : 1;
    for (int i = 0; i < envCount; ++i)
        if (auto* p = apvts.getParameter (envIds[i]))
            p->setValueNotifyingHost (p->convertTo0to1 (pr.e[i]));
    apvts.state.setProperty ("preset", index, nullptr);
    updateHostDisplay (ChangeDetails().withProgramChanged (true));
}

void VoltstringProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void VoltstringProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
            currentPreset = juce::jlimit (0, numVoltstringPresets - 1, (int) apvts.state.getProperty ("preset", 0));
        }
}

juce::AudioProcessorEditor* VoltstringProcessor::createEditor() { return new VoltstringEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new VoltstringProcessor(); }
