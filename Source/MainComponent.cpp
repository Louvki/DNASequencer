#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
    : view (*this,
            midiClockService,
            aminoAcidSequencePlayer,
            [this]
            {
                midiClockService.setLocalPaused (true);
                aminoAcidSequencePlayer.resetReadPosition();
                aminoAcidSequencePlayer.stopActiveNote();
            })
{
    addAndMakeVisible (view);
    setSize (520, 480);

    aminoAcidSequencePlayer.setDnaSequenceProvider ([this]
    {
        return view.getSequenceFileLoader().getLoadedDnaSequence();
    });

    aminoAcidSequencePlayer.setStartCodonMapProvider ([this]
    {
        return view.getSequenceFileLoader().getStartCodonMap();
    });

    aminoAcidSequencePlayer.setSequenceRevisionProvider ([this]
    {
        return view.getSequenceFileLoader().getSequenceRevision();
    });

    midiClockService.addListener (&aminoAcidSequencePlayer);

    openDefaultMidiOutput();
    aminoAcidSequencePlayer.setMidiOutput (midiOutput.get());
    aminoAcidSequencePlayer.resetReadPosition();
    setAudioChannels (0, 0);
}

MainComponent::~MainComponent()
{
    midiClockService.removeListener (&aminoAcidSequencePlayer);
    aminoAcidSequencePlayer.stopActiveNote();
    closeMidiOutput();
    shutdownAudio();
}

//==============================================================================
/* Opens the first MIDI output, registers AminoAcidSequencePlayer as a clock listener, 
and forwards incoming MIDI: */
void MainComponent::handleIncomingMidiMessage (juce::MidiInput*, const juce::MidiMessage& message)
{
    handleTransportSideEffects (message);
    midiClockService.handleMidiMessage (message);
}

void MainComponent::handleTransportSideEffects (const juce::MidiMessage& message)
{
    const bool isMidiStop = message.getRawDataSize() >= 1 && message.getRawData()[0] == 0xfc;
    if (isMidiStop)
        aminoAcidSequencePlayer.stopActiveNote();
}

void MainComponent::openDefaultMidiOutput()
{
    const auto outputs = juce::MidiOutput::getAvailableDevices();

    if (outputs.isEmpty())
        return;

    midiOutput = juce::MidiOutput::openDevice (outputs[0].identifier);
}

void MainComponent::closeMidiOutput()
{
    midiOutput.reset();
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
    view.setBounds (getLocalBounds());
}
