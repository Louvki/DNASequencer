#include "DataStructures/AminoAcids.h"

namespace dna
{

const std::vector<AminoAcid>& getDefaultAminoAcids()
{
    static const std::vector<AminoAcid> aminoAcids
    {
        { "Ala", AminoAcidProperty::nonpolar,       { "GCT", "GCC", "GCA", "GCG" } },
        { "Arg", AminoAcidProperty::positiveCharge, { "CGT", "CGC", "CGA", "CGG", "AGA", "AGG" } },
        { "Asn", AminoAcidProperty::uncharged,      { "AAT", "AAC" } },
        { "Asp", AminoAcidProperty::negativeCharge, { "GAT", "GAC" }},
        { "Cys", AminoAcidProperty::uncharged,      { "TGT", "TGC" }},
        { "Gln", AminoAcidProperty::uncharged,      { "CAA", "CAG" }},
        { "Glu", AminoAcidProperty::negativeCharge, { "GAA", "GAG" }},
        { "Gly", AminoAcidProperty::nonpolar,       { "GGT", "GGC", "GGA", "GGG" }},
        { "His", AminoAcidProperty::positiveCharge, { "CAT", "CAC" }},
        { "Ile", AminoAcidProperty::nonpolar,       { "ATT", "ATC", "ATA" }},
        { "Leu", AminoAcidProperty::nonpolar,       { "TTA", "TTG", "CTT", "CTC", "CTA", "CTG" }},
        { "Lys", AminoAcidProperty::positiveCharge, { "AAA", "AAG" }},
        { "Met", AminoAcidProperty::nonpolar,       { "ATG" }},
        { "Phe", AminoAcidProperty::nonpolar,       { "TTT", "TTC" }},
        { "Pro", AminoAcidProperty::nonpolar,       { "CCT", "CCC", "CCA", "CCG" }},
        { "Ser", AminoAcidProperty::uncharged,      { "TCT", "TCC", "TCA", "TCG", "AGT", "AGC" }},
        { "Thr", AminoAcidProperty::uncharged,      { "ACT", "ACC", "ACA", "ACG" }},
        { "Trp", AminoAcidProperty::nonpolar,       { "TGG" }},
        { "Tyr", AminoAcidProperty::uncharged,      { "TAT", "TAC" }},
        { "Val", AminoAcidProperty::nonpolar,       { "GTT", "GTC", "GTA", "GTG" }}
    };

    return aminoAcids;
}

void AminoAcids::rebuildCodonMidiPlaybackMap (const std::vector<AminoAcid>& aminoAcids)
{
    codonMidiPlaybackMap.clear();

    for (const auto& aminoAcid : aminoAcids)
    {
        for (const auto& codon : aminoAcid.codons)
            codonMidiPlaybackMap[juce::String (codon)] = { aminoAcid.codonNoteValue, aminoAcid.codonNoteVelocity };
    }
}

// Finds a codon based on its name (fx "CGT") and returns note and velocity. 
std::optional<MidiMessage> AminoAcids::lookupCodon (const juce::String& codon) const
{
    const auto it = codonMidiPlaybackMap.find (codon);
    if (it == codonMidiPlaybackMap.end())
        return std::nullopt;

    return it->second;
}

bool AminoAcids::isStopCodon (const juce::String& codon) const noexcept
{
    return codon == "TAA" || codon == "TAG" || codon == "TGA";
}
}
