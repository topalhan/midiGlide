#include "PluginEditor.h"
#include "PluginProcessor.h"

MidiMonitorAudioProcessorEditor::MidiMonitorAudioProcessorEditor (MidiMonitorAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      inMonitorTab (p, TabMode::MidiIn),
      outMonitorTab (p, TabMode::MidiOut),
      portamentoTab (p)
{
    inScratchBuffer.reserve (256);
    outScratchBuffer.reserve (256);

    setSize (920, 580);
    setResizable (true, true);
    setResizeLimits (740, 440, 1600, 1200);

    // Title
    titleLabel.setText ("MIDI GLIDE v0.1", juce::dontSendNotification);
    titleLabel.setFont (juce::FontOptions (18.0f, juce::Font::bold));
    titleLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (titleLabel);

    // Stats
    statsLabel.setText ("In: 0 | Out: 0", juce::dontSendNotification);
    statsLabel.setFont (juce::FontOptions (12.0f));
    statsLabel.setColour (juce::Label::textColourId, juce::Colour (0xff9da5b4));
    addAndMakeVisible (statsLabel);

    // Panic Button
    panicButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xffc0392b));
    panicButton.setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    panicButton.onClick = [this] { sendPanic(); };
    addAndMakeVisible (panicButton);

    // Setup TabbedComponent
    tabs.setTabBarDepth (32);
    tabs.getTabbedButtonBar().setColour (juce::TabbedButtonBar::tabOutlineColourId, juce::Colour (0xff2c313a));
    tabs.addTab ("Portamento Glide",  juce::Colour (0xff181a1f), &portamentoTab, false);
    tabs.addTab ("MIDI In",           juce::Colour (0xff181a1f), &inMonitorTab, false);
    tabs.addTab ("MIDI Out",          juce::Colour (0xff181a1f), &outMonitorTab, false);
    addAndMakeVisible (tabs);

    startTimerHz (30);
}

MidiMonitorAudioProcessorEditor::~MidiMonitorAudioProcessorEditor()
{
    stopTimer();
}

void MidiMonitorAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff181a1f));

    // Top Header Bar
    g.setColour (juce::Colour (0xff21252b));
    g.fillRect (0, 0, getWidth(), 40);

    g.setColour (juce::Colour (0xff2c313a));
    g.drawHorizontalLine (40, 0.0f, static_cast<float>(getWidth()));

    // Activity LED
    auto ledBounds = juce::Rectangle<float> (130.0f, 15.0f, 10.0f, 10.0f);
    if (activityFlashCounter > 0)
    {
        g.setColour (juce::Colour (0xff00e676));
        g.fillEllipse (ledBounds);
        g.setColour (juce::Colour (0x6600e676));
        g.drawEllipse (ledBounds.expanded (3.0f), 2.0f);
    }
    else
    {
        g.setColour (juce::Colour (0xff3b4048));
        g.fillEllipse (ledBounds);
    }
}

void MidiMonitorAudioProcessorEditor::resized()
{
    int w = getWidth();
    int h = getHeight();

    titleLabel.setBounds (12, 8, 115, 24);
    statsLabel.setBounds (150, 8, 220, 24);
    panicButton.setBounds (w - 82, 8, 70, 24);

    // Tabs occupy entire area below header
    tabs.setBounds (0, 41, w, h - 41);
}

void MidiMonitorAudioProcessorEditor::timerCallback()
{
    if (activityFlashCounter > 0)
    {
        --activityFlashCounter;
        repaint (120, 8, 30, 24);
    }

    inScratchBuffer.clear();
    int numInPopped = audioProcessor.getMidiInQueue().popAll (inScratchBuffer, 2048);

    outScratchBuffer.clear();
    int numOutPopped = audioProcessor.getMidiOutQueue().popAll (outScratchBuffer, 2048);

    if (numInPopped > 0 || numOutPopped > 0)
    {
        activityFlashCounter = 4;
        repaint (120, 8, 30, 24);

        totalInCount += static_cast<uint64_t>(numInPopped);
        totalOutCount += static_cast<uint64_t>(numOutPopped);
        statsLabel.setText ("In: " + juce::String (totalInCount) + " | Out: " + juce::String (totalOutCount), juce::dontSendNotification);

        if (numInPopped > 0)
            inMonitorTab.updateWithNewEntries (inScratchBuffer);

        if (numOutPopped > 0)
            outMonitorTab.updateWithNewEntries (outScratchBuffer);
    }
}

void MidiMonitorAudioProcessorEditor::sendPanic()
{
    for (int ch = 1; ch <= 16; ++ch)
    {
        audioProcessor.injectMidiMessage (juce::MidiMessage::allNotesOff (ch));
        audioProcessor.injectMidiMessage (juce::MidiMessage::allSoundOff (ch));
        audioProcessor.injectMidiMessage (juce::MidiMessage::controllerEvent (ch, 64, 0));
    }
    audioProcessor.getPortamentoProcessor().reset();
    inMonitorTab.clearLog();
    outMonitorTab.clearLog();
}
