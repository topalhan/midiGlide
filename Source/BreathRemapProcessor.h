#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <array>
#include <atomic>
#include <cmath>
#include <algorithm>

namespace MidiMonitor
{

enum class BreathCurveProfile : int
{
    Linear = 1,
    Exponential = 2,
    Logarithmic = 3,
    Sigmoid = 4
};

enum class BreathRoutingMode : int
{
    RemapReplace = 1,  // Replace incoming breath with modulation (original CC swallowed)
    MirrorDuplicate = 2 // Emit modulation AND keep original breath CC
};

struct BreathRemapSettings
{
    std::atomic<bool> enabled { true };             // Master enable toggle
    std::atomic<int> sourceController { 2 };         // Default: CC #2 (Breath Controller), -1 for Channel Aftertouch
    std::atomic<int> targetController { 1 };         // Default: CC #1 (Modulation Wheel)
    std::atomic<int> routingMode { 1 };              // 1 = Remap/Replace, 2 = Mirror/Duplicate
    std::atomic<int> curveProfile { 1 };            // 1 = Linear, 2 = Exponential, 3 = Logarithmic, 4 = Sigmoid
    std::atomic<int> deadzone { 0 };                 // 0 to 30: Lip rest deadzone
    std::atomic<float> gain { 1.0f };                // 0.50x to 2.00x sensitivity
    std::atomic<int> minOutput { 0 };                // 0 to 127
    std::atomic<int> maxOutput { 127 };              // 0 to 127
    std::atomic<bool> invert { false };              // Invert curve response
    std::atomic<float> smoothing { 0.0f };           // 0.0 to 0.85 EMA jitter filter
};

struct BreathRemapLiveState
{
    std::atomic<int> lastInValue { 0 };             // 0 to 127
    std::atomic<int> lastOutValue { 0 };            // 0 to 127
    std::atomic<uint32_t> totalRemappedCount { 0 };
    std::atomic<int> lastChannel { 1 };
    std::atomic<bool> isActive { false };
};

class BreathRemapProcessor
{
public:
    BreathRemapProcessor() = default;

    void prepareToPlay (double /*sampleRate*/)
    {
        reset();
    }

    void reset()
    {
        smoothedOutput = 0.0f;
        liveState.lastInValue.store (0, std::memory_order_relaxed);
        liveState.lastOutValue.store (0, std::memory_order_relaxed);
        liveState.isActive.store (false, std::memory_order_relaxed);
    }

    BreathRemapSettings& getSettings() noexcept { return settings; }
    const BreathRemapSettings& getSettings() const noexcept { return settings; }

    const BreathRemapLiveState& getLiveState() const noexcept { return liveState; }

    bool isMatch (const juce::MidiMessage& msg) const noexcept
    {
        if (!settings.enabled.load (std::memory_order_relaxed))
            return false;

        int src = settings.sourceController.load (std::memory_order_relaxed);
        if (src == -1)
            return msg.isChannelPressure();

        return msg.isController() && msg.getControllerNumber() == src;
    }

    int getSourceController() const noexcept
    {
        return settings.sourceController.load (std::memory_order_relaxed);
    }

    int getTargetController() const noexcept
    {
        return settings.targetController.load (std::memory_order_relaxed);
    }

    int getRoutingMode() const noexcept
    {
        return settings.routingMode.load (std::memory_order_relaxed);
    }

    // Process matching MIDI message and return transformed 0-127 output value
    int processMidiEvent (const juce::MidiMessage& msg) noexcept
    {
        int rawVal = 0;
        int ch = msg.getChannel();

        if (msg.isChannelPressure())
            rawVal = msg.getChannelPressureValue();
        else if (msg.isController())
            rawVal = msg.getControllerValue();

        rawVal = std::clamp (rawVal, 0, 127);
        liveState.lastInValue.store (rawVal, std::memory_order_relaxed);
        liveState.lastChannel.store (ch, std::memory_order_relaxed);

        int outVal = calculateTransformedValue (rawVal);

        liveState.lastOutValue.store (outVal, std::memory_order_relaxed);
        liveState.totalRemappedCount.fetch_add (1, std::memory_order_relaxed);
        liveState.isActive.store (rawVal > settings.deadzone.load (std::memory_order_relaxed), std::memory_order_relaxed);

        return outVal;
    }

    // Pure calculation without mutating audio-thread filter state (useful for UI curve plotting)
    float evaluateTransferFunction (float normalizedInput) const noexcept
    {
        float dead = static_cast<float>(settings.deadzone.load (std::memory_order_relaxed)) / 127.0f;
        float x = 0.0f;

        if (normalizedInput > dead)
        {
            if (dead < 0.999f)
                x = (normalizedInput - dead) / (1.0f - dead);
            else
                x = 1.0f;
        }

        x = std::clamp (x, 0.0f, 1.0f);

        // Gain
        float g = settings.gain.load (std::memory_order_relaxed);
        x = std::clamp (x * g, 0.0f, 1.0f);

        // Curve profile
        int curve = settings.curveProfile.load (std::memory_order_relaxed);
        float y = x;

        switch (static_cast<BreathCurveProfile>(curve))
        {
            case BreathCurveProfile::Exponential:
                y = std::pow (x, 2.2f);
                break;
            case BreathCurveProfile::Logarithmic:
                y = std::pow (x, 0.45f);
                break;
            case BreathCurveProfile::Sigmoid:
                y = x * x * (3.0f - 2.0f * x); // Smoothstep S-curve
                break;
            case BreathCurveProfile::Linear:
            default:
                y = x;
                break;
        }

        // Invert
        if (settings.invert.load (std::memory_order_relaxed))
            y = 1.0f - y;

        // Output scaling
        float minV = static_cast<float>(settings.minOutput.load (std::memory_order_relaxed)) / 127.0f;
        float maxV = static_cast<float>(settings.maxOutput.load (std::memory_order_relaxed)) / 127.0f;

        float outNorm = minV + y * (maxV - minV);
        return std::clamp (outNorm, 0.0f, 1.0f);
    }

private:
    int calculateTransformedValue (int rawValue) noexcept
    {
        float normIn = static_cast<float>(rawValue) / 127.0f;
        float targetNorm = evaluateTransferFunction (normIn);
        float targetVal = targetNorm * 127.0f;

        // Jitter / smoothing filter (EMA)
        float smoothCoeff = std::clamp (settings.smoothing.load (std::memory_order_relaxed), 0.0f, 0.85f);
        if (smoothCoeff > 0.001f)
        {
            smoothedOutput = (smoothedOutput * smoothCoeff) + (targetVal * (1.0f - smoothCoeff));
        }
        else
        {
            smoothedOutput = targetVal;
        }

        return std::clamp (static_cast<int>(std::round (smoothedOutput)), 0, 127);
    }

    BreathRemapSettings settings;
    BreathRemapLiveState liveState;
    float smoothedOutput = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BreathRemapProcessor)
};

} // namespace MidiMonitor
