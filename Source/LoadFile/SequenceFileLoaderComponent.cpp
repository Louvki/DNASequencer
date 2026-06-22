#include "LoadFile/SequenceFileLoaderComponent.h"
#include "LoadFile/DnaFastaLoader.h"
#include "ErrorReporting/ErrorLog.h"

/* UI only: */

//==============================================================================
SequenceFileLoaderComponent::SequenceFileLoaderComponent()
    : SequenceFileLoaderComponent (nullptr)
{
}

SequenceFileLoaderComponent::SequenceFileLoaderComponent (std::function<void()> onResetClicked)
    : onResetClicked_ (std::move (onResetClicked))
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
        fileChooserHoldAlive_ = std::make_unique<juce::FileChooser> ("Select DNA/FASTA file", juce::File {}, "*");
        const auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
        // Open the dialog and handle selected file
        fileChooserHoldAlive_->launchAsync (flags, [this] (const juce::FileChooser& browser) { 
            handleFileChooserResult (browser); 
        });
    };

    addAndMakeVisible (resetButton);
    resetButton.onClick = [this]
    {
        if (onResetClicked_ != nullptr)
            onResetClicked_();
    };
    resetButton.setVisible (onResetClicked_ != nullptr);
}

void SequenceFileLoaderComponent::setOnResetClicked (std::function<void()> callback)
{
    onResetClicked_ = std::move (callback);
    resetButton.setVisible (onResetClicked_ != nullptr);
}

// On destroy hook
SequenceFileLoaderComponent::~SequenceFileLoaderComponent()
{
    // Ensure the worker never touches freed members (`this` owns the thread).
    joinLoadThread();
}

// On window resize
void SequenceFileLoaderComponent::resized()
{
    auto r = getLocalBounds();
    constexpr int kButtonHeight = 28;
    constexpr int kButtonGap = 4;
    const auto buttonWidth = juce::jmin (220, r.getWidth());

    openButton.setBounds (r.removeFromTop (kButtonHeight).withWidth (buttonWidth));
    r.removeFromTop (kButtonGap);
    resetButton.setBounds (r.removeFromTop (kButtonHeight).withWidth (juce::jmin (100, buttonWidth)));
    r.removeFromTop (6);
    fileNameLabel.setBounds (r.removeFromTop (22));
    fileStatusLabel.setBounds (r.removeFromTop (22));
}

juce::String SequenceFileLoaderComponent::getLoadedDnaSequence() const
{
    const juce::ScopedLock sl (dataLock_);
    return loadedDnaSequence_;
}

std::vector<std::int64_t> SequenceFileLoaderComponent::getStartCodonMap() const
{
    const juce::ScopedLock sl (dataLock_);
    return startCodonMap_;
}

void SequenceFileLoaderComponent::handleFileChooserResult (const juce::FileChooser& browser)
{
    const auto selectedFiles = browser.getResults();

    // Cancel if more than 1 file is selected
    if (selectedFiles.size() != 1)
    {
        fileChooserHoldAlive_.reset();
        return;
    }

    beginLoadFromFile (selectedFiles.getReference (0));
    fileChooserHoldAlive_.reset();
}

void SequenceFileLoaderComponent::joinLoadThread()
{
    // Tells `runBackgroundLoad` to bail before scheduling UI updates (new file picked, or component destroyed).
    isFileLoadCancelRequested_.store (true);

    if (loadFileThread_ != nullptr)
    {
        if (loadFileThread_->joinable())
            loadFileThread_->join();

        loadFileThread_.reset();
    }

    // if no follow-up load is started.
    isFileLoadCancelRequested_.store (false);
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
        displayErrorInTheUi ("File does not exist.");
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

    // We copy the juce::File object here because it only holds a path name internally,
    // so passing it by value is cheap. Copying ensures the worker lambda owns its own
    // copy of the path, protecting us from issues if the original 'file' variable goes
    // out of scope or is modified before the background thread starts.
    juce::File fileCopy (file);

    
    // This line creates a new background thread to load and process the selected sequence file without blocking the main UI.
    // It makes a unique_ptr to a std::thread running the member function runBackgroundLoad, passing a copy of the selected file.
    // This keeps the UI responsive while performing potentially slow file and CPU operations in the background.
    loadFileThread_ = std::make_unique<std::thread> ([this, fileCopy]() { runBackgroundLoad (fileCopy); });
}

void SequenceFileLoaderComponent::runBackgroundLoad (juce::File file)
{
    dna::FileReadResult result = dna::DnaFastaLoader::processFile (file, &isFileLoadCancelRequested_);

    // CANCELLED: Early return
    if (result.cancelled) return;

    // ERROR: Display errors in the UI
    if (result.error.isNotEmpty()) {
        juce::MessageManager::callAsync (
            [weak = juce::Component::SafePointer<SequenceFileLoaderComponent> (this),
             msg = std::move (result.error)]() mutable

            //  Create a lock
            {
                if (weak != nullptr)
                    weak->displayErrorInTheUi (std::move (msg));
            }
        );
                             
        return;
    }

    // SUCCESS: 
    juce::MessageManager::callAsync(
        [weak = juce::Component::SafePointer<SequenceFileLoaderComponent>(this),
         // std::move is used here to transfer ownership of result.dnaSequence and result.startCodonMap
         // into the lambda's captured variables without copying their contents. This allows efficient
         // transfer of large data structures by converting them into rvalue references.
         dnaSequence = std::move(result.dnaSequence),
         startCodonMap = std::move(result.startCodonMap)]() mutable
        {
            if (weak != nullptr) {
                weak->persistLoadedDna (std::move (dnaSequence), std::move (startCodonMap));
                weak->updateUiLabels();
            }
        }
    );                     
}

void SequenceFileLoaderComponent::persistLoadedDna (juce::String dnaSequence, std::vector<std::int64_t> startCodonMap)
{
    // Stores `fileContent` + `startCodonMap` analogues for later sequencing code.
    isFileLoadInProgress_.store (false);

    // We lock here to ensure thread safety when updating shared state.
    // This code runs on the message thread (from callAsync) but updates fields also accessed by the loader worker thread and UI getters.
    // The lock guarantees that updates to loadedDnaSequence_, startCodonMap_, and lastError_ are atomic with respect to any other thread
    // accessing or mutating these fields, preventing inconsistent or partially updated state due to data races.
    {
        const juce::ScopedLock sl (dataLock_);
        loadedDnaSequence_ = std::move (dnaSequence);
        startCodonMap_ = std::move (startCodonMap);
        lastError_.clear();
    }

    sequenceRevision_.fetch_add (1, std::memory_order_release);
}

// Failure path queued from worker open errors; keeps label updates coherent with successes.
void SequenceFileLoaderComponent::displayErrorInTheUi (juce::String error)
{
    isFileLoadInProgress_.store (false);

    {
        const juce::ScopedLock sl (dataLock_);
        lastError_ = error;
    }

    ErrorLog::getInstance().addError ("SequenceFileLoader", error);
    updateUiLabels();
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

        if (!isReadingFile && errCopy.isEmpty() && loadedDnaSequence_.isNotEmpty())
            successLine = "Finished. Length: "
                          + juce::String ((juce::int64) loadedDnaSequence_.length())
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
