#include "Sequencer/AminoAcidPlaybackSettingsComponent.h"

#include "DataStructures/MidiScales.h"

namespace
{
constexpr int kLabelWidth = 110;
constexpr int kRowHeight = 28;
constexpr int kTitleHeight = 14;
constexpr int kKnobSize = 72;
constexpr int kValueHeight = 14;
constexpr int kLabelOverlap = 10;
constexpr int kTitleGap = 2;
constexpr int kScaleBoxWidth = 180;
constexpr int kDivisionBoxWidth = 100;
constexpr int kSliderRowBottomMargin = 24;
constexpr int kDropdownRowBottomMargin = 12;

constexpr int kRotaryColumnHeight = kTitleHeight + kKnobSize + kValueHeight - (2 * kLabelOverlap) + kTitleGap;
const auto kDurationFillColour = juce::Colour (0xffA1EF8B);
constexpr float kDisabledAlpha = 0.5f;

void setupRotarySlider (juce::Slider& slider)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                                juce::MathConstants<float>::pi * 2.8f,
                                true);
    slider.setColour (juce::Slider::thumbColourId, juce::Colours::transparentBlack);
    slider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xffA1EF8B));
}

void setupTitleLabel (juce::Label& label, const juce::String& text)
{
    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::FontOptions (12.0f));
}

void setupValueLabel (juce::Label& label)
{
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::FontOptions (12.0f));
}

void layoutLabelledRow (juce::Label& label, juce::Component& control, juce::Rectangle<int> row)
{
    label.setBounds (row.removeFromLeft (kLabelWidth));
    control.setBounds (row);
}

void layoutRotaryColumn (juce::Label& title,
                         juce::Slider& slider,
                         juce::Label& value,
                         juce::Rectangle<int> area)
{
    const auto knob = area.withSizeKeepingCentre (kKnobSize, kKnobSize);
    slider.setBounds (knob);

    title.setBounds (area.getX(), knob.getY() - kTitleHeight + kLabelOverlap - kTitleGap, area.getWidth(), kTitleHeight);
    value.setBounds (area.getX(), knob.getBottom() - kLabelOverlap, area.getWidth(), kValueHeight);
}
} // namespace

AminoAcidPlaybackSettingsComponent::AminoAcidPlaybackSettingsComponent (AminoAcidSequencePlayer& player,
                                                                        MidiClockService& clockService)
    : sequencePlayer (player),
      clockDivisionSelector (clockService)
{
    auto setupLabel = [] (juce::Label& label, const juce::String& text)
    {
        label.setText (text, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centredLeft);
    };

    setupTitleLabel (rootNoteLabel, "Root note");
    addAndMakeVisible (rootNoteLabel);
    addAndMakeVisible (rootNoteSlider);
    setupRotarySlider (rootNoteSlider);
    addAndMakeVisible (rootNoteValueLabel);
    setupValueLabel (rootNoteValueLabel);
    rootNoteSlider.setRange (0, 127, 1);
    rootNoteSlider.setValue (60, juce::dontSendNotification);
    rootNoteSlider.textFromValueFunction = [] (double value)
    {
        return juce::MidiMessage::getMidiNoteName ((int) value, true, true, 3);
    };
    rootNoteSlider.onValueChange = [this]
    {
        updateSliderValueLabels();
        applySettingsToPlayer();
    };

    addAndMakeVisible (scaleBox);
    scaleBox.onChange = [this] { applySettingsToPlayer(); };
    populateScaleList();

    addAndMakeVisible (clockDivisionSelector);

    setupTitleLabel (notePoolLabel, "Note pool");
    addAndMakeVisible (notePoolLabel);
    addAndMakeVisible (notePoolSlider);
    setupRotarySlider (notePoolSlider);
    addAndMakeVisible (notePoolValueLabel);
    setupValueLabel (notePoolValueLabel);
    notePoolSlider.setRange (1, 20, 1);
    notePoolSlider.setValue (20, juce::dontSendNotification);
    notePoolSlider.onValueChange = [this]
    {
        updateSliderValueLabels();
        applySettingsToPlayer();
    };

    setupTitleLabel (whitespaceLabel, "WSpeed");
    addAndMakeVisible (whitespaceLabel);
    addAndMakeVisible (whitespaceSlider);
    setupRotarySlider (whitespaceSlider);
    addAndMakeVisible (whitespaceValueLabel);
    setupValueLabel (whitespaceValueLabel);
    whitespaceSlider.setRange (1, 64, 1);
    whitespaceSlider.setValue (15, juce::dontSendNotification);
    whitespaceSlider.onValueChange = [this]
    {
        updateSliderValueLabels();
        applySettingsToPlayer();
    };

    setupTitleLabel (noteDurationLabel, "Duration");
    addAndMakeVisible (noteDurationLabel);
    addAndMakeVisible (noteDurationSlider);
    setupRotarySlider (noteDurationSlider);
    addAndMakeVisible (noteDurationValueLabel);
    setupValueLabel (noteDurationValueLabel);
    noteDurationSlider.setRange (1, 3000, 1);
    noteDurationSlider.setValue (100, juce::dontSendNotification);
    noteDurationSlider.textFromValueFunction = [] (double value)
    {
        return juce::String ((int) value) + " ms";
    };
    noteDurationSlider.onValueChange = [this]
    {
        updateSliderValueLabels();
        applySettingsToPlayer();
    };

    setupLabel (sustainLabel, "Sustain");
    addAndMakeVisible (sustainLabel);
    addAndMakeVisible (sustainToggle);
    sustainToggle.setToggleState (false, juce::dontSendNotification);
    sustainToggle.onClick = [this]
    {
        updateDurationControlAppearance();
        applySettingsToPlayer();
    };

    setupLabel (chordsLabel, "Chords");
    addAndMakeVisible (chordsLabel);
    addAndMakeVisible (chordsToggle);
    chordsToggle.setToggleState (false, juce::dontSendNotification);
    chordsToggle.onClick = [this] { applySettingsToPlayer(); };

    updateDurationControlAppearance();
    updateSliderValueLabels();
    applySettingsToPlayer();
}

void AminoAcidPlaybackSettingsComponent::resized()
{
    auto r = getLocalBounds();

    auto slidersRow = r.removeFromTop (kRotaryColumnHeight);
    const auto columnWidth = slidersRow.getWidth() / 4;

    layoutRotaryColumn (rootNoteLabel, rootNoteSlider, rootNoteValueLabel, slidersRow.removeFromLeft (columnWidth));
    layoutRotaryColumn (noteDurationLabel, noteDurationSlider, noteDurationValueLabel, slidersRow.removeFromLeft (columnWidth));
    layoutRotaryColumn (whitespaceLabel, whitespaceSlider, whitespaceValueLabel, slidersRow.removeFromLeft (columnWidth));
    layoutRotaryColumn (notePoolLabel, notePoolSlider, notePoolValueLabel, slidersRow);
    r.removeFromTop (kSliderRowBottomMargin);

    scaleBox.setBounds (r.removeFromTop (kRowHeight).removeFromLeft (kScaleBoxWidth));
    r.removeFromTop (kDropdownRowBottomMargin);
    clockDivisionSelector.setBounds (r.removeFromTop (kRowHeight).removeFromLeft (kDivisionBoxWidth));
    r.removeFromTop (kDropdownRowBottomMargin);
    layoutLabelledRow (sustainLabel, sustainToggle, r.removeFromTop (kRowHeight));
    layoutLabelledRow (chordsLabel, chordsToggle, r.removeFromTop (kRowHeight));
}

void AminoAcidPlaybackSettingsComponent::populateScaleList()
{
    scaleBox.clear (juce::dontSendNotification);

    const auto& scales = dna::getAllMidiScales();
    for (int i = 0; i < (int) scales.size(); ++i)
        scaleBox.addItem (dna::getMidiScaleLabel (scales[(size_t) i]), i + 1);

    const auto defaultIndex = (int) dna::MidiScale::majorIonian;
    scaleBox.setSelectedId (defaultIndex + 1, juce::dontSendNotification);
}

void AminoAcidPlaybackSettingsComponent::updateSliderValueLabels()
{
    auto setFromSlider = [] (juce::Slider& slider, juce::Label& label)
    {
        if (slider.textFromValueFunction != nullptr)
            label.setText (slider.textFromValueFunction (slider.getValue()), juce::dontSendNotification);
        else
            label.setText (juce::String ((int) slider.getValue()), juce::dontSendNotification);
    };

    setFromSlider (rootNoteSlider, rootNoteValueLabel);
    setFromSlider (notePoolSlider, notePoolValueLabel);
    setFromSlider (whitespaceSlider, whitespaceValueLabel);
    setFromSlider (noteDurationSlider, noteDurationValueLabel);
}

void AminoAcidPlaybackSettingsComponent::updateDurationControlAppearance()
{
    const bool enabled = ! sustainToggle.getToggleState();

    noteDurationSlider.setInterceptsMouseClicks (enabled, true);
    noteDurationSlider.setColour (juce::Slider::rotarySliderFillColourId,
                                  enabled ? kDurationFillColour
                                          : kDurationFillColour.withAlpha (kDisabledAlpha));
    noteDurationLabel.setAlpha (enabled ? 1.0f : kDisabledAlpha);
    noteDurationValueLabel.setAlpha (enabled ? 1.0f : kDisabledAlpha);
}

void AminoAcidPlaybackSettingsComponent::applySettingsToPlayer()
{
    sequencePlayer.setRootNote ((int) rootNoteSlider.getValue());

    const int scaleIndex = scaleBox.getSelectedId() - 1;
    const auto& scales = dna::getAllMidiScales();
    if (scaleIndex >= 0 && scaleIndex < (int) scales.size())
        sequencePlayer.setScale (scales[(size_t) scaleIndex]);

    sequencePlayer.setNotePoolSize ((int) notePoolSlider.getValue());
    sequencePlayer.setWhiteSpaceReadSpeed ((int) whitespaceSlider.getValue());
    sequencePlayer.setNoteDurationMs ((int) noteDurationSlider.getValue());
    sequencePlayer.setSustainEnabled (sustainToggle.getToggleState());
    sequencePlayer.setChordsEnabled (chordsToggle.getToggleState());
}
