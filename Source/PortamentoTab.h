#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "PortamentoProcessor.h"

class MidiMonitorAudioProcessor;

class PortamentoTab : public juce::Component,
                      public juce::Timer
{
public:
    explicit PortamentoTab (MidiMonitorAudioProcessor&);
    ~PortamentoTab() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    MidiMonitorAudioProcessor& processor;

    // Main Settings Controls
    juce::ToggleButton enablePortamento { "Enable Portamento Glide Engine" };
    juce::ToggleButton requireCC65Toggle { "Require MIDI CC65 On (Portamento Switch)" };
    juce::ToggleButton legatoOnlyToggle { "Legato Only (Only glide when notes overlap)" };
    juce::ToggleButton useCC5Toggle { "Scale glide speed by incoming MIDI CC5 (Portamento Time)" };

    // Mode & Curves
    juce::ComboBox glideModeBox;
    juce::Label glideModeLabel { {}, "Glide Mode:" };

    juce::ComboBox pitchBendRangeBox;
    juce::Label pitchBendRangeLabel { {}, "Synth Pitch Bend Range:" };

    juce::ComboBox glideCurveBox;
    juce::Label glideCurveLabel { {}, "Analogue Glide Curve:" };

    juce::Slider glideTimeSlider;
    juce::Label glideTimeLabel { {}, "Base Glide Time:" };

    juce::Slider maxGapSlider;
    juce::Label maxGapLabel { {}, "Max Phrase Pause:" };

    juce::ComboBox stepIntervalBox;
    juce::Label stepIntervalLabel { {}, "Glissando Step Resolution (Stepped Mode):" };

    juce::TextButton sendRPNButton { "Send Pitch Bend Range (RPN 0,0) to Synth" };

    // Live Visualizer Display elements
    juce::Label statusHeaderLabel { {}, "LIVE PORTAMENTO STATUS" };
    juce::Label modeBadgeLabel;
    juce::Label transitionNotesLabel;
    juce::Label pitchBendGaugeLabel;
    juce::Label ccStatusLabel;
    juce::Label statsLabel;

    // Test buttons
    juce::TextButton testUpButton { "Test Glide Up (C3 -> G3)" };
    juce::TextButton testDownButton { "Test Glide Down (G3 -> C3)" };

    float currentProgress = 0.0f;
    bool isCurrentlyGliding = false;
    int currentPitchBend = 0;

    void applySettingsToProcessor();
    void sendPitchBendRPN();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PortamentoTab)
};
