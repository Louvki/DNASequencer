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
    std::vector<int> codonChordNotes;
    std::string function;
};

/** MIDI note and velocity assigned to a DNA codon codon. */
struct CodonPlayback
{
    int note = 0;
    int velocity = 127;
};

/** Returns the canonical table of all 20 amino acids and their codons. */
const std::vector<AminoAcid>& getAminoAcids();

/** Fast lookup from DNA codons to scaled playback parameters. */
class AminoAcids
{
public:
    /** Rebuilds the codon lookup table from scale-adjusted amino acid data. */
    void rebuildFromScaledAminoAcids (const std::vector<AminoAcid>& scaledAminoAcids);

    /** Returns note/velocity for a codon, or nullopt if the codon is unknown. */
    std::optional<CodonPlayback> lookupCodon (const juce::String& codon) const;

    /** Returns true for stop codons (TAA, TAG, TGA). */
    bool isStopCodon (const juce::String& codon) const noexcept;

private:
    std::unordered_map<juce::String, CodonPlayback> codonToPlayback;
};
}
