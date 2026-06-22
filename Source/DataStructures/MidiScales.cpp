#include "DataStructures/MidiScales.h"

namespace dna
{
namespace
{
constexpr int midiMin = 0;
constexpr int midiMax = 127;

bool isValidMidiNote (int note) noexcept
{
    return note >= midiMin && note <= midiMax;
}

const std::vector<int>& getScaleSteps (MidiScale scale)
{
    static const std::vector<std::vector<int>> allScaleSteps {
        { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 },
        { 0, 2, 4, 5, 7, 9, 11 },
        { 0, 2, 3, 5, 7, 8, 10 },
        { 0, 2, 3, 5, 7, 8, 11 },
        { 0, 1, 3, 5, 7, 8, 10 },
        { 0, 2, 4, 6, 7, 9, 11 },
        { 0, 2, 4, 5, 7, 9, 10 },
        { 0, 2, 4, 6, 8, 10 },
        { 0, 2, 3, 5, 6, 8, 9, 11 },
        { 0, 2, 4, 6, 7, 9, 10 },
        { 0, 2, 3, 6, 7, 8, 11 },
        { 0, 3, 5, 7, 10 },
        { 0, 3, 5, 6, 7, 10 }
    };

    const auto index = static_cast<size_t> (scale);
    if (index >= allScaleSteps.size())
        return allScaleSteps[0];

    return allScaleSteps[index];
}
} // namespace

juce::String getMidiScaleLabel (MidiScale scale)
{
    static const juce::StringArray labels {
        "Chromatic",
        "Major (Ionian)",
        "Natural Minor",
        "Harmonic Minor",
        "Phrygian",
        "Lydian",
        "Mixolydian",
        "Whole Tone",
        "Diminished",
        "Lydian Dominant",
        "Hungarian Minor",
        "Pentatonic Minor",
        "Blues"
    };

    const auto index = static_cast<int> (scale);
    if (index >= 0 && index < labels.size())
        return labels[index];

    return labels[0];
}

const std::vector<MidiScale>& getAllMidiScales()
{
    static const std::vector<MidiScale> scales {
        MidiScale::chromatic,
        MidiScale::majorIonian,
        MidiScale::naturalMinor,
        MidiScale::harmonicMinor,
        MidiScale::phrygian,
        MidiScale::lydian,
        MidiScale::mixolydian,
        MidiScale::wholeTone,
        MidiScale::diminished,
        MidiScale::lydianDominant,
        MidiScale::hungarianMinor,
        MidiScale::pentatonicMinor,
        MidiScale::blues
    };

    return scales;
}

std::vector<int> buildScaleNotes (int rootNote, const std::vector<int>& scaleSteps)
{
    std::vector<int> notes;
    int octave = 0;

    for (;;)
    {
        std::vector<int> octaveNotes;
        octaveNotes.reserve (scaleSteps.size());

        for (const auto step : scaleSteps)
        {
            const auto note = rootNote + step + (octave * 12);
            if (isValidMidiNote (note))
                octaveNotes.push_back (note);
        }

        if (octaveNotes.empty())
            break;

        notes.insert (notes.end(), octaveNotes.begin(), octaveNotes.end());
        ++octave;
    }

    return notes;
}

std::vector<AminoAcid> applyScaleToAminoAcids (int rootNote, MidiScale scale, int notePoolSize)
{
    auto aminoAcids = getAminoAcids();
    auto availableNotes = buildScaleNotes (rootNote, getScaleSteps (scale));

    if (availableNotes.empty())
        availableNotes.push_back (juce::jlimit (midiMin, midiMax, rootNote));

    const auto limit = juce::jlimit (1, 20, notePoolSize);
    if ((int) availableNotes.size() > limit)
        availableNotes.resize ((size_t) limit);

    for (size_t i = 0; i < aminoAcids.size(); ++i)
        aminoAcids[i].codonNoteValue = availableNotes[i % availableNotes.size()];

    return aminoAcids;
}
}
