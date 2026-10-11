#include "PluginProcessor.h"
#include "PluginEditor.h"

MidiMonitorAudioProcessor::MidiMonitorAudioProcessor()
    : AudioProcessor (BusesProperties()
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    portamentoProcessor.setMicrotuningProcessor (&microtuningProcessor);
    portamentoProcessor.setBreathRemapProcessor (&breathRemapProcessor);
}

MidiMonitorAudioProcessor::~MidiMonitorAudioProcessor()
{
}

const juce::String MidiMonitorAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool MidiMonitorAudioProcessor::acceptsMidi() const
{
    return true;
}

bool MidiMonitorAudioProcessor::producesMidi() const
{
    return true;
}

bool MidiMonitorAudioProcessor::isMidiEffect() const
{
    return false;
}

double MidiMonitorAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int MidiMonitorAudioProcessor::getNumPrograms()
{
    return 1;
}

int MidiMonitorAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MidiMonitorAudioProcessor::setCurrentProgram (int)
{
}

const juce::String MidiMonitorAudioProcessor::getProgramName (int)
{
    return {};
}

void MidiMonitorAudioProcessor::changeProgramName (int, const juce::String&)
{
}

void MidiMonitorAudioProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleCount.store (0);
    midiInQueue.reset();
    midiOutQueue.reset();
    portamentoProcessor.prepareToPlay (sampleRate);
    breathRemapProcessor.prepareToPlay (sampleRate);
}

void MidiMonitorAudioProcessor::releaseResources()
{
    portamentoProcessor.reset();
    breathRemapProcessor.reset();
}

bool MidiMonitorAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void MidiMonitorAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Clear output audio so MidiGlide produces silence
    buffer.clear();

    auto* playHead = getPlayHead();
    juce::AudioPlayHead::PositionInfo posInfo;
    bool hasPosInfo = false;
    if (playHead != nullptr)
    {
        auto pos = playHead->getPosition();
        if (pos.hasValue())
        {
            posInfo = *pos;
            hasPosInfo = true;
        }
    }

    double sampleRate = getSampleRate();
    if (sampleRate <= 0.0)
        sampleRate = 44100.0;

    auto baseSample = currentSampleCount.load (std::memory_order_relaxed);

    // 1. Process any user-injected MIDI events (e.g. Test buttons from UI) first
    //    so they flow through the portamento processor!
    int numReady = injectFifo.getNumReady();
    if (numReady > 0)
    {
        int start1, size1, start2, size2;
        injectFifo.prepareToRead (numReady, start1, size1, start2, size2);

        auto injectFromRange = [&] (int start, int size)
        {
            for (int i = 0; i < size; ++i)
            {
                const auto& item = injectBuffer[static_cast<size_t>(start + i)];
                if (item.size > 0)
                {
                    juce::MidiMessage m (item.data, item.size);
                    midiMessages.addEvent (m, 0);
                }
            }
        };

        injectFromRange (start1, size1);
        injectFromRange (start2, size2);

        injectFifo.finishedRead (size1 + size2);
    }

    // 2. Process portamento note transition injection and log all events
    portamentoProcessor.processBlock (midiMessages, buffer.getNumSamples(), baseSample,
                                      hasPosInfo ? &posInfo : nullptr, midiInQueue, midiOutQueue);

    currentSampleCount.fetch_add (static_cast<uint64_t>(buffer.getNumSamples()), std::memory_order_relaxed);

    // IMPORTANT: Incoming midiMessages is kept intact!
    // The exact MIDI stream passes through directly to downstream plugins.
}

void MidiMonitorAudioProcessor::injectMidiMessage (const juce::MidiMessage& msg)
{
    int rawSize = msg.getRawDataSize();
    if (rawSize <= 0 || rawSize > 4)
        return;

    int start1, size1, start2, size2;
    injectFifo.prepareToWrite (1, start1, size1, start2, size2);
    if (size1 + size2 > 0)
    {
        int idx = (size1 > 0) ? start1 : start2;
        auto& item = injectBuffer[static_cast<size_t>(idx)];
        item.size = static_cast<uint8_t>(rawSize);
        std::memcpy (item.data, msg.getRawData(), static_cast<size_t>(rawSize));
        injectFifo.finishedWrite (1);
    }
}

bool MidiMonitorAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* MidiMonitorAudioProcessor::createEditor()
{
    return new MidiMonitorAudioProcessorEditor (*this);
}

void MidiMonitorAudioProcessor::getStateInformation (juce::MemoryBlock& /*destData*/)
{
}

void MidiMonitorAudioProcessor::setStateInformation (const void* /*data*/, int /*sizeInBytes*/)
{
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MidiMonitorAudioProcessor();
}
