#include "Sequencer/MidiClockService.h"
#include "ErrorReporting/ErrorLog.h"

void MidiClockService::setDivision (MidiClockDivision division) noexcept
{
    if (division == currentDivision)
        return;

    currentDivision = division;
    ticksPerPulse.store (getMidiClockTicksPerPulse (division), std::memory_order_release);
    resetPulseAccumulator();
}

void MidiClockService::addListener (MidiClockTickListener* listener)
{
    listeners.add (listener);
}

void MidiClockService::removeListener (MidiClockTickListener* listener)
{
    listeners.remove (listener);
}

void MidiClockService::setLocalPaused (bool paused) noexcept
{
    localPaused.store (paused, std::memory_order_release);

    if (paused)
        transportRunning.store (false, std::memory_order_release);
}

bool MidiClockService::hasRecentClockActivity (int withinMs) const noexcept
{
    const auto lastMs = lastClockMessageMs.load (std::memory_order_acquire);
    if (lastMs == 0)
        return false;

    const auto elapsed = juce::Time::getMillisecondCounter() - lastMs;
    return elapsed <= static_cast<std::uint32_t> (withinMs);
}

void MidiClockService::handleMidiMessage (const juce::MidiMessage& message)
{
    if (message.isMidiClock())
    {
        if (localPaused.load (std::memory_order_acquire))
            return;

        lastClockMessageMs.store (juce::Time::getMillisecondCounter(), std::memory_order_release);
        transportRunning.store (true, std::memory_order_release);
        advanceClockAndMaybeEmit();
        return;
    }

    const bool isMidiStop = message.getRawDataSize() >= 1 && message.getRawData()[0] == 0xfc;
    if (isMidiStop)
        transportRunning.store (false, std::memory_order_release);
}

void MidiClockService::resetPulseAccumulator() noexcept
{
    pulseTickAccumulator = 0.0;
}

void MidiClockService::advanceClockAndMaybeEmit() noexcept
{
    emitClockTick();

    const auto ticksPerPulseValue = ticksPerPulse.load (std::memory_order_acquire);

    pulseTickAccumulator += 1.0;

    if (pulseTickAccumulator >= ticksPerPulseValue)
    {
        pulseTickAccumulator -= ticksPerPulseValue;
        emitDivisionPulse();
    }
}

void MidiClockService::emitClockTick() noexcept
{
    listeners.call ([] (MidiClockTickListener& l) { l.onMidiClockTick(); });
}

void MidiClockService::emitDivisionPulse() noexcept
{
    listeners.call ([] (MidiClockTickListener& l) { l.onDivisionPulse(); });
}
