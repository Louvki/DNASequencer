# DNASequencer — Mac MIDI Bus Setup

This app listens to **MIDI clock** from your DAW and sends **note output** on a separate MIDI port. On Mac, the usual way to connect Ableton (or another DAW) to the app is the built-in **IAC Driver** (Inter-Application Communication bus).

## Overview

```
Ableton Live  --(MIDI clock)-->  IAC Bus  --(MIDI input)-->  DNASequencer
DNASequencer  --(MIDI notes)-->  Your synth / instrument port
```

- **Input:** clock + transport from the Bus (selected in the app)
- **Output:** notes go to the first available MIDI output device on your system (not through the Bus)

---

## 1. Enable the IAC Driver

1. Open **Audio MIDI Setup**  
   (`Applications` → `Utilities` → **Audio MIDI Setup**)
2. Open **Window** → **Show MIDI Studio** (or press `Cmd+2`)
3. Double-click **IAC Driver**
4. Check **Device is online**
5. Under **Ports**, ensure at least one bus exists (default: **Bus 1**)  
   - If needed, click **Add Port** to create one
6. Close the window

You should now see **IAC Driver Bus 1** (or similar) in MIDI device lists.

---

## 2. Route MIDI clock from Ableton to the Bus

1. Open **Ableton Live**
2. Go to **Settings / Preferences** → **Link, Tempo & MIDI**
3. In the **MIDI** section, find **IAC Driver (Bus 1)** in the output column
4. Turn **Sync** **On** for that bus  
   (This sends MIDI clock and transport to the Bus.)
5. Leave **Track** off unless you also want to send note/CC data from tracks to the Bus

### Ableton track / session notes

- Start playback in Ableton to send clock ticks
- The app uses a **clock-driven** model: if clock is running and the app is not paused, it treats transport as active
- **MIDI Stop** from Ableton will stop sounding notes in the app

---

## 3. Configure DNASequencer

1. Launch **DNASequencer**
2. At the top, open the **MIDI Input** dropdown
3. Select **IAC Driver Bus 1** (or the bus you enabled)
4. Confirm the status line shows something like:  
   `Listening: IAC Driver Bus 1`
5. Click **Play** in the app (local transport)  
   - **Pause** ignores incoming clock  
   - **Reset** resets read position and auto-pauses
6. Load a DNA/FASTA file with **Select file**
7. Press **Play** again when ready

### Status LED / labels

| Status | Meaning |
|--------|---------|
| **Paused** | App is not processing clock |
| **MIDI clock running** | Clock received and app is playing |
| **MIDI clock stopped** | No recent clock |

---

## 4. MIDI note output (synth / instrument)

The app sends notes to the **first MIDI output device** macOS reports. It does **not** currently send notes back through the IAC Bus.

Typical setups:

- **Hardware synth:** connect USB, ensure the synth appears as a MIDI output; if it is the first device, notes go there
- **Software instrument in another DAW:** use a virtual MIDI port or IAC routing from the app's output into that DAW (advanced; depends on your DAW)
- **Ableton instrument:** route the app's MIDI output into Ableton via another IAC port or external MIDI routing tool

If you hear clock but no notes, check which MIDI output device is first in the system and that a synth is listening on that port.

---

## 5. Recommended test workflow

1. Enable IAC Bus (steps above)
2. Ableton: Sync **On** → IAC Bus 1
3. DNASequencer: select **IAC Driver Bus 1**
4. Load a sequence file
5. Click **Play** in DNASequencer
6. Press **Play** in Ableton
7. Watch the read index — **scanning** until an ATG start codon, then **codons**
8. Adjust **division** (e.g. `1/8`, `1/16`, `1/64`) for note speed in codon mode

---

## Troubleshooting

### No clock / "MIDI clock stopped"

- IAC Driver **Device is online** is checked
- Ableton **Sync** is enabled for the correct IAC bus
- Correct bus selected in DNASequencer
- DNASequencer **Play** is on (not Paused)
- Ableton transport is playing

### Clock works but no notes

- DNA file loaded successfully
- Read index reached an **ATG** and mode shows **codons**
- A MIDI **output** device is available and a synth is listening
- **Sustain** may suppress repeated identical pitches
- **Pause** is off and **Play** is on

### Division feels wrong or resets

- Division only affects **codon playback**, not whitespace scanning
- Changing division should stick; if behaviour changes, check you are still in **codon** mode
- **Reset** restarts scanning from the beginning and pauses the app

### Connecting after Ableton is already playing

- Start or restart Ableton transport after the app is listening, or press **Play** in the app once clock is flowing

---

## Quick reference

| Setting | Where |
|---------|--------|
| Enable IAC Bus | Audio MIDI Setup → IAC Driver |
| Send clock | Ableton → MIDI → IAC Sync **On** |
| Receive clock | DNASequencer → MIDI Input → IAC Bus |
| Note output | First system MIDI output device |
| Local transport | DNASequencer **Play** / **Pause** |
