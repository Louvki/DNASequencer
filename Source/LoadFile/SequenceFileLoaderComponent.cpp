#include "LoadFile/SequenceFileLoaderComponent.h"
#include "LoadFile/DnaFastaLoader.h"
#include "ErrorReporting/ErrorLog.h"

#include <functional>

namespace
{
const auto kFileSelectedFillColour = juce::Colour (0xffA1EF8B);

class FileDropAreaComponentImpl : public juce::Component,
                                  public juce::FileDragAndDropTarget
{
public:
    std::function<void()> onBrowseClicked;
    std::function<void(const juce::File&)> onFileDropped;

    void setFileSelected (bool selected)
    {
        if (fileSelected == selected)
            return;

        fileSelected = selected;
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced (0.5f);

        juce::Colour fillColour;
        juce::Colour outlineColour;
        juce::String labelText;

        if (isDragOver)
        {
            fillColour = findColour (juce::TextButton::buttonOnColourId);
            outlineColour = fillColour.contrasting (0.2f);
            labelText = fileSelected ? "File selected" : "Select file or drop here";
        }
        else if (fileSelected)
        {
            fillColour = kFileSelectedFillColour.withAlpha (0.5f);
            outlineColour = kFileSelectedFillColour;
            labelText = "File selected";
        }
        else
        {
            fillColour = findColour (juce::TextButton::buttonColourId);
            outlineColour = fillColour.contrasting (0.35f);
            labelText = "Select file or drop here";
        }

        g.setColour (fillColour);
        g.fillRoundedRectangle (bounds, 4.0f);
        g.setColour (outlineColour);
        g.drawRoundedRectangle (bounds, 4.0f, 1.0f);

        g.setColour (findColour (juce::Label::textColourId));
        g.setFont (juce::FontOptions (14.0f));
        g.drawFittedText (labelText, getLocalBounds(), juce::Justification::centred, 2);
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (e.mouseWasClicked() && onBrowseClicked != nullptr)
            onBrowseClicked();
    }

    bool isInterestedInFileDrag (const juce::StringArray& files) override
    {
        return files.size() == 1 && juce::File (files[0]).existsAsFile();
    }

    void fileDragEnter (const juce::StringArray&, int, int) override
    {
        isDragOver = true;
        repaint();
    }

    void fileDragExit (const juce::StringArray&) override
    {
        isDragOver = false;
        repaint();
    }

    void filesDropped (const juce::StringArray& files, int, int) override
    {
        isDragOver = false;
        repaint();

        if (files.size() == 1 && onFileDropped != nullptr)
            onFileDropped (juce::File (files[0]));
    }

private:
    bool isDragOver { false };
    bool fileSelected { false };
};
} // namespace

struct SequenceFileLoaderComponent::FileDropAreaComponent : FileDropAreaComponentImpl
{
};

/* UI only: */

//==============================================================================
SequenceFileLoaderComponent::SequenceFileLoaderComponent()
{
    fileDropArea = std::make_unique<FileDropAreaComponent>();
    fileDropArea->onBrowseClicked = [this] { showFileBrowser(); };
    fileDropArea->onFileDropped = [this] (const juce::File& file) { beginLoadFromFile (file); };
    addAndMakeVisible (*fileDropArea);

    addAndMakeVisible (statusLogLabel);
    statusLogLabel.setJustificationType (juce::Justification::topLeft);
    statusLogLabel.setFont (juce::FontOptions (13.0f));
    statusLogLabel.setVisible (false);
}

void SequenceFileLoaderComponent::showFileBrowser()
{
    fileChooserHoldAlive_ = std::make_unique<juce::FileChooser> ("Select DNA/FASTA file", juce::File {}, "*");
    const auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
    fileChooserHoldAlive_->launchAsync (flags, [this] (const juce::FileChooser& browser)
    {
        handleFileChooserResult (browser);
    });
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
    constexpr int kDropAreaHeight = 48;
    constexpr int kDropAreaWidth = 180;
    constexpr int kStatusLogGap = 10;
    constexpr int kBottomMargin = 24;

    r.removeFromBottom (kBottomMargin);
    auto row = r.removeFromTop (kDropAreaHeight);
    fileDropArea->setBounds (row.removeFromLeft (juce::jmin (kDropAreaWidth, row.getWidth())));
    row.removeFromLeft (kStatusLogGap);
    statusLogLabel.setBounds (row);
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
    juce::String fileName;
    juce::int64 sequenceLength = 0;
    juce::int64 startCodonCount = 0;
    bool hasLoadedData = false;

    const bool isReadingFile = isFileLoadInProgress_.load();

    {
        const juce::ScopedLock sl (dataLock_);

        fileName = displayedFileShortName_;

        if (! isReadingFile && lastError_.isEmpty() && loadedDnaSequence_.isNotEmpty())
        {
            hasLoadedData = true;
            sequenceLength = loadedDnaSequence_.length();
            startCodonCount = static_cast<juce::int64> (startCodonMap_.size());
        }
    }

    if (hasLoadedData)
    {
        fileDropArea->setFileSelected (true);
        statusLogLabel.setVisible (true);
        statusLogLabel.setText ("Name:\t\t" + dna::DnaFastaLoader::truncateFileNameForDisplay (fileName)
                                + "\nLength:\t\t" + dna::DnaFastaLoader::formatCountForDisplay (sequenceLength)
                                + "\nStart Codons:\t" + dna::DnaFastaLoader::formatCountForDisplay (startCodonCount),
                                juce::dontSendNotification);
    }
    else
    {
        fileDropArea->setFileSelected (false);
        statusLogLabel.setVisible (false);
        statusLogLabel.setText ({}, juce::dontSendNotification);
    }
}
