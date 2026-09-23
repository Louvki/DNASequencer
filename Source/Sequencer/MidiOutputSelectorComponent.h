#pragma once

#include <JuceHeader.h>

#include <functional>

class MidiOutputBusPool;

/** Owns MIDI output device selection UI; device handles come from MidiOutputBusPool. */
class MidiOutputSelectorComponent : public juce::Component
{
public:
    using CanSelectOutputFn = std::function<bool (const juce::String& deviceIdentifier)>;
    using OutputChangedFn = std::function<void (const juce::String& deviceIdentifier)>;

    explicit MidiOutputSelectorComponent (MidiOutputBusPool& outputPool);
    ~MidiOutputSelectorComponent() override;

    void setCanSelectOutput (CanSelectOutputFn predicate);
    void setOnOutputChanged (OutputChangedFn callback);
    void setExcludedBusName (const juce::String& busName);

    juce::String getSelectedDeviceIdentifier() const;
    juce::String getSelectedDeviceName() const;
    juce::MidiOutput* getMidiOutput() noexcept { return midiOutput; }

    void selectNoOutput();
    void refreshDeviceList();

    void resized() override;

private:
    static constexpr int kNoOutputItemId = 1;
    static constexpr int kFirstDeviceItemId = 2;

    void populateMidiOutputDeviceList();
    void outputSelectionChanged();
    bool trySelectNoOutput (bool revertOnFailure);
    bool trySelectDevice (int filteredDeviceIndex, bool revertOnFailure);
    bool isDeviceExcluded (const juce::MidiDeviceInfo& device) const;
    void releaseCurrentOutput();

    MidiOutputBusPool& outputPool;
    CanSelectOutputFn canSelectOutput;
    OutputChangedFn onOutputChanged;

    juce::MidiOutput* midiOutput = nullptr;
    juce::String selectedDeviceIdentifier;
    juce::String selectedDeviceName;
    juce::String excludedBusName;
    juce::Array<juce::MidiDeviceInfo> filteredDevices;

    juce::ComboBox midiOutputBox;
    juce::Label midiOutputLabel { {}, "MIDI Output" };
    juce::Label statusLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiOutputSelectorComponent)
};
