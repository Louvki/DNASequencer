#pragma once

#include <JuceHeader.h>

#include "Sequencer/MidiClockService.h"

/** Combo box for selecting the MIDI clock grid division. */
class MidiClockDivisionSelectorComponent : public juce::Component
{
public:
    explicit MidiClockDivisionSelectorComponent (MidiClockService& clockService);

    void resized() override;

private:
    void populateDivisionList();
    void divisionSelectionChanged();

    MidiClockService& clockService;
    juce::ComboBox divisionBox;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiClockDivisionSelectorComponent)
};
