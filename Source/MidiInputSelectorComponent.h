#pragma once

#include <JuceHeader.h>

#include <memory>

/** Owns MIDI input device selection UI + open/close logic. */
class MidiInputSelectorComponent : public juce::Component
{
public:
    explicit MidiInputSelectorComponent (juce::MidiInputCallback& midiCallbackTarget);
    ~MidiInputSelectorComponent() override;

    void resized() override;

private:
    void initialiseMidiInputs();
    void populateMidiInputDeviceList();
    void selectFirstMidiInputIfAvailable();
    void midiInputSelectionChanged();
    void selectMidiInputDevice (int deviceIndex);

    juce::MidiInputCallback& callbackTarget;
    std::unique_ptr<juce::MidiInput> midiInput;

    juce::ComboBox midiInputBox;
    juce::Label midiInputLabel { {}, "MIDI Input" };
    juce::Label statusLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiInputSelectorComponent)
};
