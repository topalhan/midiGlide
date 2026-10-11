#include "MicrotuningTab.h"
#include "PluginProcessor.h"

//==============================================================================
// KeyStripComponent Implementation
//==============================================================================
MicrotuningTab::KeyStripComponent::KeyStripComponent()
{
    // Slider
    centSlider.setSliderStyle (juce::Slider::LinearVertical);
    centSlider.setRange (-100.0, 100.0, 0.5);
    centSlider.setValue (0.0, juce::dontSendNotification);
    centSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 56, 18);
    centSlider.setTextValueSuffix ("c");
    centSlider.setColour (juce::Slider::trackColourId, juce::Colour (0xff2c313a));
    centSlider.setColour (juce::Slider::thumbColourId, juce::Colour (0xff61afef));
    centSlider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colour (0xff2c313a));
    centSlider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xff181a1f));
    centSlider.setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);

    centSlider.onValueChange = [this]
    {
        float val = static_cast<float>(centSlider.getValue());
        double commas = static_cast<double>(val) / 22.6415;
        commaLabel.setText (juce::String (commas, 1) + "k", juce::dontSendNotification);

        // Adjust thumb color based on value
        if (std::abs (val) < 0.1f)
            centSlider.setColour (juce::Slider::thumbColourId, juce::Colour (0xff98c379));
        else if (val < 0.0f)
            centSlider.setColour (juce::Slider::thumbColourId, juce::Colour (0xffe06c75)); // Flat: coral/red
        else
            centSlider.setColour (juce::Slider::thumbColourId, juce::Colour (0xffc678dd)); // Sharp: purple

        if (onCentsChanged)
            onCentsChanged (chromaIndex, val);
    };
    addAndMakeVisible (centSlider);

    // Note name label
    nameLabel.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    nameLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (nameLabel);

    // Turkish Perde label
    perdeLabel.setFont (juce::FontOptions (10.0f));
    perdeLabel.setJustificationType (juce::Justification::centred);
    perdeLabel.setColour (juce::Label::textColourId, juce::Colour (0xffabb2bf));
    addAndMakeVisible (perdeLabel);

    // Comma label
    commaLabel.setFont (juce::FontOptions (10.0f));
    commaLabel.setJustificationType (juce::Justification::centred);
    commaLabel.setColour (juce::Label::textColourId, juce::Colour (0xffe5c07b));
    commaLabel.setText ("0.0k", juce::dontSendNotification);
    addAndMakeVisible (commaLabel);

    // Zero button
    zeroButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff2c313a));
    zeroButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xffabb2bf));
    zeroButton.onClick = [this]
    {
        centSlider.setValue (0.0, juce::sendNotification);
    };
    addAndMakeVisible (zeroButton);
}

void MicrotuningTab::KeyStripComponent::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Background card for key
    juce::Colour bgCol = isBlackKey ? juce::Colour (0xff1e2227) : juce::Colour (0xff252930);
    if (isScaleNote)
        bgCol = bgCol.brighter (0.08f);

    g.setColour (bgCol);
    g.fillRoundedRectangle (bounds, 5.0f);

    // Border: highlight if scale note or has special role
    juce::Colour borderCol = juce::Colour (0xff2c313a);
    float borderWidth = 1.0f;

    if (degreeRole == "KARAR")
    {
        borderCol = juce::Colour (0xffe5c07b); // Gold for tonic
        borderWidth = 2.0f;
    }
    else if (degreeRole == "GUCLU")
    {
        borderCol = juce::Colour (0xff61afef); // Blue for dominant
        borderWidth = 1.5f;
    }
    else if (std::abs (centSlider.getValue()) > 1.0)
    {
        borderCol = juce::Colour (0xffe06c75); // Red/Coral for microtone
        borderWidth = 1.2f;
    }

    g.setColour (borderCol);
    g.drawRoundedRectangle (bounds.reduced (0.5f), 5.0f, borderWidth);

    // Header badge area
    auto headerRect = bounds.removeFromTop (24.0f);
    juce::Colour headerBg = isBlackKey ? juce::Colour (0xff16181c) : juce::Colour (0xff323742);
    if (degreeRole == "KARAR")
        headerBg = juce::Colour (0x33e5c07b);
    else if (degreeRole == "GUCLU")
        headerBg = juce::Colour (0x3361afef);

    g.setColour (headerBg);
    g.fillRoundedRectangle (headerRect.reduced (1.0f), 4.0f);

    // Role badge text if applicable
    if (degreeRole.isNotEmpty())
    {
        g.setFont (juce::FontOptions (8.5f, juce::Font::bold));
        bool isKarar = (degreeRole == "KARAR");
        juce::Colour badgeCol = isKarar ? juce::Colour (0xffe5c07b) : juce::Colour (0xff61afef);
        g.setColour (badgeCol);
        juce::String roleText = isKarar ? "KARAR" : juce::String::fromUTF8 ("GÜÇLÜ");
        g.drawText (roleText, getLocalBounds().removeFromBottom (16).toFloat(), juce::Justification::centred, false);
    }
}

void MicrotuningTab::KeyStripComponent::resized()
{
    int w = getWidth();
    int h = getHeight();

    nameLabel.setBounds (0, 2, w, 20);
    perdeLabel.setBounds (0, 24, w, 14);
    commaLabel.setBounds (0, 38, w, 14);

    int bottomMargin = degreeRole.isNotEmpty() ? 34 : 20;
    int sliderHeight = h - 60 - bottomMargin;
    sliderHeight = std::max (40, sliderHeight);

    centSlider.setBounds (2, 54, w - 4, sliderHeight);
    zeroButton.setBounds (w / 2 - 12, 54 + sliderHeight + 2, 24, 14);
}

void MicrotuningTab::KeyStripComponent::updateDisplay (float cents, const juce::String& perde, const juce::String& role, bool inScale)
{
    perdeName = perde;
    degreeRole = role;
    isScaleNote = inScale;

    perdeLabel.setText (perdeName, juce::dontSendNotification);
    centSlider.setValue (cents, juce::dontSendNotification);

    double commas = static_cast<double>(cents) / 22.6415;
    commaLabel.setText (juce::String (commas, 1) + "k", juce::dontSendNotification);

    if (std::abs (cents) < 0.1f)
        centSlider.setColour (juce::Slider::thumbColourId, juce::Colour (0xff98c379));
    else if (cents < 0.0f)
        centSlider.setColour (juce::Slider::thumbColourId, juce::Colour (0xffe06c75));
    else
        centSlider.setColour (juce::Slider::thumbColourId, juce::Colour (0xffc678dd));

    repaint();
}

//==============================================================================
// MicrotuningTab Implementation
//==============================================================================
MicrotuningTab::MicrotuningTab (MidiMonitorAudioProcessor& p)
    : processor (p)
{
    auto& mtProcessor = processor.getMicrotuningProcessor();
    auto& settings = mtProcessor.getSettings();

    // Enable Toggle
    enableMicrotuning.setColour (juce::ToggleButton::textColourId, juce::Colour (0xff98c379));
    enableMicrotuning.setToggleState (settings.enabled.load(), juce::dontSendNotification);
    enableMicrotuning.onClick = [this] { applySettingsFromUI(); };
    addAndMakeVisible (enableMicrotuning);

    // Makam Selector Box
    const auto& makamData = MidiMonitor::getAllMakamData();
    for (size_t i = 0; i < makamData.size(); ++i)
    {
        makamSelectorBox.addItem (juce::String (static_cast<int>(i) + 1) + ". " +
                                  makamData[i].getTurkishName() + juce::String::fromUTF8 (" Makamı (") + makamData[i].name + ")",
                                  static_cast<int>(i) + 1);
    }
    makamSelectorBox.setSelectedId (settings.makamId.load() + 1, juce::dontSendNotification);
    makamSelectorBox.onChange = [this]
    {
        // When Makam changes, update default Karar as well!
        int mIdx = makamSelectorBox.getSelectedId() - 1;
        const auto& allData = MidiMonitor::getAllMakamData();
        if (mIdx >= 0 && mIdx < static_cast<int>(allData.size()))
        {
            rootKeyBox.setSelectedId (allData[static_cast<size_t>(mIdx)].defaultKarar + 1, juce::dontSendNotification);
        }
        applySettingsFromUI();
    };
    addAndMakeVisible (makamSelectorBox);

    makamSelectorLabel.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    makamSelectorLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (makamSelectorLabel);

    // Root Key Box (Karar)
    static const char* noteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    for (int i = 0; i < 12; ++i)
        rootKeyBox.addItem (juce::String (noteNames[i]), i + 1);

    rootKeyBox.setSelectedId (settings.rootNote.load() + 1, juce::dontSendNotification);
    rootKeyBox.onChange = [this] { applySettingsFromUI(); };
    addAndMakeVisible (rootKeyBox);

    rootKeyLabel.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    rootKeyLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (rootKeyLabel);

    defaultKararButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff2c3e50));
    defaultKararButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xffe5c07b));
    defaultKararButton.onClick = [this]
    {
        int mIdx = makamSelectorBox.getSelectedId() - 1;
        const auto& allData = MidiMonitor::getAllMakamData();
        if (mIdx >= 0 && mIdx < static_cast<int>(allData.size()))
        {
            rootKeyBox.setSelectedId (allData[static_cast<size_t>(mIdx)].defaultKarar + 1, juce::dontSendNotification);
            applySettingsFromUI();
        }
    };
    addAndMakeVisible (defaultKararButton);

    // Tuning Mode Selector
    tuningModeBox.addItem (juce::String::fromUTF8 ("Authentic Performance (İcra / Sahne)"), 1);
    tuningModeBox.addItem ("53-Holdrian Comma (Arel-Ezgi Nazariyat)", 2);
    tuningModeBox.addItem ("Custom User Tuning", 3);
    tuningModeBox.setSelectedId (settings.tuningMode.load() + 1, juce::dontSendNotification);
    tuningModeBox.onChange = [this] { applySettingsFromUI(); };
    addAndMakeVisible (tuningModeBox);

    tuningModeLabel.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    tuningModeLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (tuningModeLabel);

    // Pitch Bend Range
    pitchBendRangeBox.addItem ("+/- 12 Semitones (1 Octave - Standard)", 12);
    pitchBendRangeBox.addItem ("+/- 2 Semitones (Whole Tone)", 2);
    pitchBendRangeBox.addItem ("+/- 24 Semitones (2 Octaves)", 24);
    pitchBendRangeBox.addItem ("+/- 48 Semitones (4 Octaves)", 48);
    pitchBendRangeBox.setSelectedId (settings.pitchBendRange.load(), juce::dontSendNotification);
    pitchBendRangeBox.onChange = [this] { applySettingsFromUI(); };
    addAndMakeVisible (pitchBendRangeBox);

    pitchBendRangeLabel.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    pitchBendRangeLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (pitchBendRangeLabel);

    // Send RPN Button
    sendRPNButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff34495e));
    sendRPNButton.setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    sendRPNButton.onClick = [this] { sendPitchBendRPN(); };
    addAndMakeVisible (sendRPNButton);

    // Makam Information Banner
    makamTitleLabel.setFont (juce::FontOptions (16.0f, juce::Font::bold));
    makamTitleLabel.setColour (juce::Label::textColourId, juce::Colour (0xff61afef));
    addAndMakeVisible (makamTitleLabel);

    makamDetailsLabel.setFont (juce::FontOptions (12.0f));
    makamDetailsLabel.setColour (juce::Label::textColourId, juce::Colour (0xffe5c07b));
    addAndMakeVisible (makamDetailsLabel);

    makamDescriptionLabel.setFont (juce::FontOptions (11.5f));
    makamDescriptionLabel.setColour (juce::Label::textColourId, juce::Colour (0xffabb2bf));
    addAndMakeVisible (makamDescriptionLabel);

    // Setup 12 Key Strips
    static const bool blackKeys[12] = { false, true, false, true, false, false, true, false, true, false, true, false };
    for (int i = 0; i < 12; ++i)
    {
        keyStrips[static_cast<size_t>(i)] = std::make_unique<KeyStripComponent>();
        auto* strip = keyStrips[static_cast<size_t>(i)].get();
        strip->chromaIndex = i;
        strip->isBlackKey = blackKeys[i];
        strip->noteName = noteNames[i];
        strip->nameLabel.setText (noteNames[i], juce::dontSendNotification);
        strip->nameLabel.setColour (juce::Label::textColourId, blackKeys[i] ? juce::Colour (0xffe5c07b) : juce::Colours::white);

        strip->onCentsChanged = [this] (int chroma, float cents)
        {
            processor.getMicrotuningProcessor().setCustomChromaOffset (chroma, cents);
            tuningModeBox.setSelectedId (3, juce::dontSendNotification); // Switch to Custom
        };

        addAndMakeVisible (strip);
    }

    // Reset Buttons
    resetPresetButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff2c313a));
    resetPresetButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xff98c379));
    resetPresetButton.onClick = [this]
    {
        int mIdx = makamSelectorBox.getSelectedId() - 1;
        int rKey = rootKeyBox.getSelectedId() - 1;
        int tMode = tuningModeBox.getSelectedId() - 1;
        if (tMode > 1) tMode = 0;
        tuningModeBox.setSelectedId (tMode + 1, juce::dontSendNotification);
        processor.getMicrotuningProcessor().applyPreset (static_cast<MidiMonitor::TurkishMakamId>(mIdx),
                                                         rKey,
                                                         static_cast<MidiMonitor::TuningMode>(tMode));
        refreshKeyStrips();
    };
    addAndMakeVisible (resetPresetButton);

    allZeroButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff2c313a));
    allZeroButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xffabb2bf));
    allZeroButton.onClick = [this]
    {
        processor.getMicrotuningProcessor().resetAllToEqualTemperament();
        tuningModeBox.setSelectedId (3, juce::dontSendNotification);
        refreshKeyStrips();
    };
    addAndMakeVisible (allZeroButton);

    // Audition Test Buttons
    testKararButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff2c3e50));
    testKararButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xffe5c07b));
    testKararButton.onClick = [this]
    {
        int rKey = rootKeyBox.getSelectedId() - 1;
        playAuditionNote (60 + rKey, 800); // Middle C octave + root
    };
    addAndMakeVisible (testKararButton);

    testMicrotoneButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff4b2b36));
    testMicrotoneButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xffe06c75));
    testMicrotoneButton.onClick = [this]
    {
        // Find the most prominent microtonal note in the makam
        int mIdx = makamSelectorBox.getSelectedId() - 1;
        int rKey = rootKeyBox.getSelectedId() - 1;
        int microRel = 2; // Default 2nd degree (Segah)
        if (mIdx == static_cast<int>(MidiMonitor::TurkishMakamId::Rast))
            microRel = 4; // Segah is 3rd degree in Rast (4 semitones)
        else if (mIdx == static_cast<int>(MidiMonitor::TurkishMakamId::Segah))
            microRel = 0; // Segah is tonic!
        else if (mIdx == static_cast<int>(MidiMonitor::TurkishMakamId::Hicaz))
            microRel = 1; // Dik Kürdi is 1 semitone
        else if (mIdx == static_cast<int>(MidiMonitor::TurkishMakamId::Saba))
            microRel = 5; // Saba D flat

        playAuditionNote (60 + rKey + microRel, 900);
    };
    addAndMakeVisible (testMicrotoneButton);

    testScaleButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff2c3e50));
    testScaleButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xff61afef));
    testScaleButton.onClick = [this] { playAuditionScale(); };
    addAndMakeVisible (testScaleButton);

    stopAuditionButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff3b2020));
    stopAuditionButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xffe06c75));
    stopAuditionButton.onClick = [this]
    {
        for (int ch = 1; ch <= 16; ++ch)
        {
            processor.injectMidiMessage (juce::MidiMessage::allNotesOff (ch));
            processor.injectMidiMessage (juce::MidiMessage::pitchWheel (ch, 8192));
        }
    };
    addAndMakeVisible (stopAuditionButton);

    // Live Monitoring Elements
    liveStatusHeaderLabel.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    liveStatusHeaderLabel.setColour (juce::Label::textColourId, juce::Colour (0xff61afef));
    addAndMakeVisible (liveStatusHeaderLabel);

    liveNoteLabel.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    liveNoteLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    liveNoteLabel.setText ("Last Note: None", juce::dontSendNotification);
    addAndMakeVisible (liveNoteLabel);

    liveCentsLabel.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    liveCentsLabel.setColour (juce::Label::textColourId, juce::Colour (0xffe06c75));
    liveCentsLabel.setText ("Offset: 0.0 cents (0.0 koma)", juce::dontSendNotification);
    addAndMakeVisible (liveCentsLabel);

    livePitchWheelLabel.setFont (juce::FontOptions (12.0f));
    livePitchWheelLabel.setColour (juce::Label::textColourId, juce::Colour (0xffe5c07b));
    livePitchWheelLabel.setText ("Pitch Bend: 8192 (Center)", juce::dontSendNotification);
    addAndMakeVisible (livePitchWheelLabel);

    liveNotesCounterLabel.setFont (juce::FontOptions (12.0f));
    liveNotesCounterLabel.setColour (juce::Label::textColourId, juce::Colour (0xff98c379));
    liveNotesCounterLabel.setText ("Tuned Notes: 0", juce::dontSendNotification);
    addAndMakeVisible (liveNotesCounterLabel);

    // Initial display refresh
    applySettingsFromUI();
    startTimerHz (30);
}

MicrotuningTab::~MicrotuningTab()
{
    stopTimer();
}

void MicrotuningTab::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff181a1f));

    // Section 1: Settings bar card background
    g.setColour (juce::Colour (0xff21252b));
    g.fillRoundedRectangle (10.0f, 8.0f, static_cast<float>(getWidth() - 20), 40.0f, 6.0f);
    g.setColour (juce::Colour (0xff2c313a));
    g.drawRoundedRectangle (10.0f, 8.0f, static_cast<float>(getWidth() - 20), 40.0f, 6.0f, 1.0f);

    // Section 2: Makam Info Card background
    g.setColour (juce::Colour (0xff21252b));
    g.fillRoundedRectangle (10.0f, 52.0f, static_cast<float>(getWidth() - 20), 54.0f, 6.0f);
    g.setColour (juce::Colour (0xff2c313a));
    g.drawRoundedRectangle (10.0f, 52.0f, static_cast<float>(getWidth() - 20), 54.0f, 6.0f, 1.0f);

    // Section 3: Keyboard Strips area background
    float stripsY = 112.0f;
    float liveY = static_cast<float>(getHeight() - 105);
    float actionY = liveY - 32.0f;
    float stripsH = actionY - stripsY - 6.0f;

    g.setColour (juce::Colour (0xff1c1f24));
    g.fillRoundedRectangle (10.0f, stripsY, static_cast<float>(getWidth() - 20), stripsH, 6.0f);
    g.setColour (juce::Colour (0xff2c313a));
    g.drawRoundedRectangle (10.0f, stripsY, static_cast<float>(getWidth() - 20), stripsH, 6.0f, 1.0f);

    // Section 4: Live Monitoring Card background
    float liveH = 95.0f;
    g.setColour (juce::Colour (0xff21252b));
    g.fillRoundedRectangle (10.0f, liveY, static_cast<float>(getWidth() - 20), liveH, 6.0f);
    g.setColour (juce::Colour (0xff2c313a));
    g.drawRoundedRectangle (10.0f, liveY, static_cast<float>(getWidth() - 20), liveH, 6.0f, 1.0f);

    // Live Pitch Bend Bipolar Meter Bar
    float meterX = 18.0f;
    float meterY = liveY + 48.0f;
    float meterW = static_cast<float>(getWidth() - 36);
    float meterH = 14.0f;

    // Meter track background
    g.setColour (juce::Colour (0xff181a1f));
    g.fillRoundedRectangle (meterX, meterY, meterW, meterH, 4.0f);
    g.setColour (juce::Colour (0xff2c313a));
    g.drawRoundedRectangle (meterX, meterY, meterW, meterH, 4.0f, 1.0f);

    // Center tick
    float centerX = meterX + meterW * 0.5f;
    g.setColour (juce::Colour (0xff5c6370));
    g.drawVerticalLine (static_cast<int>(centerX), meterY, meterY + meterH);

    // Deflection bar
    int range = processor.getMicrotuningProcessor().getSettings().pitchBendRange.load();
    if (range <= 0) range = 12;
    float maxCents = static_cast<float>(range * 100);

    float normalizedDeflection = std::clamp (currentLiveCents / maxCents, -1.0f, 1.0f);
    float barWidth = std::abs (normalizedDeflection) * (meterW * 0.5f);

    if (barWidth > 1.0f)
    {
        juce::Colour barCol = (normalizedDeflection < 0.0f) ? juce::Colour (0xffe06c75) : juce::Colour (0xffc678dd);
        float barX = (normalizedDeflection < 0.0f) ? (centerX - barWidth) : centerX;
        g.setColour (barCol);
        g.fillRoundedRectangle (barX, meterY + 2.0f, barWidth, meterH - 4.0f, 3.0f);
    }

    // Helper notice below meter bar
    g.setFont (juce::FontOptions (10.5f));
    g.setColour (juce::Colour (0xff7f848e));
    g.drawText ("Notice: Ensure your synthesizer's Pitch Bend Range matches MidiGlide's setting (typically +/- 2 or +/- 12 semitones)",
                (int)meterX, (int)(liveY + 68.0f), (int)meterW, 18, juce::Justification::centredLeft, false);
}

void MicrotuningTab::resized()
{
    int w = getWidth();
    int h = getHeight();

    // Row 1: Header controls (y: 14, h: 26)
    int x = 16;
    enableMicrotuning.setBounds (x, 14, 155, 26);
    x += 160;

    makamSelectorLabel.setBounds (x, 14, 45, 26);
    x += 48;
    makamSelectorBox.setBounds (x, 14, 150, 26);
    x += 158;

    rootKeyLabel.setBounds (x, 14, 45, 26);
    x += 48;
    rootKeyBox.setBounds (x, 14, 52, 26);
    x += 56;
    defaultKararButton.setBounds (x, 14, 55, 26);
    x += 62;

    tuningModeLabel.setBounds (x, 14, 50, 26);
    x += 52;
    tuningModeBox.setBounds (x, 14, 140, 26);
    x += 148;

    // Pitch bend range and send RPN on the right side
    int rpnW = 58;
    int pbBoxW = 90;
    int pbLblW = 65;
    sendRPNButton.setBounds (w - 16 - rpnW, 14, rpnW, 26);
    pitchBendRangeBox.setBounds (w - 16 - rpnW - 6 - pbBoxW, 14, pbBoxW, 26);
    pitchBendRangeLabel.setBounds (w - 16 - rpnW - 6 - pbBoxW - 4 - pbLblW, 14, pbLblW, 26);

    // Row 2: Makam Info Banner (y: 54, h: 50)
    makamTitleLabel.setBounds (18, 54, 280, 20);
    makamDetailsLabel.setBounds (305, 54, 320, 20);
    allZeroButton.setBounds (w - 16 - 125, 54, 125, 22);
    resetPresetButton.setBounds (w - 16 - 125 - 6 - 135, 54, 135, 22);
    makamDescriptionLabel.setBounds (18, 74, w - 36, 26);

    // Bottom Card metrics:
    int liveH = 95;
    int liveY = h - liveH - 10;

    // Action buttons row (above live monitor):
    int actionH = 24;
    int actionY = liveY - actionH - 8;

    testKararButton.setBounds (18, actionY, 105, actionH);
    testMicrotoneButton.setBounds (130, actionY, 115, actionH);
    testScaleButton.setBounds (252, actionY, 125, actionH);
    stopAuditionButton.setBounds (384, actionY, 95, actionH);

    // Row 3: 12 Key Strips (between banner and action buttons)
    int stripsY = 112;
    int stripsH = actionY - stripsY - 8;
    stripsH = std::max (120, stripsH);

    int marginX = 14;
    int availW = w - (marginX * 2);
    int keySpacing = 4;
    int keyWidth = (availW - (11 * keySpacing)) / 12;

    for (int i = 0; i < 12; ++i)
    {
        int kx = marginX + i * (keyWidth + keySpacing);
        keyStrips[static_cast<size_t>(i)]->setBounds (kx, stripsY, keyWidth, stripsH);
    }

    // Row 5: Live Monitor Card (bottom)
    liveStatusHeaderLabel.setBounds (18, liveY + 4, 250, 18);
    liveNoteLabel.setBounds (18, liveY + 24, 140, 20);
    liveCentsLabel.setBounds (165, liveY + 24, 230, 20);
    livePitchWheelLabel.setBounds (400, liveY + 24, 220, 20);
    liveNotesCounterLabel.setBounds (w - 150, liveY + 24, 130, 20);
}

void MicrotuningTab::timerCallback()
{
    const auto& live = processor.getMicrotuningProcessor().getLiveState();
    int note = live.lastMidiNote.load (std::memory_order_relaxed);
    float cents = live.lastAppliedCents.load (std::memory_order_relaxed);
    int pb = live.lastPitchBendRaw.load (std::memory_order_relaxed);
    uint32_t total = live.totalTunedNotes.load (std::memory_order_relaxed);

    if (note != currentLiveMidiNote || std::abs (cents - currentLiveCents) > 0.05f || pb != currentLivePitchWheel)
    {
        currentLiveMidiNote = note;
        currentLiveCents = cents;
        currentLivePitchWheel = pb;

        if (note >= 0)
        {
            static const char* noteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
            int chroma = note % 12;
            int oct = (note / 12) - 1;
            juce::String noteStr = juce::String (noteNames[chroma]) + juce::String (oct);
            liveNoteLabel.setText ("Last Note: " + noteStr, juce::dontSendNotification);

            double komas = static_cast<double>(cents) / 22.6415;
            juce::String centsSign = (cents >= 0.0f) ? "+" : "";
            liveCentsLabel.setText ("Offset: " + centsSign + juce::String (cents, 1) + " cents (" +
                                    centsSign + juce::String (komas, 2) + " koma)", juce::dontSendNotification);

            int signedBend = pb - 8192;
            juce::String pbSign = (signedBend >= 0) ? "+" : "";
            livePitchWheelLabel.setText ("Pitch Bend: " + juce::String (pb) + " (" + pbSign + juce::String (signedBend) + ")", juce::dontSendNotification);
        }
        liveNotesCounterLabel.setText ("Tuned Notes: " + juce::String (total), juce::dontSendNotification);
        repaint (10, getHeight() - 55, getWidth() - 20, 25);
    }
}

void MicrotuningTab::applySettingsFromUI()
{
    auto& mtProcessor = processor.getMicrotuningProcessor();
    auto& settings = mtProcessor.getSettings();

    bool enabled = enableMicrotuning.getToggleState();
    int mIdx = makamSelectorBox.getSelectedId() - 1;
    int rKey = rootKeyBox.getSelectedId() - 1;
    int tMode = tuningModeBox.getSelectedId() - 1;
    int pbRange = pitchBendRangeBox.getSelectedId();
    if (pbRange <= 0) pbRange = 12;

    settings.enabled.store (enabled);
    settings.pitchBendRange.store (pbRange);

    if (tMode != static_cast<int>(MidiMonitor::TuningMode::Custom))
    {
        mtProcessor.applyPreset (static_cast<MidiMonitor::TurkishMakamId>(mIdx),
                                 rKey,
                                 static_cast<MidiMonitor::TuningMode>(tMode));
    }
    else
    {
        settings.makamId.store (mIdx);
        settings.rootNote.store (rKey);
    }

    refreshKeyStrips();
}

void MicrotuningTab::refreshKeyStrips()
{
    auto& mtProcessor = processor.getMicrotuningProcessor();
    const auto& settings = mtProcessor.getSettings();

    int mIdx = settings.makamId.load();
    int rKey = settings.rootNote.load();
    const auto& allData = MidiMonitor::getAllMakamData();

    if (mIdx < 0 || mIdx >= static_cast<int>(allData.size()))
        mIdx = 0;

    const auto& makam = allData[static_cast<size_t>(mIdx)];

    // Update banner
    makamTitleLabel.setText (makam.getTurkishName() + juce::String::fromUTF8 (" Makamı (") + makam.name + ")", juce::dontSendNotification);
    makamDetailsLabel.setText ("Karar: " + rootKeyBox.getText() +
                               juce::String::fromUTF8 (" | Güçlü: ") + makam.getDominantName() +
                               juce::String::fromUTF8 (" | Seyir: ") + makam.getSeyir(),
                               juce::dontSendNotification);
    makamDescriptionLabel.setText (makam.getDescription(), juce::dontSendNotification);

    // Update 12 Key Strips
    for (int absKey = 0; absKey < 12; ++absKey)
    {
        float cents = settings.chromaticCentsOffsets[static_cast<size_t>(absKey)].load();
        int relDeg = ((absKey - rKey) % 12 + 12) % 12;

        bool inScale = makam.isScaleDegree[relDeg];
        juce::String perde = makam.getPerdeName (static_cast<size_t>(relDeg));

        juce::String role = "";
        if (relDeg == 0)
            role = "KARAR";
        else if (relDeg == 7) // 5th degree
            role = "GUCLU";

        keyStrips[static_cast<size_t>(absKey)]->updateDisplay (cents, perde, role, inScale);
    }

    repaint();
}

void MicrotuningTab::sendPitchBendRPN()
{
    int range = pitchBendRangeBox.getSelectedId();
    if (range <= 0) range = 12;

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

void MicrotuningTab::playAuditionNote (int midiNote, int durationMs)
{
    processor.injectMidiMessage (juce::MidiMessage::noteOn (1, midiNote, (uint8_t)100));
    juce::Timer::callAfterDelay (durationMs, [this, midiNote]
    {
        processor.injectMidiMessage (juce::MidiMessage::noteOff (1, midiNote, (uint8_t)64));
    });
}

void MicrotuningTab::playAuditionScale()
{
    int mIdx = makamSelectorBox.getSelectedId() - 1;
    int rKey = rootKeyBox.getSelectedId() - 1;
    const auto& allData = MidiMonitor::getAllMakamData();
    if (mIdx < 0 || mIdx >= static_cast<int>(allData.size())) mIdx = 0;
    const auto& makam = allData[static_cast<size_t>(mIdx)];

    std::vector<int> scaleNotes;
    int baseMidi = 60 + rKey; // Karar octave

    for (int rel = 0; rel < 12; ++rel)
    {
        if (makam.isScaleDegree[rel])
            scaleNotes.push_back (baseMidi + rel);
    }
    scaleNotes.push_back (baseMidi + 12); // Octave

    int delay = 0;
    int stepTime = 320;

    for (size_t i = 0; i < scaleNotes.size(); ++i)
    {
        int note = scaleNotes[i];
        juce::Timer::callAfterDelay (delay, [this, note, stepTime]
        {
            processor.injectMidiMessage (juce::MidiMessage::noteOn (1, note, (uint8_t)100));
            juce::Timer::callAfterDelay (stepTime - 40, [this, note]
            {
                processor.injectMidiMessage (juce::MidiMessage::noteOff (1, note, (uint8_t)64));
            });
        });
        delay += stepTime;
    }
}
