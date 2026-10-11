#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "MidiMonitorTab.h"
#include "PortamentoTab.h"
#include "MicrotuningTab.h"
#include "BreathRemapTab.h"

class MidiMonitorAudioProcessorEditor : public juce::AudioProcessorEditor,
                                        public juce::Timer
{
public:
    explicit MidiMonitorAudioProcessorEditor (MidiMonitorAudioProcessor&);
    ~MidiMonitorAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    MidiMonitorAudioProcessor& audioProcessor;

    // Header Controls
    juce::Label titleLabel;
    juce::Label statsLabel;
    juce::TextButton panicButton { "PANIC" };

    // Tabbed interface
    juce::TabbedComponent tabs { juce::TabbedButtonBar::TabsAtTop };
    PortamentoTab portamentoTab;
    MicrotuningTab microtuningTab;
    BreathRemapTab breathRemapTab;
    MidiMonitorTab inMonitorTab;
    MidiMonitorTab outMonitorTab;

    std::vector<MidiMonitor::MidiLogEntry> inScratchBuffer;
    std::vector<MidiMonitor::MidiLogEntry> outScratchBuffer;
    int activityFlashCounter = 0;
    uint64_t totalInCount = 0;
    uint64_t totalOutCount = 0;

    void sendPanic();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiMonitorAudioProcessorEditor)
};
