# ReChop

Vocal-chop pattern player for Cubase — VST3, by Pouria Motabean.

ReChop does not make sound. It plays **MIDI patterns** on the vocal slices/chops you already have in a sampler
(Cubase Sampler Track, Intersect, ...), locked to the project tempo and snapped to a root + scale.

## Install
Build runs on GitHub Actions (`.github/workflows/build.yml`). Download the artifact:
- `ReChop-Setup-<version>.exe` — installs into `C:\Program Files\Common Files\VST3` (replaces the old one)
- `ReChop-<version>-VST3.zip` — manual copy of `ReChop.vst3`

## Use in Cubase
1. Create an **Instrument Track** with ReChop.
2. Create a sampler track (or Intersect) with your vocal chops.
3. Route ReChop's MIDI output to that track (MIDI output of the ReChop track -> the sampler track).
4. Press play. In the sampler, set the **root key** to the pitch the vocal was sung at; in ReChop set the song's **root + scale**.
   The sampler does the pitch shifting, ReChop only decides which note is played when.

If the MIDI routing does not work in your Cubase, use the **DRAG MIDI > CUBASE** button instead: drag the pattern
(with all current settings, 2/4/8 bars) onto a MIDI track that is routed to the sampler, then edit it as normal MIDI.

## Patterns
Two kinds:
- **melodic** — written as scale degrees (1 = tonic). Changing root/scale re-pitches the whole pattern.
- **slices** — written as slice numbers; slice 1 plays the `SLICE 1 NOTE` key (default C1), slice 2 the next key, etc.
  (this is how Cubase's slicer maps slices).

Families: Dance (EDM, Electro Dance, House, Deep House), Pop (Pop, Retro Pop, Indie Pop, Dark Pop), Synthwave.

Patterns live in `Source/Generator.h` as one line of text each — one token per 16th note:

| token | meaning |
|---|---|
| `.` | rest |
| `_` | hold the previous note one more 16th |
| `1`..`7`, `8` | scale degree (8 = tonic one octave up) / slice number |
| `b5` | degree 5 one octave down |
| `5!` | accented |

Example: `1! . . 3 _ . 5 .` — a pattern is 16 tokens per bar (32 for a 2-bar pattern).

## Controls
Key root, scale, octave, density (thins the pattern, accents last), swing, humanize (timing + velocity, only ever late),
gate (note length), fill (16th stutter on the last beat of every 4th bar), export bars.

## Notes
- Time signature is assumed to be 4/4 (patterns count 16ths from project start).
- Incoming MIDI is not passed through.
- Version number is in `CMakeLists.txt` (`project(ReChop VERSION x.y.z)`) — bump it for every new build.
