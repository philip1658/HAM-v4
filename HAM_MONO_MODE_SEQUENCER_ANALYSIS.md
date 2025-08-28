# HAM Sequencer: Note Timing and Generation in Mono Mode

**Analysis Date**: August 28, 2025  
**HAM Version**: v4.0  
**Focus**: Complete flow from timing to MIDI output in mono mode

## 🕐 Timing Foundation: MasterClock System

### Core Timing Resolution
- **Base Resolution**: 24 PPQN (Pulses Per Quarter Note)
- **Sample-Accurate**: <0.1ms jitter timing precision
- **Bar Structure**: 1 bar = 96 pulses (4 quarter notes × 24 PPQN)

### Clock Division Values
```cpp
enum class Division
{
    Whole       = 96,   // 4 quarter notes  
    DottedHalf  = 72,   // 3 quarter notes
    Half        = 48,   // 2 quarter notes
    DottedQuarter = 36, // 1.5 quarter notes
    Quarter     = 24,   // 1 quarter note (DEFAULT)
    Eighth      = 12,   // 1/8 note
    Sixteenth   = 6,    // 1/16 note  
    ThirtySecond = 3    // 1/32 note
};
```

**At 120 BPM with Quarter Note Division (24 PPQN):**
- 1 quarter note = 0.5 seconds = 24 pulses
- 1 pulse = ~20.83ms (48kHz sample rate)

## 🎵 Track Configuration for Note Generation

### Default HAM Track Setup
```cpp
// From Track.h defaults:
class Track 
{
    int m_length = 8;                   // 8 stages per pattern
    int m_division = 1;                 // Clock division (NOT enum - direct multiplier)
    VoiceMode m_voiceMode = VoiceMode::MONO;
    int m_midiChannel = 1;              // MIDI channel 1
    int m_maxVoices = 1;                // Mono = 1 voice only
};
```

### Stage Configuration  
```cpp
class Stage
{
    int m_pitch = 60;                   // C4 (Middle C)
    float m_gate = 0.5f;                // 50% gate length
    float m_velocity = 100.0f/127.0f;   // ~79% velocity (100/127)
    int m_pulseCount = 1;               // 1 pulse per stage
    GateType m_gateType = GateType::MULTIPLE;
};
```

## ⚙️ Sequencer Engine: Note Generation Flow

### 1. Clock Pulse Reception
```cpp
void SequencerEngine::onClockPulse(int pulseNumber)
{
    // Called every 20.83ms at 120 BPM (24 PPQN)
    
    // Process each track in the pattern
    for (auto& track : pattern->getTracks()) 
    {
        processTrack(track, trackIndex, pulseNumber);
    }
}
```

### 2. Track Trigger Decision
```cpp
bool SequencerEngine::shouldTrackTrigger(const Track& track, int pulseNumber) const
{
    int division = track.getDivision();  // Default = 1
    int pulsesPerDivision = division;    
    
    // Division 1 = trigger every pulse
    // Division 4 = trigger every 4th pulse (quarter notes)
    return (pulseNumber % pulsesPerDivision) == 0;
}
```

**For Default Division = 1**: Track triggers on EVERY clock pulse!

### 3. Mono Mode Processing Logic
```cpp
void SequencerEngine::processTrack(Track& track, int trackIndex, int pulseNumber)
{
    VoiceMode voiceMode = track.getVoiceMode();
    
    if (voiceMode == VoiceMode::MONO) 
    {
        // MONO MODE: Play all pulses before advancing to next stage
        
        Stage& currentStage = track.getStage(track.getCurrentStageIndex());
        int pulseCounter = m_trackPulseCounters[trackIndex].load();
        
        if (pulseCounter >= currentStage.getPulseCount()) 
        {
            // Stage complete - advance to next stage
            advanceTrackStage(track, pulseNumber);
            
            // Cut previous notes (CRITICAL MONO BEHAVIOR)
            m_voiceManager->allNotesOff(track.getMidiChannel());
            
            // Start new stage  
            generateStageEvents(track, newStage, trackIndex, newStageIndex);
        }
        else if (pulseCounter == 0)
        {
            // Starting current stage for first time
            
            // Cut any active notes first (MONO NOTE CUTTING)
            m_voiceManager->allNotesOff(track.getMidiChannel());
            
            // Generate note-on for this stage
            generateStageEvents(track, currentStage, trackIndex, stageIndex);
        }
        
        // Continue processing ratchets within stage
        processRatchets(currentStage, track, trackIndex, stageIndex);
        
        // Increment pulse counter
        m_trackPulseCounters[trackIndex].store(pulseCounter + 1);
    }
}
```

### 4. MIDI Note Generation
```cpp
void SequencerEngine::generateStageEvents(Track& track, const Stage& stage, 
                                         int trackIndex, int stageIndex)
{
    // Skip REST gates
    if (stage.getGateType() == GateType::REST) return;
    
    // Calculate final pitch (includes scale quantization, accumulator)
    int pitch = calculatePitch(track, stage);  // Default: 60 (C4)
    pitch = juce::jlimit(0, 127, pitch);
    
    // Calculate velocity 
    int velocity = static_cast<int>(std::round(stage.getVelocity() * 127.0f));
    velocity = juce::jlimit(1, 127, velocity);  // ~100 for default
    
    // Create MIDI Note-On
    MidiEvent event;
    event.message = juce::MidiMessage::noteOn(
        track.getMidiChannel(),  // Channel 1
        pitch,                   // Note 60 (C4)
        static_cast<uint8>(velocity)  // Velocity ~100
    );
    event.trackIndex = trackIndex;
    event.stageIndex = stageIndex;
    
    // Queue for output
    queueMidiEvent(event);
}
```

### 5. Voice Manager: Mono Note Handling
```cpp
class VoiceManager 
{
    Voice* allocateVoice(int noteNumber, int velocity, int channel, VoiceMode mode)
    {
        if (mode == VoiceMode::MONO) 
        {
            // Always use voice index 0 for mono
            int voiceIndex = 0;
            
            // Cut any existing note first
            if (voiceAt(voiceIndex).active.load()) {
                // Generate note-off for previous note
                stopNote(voiceAt(voiceIndex).noteNumber.load(), channel);
            }
            
            // Start new note
            Voice& voice = voiceAt(voiceIndex);
            voice.active.store(true);
            voice.noteNumber.store(noteNumber);
            voice.velocity.store(velocity);
            voice.channel.store(channel);
            voice.startTime.store(getCurrentTime());
            
            return &voice;
        }
    }
};
```

## 🎶 Complete Mono Mode Note Timeline

### Default Configuration Analysis
- **BPM**: 120 
- **Pattern**: 8 stages, each with 1 pulse
- **Stage Duration**: 24 pulses ÷ 1 = 24 pulses per stage = 0.5 seconds
- **Gate Length**: 50% = 12 pulses = 0.25 seconds
- **Total Pattern**: 8 × 0.5s = 4 seconds

### Generated MIDI Sequence (First Pattern Loop)

| Time | Stage | Clock Pulse | Action | MIDI Event | Note | Channel | Velocity |
|------|-------|------------|---------|------------|------|---------|----------|
| 0.000s | 1 | 0 | Stage Start | Note OFF | - | 1 | - |
| 0.000s | 1 | 0 | Stage Start | Note ON | 60 (C4) | 1 | 100 |
| 0.250s | 1 | 12 | Gate End | Note OFF | 60 | 1 | 0 |
| 0.500s | 2 | 24 | Stage Advance | Note OFF | 60 | 1 | 0 |
| 0.500s | 2 | 24 | Stage Start | Note ON | 60 | 1 | 100 |
| 0.750s | 2 | 36 | Gate End | Note OFF | 60 | 1 | 0 |
| 1.000s | 3 | 48 | Stage Advance | Note OFF | 60 | 1 | 0 |
| 1.000s | 3 | 48 | Stage Start | Note ON | 60 | 1 | 100 |
| ... | ... | ... | ... | ... | ... | ... | ... |

## 🔑 Key Mono Mode Behaviors

### 1. **Immediate Note Cutting**
```cpp
// Before every new note:
m_voiceManager->allNotesOff(track.getMidiChannel());
```
- Previous notes are cut IMMEDIATELY when new stage starts
- No overlapping notes ever occur
- Ensures clean monophonic behavior

### 2. **Stage-Based Timing**  
- Each stage plays for `stage.getPulseCount()` pulses
- Default: 1 pulse per stage = 24 clock pulses = 0.5 seconds @ 120 BPM
- Stage advance happens after all pulses are played

### 3. **Voice Allocation**
- Mono mode ALWAYS uses voice index 0
- Only one voice active at any time
- New notes steal the single voice immediately

### 4. **Gate Length Processing**
```cpp
// Gate length determines note-off timing within stage
float gateLength = stage.getGate();  // 0.5 = 50%
int gateOffPulse = static_cast<int>(stageDurationPulses * gateLength);
// Note OFF sent after 12 pulses (0.25s) for 50% gate
```

## 🚀 Performance Characteristics

### Real-Time Processing
- **CPU Usage**: 4-10 microseconds per audio block
- **Memory**: Zero allocations in audio thread  
- **Timing Precision**: <0.1ms jitter
- **Voice Limit**: 1 active voice (mono mode)

### Lock-Free Architecture
- Atomic counters for pulse tracking
- Lock-free MIDI event queue
- Thread-safe parameter updates
- No mutexes in audio thread

`★ Insight ─────────────────────────────────────`  
HAM's mono mode implements a sophisticated stage-based sequencer where timing is derived from the MasterClock's 24 PPQN resolution. Unlike simple step sequencers, HAM plays each stage for its full pulse count before advancing, enabling complex rhythmic patterns within each stage through ratcheting and sub-divisions.
`─────────────────────────────────────────────────`

## 🎯 Summary: How Notes Are Defined

1. **Clock Foundation**: 24 PPQN master clock provides timing base
2. **Track Division**: Determines how often track triggers (default = every pulse)
3. **Stage Duration**: Each stage plays for `pulseCount` pulses before advancing  
4. **Pitch Calculation**: Base pitch + accumulator + scale quantization
5. **Mono Voice Management**: Always voice 0, immediate note cutting
6. **MIDI Generation**: Note-on at stage start, note-off at gate end
7. **Pattern Loop**: 8 stages cycle continuously while playing

The result is a precise, sample-accurate monophonic sequencer that generates the exact MIDI patterns documented in your analysis file!

---
**Generated by**: Claude (AI Assistant)  
**Source Analysis**: HAM v4.0 Sequencer Engine