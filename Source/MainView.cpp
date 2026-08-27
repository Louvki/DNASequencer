#include "MainView.h"

namespace { } // namespace

MainView::MainView (juce::MidiInputCallback& midiCallbackTarget,
                    MidiClockService& clockService,
                    AminoAcidSequencePlayer& sequencePlayer,
                    std::function<void()> resetReadPosition)
    : midiInputSelector (midiCallbackTarget),
      playbackStatus (clockService, sequencePlayer, std::move (resetReadPosition)),
      aminoAcidPlaybackSettings (sequencePlayer, clockService),
      sequenceFileLoader()
{
    addAndMakeVisible (midiInputSelector);
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
    midiInputSelector.setBounds (r.removeFromTop (60));
    sequenceFileLoader.setBounds (r.removeFromTop (72));
    playbackStatus.setBounds (r.removeFromTop (70));
    aminoAcidPlaybackSettings.setBounds (r.removeFromTop (242));
}
