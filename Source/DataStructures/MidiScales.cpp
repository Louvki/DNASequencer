#include "DataStructures/MidiScales.h"

#include <algorithm>

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
    int octave = 0; // 0 = root octave, 1 = +12 semitones, 2 = +24, ...

    // Walk up octave by octave until an entire pass produces no notes in 0–127.
    // Infinite loop; repeatedly executes the loop body until an explicit 'break' is reached
    for (;;)
    {
        std::vector<int> octaveNotes;
        octaveNotes.reserve (scaleSteps.size());


        // Go over the scale steps for the current octave.
        for (const auto step : scaleSteps)
        {
            const auto note = rootNote + step + (octave * 12);
            if (isValidMidiNote (note))
                octaveNotes.push_back (note);
        }

        // climbed past 127 — nothing left to collect
        if (octaveNotes.empty())
            break; 

        // Add up the notes and increase the octave
        notes.insert (notes.end(), octaveNotes.begin(), octaveNotes.end());
        ++octave;
    }

    return notes;
}

std::vector<int> buildDiatonicTriad (int note, int rootNote, MidiScale scale)
{
    const auto scaleNotes = buildScaleNotes (rootNote, getScaleSteps (scale));

    const auto it = std::find (scaleNotes.begin(), scaleNotes.end(), note);
    if (it == scaleNotes.end())
        return { note };

    const auto index = static_cast<size_t> (std::distance (scaleNotes.begin(), it));
    std::vector<int> triad;
    triad.reserve (3);
    triad.push_back (scaleNotes[index]);

    if (index + 2 < scaleNotes.size())
        triad.push_back (scaleNotes[index + 2]);

    if (index + 4 < scaleNotes.size())
        triad.push_back (scaleNotes[index + 4]);

    return triad;
}

std::vector<AminoAcid> applyScaleToAminoAcids (int rootNote, MidiScale scale, int notePoolSize, bool chordsEnabled)
{
    auto aminoAcids = getDefaultAminoAcids();
    auto availableNotes = buildScaleNotes (rootNote, getScaleSteps (scale));

    if (availableNotes.empty())
        availableNotes.push_back (juce::jlimit (midiMin, midiMax, rootNote));

    const auto limit = juce::jlimit (1, 20, notePoolSize);
    if ((int) availableNotes.size() > limit)
        availableNotes.resize ((size_t) limit);

    for (size_t i = 0; i < aminoAcids.size(); ++i)
    {
        const auto note = availableNotes[i % availableNotes.size()];
        aminoAcids[i].codonNoteValue = note;

        if (chordsEnabled)
            aminoAcids[i].codonChordNotes = buildDiatonicTriad (note, rootNote, scale);
        else
            aminoAcids[i].codonChordNotes.clear();
    }

    return aminoAcids;
}
}
