#include "MainView.h"

namespace
{
/** Draws the usage hint below the controls. */
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

MainView::MainView (juce::MidiInputCallback& midiCallbackTarget)
    : midiInputSelector (midiCallbackTarget)
{
    addAndMakeVisible (midiInputSelector);
    addAndMakeVisible (sequenceFileLoader);
}

/** Fills the background and draws routing/sync instructions in the lower area. */
void MainView::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    drawInstructions (g,
                      juce::Rectangle<int> { 14, sequenceFileLoader.getBottom() + 10,
                                             getWidth() - 28, juce::jmax (40, getHeight() - sequenceFileLoader.getBottom() - 22) });
}

/** Positions the MIDI combo and status label at the top with standard margins. */
void MainView::resized()
{
    auto r = getLocalBounds().reduced (12);
    midiInputSelector.setBounds (r.removeFromTop (60));
    sequenceFileLoader.setBounds (r.withTrimmedTop (6).removeFromTop (64));
}
