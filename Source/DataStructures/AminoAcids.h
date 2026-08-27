#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <JuceHeader.h>

namespace dna
{
enum class AminoAcidProperty
{
    nonpolar = 1,
    uncharged = 2,
    positiveCharge = 3,
    negativeCharge = 4
};

struct AminoAcid
{
    std::string name;
    AminoAcidProperty charge;
    std::vector<std::string> codons;
    int codonNoteValue { 0 };
    int codonNoteVelocity { 127 };
    std::vector<int> codonChordNotes {};
};

/** MIDI note and velocity assigned to a DNA codon. */
struct MidiMessage
{
    int note = 0;
    int velocity = 127;
    std::vector<int> chordNotes;
};

/** Returns the canonical table of all 20 amino acids and their codons. */
const std::vector<AminoAcid>& getDefaultAminoAcids();

/** Fast lookup from DNA codons to scaled playback parameters. */
class AminoAcids
{
public:
    /** Rebuilds the codon lookup table from provided amino acid data. */
    void rebuildCodonMidiPlaybackMap (const std::vector<AminoAcid>& aminoAcids);

    /** Returns note/velocity for a codon, or nullopt if the codon is unknown. */
    std::optional<MidiMessage> lookupCodon (const juce::String& codon) const;

    /** Returns true for stop codons (TAA, TAG, TGA). */
    bool isStopCodon (const juce::String& codon) const noexcept;

private:
    /** A lookup map containing note and velocity for each DNA codone  */
    std::unordered_map<juce::String, MidiMessage> codonMidiPlaybackMap;
};
}
