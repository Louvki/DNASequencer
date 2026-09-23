#pragma once

#include <JuceHeader.h>

#include <functional>
#include <memory>

class MidiClockInputService;

/** macOS menu bar: Settings and Window menus. */
class AppMenuBar : private juce::MenuBarModel
{
public:
    static AppMenuBar& getInstance();

    void installForMac();
    void uninstallForMac();

    void setClockInputService (MidiClockInputService* service) noexcept;

private:
    AppMenuBar() = default;

    juce::StringArray getMenuBarNames() override;
    juce::PopupMenu getMenuForIndex (int menuIndex, const juce::String& menuName) override;
    void menuItemSelected (int menuItemID, int topLevelMenuIndex) override;

    void showSettingsWindow();
    void showErrorLogWindow();

    static constexpr int showSettingsMenuId = 1;
    static constexpr int showErrorLogMenuId = 2;
    static constexpr int clearErrorLogMenuId = 3;

    MidiClockInputService* clockInputService = nullptr;
    std::unique_ptr<juce::DocumentWindow> settingsWindow_;
    std::unique_ptr<juce::DocumentWindow> errorLogWindow_;
};
