#include "MainView.h"

MainView::MainView (AminoAcidSequencePlayer& sequencePlayer,
                    MidiClockService& clockService,
                    MidiOutputBusPool& outputPool,
                    std::function<void()> resetReadPosition)
    : midiOutputSelector (outputPool),
      playbackStatus (clockService, sequencePlayer, std::move (resetReadPosition)),
      aminoAcidPlaybackSettings (sequencePlayer)
{
    addAndMakeVisible (midiOutputSelector);
    addAndMakeVisible (sequenceFileLoader);
    addAndMakeVisible (playbackStatus);
    addAndMakeVisible (aminoAcidPlaybackSettings);
}

void MainView::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void MainView::resized()
{
    auto r = getLocalBounds().reduced (12);
    midiOutputSelector.setBounds (r.removeFromTop (60));
    sequenceFileLoader.setBounds (r.removeFromTop (72));
    playbackStatus.setBounds (r.removeFromTop (70));
    aminoAcidPlaybackSettings.setBounds (r.removeFromTop (454));
}
