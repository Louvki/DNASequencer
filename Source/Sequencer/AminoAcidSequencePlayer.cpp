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

void AminoAcidSequencePlayer::setChordChancePercent (int percent) noexcept
{
    chordChancePercent = juce::jlimit (0, 100, percent);
}

void AminoAcidSequencePlayer::setChordTypeWeight (dna::ChordType type, int weight) noexcept
{
    const auto index = static_cast<size_t> (type);
    if (index < chordTypeWeights.size())
        chordTypeWeights[index] = juce::jlimit (0, 100, weight);
}

void AminoAcidSequencePlayer::setChordStrumMaxMs (int maxMs) noexcept
{
    chordStrumMaxMs = juce::jlimit (0, 200, maxMs);
}

void AminoAcidSequencePlayer::setChordVelocityRange (int range) noexcept
{
    chordVelocityRange = juce::jlimit (0, 64, range);
}

void AminoAcidSequencePlayer::setWhiteSpaceReadSpeed (int speed) noexcept
{
    whiteSpaceReadSpeed = juce::jmax (1, speed);
}

void AminoAcidSequencePlayer::setNoteDurationMs (int durationMs) noexcept
{
    noteDurationMs = juce::jlimit (1, 3000, durationMs);
}

void AminoAcidSequencePlayer::setSustainEnabled (bool enabled) noexcept
{
    sustainEnabled = enabled;
}

void AminoAcidSequencePlayer::rebuildCodonMap()
{
    const auto aminoAcidsWithScaleApplied = dna::applyScaleToAminoAcids (rootNote, scale, notePoolSize);
    codonMap.rebuildCodonMidiPlaybackMap (aminoAcidsWithScaleApplied);
}

int AminoAcidSequencePlayer::getSequenceLength()
{
    checkSequenceReload();
    return cachedDna.length();
}

void AminoAcidSequencePlayer::resetReadPosition()
{
    currentReadIndex.store (0, std::memory_order_release);
    isReadingCodonsFlag.store (false, std::memory_order_release);
    syncStartMapTrunc();

    const juce::ScopedLock sl (noteStateLock);
    activeSustainNotes.clear();
    scheduledNoteOns.clear();
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
        scheduledNoteOns.clear();

        notesToStop.insert (notesToStop.end(), activeSustainNotes.begin(), activeSustainNotes.end());
        activeSustainNotes.clear();
    }

    for (const auto note : notesToStop)
        midiOutput->sendMessageNow (juce::MidiMessage::noteOff (1, note));
}

void AminoAcidSequencePlayer::playNotes (const std::vector<int>& notes, int velocity)
{
    if (midiOutput == nullptr || notes.empty())
        return;

    const bool isChord = notes.size() > 1;
    const bool useStrum = isChord && chordStrumMaxMs > 0;

    const auto velocityFor = [&] (size_t index)
    {
        if (! isChord || chordVelocityRange <= 0)
            return velocity;

        juce::ignoreUnused (index);
        return randomizeChordVelocity (velocity);
    };

    if (sustainEnabled)
    {
        const juce::ScopedLock sl (noteStateLock);

        const bool sameNotes = (int) activeSustainNotes.size() == (int) notes.size()
                               && std::equal (activeSustainNotes.begin(), activeSustainNotes.end(), notes.begin());

        if (sameNotes)
            return;

        for (const auto note : activeSustainNotes)
            sendNoteOff (note);

        scheduledNoteOns.clear();
        activeSustainNotes = notes;

        if (! useStrum)
        {
            for (size_t i = 0; i < notes.size(); ++i)
                sendNoteOn (notes[i], velocityFor (i));
        }
        else
        {
            const auto now = juce::Time::getMillisecondCounter();

            for (size_t i = 0; i < notes.size(); ++i)
            {
                const auto delayMs = static_cast<std::uint32_t> (strumRandom.nextInt (chordStrumMaxMs + 1));
                scheduledNoteOns.push_back ({
                    notes[i],
                    velocityFor (i),
                    now + delayMs,
                    false
                });
            }

            if (! isTimerRunning())
                startTimer (5);
        }

        return;
    }

    if (! useStrum)
    {
        for (size_t i = 0; i < notes.size(); ++i)
        {
            sendNoteOn (notes[i], velocityFor (i));
            scheduleNoteOff (notes[i]);
        }

        return;
    }

    for (size_t i = 0; i < notes.size(); ++i)
        scheduleNoteOn (notes[i], velocityFor (i), strumRandom.nextInt (chordStrumMaxMs + 1), true);
}

int AminoAcidSequencePlayer::randomizeChordVelocity (int baseVelocity) const noexcept
{
    if (chordVelocityRange <= 0)
        return baseVelocity;

    const auto offset = strumRandom.nextInt (chordVelocityRange * 2 + 1) - chordVelocityRange;
    return juce::jlimit (1, 127, baseVelocity + offset);
}

dna::ChordType AminoAcidSequencePlayer::pickWeightedChordType() const noexcept
{
    int totalWeight = 0;

    for (const auto weight : chordTypeWeights)
        totalWeight += juce::jmax (0, weight);

    if (totalWeight <= 0)
        return dna::ChordType::triad;

    auto roll = strumRandom.nextInt (totalWeight);

    for (size_t i = 0; i < chordTypeWeights.size(); ++i)
    {
        const auto weight = juce::jmax (0, chordTypeWeights[i]);
        if (weight <= 0)
            continue;

        if (roll < weight)
            return static_cast<dna::ChordType> (i);

        roll -= weight;
    }

    return dna::ChordType::triad;
}

std::vector<int> AminoAcidSequencePlayer::resolvePlaybackNotes (int baseNote)
{
    if (chordChancePercent <= 0)
        return { baseNote };

    if (chordChancePercent < 100 && strumRandom.nextInt (100) >= chordChancePercent)
        return { baseNote };

    return dna::buildDiatonicChord (baseNote, rootNote, scale, pickWeightedChordType());
}

void AminoAcidSequencePlayer::sendNoteOn (int note, int velocity)
{
    if (midiOutput == nullptr)
        return;

    midiOutput->sendMessageNow (juce::MidiMessage::noteOn (1, note, (juce::uint8) velocity));
}

void AminoAcidSequencePlayer::scheduleNoteOn (int note, int velocity, int delayMs, bool scheduleOffAfter)
{
    if (delayMs <= 0)
    {
        sendNoteOn (note, velocity);

        if (scheduleOffAfter)
            scheduleNoteOff (note);

        return;
    }

    const juce::ScopedLock sl (noteStateLock);

    scheduledNoteOns.push_back ({
        note,
        velocity,
        juce::Time::getMillisecondCounter() + static_cast<std::uint32_t> (delayMs),
        scheduleOffAfter
    });

    if (! isTimerRunning())
        startTimer (5);
}

void AminoAcidSequencePlayer::scheduleNoteOff (int note)
{
    const juce::ScopedLock sl (noteStateLock);

    scheduledNoteOffs.erase (
        std::remove_if (scheduledNoteOffs.begin(), scheduledNoteOffs.end(),
                        [note] (const ScheduledNoteOff& scheduled) { return scheduled.note == note; }),
        scheduledNoteOffs.end());

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
    std::vector<ScheduledNoteOn> notesToStart;
    std::vector<int> notesToStop;

    {
        const juce::ScopedLock sl (noteStateLock);

        for (int i = (int) scheduledNoteOns.size() - 1; i >= 0; --i)
        {
            if (scheduledNoteOns[(size_t) i].onAtMs > now)
                continue;

            notesToStart.push_back (scheduledNoteOns[(size_t) i]);
            scheduledNoteOns.erase (scheduledNoteOns.begin() + i);
        }

        for (int i = (int) scheduledNoteOffs.size() - 1; i >= 0; --i)
        {
            if (scheduledNoteOffs[(size_t) i].offAtMs > now)
                continue;

            notesToStop.push_back (scheduledNoteOffs[(size_t) i].note);
            scheduledNoteOffs.erase (scheduledNoteOffs.begin() + i);
        }

        if (scheduledNoteOns.empty() && scheduledNoteOffs.empty())
            stopTimer();
    }

    for (const auto& scheduled : notesToStart)
    {
        sendNoteOn (scheduled.note, scheduled.velocity);

        if (scheduled.scheduleOffAfter)
            scheduleNoteOff (scheduled.note);
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
        if (sustainEnabled)
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

    const auto notes = resolvePlaybackNotes (playback->note);

    playNotes (notes, playback->velocity);
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
