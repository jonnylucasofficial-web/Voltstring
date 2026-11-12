# Voltstring — guitar synth (AU / VST3)

Plug your guitar in and Voltstring plays a synth that follows what you play: it tracks the pitch of
single notes (bends, slides and vibrato included) and follows your picking dynamics.

| Section | Controls |
|---|---|
| **Engine** | SUB (square + two sub-octaves), ANALOG (three detuned saws), LEAD (bright, fast), PAD (slow swell, long release), FIFTHS (adds a fifth above, power-chord style) |
| **Pitch** | Transpose (±24 st), Glide, Detune (spread of the detuned saws) |
| **Dynamics** | Sens (how strongly the synth follows your playing), Thresh (ignore string noise below this level) |
| **Filter** | Cutoff, Env Amt (filter opens as you pick harder), Reso |
| **FX** | Drive, Crush (bit depth + sample rate), Ring + Ring Hz (ring modulator), Verb + Verb Gate (reverb that cuts off when you stop playing) |
| **Out** | Mix (dry guitar ↔ synth), Output |

Tips: play single notes; the neck pickup with the tone rolled back tracks best.

## Getting it

Every push builds the Mac installer on GitHub Actions. Download **Voltstring-Installer.pkg** from the
`latest` release, open it (macOS: Done → System Settings → Privacy & Security → Open Anyway), then
rescan plug-ins in Ableton. It appears under Plug-Ins → Audio Units / VST3 → Jonny Lucas → Voltstring.

## Building locally

CMake 3.22+ and Xcode (macOS) or Visual Studio 2022 (Windows). JUCE downloads automatically.

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```
