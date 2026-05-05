#pragma once

#include <JuceHeader.h>

#include <atomic>
#include <cstdint>
#include <memory>
#include <thread>
#include <vector>

/** UI for choosing a file; disk + parsing implemented in `dna::DnaFastaLoader`. */
class SequenceFileLoaderComponent : public juce::Component
{
public:
    SequenceFileLoaderComponent();
    ~SequenceFileLoaderComponent() override;

    /** Full cleaned sequence (`chunks.join("")` then uppercased); empty until load finishes. */
    juce::String getLoadedSequence() const;
    /** Global indices where `chunk[i:i+3]` was ATG in the sanitized stream (same semantics as JS). */
    std::vector<std::int64_t> getStartCodoneMap() const;

    juce::String getLastError() const;

    void resized() override;

private:
    void joinLoadThread();

    /** Full load + CPU-heavy string work runs here (never on the message thread). */
    void runBackgroundLoad (juce::File file);

    void handleFileChooserResult (const juce::FileChooser& browser);
    void beginLoadFromFile (const juce::File& file);
    /** Message-thread only — async delivery from worker. */
    void deliverLoadSucceeded (juce::String&& sequenceUtf8Upper, std::vector<std::int64_t>&& codonPositions);
    void deliverLoadFailed (juce::String error);

    void updateUiLabels();

    juce::TextButton openButton { "Open FASTA/DNA..." };
    juce::Label fileNameLabel;
    juce::Label fileStatusLabel;

    mutable juce::CriticalSection dataLock_; // Guards fields read by getters + `updateUiLabels` from the GUI.
    juce::String loadedSequence_;
    std::vector<std::int64_t> startCodonMap_;

    juce::String displayedFileShortName_ { "No file selected" }; // Mirrors JS short name on outlet 5.
    juce::String lastError_;

    // `isFileLoadInProgress_` is an atomic boolean flag to indicate when a file load is in progress
    std::atomic<bool> isFileLoadInProgress_ { false }; 

    // `isFileLoadCancelRequested_` is an atomic boolean flag that is used to signal cancellation of a file load
    // operation running in a background thread. It is checked by the worker thread (and in the DNA parser)
    // to cooperatively abort load and parsing when, for example, the user selects a new file or the component
    // is destroyed. Here we reset it to false before starting a new load.
    std::atomic<bool> isFileLoadCancelRequested_ { false };

    std::unique_ptr<std::thread> loadThread_;

    std::unique_ptr<juce::FileChooser> aliveChooserHold_; // Must outlive `launchAsync` callback.

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SequenceFileLoaderComponent)
};
