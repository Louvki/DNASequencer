#include "Sequencer/SequencerSession.h"

SequencerSession::SequencerSession (int sessionTabId,
                                  MidiClockService& service,
                                  TabResourceCoordinator& resourceCoordinator,
                                  MidiClockInputService& inputService,
                                  MidiOutputBusPool& outputPool)
    : tabId (sessionTabId),
      clockService (service),
      coordinator (resourceCoordinator),
      clockInputService (inputService),
      view (player,
            clockService,
            outputPool,
            [this] { handleResetClicked(); })
{
    wirePlayerProviders();
    wireResourceValidation();

    clockService.addListener (&player);

    view.getMidiOutputSelector().setExcludedBusName (clockInputService.getSelectedDeviceName());
    view.getMidiOutputSelector().selectNoOutput();

    player.setMidiOutput (view.getMidiOutputSelector().getMidiOutput());
    player.resetReadPosition();

    addAndMakeVisible (view);
}

SequencerSession::~SequencerSession()
{
    prepareForRemoval();
}

void SequencerSession::prepareForRemoval()
{
    if (preparedForRemoval)
        return;

    preparedForRemoval = true;

    clockService.removeListener (&player);
    player.stopActiveNote();
    view.getSequenceFileLoader().cancelLoad();
    view.getMidiOutputSelector().selectNoOutput();
    coordinator.releaseTab (tabId);
}

void SequencerSession::applyExcludedClockBus (const juce::String& excludedBusName)
{
    auto& outputSelector = view.getMidiOutputSelector();
    const auto previousName = outputSelector.getSelectedDeviceName();

    outputSelector.setExcludedBusName (excludedBusName);
    outputSelector.refreshDeviceList();

    if (previousName.isNotEmpty() && previousName == excludedBusName)
        outputSelector.selectNoOutput();

    player.setMidiOutput (outputSelector.getMidiOutput());
}

void SequencerSession::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void SequencerSession::resized()
{
    view.setBounds (getLocalBounds());
}

void SequencerSession::wirePlayerProviders()
{
    player.setDnaSequenceProvider ([this]
    {
        return view.getSequenceFileLoader().getLoadedDnaSequence();
    });

    player.setStartCodonMapProvider ([this]
    {
        return view.getSequenceFileLoader().getStartCodonMap();
    });

    player.setSequenceRevisionProvider ([this]
    {
        return view.getSequenceFileLoader().getSequenceRevision();
    });
}

void SequencerSession::wireResourceValidation()
{
    view.getSequenceFileLoader().setCanLoadFile ([this] (const juce::File& file)
    {
        return coordinator.canAssignFile (tabId, file.getFullPathName());
    });

    view.getSequenceFileLoader().setOnFileLoaded ([this] (const juce::String& filePath)
    {
        coordinator.registerFile (tabId, filePath);
    });

    view.getMidiOutputSelector().setCanSelectOutput ([this] (const juce::String& deviceId)
    {
        return isOutputAllowed (deviceId);
    });

    view.getMidiOutputSelector().setOnOutputChanged ([this] (const juce::String& deviceId)
    {
        handleOutputChanged (deviceId);
    });
}

bool SequencerSession::isOutputAllowed (const juce::String& deviceIdentifier) const
{
    const auto devices = juce::MidiOutput::getAvailableDevices();

    for (int i = 0; i < devices.size(); ++i)
    {
        if (devices.getReference (i).identifier == deviceIdentifier)
            return ! clockInputService.isOutputDeviceExcluded (devices.getReference (i));
    }

    return true;
}

void SequencerSession::handleResetClicked()
{
    player.setLocallyPaused (true);
    player.resetReadPosition();
    player.stopActiveNote();
}

void SequencerSession::handleOutputChanged (const juce::String& deviceIdentifier)
{
    juce::ignoreUnused (deviceIdentifier);
    player.setMidiOutput (view.getMidiOutputSelector().getMidiOutput());
}
