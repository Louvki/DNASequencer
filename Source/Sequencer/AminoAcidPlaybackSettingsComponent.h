#pragma once

#include <JuceHeader.h>

#include "Sequencer/AminoAcidSequencePlayer.h"
#include "Sequencer/MidiClockDivisionSelectorComponent.h"
#include "Sequencer/MidiClockService.h"

class AminoAcidPlaybackSettingsComponent : public juce::Component
{
public:
    AminoAcidPlaybackSettingsComponent (AminoAcidSequencePlayer& player,
                                        MidiClockService& clockService);

    void resized() override;

private:
    void applySettingsToPlayer();
    void populateScaleList();
    void updateSliderValueLabels();
    void updateDurationControlAppearance();

    AminoAcidSequencePlayer& sequencePlayer;

    juce::Label rootNoteLabel;
    juce::Slider rootNoteSlider;
    juce::Label rootNoteValueLabel;

    juce::ComboBox scaleBox;
    MidiClockDivisionSelectorComponent clockDivisionSelector;

    juce::Label notePoolLabel;
    juce::Slider notePoolSlider;
    juce::Label notePoolValueLabel;

    juce::Label whitespaceLabel;
    juce::Slider whitespaceSlider;
    juce::Label whitespaceValueLabel;

    juce::Label noteDurationLabel;
    juce::Slider noteDurationSlider;
    juce::Label noteDurationValueLabel;

    juce::Label sustainLabel;
    juce::ToggleButton sustainToggle;

    juce::Label chordsLabel;
    juce::ToggleButton chordsToggle;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AminoAcidPlaybackSettingsComponent)
};
