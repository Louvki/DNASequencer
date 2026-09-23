#pragma once

#include <array>

#include <JuceHeader.h>

#include "Sequencer/AminoAcidSequencePlayer.h"
#include "Sequencer/MidiClockDivisionSelectorComponent.h"

class AminoAcidPlaybackSettingsComponent : public juce::Component
{
public:
    AminoAcidPlaybackSettingsComponent (AminoAcidSequencePlayer& player);

    void resized() override;

private:
    struct ChordTypeWeightControl
    {
        juce::Label label;
        juce::Slider slider;
        juce::Label shareLabel;
    };

    void applySettingsToPlayer();
    void populateScaleList();
    void updateSliderValueLabels();
    void updateDurationControlAppearance();
    void updateChordControlAppearance();
    void updateChordTypeShareLabels();

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

    juce::Label chordChanceLabel;
    juce::Slider chordChanceSlider;

    juce::Label chordTypeMixLabel;

    std::array<ChordTypeWeightControl, 5> chordTypeWeightControls;

    juce::Label chordStrumLabel;
    juce::Slider chordStrumSlider;

    juce::Label chordVelocityLabel;
    juce::Slider chordVelocitySlider;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AminoAcidPlaybackSettingsComponent)
};
