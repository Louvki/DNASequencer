#pragma once

#include <JuceHeader.h>

class MidiClockInputService;

/** Clock input bus dropdown UI; device open/close is owned by MidiClockInputService. */
class MidiInputSelectorComponent : public juce::Component
{
public:
    explicit MidiInputSelectorComponent (MidiClockInputService& clockInputService);
    ~MidiInputSelectorComponent() override;

    void resized() override;

private:
    void initialiseMidiInputs();
    void populateMidiInputDeviceList();
    void selectFirstMidiInputIfAvailable();
    void midiInputSelectionChanged();
    void selectMidiInputDevice (int deviceIndex);

    MidiClockInputService& clockInputService;

    juce::ComboBox midiInputBox;
    juce::Label midiInputLabel { {}, "Clock Input Bus" };
    juce::Label statusLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiInputSelectorComponent)
};
