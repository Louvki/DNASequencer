#include "Sequencer/AminoAcidPlaybackSettingsComponent.h"

#include "DataStructures/MidiScales.h"

namespace
{
constexpr int kLabelWidth = 110;
constexpr int kRowHeight = 28;
constexpr int kSectionLabelHeight = 16;
constexpr int kShareLabelWidth = 48;
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

constexpr std::array<const char*, 5> kChordTypeLabels { "Triads", "7ths", "9ths", "11ths", "13ths" };

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

void setupPercentSlider (juce::Slider& slider)
{
    slider.setSliderStyle (juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 44, 20);
    slider.setRange (0, 100, 1);
    slider.textFromValueFunction = [] (double value)
    {
        return juce::String ((int) value) + " %";
    };
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

void layoutLabelledRowWithShare (juce::Label& label,
                                 juce::Slider& slider,
                                 juce::Label& shareLabel,
                                 juce::Rectangle<int> row)
{
    shareLabel.setBounds (row.removeFromRight (kShareLabelWidth));
    row.removeFromRight (4);
    label.setBounds (row.removeFromLeft (kLabelWidth));
    slider.setBounds (row);
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

AminoAcidPlaybackSettingsComponent::AminoAcidPlaybackSettingsComponent (AminoAcidSequencePlayer& player)
    : sequencePlayer (player),
      clockDivisionSelector (player)
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

    setupLabel (chordChanceLabel, "Chord mix");
    addAndMakeVisible (chordChanceLabel);
    addAndMakeVisible (chordChanceSlider);
    setupPercentSlider (chordChanceSlider);
    chordChanceSlider.setValue (0, juce::dontSendNotification);
    chordChanceSlider.onValueChange = [this]
    {
        updateChordControlAppearance();
        applySettingsToPlayer();
    };

    chordTypeMixLabel.setText ("Type mix", juce::dontSendNotification);
    chordTypeMixLabel.setJustificationType (juce::Justification::centredLeft);
    chordTypeMixLabel.setFont (juce::FontOptions (11.0f));
    chordTypeMixLabel.setColour (juce::Label::textColourId, juce::Colours::grey);
    addAndMakeVisible (chordTypeMixLabel);

    for (size_t i = 0; i < chordTypeWeightControls.size(); ++i)
    {
        auto& control = chordTypeWeightControls[i];
        setupLabel (control.label, kChordTypeLabels[i]);
        addAndMakeVisible (control.label);
        addAndMakeVisible (control.slider);
        setupPercentSlider (control.slider);
        control.slider.setValue (i == 0 ? 100 : 0, juce::dontSendNotification);
        control.slider.onValueChange = [this]
        {
            updateChordTypeShareLabels();
            applySettingsToPlayer();
        };

        control.shareLabel.setJustificationType (juce::Justification::centredRight);
        control.shareLabel.setFont (juce::FontOptions (11.0f));
        control.shareLabel.setColour (juce::Label::textColourId, juce::Colours::grey);
        addAndMakeVisible (control.shareLabel);
    }

    setupLabel (chordStrumLabel, "Strum");
    addAndMakeVisible (chordStrumLabel);
    addAndMakeVisible (chordStrumSlider);
    chordStrumSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    chordStrumSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 44, 20);
    chordStrumSlider.setRange (0, 200, 1);
    chordStrumSlider.setValue (0, juce::dontSendNotification);
    chordStrumSlider.setTextValueSuffix (" ms");
    chordStrumSlider.onValueChange = [this] { applySettingsToPlayer(); };

    setupLabel (chordVelocityLabel, "Velocity");
    addAndMakeVisible (chordVelocityLabel);
    addAndMakeVisible (chordVelocitySlider);
    chordVelocitySlider.setSliderStyle (juce::Slider::LinearHorizontal);
    chordVelocitySlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 44, 20);
    chordVelocitySlider.setRange (0, 64, 1);
    chordVelocitySlider.setValue (0, juce::dontSendNotification);
    chordVelocitySlider.onValueChange = [this] { applySettingsToPlayer(); };

    updateDurationControlAppearance();
    updateChordControlAppearance();
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
    layoutLabelledRow (chordChanceLabel, chordChanceSlider, r.removeFromTop (kRowHeight));

    chordTypeMixLabel.setBounds (r.removeFromTop (kSectionLabelHeight));

    for (auto& control : chordTypeWeightControls)
        layoutLabelledRowWithShare (control.label, control.slider, control.shareLabel, r.removeFromTop (kRowHeight));

    layoutLabelledRow (chordStrumLabel, chordStrumSlider, r.removeFromTop (kRowHeight));
    layoutLabelledRow (chordVelocityLabel, chordVelocitySlider, r.removeFromTop (kRowHeight));
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

void AminoAcidPlaybackSettingsComponent::updateChordControlAppearance()
{
    const bool chordsActive = chordChanceSlider.getValue() > 0.0;
    const float alpha = chordsActive ? 1.0f : kDisabledAlpha;

    chordTypeMixLabel.setAlpha (alpha);

    for (auto& control : chordTypeWeightControls)
    {
        control.slider.setEnabled (chordsActive);
        control.label.setAlpha (alpha);
        control.shareLabel.setAlpha (alpha);
    }

    chordStrumSlider.setEnabled (chordsActive);
    chordStrumLabel.setAlpha (alpha);
    chordVelocitySlider.setEnabled (chordsActive);
    chordVelocityLabel.setAlpha (alpha);

    updateChordTypeShareLabels();
}

void AminoAcidPlaybackSettingsComponent::updateChordTypeShareLabels()
{
    int totalWeight = 0;

    for (const auto& control : chordTypeWeightControls)
        totalWeight += juce::jmax (0, (int) control.slider.getValue());

    const bool showShares = chordChanceSlider.getValue() > 0.0 && totalWeight > 0;

    for (auto& control : chordTypeWeightControls)
    {
        if (! showShares)
        {
            control.shareLabel.setText ({}, juce::dontSendNotification);
            continue;
        }

        const auto weight = juce::jmax (0, (int) control.slider.getValue());

        if (weight <= 0)
        {
            control.shareLabel.setText ("(0%)", juce::dontSendNotification);
            continue;
        }

        const auto sharePercent = juce::roundToInt (100.0 * (double) weight / (double) totalWeight);
        control.shareLabel.setText ("(" + juce::String (sharePercent) + "%)", juce::dontSendNotification);
    }
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
    sequencePlayer.setChordChancePercent ((int) chordChanceSlider.getValue());
    sequencePlayer.setChordStrumMaxMs ((int) chordStrumSlider.getValue());
    sequencePlayer.setChordVelocityRange ((int) chordVelocitySlider.getValue());

    for (size_t i = 0; i < chordTypeWeightControls.size(); ++i)
        sequencePlayer.setChordTypeWeight (static_cast<dna::ChordType> (i),
                                           (int) chordTypeWeightControls[i].slider.getValue());
}
 