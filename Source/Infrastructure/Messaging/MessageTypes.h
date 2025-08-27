/*
  ==============================================================================

    MessageTypes.h
    Message type definitions for UI<->Engine communication
    Designed for zero-allocation in real-time thread

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <variant>
#include <array>

namespace HAM
{

//==============================================================================
// UI to Engine Messages

/**
 * Messages sent from UI to audio engine
 * All data is POD or fixed-size for zero allocation
 */
struct UIToEngineMessage
{
    enum Type
    {
        // Transport Control (CRITICAL priority)
        TRANSPORT_PLAY,
        TRANSPORT_STOP,
        TRANSPORT_PAUSE,
        TRANSPORT_RECORD,
        TRANSPORT_PANIC,        // Stop all notes immediately
        
        // Parameter Changes (HIGH priority)
        SET_BPM,
        SET_SWING,
        SET_MASTER_VOLUME,
        SET_PATTERN_LENGTH,
        
        // Pattern Changes (NORMAL priority)
        LOAD_PATTERN,
        CLEAR_PATTERN,
        UPDATE_STAGE,
        UPDATE_TRACK,
        
        // Track Control (NORMAL priority)
        SET_TRACK_MUTE,
        SET_TRACK_SOLO,
        SET_TRACK_VOICE_MODE,
        SET_TRACK_DIVISION,
        SET_TRACK_CHANNEL,
        ADD_TRACK,
        REMOVE_TRACK,
        
        // Stage Parameters (NORMAL priority)
        SET_STAGE_PITCH,
        SET_STAGE_VELOCITY,
        SET_STAGE_GATE,
        SET_STAGE_PULSE_COUNT,
        SET_STAGE_RATCHETS,
        
        // Engine Configuration (LOW priority)
        SET_SCALE,
        SET_ACCUMULATOR_MODE,
        SET_GATE_TYPE,
        SET_VOICE_STEALING_MODE,
        
        // MIDI Preview (HIGH priority)
        PREVIEW_NOTE_ON,
        PREVIEW_NOTE_OFF,
        PREVIEW_SCALE,
        
        // Morphing Control (NORMAL priority)
        START_MORPH,
        SET_MORPH_POSITION,
        SAVE_SNAPSHOT,
        LOAD_SNAPSHOT,
        
        // System Control (DEFERRED priority)
        REQUEST_STATE_DUMP,
        RESET_STATISTICS,
        ENABLE_DEBUG_MODE,
        DISABLE_DEBUG_MODE
    };
    
    Type type;
    
    // Parameter data (union to save memory)
    union
    {
        struct { float value; } floatParam;
        struct { int value; } intParam;
        struct { bool value; } boolParam;
        struct { int trackIndex; int value; } trackParam;
        struct { int trackIndex; int stageIndex; float value; } stageParam;
        struct { int patternId; } patternParam;
        struct { int snapshotSlot; } snapshotParam;
        struct { int sourceSlot; int targetSlot; float position; } morphParam;
        struct { int note; float velocity; int channel; } previewParam;
    } data;
    
    // For complex data that doesn't fit in union
    std::array<float, 8> extraData;  // Fixed size array for ratchets, etc.
};

//==============================================================================
// Engine to UI Messages

/**
 * Messages sent from audio engine to UI
 * Contains real-time status and feedback
 */
struct EngineToUIMessage
{
    enum Type
    {
        // Transport Status (sent every process block)
        TRANSPORT_STATUS,
        PLAYHEAD_POSITION,
        
        // Voice Activity
        VOICE_TRIGGERED,
        VOICE_RELEASED,
        VOICE_STOLEN,
        ACTIVE_VOICE_COUNT,
        
        // Pattern Progress
        CURRENT_STAGE,
        CURRENT_PULSE,
        PATTERN_LOOPED,
        
        // MIDI Events (for visualization)
        MIDI_NOTE_ON,
        MIDI_NOTE_OFF,
        MIDI_CC,
        
        // Performance Metrics
        CPU_USAGE,
        BUFFER_UNDERRUN,
        TIMING_DRIFT,
        
        // Morphing Status
        MORPH_PROGRESS,
        SNAPSHOT_SAVED,
        
        // Error Reporting
        ERROR_PATTERN_LOAD_FAILED,
        ERROR_MIDI_DEVICE_LOST,
        ERROR_CPU_OVERLOAD,
        
        // Debug Information
        DEBUG_TIMING_INFO,
        DEBUG_VOICE_INFO,
        DEBUG_QUEUE_STATS
    };
    
    Type type;
    
    // Status data
    union
    {
        struct { bool playing; bool recording; float bpm; } transport;
        struct { float bars; float beats; float pulses; } playhead;
        struct { int note; int velocity; int channel; } midi;
        struct { int count; int stolen; int peak; } voices;
        struct { int track; int stage; int pulse; } position;
        struct { float position; int sourceSlot; int targetSlot; } morph;
        struct { float cpu; float peak; int underruns; } performance;
        struct { float drift; float jitter; float latency; } timing;
    } data;
    
    // Timestamp for latency measurement
    uint64_t timestamp;
    
    // Optional error/debug message (fixed size)
    std::array<char, 128> message;
};

//==============================================================================
// Batch Message Container

/**
 * Container for batching multiple messages
 * Reduces overhead of individual message passing
 */
template<typename MessageType>
struct MessageBatch
{
    static constexpr size_t MAX_BATCH_SIZE = 32;
    
    std::array<MessageType, MAX_BATCH_SIZE> messages;
    size_t count = 0;
    
    bool add(const MessageType& msg)
    {
        if (count < MAX_BATCH_SIZE)
        {
            messages[count++] = msg;
            return true;
        }
        return false;
    }
    
    void clear()
    {
        count = 0;
    }
    
    bool isEmpty() const
    {
        return count == 0;
    }
    
    bool isFull() const
    {
        return count >= MAX_BATCH_SIZE;
    }
};

//==============================================================================
// Message Factory Helpers

/**
 * Helper functions to create messages without allocations
 */
class MessageFactory
{
public:
    // Transport messages
    static UIToEngineMessage makePlayMessage()
    {
        UIToEngineMessage msg;
        msg.type = UIToEngineMessage::TRANSPORT_PLAY;
        return msg;
    }
    
    static UIToEngineMessage makeStopMessage()
    {
        UIToEngineMessage msg;
        msg.type = UIToEngineMessage::TRANSPORT_STOP;
        return msg;
    }
    
    static UIToEngineMessage makePanicMessage()
    {
        UIToEngineMessage msg;
        msg.type = UIToEngineMessage::TRANSPORT_PANIC;
        return msg;
    }
    
    // Parameter messages
    static UIToEngineMessage setBPM(float bpm)
    {
        UIToEngineMessage msg;
        msg.type = UIToEngineMessage::SET_BPM;
        msg.data.floatParam.value = bpm;
        return msg;
    }
    
    static UIToEngineMessage setSwing(float swing)
    {
        UIToEngineMessage msg;
        msg.type = UIToEngineMessage::SET_SWING;
        msg.data.floatParam.value = swing;
        return msg;
    }
    
    // Track messages
    static UIToEngineMessage setTrackMute(int track, bool mute)
    {
        UIToEngineMessage msg;
        msg.type = UIToEngineMessage::SET_TRACK_MUTE;
        msg.data.trackParam.trackIndex = track;
        msg.data.trackParam.value = mute ? 1 : 0;
        return msg;
    }

        static UIToEngineMessage addTrack(int newTrackIndex)
        {
            UIToEngineMessage msg;
            msg.type = UIToEngineMessage::ADD_TRACK;
            msg.data.trackParam.trackIndex = newTrackIndex;
            msg.data.trackParam.value = 0;
            return msg;
        }
    
    // Stage messages
    static UIToEngineMessage setStagePitch(int track, int stage, float pitch)
    {
        UIToEngineMessage msg;
        msg.type = UIToEngineMessage::SET_STAGE_PITCH;
        msg.data.stageParam.trackIndex = track;
        msg.data.stageParam.stageIndex = stage;
        msg.data.stageParam.value = pitch;
        return msg;
    }
    
    // Morphing messages
    static UIToEngineMessage startMorph(int source, int target, float position)
    {
        UIToEngineMessage msg;
        msg.type = UIToEngineMessage::START_MORPH;
        msg.data.morphParam.sourceSlot = source;
        msg.data.morphParam.targetSlot = target;
        msg.data.morphParam.position = position;
        return msg;
    }
    
    // Engine status messages
    static EngineToUIMessage makeTransportStatus(bool playing, bool recording, float bpm)
    {
        EngineToUIMessage msg;
        msg.type = EngineToUIMessage::TRANSPORT_STATUS;
        msg.data.transport.playing = playing;
        msg.data.transport.recording = recording;
        msg.data.transport.bpm = bpm;
        msg.timestamp = juce::Time::currentTimeMillis();
        return msg;
    }
    
    static EngineToUIMessage makePlayheadPosition(float bars, float beats, float pulses)
    {
        EngineToUIMessage msg;
        msg.type = EngineToUIMessage::PLAYHEAD_POSITION;
        msg.data.playhead.bars = bars;
        msg.data.playhead.beats = beats;
        msg.data.playhead.pulses = pulses;
        msg.timestamp = juce::Time::currentTimeMillis();
        return msg;
    }
    
    static EngineToUIMessage makeVoiceActivity(int count, int stolen, int peak)
    {
        EngineToUIMessage msg;
        msg.type = EngineToUIMessage::ACTIVE_VOICE_COUNT;
        msg.data.voices.count = count;
        msg.data.voices.stolen = stolen;
        msg.data.voices.peak = peak;
        msg.timestamp = juce::Time::currentTimeMillis();
        return msg;
    }
    
    static EngineToUIMessage makeMidiNote(bool noteOn, int note, int velocity, int channel)
    {
        EngineToUIMessage msg;
        msg.type = noteOn ? EngineToUIMessage::MIDI_NOTE_ON : EngineToUIMessage::MIDI_NOTE_OFF;
        msg.data.midi.note = note;
        msg.data.midi.velocity = velocity;
        msg.data.midi.channel = channel;
        msg.timestamp = juce::Time::currentTimeMillis();
        return msg;
    }
};

} // namespace HAM