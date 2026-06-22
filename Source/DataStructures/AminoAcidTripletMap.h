#pragma once

#include <optional>
#include <unordered_map>

#include <JuceHeader.h>

#include "DataStructures/AminoAcids.h"

namespace dna
{
struct TripletPlayback
{
    int note = 0;
    int velocity = 127;
};

class AminoAcidTripletMap
{
public:
    void rebuildFromScaledAminoAcids (const std::vector<AminoAcid>& scaledAminoAcids);

    std::optional<TripletPlayback> lookupTriplet (const juce::String& triplet) const;
    bool isStopCodon (const juce::String& triplet) const noexcept;

private:
    std::unordered_map<juce::String, TripletPlayback> tripletToPlayback;
};
}
