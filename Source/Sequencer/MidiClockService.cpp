#include "Sequencer/MidiClockService.h"
#include "ErrorReporting/ErrorLog.h"

void MidiClockService::addListener (MidiClockTickListener* listener)
{
    listeners.add (listener);
}

void MidiClockService::removeListener (MidiClockTickListener* listener)
{
    listeners.remove (listener);
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
        lastClockMessageMs.store (juce::Time::getMillisecondCounter(), std::memory_order_release);
        transportRunning.store (true, std::memory_order_release);
        emitClockTick();
        return;
    }

    const bool isMidiStop = message.getRawDataSize() >= 1 && message.getRawData()[0] == 0xfc;
    if (isMidiStop)
        transportRunning.store (false, std::memory_order_release);
}

void MidiClockService::emitClockTick() noexcept
{
    listeners.call ([] (MidiClockTickListener& l) { l.onMidiClockTick(); });
}
