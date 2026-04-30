#pragma once

#include <atomic>
#include <memory>

#include <JuceHeader.h>

#include "MainView.h"

//==============================================================================
class MainComponent  : public juce::AudioAppComponent,
                       public juce::MidiInputCallback
{
public:
    /** Builds the UI, fills the MIDI device list, connects the default port, sizes the window,
        and opens the audio device with zero I/O channels (required by `AudioAppComponent`). */
    MainComponent();
    /** Stops audio I/O and closes any open MIDI input device. */
    ~MainComponent() override;

    /** MainView calls this when the MIDI input combo selection changes. */
    void midiInputSelectionChangedFromView();

    /** `AudioAppComponent` hook before I/O runs; no audio synthesis — parameters unused. */
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    /** Clears each output block (zero audio channels; satisfies the base class contract). */
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    /** Release hook for audio resources (unused here). */
    void releaseResources() override;

    /** Sizes the embedded `MainView` to fill this component. */
    void resized() override;

    /** Receives MIDI clock and transport commands from the open input device. */
    void handleIncomingMidiMessage (juce::MidiInput* source, const juce::MidiMessage& message) override;

private:
    /** Refills the MIDI device combo from the OS device list. */
    void populateMidiInputDeviceList();
    /** Opens or switches to the device at `deviceIndex`; updates status label. */
    void selectMidiInputDevice (int deviceIndex);
    /** Populates the device list and connects the first port when present. */
    void initialiseMidiInputs();
    /** Selects list index 0 in the combo and opens it (internal startup path). */
    void selectFirstMidiInputIfAvailable();

    std::unique_ptr<juce::MidiInput> midiInput;

    MainView view;

    /** MIDI clock position within the current quarter note (0–23); only touched on MIDI input thread. */
    int midiClockTickInBeat = 0;

    /** Set true after MIDI Start / Continue, false after MIDI Stop (atomic for cross-thread visibility). */
    std::atomic<bool> transportRunning { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
