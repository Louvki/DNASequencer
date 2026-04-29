#pragma once

#include <atomic>
#include <memory>

#include <JuceHeader.h>

//==============================================================================
class MainComponent  : public juce::AudioAppComponent,
                       public juce::MidiInputCallback
{
public:
    MainComponent();
    ~MainComponent() override;

    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    void handleIncomingMidiMessage (juce::MidiInput* source, const juce::MidiMessage& message) override;

private:
    void refreshMidiInputList();
    void openMidiInput (int deviceIndex);

    static bool isMidiStart (const juce::MidiMessage& m);
    static bool isMidiContinue (const juce::MidiMessage& m);
    static bool isMidiStop (const juce::MidiMessage& m);

    std::unique_ptr<juce::MidiInput> midiInput;

    juce::ComboBox midiInputBox;
    juce::Label midiInputLabel  { {}, "MIDI Input" };
    juce::Label statusLabel;

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
