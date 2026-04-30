#pragma once

#include <JuceHeader.h>

class MainView : public juce::Component
{
public:
    MainView();

    juce::ComboBox& getMidiInputBox() noexcept { return midiInputBox; }
    juce::Label& getStatusLabel() noexcept { return statusLabel; }

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    juce::ComboBox midiInputBox;
    juce::Label midiInputLabel { {}, "MIDI Input" };
    juce::Label statusLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainView)
};
