#include "SequenceFileLoaderComponent.h"
#include "DnaFastaLoader.h"

/* UI only: */

//==============================================================================
SequenceFileLoaderComponent::SequenceFileLoaderComponent()
{
    // File name label
    addAndMakeVisible (fileNameLabel);
    fileNameLabel.setJustificationType (juce::Justification::centredLeft);
    fileNameLabel.setMinimumHorizontalScale (1.0f);
    fileNameLabel.setText ("No file selected", juce::dontSendNotification);

    // File loading... Etc
    addAndMakeVisible (fileStatusLabel);
    fileStatusLabel.setJustificationType (juce::Justification::centredLeft);
    fileStatusLabel.setText ({}, juce::dontSendNotification);

    addAndMakeVisible (openButton);
    openButton.onClick = [this]
    {
        // Keep the file chooser alive until the async callback returns (JUCE lifetime rule).
        aliveChooserHold_ = std::make_unique<juce::FileChooser> ("Select DNA/FASTA file", juce::File {}, "*");

        const auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

        // Open the dialog and handle selected file
        aliveChooserHold_->launchAsync (flags, [this] (const juce::FileChooser& browser) { 
            handleFileChooserResult (browser); 
        });
    };
}

// On destroy hook
SequenceFileLoaderComponent::~SequenceFileLoaderComponent()
{
    // Ensure the worker never touches freed members (`this` owns the thread).
    joinLoadThread();
}

void SequenceFileLoaderComponent::joinLoadThread()
{
    // Tells `runBackgroundLoad` to bail before scheduling UI updates (new file picked, or component destroyed).
    isFileLoadCancelRequested_.store (true);

    if (loadThread_ != nullptr)
    {
        if (loadThread_->joinable())
            loadThread_->join();

        loadThread_.reset();
    }

    // Ready for the next session; cleared again in `beginLoadFromFile`, but resetting here avoids weird states
    // if no follow-up load is started.
    isFileLoadCancelRequested_.store (false);
}

void SequenceFileLoaderComponent::handleFileChooserResult (const juce::FileChooser& browser)
{
    const auto selectedFiles = browser.getResults();

    // Cancel if more than 1 file is selected
    if (selectedFiles.size() != 1)
    {
        aliveChooserHold_.reset();
        return;
    }

    beginLoadFromFile (selectedFiles.getReference (0));
    aliveChooserHold_.reset();
}

juce::String SequenceFileLoaderComponent::getLastError() const
{
    const juce::ScopedLock sl (dataLock_);
    return lastError_;
}

juce::String SequenceFileLoaderComponent::getLoadedSequence() const
{
    const juce::ScopedLock sl (dataLock_);
    return loadedSequence_;
}

std::vector<std::int64_t> SequenceFileLoaderComponent::getStartCodoneMap() const
{
    const juce::ScopedLock sl (dataLock_);
    return startCodonMap_;
}

void SequenceFileLoaderComponent::resized()
{
    auto r = getLocalBounds();
    auto top = r.removeFromTop (30);
    openButton.setBounds (top.withWidth (juce::jmin (220, top.getWidth() - 4)));
    auto rest = top.withTrimmedLeft (openButton.getWidth() + 8).withTrimmedBottom (4);
    fileNameLabel.setBounds (rest);

    fileStatusLabel.setBounds (r.withTrimmedTop (2).removeFromTop (24));
}

// Runs on the JUCE message thread only — analogous to firing Max outlets after processing.
void SequenceFileLoaderComponent::deliverLoadSucceeded (juce::String&& sequenceUtf8Upper,
                                                      std::vector<std::int64_t>&& codonPositions)
{
    // Stores `fileContent` + `startCodonMap` analogues for later sequencing code.
    isFileLoadInProgress_.store (false);

    {
        const juce::ScopedLock sl (dataLock_);
        loadedSequence_ = std::move (sequenceUtf8Upper);
        startCodonMap_ = std::move (codonPositions);
        lastError_.clear();
    }

    updateUiLabels();
}

// Failure path queued from worker open errors; keeps label updates coherent with successes.
void SequenceFileLoaderComponent::deliverLoadFailed (juce::String error)
{
    isFileLoadInProgress_.store (false);

    {
        const juce::ScopedLock sl (dataLock_);
        lastError_ = error;
    }

    updateUiLabels();
}

void SequenceFileLoaderComponent::beginLoadFromFile (const juce::File& file)
{
    // Never two loads at once: wait out the previous worker before mutating shared UI/file state.
    joinLoadThread();

    // The braces are to create a lock. This is so the background thread and the UI thread 
    // both do not touch lastError at the same time which might cause errors.
    {
        const juce::ScopedLock sl (dataLock_);
        lastError_.clear();
    }

    // Early exit
    if (!file.existsAsFile())
    {
        deliverLoadFailed ("File does not exist.");
        return;
    }

    // The braces are to create a lock. This is so the background thread and the UI thread 
    // both do not touch lastError at the same time which might cause errors.
    {
        const juce::ScopedLock sl (dataLock_);
        displayedFileShortName_ = dna::DnaFastaLoader::truncateFileNameForDisplay (file.getFileName());
    }

    isFileLoadInProgress_.store (true);
    
    isFileLoadCancelRequested_.store (false); // Reset to default value

    updateUiLabels();

    // `juce::File` is a path handle; copy by value for the worker lambda.
    juce::File fileCopy (file);

    loadThread_ = std::make_unique<std::thread> ([this, fileCopy]()
                                                {
                                                    runBackgroundLoad (fileCopy);
                                                });
}

void SequenceFileLoaderComponent::runBackgroundLoad (juce::File file)
{
    auto result = dna::DnaFastaLoader::processFile (file, &isFileLoadCancelRequested_);

    if (result.cancelled)
        return;

    if (result.error.isNotEmpty())
    {
        juce::MessageManager::callAsync ([weak = juce::Component::SafePointer<SequenceFileLoaderComponent> (this),
                                          msg = std::move (result.error)]() mutable
                                         {
                                             if (weak != nullptr)
                                                 weak->deliverLoadFailed (std::move (msg));
                                         });
        return;
    }

    juce::MessageManager::callAsync ([weak = juce::Component::SafePointer<SequenceFileLoaderComponent> (this),
                                      seqUpper = std::move (result.dnaSequence),
                                      codons = std::move (result.startCodonMap)]() mutable
                                     {
                                         if (weak != nullptr)
                                             weak->deliverLoadSucceeded (std::move (seqUpper),
                                                                         std::move (codons));
                                     });
}

void SequenceFileLoaderComponent::updateUiLabels()
{
    // Label writes must run on the message thread; we snapshot strings under `dataLock_` first.
    juce::String fileShown;
    juce::String errCopy;
    juce::String successLine;

    const bool isReadingFile = isFileLoadInProgress_.load();

    {
        const juce::ScopedLock sl (dataLock_);

        fileShown = displayedFileShortName_;
        errCopy = lastError_;

        if (!isReadingFile && errCopy.isEmpty() && loadedSequence_.isNotEmpty())
            successLine = "Finished. Length: "
                          + juce::String ((juce::int64) loadedSequence_.length())
                          + " bp, ATG codons: "
                          + juce::String ((juce::int64) startCodonMap_.size());
    }

    fileNameLabel.setText (fileShown, juce::dontSendNotification);

    if (errCopy.isNotEmpty())
        fileStatusLabel.setText ("Error: " + errCopy, juce::dontSendNotification);
    else if (isReadingFile)
        fileStatusLabel.setText ("Reading file...", juce::dontSendNotification);
    else if (successLine.isNotEmpty())
        fileStatusLabel.setText (successLine, juce::dontSendNotification);
    else
        fileStatusLabel.setText ({}, juce::dontSendNotification);
}
