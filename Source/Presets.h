#pragma once

// Factory presets. Order of values matches presetParamIds.
static const char* presetParamIds[] = { "engine", "transpose", "glide", "detune", "sens", "thresh",
                                        "cutoff", "envamt", "reso", "drive", "crush", "ring", "ringhz",
                                        "verb", "vgate", "mix", "output" };
static constexpr int numPresetParams = 17;

struct VoltstringPreset
{
    const char* category;
    const char* name;
    // engine (0 Sub,1 Analog,2 Lead,3 Pad,4 Fifths), transpose st, glide ms, detune ct, sens, thresh,
    // cutoff Hz, env amt Hz, reso, drive, crush, ring, ring Hz, verb, verb gate ms, mix, output dB
    float v[numPresetParams];
};

static const VoltstringPreset voltstringPresets[] = {
    // ---- Init ----
    { "Init",   "Init",              { 1,   0,  25, 12,  8, 0.02f,  900, 3500, 0.40f,  2.0f, 0.00f, 0.00f,  220, 0.25f,  250, 0.90f,  0.0f } },

    // ---- Bass ----
    { "Bass",   "Basement Sub",      { 0, -12,  15,  0, 10, 0.03f,  220, 1200, 0.20f,  1.5f, 0.00f, 0.00f,  220, 0.00f,  250, 1.00f,  0.0f } },
    { "Bass",   "Rubber Pluck",      { 0, -12,   5,  0, 12, 0.03f,  160, 4200, 0.65f,  2.5f, 0.00f, 0.00f,  220, 0.05f,  120, 1.00f, -1.0f } },
    { "Bass",   "Two Octaves Down",  { 0, -24,  20,  0, 10, 0.03f,  300, 1500, 0.15f,  3.0f, 0.00f, 0.00f,  220, 0.00f,  250, 0.85f, -1.0f } },
    { "Bass",   "Growl Unit",        { 1, -12,  10, 18, 12, 0.03f,  350, 2600, 0.55f,  9.0f, 0.10f, 0.00f,  220, 0.00f,  250, 1.00f, -4.0f } },
    { "Bass",   "Synthwave Octave",  { 1, -12,  30, 14, 10, 0.02f,  600, 2200, 0.35f,  2.0f, 0.00f, 0.00f,  220, 0.20f,  300, 0.90f, -1.0f } },

    // ---- Leads ----
    { "Leads",  "Neon Lead",         { 2,   0,  40,  8, 10, 0.02f, 1400, 4000, 0.45f,  3.0f, 0.00f, 0.00f,  220, 0.30f,  350, 0.90f, -2.0f } },
    { "Leads",  "Octave Scream",     { 2,  12,  25, 10, 12, 0.03f, 1800, 5000, 0.50f,  8.0f, 0.00f, 0.00f,  220, 0.20f,  250, 1.00f, -6.0f } },
    { "Leads",  "Portamento Wire",   { 2,   0, 250,  6, 10, 0.02f, 1100, 3000, 0.40f,  2.5f, 0.00f, 0.00f,  220, 0.25f,  400, 0.95f, -2.0f } },
    { "Leads",  "Square Solo",       { 0,  12,  35,  0, 10, 0.02f, 1600, 3500, 0.30f,  2.0f, 0.00f, 0.00f,  220, 0.30f,  400, 0.90f, -2.0f } },
    { "Leads",  "Fuzz Synth",        { 2,   0,  15, 15, 14, 0.03f, 2500, 4500, 0.35f, 16.0f, 0.00f, 0.00f,  220, 0.15f,  200, 1.00f, -8.0f } },
    { "Leads",  "Talk Box-ish",      { 1,   0,  30, 10, 14, 0.02f,  400, 6500, 0.80f,  3.0f, 0.00f, 0.00f,  220, 0.15f,  250, 1.00f, -3.0f } },

    // ---- Pads ----
    { "Pads",   "Swell Strings",     { 3,   0,  80, 18,  8, 0.02f, 1800, 2000, 0.20f,  1.5f, 0.00f, 0.00f,  220, 0.45f, 1200, 0.80f,  0.0f } },
    { "Pads",   "Cathedral Bow",     { 3, -12, 120, 22,  8, 0.02f, 1200, 1500, 0.15f,  1.2f, 0.00f, 0.00f,  220, 0.70f, 1500, 0.85f, -1.0f } },
    { "Pads",   "Warm Blanket",      { 3,   0,  60, 12,  8, 0.02f,  700, 1000, 0.10f,  1.5f, 0.00f, 0.00f,  220, 0.35f, 1000, 0.75f,  1.0f } },
    { "Pads",   "Shimmer Octave",    { 3,  12, 100, 16,  8, 0.02f, 3000, 2500, 0.20f,  1.2f, 0.00f, 0.00f,  220, 0.60f, 1400, 0.70f, -1.0f } },
    { "Pads",   "Ghost Choir",       { 3,   0, 150, 30,  8, 0.02f,  900, 1800, 0.55f,  1.5f, 0.00f, 0.00f,  220, 0.55f, 1300, 0.80f, -1.0f } },

    // ---- Stacks ----
    { "Stacks", "Power Fifths",      { 4,   0,  20, 10, 10, 0.03f, 1500, 3500, 0.35f,  6.0f, 0.00f, 0.00f,  220, 0.20f,  250, 0.90f, -4.0f } },
    { "Stacks", "Arena Stack",       { 4, -12,  25, 14, 10, 0.03f, 1200, 4000, 0.40f, 10.0f, 0.00f, 0.00f,  220, 0.35f,  300, 0.95f, -6.0f } },
    { "Stacks", "Brass Section",     { 4,   0,  15,  8, 12, 0.03f,  500, 5500, 0.30f,  3.0f, 0.00f, 0.00f,  220, 0.25f,  300, 1.00f, -3.0f } },
    { "Stacks", "Organ Drawbars",    { 4,  12,   5,  4,  8, 0.02f, 3500,  500, 0.05f,  1.5f, 0.00f, 0.00f,  220, 0.30f,  500, 0.85f, -2.0f } },

    // ---- FX / Lo-Fi ----
    { "FX",     "8-Bit Hero",        { 0,  12,   0,  0, 10, 0.02f, 6000, 3000, 0.10f,  1.5f, 0.75f, 0.00f,  220, 0.15f,  200, 1.00f, -3.0f } },
    { "FX",     "Broken Console",    { 2,   0,  10,  6, 10, 0.02f, 4000, 3000, 0.30f,  3.0f, 0.90f, 0.15f,  330, 0.10f,  150, 1.00f, -4.0f } },
    { "FX",     "Robot Bells",       { 2,  12,  20,  0, 10, 0.02f, 3000, 3000, 0.30f,  2.0f, 0.00f, 0.80f,  880, 0.40f,  600, 0.90f, -3.0f } },
    { "FX",     "Metal Detector",    { 1,   0,  30, 25, 10, 0.02f, 2000, 4000, 0.60f,  4.0f, 0.20f, 0.60f,  140, 0.30f,  400, 1.00f, -4.0f } },
    { "FX",     "Gated 80s Snap",    { 1,   0,  20, 14, 12, 0.03f, 1500, 5000, 0.40f,  3.0f, 0.00f, 0.00f,  220, 0.90f,  120, 0.90f, -3.0f } },
    { "FX",     "Tape Wobble Lo-Fi", { 3,   0,  90, 40,  8, 0.02f,  900, 1500, 0.20f,  2.0f, 0.45f, 0.00f,  220, 0.40f,  900, 0.80f, -1.0f } },
};

static constexpr int numVoltstringPresets = (int) (sizeof (voltstringPresets) / sizeof (voltstringPresets[0]));
