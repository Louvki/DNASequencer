#pragma once

#include <JuceHeader.h>

/** Musical grid divisions relative to MIDI clock (24 ticks per quarter note). */
enum class MidiClockDivision
{
    wholeNote          = 1,
    dottedHalfNote,
    halfNote,
    halfNoteTriplet,
    dottedQuarterNote,
    quarterNote,
    quarterNoteTriplet,
    dottedEighthNote,
    eighthNote,
    eighthNoteTriplet,
    dottedSixteenthNote,
    sixteenthNote,
    sixteenthNoteTriplet,
    dottedThirtySecondNote,
    thirtySecondNote,
    thirtySecondNoteTriplet,
    sixtyFourthNote,
    sixtyFourthNoteTriplet
};

/** Returns display label (e.g. "1/8t") for the combo box. */
juce::String getMidiClockDivisionLabel (MidiClockDivision division);

/** MIDI clock ticks between pulses; may be fractional (e.g. 1.5 for 1/64). */
double getMidiClockTicksPerPulse (MidiClockDivision division);

/** All divisions in menu order. */
juce::Array<MidiClockDivision> getAllMidiClockDivisions();
