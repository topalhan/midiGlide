# MidiGlide 🎵🎛️

**Expressive Real-Time MIDI Portamento Glide & Diagnostic Inspector for VST3 and Standalone.**

[![Release](https://img.shields.io/github/v/release/topalhan/midiGlide?style=flat-square)](https://github.com/topalhan/midiGlide/releases)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg?style=flat-square)](LICENSE)
[![Format](https://img.shields.io/badge/Format-VST3%20%7C%20Standalone-purple.svg?style=flat-square)](#)
[![JUCE 8](https://img.shields.io/badge/Framework-JUCE%208-orange.svg?style=flat-square)](https://juce.com)

---

## Overview

**MidiGlide** is an open-source real-time MIDI processor engineered for synthesizers, samplers, wind controllers (Akai EWI, Roland Aerophone, Sylphyo), and keyboard controllers. 

Many software instruments lack dedicated monophonic portamento or exhibit unnatural jumps when playing legato. MidiGlide intercepts incoming MIDI note streams and injects high-resolution pitch bend curves or discrete chromatic transitions before passing them to your target instrument.

In addition to its expressive glide engine, MidiGlide features dual real-time packet inspectors (**MIDI In** and **MIDI Out**) for latency-free monitoring and debugging of MIDI traffic.

---

## Key Features

### 🎚️ 1. Dual Portamento Engines
- **Analogue Continuous Pitch Bend (14-Bit)**:
  - Generates smooth, micro-stepped 14-bit pitch wheel movements ($0 - 16383$).
  - Target note is pre-bent before trigger, ensuring attack envelopes ring out at the starting pitch without voice dropouts.
  - Selectable glide response curves:
    - **Analogue RC Exponential**: Warm, natural curve that starts fast and relaxes into the target note.
    - **Linear**: Constant pitch glide rate.
- **Stepped Glissando (Discrete Notes)**:
  - Steps through intermediate scale degrees (Chromatic 1-semitone, Whole Tone 2-semitones, or Minor 3rd).
  - Built-in **Legato Overlap**: Uses an 8–12 ms note overlap window and softened intermediate velocities to eliminate voice-stealing pops and envelope clicks.

### ⏱️ 2. Intelligent Phrase Pause Detection
- **Configurable Phrase Boundary (50 ms – 2000 ms)**:
  - When playing non-legato or staccato lines, pauses longer than the threshold trigger fresh notes from silence with zero glide.
  - Eliminates unwanted glides across musical pauses or breaks.
  - Automatically disabled when **Legato Only** mode is active.

### 💨 3. Expressive Controller Support
- **MIDI CC #65 (Portamento Switch)**: Dynamically toggle portamento on/off from foot pedals, buttons, or bite sensors.
- **MIDI CC #5 (Portamento Time)**: Dynamically scale glide speed in real time.
- **RPN 0,0 Sender**: One-click broadcast of MIDI Registered Parameter Number 0,0 (Pitch Bend Sensitivity) across all 16 MIDI channels to match synth pitch bend ranges (2, 12, 24, 48 semitones).

### 🇹🇷 4. Turkish Makam Microtuning Engine (Pitchbend Microtonality)
- **10 Major Turkish Makams**:
  - **1. Rast**: Segâh (3rd degree -1 koma) & Eviç (7th degree -1 koma).
  - **2. Uşşak**: Characteristic poignant Segâh (2nd degree ~2–3 komas flat).
  - **3. Hicaz**: Augmented 2nd with Dik Kürdî (B♭ +1 koma) and Nîm Hicâz (C♯ -1 koma).
  - **4. Hüseyni**: Pastoral folk favorite with Segâh & ascending Eviç.
  - **5. Saba**: Mystical dawn mode with flattened Saba 4th degree (~ -35¢) and Segâh.
  - **6. Segâh**: Sacred contemplative mode where Tonic itself is microtonal (B -1 koma).
  - **7. Karcığar**: Uşşak on Dügâh + Hicaz on Nevâ with Hisâr (E♭ / E -45¢).
  - **8. Nihavend**: Lyrical minor with subtle Holdrian intonation.
  - **9. Kürdî**: Modern pop and classical mode with B♭ Kürdî bakiye flat (+9¢).
  - **10. Bûselik**: Pure Pythagorean 5ths minor scale.
- **Dual Tuning Frameworks**:
  - **Authentic Performance (İcra / Sahne)**: Real-world ear-tuned comma values used by virtuoso masters.
  - **53-Holdrian Comma (Arel-Ezgi Nazariyat)**: Strict Holdrian comma mathematics ($1\text{ koma} \approx 22.64\text{ cents}$).
  - **Custom User Tuning**: Interactive per-note sliders (-100¢ to +100¢) with Holdrian comma indicators.
- **Transposable Tonic (Karar Sesi)**: Select any root key (C through B) with instant scale transposition and traditional default Karar reset.
- **Flawless Portamento Integration**: Glides seamlessly compute start and target microtonal offsets, ensuring smooth portamento transitions between microtuned notes.
- **Audition Player & Live Gauge**: Includes Karar, Microtone, and Scale audition buttons, plus a live bipolar pitch deflection meter.

### 🌬️ 5. Breath Controller to Modulation Control (EWI & Wind Synth Bridge)
- **Direct CC Remapping & Mirroring**:
  - Automatically translates incoming **Breath Data (MIDI CC #2)** or **Channel Aftertouch** into **Modulation Wheel (MIDI CC #1)** for softsynths and VSTs that only support modulation control.
  - **Routing Modes**:
    - **Remap / Replace**: Replaces incoming breath with modulation control (swallowing CC #2 so synths don't receive unwanted CC2).
    - **Mirror / Duplicate**: Emits CC #1 while simultaneously passing through the original CC #2 data.
- **Wind Instrument Breath Shaping & Calibration**:
  - **4 Response Curves**: `Linear (1:1)`, `Exponential (Expressive / Soft)`, `Logarithmic (Punchy / Instant Attack)`, and `S-Curve (Sigmoid / Acoustic Reed)`.
  - **Lip Rest Deadzone (0–30)**: Eliminates mouth resting drift and lip noise on EWI breath sensors.
  - **Sensitivity / Gain (0.50x – 2.00x)**: Dial in dynamic expression reach.
  - **Min / Max Output Clamping (0–127)**: Limit modulation depth to prevent harsh filter opening.
  - **Jitter Filter (0–85% EMA)**: Exponential moving average filter for breath sensor stability.
  - **Inversion Toggle**: Invert output direction for ducking or reverse sweep patches.
- **Visual Deck & Test Simulator**:
  - Real-time dual LED meters: **Incoming Breath (CC #2)** and **Outgoing Modulation (CC #1)**.
  - Interactive transfer function curve with an active real-time tracking dot.
  - Built-in **Test Breath Blow (Pulse 0 -> 127 -> 0)** button for testing sound response without blowing.

### 🔍 6. Real-Time MIDI Diagnostics & Scope
- **Dual Inspectors (MIDI In / MIDI Out)**:
  - Millisecond-accurate timestamps and musical bar/beat positions.
  - Distinguishes unmodified events (`PASS-THRU`) from transformed/injected events (`[PORTA BEND]`, `[MAKAM TUNE]`, `[PORTA TARGET]`, `[PORTA STEP]`, `[PORTA OFF]`).
  - Raw Hex byte readout, event origin tags, and search/freeze controls.
- **Master Panic Button**:
  - Instantly issues All-Notes-Off and All-Sound-Off across all 16 MIDI channels with pitch wheel centering.

---

## UI Preview

```
┌────────────────────────────────────────────────────────────────────────┐
│  MIDI GLIDE v0.1          In: 1,420 | Out: 1,845          [ PANIC ]    │
├────────────────────────────────────────────────────────────────────────┤
│  [ Portamento Glide ]  [ Turkish Makam ]  [ Breath & CC Remap ]  [ MIDI In ]  [ MIDI Out ] │
├────────────────────────────────────────────────────────────────────────┤
│  [x] Enable Turkish Makam    Makam: [ 1. Rast Makamı (Rast)         ]  │
│  Karar (Root): [ G ] [ Default ]   Tuning: [ Authentic Performance  ]  │
│  Pitch Bend Range: [ +/- 12 Semitones ]   [ Send RPN (0,0) ]           │
├────────────────────────────────────────────────────────────────────────┤
│  Rast Makamı (Rast) • Karar: G | Güçlü: Nevâ (D) | Seyir: Çıkıcı       │
│  [ C ] [ C#] [ D ] [ D#] [ E ] [ F ] [ F#] [ G ] [ G#] [ A ] [ A#] [ B ]│
│  Rast        Nevâ        Hüseynî     Eviç  Rast        Dügâh     Segâh │
│   0¢   0¢     0¢    0¢    0¢    0¢   -24¢   0¢    0¢    0¢    0¢   -24¢ │
│  0.0k 0.0k   0.0k  0.0k  0.0k  0.0k  -1.1k 0.0k  0.0k  0.0k  0.0k -1.1k│
├────────────────────────────────────────────────────────────────────────┤
│  [ Play Karar ] [ Play Microtone ] [ Play Makam Scale ] [ Notes Off ]   │
│  LIVE: Last Note: B3 | Offset: -24.0¢ (-1.06 koma) | Pitch Bend: 8028  │
│  [--------------------------|====                                    ] │
└────────────────────────────────────────────────────────────────────────┘
```

---

## Installation

### Windows (VST3)
1. Download `MidiGlide-v0.1-Windows-VST3.zip` from [Releases](https://github.com/topalhan/midiGlide/releases).
2. Extract `MidiGlide.vst3` into your VST3 folder:
   - User: `%LOCALAPPDATA%\Programs\Common\VST3\`
   - System: `C:\Program Files\Common Files\VST3\`
3. Alternatively, run the included `install.bat`.

### DAW Setup Guide

#### **Cubase / Nuendo**
Because MidiGlide transforms MIDI and outputs MIDI to downstream synths, it is registered as an **Instrument (Synth)** plugin with MIDI In and MIDI Out:
1. Create an Instrument track with **MidiGlide**.
2. Set your MIDI controller (keyboard, EWI, etc.) as the input to the MidiGlide track.
3. On your sound-generating synth track (e.g. Vital, Surge, HALion, Kontakt):
   - Set the track MIDI input to **MidiGlide - MIDI Out**.
   - Enable Monitor (speaker icon) or Record Enable.

#### **Reaper**
1. Add **MidiGlide** as the first plugin in your FX chain.
2. Add your synth plugin immediately below MidiGlide in the same FX chain.
3. Incoming MIDI will flow through MidiGlide into the synth.

---

## Building from Source

### Prerequisites
- **CMake** 3.22 or newer
- **C++20** compatible compiler (MSVC 2022, Clang 14+, or GCC 12+)
- **Git**

### Clone & Build
```bash
git clone https://github.com/topalhan/midiGlide.git
cd midiGlide

# Configure CMake (JUCE 8 will be automatically resolved)
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build VST3 and Standalone targets
cmake --build build --config Release --parallel
```

Artifacts will be generated in `build/MidiGlide_artefacts/Release/`:
- `VST3/MidiGlide.vst3`
- `Standalone/MidiGlide.exe`

---

## Architecture & Code Map

- [`Source/PortamentoProcessor.h`](Source/PortamentoProcessor.h): Core DSP engine handling MIDI stream transformation, pitch bend generation, and pause gap detection. Allocation-free and lock-free for audio-thread safety.
- [`Source/MicrotuningProcessor.h`](Source/MicrotuningProcessor.h): Turkish Makam microtonality engine storing scale intervals for 10 major makams, 53-Holdrian & authentic performance tunings, and calculating real-time 14-bit pitch bend offsets.
- [`Source/MicrotuningTab.h`](Source/MicrotuningTab.h) / [`.cpp`](Source/MicrotuningTab.cpp): Dedicated Turkish Makam microtuning tab featuring interactive 12-key keyboard strips, per-key cents/comma adjustments, theory banner, audition playback, and live pitch deflection gauge.
- [`Source/PortamentoTab.h`](Source/PortamentoTab.h) / [`.cpp`](Source/PortamentoTab.cpp): Glide configuration UI, bipolar pitch bend meter, and live animation display.
- [`Source/MidiMonitorTab.h`](Source/MidiMonitorTab.h) / [`.cpp`](Source/MidiMonitorTab.cpp): Real-time MIDI packet table inspector for MIDI In and Out.
- [`Source/MidiLogQueue.h`](Source/MidiLogQueue.h): Lock-free ring buffer queue decoupling the real-time audio thread from the GUI renderer.
- [`Source/PluginProcessor.h`](Source/PluginProcessor.h) / [`.cpp`](Source/PluginProcessor.cpp): JUCE AudioProcessor managing routing, buses, and lifecycles.
- [`Source/PluginEditor.h`](Source/PluginEditor.h) / [`.cpp`](Source/PluginEditor.cpp): Main editor component with dark mode aesthetic.

---

## License

Released under the [MIT License](LICENSE).
Created by **topalhan**. Contributions, issues, and feature requests are welcome!
