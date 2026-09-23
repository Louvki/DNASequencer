#pragma once

#include <functional>

#include <JuceHeader.h>

#include "Sequencer/AminoAcidPlaybackSettingsComponent.h"
#include "Sequencer/AminoAcidSequencePlayer.h"
#include "Sequencer/MidiClockService.h"
#include "Sequencer/MidiOutputBusPool.h"
#include "Sequencer/MidiOutputSelectorComponent.h"
#include "Sequencer/PlaybackStatusComponent.h"
#include "LoadFile/SequenceFileLoaderComponent.h"

/** Per-tab UI panel: output selector, file loader, transport, and playback settings. */
class MainView : public juce::Component
{
public:
    MainView (AminoAcidSequencePlayer& sequencePlayer,
              MidiClockService& clockService,
              MidiOutputBusPool& outputPool,
              std::function<void()> onResetReadPosition);

    SequenceFileLoaderComponent& getSequenceFileLoader() noexcept { return sequenceFileLoader; }
    MidiOutputSelectorComponent& getMidiOutputSelector() noexcept { return midiOutputSelector; }

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    MidiOutputSelectorComponent midiOutputSelector;
    SequenceFileLoaderComponent sequenceFileLoader;
    PlaybackStatusComponent playbackStatus;
    AminoAcidPlaybackSettingsComponent aminoAcidPlaybackSettings;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainView)
};
