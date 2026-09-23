#include "Sequencer/MidiOutputSelectorComponent.h"

#include "ErrorReporting/ErrorLog.h"
#include "Sequencer/MidiOutputBusPool.h"

MidiOutputSelectorComponent::MidiOutputSelectorComponent (MidiOutputBusPool& pool)
    : outputPool (pool)
{
    midiOutputLabel.attachToComponent (&midiOutputBox, true);

    addAndMakeVisible (midiOutputBox);
    addAndMakeVisible (statusLabel);

    midiOutputBox.onChange = [this]
    {
        outputSelectionChanged();
    };

    populateMidiOutputDeviceList();
}

MidiOutputSelectorComponent::~MidiOutputSelectorComponent()
{
    releaseCurrentOutput();
}

void MidiOutputSelectorComponent::setCanSelectOutput (CanSelectOutputFn predicate)
{
    canSelectOutput = std::move (predicate);
}

void MidiOutputSelectorComponent::setOnOutputChanged (OutputChangedFn callback)
{
    onOutputChanged = std::move (callback);
}

void MidiOutputSelectorComponent::setExcludedBusName (const juce::String& busName)
{
    excludedBusName = busName;
}

juce::String MidiOutputSelectorComponent::getSelectedDeviceIdentifier() const
{
    return selectedDeviceIdentifier;
}

juce::String MidiOutputSelectorComponent::getSelectedDeviceName() const
{
    return selectedDeviceName;
}

void MidiOutputSelectorComponent::selectNoOutput()
{
    populateMidiOutputDeviceList();
    midiOutputBox.setSelectedId (kNoOutputItemId, juce::dontSendNotification);
    trySelectNoOutput (false);
}

void MidiOutputSelectorComponent::refreshDeviceList()
{
    const auto previousIdentifier = selectedDeviceIdentifier;
    populateMidiOutputDeviceList();

    if (previousIdentifier.isEmpty())
    {
        midiOutputBox.setSelectedId (kNoOutputItemId, juce::dontSendNotification);
        return;
    }

    for (int i = 0; i < filteredDevices.size(); ++i)
    {
        if (filteredDevices.getReference (i).identifier == previousIdentifier)
        {
            midiOutputBox.setSelectedId (kFirstDeviceItemId + i, juce::dontSendNotification);
            return;
        }
    }

    selectNoOutput();
    statusLabel.setText ("Output unavailable (reserved for clock input).", juce::dontSendNotification);
}

void MidiOutputSelectorComponent::releaseCurrentOutput()
{
    if (selectedDeviceIdentifier.isNotEmpty())
        outputPool.release (selectedDeviceIdentifier);

    midiOutput = nullptr;
    selectedDeviceIdentifier = {};
    selectedDeviceName = {};
}

void MidiOutputSelectorComponent::resized()
{
    auto r = getLocalBounds();
    midiOutputBox.setBounds (r.removeFromTop (28).withWidth (juce::jmin (360, r.getWidth())));
    statusLabel.setBounds (r.withTrimmedTop (4).removeFromTop (28));
}

bool MidiOutputSelectorComponent::isDeviceExcluded (const juce::MidiDeviceInfo& device) const
{
    return excludedBusName.isNotEmpty() && device.name == excludedBusName;
}

void MidiOutputSelectorComponent::populateMidiOutputDeviceList()
{
    midiOutputBox.clear (juce::dontSendNotification);
    filteredDevices.clear();

    midiOutputBox.addItem ("No Output", kNoOutputItemId);

    const auto devices = juce::MidiOutput::getAvailableDevices();

    for (int i = 0; i < devices.size(); ++i)
    {
        const auto& device = devices.getReference (i);

        if (isDeviceExcluded (device))
            continue;

        filteredDevices.add (device);
        midiOutputBox.addItem (device.name, kFirstDeviceItemId + filteredDevices.size() - 1);
    }

    if (filteredDevices.isEmpty())
        midiOutputBox.addItem ("(no MIDI outputs)", kFirstDeviceItemId);

    repaint();
}

void MidiOutputSelectorComponent::outputSelectionChanged()
{
    const int comboItemId = midiOutputBox.getSelectedId();

    if (comboItemId == kNoOutputItemId)
    {
        trySelectNoOutput (true);
        return;
    }

    trySelectDevice (comboItemId - kFirstDeviceItemId, true);
}

bool MidiOutputSelectorComponent::trySelectNoOutput (bool revertOnFailure)
{
    const auto previousIdentifier = selectedDeviceIdentifier;
    const int previousSelectedId = midiOutputBox.getSelectedId();

    releaseCurrentOutput();
    statusLabel.setText ("No output selected.", juce::dontSendNotification);

    if (onOutputChanged != nullptr)
        onOutputChanged ({});

    juce::ignoreUnused (revertOnFailure, previousIdentifier, previousSelectedId);
    return true;
}

bool MidiOutputSelectorComponent::trySelectDevice (int filteredDeviceIndex, bool revertOnFailure)
{
    const auto previousIdentifier = selectedDeviceIdentifier;
    const int previousSelectedId = midiOutputBox.getSelectedId();

    releaseCurrentOutput();

    if (filteredDeviceIndex < 0 || filteredDeviceIndex >= filteredDevices.size())
    {
        const auto error = juce::String ("No MIDI output device.");
        statusLabel.setText (error, juce::dontSendNotification);
        ErrorLog::getInstance().addError ("MIDI", error);
        return false;
    }

    const auto& device = filteredDevices.getReference (filteredDeviceIndex);

    if (canSelectOutput != nullptr && ! canSelectOutput (device.identifier))
    {
        const auto error = juce::String ("This MIDI output is reserved for clock input.");
        statusLabel.setText (error, juce::dontSendNotification);
        ErrorLog::getInstance().addError ("MIDI", error);

        if (revertOnFailure)
        {
            populateMidiOutputDeviceList();

            if (previousIdentifier.isNotEmpty())
            {
                for (int i = 0; i < filteredDevices.size(); ++i)
                {
                    if (filteredDevices.getReference (i).identifier == previousIdentifier)
                    {
                        midiOutputBox.setSelectedId (kFirstDeviceItemId + i, juce::dontSendNotification);
                        trySelectDevice (i, false);
                        break;
                    }
                }
            }
            else
            {
                midiOutputBox.setSelectedId (kNoOutputItemId, juce::dontSendNotification);
                trySelectNoOutput (false);
            }
        }

        return false;
    }

    midiOutput = outputPool.acquire (device.identifier);
    if (midiOutput == nullptr)
    {
        statusLabel.setText ("Could not open MIDI output.", juce::dontSendNotification);
        return false;
    }

    selectedDeviceIdentifier = device.identifier;
    selectedDeviceName = device.name;
    statusLabel.setText ("Output: " + device.name, juce::dontSendNotification);

    if (onOutputChanged != nullptr)
        onOutputChanged (selectedDeviceIdentifier);

    return true;
}
