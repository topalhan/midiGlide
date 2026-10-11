#include "BreathRemapTab.h"
#include "PluginProcessor.h"

BreathRemapTab::BreathRemapTab (MidiMonitorAudioProcessor& p)
    : processor (p)
{
    auto& settings = processor.getBreathRemapProcessor().getSettings();

    // 1. Master Enable Toggle
    enableRemapper.setColour (juce::ToggleButton::textColourId, juce::Colour (0xff00e676));
    enableRemapper.setColour (juce::ToggleButton::tickColourId, juce::Colour (0xff00e676));
    enableRemapper.setToggleState (settings.enabled.load(), juce::dontSendNotification);
    enableRemapper.onClick = [this] { applySettingsToProcessor(); };
    addAndMakeVisible (enableRemapper);

    // 2. Source Controller Box
    sourceControllerBox.addItem ("CC #2 (Breath Controller - EWI standard)", 2);
    sourceControllerBox.addItem ("CC #11 (Expression)", 11);
    sourceControllerBox.addItem ("CC #1 (Modulation Wheel)", 1);
    sourceControllerBox.addItem ("CC #7 (Channel Volume)", 7);
    sourceControllerBox.addItem ("Channel Aftertouch (Channel Pressure)", 1000); // 1000 maps to -1
    int srcId = settings.sourceController.load();
    sourceControllerBox.setSelectedId (srcId == -1 ? 1000 : srcId, juce::dontSendNotification);
    sourceControllerBox.onChange = [this] { applySettingsToProcessor(); };
    addAndMakeVisible (sourceControllerBox);

    sourceControllerLabel.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    sourceControllerLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (sourceControllerLabel);

    // 3. Target Controller Box
    targetControllerBox.addItem ("CC #1 (Modulation Wheel - Target VST)", 1);
    targetControllerBox.addItem ("CC #11 (Expression)", 11);
    targetControllerBox.addItem ("CC #2 (Breath Controller)", 2);
    targetControllerBox.addItem ("CC #74 (Brightness / Filter Cutoff)", 74);
    targetControllerBox.addItem ("CC #7 (Channel Volume)", 7);
    targetControllerBox.setSelectedId (settings.targetController.load(), juce::dontSendNotification);
    targetControllerBox.onChange = [this] { applySettingsToProcessor(); };
    addAndMakeVisible (targetControllerBox);

    targetControllerLabel.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    targetControllerLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (targetControllerLabel);

    // 4. Routing Mode Box
    routingModeBox.addItem ("Remap (Replace: swallow source CC and output target CC)", 1);
    routingModeBox.addItem ("Mirror (Duplicate: output target CC AND pass source CC)", 2);
    routingModeBox.setSelectedId (settings.routingMode.load(), juce::dontSendNotification);
    routingModeBox.onChange = [this] { applySettingsToProcessor(); };
    addAndMakeVisible (routingModeBox);

    routingModeLabel.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    routingModeLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (routingModeLabel);

    // 5. Curve Profile Box
    curveProfileBox.addItem ("Linear (1:1 Direct Response)", 1);
    curveProfileBox.addItem ("Exponential (Expressive / Soft Attack)", 2);
    curveProfileBox.addItem ("Logarithmic (Punchy / Instant Attack)", 3);
    curveProfileBox.addItem ("S-Curve (Sigmoid / Acoustic Reed Feel)", 4);
    curveProfileBox.setSelectedId (settings.curveProfile.load(), juce::dontSendNotification);
    curveProfileBox.onChange = [this] { applySettingsToProcessor(); };
    addAndMakeVisible (curveProfileBox);

    curveProfileLabel.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    curveProfileLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (curveProfileLabel);

    // 6. Lip Rest Deadzone Slider
    deadzoneSlider.setRange (0.0, 30.0, 1.0);
    deadzoneSlider.setValue (settings.deadzone.load(), juce::dontSendNotification);
    deadzoneSlider.setTextValueSuffix (" / 127");
    deadzoneSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    deadzoneSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 65, 22);
    deadzoneSlider.setColour (juce::Slider::thumbColourId, juce::Colour (0xffe5c07b));
    deadzoneSlider.setColour (juce::Slider::trackColourId, juce::Colour (0xff3b4048));
    deadzoneSlider.onValueChange = [this] { applySettingsToProcessor(); };
    addAndMakeVisible (deadzoneSlider);

    deadzoneLabel.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    deadzoneLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (deadzoneLabel);

    // 7. Sensitivity / Gain Slider
    gainSlider.setRange (0.50, 2.00, 0.05);
    gainSlider.setValue (settings.gain.load(), juce::dontSendNotification);
    gainSlider.setTextValueSuffix ("x");
    gainSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    gainSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 65, 22);
    gainSlider.setColour (juce::Slider::thumbColourId, juce::Colour (0xff61afef));
    gainSlider.setColour (juce::Slider::trackColourId, juce::Colour (0xff3b4048));
    gainSlider.onValueChange = [this] { applySettingsToProcessor(); };
    addAndMakeVisible (gainSlider);

    gainLabel.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    gainLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (gainLabel);

    // 8. Min Output Limit Slider
    minOutputSlider.setRange (0.0, 127.0, 1.0);
    minOutputSlider.setValue (settings.minOutput.load(), juce::dontSendNotification);
    minOutputSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    minOutputSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 65, 22);
    minOutputSlider.setColour (juce::Slider::thumbColourId, juce::Colour (0xff98c379));
    minOutputSlider.setColour (juce::Slider::trackColourId, juce::Colour (0xff3b4048));
    minOutputSlider.onValueChange = [this] {
        if (minOutputSlider.getValue() > maxOutputSlider.getValue())
            maxOutputSlider.setValue (minOutputSlider.getValue(), juce::dontSendNotification);
        applySettingsToProcessor();
    };
    addAndMakeVisible (minOutputSlider);

    minOutputLabel.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    minOutputLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (minOutputLabel);

    // 9. Max Output Limit Slider
    maxOutputSlider.setRange (0.0, 127.0, 1.0);
    maxOutputSlider.setValue (settings.maxOutput.load(), juce::dontSendNotification);
    maxOutputSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    maxOutputSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 65, 22);
    maxOutputSlider.setColour (juce::Slider::thumbColourId, juce::Colour (0xff98c379));
    maxOutputSlider.setColour (juce::Slider::trackColourId, juce::Colour (0xff3b4048));
    maxOutputSlider.onValueChange = [this] {
        if (maxOutputSlider.getValue() < minOutputSlider.getValue())
            minOutputSlider.setValue (maxOutputSlider.getValue(), juce::dontSendNotification);
        applySettingsToProcessor();
    };
    addAndMakeVisible (maxOutputSlider);

    maxOutputLabel.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    maxOutputLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (maxOutputLabel);

    // 10. Jitter Filter / Smoothing Slider
    smoothingSlider.setRange (0.0, 85.0, 1.0);
    smoothingSlider.setValue (settings.smoothing.load() * 100.0f, juce::dontSendNotification);
    smoothingSlider.setTextValueSuffix (" %");
    smoothingSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    smoothingSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 65, 22);
    smoothingSlider.setColour (juce::Slider::thumbColourId, juce::Colour (0xffc678dd));
    smoothingSlider.setColour (juce::Slider::trackColourId, juce::Colour (0xff3b4048));
    smoothingSlider.onValueChange = [this] { applySettingsToProcessor(); };
    addAndMakeVisible (smoothingSlider);

    smoothingLabel.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    smoothingLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (smoothingLabel);

    // 11. Invert Direction Toggle
    invertToggle.setColour (juce::ToggleButton::textColourId, juce::Colour (0xffabb2bf));
    invertToggle.setToggleState (settings.invert.load(), juce::dontSendNotification);
    invertToggle.onClick = [this] { applySettingsToProcessor(); };
    addAndMakeVisible (invertToggle);

    // 12. Actions
    testBlowButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff2c3e50));
    testBlowButton.onClick = [this] { triggerTestBreathPulse(); };
    addAndMakeVisible (testBlowButton);

    resetDefaultsButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff3b4048));
    resetDefaultsButton.onClick = [this] { resetDefaults(); };
    addAndMakeVisible (resetDefaultsButton);

    // 13. Status Header & Badges
    statusHeaderLabel.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    statusHeaderLabel.setColour (juce::Label::textColourId, juce::Colour (0xff61afef));
    addAndMakeVisible (statusHeaderLabel);

    routeBadgeLabel.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    routeBadgeLabel.setColour (juce::Label::textColourId, juce::Colour (0xff98c379));
    routeBadgeLabel.setText ("MAPPING: CC #2 (Breath) -> CC #1 (Modulation)", juce::dontSendNotification);
    addAndMakeVisible (routeBadgeLabel);

    inValueLabel.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    inValueLabel.setColour (juce::Label::textColourId, juce::Colour (0xff61afef));
    inValueLabel.setText ("INCOMING BREATH (CC #2): 0 / 127", juce::dontSendNotification);
    addAndMakeVisible (inValueLabel);

    outValueLabel.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    outValueLabel.setColour (juce::Label::textColourId, juce::Colour (0xffe5c07b));
    outValueLabel.setText ("OUTGOING MODULATION (CC #1): 0 / 127", juce::dontSendNotification);
    addAndMakeVisible (outValueLabel);

    curveInfoLabel.setFont (juce::FontOptions (11.0f));
    curveInfoLabel.setColour (juce::Label::textColourId, juce::Colour (0xffabb2bf));
    curveInfoLabel.setText ("Transfer Function: Linear | Gain: 1.00x | Deadzone: 0", juce::dontSendNotification);
    addAndMakeVisible (curveInfoLabel);

    statsLabel.setFont (juce::FontOptions (11.0f));
    statsLabel.setColour (juce::Label::textColourId, juce::Colour (0xff5c6370));
    statsLabel.setText ("Remapped Packets: 0 | Channel: 1", juce::dontSendNotification);
    addAndMakeVisible (statsLabel);

    applySettingsToProcessor();
    startTimerHz (30);
}

BreathRemapTab::~BreathRemapTab()
{
    stopTimer();
}

void BreathRemapTab::resetDefaults()
{
    enableRemapper.setToggleState (true, juce::dontSendNotification);
    sourceControllerBox.setSelectedId (2, juce::dontSendNotification);
    targetControllerBox.setSelectedId (1, juce::dontSendNotification);
    routingModeBox.setSelectedId (1, juce::dontSendNotification);
    curveProfileBox.setSelectedId (1, juce::dontSendNotification);
    deadzoneSlider.setValue (0.0, juce::dontSendNotification);
    gainSlider.setValue (1.0, juce::dontSendNotification);
    minOutputSlider.setValue (0.0, juce::dontSendNotification);
    maxOutputSlider.setValue (127.0, juce::dontSendNotification);
    smoothingSlider.setValue (0.0, juce::dontSendNotification);
    invertToggle.setToggleState (false, juce::dontSendNotification);

    applySettingsToProcessor();
}

void BreathRemapTab::applySettingsToProcessor()
{
    auto& settings = processor.getBreathRemapProcessor().getSettings();
    settings.enabled.store (enableRemapper.getToggleState(), std::memory_order_relaxed);

    int srcSel = sourceControllerBox.getSelectedId();
    settings.sourceController.store (srcSel == 1000 ? -1 : srcSel, std::memory_order_relaxed);

    settings.targetController.store (targetControllerBox.getSelectedId(), std::memory_order_relaxed);
    settings.routingMode.store (routingModeBox.getSelectedId(), std::memory_order_relaxed);
    settings.curveProfile.store (curveProfileBox.getSelectedId(), std::memory_order_relaxed);
    settings.deadzone.store (static_cast<int>(deadzoneSlider.getValue()), std::memory_order_relaxed);
    settings.gain.store (static_cast<float>(gainSlider.getValue()), std::memory_order_relaxed);
    settings.minOutput.store (static_cast<int>(minOutputSlider.getValue()), std::memory_order_relaxed);
    settings.maxOutput.store (static_cast<int>(maxOutputSlider.getValue()), std::memory_order_relaxed);
    settings.smoothing.store (static_cast<float>(smoothingSlider.getValue() / 100.0), std::memory_order_relaxed);
    settings.invert.store (invertToggle.getToggleState(), std::memory_order_relaxed);

    // Update badge description
    juce::String srcName = (srcSel == 1000) ? "Aftertouch" : ("CC #" + juce::String (srcSel));
    juce::String tgtName = "CC #" + juce::String (targetControllerBox.getSelectedId());
    if (enableRemapper.getToggleState())
        routeBadgeLabel.setText ("MAPPING ACTIVE: " + srcName + " -> " + tgtName, juce::dontSendNotification);
    else
        routeBadgeLabel.setText ("REMAPPER BYPASSED (Passthrough)", juce::dontSendNotification);

    juce::String cName = curveProfileBox.getText();
    curveInfoLabel.setText ("Curve: " + cName + " | Gain: " + juce::String (gainSlider.getValue(), 2)
                            + "x | Deadzone: " + juce::String (static_cast<int>(deadzoneSlider.getValue())),
                            juce::dontSendNotification);

    repaint();
}

void BreathRemapTab::triggerTestBreathPulse()
{
    testPulseCounter = 0;
}

void BreathRemapTab::timerCallback()
{
    const auto& live = processor.getBreathRemapProcessor().getLiveState();
    currentInVal = live.lastInValue.load (std::memory_order_relaxed);
    currentOutVal = live.lastOutValue.load (std::memory_order_relaxed);
    isActive = live.isActive.load (std::memory_order_relaxed);
    totalPackets = live.totalRemappedCount.load (std::memory_order_relaxed);
    int ch = live.lastChannel.load (std::memory_order_relaxed);

    int srcSel = sourceControllerBox.getSelectedId();
    juce::String srcName = (srcSel == 1000) ? "Aftertouch" : ("CC #" + juce::String (srcSel));
    juce::String tgtName = "CC #" + juce::String (targetControllerBox.getSelectedId());

    inValueLabel.setText ("INCOMING (" + srcName + "): " + juce::String (currentInVal) + " / 127", juce::dontSendNotification);
    outValueLabel.setText ("OUTGOING (" + tgtName + "): " + juce::String (currentOutVal) + " / 127", juce::dontSendNotification);
    statsLabel.setText ("Remapped Packets: " + juce::String (totalPackets) + " | Channel: " + juce::String (ch), juce::dontSendNotification);

    // Handle test breath pulse simulation
    if (testPulseCounter >= 0)
    {
        // 32-step envelope: 0 -> 127 -> 0
        int val = 0;
        if (testPulseCounter <= 16)
            val = static_cast<int>(std::round ((testPulseCounter / 16.0f) * 127.0f));
        else if (testPulseCounter <= 32)
            val = static_cast<int>(std::round (((32 - testPulseCounter) / 16.0f) * 127.0f));

        int src = processor.getBreathRemapProcessor().getSourceController();
        if (src == -1)
            processor.injectMidiMessage (juce::MidiMessage::channelPressureChange (1, val));
        else
            processor.injectMidiMessage (juce::MidiMessage::controllerEvent (1, src, val));

        ++testPulseCounter;
        if (testPulseCounter > 32)
            testPulseCounter = -1;
    }

    repaint();
}

void BreathRemapTab::drawCurvePreview (juce::Graphics& g, juce::Rectangle<float> bounds)
{
    // Background card for curve graph
    g.setColour (juce::Colour (0xff181a1f));
    g.fillRoundedRectangle (bounds, 4.0f);
    g.setColour (juce::Colour (0xff3b4048));
    g.drawRoundedRectangle (bounds, 4.0f, 1.0f);

    // Inner plot area
    auto plotArea = bounds.reduced (12.0f, 12.0f);

    // Grid lines
    g.setColour (juce::Colour (0xff252930));
    g.drawHorizontalLine (static_cast<int>(plotArea.getY() + plotArea.getHeight() * 0.5f), plotArea.getX(), plotArea.getRight());
    g.drawVerticalLine (static_cast<int>(plotArea.getX() + plotArea.getWidth() * 0.5f), plotArea.getY(), plotArea.getBottom());

    // Deadzone guide line
    float deadNorm = static_cast<float>(deadzoneSlider.getValue()) / 127.0f;
    if (deadNorm > 0.0f)
    {
        float deadX = plotArea.getX() + plotArea.getWidth() * deadNorm;
        g.setColour (juce::Colour (0x66e5c07b));
        g.drawVerticalLine (static_cast<int>(deadX), plotArea.getY(), plotArea.getBottom());
    }

    // Mathematical transfer curve
    const auto& remapProc = processor.getBreathRemapProcessor();
    juce::Path curvePath;
    const int numSteps = 64;

    for (int i = 0; i <= numSteps; ++i)
    {
        float inNorm = static_cast<float>(i) / static_cast<float>(numSteps);
        float outNorm = remapProc.evaluateTransferFunction (inNorm);

        float px = plotArea.getX() + plotArea.getWidth() * inNorm;
        float py = plotArea.getBottom() - plotArea.getHeight() * outNorm;

        if (i == 0)
            curvePath.startNewSubPath (px, py);
        else
            curvePath.lineTo (px, py);
    }

    g.setColour (juce::Colour (0xff61afef));
    g.strokePath (curvePath, juce::PathStrokeType (2.0f));

    // Live operating tracking dot
    float liveInNorm = static_cast<float>(currentInVal) / 127.0f;
    float liveOutNorm = static_cast<float>(currentOutVal) / 127.0f;
    float dotX = plotArea.getX() + plotArea.getWidth() * liveInNorm;
    float dotY = plotArea.getBottom() - plotArea.getHeight() * liveOutNorm;

    auto dotBounds = juce::Rectangle<float> (dotX - 5.0f, dotY - 5.0f, 10.0f, 10.0f);
    g.setColour (juce::Colour (0xff00e676));
    g.fillEllipse (dotBounds);
    g.setColour (juce::Colour (0x6600e676));
    g.drawEllipse (dotBounds.expanded (3.0f), 2.0f);
}

void BreathRemapTab::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff181a1f));

    float w = static_cast<float>(getWidth());
    float h = static_cast<float>(getHeight());

    // Left Configuration Card (54% width)
    auto settingsBounds = juce::Rectangle<float> (12.0f, 12.0f, (w - 24.0f) * 0.54f, h - 24.0f);
    g.setColour (juce::Colour (0xff21252b));
    g.fillRoundedRectangle (settingsBounds, 6.0f);
    g.setColour (juce::Colour (0xff2c313a));
    g.drawRoundedRectangle (settingsBounds, 6.0f, 1.0f);

    // Left Title
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    g.setColour (juce::Colour (0xff61afef));
    g.drawText ("BREATH & CC ROUTING CONFIGURATION", settingsBounds.reduced (16.0f, 10.0f).removeFromTop (20.0f), juce::Justification::topLeft, true);

    // Right Live Deck Card (46% width)
    auto statusBounds = juce::Rectangle<float> (settingsBounds.getRight() + 12.0f, 12.0f,
                                                w - settingsBounds.getRight() - 24.0f,
                                                h - 24.0f);
    g.setColour (juce::Colour (0xff21252b));
    g.fillRoundedRectangle (statusBounds, 6.0f);
    g.setColour (juce::Colour (0xff2c313a));
    g.drawRoundedRectangle (statusBounds, 6.0f, 1.0f);

    // Right Title
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    g.setColour (juce::Colour (0xff61afef));
    g.drawText ("LIVE BREATH & MODULATION MONITOR", statusBounds.reduced (16.0f, 10.0f).removeFromTop (20.0f), juce::Justification::topLeft, true);

    // Live Meter 1: Incoming Breath (Cyan / Green)
    float meterX = statusBounds.getX() + 16.0f;
    float meterW = statusBounds.getWidth() - 32.0f;
    float meterH = 14.0f;
    float inMeterY = statusBounds.getY() + 75.0f;

    g.setColour (juce::Colour (0xff181a1f));
    g.fillRoundedRectangle (meterX, inMeterY, meterW, meterH, 3.0f);
    g.setColour (juce::Colour (0xff3b4048));
    g.drawRoundedRectangle (meterX, inMeterY, meterW, meterH, 3.0f, 1.0f);

    if (currentInVal > 0)
    {
        float inRatio = std::clamp (currentInVal / 127.0f, 0.01f, 1.0f);
        g.setColour (isActive ? juce::Colour (0xff00e676) : juce::Colour (0xff61afef));
        g.fillRoundedRectangle (meterX + 1.0f, inMeterY + 1.0f, (meterW - 2.0f) * inRatio, meterH - 2.0f, 2.0f);
    }

    // Live Meter 2: Outgoing Modulation (Gold / Amber)
    float outMeterY = inMeterY + 45.0f;
    g.setColour (juce::Colour (0xff181a1f));
    g.fillRoundedRectangle (meterX, outMeterY, meterW, meterH, 3.0f);
    g.setColour (juce::Colour (0xff3b4048));
    g.drawRoundedRectangle (meterX, outMeterY, meterW, meterH, 3.0f, 1.0f);

    if (currentOutVal > 0)
    {
        float outRatio = std::clamp (currentOutVal / 127.0f, 0.01f, 1.0f);
        g.setColour (juce::Colour (0xffe5c07b));
        g.fillRoundedRectangle (meterX + 1.0f, outMeterY + 1.0f, (meterW - 2.0f) * outRatio, meterH - 2.0f, 2.0f);
    }

    // Interactive Curve Preview Graph
    float graphY = outMeterY + 45.0f;
    float graphH = statusBounds.getBottom() - graphY - 110.0f;
    if (graphH > 70.0f)
    {
        drawCurvePreview (g, juce::Rectangle<float> (meterX, graphY, meterW, graphH));
    }
}

void BreathRemapTab::resized()
{
    int w = getWidth();

    int leftW = static_cast<int>((w - 24) * 0.54f);
    int lx = 26;
    int ly = 38;
    int innerW = leftW - 28;

    enableRemapper.setBounds (lx, ly, innerW, 24);
    ly += 28;

    // Routing group
    sourceControllerLabel.setBounds (lx, ly, 150, 20);
    sourceControllerBox.setBounds (lx + 155, ly, innerW - 155, 24);
    ly += 28;

    targetControllerLabel.setBounds (lx, ly, 150, 20);
    targetControllerBox.setBounds (lx + 155, ly, innerW - 155, 24);
    ly += 28;

    routingModeLabel.setBounds (lx, ly, 150, 20);
    routingModeBox.setBounds (lx + 155, ly, innerW - 155, 24);
    ly += 30;

    // Calibration group
    curveProfileLabel.setBounds (lx, ly, 150, 20);
    curveProfileBox.setBounds (lx + 155, ly, innerW - 155, 24);
    ly += 28;

    deadzoneLabel.setBounds (lx, ly, 150, 20);
    deadzoneSlider.setBounds (lx + 155, ly, innerW - 155, 24);
    ly += 28;

    gainLabel.setBounds (lx, ly, 150, 20);
    gainSlider.setBounds (lx + 155, ly, innerW - 155, 24);
    ly += 28;

    minOutputLabel.setBounds (lx, ly, 150, 20);
    minOutputSlider.setBounds (lx + 155, ly, innerW - 155, 24);
    ly += 28;

    maxOutputLabel.setBounds (lx, ly, 150, 20);
    maxOutputSlider.setBounds (lx + 155, ly, innerW - 155, 24);
    ly += 28;

    smoothingLabel.setBounds (lx, ly, 150, 20);
    smoothingSlider.setBounds (lx + 155, ly, innerW - 155, 24);
    ly += 28;

    invertToggle.setBounds (lx, ly, innerW, 22);
    ly += 28;

    // Buttons
    resetDefaultsButton.setBounds (lx, ly, 190, 24);

    // Right Live Monitor deck layout
    int rx = leftW + 24;
    int ry = 36;
    int rightW = w - rx - 16;

    routeBadgeLabel.setBounds (rx, ry, rightW, 20);
    ry += 22;

    inValueLabel.setBounds (rx, ry, rightW, 16);
    ry += 45; // skips over inMeter (height 14 drawn in paint)

    outValueLabel.setBounds (rx, ry, rightW, 16);

    // Bottom items in right card
    int btmY = getHeight() - 95;
    curveInfoLabel.setBounds (rx, btmY, rightW, 18);
    statsLabel.setBounds (rx, btmY + 20, rightW, 18);
    testBlowButton.setBounds (rx, btmY + 44, std::min (260, rightW), 24);
}
