#pragma once

#include <JuceHeader.h>

#include "Sequencer/AminoAcidSequencePlayer.h"
#include "Sequencer/MidiClockService.h"

/** Transport / clock activity LED and read-position display. */
class PlaybackStatusComponent : public juce::Component,
                                private juce::Timer
{
public:
    PlaybackStatusComponent (MidiClockService& clockService,
                             AminoAcidSequencePlayer& sequencePlayer);

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;
    void refreshLabels();

    MidiClockService& clockService;
    AminoAcidSequencePlayer& sequencePlayer;

    juce::Label clockStatusLabel;
    juce::Label readIndexLabel;

    juce::TextButton playButton { "Play" };
    juce::TextButton pauseButton { "Pause" };

    juce::Colour ledColour { juce::Colours::grey };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PlaybackStatusComponent)
};
