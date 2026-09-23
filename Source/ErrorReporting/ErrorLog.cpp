#include "ErrorReporting/ErrorLog.h"

ErrorLog& ErrorLog::getInstance()
{
    static ErrorLog instance;
    return instance;
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
