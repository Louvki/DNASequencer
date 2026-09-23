#include "Sequencer/MidiOutputBusPool.h"

#include "ErrorReporting/ErrorLog.h"

juce::MidiOutput* MidiOutputBusPool::acquire (const juce::String& deviceIdentifier)
{
    if (deviceIdentifier.isEmpty())
        return nullptr;

    const juce::ScopedLock sl (lock_);
    auto& entry = outputs_[deviceIdentifier];

    if (entry.device == nullptr)
    {
        entry.device = juce::MidiOutput::openDevice (deviceIdentifier);

        if (entry.device == nullptr)
        {
            outputs_.erase (deviceIdentifier);
            ErrorLog::getInstance().addError ("MIDI", "Could not open MIDI output.");
            return nullptr;
        }
    }

    ++entry.refCount;
    return entry.device.get();
}

void MidiOutputBusPool::release (const juce::String& deviceIdentifier)
{
    if (deviceIdentifier.isEmpty())
        return;

    const juce::ScopedLock sl (lock_);
    const auto it = outputs_.find (deviceIdentifier);

    if (it == outputs_.end())
        return;

    auto& entry = it->second;

    if (entry.refCount > 0)
        --entry.refCount;

    if (entry.refCount <= 0)
        outputs_.erase (it);
}
