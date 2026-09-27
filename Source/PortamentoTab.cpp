#include "PortamentoTab.h"
#include "PluginProcessor.h"

PortamentoTab::PortamentoTab (MidiMonitorAudioProcessor& p)
    : processor (p)
{
    auto& settings = processor.getPortamentoProcessor().getSettings();

    // Enable Toggle
    enablePortamento.setColour (juce::ToggleButton::textColourId, juce::Colour (0xff98c379));
    enablePortamento.setToggleState (settings.enabled.load(), juce::dontSendNotification);
    enablePortamento.onClick = [this] { applySettingsToProcessor(); };
    addAndMakeVisible (enablePortamento);

    // Glide Mode Selector
    glideModeBox.addItem ("Analogue Continuous (14-Bit Pitch Bend - Smooth)", 1);
    glideModeBox.addItem ("Stepped Glissando (Discrete Notes - Piano/Harp)", 2);
    glideModeBox.setSelectedId (settings.glideMode.load(), juce::dontSendNotification);
    glideModeBox.onChange = [this] { applySettingsToProcessor(); };
    addAndMakeVisible (glideModeBox);

    glideModeLabel.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    glideModeLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (glideModeLabel);

    // Pitch Bend Range Selector
    pitchBendRangeBox.addItem ("+/- 12 Semitones (1 Octave - Standard)", 12);
    pitchBendRangeBox.addItem ("+/- 24 Semitones (2 Octaves)", 24);
    pitchBendRangeBox.addItem ("+/- 2 Semitones (Whole Step)", 2);
    pitchBendRangeBox.addItem ("+/- 48 Semitones (4 Octaves)", 48);
    pitchBendRangeBox.setSelectedId (settings.pitchBendRange.load(), juce::dontSendNotification);
    pitchBendRangeBox.onChange = [this] { applySettingsToProcessor(); };
    addAndMakeVisible (pitchBendRangeBox);

    pitchBendRangeLabel.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    pitchBendRangeLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (pitchBendRangeLabel);

    // Glide Curve Selector
    glideCurveBox.addItem ("Analogue RC Exponential (Warm Natural Ease)", 1);
    glideCurveBox.addItem ("Linear (Constant Pitch Rate)", 2);
    glideCurveBox.setSelectedId (settings.glideCurve.load(), juce::dontSendNotification);
    glideCurveBox.onChange = [this] { applySettingsToProcessor(); };
    addAndMakeVisible (glideCurveBox);

    glideCurveLabel.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    glideCurveLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (glideCurveLabel);

    // Send RPN Button
    sendRPNButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff34495e));
    sendRPNButton.onClick = [this] { sendPitchBendRPN(); };
    addAndMakeVisible (sendRPNButton);

    // Toggles
    requireCC65Toggle.setColour (juce::ToggleButton::textColourId, juce::Colour (0xffabb2bf));
    requireCC65Toggle.setToggleState (settings.requireCC65.load(), juce::dontSendNotification);
    requireCC65Toggle.onClick = [this] { applySettingsToProcessor(); };
    addAndMakeVisible (requireCC65Toggle);

    legatoOnlyToggle.setColour (juce::ToggleButton::textColourId, juce::Colour (0xffabb2bf));
    legatoOnlyToggle.setToggleState (settings.legatoOnly.load(), juce::dontSendNotification);
    legatoOnlyToggle.onClick = [this] { applySettingsToProcessor(); };
    addAndMakeVisible (legatoOnlyToggle);

    useCC5Toggle.setColour (juce::ToggleButton::textColourId, juce::Colour (0xffabb2bf));
    useCC5Toggle.setToggleState (settings.useCC5Time.load(), juce::dontSendNotification);
    useCC5Toggle.onClick = [this] { applySettingsToProcessor(); };
    addAndMakeVisible (useCC5Toggle);

    // Glide Time Slider
    glideTimeSlider.setRange (10.0, 2000.0, 1.0);
    glideTimeSlider.setValue (settings.baseGlideTimeMs.load(), juce::dontSendNotification);
    glideTimeSlider.setTextValueSuffix (" ms");
    glideTimeSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    glideTimeSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 75, 22);
    glideTimeSlider.setColour (juce::Slider::thumbColourId, juce::Colour (0xff61afef));
    glideTimeSlider.setColour (juce::Slider::trackColourId, juce::Colour (0xff3b4048));
    glideTimeSlider.onValueChange = [this] { applySettingsToProcessor(); };
    addAndMakeVisible (glideTimeSlider);

    glideTimeLabel.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    glideTimeLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (glideTimeLabel);

    // Max Phrase Gap Slider
    maxGapSlider.setRange (50.0, 2000.0, 10.0);
    maxGapSlider.setValue (settings.maxStaccatoGapMs.load(), juce::dontSendNotification);
    maxGapSlider.setTextValueSuffix (" ms");
    maxGapSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    maxGapSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 75, 22);
    maxGapSlider.setColour (juce::Slider::thumbColourId, juce::Colour (0xffe5c07b));
    maxGapSlider.setColour (juce::Slider::trackColourId, juce::Colour (0xff3b4048));
    maxGapSlider.onValueChange = [this] { applySettingsToProcessor(); };
    addAndMakeVisible (maxGapSlider);

    maxGapLabel.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    maxGapLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (maxGapLabel);

    // Step Interval ComboBox (Stepped mode)
    stepIntervalBox.addItem ("Chromatic (1 Semitone)", 1);
    stepIntervalBox.addItem ("Whole Tone (2 Semitones)", 2);
    stepIntervalBox.addItem ("Minor 3rd (3 Semitones)", 3);
    stepIntervalBox.setSelectedId (settings.stepInterval.load(), juce::dontSendNotification);
    stepIntervalBox.onChange = [this] { applySettingsToProcessor(); };
    addAndMakeVisible (stepIntervalBox);

    stepIntervalLabel.setFont (juce::FontOptions (12.0f));
    stepIntervalLabel.setColour (juce::Label::textColourId, juce::Colour (0xffabb2bf));
    addAndMakeVisible (stepIntervalLabel);

    // Status Header
    statusHeaderLabel.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    statusHeaderLabel.setColour (juce::Label::textColourId, juce::Colour (0xff61afef));
    addAndMakeVisible (statusHeaderLabel);

    modeBadgeLabel.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    modeBadgeLabel.setColour (juce::Label::textColourId, juce::Colour (0xff98c379));
    modeBadgeLabel.setText ("Mode: Analogue 14-Bit Pitch Bend", juce::dontSendNotification);
    addAndMakeVisible (modeBadgeLabel);

    transitionNotesLabel.setFont (juce::FontOptions (13.0f));
    transitionNotesLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    transitionNotesLabel.setText ("Idle (Play two notes or click test buttons)", juce::dontSendNotification);
    addAndMakeVisible (transitionNotesLabel);

    pitchBendGaugeLabel.setFont (juce::FontOptions (12.0f));
    pitchBendGaugeLabel.setColour (juce::Label::textColourId, juce::Colour (0xffe5c07b));
    pitchBendGaugeLabel.setText ("Pitch Wheel: Centered (0)", juce::dontSendNotification);
    addAndMakeVisible (pitchBendGaugeLabel);

    ccStatusLabel.setFont (juce::FontOptions (12.0f));
    ccStatusLabel.setColour (juce::Label::textColourId, juce::Colour (0xffabb2bf));
    ccStatusLabel.setText ("CC65 Switch: OFF | CC5 Time: 64", juce::dontSendNotification);
    addAndMakeVisible (ccStatusLabel);

    statsLabel.setFont (juce::FontOptions (12.0f));
    statsLabel.setColour (juce::Label::textColourId, juce::Colour (0xff98c379));
    statsLabel.setText ("Injected Events: 0", juce::dontSendNotification);
    addAndMakeVisible (statsLabel);

    // Test buttons
    testUpButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff2c3e50));
    testUpButton.onClick = [this] {
        processor.injectMidiMessage (juce::MidiMessage::noteOn (1, 48, (uint8_t)100));
        juce::Timer::callAfterDelay (80, [this] {
            processor.injectMidiMessage (juce::MidiMessage::noteOn (1, 55, (uint8_t)100));
            juce::Timer::callAfterDelay (800, [this] {
                processor.injectMidiMessage (juce::MidiMessage::noteOff (1, 55, (uint8_t)64));
            });
        });
    };
    addAndMakeVisible (testUpButton);

    testDownButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff2c3e50));
    testDownButton.onClick = [this] {
        processor.injectMidiMessage (juce::MidiMessage::noteOn (1, 55, (uint8_t)100));
        juce::Timer::callAfterDelay (80, [this] {
            processor.injectMidiMessage (juce::MidiMessage::noteOn (1, 48, (uint8_t)100));
            juce::Timer::callAfterDelay (800, [this] {
                processor.injectMidiMessage (juce::MidiMessage::noteOff (1, 48, (uint8_t)64));
            });
        });
    };
    addAndMakeVisible (testDownButton);

    applySettingsToProcessor();
    startTimerHz (30);
}

PortamentoTab::~PortamentoTab()
{
    stopTimer();
}

void PortamentoTab::sendPitchBendRPN()
{
    int range = pitchBendRangeBox.getSelectedId();
    if (range <= 0) range = 12;

    // Standard MIDI Registered Parameter Number 0,0 (Pitch Bend Sensitivity)
    // RPN MSB (CC 101) = 0
    // RPN LSB (CC 100) = 0
    // Data Entry MSB (CC 6) = Semitones (range)
    // Data Entry LSB (CC 38) = Cents (0)
    // RPN Null (CC 101 = 127, CC 100 = 127)
    for (int ch = 1; ch <= 16; ++ch)
    {
        processor.injectMidiMessage (juce::MidiMessage::controllerEvent (ch, 101, 0));
        processor.injectMidiMessage (juce::MidiMessage::controllerEvent (ch, 100, 0));
        processor.injectMidiMessage (juce::MidiMessage::controllerEvent (ch, 6, static_cast<uint8_t>(range)));
        processor.injectMidiMessage (juce::MidiMessage::controllerEvent (ch, 38, 0));
        processor.injectMidiMessage (juce::MidiMessage::controllerEvent (ch, 101, 127));
        processor.injectMidiMessage (juce::MidiMessage::controllerEvent (ch, 100, 127));
    }
}

void PortamentoTab::applySettingsToProcessor()
{
    auto& settings = processor.getPortamentoProcessor().getSettings();
    settings.enabled.store (enablePortamento.getToggleState(), std::memory_order_relaxed);
    settings.glideMode.store (glideModeBox.getSelectedId(), std::memory_order_relaxed);
    settings.pitchBendRange.store (pitchBendRangeBox.getSelectedId(), std::memory_order_relaxed);
    settings.glideCurve.store (glideCurveBox.getSelectedId(), std::memory_order_relaxed);
    settings.requireCC65.store (requireCC65Toggle.getToggleState(), std::memory_order_relaxed);
    settings.legatoOnly.store (legatoOnlyToggle.getToggleState(), std::memory_order_relaxed);
    settings.useCC5Time.store (useCC5Toggle.getToggleState(), std::memory_order_relaxed);
    settings.baseGlideTimeMs.store (static_cast<float>(glideTimeSlider.getValue()), std::memory_order_relaxed);
    settings.maxStaccatoGapMs.store (static_cast<float>(maxGapSlider.getValue()), std::memory_order_relaxed);
    settings.stepInterval.store (stepIntervalBox.getSelectedId(), std::memory_order_relaxed);

    bool isStepped = (glideModeBox.getSelectedId() == 2);
    stepIntervalBox.setEnabled (isStepped);
    stepIntervalLabel.setEnabled (isStepped);
    pitchBendRangeBox.setEnabled (!isStepped);
    pitchBendRangeLabel.setEnabled (!isStepped);
    glideCurveBox.setEnabled (!isStepped);
    glideCurveLabel.setEnabled (!isStepped);
    sendRPNButton.setEnabled (!isStepped);

    bool isLegatoOnly = legatoOnlyToggle.getToggleState();
    maxGapSlider.setEnabled (!isLegatoOnly);
    maxGapLabel.setEnabled (!isLegatoOnly);
}

void PortamentoTab::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff181a1f));

    // Left Settings Card
    auto settingsBounds = juce::Rectangle<float> (12.0f, 12.0f, static_cast<float>(getWidth() - 24) * 0.54f, static_cast<float>(getHeight() - 24));
    g.setColour (juce::Colour (0xff21252b));
    g.fillRoundedRectangle (settingsBounds, 6.0f);
    g.setColour (juce::Colour (0xff2c313a));
    g.drawRoundedRectangle (settingsBounds, 6.0f, 1.0f);

    // Right Status Card
    auto statusBounds = juce::Rectangle<float> (settingsBounds.getRight() + 12.0f, 12.0f,
                                                static_cast<float>(getWidth()) - settingsBounds.getRight() - 24.0f,
                                                static_cast<float>(getHeight() - 24));
    g.setColour (juce::Colour (0xff21252b));
    g.fillRoundedRectangle (statusBounds, 6.0f);
    g.setColour (juce::Colour (0xff2c313a));
    g.drawRoundedRectangle (statusBounds, 6.0f, 1.0f);

    // Title
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    g.setColour (juce::Colour (0xff61afef));
    g.drawText ("GLIDE CONFIGURATION", settingsBounds.reduced (16.0f, 10.0f).removeFromTop (20.0f), juce::Justification::topLeft, true);

    // Live Progress Bar (Right card)
    float barX = statusBounds.getX() + 16.0f;
    float barY = statusBounds.getY() + 115.0f;
    float barW = statusBounds.getWidth() - 32.0f;
    float barH = 14.0f;

    g.setColour (juce::Colour (0xff181a1f));
    g.fillRoundedRectangle (barX, barY, barW, barH, 3.0f);
    g.setColour (juce::Colour (0xff3b4048));
    g.drawRoundedRectangle (barX, barY, barW, barH, 3.0f, 1.0f);

    if (currentProgress > 0.0f)
    {
        float fillW = barW * std::clamp (currentProgress, 0.02f, 1.0f);
        juce::Colour fillCol = isCurrentlyGliding ? juce::Colour (0xff00e676) : juce::Colour (0xff61afef);
        g.setColour (fillCol);
        g.fillRoundedRectangle (barX + 1.0f, barY + 1.0f, fillW - 2.0f, barH - 2.0f, 2.0f);
    }

    // Pitch Bend Bipolar Gauge Bar (Right card)
    float pbY = barY + 45.0f;
    g.setColour (juce::Colour (0xff181a1f));
    g.fillRoundedRectangle (barX, pbY, barW, barH, 3.0f);
    g.setColour (juce::Colour (0xff3b4048));
    g.drawRoundedRectangle (barX, pbY, barW, barH, 3.0f, 1.0f);

    // Center zero mark
    float centerX = barX + barW * 0.5f;
    g.setColour (juce::Colour (0xff5c6370));
    g.drawVerticalLine (static_cast<int>(centerX), pbY - 2.0f, pbY + barH + 2.0f);

    if (currentPitchBend != 0)
    {
        float bendRatio = std::clamp (static_cast<float>(currentPitchBend) / 8192.0f, -1.0f, 1.0f);
        float meterW = (barW * 0.5f) * std::abs (bendRatio);
        juce::Colour bendCol = bendRatio > 0.0f ? juce::Colour (0xffe5c07b) : juce::Colour (0xff61afef);
        g.setColour (bendCol);

        if (bendRatio > 0.0f)
            g.fillRoundedRectangle (centerX, pbY + 1.0f, meterW - 1.0f, barH - 2.0f, 2.0f);
        else
            g.fillRoundedRectangle (centerX - meterW, pbY + 1.0f, meterW, barH - 2.0f, 2.0f);
    }
}

void PortamentoTab::resized()
{
    int w = getWidth();

    int leftW = static_cast<int>((w - 24) * 0.54f);
    int lx = 26;
    int ly = 38;
    int innerW = leftW - 28;

    enablePortamento.setBounds (lx, ly, innerW, 24);
    ly += 28;

    glideModeLabel.setBounds (lx, ly, 100, 20);
    glideModeBox.setBounds (lx + 105, ly, innerW - 105, 24);
    ly += 28;

    pitchBendRangeLabel.setBounds (lx, ly, 150, 20);
    pitchBendRangeBox.setBounds (lx + 155, ly, innerW - 155, 24);
    ly += 28;

    glideCurveLabel.setBounds (lx, ly, 140, 20);
    glideCurveBox.setBounds (lx + 145, ly, innerW - 145, 24);
    ly += 28;

    glideTimeLabel.setBounds (lx, ly, 110, 20);
    glideTimeSlider.setBounds (lx + 115, ly, innerW - 115, 24);
    ly += 28;

    maxGapLabel.setBounds (lx, ly, 125, 20);
    maxGapSlider.setBounds (lx + 130, ly, innerW - 130, 24);
    ly += 28;

    requireCC65Toggle.setBounds (lx, ly, innerW, 22);
    ly += 24;

    legatoOnlyToggle.setBounds (lx, ly, innerW, 22);
    ly += 24;

    useCC5Toggle.setBounds (lx, ly, innerW, 22);
    ly += 26;

    stepIntervalLabel.setBounds (lx, ly, innerW, 18);
    ly += 20;
    stepIntervalBox.setBounds (lx, ly, innerW, 24);
    ly += 30;

    sendRPNButton.setBounds (lx, ly, innerW, 24);
    ly += 28;

    // Test buttons
    testUpButton.setBounds (lx, ly, (innerW - 8) / 2, 26);
    testDownButton.setBounds (lx + (innerW - 8) / 2 + 8, ly, (innerW - 8) / 2, 26);

    // Right card: Status
    int rx = 12 + leftW + 20;
    int rw = w - rx - 20;
    int ry = 22;

    statusHeaderLabel.setBounds (rx, ry, rw, 22);
    ry += 24;

    modeBadgeLabel.setBounds (rx, ry, rw, 20);
    ry += 26;

    transitionNotesLabel.setBounds (rx, ry, rw, 22);
    ry += 64; // past progress bar

    pitchBendGaugeLabel.setBounds (rx, ry, rw, 20);
    ry += 42; // past bend meter

    ccStatusLabel.setBounds (rx, ry, rw, 20);
    ry += 24;

    statsLabel.setBounds (rx, ry, rw, 20);
}

void PortamentoTab::timerCallback()
{
    const auto& live = processor.getPortamentoProcessor().getLiveState();

    isCurrentlyGliding = live.isGliding.load (std::memory_order_relaxed);
    currentProgress = live.progress.load (std::memory_order_relaxed);
    currentPitchBend = live.currentPitchBend.load (std::memory_order_relaxed);

    int src = live.sourceNote.load (std::memory_order_relaxed);
    int tgt = live.targetNote.load (std::memory_order_relaxed);
    int step = live.currentStepNote.load (std::memory_order_relaxed);

    int mode = processor.getPortamentoProcessor().getSettings().glideMode.load (std::memory_order_relaxed);
    if (mode == 1)
        modeBadgeLabel.setText ("Mode: Analogue 14-Bit Continuous Bend", juce::dontSendNotification);
    else
        modeBadgeLabel.setText ("Mode: Stepped Glissando (Notes)", juce::dontSendNotification);

    if (isCurrentlyGliding && src >= 0 && tgt >= 0)
    {
        juce::String str = "Gliding: " + juce::String (MidiMonitor::MidiLogEntry::getMidiNoteName (src))
                         + " (" + juce::String (src) + ") ➔ "
                         + juce::String (MidiMonitor::MidiLogEntry::getMidiNoteName (tgt))
                         + " (" + juce::String (tgt) + ")"
                         + " | Current: " + juce::String (MidiMonitor::MidiLogEntry::getMidiNoteName (step));
        transitionNotesLabel.setText (str, juce::dontSendNotification);
    }
    else if (src >= 0)
    {
        transitionNotesLabel.setText ("Sounding: " + juce::String (MidiMonitor::MidiLogEntry::getMidiNoteName (src))
                                      + " (" + juce::String (src) + ")", juce::dontSendNotification);
    }
    else
    {
        transitionNotesLabel.setText ("Idle (No notes held)", juce::dontSendNotification);
    }

    // Pitch bend readout
    int range = pitchBendRangeBox.getSelectedId();
    if (range <= 0) range = 12;
    double bendSemitones = (static_cast<double>(currentPitchBend) / 8192.0) * static_cast<double>(range);
    juce::String pbStr = "Pitch Wheel: ";
    if (currentPitchBend > 0) pbStr << "+";
    pbStr << currentPitchBend << " (";
    if (bendSemitones > 0) pbStr << "+";
    pbStr << juce::String (bendSemitones, 1) << " st)";
    pitchBendGaugeLabel.setText (pbStr, juce::dontSendNotification);

    bool cc65 = live.cc65Active.load (std::memory_order_relaxed);
    int cc5 = live.cc5Value.load (std::memory_order_relaxed);
    ccStatusLabel.setText ("CC65 Switch: " + juce::String (cc65 ? "ON (Active)" : "OFF")
                           + " | CC5 Portamento Time: " + juce::String (cc5), juce::dontSendNotification);

    uint32_t totalInjected = live.totalInjectedNotes.load (std::memory_order_relaxed);
    statsLabel.setText ("Total Injected Events: " + juce::String (totalInjected), juce::dontSendNotification);

    repaint();
}
