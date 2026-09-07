#pragma once

#include <vector>

#include <JuceHeader.h>

#include "DataStructures/AminoAcids.h"

namespace dna
{
enum class MidiScale
{
    chromatic = 0,
    majorIonian,
    naturalMinor,
    harmonicMinor,
    phrygian,
    lydian,
    mixolydian,
    wholeTone,
    diminished,
    lydianDominant,
    hungarianMinor,
    pentatonicMinor,
    blues
};

enum class ChordType
{
    triad = 0,
    seventh,
    ninth,
    eleventh,
    thirteenth
};

juce::String getMidiScaleLabel (MidiScale scale);
const std::vector<MidiScale>& getAllMidiScales();

std::vector<int> buildScaleNotes (int rootNote, const std::vector<int>& scaleSteps);
std::vector<int> buildDiatonicChord (int note, int rootNote, MidiScale scale, ChordType type);
std::vector<AminoAcid> applyScaleToAminoAcids (int rootNote, MidiScale scale, int notePoolSize);
}
