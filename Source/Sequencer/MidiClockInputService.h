#pragma once

#include <JuceHeader.h>

#include <memory>

/** Owns the open MIDI clock input device; broadcasts when the selected bus changes. */
class MidiClockInputService : public juce::ChangeBroadcaster
{
public:
    explicit MidiClockInputService (juce::MidiInputCallback& callbackTarget);

    bool selectDevice (int deviceIndex);
    bool selectDeviceByIdentifier (const juce::String& deviceIdentifier);
    bool selectFirstAvailableDevice();

    juce::String getSelectedDeviceName() const noexcept { return selectedDeviceName; }
    juce::String getSelectedDeviceIdentifier() const noexcept { return selectedDeviceIdentifier; }

    bool isOutputDeviceExcluded (const juce::MidiDeviceInfo& outputDevice) const noexcept;

private:
    void closeDevice();

    juce::MidiInputCallback& callbackTarget;
    std::unique_ptr<juce::MidiInput> midiInput;

    juce::String selectedDeviceName;
    juce::String selectedDeviceIdentifier;
};
