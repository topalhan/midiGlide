#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <array>
#include <atomic>
#include <cmath>
#include <algorithm>
#include <string>

namespace MidiMonitor
{

enum class TurkishMakamId : int
{
    Rast = 0,
    Ussak,
    Hicaz,
    Huseyni,
    Saba,
    Segah,
    Karcigar,
    Nihavend,
    Kurdi,
    Buselik,
    Count
};

enum class TuningMode : int
{
    Performance = 0,   // Stage & recording practice (authentic ear-tuned comma cents)
    Holdrian53,        // Strict 53-Holdrian Comma (Arel-Ezgi-Uzdilek 53-TET)
    Custom             // User customized per-note cents
};

struct MakamInfo
{
    TurkishMakamId id;
    const char* name;
    const char* turkishName;
    int defaultKarar; // 0=C, 1=C#, 2=D, ... 7=G, 9=A, 11=B
    const char* dominantName;
    const char* seyir; // Ascending, Descending, etc.
    const char* description;
    // Offsets for the 12 chromatic degrees relative to Karar (0 to 11 semitones)
    // Positive = sharp, Negative = flat (in cents)
    float performanceOffsets[12];
    float holdrian53Offsets[12];
    // Scale membership mask (1 = scale degree, 0 = passing/accidental)
    bool isScaleDegree[12];
    // Perde names for the 7 primary scale notes
    const char* degreePerdeNames[12];

    juce::String getTurkishName() const { return juce::String::fromUTF8 (turkishName); }
    juce::String getDominantName() const { return juce::String::fromUTF8 (dominantName); }
    juce::String getSeyir() const { return juce::String::fromUTF8 (seyir); }
    juce::String getDescription() const { return juce::String::fromUTF8 (description); }
    juce::String getPerdeName (size_t degree) const
    {
        if (degree < 12 && degreePerdeNames[degree] != nullptr && degreePerdeNames[degree][0] != '\0')
            return juce::String::fromUTF8 (degreePerdeNames[degree]);
        return {};
    }
};

inline const std::array<MakamInfo, static_cast<size_t>(TurkishMakamId::Count)>& getAllMakamData()
{
    static const std::array<MakamInfo, static_cast<size_t>(TurkishMakamId::Count)> data = {{
        // 1. Rast
        {
            TurkishMakamId::Rast,
            "Rast",
            "Rast",
            7, // G (Rast)
            "Nevâ (D)",
            "Çıkıcı (Ascending)",
            "One of the foundational makams. Features Segâh (3rd degree -1 koma) and Eviç (7th degree -1 koma). Conveys joy, tranquility, and nobility.",
            // Relative degrees: 0(Rast), 1, 2(Dügâh), 3, 4(Segâh), 5(Çârgâh), 6, 7(Nevâ), 8, 9(Hüseynî), 10, 11(Eviç)
            { 0.0f, 0.0f,  0.0f, 0.0f, -24.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -24.0f }, // Performance
            { 0.0f, 0.0f, +3.8f, 0.0f, -15.1f, -1.9f, 0.0f, +1.9f, 0.0f, +5.7f, 0.0f, -13.2f }, // 53-Holdrian
            { true, false, true, false, true, true, false, true, false, true, false, true },
            { "Rast", "", "Dügâh", "", "Segâh (-1k)", "Çârgâh", "", "Nevâ", "", "Hüseynî", "", "Eviç (-1k)" }
        },
        // 2. Uşşak
        {
            TurkishMakamId::Ussak,
            "Ussak",
            "Uşşak",
            9, // A (Dügâh)
            "Nevâ (D)",
            "Çıkıcı (Ascending)",
            "Deeply soulful and contemplative. Defined by Uşşak tetrachord with its poignant Segâh 2nd degree (~2 to 3 komas flat) resolving to Dügâh.",
            // Rel degrees from A: 0(Dügâh), 1, 2(Segâh), 3(Çârgâh), 4, 5(Nevâ), 6, 7(Hüseynî), 8(Acem), 9, 10(Gerdâniye), 11
            { 0.0f, 0.0f, -27.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f }, // Performance
            { 0.0f, 0.0f, -18.9f, -5.7f, 0.0f, -1.9f, 0.0f, +1.9f, -7.5f, 0.0f, -3.8f, 0.0f }, // 53-Holdrian
            { true, false, true, true, false, true, false, true, true, false, true, false },
            { "Dügâh", "", "Segâh (-1-2k)", "Çârgâh", "", "Nevâ", "", "Hüseynî", "Acem", "", "Gerdâniye", "" }
        },
        // 3. Hicaz
        {
            TurkishMakamId::Hicaz,
            "Hicaz",
            "Hicaz",
            9, // A (Dügâh)
            "Nevâ (D)",
            "İnici-Çıkıcı (Ascending-Descending)",
            "Passionate and dramatic with an augmented 2nd interval. Uses Dik Kürdî (Bb +1 koma) and Nîm Hicâz (C# -1 koma), often with Eviç (F# -1 koma) on upper register.",
            // Rel degrees from A: 0(Dügâh), 1(Dik Kürdî), 2, 3, 4(Nîm Hicâz), 5(Nevâ), 6, 7(Hüseynî), 8(Acem), 9(Eviç), 10(Gerdâniye), 11
            { 0.0f, +15.0f, 0.0f, 0.0f, -15.0f, 0.0f, 0.0f, 0.0f, 0.0f, -24.0f, 0.0f, 0.0f }, // Performance
            { 0.0f, +13.2f, 0.0f, 0.0f, -15.1f, -1.9f, 0.0f, +1.9f, 0.0f, -13.2f, -3.8f, 0.0f }, // 53-Holdrian
            { true, true, false, false, true, true, false, true, false, true, true, false },
            { "Dügâh", "Dik Kürdî (+1k)", "", "", "Nîm Hicâz (-1k)", "Nevâ", "", "Hüseynî", "", "Eviç (-1k)", "Gerdâniye", "" }
        },
        // 4. Hüseyni
        {
            TurkishMakamId::Huseyni,
            "Huseyni",
            "Hüseyni",
            9, // A (Dügâh)
            "Hüseynî (E)",
            "İnici-Çıkıcı (Ascending-Descending)",
            "Beloved pastoral and heroic mode in Turkish music. Uses Segâh (2nd degree) and features Eviç (F# -1 koma) ascending, moving down to Acem (F natural).",
            // Rel degrees from A: 0(Dügâh), 1, 2(Segâh), 3(Çârgâh), 4, 5(Nevâ), 6, 7(Hüseynî), 8(Acem), 9(Eviç), 10(Gerdâniye), 11
            { 0.0f, 0.0f, -27.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -24.0f, 0.0f, 0.0f }, // Performance
            { 0.0f, 0.0f, -18.9f, -5.7f, 0.0f, -1.9f, 0.0f, +1.9f, 0.0f, -17.0f, -3.8f, 0.0f }, // 53-Holdrian
            { true, false, true, true, false, true, false, true, true, true, true, false },
            { "Dügâh", "", "Segâh (-1-2k)", "Çârgâh", "", "Nevâ", "", "Hüseynî", "Acem", "Eviç (-1k)", "Gerdâniye", "" }
        },
        // 5. Saba
        {
            TurkishMakamId::Saba,
            "Saba",
            "Saba",
            9, // A (Dügâh)
            "Çârgâh (C)",
            "Çıkıcı (Ascending)",
            "Incomparable mystical pathos and spiritual devotion. Features the unique Saba 4th degree (Hicâz perdesi on D flattened by ~35-40 cents) and Segâh.",
            // Rel degrees from A: 0(Dügâh), 1, 2(Segâh), 3(Çârgâh), 4, 5(D flat/Saba), 6, 7(Nevâ/Hisâr), 8(Acem), 9, 10(Gerdâniye), 11
            { 0.0f, 0.0f, -27.0f, 0.0f, 0.0f, -35.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f }, // Performance
            { 0.0f, 0.0f, -18.9f, -5.7f, 0.0f, -30.2f, 0.0f, +1.9f, -7.5f, 0.0f, -3.8f, 0.0f }, // 53-Holdrian
            { true, false, true, true, false, true, false, true, true, false, true, false },
            { "Dügâh", "", "Segâh (-1-2k)", "Çârgâh", "", "Saba (-35c)", "", "Nevâ", "Acem", "", "Gerdâniye", "" }
        },
        // 6. Segah
        {
            TurkishMakamId::Segah,
            "Segah",
            "Segâh",
            11, // B (Segâh, key 11)
            "Nevâ (D)",
            "Çıkıcı (Ascending)",
            "Revered spiritual makam where the Tonic itself is the microtonal Segâh note (B -1 koma). Features Eviç (F# -1 koma) as the fifth degree.",
            // Rel degrees from B: 0(Segâh -1k), 1(Çârgâh), 2, 3(Nevâ), 4, 5(Hüseynî), 6, 7(Eviç -1k), 8(Gerdâniye), 9, 10(Muhayyer), 11
            { -24.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -24.0f, 0.0f, 0.0f, 0.0f, 0.0f }, // Performance
            { -15.1f, -1.9f, 0.0f, +1.9f, 0.0f, +5.7f, 0.0f, -13.2f, -3.8f, 0.0f, 0.0f, 0.0f }, // 53-Holdrian
            { true, true, false, true, false, true, false, true, true, false, true, false },
            { "Segâh (-1k)", "Çârgâh", "", "Nevâ", "", "Hüseynî", "", "Eviç (-1k)", "Gerdâniye", "", "Muhayyer", "" }
        },
        // 7. Karcığar
        {
            TurkishMakamId::Karcigar,
            "Karcigar",
            "Karcığar",
            9, // A (Dügâh)
            "Nevâ (D)",
            "İnici-Çıkıcı (Ascending-Descending)",
            "Combines an Uşşak tetrachord on Dügâh with a Hicaz pentachord on Nevâ. Has Segâh (2nd), Hisâr (Eb / E flattened by ~45 cents), and Eviç (F# -1 koma).",
            // Rel degrees from A: 0(Dügâh), 1, 2(Segâh), 3(Çârgâh), 4, 5(Nevâ), 6, 7(Hisâr), 8, 9(Eviç), 10(Gerdâniye), 11
            { 0.0f, 0.0f, -27.0f, 0.0f, 0.0f, 0.0f, 0.0f, -45.0f, 0.0f, -24.0f, 0.0f, 0.0f }, // Performance
            { 0.0f, 0.0f, -18.9f, -5.7f, 0.0f, -1.9f, 0.0f, -37.7f, 0.0f, -17.0f, -3.8f, 0.0f }, // 53-Holdrian
            { true, false, true, true, false, true, false, true, false, true, true, false },
            { "Dügâh", "", "Segâh (-1-2k)", "Çârgâh", "", "Nevâ", "", "Hisâr (-45c)", "", "Eviç (-1k)", "Gerdâniye", "" }
        },
        // 8. Nihavend
        {
            TurkishMakamId::Nihavend,
            "Nihavend",
            "Nihavend",
            7, // G (Rast)
            "Nevâ (D)",
            "İnici-Çıkıcı (Ascending-Descending)",
            "Sweet, lyrical and romantic; closest to the Western minor scale. Uses Bûselik pentachord on Rast with a Kürdî tetrachord on Nevâ.",
            // Rel degrees from G: 0(Rast), 1, 2(Dügâh), 3(Bb Kürdî), 4, 5(Çârgâh), 6, 7(Nevâ), 8(Eb), 9, 10(F), 11(F#)
            { 0.0f, 0.0f, 0.0f, -8.0f, 0.0f, 0.0f, 0.0f, 0.0f, -8.0f, 0.0f, 0.0f, 0.0f }, // Performance
            { 0.0f, 0.0f, +3.8f, -5.7f, 0.0f, -1.9f, 0.0f, +1.9f, -7.5f, 0.0f, -3.8f, 0.0f }, // 53-Holdrian
            { true, false, true, true, false, true, false, true, true, false, true, true },
            { "Rast", "", "Dügâh", "Kürdî (-8c)", "", "Çârgâh", "", "Nevâ", "Bakiye b", "", "Gerdâniye", "Yegâh #" }
        },
        // 9. Kürdi
        {
            TurkishMakamId::Kurdi,
            "Kurdi",
            "Kürdî",
            9, // A (Dügâh)
            "Nevâ (D)",
            "Çıkıcı (Ascending)",
            "Very popular in modern and classical music. Characterized by the Bb Bakiye bemol (Kürdî perdesi) on Dügâh leading up to Nevâ.",
            // Rel degrees from A: 0(Dügâh), 1(Bb Kürdî), 2, 3(Çârgâh), 4, 5(Nevâ), 6, 7(Hüseynî), 8(Acem), 9, 10(Gerdâniye), 11
            { 0.0f, +9.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f }, // Performance
            { 0.0f, +9.4f, 0.0f, -5.7f, 0.0f, -1.9f, 0.0f, +1.9f, -7.5f, 0.0f, -3.8f, 0.0f }, // 53-Holdrian
            { true, true, false, true, false, true, false, true, true, false, true, false },
            { "Dügâh", "Kürdî (+9c)", "", "Çârgâh", "", "Nevâ", "", "Hüseynî", "Acem", "", "Gerdâniye", "" }
        },
        // 10. Bûselik
        {
            TurkishMakamId::Buselik,
            "Buselik",
            "Bûselik",
            9, // A (Dügâh)
            "Hüseynî (E)",
            "İnici-Çıkıcı (Ascending-Descending)",
            "Classical minor-like makam built from pure Pythagorean fifths. Features natural Bûselik (B natural) and Çârgâh (C natural) without flat microtones on 2nd.",
            // Rel degrees from A: 0(Dügâh), 1, 2(Bûselik), 3(Çârgâh), 4, 5(Nevâ), 6, 7(Hüseynî), 8(Acem), 9, 10(Gerdâniye), 11(Nim Zirgüle)
            { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f }, // Performance
            { 0.0f, 0.0f, +3.8f, -5.7f, 0.0f, -1.9f, 0.0f, +1.9f, -7.5f, 0.0f, -3.8f, 0.0f }, // 53-Holdrian
            { true, false, true, true, false, true, false, true, true, false, true, true },
            { "Dügâh", "", "Bûselik", "Çârgâh", "", "Nevâ", "", "Hüseynî", "Acem", "", "Gerdâniye", "Nîm Zirgüle" }
        }
    }};

    return data;
}

struct MicrotuningSettings
{
    std::atomic<bool> enabled { true };
    std::atomic<int> makamId { static_cast<int>(TurkishMakamId::Rast) };
    std::atomic<int> rootNote { 7 }; // 0 = C, 1 = C#, 2 = D, ... 7 = G (Rast default)
    std::atomic<int> tuningMode { static_cast<int>(TuningMode::Performance) };
    std::atomic<int> pitchBendRange { 12 }; // In semitones (+/- 12 semitones default)

    // Current effective cents offsets for the 12 absolute chromatic semitones (0=C, 1=C# ... 11=B)
    std::array<std::atomic<float>, 12> chromaticCentsOffsets;

    MicrotuningSettings()
    {
        for (auto& val : chromaticCentsOffsets)
            val.store (0.0f);
    }
};

struct MicrotuningLiveState
{
    std::atomic<int> lastMidiNote { -1 };
    std::atomic<float> lastAppliedCents { 0.0f };
    std::atomic<int> lastPitchBendRaw { 8192 };
    std::atomic<uint32_t> totalTunedNotes { 0 };
};

class MicrotuningProcessor
{
public:
    MicrotuningProcessor()
    {
        applyPreset (TurkishMakamId::Rast, 7, TuningMode::Performance);
    }

    MicrotuningSettings& getSettings() noexcept { return settings; }
    const MicrotuningSettings& getSettings() const noexcept { return settings; }

    const MicrotuningLiveState& getLiveState() const noexcept { return liveState; }

    // Calculates the microtonal offset in cents for a given MIDI note (0-127)
    float getCentsForMidiNote (int midiNoteNumber) const noexcept
    {
        if (!settings.enabled.load (std::memory_order_relaxed))
            return 0.0f;

        if (midiNoteNumber < 0 || midiNoteNumber > 127)
            return 0.0f;

        int chroma = midiNoteNumber % 12;
        return settings.chromaticCentsOffsets[static_cast<size_t>(chroma)].load (std::memory_order_relaxed);
    }

    // Calculates the microtonal offset in semitones (-1.0 to +1.0)
    double getSemitonesOffsetForMidiNote (int midiNoteNumber) const noexcept
    {
        return static_cast<double>(getCentsForMidiNote (midiNoteNumber)) / 100.0;
    }

    // Calculates the 14-bit MIDI pitch wheel value (0 to 16383, 8192 = center) for a note
    int calculatePitchBendForMidiNote (int midiNoteNumber) const noexcept
    {
        float cents = getCentsForMidiNote (midiNoteNumber);
        int pbRange = std::max (1, settings.pitchBendRange.load (std::memory_order_relaxed));

        double semitones = static_cast<double>(cents) / 100.0;
        double bendFraction = semitones / static_cast<double>(pbRange);
        bendFraction = std::clamp (bendFraction, -1.0, 1.0);

        int pitchWheel = 8192 + static_cast<int>(std::round (bendFraction * 8191.0));
        return std::clamp (pitchWheel, 0, 16383);
    }

    // Record an active note event in the live state monitor
    void recordNoteTuned (int midiNoteNumber, float cents, int pitchWheelRaw) noexcept
    {
        liveState.lastMidiNote.store (midiNoteNumber, std::memory_order_relaxed);
        liveState.lastAppliedCents.store (cents, std::memory_order_relaxed);
        liveState.lastPitchBendRaw.store (pitchWheelRaw, std::memory_order_relaxed);
        liveState.totalTunedNotes.fetch_add (1, std::memory_order_relaxed);
    }

    // Apply a standard preset makam and root key
    void applyPreset (TurkishMakamId makamId, int rootKey, TuningMode mode)
    {
        const auto& allData = getAllMakamData();
        int idx = static_cast<int>(makamId);
        if (idx < 0 || idx >= static_cast<int>(allData.size()))
            idx = 0;

        const auto& makam = allData[static_cast<size_t>(idx)];
        rootKey = ((rootKey % 12) + 12) % 12;

        settings.makamId.store (idx);
        settings.rootNote.store (rootKey);
        settings.tuningMode.store (static_cast<int>(mode));

        const float* srcOffsets = (mode == TuningMode::Holdrian53)
                                ? makam.holdrian53Offsets
                                : makam.performanceOffsets;

        // Map the 12 scale intervals from rootKey to absolute chromatic pitch classes
        for (int relDeg = 0; relDeg < 12; ++relDeg)
        {
            int absKey = (rootKey + relDeg) % 12;
            settings.chromaticCentsOffsets[static_cast<size_t>(absKey)].store (srcOffsets[relDeg]);
        }
    }

    // Set a custom cents offset for an absolute chromatic key (0=C, 1=C#, etc.)
    void setCustomChromaOffset (int chromaKey, float cents)
    {
        chromaKey = ((chromaKey % 12) + 12) % 12;
        settings.chromaticCentsOffsets[static_cast<size_t>(chromaKey)].store (cents);
        settings.tuningMode.store (static_cast<int>(TuningMode::Custom));
    }

    // Reset all notes to zero (Equal Temperament)
    void resetAllToEqualTemperament()
    {
        for (auto& val : settings.chromaticCentsOffsets)
            val.store (0.0f);
        settings.tuningMode.store (static_cast<int>(TuningMode::Custom));
    }

private:
    MicrotuningSettings settings;
    MicrotuningLiveState liveState;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MicrotuningProcessor)
};

} // namespace MidiMonitor
