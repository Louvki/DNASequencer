#include "Sequencer/MidiClockInputService.h"

#include "ErrorReporting/ErrorLog.h"

MidiClockInputService::MidiClockInputService (juce::MidiInputCallback& callback)
    : callbackTarget (callback)
{
}

bool MidiClockInputService::selectDevice (int deviceIndex)
{
    closeDevice();

    const auto devices = juce::MidiInput::getAvailableDevices();

    if (deviceIndex < 0 || deviceIndex >= devices.size())
    {
        selectedDeviceName = {};
        selectedDeviceIdentifier = {};
        sendChangeMessage();
        return false;
    }

    const auto& device = devices.getReference (deviceIndex);
    midiInput = juce::MidiInput::openDevice (device.identifier, &callbackTarget);

    if (midiInput == nullptr)
    {
        const auto error = juce::String ("Could not open MIDI clock input.");
        ErrorLog::getInstance().addError ("MIDI", error);
        selectedDeviceName = {};
        selectedDeviceIdentifier = {};
        sendChangeMessage();
        return false;
    }

    midiInput->start();
    selectedDeviceName = device.name;
    selectedDeviceIdentifier = device.identifier;
    sendChangeMessage();
    return true;
}

bool MidiClockInputService::selectFirstAvailableDevice()
{
    const auto devices = juce::MidiInput::getAvailableDevices();

    if (devices.isEmpty())
    {
        closeDevice();
        selectedDeviceName = {};
        selectedDeviceIdentifier = {};
        sendChangeMessage();
        return false;
    }

    const int defaultDeviceIndex = devices.size() >= 2 ? 1 : 0;
    return selectDevice (defaultDeviceIndex);
}

bool MidiClockInputService::selectDeviceByIdentifier (const juce::String& deviceIdentifier)
{
    if (deviceIdentifier.isEmpty())
    {
        closeDevice();
        selectedDeviceName = {};
        selectedDeviceIdentifier = {};
        sendChangeMessage();
        return true;
    }

    const auto devices = juce::MidiInput::getAvailableDevices();

    for (int i = 0; i < devices.size(); ++i)
    {
        if (devices.getReference (i).identifier == deviceIdentifier)
            return selectDevice (i);
    }

    return false;
}

bool MidiClockInputService::isOutputDeviceExcluded (const juce::MidiDeviceInfo& outputDevice) const noexcept
{
    return selectedDeviceName.isNotEmpty() && outputDevice.name == selectedDeviceName;
}

void MidiClockInputService::closeDevice()
{
    if (midiInput != nullptr)
    {
        midiInput->stop();
        midiInput.reset();
    }
}
