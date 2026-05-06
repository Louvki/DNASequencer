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

    void resized() override;

    /** Full cleaned sequence (`chunks.join("")` then uppercased); empty until load finishes. */
    juce::String getLoadedDnaSequence() const;
    /** Global indices where `chunk[i:i+3]` was ATG in the sanitized stream (same semantics as JS). */
    std::vector<std::int64_t> getStartCodonMap() const;

private:
    void handleFileChooserResult (const juce::FileChooser& browser);
    void joinLoadThread();
    void beginLoadFromFile (const juce::File& file);
    void runBackgroundLoad (juce::File file); /** Full load + CPU-heavy string work runs here (never on the message thread). */
    void persistLoadedDna (juce::String dnaSequence, std::vector<std::int64_t> startCodonMap);
    void displayErrorInTheUi (juce::String error);
    void updateUiLabels();

    juce::TextButton openButton { "Open FASTA/DNA..." };
    juce::Label fileNameLabel;
    juce::Label fileStatusLabel;
    juce::String displayedFileShortName_ { "No file selected" };
    juce::String lastError_; // Used for displauying the error in the UI

    mutable juce::CriticalSection dataLock_; // Guards fields read by getters + `updateUiLabels` from the GUI.

    // The fields we get after reading the DNA file
    juce::String loadedDnaSequence_;
    std::vector<std::int64_t> startCodonMap_;

    // `isFileLoadInProgress_` is an atomic boolean flag to indicate when a file load is in progress
    std::atomic<bool> isFileLoadInProgress_ { false }; 

    // `isFileLoadCancelRequested_` is an atomic boolean flag that is used to signal cancellation of a file load.
    std::atomic<bool> isFileLoadCancelRequested_ { false };

    // Pointer to the worker thread which we use to load the DNA file
    std::unique_ptr<std::thread> loadFileThread_;

    // Pointer used for keeping the file selector open
    std::unique_ptr<juce::FileChooser> fileChooserHoldAlive_;

    // This JUCE macro disables copy constructor and copy assignment for this class,
    // and also adds built-in memory leak detection in debug builds.
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SequenceFileLoaderComponent)
};
