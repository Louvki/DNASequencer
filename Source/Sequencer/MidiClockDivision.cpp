#include "Sequencer/MidiClockDivision.h"

namespace
{
constexpr double kTicksPerQuarterNote = 24.0;
} // namespace

juce::String getMidiClockDivisionLabel (MidiClockDivision division)
{
    switch (division)
    {
        case MidiClockDivision::wholeNote:                 return "1";
        case MidiClockDivision::dottedHalfNote:            return "1/2.";
        case MidiClockDivision::halfNote:                  return "1/2";
        case MidiClockDivision::halfNoteTriplet:           return "1/2t";
        case MidiClockDivision::dottedQuarterNote:         return "1/4.";
        case MidiClockDivision::quarterNote:               return "1/4";
        case MidiClockDivision::quarterNoteTriplet:        return "1/4t";
        case MidiClockDivision::dottedEighthNote:          return "1/8.";
        case MidiClockDivision::eighthNote:                return "1/8";
        case MidiClockDivision::eighthNoteTriplet:         return "1/8t";
        case MidiClockDivision::dottedSixteenthNote:       return "1/16.";
        case MidiClockDivision::sixteenthNote:             return "1/16";
        case MidiClockDivision::sixteenthNoteTriplet:      return "1/16t";
        case MidiClockDivision::dottedThirtySecondNote:    return "1/32.";
        case MidiClockDivision::thirtySecondNote:          return "1/32";
        case MidiClockDivision::thirtySecondNoteTriplet:   return "1/32t";
        case MidiClockDivision::sixtyFourthNote:           return "1/64";
        case MidiClockDivision::sixtyFourthNoteTriplet:    return "1/64t";
        default:                                           return "1";
    }
}

double getMidiClockTicksPerPulse (MidiClockDivision division)
{
    switch (division)
    {
        case MidiClockDivision::wholeNote:                 return kTicksPerQuarterNote * 4.0;
        case MidiClockDivision::dottedHalfNote:            return kTicksPerQuarterNote * 3.0;
        case MidiClockDivision::halfNote:                  return kTicksPerQuarterNote * 2.0;
        case MidiClockDivision::halfNoteTriplet:           return kTicksPerQuarterNote * 4.0 / 3.0;
        case MidiClockDivision::dottedQuarterNote:         return kTicksPerQuarterNote * 1.5;
        case MidiClockDivision::quarterNote:               return kTicksPerQuarterNote;
        case MidiClockDivision::quarterNoteTriplet:        return kTicksPerQuarterNote * 2.0 / 3.0;
        case MidiClockDivision::dottedEighthNote:          return kTicksPerQuarterNote * 0.75;
        case MidiClockDivision::eighthNote:                return kTicksPerQuarterNote * 0.5;
        case MidiClockDivision::eighthNoteTriplet:         return kTicksPerQuarterNote / 3.0;
        case MidiClockDivision::dottedSixteenthNote:       return kTicksPerQuarterNote * 0.375;
        case MidiClockDivision::sixteenthNote:             return kTicksPerQuarterNote * 0.25;
        case MidiClockDivision::sixteenthNoteTriplet:      return kTicksPerQuarterNote / 6.0;
        case MidiClockDivision::dottedThirtySecondNote:    return kTicksPerQuarterNote * 0.1875;
        case MidiClockDivision::thirtySecondNote:          return kTicksPerQuarterNote * 0.125;
        case MidiClockDivision::thirtySecondNoteTriplet:   return kTicksPerQuarterNote / 12.0;
        case MidiClockDivision::sixtyFourthNote:           return kTicksPerQuarterNote * 0.0625;
        case MidiClockDivision::sixtyFourthNoteTriplet:    return kTicksPerQuarterNote / 24.0;
        default:                                           return kTicksPerQuarterNote;
    }
}

juce::Array<MidiClockDivision> getAllMidiClockDivisions()
{
    return {
        MidiClockDivision::wholeNote,
        MidiClockDivision::dottedHalfNote,
        MidiClockDivision::halfNote,
        MidiClockDivision::halfNoteTriplet,
        MidiClockDivision::dottedQuarterNote,
        MidiClockDivision::quarterNote,
        MidiClockDivision::quarterNoteTriplet,
        MidiClockDivision::dottedEighthNote,
        MidiClockDivision::eighthNote,
        MidiClockDivision::eighthNoteTriplet,
        MidiClockDivision::dottedSixteenthNote,
        MidiClockDivision::sixteenthNote,
        MidiClockDivision::sixteenthNoteTriplet,
        MidiClockDivision::dottedThirtySecondNote,
        MidiClockDivision::thirtySecondNote,
        MidiClockDivision::thirtySecondNoteTriplet,
        MidiClockDivision::sixtyFourthNote,
        MidiClockDivision::sixtyFourthNoteTriplet
    };
}
