#include "MainComponent.h"

namespace
{
constexpr float kTwoPi = juce::MathConstants<float>::twoPi;
} // namespace

//==============================================================================
/** Builds the UI, fills the MIDI device list and connects the default port, sizes the window,
    and starts audio via `setupAudioOutputs`. */
MainComponent::MainComponent()
    : view (*this)
{
    // Initializes the GUI
    addAndMakeVisible (view); 
    setSize (520, 200);

    initialiseMidiInputs();

    setupAudioOutputs();
}

/** Destructor Stops audio I/O and closes any open MIDI input device. */
MainComponent::~MainComponent()
{
    shutdownAudio();
    if (midiInput != nullptr) midiInput->stop();
    midiInput.reset();
}

/** Enables stereo playback; requests mic-related permission when the OS ties output to it (e.g. Android). */
void MainComponent::setupAudioOutputs()
{
    if (juce::RuntimePermissions::isRequired (juce::RuntimePermissions::recordAudio)
        && ! juce::RuntimePermissions::isGranted (juce::RuntimePermissions::recordAudio))
    {
        juce::RuntimePermissions::request (juce::RuntimePermissions::recordAudio,
                                           [&] (bool granted) { setAudioChannels (0, granted ? 2 : 0); });
    }
    else
        setAudioChannels (0, 2);
}



//==============================================================================
/** MIDI callback: counts clock ticks into quarter notes while transport runs; toggles transportRunning on Start / Continue / Stop. */
void MainComponent::handleIncomingMidiMessage (juce::MidiInput*, const juce::MidiMessage& message)
{
    // MIDI clock: 24 ticks per quarter note; each full beat queues one audible click on the audio thread.
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

    bool isMidiStart = m.getRawDataSize() >= 1 && m.getRawData()[0] == 0xfa;
    if (isMidiStart (message))
    {
        midiClockTickInBeat = 0;
        transportRunning.store (true, std::memory_order_release);
        return;
    }

    bool isMidiContinue = m.getRawDataSize() >= 1 && m.getRawData()[0] == 0xfb;
    if (isMidiContinue (message))
    {
        transportRunning.store (true, std::memory_order_release);
        return;
    }

    bool isMidiStop = m.getRawDataSize() >= 1 && m.getRawData()[0] == 0xfc;
    if (isMidiStop (message))
    {
        transportRunning.store (false, std::memory_order_release);
        return;
    }
}

//==============================================================================
// Selecting MIDI Inputs 
/** Populates the MIDI device dropdown and selects/opens the first device when the system reports any inputs. */
void MainComponent::initialiseMidiInputs()
{
    populateMidiInputDeviceList();
    selectFirstMidiInputIfAvailable();
}

/** Rebuilds combo items from `MidiInput::getAvailableDevices()`, or a single placeholder row if none exist. */
void MainComponent::populateMidiInputDeviceList()
{
    auto& midiInputBox = view.getMidiInputBox();

    midiInputBox.clear (juce::dontSendNotification);

    auto devices = juce::MidiInput::getAvailableDevices();

    for (int i = 0; i < devices.size(); ++i)
        midiInputBox.addItem (devices[(size_t) i].name, i + 1);

    if (devices.isEmpty())
        midiInputBox.addItem ("(no MIDI inputs)", 1);

    view.repaint();
}

/** Selects combo entry 1 without firing notifications and connects to device index 0 (no-op when no devices). */
void MainComponent::selectFirstMidiInputIfAvailable()
{
    if (juce::MidiInput::getAvailableDevices().isEmpty())
        return;

    auto& midiInputBox = view.getMidiInputBox();
    midiInputBox.setSelectedId (1, juce::dontSendNotification);
    selectMidiInputDevice (0);
}

/** Invoked when the user picks a MIDI port in the combo; maps combo IDs (1-based) to device indices and opens that input. */
void MainComponent::midiInputSelectionChangedFromView()
{
    const int comboItemId = view.getMidiInputBox().getSelectedId();
    selectMidiInputDevice (comboItemId - 1);
}

/** Closes any previous MIDI input, opens `deviceIndex` from the current device list, updates status text, or reports failure. */
void MainComponent::selectMidiInputDevice (int deviceIndex)
{
    if (midiInput != nullptr)
    {
        midiInput->stop();
        midiInput.reset();
    }

    auto devices = juce::MidiInput::getAvailableDevices();

    auto& statusLabel = view.getStatusLabel();

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
/** Stores sample rate and computes how many samples make one beat-click envelope (~35 ms). */
void MainComponent::prepareToPlay (int, double sampleRate)
{
    currentSampleRate = sampleRate;
    // Short sine burst (~35 ms) at output sample rate for the beat click.
    totalBeepSamples = juce::jmax (1, (int) (0.035 * sampleRate));
    beepSamplesRemaining = 0;
    beepPhase = 0.0f;
}

/** Outputs silence unless a beat click was queued from MIDI clock; then mixes a decaying sine burst onto all channels. */
void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    bufferToFill.clearActiveBufferRegion();

    auto* buffer = bufferToFill.buffer;

    if (buffer == nullptr || buffer->getNumChannels() < 1)
        return;

    // Atomically consume pending beep flag from MIDI thread; start envelope on audio thread.
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
        // Linear decay envelope + sine; add to all channels.
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

/** No teardown needed beyond base class handling. */
void MainComponent::releaseResources() {}

//==============================================================================
/** Gives the entire bounds to `MainView`. */
void MainComponent::resized()
{
    view.setBounds (getLocalBounds());
}
