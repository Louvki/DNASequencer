#include "MainView.h"

namespace { } // namespace

MainView::MainView (juce::MidiInputCallback& midiCallbackTarget,
                    MidiClockService& clockService,
                    AminoAcidSequencePlayer& sequencePlayer,
                    std::function<void()> resetReadPosition)
    : onResetReadPosition (std::move (resetReadPosition)),
      midiInputSelector (midiCallbackTarget),
      playbackStatus (clockService, sequencePlayer),
      aminoAcidPlaybackSettings (sequencePlayer, clockService),
      sequenceFileLoader (onResetReadPosition)
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
    sequenceFileLoader.setBounds (r.removeFromTop (116));
    playbackStatus.setBounds (r.removeFromTop (40));
    aminoAcidPlaybackSettings.setBounds (r.removeFromTop (166));
}
