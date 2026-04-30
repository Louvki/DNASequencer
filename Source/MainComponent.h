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
    /** Hosts `MainView`, builds the MIDI device list, opens default input, configures audio outputs. */
    MainComponent();
    /** Stops audio I/O and releases the MIDI input. */
    ~MainComponent() override;

    /** MainView calls this when the MIDI input combo selection changes. */
    void midiInputSelectionChangedFromView();

    /** Called before playback starts; caches sample rate for click synthesis. */
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    /** Generates audio each block (silence or beat click mix-in). */
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

    /** Enables stereo output; on platforms that require it, asks for record-audio permission first. */
    void setupAudioOutputs();

    /** Recognises MIDI realtime Start / Continue / Stop status bytes. */
    static bool isMidiStart (const juce::MidiMessage& m);
    static bool isMidiContinue (const juce::MidiMessage& m);
    static bool isMidiStop (const juce::MidiMessage& m);

    std::unique_ptr<juce::MidiInput> midiInput;

    MainView view;

    double currentSampleRate = 44100.0;

    /** Only touched from MIDI input thread. */
    int midiClockTickInBeat = 0;

    std::atomic<bool> transportRunning { false };
    std::atomic<bool> beepPending { false };

    int totalBeepSamples = 0;
    int beepSamplesRemaining = 0;
    float beepPhase = 0.0f;
    static constexpr float beepFrequencyHz = 880.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
