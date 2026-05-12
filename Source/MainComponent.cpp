#include "MainComponent.h"

//==============================================================================
/** Builds the UI, fills the MIDI device list and connects the default port, sizes the window,
    and opens the audio device with zero I/O channels (required by `AudioAppComponent`). */
MainComponent::MainComponent()
    : view (*this)
{
    // Initializes the GUI
    addAndMakeVisible (view);
    setSize (520, 300);

    setAudioChannels (0, 0);
}

/** Destructor: Stops audio I/O and closes any open MIDI input device. */
MainComponent::~MainComponent()
{
    shutdownAudio();
}

//==============================================================================
/** MIDI callback: counts clock ticks into quarter notes (24 PPQN) while transport runs;
    toggles `transportRunning` on Start (0xFA), Continue (0xFB), and Stop (0xFC). */
void MainComponent::handleIncomingMidiMessage (juce::MidiInput*, const juce::MidiMessage& message)
{
    if (message.isMidiClock())
    {
        if (!transportRunning.load (std::memory_order_acquire))
            return;

        if (++midiClockTickInBeat >= 24)
            midiClockTickInBeat = 0;

        return;
    }

    
    const bool isMidiStart = message.getRawDataSize() >= 1 && message.getRawData()[0] == 0xfa;
    if (isMidiStart)
    {
        midiClockTickInBeat = 0;
        transportRunning.store (true, std::memory_order_release);
        return;
    }
    
    const bool isMidiContinue = message.getRawDataSize() >= 1 && message.getRawData()[0] == 0xfb;
    if (isMidiContinue)
    {
        transportRunning.store (true, std::memory_order_release);
        return;
    }
    
    const bool isMidiStop = message.getRawDataSize() >= 1 && message.getRawData()[0] == 0xfc;
    if (isMidiStop)
    {
        transportRunning.store (false, std::memory_order_release);
        return;
    }
}

//==============================================================================
/** `AudioAppComponent` hook before I/O runs; no audio output in this app — parameters ignored. */
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    juce::ignoreUnused (samplesPerBlockExpected, sampleRate);
}

/** Clears each output block; no samples are synthesized (MIDI clock handling is on the MIDI thread). */
void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    bufferToFill.clearActiveBufferRegion();
}

/** No teardown needed beyond base class handling. */
void MainComponent::releaseResources() {}

//==============================================================================
/** Gives the entire bounds to `MainView`. */
void MainComponent::resized()
{
    view.setBounds (getLocalBounds());
}
