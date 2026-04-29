#include "MainComponent.h"

namespace
{
constexpr float kTwoPi = juce::MathConstants<float>::twoPi;

void drawInstructions (juce::Graphics& g, juce::Rectangle<int> area)
{
    juce::GlyphArrangement glyphs;
    glyphs.addJustifiedText (juce::Font (14.0f),
                             "Ableton Link / Transport: Route MIDI Clock to this port (Mac: IAC or aggregate). "
                             "Beeps sync to quarter notes while transport plays.",
                             area.toFloat(), juce::Justification::topLeft);
    glyphs.draw (g);
}
} // namespace

//==============================================================================
MainComponent::MainComponent()
{
    midiInputLabel.attachToComponent (&midiInputBox, true);

    midiInputBox.onChange = [this]
    {
        openMidiInput (midiInputBox.getSelectedId() - 1);
    };

    refreshMidiInputList();

    if (! juce::MidiInput::getAvailableDevices().isEmpty())
    {
        midiInputBox.setSelectedId (1, juce::dontSendNotification);
        openMidiInput (0);
    }

    statusLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (midiInputBox);
    addAndMakeVisible (statusLabel);

    setSize (520, 200);

    if (juce::RuntimePermissions::isRequired (juce::RuntimePermissions::recordAudio)
        && ! juce::RuntimePermissions::isGranted (juce::RuntimePermissions::recordAudio))
    {
        juce::RuntimePermissions::request (juce::RuntimePermissions::recordAudio,
                                           [&] (bool granted) { setAudioChannels (0, granted ? 2 : 0); });
    }
    else
        setAudioChannels (0, 2);
}

MainComponent::~MainComponent()
{
    shutdownAudio();

    if (midiInput != nullptr)
        midiInput->stop();

    midiInput.reset();
}

//==============================================================================
bool MainComponent::isMidiStart (const juce::MidiMessage& m)
{
    return m.getRawDataSize() >= 1 && m.getRawData()[0] == 0xfa;
}

bool MainComponent::isMidiContinue (const juce::MidiMessage& m)
{
    return m.getRawDataSize() >= 1 && m.getRawData()[0] == 0xfb;
}

bool MainComponent::isMidiStop (const juce::MidiMessage& m)
{
    return m.getRawDataSize() >= 1 && m.getRawData()[0] == 0xfc;
}

void MainComponent::handleIncomingMidiMessage (juce::MidiInput*, const juce::MidiMessage& message)
{
    if (message.isMidiClock())
    {
        if (! transportRunning.load (std::memory_order_acquire))
            return;

        if (++midiClockTickInBeat >= 24)
        {
            midiClockTickInBeat = 0;
            beepPending.store (true, std::memory_order_release);
        }

        return;
    }

    if (isMidiStart (message))
    {
        midiClockTickInBeat = 0;
        transportRunning.store (true, std::memory_order_release);
        return;
    }

    if (isMidiContinue (message))
    {
        transportRunning.store (true, std::memory_order_release);
        return;
    }

    if (isMidiStop (message))
    {
        transportRunning.store (false, std::memory_order_release);
        return;
    }
}

void MainComponent::refreshMidiInputList()
{
    midiInputBox.clear();

    auto devices = juce::MidiInput::getAvailableDevices();

    for (int i = 0; i < devices.size(); ++i)
        midiInputBox.addItem (devices[(size_t) i].name, i + 1);

    if (devices.isEmpty())
        midiInputBox.addItem ("(no MIDI inputs)", 1);

    repaint();
}

void MainComponent::openMidiInput (int deviceIndex)
{
    if (midiInput != nullptr)
    {
        midiInput->stop();
        midiInput.reset();
    }

    auto devices = juce::MidiInput::getAvailableDevices();

    if (deviceIndex < 0 || deviceIndex >= devices.size())
    {
        statusLabel.setText ("No MIDI input device.", juce::dontSendNotification);
        return;
    }

    midiInput = juce::MidiInput::openDevice (devices[(size_t) deviceIndex].identifier, this);

    if (midiInput == nullptr)
    {
        statusLabel.setText ("Could not open MIDI input.", juce::dontSendNotification);
        return;
    }

    midiInput->start();
    statusLabel.setText ("Listening: " + devices[(size_t) deviceIndex].name, juce::dontSendNotification);
}

//==============================================================================
void MainComponent::prepareToPlay (int, double sampleRate)
{
    currentSampleRate = sampleRate;
    totalBeepSamples = juce::jmax (1, (int) (0.035 * sampleRate));
    beepSamplesRemaining = 0;
    beepPhase = 0.0f;
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    bufferToFill.clearActiveBufferRegion();

    auto* buffer = bufferToFill.buffer;

    if (buffer == nullptr || buffer->getNumChannels() < 1)
        return;

    if (beepPending.exchange (false))
    {
        beepSamplesRemaining = totalBeepSamples;
        beepPhase = 0.0f;
    }

    if (beepSamplesRemaining <= 0)
        return;

    const int start = bufferToFill.startSample;
    const int numSamples = bufferToFill.numSamples;
    const int numCh = buffer->getNumChannels();
    const float inc = kTwoPi * beepFrequencyHz / (float) currentSampleRate;

    for (int i = 0; i < numSamples && beepSamplesRemaining > 0; ++i)
    {
        const float env = (float) beepSamplesRemaining / (float) totalBeepSamples;
        const float sample = env * std::sin ((double) beepPhase) * 0.25f;
        beepPhase += inc;

        while (beepPhase >= kTwoPi)
            beepPhase -= kTwoPi;

        const int pos = start + i;

        for (int ch = 0; ch < numCh; ++ch)
            buffer->addSample (ch, pos, sample);

        --beepSamplesRemaining;
    }
}

void MainComponent::releaseResources() {}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    drawInstructions (g, juce::Rectangle<int> { 14, 100, getWidth() - 28, 80 });
}

void MainComponent::resized()
{
    auto r = getLocalBounds().reduced (12);
    midiInputBox.setBounds (r.removeFromTop (28).withWidth (juce::jmin (360, r.getWidth())));
    statusLabel.setBounds (r.withTrimmedTop (4).removeFromTop (28));
}
