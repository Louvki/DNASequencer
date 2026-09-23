#pragma once

#include <JuceHeader.h>

class ErrorLog
{
public:
    static ErrorLog& getInstance();

    void addError (const juce::String& source, const juce::String& message);
    juce::String getLogText() const;
    void clear();

private:
    ErrorLog() = default;

    mutable juce::CriticalSection lock_;
    juce::StringArray entries_;
};
