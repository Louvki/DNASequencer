#pragma once

#include <JuceHeader.h>

#include "Sequencer/AminoAcidSequencePlayer.h"

/** Combo box for selecting the MIDI clock grid division. */
class MidiClockDivisionSelectorComponent : public juce::Component
{
public:
    explicit MidiClockDivisionSelectorComponent (AminoAcidSequencePlayer& sequencePlayer);

    void resized() override;

private:
    void populateDivisionList();
    void divisionSelectionChanged();

    AminoAcidSequencePlayer& sequencePlayer;
    juce::ComboBox divisionBox;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiClockDivisionSelectorComponent)
};
