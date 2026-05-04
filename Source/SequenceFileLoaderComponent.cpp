#include "SequenceFileLoaderComponent.h"
#include "DnaFastaLoader.h"

/* UI only: disk + parsing live in `dna::DnaFastaLoader`; worker thread + `callAsync` mirror Max yield + main thread. */

//==============================================================================
SequenceFileLoaderComponent::SequenceFileLoaderComponent()
{
    addAndMakeVisible (openButton);
    addAndMakeVisible (fileLabel);
    fileLabel.setJustificationType (juce::Justification::centredLeft);
    fileLabel.setText ("No file selected", juce::dontSendNotification);
    fileLabel.setMinimumHorizontalScale (1.0f);

    addAndMakeVisible (statusLabel);
    statusLabel.setJustificationType (juce::Justification::centredLeft);
    statusLabel.setText ({}, juce::dontSendNotification);

    openButton.onClick = [this]
    {
        // Keep the chooser alive until the async callback returns (JUCE lifetime rule).
        aliveChooserHold_ = std::make_unique<juce::FileChooser> ("Select DNA/FASTA file",
                                                                 juce::File {},
                                                                 "*");

        const auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

        aliveChooserHold_->launchAsync (flags, [this] (const juce::FileChooser& browser)
                                        {
                                            const auto results = browser.getResults();

                                            if (results.size() != 1)
                                            {
                                                aliveChooserHold_.reset();
                                                return;
                                            }

                                            beginLoadFromFile (results.getReference (0));
                                            aliveChooserHold_.reset();
                                        });
    };
}

SequenceFileLoaderComponent::~SequenceFileLoaderComponent()
{
    // Ensure the worker never touches freed members (`this` owns the thread).
    joinLoadThread();
}

void SequenceFileLoaderComponent::joinLoadThread()
{
    // Tells `runBackgroundLoad` to bail before scheduling UI updates (new file picked, or component destroyed).
    loadCancelRequested_.store (true);

    if (loadThread_ != nullptr)
    {
        if (loadThread_->joinable())
            loadThread_->join();

        loadThread_.reset();
    }

    // Ready for the next session; cleared again in `beginLoadFromFile`, but resetting here avoids weird states
    // if no follow-up load is started.
    loadCancelRequested_.store (false);
}

bool SequenceFileLoaderComponent::isLoadInProgress() const noexcept
{
    return loadActive_.load();
}

juce::String SequenceFileLoaderComponent::getDisplayedFileName() const
{
    const juce::ScopedLock sl (dataLock_);
    return displayedFileShortName_;
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
    fileLabel.setBounds (rest);

    statusLabel.setBounds (r.withTrimmedTop (2).removeFromTop (24));
}

// Runs on the JUCE message thread only — analogous to firing Max outlets after processing.
void SequenceFileLoaderComponent::deliverLoadSucceeded (juce::String&& sequenceUtf8Upper,
                                                      std::vector<std::int64_t>&& codonPositions)
{
    // Stores `fileContent` + `startCodonMap` analogues for later sequencing code.
    loadActive_.store (false);

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
    loadActive_.store (false);

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

    {
        const juce::ScopedLock sl (dataLock_);
        lastError_.clear();
    }

    if (! file.existsAsFile())
    {
        deliverLoadFailed ("File does not exist.");
        return;
    }

    {
        const juce::ScopedLock sl (dataLock_);
        displayedFileShortName_ = dna::DnaFastaLoader::truncateFileNameForDisplay (file.getFileName());
    }

    loadActive_.store (true);

    loadCancelRequested_.store (false);

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
    auto result = dna::DnaFastaLoader::processFile (file, &loadCancelRequested_);

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

    const bool reading = loadActive_.load();

    {
        const juce::ScopedLock sl (dataLock_);

        fileShown = displayedFileShortName_;
        errCopy = lastError_;

        if (! reading && errCopy.isEmpty() && loadedSequence_.isNotEmpty())
            successLine = "Finished. Length: "
                          + juce::String ((juce::int64) loadedSequence_.length())
                          + " bp, ATG codons: "
                          + juce::String ((juce::int64) startCodonMap_.size());
    }

    fileLabel.setText (fileShown, juce::dontSendNotification);

    if (errCopy.isNotEmpty())
        statusLabel.setText ("Error: " + errCopy, juce::dontSendNotification);
    else if (reading)
        statusLabel.setText ("Reading file...", juce::dontSendNotification);
    else if (successLine.isNotEmpty())
        statusLabel.setText (successLine, juce::dontSendNotification);
    else
        statusLabel.setText ({}, juce::dontSendNotification);
}
