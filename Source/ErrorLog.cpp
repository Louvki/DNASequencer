#include "ErrorLog.h"

namespace
{
class ErrorLogViewComponent : public juce::Component,
                              private juce::Timer
{
public:
    ErrorLogViewComponent()
    {
        addAndMakeVisible (logEditor_);
        logEditor_.setMultiLine (true);
        logEditor_.setReadOnly (true);
        logEditor_.setScrollbarsShown (true);
        logEditor_.setCaretVisible (false);
        refreshFromStore();
        startTimerHz (2);
    }

    void resized() override
    {
        logEditor_.setBounds (getLocalBounds().reduced (8));
    }

private:
    void timerCallback() override
    {
        refreshFromStore();
    }

    void refreshFromStore()
    {
        const auto latest = ErrorLog::getInstance().getLogText();

        if (latest == lastShownText_)
            return;

        lastShownText_ = latest;
        logEditor_.setText (lastShownText_, juce::dontSendNotification);
    }

    juce::TextEditor logEditor_;
    juce::String lastShownText_;
};

class ErrorLogWindow final : public juce::DocumentWindow
{
public:
    ErrorLogWindow()
        : juce::DocumentWindow ("Error Log",
                                juce::Desktop::getInstance().getDefaultLookAndFeel()
                                    .findColour (juce::ResizableWindow::backgroundColourId),
                                juce::DocumentWindow::closeButton)
    {
        setUsingNativeTitleBar (true);
        setResizable (true, true);
        setContentOwned (new ErrorLogViewComponent(), true);
        centreWithSize (700, 420);
    }

    void closeButtonPressed() override
    {
        setVisible (false);
    }
};
} // namespace

ErrorLog& ErrorLog::getInstance()
{
    static ErrorLog instance;
    return instance;
}

void ErrorLog::installForMac()
{
   #if JUCE_MAC
    juce::MenuBarModel::setMacMainMenu (this);
   #endif
}

void ErrorLog::uninstallForMac()
{
    errorLogWindow_ = nullptr;

   #if JUCE_MAC
    juce::MenuBarModel::setMacMainMenu (nullptr);
   #endif
}

void ErrorLog::addError (const juce::String& source, const juce::String& message)
{
    const auto now = juce::Time::getCurrentTime();
    const auto timestamp = now.formatted ("%Y-%m-%d %H:%M:%S");
    const auto line = "[" + timestamp + "] " + source + ": " + message;

    const juce::ScopedLock sl (lock_);
    entries_.add (line);
}

juce::String ErrorLog::getLogText() const
{
    const juce::ScopedLock sl (lock_);

    if (entries_.isEmpty())
        return "No errors logged yet.";

    return entries_.joinIntoString ("\n");
}

void ErrorLog::clear()
{
    const juce::ScopedLock sl (lock_);
    entries_.clear();
}

juce::StringArray ErrorLog::getMenuBarNames()
{
    return { "Window" };
}

juce::PopupMenu ErrorLog::getMenuForIndex (int menuIndex, const juce::String& menuName)
{
    juce::ignoreUnused (menuIndex, menuName);

    juce::PopupMenu menu;
    menu.addItem (showErrorLogMenuId, "Show Error Log");
    menu.addItem (clearErrorLogMenuId, "Clear Error Log");
    return menu;
}

void ErrorLog::menuItemSelected (int menuItemID, int topLevelMenuIndex)
{
    juce::ignoreUnused (topLevelMenuIndex);

    if (menuItemID == showErrorLogMenuId)
    {
        showErrorLogWindow();
        return;
    }

    if (menuItemID == clearErrorLogMenuId)
        clear();
}

void ErrorLog::showErrorLogWindow()
{
    if (errorLogWindow_ == nullptr)
        errorLogWindow_ = std::make_unique<ErrorLogWindow>();

    errorLogWindow_->setVisible (true);
    errorLogWindow_->toFront (true);
}
