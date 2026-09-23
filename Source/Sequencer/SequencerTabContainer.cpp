#include "Sequencer/SequencerTabContainer.h"

namespace
{
constexpr int kTabBarHeight = 32;
constexpr int kTabControlWidth = 28;
constexpr int kTabControlGap = 4;
} // namespace

SequencerTabContainer::SequencerTabContainer (MidiClockService& service,
                                              TabResourceCoordinator& resourceCoordinator,
                                              MidiClockInputService& inputService,
                                              MidiOutputBusPool& busPool)
    : clockService (service),
      coordinator (resourceCoordinator),
      clockInputService (inputService),
      outputPool (busPool)
{
    tabBar.addChangeListener (this);
    addAndMakeVisible (tabBar);

    addTabButton.onClick = [this] { addTab(); };
    addAndMakeVisible (addTabButton);

    closeTabButton.onClick = [this] { removeCurrentTab(); };
    addAndMakeVisible (closeTabButton);

    addTab();
}

SequencerTabContainer::~SequencerTabContainer()
{
    tabBar.removeChangeListener (this);

    while (! sessions.empty())
    {
        sessions.back()->prepareForRemoval();
        sessions.pop_back();
    }
}

void SequencerTabContainer::forEachSession (const std::function<void (SequencerSession&)>& callback)
{
    if (callback == nullptr)
        return;

    for (auto& session : sessions)
        callback (*session);
}

void SequencerTabContainer::refreshOutputDeviceLists (const juce::String& excludedBusName)
{
    for (auto& session : sessions)
        session->applyExcludedClockBus (excludedBusName);
}

void SequencerTabContainer::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void SequencerTabContainer::resized()
{
    auto bounds = getLocalBounds();

    auto tabRow = bounds.removeFromTop (kTabBarHeight);
    closeTabButton.setBounds (tabRow.removeFromRight (kTabControlWidth));
    tabRow.removeFromRight (kTabControlGap);
    addTabButton.setBounds (tabRow.removeFromRight (kTabControlWidth));
    tabRow.removeFromRight (kTabControlGap);
    tabBar.setBounds (tabRow);

    if (activeSessionIndex >= 0 && activeSessionIndex < (int) sessions.size())
        sessions[(size_t) activeSessionIndex]->setBounds (bounds);
}

void SequencerTabContainer::addTab()
{
    if ((int) sessions.size() >= maxTabs)
        return;

    if (activeSessionIndex >= 0 && activeSessionIndex < (int) sessions.size())
        sessions[(size_t) activeSessionIndex]->setVisible (false);

    const auto tabLabel = juce::String ("Tab ") + juce::String (nextTabId);
    const int tabIndex = (int) sessions.size();

    auto session = std::make_unique<SequencerSession> (nextTabId++, clockService, coordinator, clockInputService, outputPool);
    addAndMakeVisible (*session);

    sessions.push_back (std::move (session));
    tabBar.addTab (tabLabel, juce::Colours::darkgrey, tabIndex);

    activeSessionIndex = tabIndex;
    tabBar.setCurrentTabIndex (tabIndex);
    showSession (tabIndex);
    updateTabControls();
}

void SequencerTabContainer::removeCurrentTab()
{
    if ((int) sessions.size() <= 1)
        return;

    const int indexToRemove = activeSessionIndex;
    if (indexToRemove < 0 || indexToRemove >= (int) sessions.size())
        return;

    sessions[(size_t) indexToRemove]->prepareForRemoval();
    removeChildComponent (sessions[(size_t) indexToRemove].get());
    sessions.erase (sessions.begin() + indexToRemove);
    tabBar.removeTab (indexToRemove);

    activeSessionIndex = juce::jlimit (0, (int) sessions.size() - 1, indexToRemove);
    tabBar.setCurrentTabIndex (activeSessionIndex);
    showSession (activeSessionIndex);
    updateTabControls();
    resized();
}

void SequencerTabContainer::showSession (int index)
{
    if (index < 0 || index >= (int) sessions.size())
        return;

    if (activeSessionIndex >= 0 && activeSessionIndex < (int) sessions.size() && activeSessionIndex != index)
        sessions[(size_t) activeSessionIndex]->getPlayer().stopActiveNote();

    for (int i = 0; i < (int) sessions.size(); ++i)
        sessions[(size_t) i]->setVisible (i == index);

    activeSessionIndex = index;
    sessions[(size_t) index]->toFront (false);
    resized();
}

void SequencerTabContainer::updateTabControls()
{
    addTabButton.setEnabled ((int) sessions.size() < maxTabs);
    closeTabButton.setEnabled ((int) sessions.size() > 1);
}

void SequencerTabContainer::changeListenerCallback (juce::ChangeBroadcaster* source)
{
    if (source != &tabBar)
        return;

    const int tabIndex = tabBar.getCurrentTabIndex();
    if (tabIndex < 0 || tabIndex >= (int) sessions.size())
        return;

    showSession (tabIndex);
}
