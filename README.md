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

### 🔍 4. Real-Time MIDI Diagnostics & Scope
- **Dual Inspectors (MIDI In / MIDI Out)**:
  - Millisecond-accurate timestamps and musical bar/beat positions.
  - Distinguishes unmodified events (`PASS-THRU`) from transformed/injected events (`[PORTA BEND]`, `[PORTA TARGET]`, `[PORTA STEP]`, `[PORTA OFF]`).
  - Raw Hex byte readout, event origin tags, and search/freeze controls.
- **Master Panic Button**:
  - Instantly issues All-Notes-Off and All-Sound-Off across all 16 MIDI channels with pitch wheel centering.

---

## UI Preview

```
┌────────────────────────────────────────────────────────────────────────┐
│  MIDI GLIDE v0.1          In: 1,420 | Out: 1,845          [ PANIC ]    │
├────────────────────────────────────────────────────────────────────────┤
│  [ Portamento Glide ]  [ MIDI In ]  [ MIDI Out ]                       │
├──────────────────────────────────┬─────────────────────────────────────┤
│  GLIDE CONFIGURATION             │  LIVE PORTAMENTO STATUS             │
│  [x] Enable Portamento Glide     │  Mode: Analogue 14-Bit Continuous   │
│  Glide Mode: [ Analogue Bend   ] │  Gliding: C4 (60) -> G4 (67)        │
│  PB Range:   [ +/- 12 Semitones] │  [=================>       ] 65%    │
│  Curve:      [ RC Exponential  ] │  Pitch Wheel: +4,210 (+6.2 st)      │
│  Base Time:  [ 250 ms          ] │  [-------|====             ]        │
│  Phrase Gap: [ 350 ms          ] │  CC65 Switch: ON | CC5 Time: 64     │
│  [ ] Require CC65 Portamento On  │  Total Injected Events: 425         │
│  [ ] Legato Only (Overlap)       │                                     │
│  [x] Scale Speed with CC5        │                                     │
│  [ Send Pitch Bend RPN (0,0) ]   │                                     │
│  [ Test Glide Up ] [ Test Down ] │                                     │
└──────────────────────────────────┴─────────────────────────────────────┘
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
- [`Source/PortamentoTab.h`](Source/PortamentoTab.h) / [`.cpp`](Source/PortamentoTab.cpp): Glide configuration UI, bipolar pitch bend meter, and live animation display.
- [`Source/MidiMonitorTab.h`](Source/MidiMonitorTab.h) / [`.cpp`](Source/MidiMonitorTab.cpp): Real-time MIDI packet table inspector for MIDI In and Out.
- [`Source/MidiLogQueue.h`](Source/MidiLogQueue.h): Lock-free ring buffer queue decoupling the real-time audio thread from the GUI renderer.
- [`Source/PluginProcessor.h`](Source/PluginProcessor.h) / [`.cpp`](Source/PluginProcessor.cpp): JUCE AudioProcessor managing routing, buses, and lifecycles.
- [`Source/PluginEditor.h`](Source/PluginEditor.h) / [`.cpp`](Source/PluginEditor.cpp): Main editor component with dark mode aesthetic.

---

## License

Released under the [MIT License](LICENSE).
Created by **topalhan**. Contributions, issues, and feature requests are welcome!
