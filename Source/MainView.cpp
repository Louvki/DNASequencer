#include "MainView.h"

namespace
{
void drawInstructions (juce::Graphics& g, juce::Rectangle<int> area)
{
    g.setFont (juce::FontOptions().withHeight (14.0f));
    g.drawFittedText ("Ableton Link / Transport: Route MIDI Clock to this port (Mac: IAC or aggregate). "
                      "Beeps sync to quarter notes while transport plays.",
                      area,
                      juce::Justification::topLeft,
                      5);
}
} // namespace

MainView::MainView()
{
    midiInputLabel.attachToComponent (&midiInputBox, true);
    statusLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (midiInputBox);
    addAndMakeVisible (statusLabel);
}

void MainView::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    drawInstructions (g, juce::Rectangle<int> { 14, 100, getWidth() - 28, 80 });
}

void MainView::resized()
{
    auto r = getLocalBounds().reduced (12);
    midiInputBox.setBounds (r.removeFromTop (28).withWidth (juce::jmin (360, r.getWidth())));
    statusLabel.setBounds (r.withTrimmedTop (4).removeFromTop (28));
}
