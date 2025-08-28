# HAM Mono Mode MIDI Message Analysis
*Generated: 2025-08-28*

## Default Configuration Summary

**Based on code analysis of HAM v4.0:**
- **Voice Mode**: MONO (VoiceMode::MONO) - New notes cut previous notes immediately
- **Default BPM**: 120 BPM
- **Clock Resolution**: 24 PPQN (Pulses Per Quarter Note)
- **Track Settings**: 8 stages, Length=8, Division=4 (quarter notes)
- **Channel**: Channel 1 (m_defaultMidiOutputChannel = 1)
- **Voice Allocation**: Always voice index 0 in mono mode
- **Note Values**: Scale degree 0 → MIDI note 60 (C4) by default
- **Gate Length**: 0.5 (50% gate length)
- **Velocity**: 100
- **Stage Timing**: Each stage = 0.5 seconds @ 120 BPM

`★ Insight ─────────────────────────────────────`
HAM's mono mode implements **immediate note cutting** - when a new note starts, the previous note is stopped instantly with no overlap. This ensures clean monophonic behavior essential for bass lines and lead melodies.
`─────────────────────────────────────────────────`

## Complete MIDI Message Timeline Table

### Pattern Loop 1 (8 stages × 0.5s = 4 seconds total)

| Time | Stage | Event Type | Channel | Note | Velocity | Sample Offset | Voice | Trigger | Description |
|------|-------|------------|---------|------|----------|---------------|-------|---------|-------------|
| 0.000s | 1 | Note OFF | 1 | 60 | 0 | 0 | 0 | Stage Start | **Previous note cut** (if any) |
| 0.000s | 1 | Note ON | 1 | 60 | 100 | 0 | 0 | Stage Start | **New note starts** |
| 0.250s | 1 | Note OFF | 1 | 60 | 0 | 12000 | 0 | Gate End | Gate length 50% = 0.25s |
| 0.500s | 2 | Note OFF | 1 | 60 | 0 | 24000 | 0 | Stage Start | **Previous note cut** |
| 0.500s | 2 | Note ON | 1 | 60 | 100 | 24000 | 0 | Stage Start | **New note starts** |
| 0.750s | 2 | Note OFF | 1 | 60 | 0 | 36000 | 0 | Gate End | Gate length 50% = 0.25s |
| 1.000s | 3 | Note OFF | 1 | 60 | 0 | 48000 | 0 | Stage Start | **Previous note cut** |
| 1.000s | 3 | Note ON | 1 | 60 | 100 | 48000 | 0 | Stage Start | **New note starts** |
| 1.250s | 3 | Note OFF | 1 | 60 | 0 | 60000 | 0 | Gate End | Gate length 50% = 0.25s |
| 1.500s | 4 | Note OFF | 1 | 60 | 0 | 72000 | 0 | Stage Start | **Previous note cut** |
| 1.500s | 4 | Note ON | 1 | 60 | 100 | 72000 | 0 | Stage Start | **New note starts** |
| 1.750s | 4 | Note OFF | 1 | 60 | 0 | 84000 | 0 | Gate End | Gate length 50% = 0.25s |
| 2.000s | 5 | Note OFF | 1 | 60 | 0 | 96000 | 0 | Stage Start | **Previous note cut** |
| 2.000s | 5 | Note ON | 1 | 60 | 100 | 96000 | 0 | Stage Start | **New note starts** |
| 2.250s | 5 | Note OFF | 1 | 60 | 0 | 108000 | 0 | Gate End | Gate length 50% = 0.25s |
| 2.500s | 6 | Note OFF | 1 | 60 | 0 | 120000 | 0 | Stage Start | **Previous note cut** |
| 2.500s | 6 | Note ON | 1 | 60 | 100 | 120000 | 0 | Stage Start | **New note starts** |
| 2.750s | 6 | Note OFF | 1 | 60 | 0 | 132000 | 0 | Gate End | Gate length 50% = 0.25s |
| 3.000s | 7 | Note OFF | 1 | 60 | 0 | 144000 | 0 | Stage Start | **Previous note cut** |
| 3.000s | 7 | Note ON | 1 | 60 | 100 | 144000 | 0 | Stage Start | **New note starts** |
| 3.250s | 7 | Note OFF | 1 | 60 | 0 | 156000 | 0 | Gate End | Gate length 50% = 0.25s |
| 3.500s | 8 | Note OFF | 1 | 60 | 0 | 168000 | 0 | Stage Start | **Previous note cut** |
| 3.500s | 8 | Note ON | 1 | 60 | 100 | 168000 | 0 | Stage Start | **New note starts** |
| 3.750s | 8 | Note OFF | 1 | 60 | 0 | 180000 | 0 | Gate End | Gate length 50% = 0.25s |

**Pattern Loop Restart: 4.000s**

## Key Mono Mode Behaviors

### 1. **Immediate Note Cutting (Critical)**
```cpp
// From VoiceManager::handleMonoNoteOn()
if (voiceAt(voiceIndex).active.load()) {
    voiceAt(voiceIndex).stopNote();  // IMMEDIATE cut
    int prev = m_activeVoiceCount.fetch_sub(1);
}
// Start new note
voiceAt(voiceIndex).startNote(noteNumber, velocity, channel);
```
- **No overlapping notes** - each stage start triggers immediate Note OFF + Note ON
- **Voice 0 always used** - mono mode uses only first voice slot
- **Instantaneous switching** - <1 sample delay between OFF and ON

### 2. **Gate Length Behavior**
```cpp
// Gate length = 0.5 means 50% of stage duration
float stageTime = 0.5s;  // @ 120 BPM, quarter note division
float gateTime = stageTime * gateLength;  // 0.25s
```
- **Gate ends early** - Note OFF sent at gate duration
- **Silent periods** - Gap between gate end and next stage
- **Consistent timing** - Gate calculated per-stage, not globally

### 3. **Sample-Accurate Timing**
- **48kHz sample rate** = 48,000 samples per second
- **Stage duration** = 24,000 samples (0.5s)
- **Gate duration** = 12,000 samples (0.25s)
- **Timing precision** = ±1 sample (<0.021ms jitter)

## Expected Debug Channel Output

HAM also outputs debug information on **channel 16** (if enabled):

| Time | Event Type | Channel | Note | Data | Description |
|------|------------|---------|------|------|-------------|
| 0.000s | CC 120 | 16 | - | 1 | Track 0, Stage 1 trigger |
| 0.500s | CC 120 | 16 | - | 2 | Track 0, Stage 2 trigger |
| 1.000s | CC 120 | 16 | - | 3 | Track 0, Stage 3 trigger |
| ... | ... | 16 | - | ... | Continuing stage markers |

## Voice Manager Statistics (Expected)

After 1 complete loop (8 stages):
- **totalNotesPlayed**: 8
- **activeVoices**: 0 (after final gate ends)
- **notesStolen**: 0 (no voice stealing in mono)
- **peakVoiceCount**: 1 (mono maximum)

## Timing Calculations

### BPM to Time Conversion
```cpp
float bpm = 120.0f;
float beatsPerSecond = bpm / 60.0f;  // 2.0 beats/second
float quarterNoteTime = 1.0f / beatsPerSecond;  // 0.5 seconds
float stageTime = quarterNoteTime;  // Division = 4 (quarter notes)
```

### Sample Position Calculation (48kHz)
```cpp
float timeInSeconds = stageIndex * 0.5f;  // Each stage = 0.5s
int sampleOffset = timeInSeconds * 48000;  // Convert to samples
```

## Code-Level MIDI Flow

### Message Generation Path
```
SequencerEngine::onClockPulse()
  → TrackProcessor::processTrack()
    → MidiEventGenerator::generateStageEvents()
      → VoiceManager::noteOn() [MONO mode]
        → handleMonoNoteOn() [immediate cut + start]
          → MidiRouter::addEventToTrack()
            → MidiRouter::processBlock()
              → outputBuffer.addEvent()
```

### Mono Mode Logic
```cpp
int VoiceManager::handleMonoNoteOn(int noteNumber, int velocity, int channel) {
    int voiceIndex = 0;  // Always use first voice for mono
    
    // Stop previous note if playing (IMMEDIATE CUT)
    if (voiceAt(voiceIndex).active.load()) {
        voiceAt(voiceIndex).stopNote();
    }
    
    // Start new note
    voiceAt(voiceIndex).startNote(noteNumber, velocity, channel);
    return voiceIndex;
}
```

## Testing Verification Points

1. **Timing Accuracy**: Verify stage timing is exactly 0.5s @ 120 BPM
2. **Note Cutting**: Confirm no overlapping notes in mono mode
3. **Voice Usage**: Only voice 0 should be active
4. **Gate Length**: Note OFF at 0.25s after Note ON
5. **Channel Output**: All notes on channel 1
6. **Debug Output**: Stage markers on channel 16 (if enabled)
7. **Sample Precision**: ±1 sample accuracy for all events

This table provides a complete reference for expected MIDI behavior in HAM's default mono mode configuration.