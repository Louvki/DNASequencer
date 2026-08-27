#include "Sequencer/PlaybackStatusComponent.h"

#include "LoadFile/DnaFastaLoader.h"

namespace
{
constexpr int ledDiameter = 12;
constexpr int clockActivityWindowMs = 10000;
constexpr int transportButtonWidth = 52;
constexpr int transportButtonHeight = 24;
constexpr int transportButtonGap = 6;
constexpr int clockStatusRowHeight = 18;
constexpr int layoutRowGap = 4;
constexpr int transportRowBottomMargin = 24;
constexpr int resetButtonRightMargin = 24;
} // namespace

PlaybackStatusComponent::PlaybackStatusComponent (MidiClockService& service,
                                                  AminoAcidSequencePlayer& player,
                                                  std::function<void()> resetClicked)
    : clockService (service),
      sequencePlayer (player),
      onResetClicked (std::move (resetClicked))
{
    clockStatusLabel.setJustificationType (juce::Justification::centredLeft);

    addAndMakeVisible (clockStatusLabel);

    scanProgressLabel.setJustificationType (juce::Justification::centredLeft);
    scanProgressLabel.setFont (juce::FontOptions (12.0f));
    addAndMakeVisible (scanProgressLabel);

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

    addAndMakeVisible (resetButton);
    resetButton.onClick = [this]
    {
        if (onResetClicked != nullptr)
            onResetClicked();

        refreshLabels();
    };
    resetButton.setVisible (onResetClicked != nullptr);

    refreshLabels();
    startTimerHz (20);
}

void PlaybackStatusComponent::paint (juce::Graphics& g)
{
    g.setColour (ledColour);
    g.fillEllipse (ledBounds.toFloat());
    g.setColour (ledColour.darker (0.35f));
    g.drawEllipse (ledBounds.toFloat(), 1.0f);
}

void PlaybackStatusComponent::resized()
{
    auto r = getLocalBounds();

    auto clockRow = r.removeFromTop (clockStatusRowHeight);
    ledBounds = clockRow.removeFromLeft (ledDiameter + 8).withSizeKeepingCentre (ledDiameter, ledDiameter);
    clockStatusLabel.setBounds (clockRow);

    r.removeFromTop (layoutRowGap);
    r.removeFromBottom (transportRowBottomMargin);

    auto buttonRow = r.removeFromTop (transportButtonHeight);

    playButton.setBounds (buttonRow.removeFromLeft (transportButtonWidth));
    buttonRow.removeFromLeft (transportButtonGap);
    pauseButton.setBounds (buttonRow.removeFromLeft (transportButtonWidth));
    buttonRow.removeFromLeft (transportButtonGap);

    if (resetButton.isVisible())
    {
        auto resetArea = buttonRow.removeFromRight (transportButtonWidth + resetButtonRightMargin);
        resetButton.setBounds (resetArea.removeFromLeft (transportButtonWidth));
    }

    scanProgressLabel.setBounds (buttonRow);
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
    const auto sequenceLength = sequencePlayer.getSequenceLength();
    scanProgressLabel.setText (juce::String (readIndex) + " / "
                               + dna::DnaFastaLoader::formatCountForDisplay (sequenceLength),
                               juce::dontSendNotification);

    repaint();
}
