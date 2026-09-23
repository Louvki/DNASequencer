#pragma once

#include <JuceHeader.h>

#include <map>
#include <memory>

/** Shared MIDI output handles with refcounting so multiple tabs can use the same bus. */
class MidiOutputBusPool
{
public:
    juce::MidiOutput* acquire (const juce::String& deviceIdentifier);
    void release (const juce::String& deviceIdentifier);

private:
    struct PooledOutput
    {
        std::unique_ptr<juce::MidiOutput> device;
        int refCount = 0;
    };

    mutable juce::CriticalSection lock_;
    std::map<juce::String, PooledOutput> outputs_;
};
