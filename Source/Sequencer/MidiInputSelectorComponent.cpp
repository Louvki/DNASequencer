#include "Sequencer/MidiInputSelectorComponent.h"
#include "ErrorReporting/ErrorLog.h"

MidiInputSelectorComponent::MidiInputSelectorComponent (juce::MidiInputCallback& midiCallbackTarget)
    : callbackTarget (midiCallbackTarget)
{
    midiInputLabel.attachToComponent (&midiInputBox, true);

    addAndMakeVisible (midiInputBox);

    midiInputBox.onChange = [this]
    {
        midiInputSelectionChanged();
    };

    initialiseMidiInputs();
}

MidiInputSelectorComponent::~MidiInputSelectorComponent()
{
    if (midiInput != nullptr)
        midiInput->stop();
}

void MidiInputSelectorComponent::resized()
{
    auto r = getLocalBounds();
    midiInputBox.setBounds (r.removeFromTop (28).withWidth (juce::jmin (360, r.getWidth())));
    statusLabel.setBounds (r.withTrimmedTop (4).removeFromTop (28));
}

void MidiInputSelectorComponent::initialiseMidiInputs()
{
    populateMidiInputDeviceList();
    selectFirstMidiInputIfAvailable();
}

void MidiInputSelectorComponent::populateMidiInputDeviceList()
{
    midiInputBox.clear (juce::dontSendNotification);

    auto devices = juce::MidiInput::getAvailableDevices();
    for (int i = 0; i < devices.size(); ++i)
        midiInputBox.addItem (devices[(size_t) i].name, i + 1);

    if (devices.isEmpty())
        midiInputBox.addItem ("(no MIDI inputs)", 1);

    repaint();
}

void MidiInputSelectorComponent::selectFirstMidiInputIfAvailable()
{
    if (juce::MidiInput::getAvailableDevices().isEmpty())
        return;

    midiInputBox.setSelectedId (1, juce::dontSendNotification);
    selectMidiInputDevice (0);
}

void MidiInputSelectorComponent::midiInputSelectionChanged()
{
    const int comboItemId = midiInputBox.getSelectedId();
    selectMidiInputDevice (comboItemId - 1);
}

void MidiInputSelectorComponent::selectMidiInputDevice (int deviceIndex)
{
    if (midiInput != nullptr)
    {
        midiInput->stop();
        midiInput.reset();
    }

    auto devices = juce::MidiInput::getAvailableDevices();

    if (deviceIndex < 0 || deviceIndex >= devices.size())
    {
        const auto error = juce::String ("No MIDI input device.");
        statusLabel.setText (error, juce::dontSendNotification);
        ErrorLog::getInstance().addError ("MIDI", error);
        return;
    }

    midiInput = juce::MidiInput::openDevice (devices[(size_t) deviceIndex].identifier, &callbackTarget);
    if (midiInput == nullptr)
    {
        const auto error = juce::String ("Could not open MIDI input.");
        statusLabel.setText (error, juce::dontSendNotification);
        ErrorLog::getInstance().addError ("MIDI", error);
        return;
    }

    midiInput->start();
}
