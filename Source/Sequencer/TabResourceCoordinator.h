#pragma once

#include <JuceHeader.h>

#include <map>

/** Tracks unique file assignments across tabs. */
class TabResourceCoordinator
{
public:
    bool canAssignFile (int tabId, const juce::String& filePath) const;

    void registerFile (int tabId, const juce::String& filePath);
    void unregisterFile (int tabId);
    void releaseTab (int tabId);

    juce::String getRegisteredFile (int tabId) const;

private:
    bool isPathUsedByOtherTab (int tabId, const juce::String& filePath) const;

    mutable juce::CriticalSection lock_;
    std::map<int, juce::String> tabFiles_;
};
