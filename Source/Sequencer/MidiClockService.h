#pragma once

#include <atomic>
#include <cstdint>

#include <JuceHeader.h>

#include "Sequencer/MidiClockTickListener.h"

/** Clock-driven transport: clock ticks mean running; Stop means stopped. */
class MidiClockService
{
public:
    void addListener (MidiClockTickListener* listener);
    void removeListener (MidiClockTickListener* listener);

    /** Process incoming MIDI (clock + transport); called on the MIDI input thread. */
    void handleMidiMessage (const juce::MidiMessage& message);

    bool isTransportRunning() const noexcept { return transportRunning.load (std::memory_order_acquire); }

    /** True if a MIDI clock message arrived within the last `withinMs` milliseconds. */
    bool hasRecentClockActivity (int withinMs = 250) const noexcept;

private:
    void emitClockTick() noexcept;

    juce::ListenerList<MidiClockTickListener> listeners;

    std::atomic<bool> transportRunning { false };
    std::atomic<std::uint32_t> lastClockMessageMs { 0 };
};
