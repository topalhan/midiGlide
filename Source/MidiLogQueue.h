#pragma once

#include "MidiLogEntry.h"
#include <juce_core/juce_core.h>
#include <array>
#include <atomic>
#include <vector>

namespace MidiMonitor
{

class MidiLogQueue
{
public:
    static constexpr int QueueCapacity = 8192;

    MidiLogQueue()
        : fifo(QueueCapacity)
    {
    }

    // Call only from audio / processBlock thread
    bool push(const MidiLogEntry& entry)
    {
        totalReceived.fetch_add(1, std::memory_order_relaxed);

        int start1, size1, start2, size2;
        fifo.prepareToWrite(1, start1, size1, start2, size2);

        if (size1 + size2 == 0)
        {
            droppedCount.fetch_add(1, std::memory_order_relaxed);
            return false;
        }

        if (size1 > 0)
            buffer[static_cast<size_t>(start1)] = entry;
        else if (size2 > 0)
            buffer[static_cast<size_t>(start2)] = entry;

        fifo.finishedWrite(size1 + size2);
        return true;
    }

    // Call only from GUI thread
    int popAll(std::vector<MidiLogEntry>& destination, int maxToPop = 4096)
    {
        int numReady = fifo.getNumReady();
        if (numReady <= 0)
            return 0;

        int toRead = std::min(numReady, maxToPop);
        int start1, size1, start2, size2;
        fifo.prepareToRead(toRead, start1, size1, start2, size2);

        for (int i = 0; i < size1; ++i)
            destination.push_back(buffer[static_cast<size_t>(start1 + i)]);

        for (int i = 0; i < size2; ++i)
            destination.push_back(buffer[static_cast<size_t>(start2 + i)]);

        fifo.finishedRead(size1 + size2);
        return size1 + size2;
    }

    void reset()
    {
        fifo.reset();
        droppedCount.store(0, std::memory_order_relaxed);
    }

    uint64_t getTotalReceived() const noexcept
    {
        return totalReceived.load(std::memory_order_relaxed);
    }

    uint32_t getDroppedCount() const noexcept
    {
        return droppedCount.load(std::memory_order_relaxed);
    }

private:
    juce::AbstractFifo fifo;
    std::array<MidiLogEntry, QueueCapacity> buffer;
    std::atomic<uint64_t> totalReceived { 0 };
    std::atomic<uint32_t> droppedCount { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiLogQueue)
};

} // namespace MidiMonitor
