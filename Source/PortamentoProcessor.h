#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include "MidiLogQueue.h"
#include "MicrotuningProcessor.h"
#include "BreathRemapProcessor.h"
#include <array>
#include <atomic>
#include <vector>
#include <cmath>
#include <algorithm>

namespace MidiMonitor
{

enum class GlideMode : int
{
    AnaloguePitchBend = 1,
    SteppedGlissando  = 2
};

enum class GlideCurve : int
{
    ExponentialRC = 1,
    Linear        = 2
};

struct PortamentoSettings
{
    std::atomic<bool> enabled { true };             // Enabled by default
    std::atomic<int> glideMode { 1 };               // 1 = Analogue Pitch Bend, 2 = Stepped Glissando
    std::atomic<int> pitchBendRange { 12 };         // Semitones (e.g. 12, 24, 2, 48)
    std::atomic<int> glideCurve { 1 };             // 1 = Exponential RC, 2 = Linear
    std::atomic<bool> requireCC65 { false };         // False = always glide without needing CC65 pedal
    std::atomic<bool> legatoOnly { false };          // False = glide between any consecutive notes
    std::atomic<float> baseGlideTimeMs { 250.0f };   // Base time in ms
    std::atomic<bool> useCC5Time { false };          // False = use base slider directly
    std::atomic<int> stepInterval { 1 };             // For stepped mode: 1 = chromatic
    std::atomic<float> maxStaccatoGapMs { 350.0f };  // Max gap in ms between released note and new note to allow glide
};

struct PortamentoLiveState
{
    std::atomic<bool> isGliding { false };
    std::atomic<int> sourceNote { -1 };
    std::atomic<int> targetNote { -1 };
    std::atomic<int> currentStepNote { -1 };
    std::atomic<float> progress { 0.0f };           // 0.0 to 1.0
    std::atomic<int> currentPitchBend { 0 };        // -8192 to 8191
    std::atomic<bool> cc65Active { false };          // CC65 status
    std::atomic<int> cc5Value { 64 };               // CC5 value
    std::atomic<uint32_t> totalInjectedNotes { 0 }; // Injected notes or bends
};

class PortamentoProcessor
{
public:
    PortamentoProcessor() = default;

    void prepareToPlay (double sampleRate)
    {
        currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
        reset();
    }

    void reset()
    {
        physicallyHeldNotes.fill (false);
        numPhysicallyHeld = 0;
        soundingNote = -1;
        lastNoteNumber = -1;
        activeSourceNote = -1;
        activeTargetNote = -1;
        isGliding = false;

        // Stepped mode countdown
        stepCountdownSamples = 0.0;
        samplesPerStep = 0.0;
        stepDirection = 0;
        pendingNoteOffNote = -1;
        pendingNoteOffCountdown = 0;

        // Analogue pitch bend state
        glideElapsedSamples = 0.0;
        glideTotalSamples = 1.0;
        soundingBaseNote = -1;
        samplesSinceLastBend = 0;

        currentVelocity = 100;
        currentChannel = 1;

        lastReleaseSampleIndex = 0;
        hasReleasedNote = false;

        liveState.isGliding.store (false);
        liveState.sourceNote.store (-1);
        liveState.targetNote.store (-1);
        liveState.currentStepNote.store (-1);
        liveState.progress.store (0.0f);
        liveState.currentPitchBend.store (0);

        initialGlideOffsetSemitones = 0.0;
        targetOffsetSemitones = 0.0;
    }

    void setMicrotuningProcessor (MicrotuningProcessor* mt) noexcept { microtuningProcessor = mt; }
    MicrotuningProcessor* getMicrotuningProcessor() noexcept { return microtuningProcessor; }

    void setBreathRemapProcessor (BreathRemapProcessor* b) noexcept { breathRemapProcessor = b; }
    BreathRemapProcessor* getBreathRemapProcessor() noexcept { return breathRemapProcessor; }

    PortamentoSettings& getSettings() noexcept { return settings; }
    const PortamentoSettings& getSettings() const noexcept { return settings; }

    const PortamentoLiveState& getLiveState() const noexcept { return liveState; }

    void processBlock (juce::MidiBuffer& midiMessages,
                       int numSamples,
                       uint64_t baseSampleIndex,
                       const juce::AudioPlayHead::PositionInfo* posInfo,
                       MidiLogQueue& inLogQueue,
                       MidiLogQueue& outLogQueue)
    {
        scratchBuffer.clear();

        // 1. Process all incoming MIDI events in this block
        for (const auto metadata : midiMessages)
        {
            auto msg = metadata.getMessage();
            int offset = metadata.samplePosition;
            double timeSec = static_cast<double>(baseSampleIndex + static_cast<uint64_t>(offset)) / currentSampleRate;

            // Log incoming raw event to MIDI IN queue
            auto inEntry = MidiLogEntry::fromMidiMessage (msg, offset, timeSec, posInfo, false, "INPUT");
            inLogQueue.push (inEntry);

            // Track CC 65 (Portamento Switch)
            if (msg.isController() && msg.getControllerNumber() == 65)
            {
                bool active = msg.getControllerValue() >= 64;
                cc65Switch = active;
                liveState.cc65Active.store (active, std::memory_order_relaxed);
                addOutputEvent (msg, offset, timeSec, posInfo, outLogQueue, false, "PASS-THRU");
                continue;
            }

            // Track CC 5 (Portamento Time)
            if (msg.isController() && msg.getControllerNumber() == 5)
            {
                currentCC5 = msg.getControllerValue();
                liveState.cc5Value.store (currentCC5, std::memory_order_relaxed);
                addOutputEvent (msg, offset, timeSec, posInfo, outLogQueue, false, "PASS-THRU");
                continue;
            }

            // Check Breath / CC Remapper (e.g. Breath CC 2 -> Modulation CC 1)
            if (breathRemapProcessor != nullptr && breathRemapProcessor->isMatch (msg))
            {
                int outVal = breathRemapProcessor->processMidiEvent (msg);
                int targetCC = breathRemapProcessor->getTargetController();
                int ch = msg.getChannel();
                int mode = breathRemapProcessor->getRoutingMode();

                if (mode == static_cast<int>(BreathRoutingMode::MirrorDuplicate))
                {
                    addOutputEvent (msg, offset, timeSec, posInfo, outLogQueue, false, "PASS-THRU");
                }

                auto mappedMsg = juce::MidiMessage::controllerEvent (ch, targetCC, static_cast<uint8_t>(outVal));
                addOutputEvent (mappedMsg, offset, timeSec, posInfo, outLogQueue, true, "[BREATH->MOD]");
                continue;
            }

            // Note Off handling
            if (msg.isNoteOff())
            {
                int note = msg.getNoteNumber();
                if (note >= 0 && note < 128)
                {
                    if (physicallyHeldNotes[static_cast<size_t>(note)])
                    {
                        physicallyHeldNotes[static_cast<size_t>(note)] = false;
                        if (numPhysicallyHeld > 0) --numPhysicallyHeld;
                    }
                }

                if (numPhysicallyHeld == 0)
                {
                    lastReleaseSampleIndex = baseSampleIndex + static_cast<uint64_t>(offset);
                    hasReleasedNote = true;
                }

                if (!settings.enabled.load (std::memory_order_relaxed))
                {
                    if (soundingNote == note) soundingNote = -1;
                    addOutputEvent (msg, offset, timeSec, posInfo, outLogQueue, false, "PASS-THRU");
                    continue;
                }

                if (isGliding)
                {
                    // Only stop glide if the target note is released or no keys remain held
                    if (note == activeTargetNote || numPhysicallyHeld == 0)
                    {
                        cancelActiveGlide (offset, timeSec, posInfo, outLogQueue);
                    }
                    // Releasing source note during a legato transition does NOT stop the glide!
                }
                else
                {
                    if (note == soundingNote || numPhysicallyHeld == 0)
                    {
                        if (soundingNote >= 0)
                        {
                            addOutputEvent (juce::MidiMessage::noteOff (currentChannel, soundingNote, static_cast<uint8_t>(msg.getVelocity())),
                                            offset, timeSec, posInfo, outLogQueue, false, "PASS-THRU");
                            soundingNote = -1;
                        }
                    }
                    else
                    {
                        addOutputEvent (msg, offset, timeSec, posInfo, outLogQueue, false, "PASS-THRU");
                    }
                }
                continue;
            }

            // Note On handling
            if (msg.isNoteOn())
            {
                int note = msg.getNoteNumber();
                int vel = msg.getVelocity();
                int ch = msg.getChannel();

                // Treat Note On with velocity 0 as Note Off
                if (vel == 0)
                {
                    if (note >= 0 && note < 128 && physicallyHeldNotes[static_cast<size_t>(note)])
                    {
                        physicallyHeldNotes[static_cast<size_t>(note)] = false;
                        if (numPhysicallyHeld > 0) --numPhysicallyHeld;
                    }
                    if (numPhysicallyHeld == 0)
                    {
                        lastReleaseSampleIndex = baseSampleIndex + static_cast<uint64_t>(offset);
                        hasReleasedNote = true;
                    }
                    if (note == activeTargetNote || numPhysicallyHeld == 0)
                    {
                        cancelActiveGlide (offset, timeSec, posInfo, outLogQueue);
                    }
                    else
                    {
                        addOutputEvent (juce::MidiMessage::noteOff (ch, note, (uint8_t)0),
                                        offset, timeSec, posInfo, outLogQueue, false, "PASS-THRU");
                    }
                    continue;
                }

                bool wasLegato = (numPhysicallyHeld > 0);
                uint64_t currentSampleIndex = baseSampleIndex + static_cast<uint64_t>(offset);

                if (note >= 0 && note < 128)
                {
                    physicallyHeldNotes[static_cast<size_t>(note)] = true;
                    ++numPhysicallyHeld;
                }

                bool isPortaEnabled = settings.enabled.load (std::memory_order_relaxed);
                bool require65 = settings.requireCC65.load (std::memory_order_relaxed);
                bool portaActive = isPortaEnabled && (!require65 || cc65Switch);
                bool onlyLegato = settings.legatoOnly.load (std::memory_order_relaxed);

                // Compute gap in milliseconds since the last note was physically released
                double gapMs = 1000000.0;
                if (hasReleasedNote && currentSampleIndex >= lastReleaseSampleIndex)
                {
                    gapMs = (static_cast<double>(currentSampleIndex - lastReleaseSampleIndex) / currentSampleRate) * 1000.0;
                }
                float maxGapMs = settings.maxStaccatoGapMs.load (std::memory_order_relaxed);
                bool withinGap = hasReleasedNote && (gapMs <= static_cast<double>(maxGapMs));

                // Only allow gliding from a previously released note if played within maxGapMs!
                // Any pause longer than maxGapMs is treated as a fresh new phrase without glide.
                int fromNote = (soundingNote >= 0) ? soundingNote : ((!onlyLegato && withinGap) ? lastNoteNumber : -1);
                bool canGlide = portaActive && (fromNote >= 0) && (fromNote != note) && (onlyLegato ? wasLegato : true);

                if (canGlide)
                {
                    // Start portamento transition
                    activeSourceNote = fromNote;
                    activeTargetNote = note;
                    currentVelocity = vel;
                    currentChannel = ch;

                    // Calculate glide duration in milliseconds
                    float baseTimeMs = settings.baseGlideTimeMs.load (std::memory_order_relaxed);
                    if (settings.useCC5Time.load (std::memory_order_relaxed))
                    {
                        float cc5Factor = std::max (0.05f, static_cast<float>(currentCC5) / 64.0f);
                        baseTimeMs *= cc5Factor;
                    }
                    baseTimeMs = std::clamp (baseTimeMs, 10.0f, 4000.0f);

                    glideTotalSamples = (static_cast<double>(baseTimeMs) / 1000.0) * currentSampleRate;
                    glideTotalSamples = std::max (32.0, glideTotalSamples);
                    glideElapsedSamples = 0.0;
                    samplesSinceLastBend = 0;

                    int mode = settings.glideMode.load (std::memory_order_relaxed);

                    if (mode == static_cast<int>(GlideMode::AnaloguePitchBend))
                    {
                        int pbRange = std::max (1, settings.pitchBendRange.load (std::memory_order_relaxed));
                        int totalSemitones = activeTargetNote - activeSourceNote;

                        // 1. If previous sounding note is still active and different, turn it off cleanly
                        if (soundingNote >= 0 && soundingNote != activeTargetNote)
                        {
                            addOutputEvent (juce::MidiMessage::noteOff (currentChannel, soundingNote, (uint8_t)0),
                                            offset, timeSec, posInfo, outLogQueue, true, "[PORTA OFF]");
                        }

                        // 2. Calculate initial pitch bend offset: target note starts bent so it sounds activeSourceNote's pitch
                        double sourceMicro = (microtuningProcessor != nullptr && microtuningProcessor->getSettings().enabled.load (std::memory_order_relaxed))
                                           ? microtuningProcessor->getSemitonesOffsetForMidiNote (activeSourceNote)
                                           : 0.0;
                        double targetMicro = (microtuningProcessor != nullptr && microtuningProcessor->getSettings().enabled.load (std::memory_order_relaxed))
                                           ? microtuningProcessor->getSemitonesOffsetForMidiNote (activeTargetNote)
                                           : 0.0;

                        targetOffsetSemitones = targetMicro;
                        initialGlideOffsetSemitones = -static_cast<double>(totalSemitones) + sourceMicro;
                        initialGlideOffsetSemitones = std::clamp (initialGlideOffsetSemitones, -static_cast<double>(pbRange), static_cast<double>(pbRange));

                        double bendFraction = initialGlideOffsetSemitones / static_cast<double>(pbRange);
                        int initialPitchWheel = 8192 + static_cast<int>(std::round (bendFraction * 8191.0));
                        initialPitchWheel = std::clamp (initialPitchWheel, 0, 16383);

                        // 3. Send initial pitch bend BEFORE Note On so the synth triggers at the starting pitch
                        addOutputEvent (juce::MidiMessage::pitchWheel (currentChannel, initialPitchWheel),
                                        offset, timeSec, posInfo, outLogQueue, true, "[PORTA BEND]");

                        // 4. Immediately trigger target note! The synth is now sounding activeSourceNote's pitch with active voice!
                        addOutputEvent (juce::MidiMessage::noteOn (currentChannel, activeTargetNote, static_cast<uint8_t>(currentVelocity)),
                                        offset, timeSec, posInfo, outLogQueue, true, "[PORTA TARGET]");

                        soundingNote = activeTargetNote;
                        soundingBaseNote = activeTargetNote;
                        lastNoteNumber = activeTargetNote;
                        isGliding = true;

                        liveState.isGliding.store (true, std::memory_order_relaxed);
                        liveState.sourceNote.store (activeSourceNote, std::memory_order_relaxed);
                        liveState.targetNote.store (activeTargetNote, std::memory_order_relaxed);
                        liveState.currentStepNote.store (activeSourceNote, std::memory_order_relaxed);
                        liveState.progress.store (0.0f, std::memory_order_relaxed);
                        liveState.currentPitchBend.store (initialPitchWheel - 8192, std::memory_order_relaxed);

                        continue;
                    }
                    else // SteppedGlissando
                    {
                        int interval = std::max (1, settings.stepInterval.load (std::memory_order_relaxed));
                        int totalSemitones = std::abs (activeTargetNote - activeSourceNote);
                        int totalSteps = std::max (1, (totalSemitones + interval - 1) / interval);
                        stepDirection = (activeTargetNote > activeSourceNote) ? interval : -interval;
                        samplesPerStep = glideTotalSamples / static_cast<double>(totalSteps);
                        stepCountdownSamples = samplesPerStep;

                        // Center pitch bend
                        addOutputEvent (juce::MidiMessage::pitchWheel (currentChannel, 8192),
                                        offset, timeSec, posInfo, outLogQueue, true, "[PORTA RESET]");

                        // Calculate first step note
                        int firstStepNote = activeSourceNote + stepDirection;
                        if ((stepDirection > 0 && firstStepNote >= activeTargetNote) ||
                            (stepDirection < 0 && firstStepNote <= activeTargetNote))
                        {
                            firstStepNote = activeTargetNote;
                            isGliding = false;
                        }
                        else
                        {
                            isGliding = true;
                        }

                        // Softer velocity on intermediate passing step to prevent loud attack transient clicks
                        uint8_t stepVel = isGliding
                            ? static_cast<uint8_t>(std::clamp (static_cast<int>(currentVelocity * 0.65f), 35, 75))
                            : static_cast<uint8_t>(currentVelocity);

                        // 1. Turn ON first step note FIRST (Legato hand-off)
                        addOutputEvent (juce::MidiMessage::noteOn (currentChannel, firstStepNote, stepVel),
                                        offset, timeSec, posInfo, outLogQueue, true, isGliding ? "[PORTA STEP]" : "[PORTA TARGET]");

                        // 2. Schedule previous sounding note to turn off after short legato overlap (~8-12ms)
                        if (soundingNote >= 0 && soundingNote != firstStepNote)
                        {
                            pendingNoteOffNote = soundingNote;
                            pendingNoteOffCountdown = std::clamp (static_cast<int>(samplesPerStep * 0.35), 64, static_cast<int>(0.015 * currentSampleRate));
                        }

                        soundingNote = firstStepNote;
                        lastNoteNumber = firstStepNote;

                        liveState.isGliding.store (isGliding, std::memory_order_relaxed);
                        liveState.sourceNote.store (activeSourceNote, std::memory_order_relaxed);
                        liveState.targetNote.store (activeTargetNote, std::memory_order_relaxed);
                        liveState.currentStepNote.store (firstStepNote, std::memory_order_relaxed);
                        liveState.progress.store (0.0f, std::memory_order_relaxed);
                        liveState.currentPitchBend.store (0, std::memory_order_relaxed);
                        liveState.totalInjectedNotes.fetch_add (1, std::memory_order_relaxed);

                        continue;
                    }
                }

                // If not gliding, turn off previous note and play target note
                if (soundingNote >= 0 && soundingNote != note)
                {
                    addOutputEvent (juce::MidiMessage::noteOff (currentChannel, soundingNote, (uint8_t)0),
                                    offset, timeSec, posInfo, outLogQueue, false, "PASS-THRU");
                }

                // Calculate target pitch bend with microtuning
                int targetPitchWheel = 8192;
                bool hasTuning = false;
                if (microtuningProcessor != nullptr && microtuningProcessor->getSettings().enabled.load (std::memory_order_relaxed))
                {
                    targetPitchWheel = microtuningProcessor->calculatePitchBendForMidiNote (note);
                    float cents = microtuningProcessor->getCentsForMidiNote (note);
                    microtuningProcessor->recordNoteTuned (note, cents, targetPitchWheel);
                    hasTuning = (targetPitchWheel != 8192);
                }

                // Set pitch bend for target microtuned note before note triggers
                addOutputEvent (juce::MidiMessage::pitchWheel (ch, targetPitchWheel),
                                offset, timeSec, posInfo, outLogQueue, hasTuning,
                                hasTuning ? "[MAKAM TUNE]" : "PASS-THRU");

                soundingNote = note;
                soundingBaseNote = note;
                lastNoteNumber = note;
                activeSourceNote = note;
                currentVelocity = vel;
                currentChannel = ch;
                isGliding = false;
                hasReleasedNote = false;

                liveState.isGliding.store (false, std::memory_order_relaxed);
                liveState.sourceNote.store (note, std::memory_order_relaxed);
                liveState.targetNote.store (note, std::memory_order_relaxed);
                liveState.currentStepNote.store (note, std::memory_order_relaxed);
                liveState.progress.store (1.0f, std::memory_order_relaxed);
                liveState.currentPitchBend.store (targetPitchWheel - 8192, std::memory_order_relaxed);

                addOutputEvent (msg, offset, timeSec, posInfo, outLogQueue, false, "PASS-THRU");
                continue;
            }

            // All other MIDI messages pass through untouched
            addOutputEvent (msg, offset, timeSec, posInfo, outLogQueue, false, "PASS-THRU");
        }

        // 2. Process in-flight portamento advancement in this audio block
        if ((isGliding && activeTargetNote >= 0) || pendingNoteOffNote >= 0)
        {
            int mode = settings.glideMode.load (std::memory_order_relaxed);

            if (mode == static_cast<int>(GlideMode::AnaloguePitchBend))
            {
                processAnalogueGlide (numSamples, baseSampleIndex, posInfo, outLogQueue);
            }
            else
            {
                processSteppedGlissando (numSamples, baseSampleIndex, posInfo, outLogQueue);
            }
        }

        midiMessages.swapWith (scratchBuffer);
    }

private:
    void addOutputEvent (const juce::MidiMessage& msg, int offset, double timeSec,
                         const juce::AudioPlayHead::PositionInfo* posInfo,
                         MidiLogQueue& outLogQueue, bool isInjected, const char* originTag)
    {
        scratchBuffer.addEvent (msg, offset);
        auto entry = MidiLogEntry::fromMidiMessage (msg, offset, timeSec, posInfo, isInjected, originTag);
        outLogQueue.push (entry);
    }

    void cancelActiveGlide (int offset, double timeSec,
                            const juce::AudioPlayHead::PositionInfo* posInfo,
                            MidiLogQueue& outLogQueue)
    {
        if (pendingNoteOffNote >= 0)
        {
            auto offMsg = juce::MidiMessage::noteOff (currentChannel, pendingNoteOffNote, (uint8_t)0);
            addOutputEvent (offMsg, offset, timeSec, posInfo, outLogQueue, true, "[PORTA OFF]");
            pendingNoteOffNote = -1;
            pendingNoteOffCountdown = 0;
        }

        if (soundingNote >= 0 || soundingBaseNote >= 0)
        {
            int noteToStop = (soundingBaseNote >= 0) ? soundingBaseNote : soundingNote;
            auto offMsg = juce::MidiMessage::noteOff (currentChannel, noteToStop, (uint8_t)64);
            addOutputEvent (offMsg, offset, timeSec, posInfo, outLogQueue, true, "[PORTA OFF]");

            // Reset pitch bend wheel back to center
            auto pbMsg = juce::MidiMessage::pitchWheel (currentChannel, 8192);
            addOutputEvent (pbMsg, offset, timeSec, posInfo, outLogQueue, true, "[PORTA RESET]");

            soundingNote = -1;
            soundingBaseNote = -1;
        }

        isGliding = false;
        activeSourceNote = -1;
        activeTargetNote = -1;
        liveState.isGliding.store (false, std::memory_order_relaxed);
        liveState.sourceNote.store (-1, std::memory_order_relaxed);
        liveState.targetNote.store (-1, std::memory_order_relaxed);
        liveState.currentStepNote.store (-1, std::memory_order_relaxed);
        liveState.progress.store (0.0f, std::memory_order_relaxed);
        liveState.currentPitchBend.store (0, std::memory_order_relaxed);
    }

    void processAnalogueGlide (int numSamples, uint64_t baseSampleIndex,
                               const juce::AudioPlayHead::PositionInfo* posInfo,
                               MidiLogQueue& outLogQueue)
    {
        int pbRange = std::max (1, settings.pitchBendRange.load (std::memory_order_relaxed));
        int curveType = settings.glideCurve.load (std::memory_order_relaxed);

        const int bendIntervalSamples = 32; // ~1.3 kHz update rate at 44.1kHz

        for (int s = 0; s < numSamples && isGliding; ++s)
        {
            glideElapsedSamples += 1.0;
            ++samplesSinceLastBend;

            double u = std::clamp (glideElapsedSamples / glideTotalSamples, 0.0, 1.0);

            // Shape progress curve
            double t = u;
            if (curveType == static_cast<int>(GlideCurve::ExponentialRC))
            {
                constexpr double k = 4.2;
                t = (1.0 - std::exp (-k * u)) / (1.0 - std::exp (-k));
            }

            // Offset glides from initialGlideOffsetSemitones smoothly toward targetOffsetSemitones
            double currentOffsetSemitones = initialGlideOffsetSemitones * (1.0 - t) + targetOffsetSemitones * t;

            // Emit pitch bend message periodically or on destination
            if (samplesSinceLastBend >= bendIntervalSamples || u >= 1.0)
            {
                samplesSinceLastBend = 0;

                double bendFraction = currentOffsetSemitones / static_cast<double>(pbRange);
                bendFraction = std::clamp (bendFraction, -1.0, 1.0);

                int pitchWheelValue = 8192 + static_cast<int>(std::round (bendFraction * 8191.0));
                pitchWheelValue = std::clamp (pitchWheelValue, 0, 16383);

                double timeSec = static_cast<double>(baseSampleIndex + static_cast<uint64_t>(s)) / currentSampleRate;
                addOutputEvent (juce::MidiMessage::pitchWheel (currentChannel, pitchWheelValue),
                                s, timeSec, posInfo, outLogQueue, true, "[PORTA BEND]");

                int signedBend = pitchWheelValue - 8192;
                liveState.currentPitchBend.store (signedBend, std::memory_order_relaxed);
                liveState.progress.store (static_cast<float>(t), std::memory_order_relaxed);
                liveState.totalInjectedNotes.fetch_add (1, std::memory_order_relaxed);

                int estimatedNote = static_cast<int>(std::round (static_cast<double>(activeTargetNote) + currentOffsetSemitones));
                liveState.currentStepNote.store (estimatedNote, std::memory_order_relaxed);
            }

            // Destination reached!
            if (u >= 1.0)
            {
                double timeSec = static_cast<double>(baseSampleIndex + static_cast<uint64_t>(s)) / currentSampleRate;

                double bendFraction = targetOffsetSemitones / static_cast<double>(pbRange);
                bendFraction = std::clamp (bendFraction, -1.0, 1.0);
                int finalPitchWheel = 8192 + static_cast<int>(std::round (bendFraction * 8191.0));
                finalPitchWheel = std::clamp (finalPitchWheel, 0, 16383);

                bool hasTuning = (finalPitchWheel != 8192);
                addOutputEvent (juce::MidiMessage::pitchWheel (currentChannel, finalPitchWheel),
                                s, timeSec, posInfo, outLogQueue, hasTuning,
                                hasTuning ? "[MAKAM TUNE]" : "[PORTA RESET]");

                if (microtuningProcessor != nullptr && microtuningProcessor->getSettings().enabled.load (std::memory_order_relaxed))
                {
                    float c = microtuningProcessor->getCentsForMidiNote (activeTargetNote);
                    microtuningProcessor->recordNoteTuned (activeTargetNote, c, finalPitchWheel);
                }

                isGliding = false;

                liveState.isGliding.store (false, std::memory_order_relaxed);
                liveState.progress.store (1.0f, std::memory_order_relaxed);
                liveState.currentPitchBend.store (finalPitchWheel - 8192, std::memory_order_relaxed);
                liveState.currentStepNote.store (activeTargetNote, std::memory_order_relaxed);

                break;
            }
        }
    }

    void processSteppedGlissando (int numSamples, uint64_t baseSampleIndex,
                                  const juce::AudioPlayHead::PositionInfo* posInfo,
                                  MidiLogQueue& outLogQueue)
    {
        for (int s = 0; s < numSamples; ++s)
        {
            // 1. Check if a pending legato Note-Off timer has expired
            if (pendingNoteOffNote >= 0)
            {
                if (--pendingNoteOffCountdown <= 0)
                {
                    double timeSec = static_cast<double>(baseSampleIndex + static_cast<uint64_t>(s)) / currentSampleRate;
                    auto offMsg = juce::MidiMessage::noteOff (currentChannel, pendingNoteOffNote, (uint8_t)0);
                    addOutputEvent (offMsg, s, timeSec, posInfo, outLogQueue, true, "[PORTA OFF]");
                    pendingNoteOffNote = -1;
                }
            }

            if (!isGliding)
                continue;

            stepCountdownSamples -= 1.0;
            if (stepCountdownSamples <= 0.0)
            {
                int nextNote = soundingNote + stepDirection;
                if ((stepDirection > 0 && nextNote >= activeTargetNote) ||
                    (stepDirection < 0 && nextNote <= activeTargetNote))
                {
                    nextNote = activeTargetNote;
                    isGliding = false;
                }

                double timeSec = static_cast<double>(baseSampleIndex + static_cast<uint64_t>(s)) / currentSampleRate;

                // If previous note was still pending off, flush it immediately before triggering next step
                if (pendingNoteOffNote >= 0)
                {
                    auto offMsg = juce::MidiMessage::noteOff (currentChannel, pendingNoteOffNote, (uint8_t)0);
                    addOutputEvent (offMsg, s, timeSec, posInfo, outLogQueue, true, "[PORTA OFF]");
                    pendingNoteOffNote = -1;
                }

                // Legato velocity: softer on intermediate passing steps to avoid hard transient clicks;
                // full velocity on final target note!
                uint8_t stepVel = isGliding
                    ? static_cast<uint8_t>(std::clamp (static_cast<int>(currentVelocity * 0.65f), 35, 75))
                    : static_cast<uint8_t>(currentVelocity);

                // 1. Turn ON next step note FIRST (Legato slur trigger)
                auto onMsg = juce::MidiMessage::noteOn (currentChannel, nextNote, stepVel);
                addOutputEvent (onMsg, s, timeSec, posInfo, outLogQueue, true, isGliding ? "[PORTA STEP]" : "[PORTA TARGET]");

                // 2. Schedule previous note to turn off after a short legato overlap (~8-12 ms)
                pendingNoteOffNote = soundingNote;
                int overlapSamples = std::clamp (static_cast<int>(samplesPerStep * 0.35), 64, static_cast<int>(0.015 * currentSampleRate));
                pendingNoteOffCountdown = overlapSamples;

                soundingNote = nextNote;
                lastNoteNumber = nextNote;
                liveState.currentStepNote.store (nextNote, std::memory_order_relaxed);

                int totalDist = std::abs (activeTargetNote - activeSourceNote);
                int curDist = std::abs (nextNote - activeSourceNote);
                float prog = totalDist > 0 ? std::clamp (static_cast<float>(curDist) / static_cast<float>(totalDist), 0.0f, 1.0f) : 1.0f;
                liveState.progress.store (prog, std::memory_order_relaxed);
                liveState.totalInjectedNotes.fetch_add (1, std::memory_order_relaxed);

                stepCountdownSamples = samplesPerStep;

                if (!isGliding)
                {
                    int finalPitchWheel = 8192;
                    bool hasTuning = false;
                    if (microtuningProcessor != nullptr && microtuningProcessor->getSettings().enabled.load (std::memory_order_relaxed))
                    {
                        finalPitchWheel = microtuningProcessor->calculatePitchBendForMidiNote (activeTargetNote);
                        float c = microtuningProcessor->getCentsForMidiNote (activeTargetNote);
                        microtuningProcessor->recordNoteTuned (activeTargetNote, c, finalPitchWheel);
                        hasTuning = (finalPitchWheel != 8192);
                    }
                    addOutputEvent (juce::MidiMessage::pitchWheel (currentChannel, finalPitchWheel),
                                    s, timeSec, posInfo, outLogQueue, hasTuning,
                                    hasTuning ? "[MAKAM TUNE]" : "[PORTA RESET]");

                    liveState.isGliding.store (false, std::memory_order_relaxed);
                    liveState.progress.store (1.0f, std::memory_order_relaxed);
                    liveState.currentPitchBend.store (finalPitchWheel - 8192, std::memory_order_relaxed);
                }
            }
        }
    }

    double currentSampleRate = 44100.0;
    PortamentoSettings settings;
    PortamentoLiveState liveState;

    juce::MidiBuffer scratchBuffer;

    std::array<bool, 128> physicallyHeldNotes { false };
    int numPhysicallyHeld = 0;

    int soundingNote = -1;
    int soundingBaseNote = -1;
    int lastNoteNumber = -1;
    int activeSourceNote = -1;
    int activeTargetNote = -1;
    bool isGliding = false;

    // Analogue glide timing
    double glideElapsedSamples = 0.0;
    double glideTotalSamples = 1.0;
    int samplesSinceLastBend = 0;
    double initialGlideOffsetSemitones = 0.0;
    double targetOffsetSemitones = 0.0;
    MicrotuningProcessor* microtuningProcessor = nullptr;
    BreathRemapProcessor* breathRemapProcessor = nullptr;

    // Stepped glissando timing
    double samplesPerStep = 0.0;
    double stepCountdownSamples = 0.0;
    int stepDirection = 0;
    int pendingNoteOffNote = -1;
    int pendingNoteOffCountdown = 0;
    uint64_t lastReleaseSampleIndex = 0;
    bool hasReleasedNote = false;

    int currentVelocity = 100;
    int currentChannel = 1;

    bool cc65Switch = false;
    int currentCC5 = 64;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PortamentoProcessor)
};

} // namespace MidiMonitor
