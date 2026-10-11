#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "BreathRemapProcessor.h"

class MidiMonitorAudioProcessor;

class BreathRemapTab : public juce::Component,
                       public juce::Timer
{
public:
    explicit BreathRemapTab (MidiMonitorAudioProcessor&);
    ~BreathRemapTab() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    MidiMonitorAudioProcessor& processor;

    // Master switch
    juce::ToggleButton enableRemapper { "Enable Breath to Modulation Remapper" };

    // Routing controls
    juce::ComboBox sourceControllerBox;
    juce::Label sourceControllerLabel { {}, "Input Controller (Source):" };

    juce::ComboBox targetControllerBox;
    juce::Label targetControllerLabel { {}, "Output Controller (Target):" };

    juce::ComboBox routingModeBox;
    juce::Label routingModeLabel { {}, "Routing Mode:" };

    // Calibration & Shaping controls
    juce::ComboBox curveProfileBox;
    juce::Label curveProfileLabel { {}, "Breath Response Curve:" };

    juce::Slider deadzoneSlider;
    juce::Label deadzoneLabel { {}, "Lip Rest Deadzone:" };

    juce::Slider gainSlider;
    juce::Label gainLabel { {}, "Sensitivity / Gain:" };

    juce::Slider minOutputSlider;
    juce::Label minOutputLabel { {}, "Min Output Limit:" };

    juce::Slider maxOutputSlider;
    juce::Label maxOutputLabel { {}, "Max Output Limit:" };

    juce::Slider smoothingSlider;
    juce::Label smoothingLabel { {}, "Jitter Filter / Smoothing:" };

    juce::ToggleButton invertToggle { "Invert Direction (Blow harder -> lower output)" };

    // Actions & Tests
    juce::TextButton testBlowButton { "Test Breath Blow (Pulse 0 -> 127 -> 0)" };
    juce::TextButton resetDefaultsButton { "Reset Calibration to Defaults" };

    // Live Monitor Labels
    juce::Label statusHeaderLabel { {}, "LIVE MONITOR & RESPONSE CURVE" };
    juce::Label routeBadgeLabel;
    juce::Label inValueLabel;
    juce::Label outValueLabel;
    juce::Label statsLabel;
    juce::Label curveInfoLabel;

    // Cached live values
    int currentInVal = 0;
    int currentOutVal = 0;
    bool isActive = false;
    uint32_t totalPackets = 0;

    // Test blow simulation state
    int testPulseCounter = -1;

    void applySettingsToProcessor();
    void resetDefaults();
    void triggerTestBreathPulse();
    void drawCurvePreview (juce::Graphics& g, juce::Rectangle<float> bounds);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BreathRemapTab)
};
