#pragma once

#include <atomic>
#include <cstdint>

#include <JuceHeader.h>

#include "Sequencer/MidiClockDivision.h"
#include "Sequencer/MidiClockTickListener.h"

/** Clock-driven transport: clock ticks mean running; Stop means stopped. */
class MidiClockService
{
public:
    void setDivision (MidiClockDivision division) noexcept;
    MidiClockDivision getDivision() const noexcept { return currentDivision; }
    double getTicksPerPulse() const noexcept { return ticksPerPulse.load (std::memory_order_acquire); }

    void addListener (MidiClockTickListener* listener);
    void removeListener (MidiClockTickListener* listener);

    /** Process incoming MIDI (clock + transport); called on the MIDI input thread. */
    void handleMidiMessage (const juce::MidiMessage& message);

    bool isTransportRunning() const noexcept { return transportRunning.load (std::memory_order_acquire); }

    void setLocalPaused (bool paused) noexcept;
    bool isLocalPaused() const noexcept { return localPaused.load (std::memory_order_acquire); }

    /** True if a MIDI clock message arrived within the last `withinMs` milliseconds. */
    bool hasRecentClockActivity (int withinMs = 250) const noexcept;

private:
    void resetPulseAccumulator() noexcept;
    void advanceClockAndMaybeEmit() noexcept;
    void emitClockTick() noexcept;
    void emitDivisionPulse() noexcept;

    juce::ListenerList<MidiClockTickListener> listeners;

    MidiClockDivision currentDivision = MidiClockDivision::quarterNote;
    std::atomic<double> ticksPerPulse { getMidiClockTicksPerPulse (MidiClockDivision::quarterNote) };
    double pulseTickAccumulator = 0.0;

    std::atomic<bool> transportRunning { false };
    std::atomic<bool> localPaused { false };
    std::atomic<std::uint32_t> lastClockMessageMs { 0 };
};
