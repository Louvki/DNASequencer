#include "Sequencer/AminoAcidSequencePlayer.h"

#include <algorithm>
#include <optional>

#include "ErrorReporting/ErrorLog.h"

void AminoAcidSequencePlayer::setRootNote (int note) noexcept
{
    rootNote = juce::jlimit (0, 127, note);
    rebuildCodonMap();
}

void AminoAcidSequencePlayer::setScale (dna::MidiScale newScale) noexcept
{
    scale = newScale;
    rebuildCodonMap();
}

void AminoAcidSequencePlayer::setNotePoolSize (int size) noexcept
{
    notePoolSize = juce::jlimit (1, 20, size);
    rebuildCodonMap();
}

void AminoAcidSequencePlayer::setWhiteSpaceReadSpeed (int speed) noexcept
{
    whiteSpaceReadSpeed = juce::jmax (1, speed);
}

void AminoAcidSequencePlayer::setNoteDurationMs (int durationMs) noexcept
{
    noteDurationMs = juce::jlimit (1, 1000, durationMs);
}

void AminoAcidSequencePlayer::setSustainEnabled (bool enabled) noexcept
{
    sustainEnabled = enabled;
}

void AminoAcidSequencePlayer::rebuildCodonMap()
{
    const auto scaled = dna::applyScaleToAminoAcids (rootNote, scale, notePoolSize);
    codonMap.rebuildFromScaledAminoAcids (scaled);
}

void AminoAcidSequencePlayer::resetReadPosition()
{
    currentReadIndex.store (0, std::memory_order_release);
    isReadingCodonsFlag.store (false, std::memory_order_release);
    syncStartMapTrunc();

    const juce::ScopedLock sl (noteStateLock);
    activeSustainNote = -1;
}

void AminoAcidSequencePlayer::stopActiveNote()
{
    if (midiOutput == nullptr)
        return;

    std::vector<int> notesToStop;

    {
        const juce::ScopedLock sl (noteStateLock);
        stopTimer();

        notesToStop.reserve (scheduledNoteOffs.size() + 1);
        for (const auto& scheduled : scheduledNoteOffs)
            notesToStop.push_back (scheduled.note);

        scheduledNoteOffs.clear();

        if (activeSustainNote >= 0)
            notesToStop.push_back (activeSustainNote);

        activeSustainNote = -1;
    }

    for (const auto note : notesToStop)
        midiOutput->sendMessageNow (juce::MidiMessage::noteOff (1, note));
}

void AminoAcidSequencePlayer::playNote (int note, int velocity)
{
    if (midiOutput == nullptr)
        return;

    if (sustainEnabled)
    {
        const juce::ScopedLock sl (noteStateLock);

        if (note == activeSustainNote)
            return;

        if (activeSustainNote >= 0)
            sendNoteOff (activeSustainNote);

        midiOutput->sendMessageNow (juce::MidiMessage::noteOn (1, note, (juce::uint8) velocity));
        activeSustainNote = note;
        return;
    }

    midiOutput->sendMessageNow (juce::MidiMessage::noteOn (1, note, (juce::uint8) velocity));
    scheduleNoteOff (note);
}

void AminoAcidSequencePlayer::scheduleNoteOff (int note)
{
    const juce::ScopedLock sl (noteStateLock);

    scheduledNoteOffs.push_back ({
        note,
        juce::Time::getMillisecondCounter() + static_cast<std::uint32_t> (noteDurationMs)
    });

    if (! isTimerRunning())
        startTimer (5);
}

void AminoAcidSequencePlayer::sendNoteOff (int note)
{
    if (midiOutput == nullptr)
        return;

    midiOutput->sendMessageNow (juce::MidiMessage::noteOff (1, note));
}

void AminoAcidSequencePlayer::timerCallback()
{
    if (midiOutput == nullptr)
        return;

    const auto now = juce::Time::getMillisecondCounter();
    std::vector<int> notesToStop;

    {
        const juce::ScopedLock sl (noteStateLock);

        for (int i = (int) scheduledNoteOffs.size() - 1; i >= 0; --i)
        {
            if (scheduledNoteOffs[(size_t) i].offAtMs > now)
                continue;

            notesToStop.push_back (scheduledNoteOffs[(size_t) i].note);
            scheduledNoteOffs.erase (scheduledNoteOffs.begin() + i);
        }

        if (scheduledNoteOffs.empty())
            stopTimer();
    }

    for (const auto note : notesToStop)
        sendNoteOff (note);
}

void AminoAcidSequencePlayer::performFullReset()
{
    resetReadPosition();
}

void AminoAcidSequencePlayer::syncStartMapTrunc()
{
    startMapTrunc = cachedStartMap;
}

bool AminoAcidSequencePlayer::isIndexInStartMap (int index, const std::vector<std::int64_t>& startMap) noexcept
{
    return std::binary_search (startMap.begin(), startMap.end(), static_cast<std::int64_t> (index));
}

juce::String AminoAcidSequencePlayer::readCodonAt (const juce::String& dna, int index)
{
    if (index < 0 || index + 2 >= dna.length())
        return {};

    juce::String codon;
    codon << juce::CharacterFunctions::toUpperCase (dna[index])
            << juce::CharacterFunctions::toUpperCase (dna[index + 1])
            << juce::CharacterFunctions::toUpperCase (dna[index + 2]);
    return codon;
}

void AminoAcidSequencePlayer::refreshSequenceCache()
{
    if (dnaSequenceProvider)
        cachedDna = dnaSequenceProvider();

    if (startCodonMapProvider)
        cachedStartMap = startCodonMapProvider();
}

void AminoAcidSequencePlayer::checkSequenceReload()
{
    if (! sequenceRevisionProvider)
        return;

    const auto revision = sequenceRevisionProvider();
    if (revision == cachedSequenceRevision)
        return;

    cachedSequenceRevision = revision;
    refreshSequenceCache();
    performFullReset();
}

void AminoAcidSequencePlayer::advanceWhitespaceMode()
{
    const auto readIndex = currentReadIndex.load (std::memory_order_relaxed);

    if (isIndexInStartMap (readIndex, cachedStartMap))
    {
        isReadingCodonsFlag.store (true, std::memory_order_release);
        return;
    }

    const auto nextStartCodonIndex = startMapTrunc.empty() ? std::optional<std::int64_t> {}
                                                           : std::optional<std::int64_t> { startMapTrunc.front() };

    if (nextStartCodonIndex.has_value()
        && readIndex + whiteSpaceReadSpeed >= (int) *nextStartCodonIndex)
    {
        const auto atgIndex = (int) *nextStartCodonIndex;
        currentReadIndex.store (atgIndex, std::memory_order_release);
        isReadingCodonsFlag.store (true, std::memory_order_release);

        while (! startMapTrunc.empty() && atgIndex >= (int) startMapTrunc.front())
            startMapTrunc.erase (startMapTrunc.begin());

        return;
    }

    if (cachedDna.length() < readIndex + whiteSpaceReadSpeed)
    {
        performFullReset();
        return;
    }

    currentReadIndex.store (readIndex + whiteSpaceReadSpeed, std::memory_order_release);

    const auto updatedIndex = currentReadIndex.load (std::memory_order_relaxed);
    while (! startMapTrunc.empty() && updatedIndex >= (int) startMapTrunc.front())
        startMapTrunc.erase (startMapTrunc.begin());
}

void AminoAcidSequencePlayer::advanceCodonMode()
{
    const auto readIndex = currentReadIndex.load (std::memory_order_relaxed);

    if (cachedDna.length() < readIndex + 3)
    {
        performFullReset();
        return;
    }

    const auto codon = readCodonAt (cachedDna, readIndex);
    currentReadIndex.store (readIndex + 3, std::memory_order_release);

    if (codonMap.isStopCodon (codon))
    {
        isReadingCodonsFlag.store (false, std::memory_order_release);
        stopActiveNote();
        return;
    }

    const auto playback = codonMap.lookupCodon (codon);
    if (! playback.has_value())
    {
        ErrorLog::getInstance().addError ("CodonPlayback",
                                         "Invalid codon '" + codon + "' at index " + juce::String (readIndex));
        return;
    }

    playNote (playback->note, playback->velocity);
}

void AminoAcidSequencePlayer::onMidiClockTick()
{
    if (midiOutput == nullptr)
        return;

    if (isReadingCodonsFlag.load (std::memory_order_acquire))
        return;

    checkSequenceReload();

    if (cachedDna.isEmpty())
        return;

    advanceWhitespaceMode();
}

void AminoAcidSequencePlayer::onDivisionPulse()
{
    if (midiOutput == nullptr)
        return;

    if (! isReadingCodonsFlag.load (std::memory_order_acquire))
        return;

    checkSequenceReload();

    if (! isReadingCodonsFlag.load (std::memory_order_acquire))
        return;

    if (cachedDna.isEmpty())
        return;

    advanceCodonMode();
}
