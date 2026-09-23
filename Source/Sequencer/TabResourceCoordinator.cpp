#include "Sequencer/TabResourceCoordinator.h"

bool TabResourceCoordinator::canAssignFile (int tabId, const juce::String& filePath) const
{
    if (filePath.isEmpty())
        return true;

    const juce::ScopedLock sl (lock_);
    return ! isPathUsedByOtherTab (tabId, filePath);
}

void TabResourceCoordinator::registerFile (int tabId, const juce::String& filePath)
{
    const juce::ScopedLock sl (lock_);

    if (filePath.isEmpty())
        tabFiles_.erase (tabId);
    else
        tabFiles_[tabId] = filePath;
}

void TabResourceCoordinator::unregisterFile (int tabId)
{
    const juce::ScopedLock sl (lock_);
    tabFiles_.erase (tabId);
}

void TabResourceCoordinator::releaseTab (int tabId)
{
    const juce::ScopedLock sl (lock_);
    tabFiles_.erase (tabId);
}

juce::String TabResourceCoordinator::getRegisteredFile (int tabId) const
{
    const juce::ScopedLock sl (lock_);
    const auto it = tabFiles_.find (tabId);
    return it != tabFiles_.end() ? it->second : juce::String();
}

bool TabResourceCoordinator::isPathUsedByOtherTab (int tabId, const juce::String& filePath) const
{
    for (const auto& entry : tabFiles_)
    {
        if (entry.first != tabId && entry.second == filePath)
            return true;
    }

    return false;
}
