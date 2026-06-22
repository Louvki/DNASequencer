#include "Sequencer/PlaybackStatusComponent.h"

namespace
{
constexpr int ledDiameter = 12;
constexpr int clockActivityWindowMs = 10000;
constexpr int transportButtonWidth = 52;
constexpr int transportButtonHeight = 24;
constexpr int transportButtonGap = 6;
} // namespace

PlaybackStatusComponent::PlaybackStatusComponent (MidiClockService& service,
                                                  AminoAcidSequencePlayer& player)
    : clockService (service),
      sequencePlayer (player)
{
    clockStatusLabel.setJustificationType (juce::Justification::centredLeft);
    readIndexLabel.setJustificationType (juce::Justification::centredLeft);

    addAndMakeVisible (clockStatusLabel);
    addAndMakeVisible (readIndexLabel);

    addAndMakeVisible (playButton);
    playButton.onClick = [this]
    {
        clockService.setLocalPaused (false);
        refreshLabels();
    };

    addAndMakeVisible (pauseButton);
    pauseButton.onClick = [this]
    {
        clockService.setLocalPaused (true);
        sequencePlayer.stopActiveNote();
        refreshLabels();
    };

    refreshLabels();
    startTimerHz (20);
}

void PlaybackStatusComponent::paint (juce::Graphics& g)
{
    auto ledArea = getLocalBounds().removeFromLeft (ledDiameter + 8).withSizeKeepingCentre (ledDiameter, ledDiameter);
    g.setColour (ledColour);
    g.fillEllipse (ledArea.toFloat());
    g.setColour (ledColour.darker (0.35f));
    g.drawEllipse (ledArea.toFloat(), 1.0f);
}

void PlaybackStatusComponent::resized()
{
    auto r = getLocalBounds();

    const auto buttonsWidth = transportButtonWidth * 2 + transportButtonGap;
    auto buttonArea = r.removeFromRight (buttonsWidth).withSizeKeepingCentre (buttonsWidth, transportButtonHeight);
    pauseButton.setBounds (buttonArea.removeFromRight (transportButtonWidth));
    buttonArea.removeFromRight (transportButtonGap);
    playButton.setBounds (buttonArea);

    auto labelArea = r.withTrimmedLeft (ledDiameter + 12);
    clockStatusLabel.setBounds (labelArea.removeFromTop (18));
    readIndexLabel.setBounds (labelArea.removeFromTop (18));
}

void PlaybackStatusComponent::timerCallback()
{
    refreshLabels();
}

void PlaybackStatusComponent::refreshLabels()
{
    if (clockService.isLocalPaused())
    {
        ledColour = juce::Colours::grey.darker (0.2f);
        clockStatusLabel.setText ("Paused", juce::dontSendNotification);
    }
    else if (clockService.hasRecentClockActivity (clockActivityWindowMs))
    {
        ledColour = juce::Colours::limegreen;
        clockStatusLabel.setText ("MIDI clock running", juce::dontSendNotification);
    }
    else
    {
        ledColour = juce::Colours::grey.darker (0.4f);
        clockStatusLabel.setText ("MIDI clock stopped", juce::dontSendNotification);
    }

    const auto readIndex = sequencePlayer.getCurrentReadIndex();
    const auto mode = sequencePlayer.isReadingCodons() ? "codons" : "scanning";
    readIndexLabel.setText ("Read index: " + juce::String (readIndex) + " (" + mode + ")",
                            juce::dontSendNotification);

    repaint();
}
