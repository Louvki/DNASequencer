#pragma once

#include <JuceHeader.h>

#include <memory>

class ErrorLog : private juce::MenuBarModel
{
public:
    static ErrorLog& getInstance();
    void installForMac();
    void uninstallForMac();

    void addError (const juce::String& source, const juce::String& message);
    juce::String getLogText() const;
    void clear();

private:
    ErrorLog() = default;

    juce::StringArray getMenuBarNames() override;
    juce::PopupMenu getMenuForIndex (int menuIndex, const juce::String& menuName) override;
    void menuItemSelected (int menuItemID, int topLevelMenuIndex) override;
    void showErrorLogWindow();

    static constexpr int showErrorLogMenuId = 1;
    static constexpr int clearErrorLogMenuId = 2;

    mutable juce::CriticalSection lock_;
    juce::StringArray entries_;
    std::unique_ptr<juce::DocumentWindow> errorLogWindow_;
};
