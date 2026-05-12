#pragma once

#include <JuceHeader.h>

#include "MidiInputSelectorComponent.h"
#include "SequenceFileLoaderComponent.h"

class MainView : public juce::Component
{
public:
    explicit MainView (juce::MidiInputCallback& midiCallbackTarget);

    SequenceFileLoaderComponent& getSequenceFileLoader() noexcept { return sequenceFileLoader; }

    /** Background fill plus hint text about MIDI clock routing. */
    void paint (juce::Graphics& g) override;
    /** Places combo and status row under top margin. */
    void resized() override;

private:
    MidiInputSelectorComponent midiInputSelector;

    SequenceFileLoaderComponent sequenceFileLoader;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainView)
};
