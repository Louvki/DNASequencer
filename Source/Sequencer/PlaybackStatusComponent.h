#pragma once

#include <JuceHeader.h>

#include <functional>

#include "Sequencer/AminoAcidSequencePlayer.h"
#include "Sequencer/MidiClockService.h"

/** Transport / clock activity LED and read-position display. */
class PlaybackStatusComponent : public juce::Component,
                                private juce::Timer
{
public:
    PlaybackStatusComponent (MidiClockService& clockService,
                             AminoAcidSequencePlayer& sequencePlayer,
                             std::function<void()> onResetClicked = nullptr);

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;
    void refreshLabels();

    MidiClockService& clockService;
    AminoAcidSequencePlayer& sequencePlayer;

    juce::Label clockStatusLabel;
    juce::Label scanProgressLabel;

    juce::TextButton playButton { "Play" };
    juce::TextButton pauseButton { "Pause" };
    juce::TextButton resetButton { "Reset" };

    std::function<void()> onResetClicked;

    juce::Colour ledColour { juce::Colours::grey };
    juce::Rectangle<int> ledBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PlaybackStatusComponent)
};
