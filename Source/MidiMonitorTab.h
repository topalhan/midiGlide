#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "MidiLogEntry.h"
#include <vector>
#include <unordered_set>
#include <array>

class MidiMonitorAudioProcessor;

enum class TabMode
{
    MidiIn,
    MidiOut
};

class MidiMonitorTab : public juce::Component,
                       public juce::TableListBoxModel
{
public:
    MidiMonitorTab (MidiMonitorAudioProcessor&, TabMode mode);
    ~MidiMonitorTab() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    // TableListBoxModel overrides
    int getNumRows() override;
    void paintRowBackground (juce::Graphics&, int rowNumber, int width, int height, bool rowIsSelected) override;
    void paintCell (juce::Graphics&, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override;

    void updateWithNewEntries (const std::vector<MidiMonitor::MidiLogEntry>& newEntries);
    void clearLog();
    void copyLogToClipboard();

    TabMode getMode() const noexcept { return mode; }

private:
    enum ColumnId
    {
        ColIndex = 1,
        ColTime,
        ColBarBeat,
        ColChannel,
        ColType,
        ColOrigin,
        ColDescription,
        ColHex
    };

    MidiMonitorAudioProcessor& processor;
    TabMode mode;

    // Filter controls
    juce::TextButton clearButton { "CLEAR" };
    juce::TextButton copyButton  { "COPY LOG" };
    juce::ToggleButton freezeToggle { "FREEZE" };
    juce::ToggleButton autoScrollToggle { "AUTO-SCROLL" };
    juce::ToggleButton filterInjectionsOnly { "Injections Only" };

    juce::ComboBox channelFilter;
    juce::ToggleButton filterNotes       { "Notes" };
    juce::ToggleButton filterCC          { "CC" };
    juce::ToggleButton filterPitchBend   { "Pitch Bend" };
    juce::ToggleButton filterAftertouch  { "Aftertouch" };
    juce::ToggleButton filterProgChange  { "Prog Change" };
    juce::ToggleButton filterClock       { "Clock" };
    juce::ToggleButton filterSysEx       { "SysEx" };
    juce::TextEditor searchFilter;

    // Table
    juce::TableListBox table;

    // Bottom Live State Panel
    juce::Label activeNotesLabel;
    juce::Label ccMonitorLabel;
    juce::Label pitchBendLabel;

    // Data structures
    std::vector<MidiMonitor::MidiLogEntry> allEntries;
    std::vector<size_t> filteredIndices;

    std::unordered_set<int> activeNotes;
    std::array<int, 128> lastCCValues {};
    int lastPitchBendValue = 0;
    int lastChannelPressure = 0;

    void updateFilter();
    bool matchesFilter (const MidiMonitor::MidiLogEntry& e) const;
    void updateBottomStateBar();

    static juce::Colour getTypeColour (MidiMonitor::MidiLogEntry::MsgType type);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiMonitorTab)
};
