#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <array>
#include <atomic>
#include <cstdio>
#include <cstring>

namespace MidiMonitor
{

enum class EventFilterType : uint32_t
{
    Notes         = 1 << 0,
    ControlChange = 1 << 1,
    PitchBend     = 1 << 2,
    Aftertouch    = 1 << 3,
    ProgramChange = 1 << 4,
    ClockTiming   = 1 << 5,
    SysEx         = 1 << 6,
    Other         = 1 << 7,
    All           = 0xFFFFFFFF
};

struct MidiLogEntry
{
    enum class MsgType : uint8_t
    {
        NoteOn,
        NoteOff,
        ControlChange,
        PitchBend,
        ChannelPressure, // Aftertouch
        PolyPressure,
        ProgramChange,
        ClockTiming,
        SysEx,
        Other
    };

    double timestampSec = 0.0;
    int sampleOffset = 0;
    double ppqPosition = -1.0;
    int bar = 0;
    int beat = 0;
    int tick = 0;
    uint8_t channel = 1; // 1-16
    MsgType type = MsgType::Other;
    uint8_t data1 = 0;
    uint8_t data2 = 0;
    int intValue = 0; // e.g. pitch bend signed value (-8192 to 8191)
    uint8_t rawBytes[8] = { 0 };
    uint8_t rawLength = 0;
    char typeName[24] = { 0 };
    char description[96] = { 0 };
    char hexString[32] = { 0 };
    char timeString[24] = { 0 };
    char barBeatString[24] = { 0 };
    bool isInjected = false;
    char originTag[24] = { 0 };

    static const char* getMidiNoteName(int noteNumber)
    {
        static const char* const noteNames[] = {
            "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
        };
        static thread_local char buffer[12];
        if (noteNumber < 0 || noteNumber > 127) return "???";
        int octave = (noteNumber / 12) - 2; // standard MIDI octave (60 = C3 or C4 depending on standard, -2 gives C-2 for 0, C3 for 60)
        std::snprintf(buffer, sizeof(buffer), "%s%d", noteNames[noteNumber % 12], octave + 1); // C4 for 60
        return buffer;
    }

    static const char* getControllerName(int ccNumber)
    {
        switch (ccNumber)
        {
            case 0:   return "Bank Select (MSB)";
            case 1:   return "Modulation Wheel";
            case 2:   return "Breath Controller";
            case 4:   return "Foot Controller";
            case 5:   return "Portamento Time";
            case 6:   return "Data Entry (MSB)";
            case 7:   return "Channel Volume";
            case 8:   return "Balance";
            case 10:  return "Pan";
            case 11:  return "Expression";
            case 12:  return "Effect Control 1";
            case 13:  return "Effect Control 2";
            case 32:  return "Bank Select (LSB)";
            case 38:  return "Data Entry (LSB)";
            case 64:  return "Sustain (Damper)";
            case 65:  return "Portamento On/Off";
            case 66:  return "Sostenuto";
            case 67:  return "Soft Pedal";
            case 68:  return "Legato Footswitch";
            case 69:  return "Hold 2";
            case 71:  return "Resonance (Timbre)";
            case 72:  return "Release Time";
            case 73:  return "Attack Time";
            case 74:  return "Cutoff / Brightness";
            case 75:  return "Decay Time";
            case 76:  return "Vibrato Rate";
            case 77:  return "Vibrato Depth";
            case 78:  return "Vibrato Delay";
            case 84:  return "Portamento Amount";
            case 91:  return "Reverb Level";
            case 92:  return "Tremolo Level";
            case 93:  return "Chorus Level";
            case 94:  return "Detune Level";
            case 95:  return "Phaser Level";
            case 120: return "All Sound Off";
            case 121: return "Reset All Controllers";
            case 122: return "Local Control";
            case 123: return "All Notes Off";
            case 124: return "Omni Mode Off";
            case 125: return "Omni Mode On";
            case 126: return "Mono Mode On";
            case 127: return "Poly Mode On";
            default:  return "";
        }
    }

    static MidiLogEntry fromMidiMessage(const juce::MidiMessage& m,
                                        int sampleOffset,
                                        double timeInSeconds,
                                        const juce::AudioPlayHead::PositionInfo* posInfo,
                                        bool isInjected = false,
                                        const char* originTag = nullptr)
    {
        MidiLogEntry e;
        e.timestampSec = timeInSeconds;
        e.sampleOffset = sampleOffset;
        e.isInjected = isInjected;
        if (originTag != nullptr && originTag[0] != '\0')
            std::snprintf(e.originTag, sizeof(e.originTag), "%s", originTag);
        else
            std::snprintf(e.originTag, sizeof(e.originTag), "%s", isInjected ? "INJECTED" : "PASS-THRU");

        int totalSec = static_cast<int>(timeInSeconds);
        int millis = static_cast<int>((timeInSeconds - totalSec) * 1000.0);
        int mins = totalSec / 60;
        int secs = totalSec % 60;
        std::snprintf(e.timeString, sizeof(e.timeString), "%02d:%02d.%03d", mins, secs, millis);

        if (posInfo != nullptr)
        {
            if (posInfo->getPpqPosition().hasValue())
                e.ppqPosition = *posInfo->getPpqPosition();

            if (posInfo->getBarCount().hasValue())
                e.bar = static_cast<int>(*posInfo->getBarCount());

            if (posInfo->getTimeInSamples().hasValue())
            {
                // Format Bar|Beat if available
                if (posInfo->getIsPlaying())
                {
                    double ppq = e.ppqPosition >= 0.0 ? e.ppqPosition : 0.0;
                    int barNum = e.bar > 0 ? e.bar : static_cast<int>(ppq / 4.0) + 1;
                    int beatNum = (static_cast<int>(ppq) % 4) + 1;
                    int tickNum = static_cast<int>((ppq - static_cast<int>(ppq)) * 960.0);
                    std::snprintf(e.barBeatString, sizeof(e.barBeatString), "%d|%d|%03d", barNum, beatNum, tickNum);
                }
                else
                {
                    std::snprintf(e.barBeatString, sizeof(e.barBeatString), "-");
                }
            }
        }
        else
        {
            std::snprintf(e.barBeatString, sizeof(e.barBeatString), "-");
        }

        // Raw bytes (up to 8 bytes for preview)
        const uint8_t* raw = m.getRawData();
        int sz = m.getRawDataSize();
        e.rawLength = static_cast<uint8_t>(std::min(sz, 8));
        for (int i = 0; i < e.rawLength; ++i)
            e.rawBytes[i] = raw[i];

        // Format hex string
        char* hexPtr = e.hexString;
        int rem = sizeof(e.hexString);
        for (int i = 0; i < e.rawLength && rem > 4; ++i)
        {
            int written = std::snprintf(hexPtr, rem, "%02X ", raw[i]);
            if (written > 0)
            {
                hexPtr += written;
                rem -= written;
            }
        }
        if (sz > 8 && rem > 4)
            std::snprintf(hexPtr, rem, "...");

        e.channel = static_cast<uint8_t>(m.getChannel());

        if (m.isNoteOn())
        {
            e.type = MsgType::NoteOn;
            e.data1 = static_cast<uint8_t>(m.getNoteNumber());
            e.data2 = static_cast<uint8_t>(m.getVelocity());
            e.intValue = e.data2;
            std::snprintf(e.typeName, sizeof(e.typeName), "Note On");
            std::snprintf(e.description, sizeof(e.description),
                          "%s (%d) | Vel: %d",
                          getMidiNoteName(e.data1), e.data1, e.data2);
        }
        else if (m.isNoteOff())
        {
            e.type = MsgType::NoteOff;
            e.data1 = static_cast<uint8_t>(m.getNoteNumber());
            e.data2 = static_cast<uint8_t>(m.getVelocity());
            e.intValue = e.data2;
            std::snprintf(e.typeName, sizeof(e.typeName), "Note Off");
            std::snprintf(e.description, sizeof(e.description),
                          "%s (%d) | Rel Vel: %d",
                          getMidiNoteName(e.data1), e.data1, e.data2);
        }
        else if (m.isController())
        {
            e.type = MsgType::ControlChange;
            e.data1 = static_cast<uint8_t>(m.getControllerNumber());
            e.data2 = static_cast<uint8_t>(m.getControllerValue());
            e.intValue = e.data2;
            std::snprintf(e.typeName, sizeof(e.typeName), "CC #%d", e.data1);
            const char* ccName = getControllerName(e.data1);
            if (ccName[0] != '\0')
            {
                std::snprintf(e.description, sizeof(e.description),
                              "[%s] Val: %d", ccName, e.data2);
            }
            else
            {
                std::snprintf(e.description, sizeof(e.description),
                              "Val: %d", e.data2);
            }
        }
        else if (m.isPitchWheel())
        {
            e.type = MsgType::PitchBend;
            int pitchWheel = m.getPitchWheelValue(); // 0 to 16383, 8192 is center
            int signedVal = pitchWheel - 8192;
            double percent = (signedVal / 8192.0) * 100.0;
            e.intValue = signedVal;
            std::snprintf(e.typeName, sizeof(e.typeName), "Pitch Bend");
            std::snprintf(e.description, sizeof(e.description),
                          "Val: %+d (14-bit: %d, %+.1f%%)",
                          signedVal, pitchWheel, percent);
        }
        else if (m.isChannelPressure())
        {
            e.type = MsgType::ChannelPressure;
            e.data1 = static_cast<uint8_t>(m.getChannelPressureValue());
            e.intValue = e.data1;
            std::snprintf(e.typeName, sizeof(e.typeName), "Channel Aftertouch");
            std::snprintf(e.description, sizeof(e.description), "Pressure: %d", e.data1);
        }
        else if (m.isAftertouch())
        {
            e.type = MsgType::PolyPressure;
            e.data1 = static_cast<uint8_t>(m.getNoteNumber());
            e.data2 = static_cast<uint8_t>(m.getAfterTouchValue());
            e.intValue = e.data2;
            std::snprintf(e.typeName, sizeof(e.typeName), "Poly Aftertouch");
            std::snprintf(e.description, sizeof(e.description),
                          "%s (%d) | Pressure: %d",
                          getMidiNoteName(e.data1), e.data1, e.data2);
        }
        else if (m.isProgramChange())
        {
            e.type = MsgType::ProgramChange;
            e.data1 = static_cast<uint8_t>(m.getProgramChangeNumber());
            e.intValue = e.data1;
            std::snprintf(e.typeName, sizeof(e.typeName), "Program Change");
            std::snprintf(e.description, sizeof(e.description), "Program: %d", e.data1);
        }
        else if (m.isSysEx())
        {
            e.type = MsgType::SysEx;
            std::snprintf(e.typeName, sizeof(e.typeName), "SysEx");
            std::snprintf(e.description, sizeof(e.description), "SysEx (%d bytes)", sz);
        }
        else if (m.isMidiClock() || m.isMidiStart() || m.isMidiStop() || m.isMidiContinue())
        {
            e.type = MsgType::ClockTiming;
            const char* clkDesc = m.isMidiClock()    ? "MIDI Clock" :
                                  m.isMidiStart()    ? "MIDI Start" :
                                  m.isMidiStop()     ? "MIDI Stop"  :
                                  m.isMidiContinue() ? "MIDI Continue" : "Timing";
            std::snprintf(e.typeName, sizeof(e.typeName), "%s", clkDesc);
            std::snprintf(e.description, sizeof(e.description), "System Real-Time");
        }
        else
        {
            e.type = MsgType::Other;
            std::snprintf(e.typeName, sizeof(e.typeName), "Other");
            std::snprintf(e.description, sizeof(e.description), "%d bytes", sz);
        }

        return e;
    }
};

} // namespace MidiMonitor
