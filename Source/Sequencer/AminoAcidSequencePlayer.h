#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <vector>

#include <JuceHeader.h>

#include "DataStructures/AminoAcids.h"
#include "Sequencer/MidiClockTickListener.h"
#include "DataStructures/MidiScales.h"

/** Reads DNA in codon mode after ATG start codons; emits scaled amino-acid MIDI notes. */
class AminoAcidSequencePlayer : public MidiClockTickListener,
                                private juce::Timer
{
public:
    using DnaSequenceProvider = std::function<juce::String()>;
    using StartCodonMapProvider = std::function<std::vector<std::int64_t>()>;
    using SequenceRevisionProvider = std::function<std::uint32_t()>;

    void setMidiOutput (juce::MidiOutput* output) noexcept { midiOutput = output; }
    void setDnaSequenceProvider (DnaSequenceProvider provider) { dnaSequenceProvider = std::move (provider); }
    void setStartCodonMapProvider (StartCodonMapProvider provider) { startCodonMapProvider = std::move (provider); }
    void setSequenceRevisionProvider (SequenceRevisionProvider provider) { sequenceRevisionProvider = std::move (provider); }

    void setRootNote (int note) noexcept;
    void setScale (dna::MidiScale scale) noexcept;
    void setNotePoolSize (int size) noexcept;
    void setWhiteSpaceReadSpeed (int speed) noexcept;
    void setNoteDurationMs (int durationMs) noexcept;
    void setSustainEnabled (bool enabled) noexcept;

    void rebuildCodonMap();
    void resetReadPosition();
    void stopActiveNote();

    int getCurrentReadIndex() const noexcept { return currentReadIndex.load (std::memory_order_acquire); }
    bool isReadingCodons() const noexcept { return isReadingCodonsFlag.load (std::memory_order_acquire); }

    void onMidiClockTick() override;
    void onDivisionPulse() override;

private:
    void performFullReset();
    void syncStartMapTrunc();
    void refreshSequenceCache();
    void checkSequenceReload();
    void advanceWhitespaceMode();
    void advanceCodonMode();
    void playNote (int note, int velocity);
    void scheduleNoteOff (int note);
    void sendNoteOff (int note);
    void timerCallback() override;
    static bool isIndexInStartMap (int index, const std::vector<std::int64_t>& startMap) noexcept;
    static juce::String readCodonAt (const juce::String& dna, int index);

    juce::MidiOutput* midiOutput = nullptr;
    DnaSequenceProvider dnaSequenceProvider;
    StartCodonMapProvider startCodonMapProvider;
    SequenceRevisionProvider sequenceRevisionProvider;

    dna::AminoAcids codonMap;

    int rootNote = 60;
    dna::MidiScale scale = dna::MidiScale::majorIonian;
    int notePoolSize = 20;
    int whiteSpaceReadSpeed = 15;
    int noteDurationMs = 100;
    bool sustainEnabled = false;
    int activeSustainNote = -1;

    struct ScheduledNoteOff
    {
        int note = -1;
        std::uint32_t offAtMs = 0;
    };

    std::vector<ScheduledNoteOff> scheduledNoteOffs;
    juce::CriticalSection noteStateLock;

    std::atomic<int> currentReadIndex { 0 };
    std::atomic<bool> isReadingCodonsFlag { false };
    std::vector<std::int64_t> startMapTrunc;

    juce::String cachedDna;
    std::vector<std::int64_t> cachedStartMap;

    std::uint32_t cachedSequenceRevision = 0;
};
