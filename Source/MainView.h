#pragma once

#include <JuceHeader.h>

class MainComponent;

class MainView : public juce::Component
{
public:
    /** Arranges MIDI picker and status labels; combo changes notify `owner`. */
    explicit MainView (MainComponent& owner);

    /** MIDI device picker; changes invoke `MainComponent::midiInputSelectionChangedFromView`. */
    juce::ComboBox& getMidiInputBox() noexcept { return midiInputBox; }
    /** Shows whether an input opened successfully and which port name is active. */
    juce::Label& getStatusLabel() noexcept { return statusLabel; }

    /** Background fill plus hint text about MIDI clock routing. */
    void paint (juce::Graphics& g) override;
    /** Places combo and status row under top margin. */
    void resized() override;

private:
    MainComponent& owner;

    juce::ComboBox midiInputBox;
    juce::Label midiInputLabel { {}, "MIDI Input" };
    juce::Label statusLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainView)
};
