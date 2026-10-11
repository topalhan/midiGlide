#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "MicrotuningProcessor.h"

class MidiMonitorAudioProcessor;

class MicrotuningTab : public juce::Component,
                       public juce::Timer
{
public:
    explicit MicrotuningTab (MidiMonitorAudioProcessor&);
    ~MicrotuningTab() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    MidiMonitorAudioProcessor& processor;

    // Header Controls
    juce::ToggleButton enableMicrotuning { "Enable Turkish Makam Microtuning" };
    juce::ComboBox makamSelectorBox;
    juce::Label makamSelectorLabel { {}, "Makam:" };

    juce::ComboBox rootKeyBox;
    juce::Label rootKeyLabel { {}, "Karar (Root):" };

    juce::ComboBox tuningModeBox;
    juce::Label tuningModeLabel { {}, "Tuning System:" };

    juce::ComboBox pitchBendRangeBox;
    juce::Label pitchBendRangeLabel { {}, "Pitch Bend Range:" };

    juce::TextButton sendRPNButton { "Send RPN (0,0)" };
    juce::TextButton defaultKararButton { "Default Karar" };

    // Makam Info Banner
    juce::Label makamTitleLabel;
    juce::Label makamDetailsLabel;
    juce::Label makamDescriptionLabel;

    // 12-Key Keyboard Strip Components
    struct KeyStripComponent : public juce::Component
    {
        int chromaIndex = 0; // 0=C, 1=C# ... 11=B
        bool isBlackKey = false;
        juce::String noteName;
        juce::String perdeName;
        juce::String degreeRole; // "KARAR", "GÜÇLÜ", "PERDE", etc.
        bool isScaleNote = false;

        juce::Slider centSlider;
        juce::Label nameLabel;
        juce::Label perdeLabel;
        juce::Label commaLabel;
        juce::TextButton zeroButton { "0" };

        std::function<void(int chroma, float cents)> onCentsChanged;

        KeyStripComponent();
        void paint (juce::Graphics&) override;
        void resized() override;
        void updateDisplay (float cents, const juce::String& perde, const juce::String& role, bool inScale);
    };

    std::array<std::unique_ptr<KeyStripComponent>, 12> keyStrips;

    // Preset & Reset Controls
    juce::TextButton resetPresetButton { "Reset to Makam Default" };
    juce::TextButton allZeroButton { "Equal Temperament (0 cents)" };

    // Audition Test Buttons
    juce::TextButton testKararButton { "Play Karar" };
    juce::TextButton testMicrotoneButton { "Play Microtone" };
    juce::TextButton testScaleButton { "Play Makam Scale" };
    juce::TextButton stopAuditionButton { "All Notes Off" };

    // Live Monitoring
    juce::Label liveStatusHeaderLabel { {}, "LIVE MICROTONAL PITCH BEND STATUS" };
    juce::Label liveNoteLabel;
    juce::Label liveCentsLabel;
    juce::Label livePitchWheelLabel;
    juce::Label liveNotesCounterLabel;

    float currentLiveCents = 0.0f;
    int currentLivePitchWheel = 8192;
    int currentLiveMidiNote = -1;
    bool isLiveActive = false;

    void applySettingsFromUI();
    void updateUIFromProcessor();
    void refreshKeyStrips();
    void sendPitchBendRPN();
    void playAuditionNote (int midiNote, int durationMs);
    void playAuditionScale();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MicrotuningTab)
};
