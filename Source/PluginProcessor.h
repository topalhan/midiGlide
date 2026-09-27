#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "MidiLogQueue.h"
#include "PortamentoProcessor.h"

class MidiMonitorAudioProcessor : public juce::AudioProcessor
{
public:
    MidiMonitorAudioProcessor();
    ~MidiMonitorAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    MidiMonitor::MidiLogQueue& getMidiInQueue() noexcept { return midiInQueue; }
    MidiMonitor::MidiLogQueue& getMidiOutQueue() noexcept { return midiOutQueue; }
    MidiMonitor::PortamentoProcessor& getPortamentoProcessor() noexcept { return portamentoProcessor; }

    void injectMidiMessage (const juce::MidiMessage& msg);

    uint64_t getTotalProcessedSamples() const noexcept { return currentSampleCount.load(); }

private:
    MidiMonitor::MidiLogQueue midiInQueue;
    MidiMonitor::MidiLogQueue midiOutQueue;
    MidiMonitor::PortamentoProcessor portamentoProcessor;
    std::atomic<uint64_t> currentSampleCount { 0 };

    // Outgoing injection FIFO (from GUI to audio thread, e.g. Panic / All Notes Off)
    struct QueuedMidiMsg
    {
        uint8_t data[4];
        uint8_t size = 0;
    };
    juce::AbstractFifo injectFifo { 256 };
    std::array<QueuedMidiMsg, 256> injectBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiMonitorAudioProcessor)
};
