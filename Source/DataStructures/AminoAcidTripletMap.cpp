#include "DataStructures/AminoAcidTripletMap.h"

namespace dna
{
namespace
{
bool isStopTriplet (const juce::String& triplet) noexcept
{
    return triplet == "TAA" || triplet == "TAG" || triplet == "TGA";
}
} // namespace

void AminoAcidTripletMap::rebuildFromScaledAminoAcids (const std::vector<AminoAcid>& scaledAminoAcids)
{
    tripletToPlayback.clear();

    for (const auto& aminoAcid : scaledAminoAcids)
    {
        const TripletPlayback playback { aminoAcid.tripletNoteValue, aminoAcid.tripletNoteVelocity };

        for (const auto& triplet : aminoAcid.nucleoTriplets)
            tripletToPlayback[juce::String (triplet)] = playback;
    }
}

std::optional<TripletPlayback> AminoAcidTripletMap::lookupTriplet (const juce::String& triplet) const
{
    const auto it = tripletToPlayback.find (triplet);
    if (it == tripletToPlayback.end())
        return std::nullopt;

    return it->second;
}

bool AminoAcidTripletMap::isStopCodon (const juce::String& triplet) const noexcept
{
    return isStopTriplet (triplet);
}
}
