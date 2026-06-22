#pragma once

/** Observer interface for MIDI clock timing. */
class MidiClockTickListener
{
public:
    virtual ~MidiClockTickListener() = default;

    /** Called on every raw MIDI clock tick (24 per quarter note). */
    virtual void onMidiClockTick() = 0;

    /** Called on each configured musical division pulse (for note/codon timing). */
    virtual void onDivisionPulse() {}
};
