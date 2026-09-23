#pragma once

#include <functional>
#include <memory>
#include <vector>

#include <JuceHeader.h>

#include "Sequencer/MidiClockInputService.h"
#include "Sequencer/MidiClockService.h"
#include "Sequencer/MidiOutputBusPool.h"
#include "Sequencer/SequencerSession.h"
#include "Sequencer/TabResourceCoordinator.h"

/** Tab bar and active session content for up to 12 sequencer tabs. */
class SequencerTabContainer : public juce::Component,
                              private juce::ChangeListener
{
public:
    static constexpr int maxTabs = 12;

    SequencerTabContainer (MidiClockService& clockService,
                           TabResourceCoordinator& coordinator,
                           MidiClockInputService& clockInputService,
                           MidiOutputBusPool& outputPool);
    ~SequencerTabContainer() override;

    void forEachSession (const std::function<void (SequencerSession&)>& callback);
    void refreshOutputDeviceLists (const juce::String& excludedBusName);

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    void addTab();
    void removeCurrentTab();
    void showSession (int index);
    void updateTabControls();
    void changeListenerCallback (juce::ChangeBroadcaster* source) override;

    MidiClockService& clockService;
    TabResourceCoordinator& coordinator;
    MidiClockInputService& clockInputService;
    MidiOutputBusPool& outputPool;

    juce::TabbedButtonBar tabBar { juce::TabbedButtonBar::TabsAtTop };
    juce::TextButton addTabButton { "+" };
    juce::TextButton closeTabButton { "x" };

    std::vector<std::unique_ptr<SequencerSession>> sessions;
    int nextTabId = 1;
    int activeSessionIndex = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SequencerTabContainer)
};
