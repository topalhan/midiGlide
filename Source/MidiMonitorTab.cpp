#include "MidiMonitorTab.h"
#include "PluginProcessor.h"

MidiMonitorTab::MidiMonitorTab (MidiMonitorAudioProcessor& p, TabMode m)
    : processor (p), mode (m)
{
    allEntries.reserve (10000);
    filteredIndices.reserve (10000);
    lastCCValues.fill (-1);

    clearButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff2c3e50));
    clearButton.onClick = [this] { clearLog(); };
    addAndMakeVisible (clearButton);

    copyButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff34495e));
    copyButton.onClick = [this] { copyLogToClipboard(); };
    addAndMakeVisible (copyButton);

    freezeToggle.setColour (juce::ToggleButton::textColourId, juce::Colour (0xffabb2bf));
    addAndMakeVisible (freezeToggle);

    autoScrollToggle.setToggleState (true, juce::dontSendNotification);
    autoScrollToggle.setColour (juce::ToggleButton::textColourId, juce::Colour (0xffabb2bf));
    addAndMakeVisible (autoScrollToggle);

    if (mode == TabMode::MidiOut)
    {
        filterInjectionsOnly.setColour (juce::ToggleButton::textColourId, juce::Colour (0xffe5c07b));
        filterInjectionsOnly.setToggleState (false, juce::dontSendNotification);
        filterInjectionsOnly.onClick = [this] { updateFilter(); };
        addAndMakeVisible (filterInjectionsOnly);
    }

    channelFilter.addItem ("All Channels", 1);
    for (int ch = 1; ch <= 16; ++ch)
        channelFilter.addItem ("Channel " + juce::String (ch), ch + 1);
    channelFilter.setSelectedId (1, juce::dontSendNotification);
    channelFilter.onChange = [this] { updateFilter(); };
    addAndMakeVisible (channelFilter);

    auto setupFilterCheck = [this] (juce::ToggleButton& b, bool initVal) {
        b.setToggleState (initVal, juce::dontSendNotification);
        b.setColour (juce::ToggleButton::textColourId, juce::Colour (0xffabb2bf));
        b.onClick = [this] { updateFilter(); };
        addAndMakeVisible (b);
    };

    setupFilterCheck (filterNotes, true);
    setupFilterCheck (filterCC, true);
    setupFilterCheck (filterPitchBend, true);
    setupFilterCheck (filterAftertouch, true);
    setupFilterCheck (filterProgChange, true);
    setupFilterCheck (filterClock, false);
    setupFilterCheck (filterSysEx, true);

    searchFilter.setTextToShowWhenEmpty ("Search...", juce::Colour (0xff5c6370));
    searchFilter.setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff1e2227));
    searchFilter.setColour (juce::TextEditor::textColourId, juce::Colours::white);
    searchFilter.setColour (juce::TextEditor::outlineColourId, juce::Colour (0xff3e4451));
    searchFilter.onTextChange = [this] { updateFilter(); };
    addAndMakeVisible (searchFilter);

    table.setModel (this);
    table.setColour (juce::ListBox::backgroundColourId, juce::Colour (0xff181a1f));
    table.setOutlineThickness (1);

    if (mode == TabMode::MidiOut)
    {
        table.getHeader().addColumn ("#",           ColIndex,       45, 35,  60);
        table.getHeader().addColumn ("Time",        ColTime,        80, 60, 105);
        table.getHeader().addColumn ("Bar|Beat",    ColBarBeat,     70, 50,  95);
        table.getHeader().addColumn ("Ch",          ColChannel,     35, 25,  45);
        table.getHeader().addColumn ("Type",        ColType,       120, 80, 150);
        table.getHeader().addColumn ("Origin",      ColOrigin,     110, 80, 150);
        table.getHeader().addColumn ("Description", ColDescription, 280, 140, 800);
        table.getHeader().addColumn ("Raw Hex",     ColHex,         95, 70, 180);
    }
    else
    {
        table.getHeader().addColumn ("#",           ColIndex,       50, 40,  70);
        table.getHeader().addColumn ("Time",        ColTime,        85, 65, 110);
        table.getHeader().addColumn ("Bar|Beat",    ColBarBeat,     75, 55, 100);
        table.getHeader().addColumn ("Ch",          ColChannel,     40, 30,  55);
        table.getHeader().addColumn ("Type",        ColType,       135, 90, 170);
        table.getHeader().addColumn ("Description", ColDescription, 320, 150, 800);
        table.getHeader().addColumn ("Raw Hex",     ColHex,        110, 80, 200);
    }

    table.getHeader().setColour (juce::TableHeaderComponent::backgroundColourId, juce::Colour (0xff21252b));
    table.getHeader().setColour (juce::TableHeaderComponent::textColourId, juce::Colour (0xff9da5b4));
    table.getHeader().setColour (juce::TableHeaderComponent::outlineColourId, juce::Colour (0xff2c313a));
    addAndMakeVisible (table);

    activeNotesLabel.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    activeNotesLabel.setColour (juce::Label::textColourId, juce::Colour (0xff61afef));
    activeNotesLabel.setText (mode == TabMode::MidiOut ? "Out Notes: None" : "In Notes: None", juce::dontSendNotification);
    addAndMakeVisible (activeNotesLabel);

    ccMonitorLabel.setFont (juce::FontOptions (12.0f));
    ccMonitorLabel.setColour (juce::Label::textColourId, juce::Colour (0xff98c379));
    ccMonitorLabel.setText ("CC: Brth(2):- | Expr(11):- | Mod(1):- | Sus(64):- | Cutoff(74):-", juce::dontSendNotification);
    addAndMakeVisible (ccMonitorLabel);

    pitchBendLabel.setFont (juce::FontOptions (12.0f));
    pitchBendLabel.setColour (juce::Label::textColourId, juce::Colour (0xffe5c07b));
    pitchBendLabel.setText ("Pitch Bend: 0", juce::dontSendNotification);
    addAndMakeVisible (pitchBendLabel);
}

MidiMonitorTab::~MidiMonitorTab()
{
    table.setModel (nullptr);
}

void MidiMonitorTab::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff181a1f));

    // Top control bar
    g.setColour (juce::Colour (0xff21252b));
    g.fillRect (0, 0, getWidth(), 72);

    // Bottom status bar
    g.fillRect (0, getHeight() - 28, getWidth(), 28);

    g.setColour (juce::Colour (0xff2c313a));
    g.drawHorizontalLine (72, 0.0f, static_cast<float>(getWidth()));
    g.drawHorizontalLine (getHeight() - 28, 0.0f, static_cast<float>(getWidth()));
}

void MidiMonitorTab::resized()
{
    int w = getWidth();
    int h = getHeight();

    // Row 1: Actions & Search (y: 6 to 32)
    int rx = w - 12;
    rx -= 60;
    clearButton.setBounds (rx, 6, 60, 24);
    rx -= 80;
    copyButton.setBounds (rx, 6, 75, 24);
    rx -= 75;
    freezeToggle.setBounds (rx, 6, 70, 24);
    rx -= 105;
    autoScrollToggle.setBounds (rx, 6, 100, 24);

    if (mode == TabMode::MidiOut)
    {
        rx -= 125;
        filterInjectionsOnly.setBounds (rx, 6, 120, 24);
    }

    searchFilter.setBounds (12, 6, std::min (220, rx - 20), 24);

    // Row 2: Filters (y: 38 to 66)
    int fx = 12;
    channelFilter.setBounds (fx, 38, 115, 26);
    fx += 122;

    filterNotes.setBounds (fx, 38, 60, 26);
    fx += 65;
    filterCC.setBounds (fx, 38, 50, 26);
    fx += 55;
    filterPitchBend.setBounds (fx, 38, 90, 26);
    fx += 95;
    filterAftertouch.setBounds (fx, 38, 85, 26);
    fx += 90;
    filterProgChange.setBounds (fx, 38, 95, 26);
    fx += 100;
    filterClock.setBounds (fx, 38, 60, 26);
    fx += 65;
    filterSysEx.setBounds (fx, 38, 65, 26);

    // Table
    table.setBounds (0, 73, w, h - 73 - 28);

    // Bottom HUD
    int botY = h - 25;
    int colWidth = (w - 24) / 3;
    activeNotesLabel.setBounds (12, botY, colWidth, 20);
    ccMonitorLabel.setBounds (12 + colWidth, botY, colWidth + 50, 20);
    pitchBendLabel.setBounds (w - colWidth + 50, botY, colWidth - 62, 20);
}

void MidiMonitorTab::updateWithNewEntries (const std::vector<MidiMonitor::MidiLogEntry>& newEntries)
{
    bool isFrozen = freezeToggle.getToggleState();

    for (const auto& entry : newEntries)
    {
        if (entry.type == MidiMonitor::MidiLogEntry::MsgType::NoteOn)
        {
            if (entry.data2 > 0)
                activeNotes.insert ((entry.channel << 8) | entry.data1);
            else
                activeNotes.erase ((entry.channel << 8) | entry.data1);
        }
        else if (entry.type == MidiMonitor::MidiLogEntry::MsgType::NoteOff)
        {
            activeNotes.erase ((entry.channel << 8) | entry.data1);
        }
        else if (entry.type == MidiMonitor::MidiLogEntry::MsgType::ControlChange)
        {
            if (entry.data1 < 128)
                lastCCValues[entry.data1] = entry.data2;
        }
        else if (entry.type == MidiMonitor::MidiLogEntry::MsgType::PitchBend)
        {
            lastPitchBendValue = entry.intValue;
        }
        else if (entry.type == MidiMonitor::MidiLogEntry::MsgType::ChannelPressure)
        {
            lastChannelPressure = entry.intValue;
        }

        if (!isFrozen)
        {
            size_t newIndex = allEntries.size();
            allEntries.push_back (entry);

            if (matchesFilter (entry))
                filteredIndices.push_back (newIndex);
        }
    }

    if (!isFrozen && allEntries.size() > 10000)
    {
        size_t pruneCount = 2000;
        allEntries.erase (allEntries.begin(), allEntries.begin() + static_cast<ptrdiff_t>(pruneCount));
        updateFilter();
    }
    else if (!isFrozen)
    {
        table.updateContent();
        if (autoScrollToggle.getToggleState() && !filteredIndices.empty())
            table.scrollToEnsureRowIsOnscreen (static_cast<int>(filteredIndices.size() - 1));
        table.repaint();
    }

    updateBottomStateBar();
}

void MidiMonitorTab::clearLog()
{
    allEntries.clear();
    filteredIndices.clear();
    activeNotes.clear();
    table.updateContent();
    table.repaint();
    updateBottomStateBar();
}

void MidiMonitorTab::copyLogToClipboard()
{
    juce::String out = "#\tTime\tBar|Beat\tCh\tType\t";
    if (mode == TabMode::MidiOut) out << "Origin\t";
    out << "Description\tHex\n";

    for (size_t i = 0; i < filteredIndices.size(); ++i)
    {
        const auto& e = allEntries[filteredIndices[i]];
        out << (i + 1) << "\t"
            << e.timeString << "\t"
            << e.barBeatString << "\t"
            << (int)e.channel << "\t"
            << e.typeName << "\t";
        if (mode == TabMode::MidiOut)
            out << e.originTag << "\t";
        out << e.description << "\t"
            << e.hexString << "\n";
    }
    juce::SystemClipboard::copyTextToClipboard (out);
}

void MidiMonitorTab::updateBottomStateBar()
{
    juce::String prefix = (mode == TabMode::MidiOut ? "Out Notes" : "In Notes");
    if (activeNotes.empty())
    {
        activeNotesLabel.setText (prefix + ": None", juce::dontSendNotification);
    }
    else
    {
        juce::String noteStr = prefix + " (" + juce::String (activeNotes.size()) + "): ";
        int count = 0;
        for (int packed : activeNotes)
        {
            if (count > 0) noteStr << ", ";
            if (count >= 5)
            {
                noteStr << "...";
                break;
            }
            int ch = packed >> 8;
            int note = packed & 0xFF;
            noteStr << MidiMonitor::MidiLogEntry::getMidiNoteName (note) << " (Ch" << ch << ")";
            ++count;
        }
        activeNotesLabel.setText (noteStr, juce::dontSendNotification);
    }

    auto getCCStr = [this] (int ccNum) -> juce::String {
        int v = lastCCValues[static_cast<size_t>(ccNum)];
        return (v >= 0) ? juce::String (v) : "-";
    };

    juce::String ccStr = "CC: ";
    ccStr << "Brth(2):" << getCCStr (2) << " | ";
    ccStr << "Expr(11):" << getCCStr (11) << " | ";
    ccStr << "Mod(1):" << getCCStr (1) << " | ";
    int sus = lastCCValues[64];
    ccStr << "Sus(64):" << (sus < 0 ? "-" : (sus >= 64 ? "ON" : "off")) << " | ";
    ccStr << "Cut(74):" << getCCStr (74);
    ccMonitorLabel.setText (ccStr, juce::dontSendNotification);

    double pbPercent = (lastPitchBendValue / 8192.0) * 100.0;
    juce::String pbStr = "Bend: " + juce::String (lastPitchBendValue > 0 ? "+" : "")
                       + juce::String (lastPitchBendValue) + " ("
                       + juce::String (pbPercent, 1) + "%)";
    if (lastChannelPressure > 0)
        pbStr << " | AT: " << juce::String (lastChannelPressure);
    pitchBendLabel.setText (pbStr, juce::dontSendNotification);
}

bool MidiMonitorTab::matchesFilter (const MidiMonitor::MidiLogEntry& e) const
{
    // Filter Injections Only (in MidiOut mode)
    if (mode == TabMode::MidiOut && filterInjectionsOnly.getToggleState() && !e.isInjected)
        return false;

    int selectedCh = channelFilter.getSelectedId() - 1;
    if (selectedCh > 0 && e.channel != selectedCh)
        return false;

    switch (e.type)
    {
        case MidiMonitor::MidiLogEntry::MsgType::NoteOn:
        case MidiMonitor::MidiLogEntry::MsgType::NoteOff:
            if (!filterNotes.getToggleState()) return false;
            break;

        case MidiMonitor::MidiLogEntry::MsgType::ControlChange:
            if (!filterCC.getToggleState()) return false;
            break;

        case MidiMonitor::MidiLogEntry::MsgType::PitchBend:
            if (!filterPitchBend.getToggleState()) return false;
            break;

        case MidiMonitor::MidiLogEntry::MsgType::ChannelPressure:
        case MidiMonitor::MidiLogEntry::MsgType::PolyPressure:
            if (!filterAftertouch.getToggleState()) return false;
            break;

        case MidiMonitor::MidiLogEntry::MsgType::ProgramChange:
            if (!filterProgChange.getToggleState()) return false;
            break;

        case MidiMonitor::MidiLogEntry::MsgType::ClockTiming:
            if (!filterClock.getToggleState()) return false;
            break;

        case MidiMonitor::MidiLogEntry::MsgType::SysEx:
            if (!filterSysEx.getToggleState()) return false;
            break;

        case MidiMonitor::MidiLogEntry::MsgType::Other:
        default:
            break;
    }

    auto query = searchFilter.getText().trim();
    if (query.isNotEmpty())
    {
        if (!juce::String (e.typeName).containsIgnoreCase (query)
         && !juce::String (e.originTag).containsIgnoreCase (query)
         && !juce::String (e.description).containsIgnoreCase (query)
         && !juce::String (e.hexString).containsIgnoreCase (query)
         && !juce::String (e.channel).equalsIgnoreCase (query))
        {
            return false;
        }
    }

    return true;
}

void MidiMonitorTab::updateFilter()
{
    filteredIndices.clear();
    for (size_t i = 0; i < allEntries.size(); ++i)
    {
        if (matchesFilter (allEntries[i]))
            filteredIndices.push_back (i);
    }
    table.updateContent();
    if (autoScrollToggle.getToggleState() && !filteredIndices.empty())
        table.scrollToEnsureRowIsOnscreen (static_cast<int>(filteredIndices.size() - 1));
    table.repaint();
}

int MidiMonitorTab::getNumRows()
{
    return static_cast<int>(filteredIndices.size());
}

void MidiMonitorTab::paintRowBackground (juce::Graphics& g, int rowNumber,
                                         int /*width*/, int /*height*/, bool rowIsSelected)
{
    if (rowIsSelected)
    {
        g.fillAll (juce::Colour (0xff2c313a));
        return;
    }

    if (rowNumber >= 0 && rowNumber < static_cast<int>(filteredIndices.size()))
    {
        const auto& entry = allEntries[filteredIndices[static_cast<size_t>(rowNumber)]];
        if (entry.isInjected)
        {
            // Subtle amber tint for injected rows
            g.fillAll (rowNumber % 2 == 0 ? juce::Colour (0xff28221b) : juce::Colour (0xff302920));
            return;
        }
    }

    g.fillAll (rowNumber % 2 == 0 ? juce::Colour (0xff1e2227) : juce::Colour (0xff23272e));
}

juce::Colour MidiMonitorTab::getTypeColour (MidiMonitor::MidiLogEntry::MsgType type)
{
    switch (type)
    {
        case MidiMonitor::MidiLogEntry::MsgType::NoteOn:          return juce::Colour (0xff98c379);
        case MidiMonitor::MidiLogEntry::MsgType::NoteOff:         return juce::Colour (0xff5c6370);
        case MidiMonitor::MidiLogEntry::MsgType::ControlChange:    return juce::Colour (0xff61afef);
        case MidiMonitor::MidiLogEntry::MsgType::PitchBend:        return juce::Colour (0xffd19a66);
        case MidiMonitor::MidiLogEntry::MsgType::ChannelPressure:
        case MidiMonitor::MidiLogEntry::MsgType::PolyPressure:     return juce::Colour (0xffc678dd);
        case MidiMonitor::MidiLogEntry::MsgType::ProgramChange:    return juce::Colour (0xffe5c07b);
        case MidiMonitor::MidiLogEntry::MsgType::SysEx:            return juce::Colour (0xffe06c75);
        case MidiMonitor::MidiLogEntry::MsgType::ClockTiming:      return juce::Colour (0xff5c6370);
        case MidiMonitor::MidiLogEntry::MsgType::Other:
        default:                                                   return juce::Colour (0xffabb2bf);
    }
}

void MidiMonitorTab::paintCell (juce::Graphics& g, int rowNumber, int columnId,
                                int width, int height, bool /*rowIsSelected*/)
{
    if (rowNumber < 0 || rowNumber >= static_cast<int>(filteredIndices.size()))
        return;

    const auto& entry = allEntries[filteredIndices[static_cast<size_t>(rowNumber)]];
    auto bounds = juce::Rectangle<int> (4, 0, width - 8, height);

    g.setFont (juce::FontOptions (12.0f));

    switch (columnId)
    {
        case ColIndex:
        {
            g.setColour (juce::Colour (0xff5c6370));
            g.drawText (juce::String (rowNumber + 1), bounds, juce::Justification::centredLeft, true);
            break;
        }
        case ColTime:
        {
            g.setColour (juce::Colour (0xffabb2bf));
            g.drawText (entry.timeString, bounds, juce::Justification::centredLeft, true);
            break;
        }
        case ColBarBeat:
        {
            g.setColour (juce::Colour (0xff5c6370));
            g.drawText (entry.barBeatString, bounds, juce::Justification::centredLeft, true);
            break;
        }
        case ColChannel:
        {
            g.setColour (juce::Colour (0xffe5c07b));
            g.drawText (juce::String (entry.channel), bounds, juce::Justification::centred, true);
            break;
        }
        case ColType:
        {
            auto typeCol = getTypeColour (entry.type);
            auto badgeArea = bounds.toFloat().reduced (2.0f, 3.0f);
            g.setColour (typeCol.withAlpha (0.18f));
            g.fillRoundedRectangle (badgeArea, 3.0f);
            g.setColour (typeCol);
            g.drawRoundedRectangle (badgeArea, 3.0f, 1.0f);

            g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
            g.drawText (entry.typeName, bounds, juce::Justification::centred, true);
            break;
        }
        case ColOrigin:
        {
            auto originCol = entry.isInjected ? juce::Colour (0xffe5c07b) : juce::Colour (0xff5c6370);
            auto badgeArea = bounds.toFloat().reduced (2.0f, 3.0f);
            g.setColour (originCol.withAlpha (0.18f));
            g.fillRoundedRectangle (badgeArea, 3.0f);
            g.setColour (originCol);
            g.drawRoundedRectangle (badgeArea, 3.0f, 1.0f);

            g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
            g.drawText (entry.originTag, bounds, juce::Justification::centred, true);
            break;
        }
        case ColDescription:
        {
            g.setColour (entry.isInjected ? juce::Colour (0xfffff3d4) : juce::Colours::white);
            g.drawText (entry.description, bounds, juce::Justification::centredLeft, true);
            break;
        }
        case ColHex:
        {
            g.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 11.0f, juce::Font::plain));
            g.setColour (juce::Colour (0xffabb2bf));
            g.drawText (entry.hexString, bounds, juce::Justification::centredLeft, true);
            break;
        }
        default:
            break;
    }
}
