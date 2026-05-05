#include "MainComponent.h"
#include "ErrorLog.h"

//==============================================================================
/** Builds the UI, fills the MIDI device list and connects the default port, sizes the window,
    and opens the audio device with zero I/O channels (required by `AudioAppComponent`). */
MainComponent::MainComponent()
    : view (*this)
{
    // Initializes the GUI
    addAndMakeVisible (view);
    setSize (520, 300);

    initialiseMidiInputs();
    setAudioChannels (0, 0);
}

/** Destructor: Stops audio I/O and closes any open MIDI input device. */
MainComponent::~MainComponent()
{
    shutdownAudio();
    if (midiInput != nullptr) midiInput->stop();
    midiInput.reset();
}

//==============================================================================
/** MIDI callback: counts clock ticks into quarter notes (24 PPQN) while transport runs;
    toggles `transportRunning` on Start (0xFA), Continue (0xFB), and Stop (0xFC). */
void MainComponent::handleIncomingMidiMessage (juce::MidiInput*, const juce::MidiMessage& message)
{
    if (message.isMidiClock())
    {
        if (! transportRunning.load (std::memory_order_acquire))
            return;

        if (++midiClockTickInBeat >= 24)
            midiClockTickInBeat = 0;

        return;
    }

    
    const bool isMidiStart = message.getRawDataSize() >= 1 && message.getRawData()[0] == 0xfa;
    if (isMidiStart)
    {
        midiClockTickInBeat = 0;
        transportRunning.store (true, std::memory_order_release);
        return;
    }
    
    const bool isMidiContinue = message.getRawDataSize() >= 1 && message.getRawData()[0] == 0xfb;
    if (isMidiContinue)
    {
        transportRunning.store (true, std::memory_order_release);
        return;
    }
    
    const bool isMidiStop = message.getRawDataSize() >= 1 && message.getRawData()[0] == 0xfc;
    if (isMidiStop)
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
        const auto error = juce::String ("No MIDI input device.");
        statusLabel.setText (error, juce::dontSendNotification);
        ErrorLog::getInstance().addError ("MIDI", error);
        return;
    }

    midiInput = juce::MidiInput::openDevice (devices[(size_t) deviceIndex].identifier, this);

    if (midiInput == nullptr)
    {
        const auto error = juce::String ("Could not open MIDI input.");
        statusLabel.setText (error, juce::dontSendNotification);
        ErrorLog::getInstance().addError ("MIDI", error);
        return;
    }

    midiInput->start();
    statusLabel.setText ("Listening: " + devices[(size_t) deviceIndex].name, juce::dontSendNotification);
}

//==============================================================================
/** `AudioAppComponent` hook before I/O runs; no audio output in this app — parameters ignored. */
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    juce::ignoreUnused (samplesPerBlockExpected, sampleRate);
}

/** Clears each output block; no samples are synthesized (MIDI clock handling is on the MIDI thread). */
void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    bufferToFill.clearActiveBufferRegion();
}

/** No teardown needed beyond base class handling. */
void MainComponent::releaseResources() {}

//==============================================================================
/** Gives the entire bounds to `MainView`. */
void MainComponent::resized()
{
    view.setBounds (getLocalBounds());
}
