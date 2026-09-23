#include "Sequencer/MidiInputSelectorComponent.h"

#include "ErrorReporting/ErrorLog.h"
#include "Sequencer/MidiClockInputService.h"

MidiInputSelectorComponent::MidiInputSelectorComponent (MidiClockInputService& service)
    : clockInputService (service)
{
    midiInputLabel.attachToComponent (&midiInputBox, true);

    addAndMakeVisible (midiInputBox);
    addAndMakeVisible (statusLabel);

    midiInputBox.onChange = [this]
    {
        midiInputSelectionChanged();
    };

    initialiseMidiInputs();
}

MidiInputSelectorComponent::~MidiInputSelectorComponent() = default;

void MidiInputSelectorComponent::resized()
{
    auto r = getLocalBounds();
    midiInputBox.setBounds (r.removeFromTop (28).withWidth (juce::jmin (360, r.getWidth())));
    statusLabel.setBounds (r.withTrimmedTop (4).removeFromTop (28));
}

void MidiInputSelectorComponent::initialiseMidiInputs()
{
    populateMidiInputDeviceList();

    const auto selectedIdentifier = clockInputService.getSelectedDeviceIdentifier();

    if (selectedIdentifier.isNotEmpty())
    {
        const auto devices = juce::MidiInput::getAvailableDevices();

        for (int i = 0; i < devices.size(); ++i)
        {
            if (devices.getReference (i).identifier == selectedIdentifier)
            {
                midiInputBox.setSelectedId (i + 1, juce::dontSendNotification);
                statusLabel.setText ("Clock input: " + devices.getReference (i).name, juce::dontSendNotification);
                return;
            }
        }
    }

    selectFirstMidiInputIfAvailable();
}

void MidiInputSelectorComponent::populateMidiInputDeviceList()
{
    midiInputBox.clear (juce::dontSendNotification);

    const auto devices = juce::MidiInput::getAvailableDevices();
    for (int i = 0; i < devices.size(); ++i)
        midiInputBox.addItem (devices.getReference (i).name, i + 1);

    if (devices.isEmpty())
        midiInputBox.addItem ("(no MIDI inputs)", 1);

    repaint();
}

void MidiInputSelectorComponent::selectFirstMidiInputIfAvailable()
{
    const auto devices = juce::MidiInput::getAvailableDevices();

    if (devices.isEmpty())
        return;

    const int defaultDeviceIndex = devices.size() >= 2 ? 1 : 0;
    midiInputBox.setSelectedId (defaultDeviceIndex + 1, juce::dontSendNotification);
    selectMidiInputDevice (defaultDeviceIndex);
}

void MidiInputSelectorComponent::midiInputSelectionChanged()
{
    const int comboItemId = midiInputBox.getSelectedId();
    selectMidiInputDevice (comboItemId - 1);
}

void MidiInputSelectorComponent::selectMidiInputDevice (int deviceIndex)
{
    const auto devices = juce::MidiInput::getAvailableDevices();

    if (deviceIndex < 0 || deviceIndex >= devices.size())
    {
        const auto error = juce::String ("No MIDI input device.");
        statusLabel.setText (error, juce::dontSendNotification);
        ErrorLog::getInstance().addError ("MIDI", error);
        return;
    }

    if (! clockInputService.selectDevice (deviceIndex))
    {
        statusLabel.setText ("Could not open MIDI clock input.", juce::dontSendNotification);
        return;
    }

    const auto& device = devices.getReference (deviceIndex);
    statusLabel.setText ("Clock input: " + device.name, juce::dontSendNotification);
}
