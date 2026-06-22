#pragma once

#include <string>
#include <vector>

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
    std::vector<std::string> nucleoTriplets;
    int tripletNoteValue { 0 };
    int tripletNoteVelocity { 127 };
    std::vector<int> tripletChordNotes;
    std::string function;
};

const std::vector<AminoAcid>& getAminoAcids();
}
