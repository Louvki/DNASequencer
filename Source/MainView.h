#pragma once

#include <functional>

#include <JuceHeader.h>

#include "Sequencer/AminoAcidPlaybackSettingsComponent.h"
#include "Sequencer/AminoAcidSequencePlayer.h"
#include "Sequencer/MidiClockService.h"
#include "Sequencer/MidiInputSelectorComponent.h"
#include "Sequencer/PlaybackStatusComponent.h"
#include "LoadFile/SequenceFileLoaderComponent.h"

class MainView : public juce::Component
{
public:
    MainView (juce::MidiInputCallback& midiCallbackTarget,
              MidiClockService& clockService,
              AminoAcidSequencePlayer& sequencePlayer,
              std::function<void()> onResetReadPosition);

    SequenceFileLoaderComponent& getSequenceFileLoader() noexcept { return sequenceFileLoader; }

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    MidiInputSelectorComponent midiInputSelector;
    PlaybackStatusComponent playbackStatus;
    AminoAcidPlaybackSettingsComponent aminoAcidPlaybackSettings;
    SequenceFileLoaderComponent sequenceFileLoader;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainView)
};
