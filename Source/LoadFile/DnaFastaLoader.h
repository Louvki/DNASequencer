#pragma once

#include <JuceHeader.h>

#include <atomic>
#include <vector>

namespace dna
{
struct FileReadResult
{
    juce::String dnaSequence;
    std::vector<std::int64_t> startCodonMap;
    juce::String error;

    /** Set when `cancelRequested` was true mid-load — caller should discard and not treat as failure. */
    bool cancelled { false };

    bool ok() const noexcept { return error.isEmpty() && !cancelled; }
};

class DnaFastaLoader
{
public:
    /** Blocking; safe to call from a background thread. If non-null, `cancelRequested` is polled between chunks (cooperative abort). */
    static FileReadResult processFile (const juce::File& file, std::atomic<bool>* cancelRequested = nullptr);
    static juce::String truncateFileNameForDisplay (const juce::String& name);
    static juce::String formatCountForDisplay (std::int64_t value);
};

}
