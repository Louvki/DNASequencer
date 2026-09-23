#pragma once

#include <JuceHeader.h>

#include "MainView.h"
#include "Sequencer/AminoAcidSequencePlayer.h"
#include "Sequencer/MidiClockInputService.h"
#include "Sequencer/MidiClockService.h"
#include "Sequencer/MidiOutputBusPool.h"
#include "Sequencer/TabResourceCoordinator.h"

/** Owns one tab's player, UI, and resource registrations. */
class SequencerSession : public juce::Component
{
public:
    SequencerSession (int tabId,
                      MidiClockService& clockService,
                      TabResourceCoordinator& coordinator,
                      MidiClockInputService& clockInputService,
                      MidiOutputBusPool& outputPool);
    ~SequencerSession() override;

    int getTabId() const noexcept { return tabId; }
    AminoAcidSequencePlayer& getPlayer() noexcept { return player; }

    void prepareForRemoval();
    void applyExcludedClockBus (const juce::String& excludedBusName);

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    void wirePlayerProviders();
    void wireResourceValidation();
    void handleResetClicked();
    void handleOutputChanged (const juce::String& deviceIdentifier);
    bool isOutputAllowed (const juce::String& deviceIdentifier) const;

    bool preparedForRemoval = false;

    int tabId;
    MidiClockService& clockService;
    TabResourceCoordinator& coordinator;
    MidiClockInputService& clockInputService;

    AminoAcidSequencePlayer player;
    MainView view;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SequencerSession)
};
