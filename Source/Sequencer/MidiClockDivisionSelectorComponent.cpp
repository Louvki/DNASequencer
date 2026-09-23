#include "Sequencer/MidiClockDivisionSelectorComponent.h"

MidiClockDivisionSelectorComponent::MidiClockDivisionSelectorComponent (AminoAcidSequencePlayer& player)
    : sequencePlayer (player)
{
    addAndMakeVisible (divisionBox);

    divisionBox.onChange = [this] { divisionSelectionChanged(); };

    populateDivisionList();
}

void MidiClockDivisionSelectorComponent::resized()
{
    divisionBox.setBounds (getLocalBounds().removeFromTop (28));
}

void MidiClockDivisionSelectorComponent::populateDivisionList()
{
    divisionBox.clear (juce::dontSendNotification);

    const auto divisions = getAllMidiClockDivisions();
    for (int i = 0; i < divisions.size(); ++i)
    {
        const auto division = divisions.getReference (i);
        divisionBox.addItem (getMidiClockDivisionLabel (division), i + 1);
    }

    const auto defaultIndex = divisions.indexOf (MidiClockDivision::quarterNote);
    const auto selectedIndex = divisions.indexOf (sequencePlayer.getDivision());
    divisionBox.setSelectedId ((selectedIndex >= 0 ? selectedIndex : defaultIndex) + 1, juce::dontSendNotification);
}

void MidiClockDivisionSelectorComponent::divisionSelectionChanged()
{
    const int selectedIndex = divisionBox.getSelectedItemIndex();
    const auto divisions = getAllMidiClockDivisions();

    if (selectedIndex < 0 || selectedIndex >= divisions.size())
        return;

    sequencePlayer.setDivision (divisions.getReference (selectedIndex));
}
