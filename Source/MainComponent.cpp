#include "MainComponent.h"

namespace
{
constexpr int kWindowWidth = 520;
constexpr int kWindowHeight = 692;
} // namespace

//==============================================================================
MainComponent::MainComponent()
    : midiClockInputService (*this),
      tabContainer (midiClockService, tabResourceCoordinator, midiClockInputService, midiOutputBusPool)
{
    AppMenuBar::getInstance().setClockInputService (&midiClockInputService);
    midiClockInputService.addChangeListener (this);
    midiClockInputService.selectFirstAvailableDevice();

    addAndMakeVisible (tabContainer);
    setSize (kWindowWidth, kWindowHeight);
    setAudioChannels (0, 0);
}

MainComponent::~MainComponent()
{
    tabContainer.forEachSession ([] (SequencerSession& session)
    {
        session.getPlayer().stopActiveNote();
    });

    midiClockInputService.removeChangeListener (this);
    shutdownAudio();
}

//==============================================================================
void MainComponent::handleIncomingMidiMessage (juce::MidiInput*, const juce::MidiMessage& message)
{
    handleTransportSideEffects (message);
    midiClockService.handleMidiMessage (message);
}

void MainComponent::handleTransportSideEffects (const juce::MidiMessage& message)
{
    const bool isMidiStop = message.getRawDataSize() >= 1 && message.getRawData()[0] == 0xfc;
    if (! isMidiStop)
        return;

    tabContainer.forEachSession ([] (SequencerSession& session)
    {
        session.getPlayer().stopActiveNote();
    });
}

void MainComponent::handleClockInputBusChanged()
{
    tabContainer.refreshOutputDeviceLists (midiClockInputService.getSelectedDeviceName());
}

void MainComponent::changeListenerCallback (juce::ChangeBroadcaster* source)
{
    if (source == &midiClockInputService)
        handleClockInputBusChanged();
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    juce::ignoreUnused (samplesPerBlockExpected, sampleRate);
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    bufferToFill.clearActiveBufferRegion();
}

void MainComponent::releaseResources() {}

//==============================================================================
void MainComponent::resized()
{
    tabContainer.setBounds (getLocalBounds());
}
