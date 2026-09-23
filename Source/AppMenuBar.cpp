#include "AppMenuBar.h"

#include "ErrorReporting/ErrorLog.h"
#include "Sequencer/MidiClockInputService.h"
#include "Sequencer/MidiInputSelectorComponent.h"

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

class SettingsViewComponent : public juce::Component
{
public:
    explicit SettingsViewComponent (MidiClockInputService& service)
        : clockInputSelector (service)
    {
        addAndMakeVisible (clockInputSelector);
    }

    void resized() override
    {
        clockInputSelector.setBounds (getLocalBounds().reduced (12));
    }

private:
    MidiInputSelectorComponent clockInputSelector;
};

class SettingsWindow final : public juce::DocumentWindow
{
public:
    explicit SettingsWindow (MidiClockInputService& service)
        : juce::DocumentWindow ("Settings",
                                juce::Desktop::getInstance().getDefaultLookAndFeel()
                                    .findColour (juce::ResizableWindow::backgroundColourId),
                                juce::DocumentWindow::closeButton)
    {
        setUsingNativeTitleBar (true);
        setResizable (true, true);
        setContentOwned (new SettingsViewComponent (service), true);
        centreWithSize (420, 120);
    }

    void closeButtonPressed() override
    {
        setVisible (false);
    }
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

AppMenuBar& AppMenuBar::getInstance()
{
    static AppMenuBar instance;
    return instance;
}

void AppMenuBar::installForMac()
{
   #if JUCE_MAC
    juce::MenuBarModel::setMacMainMenu (this);
   #endif
}

void AppMenuBar::uninstallForMac()
{
    settingsWindow_ = nullptr;
    errorLogWindow_ = nullptr;

   #if JUCE_MAC
    juce::MenuBarModel::setMacMainMenu (nullptr);
   #endif
}

void AppMenuBar::setClockInputService (MidiClockInputService* service) noexcept
{
    clockInputService = service;
}

juce::StringArray AppMenuBar::getMenuBarNames()
{
    return { "Settings", "Window" };
}

juce::PopupMenu AppMenuBar::getMenuForIndex (int menuIndex, const juce::String& menuName)
{
    juce::ignoreUnused (menuName);

    juce::PopupMenu menu;

    if (menuIndex == 0)
        menu.addItem (showSettingsMenuId, "Show Settings...");
    else if (menuIndex == 1)
    {
        menu.addItem (showErrorLogMenuId, "Show Error Log");
        menu.addItem (clearErrorLogMenuId, "Clear Error Log");
    }

    return menu;
}

void AppMenuBar::menuItemSelected (int menuItemID, int topLevelMenuIndex)
{
    juce::ignoreUnused (topLevelMenuIndex);

    if (menuItemID == showSettingsMenuId)
    {
        showSettingsWindow();
        return;
    }

    if (menuItemID == showErrorLogMenuId)
    {
        showErrorLogWindow();
        return;
    }

    if (menuItemID == clearErrorLogMenuId)
        ErrorLog::getInstance().clear();
}

void AppMenuBar::showSettingsWindow()
{
    if (clockInputService == nullptr)
        return;

    if (settingsWindow_ == nullptr)
        settingsWindow_ = std::make_unique<SettingsWindow> (*clockInputService);

    settingsWindow_->setVisible (true);
    settingsWindow_->toFront (true);
}

void AppMenuBar::showErrorLogWindow()
{
    if (errorLogWindow_ == nullptr)
        errorLogWindow_ = std::make_unique<ErrorLogWindow>();

    errorLogWindow_->setVisible (true);
    errorLogWindow_->toFront (true);
}
