/*
  ==============================================================================

    Transport.h
    Main transport control system with play/stop/record and position management

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../Clock/MasterClock.h"
#include <atomic>
#include <functional>

namespace HAM {

//==============================================================================
/**
    Central transport control for sequencer playback.
    Manages play/stop/record states, position, loop points, and count-in.
*/
class Transport
{
public:
    //==========================================================================
    // Transport States
    enum class State
    {
        STOPPED,
        PLAYING,
        RECORDING,
        PAUSED,
        WAITING_FOR_SYNC,  // Waiting for external sync
        COUNT_IN           // Count-in before recording
    };
    
    //==========================================================================
    // Sync Modes
    enum class SyncMode
    {
        INTERNAL,          // Use internal clock
        MIDI_CLOCK,        // Sync to MIDI clock input
        ABLETON_LINK,      // Sync via Ableton Link (future)
        MTC,               // MIDI Time Code (future)
        HOST_SYNC          // Sync to plugin host (when used as plugin)
    };
    
    //==========================================================================
    // Transport Position
    struct Position
    {
        int bar = 0;
        int beat = 0;
        int pulse = 0;     // 24 PPQN
        double ppqPosition = 0.0;  // Absolute position in quarter notes
        
        // Time signature
        int numerator = 4;
        int denominator = 4;
        
        // Tempo
        float bpm = 120.0f;
        
        // Loop points (in bars)
        int loopStartBar = 0;
        int loopEndBar = 4;
        bool isLooping = false;
        
        // Recording
        bool isRecording = false;
        bool isPunching = false;
        int punchInBar = 0;
        int punchOutBar = 0;
        
        // Helpers
        int getTotalPulses() const
        {
            return (bar * 96) + (beat * 24) + pulse;  // 96 pulses per bar (4/4)
        }
        
        void setFromTotalPulses(int totalPulses)
        {
            bar = totalPulses / 96;
            int remaining = totalPulses % 96;
            beat = remaining / 24;
            pulse = remaining % 24;
            ppqPosition = totalPulses / 24.0;
        }
    };
    
    //==========================================================================
    // Listener Interface
    class Listener
    {
    public:
        virtual ~Listener() = default;
        
        /** Called when transport starts */
        virtual void onTransportStart() {}
        
        /** Called when transport stops */
        virtual void onTransportStop() {}
        
        /** Called when transport pauses */
        virtual void onTransportPause() {}
        
        /** Called when recording starts */
        virtual void onRecordingStart() {}
        
        /** Called when recording stops */
        virtual void onRecordingStop() {}
        
        /** Called when position changes */
        virtual void onPositionChanged(const Position& position) {}
        
        /** Called when sync mode changes */
        virtual void onSyncModeChanged(SyncMode mode) {}
        
        /** Called when loop state changes */
        virtual void onLoopStateChanged(bool looping) {}
    };
    
    //==========================================================================
    // Construction
    Transport(MasterClock& clock);
    ~Transport();
    
    //==========================================================================
    // Transport Control
    
    /** Start playback from current position */
    void play();
    
    /** Stop playback and optionally return to start */
    void stop(bool returnToZero = false);
    
    /** Pause playback (maintains position) */
    void pause();
    
    /** Toggle play/stop */
    void togglePlayStop();
    
    /** Start recording */
    void record(bool useCountIn = false, int countInBars = 1);
    
    /** Stop recording */
    void stopRecording();
    
    //==========================================================================
    // Position Control
    
    /** Jump to specific bar */
    void setPosition(int bar, int beat = 0, int pulse = 0);
    
    /** Jump to start */
    void returnToZero();
    
    /** Move by bars */
    void moveByBars(int bars);
    
    /** Get current position */
    Position getCurrentPosition() const;
    
    /** Get position as string (e.g., "001.1.1") */
    juce::String getPositionString() const;
    
    //==========================================================================
    // Loop Control
    
    /** Enable/disable looping */
    void setLooping(bool shouldLoop);
    
    /** Set loop points */
    void setLoopPoints(int startBar, int endBar);
    
    /** Get loop start bar */
    int getLoopStartBar() const { return m_loopStartBar; }
    
    /** Get loop end bar */
    int getLoopEndBar() const { return m_loopEndBar; }
    
    /** Check if looping */
    bool isLooping() const { return m_isLooping; }
    
    //==========================================================================
    // Punch In/Out
    
    /** Enable punch recording */
    void setPunchEnabled(bool enabled);
    
    /** Set punch points */
    void setPunchPoints(int inBar, int outBar);
    
    //==========================================================================
    // Sync Control
    
    /** Set sync mode */
    void setSyncMode(SyncMode mode);
    
    /** Get current sync mode */
    SyncMode getSyncMode() const { return m_syncMode; }
    
    /** Check if using external sync */
    bool isExternalSync() const;
    
    /** Force resync with external source */
    void resync();
    
    //==========================================================================
    // Time Signature
    
    /** Set time signature */
    void setTimeSignature(int numerator, int denominator);
    
    /** Get time signature numerator */
    int getTimeSignatureNumerator() const { return m_timeSignatureNum; }
    
    /** Get time signature denominator */  
    int getTimeSignatureDenominator() const { return m_timeSignatureDenom; }
    
    //==========================================================================
    // State Query
    
    /** Get current transport state */
    State getState() const { return m_state; }
    
    /** Check if playing */
    bool isPlaying() const { return m_state == State::PLAYING || m_state == State::RECORDING; }
    
    /** Check if stopped */
    bool isStopped() const { return m_state == State::STOPPED; }
    
    /** Check if recording */
    bool isRecording() const { return m_state == State::RECORDING; }
    
    /** Check if paused */
    bool isPaused() const { return m_state == State::PAUSED; }
    
    //==========================================================================
    // Count-In
    
    /** Set count-in bars */
    void setCountInBars(int bars) { m_countInBars = bars; }
    
    /** Get count-in bars */
    int getCountInBars() const { return m_countInBars; }
    
    /** Check if in count-in */
    bool isCountingIn() const { return m_state == State::COUNT_IN; }
    
    //==========================================================================
    // Listener Management
    
    void addListener(Listener* listener);
    void removeListener(Listener* listener);
    
    //==========================================================================
    // Clock Callback (called from MasterClock)
    void processClock(int totalPulses);
    
private:
    //==========================================================================
    // Internal State
    
    MasterClock& m_clock;
    
    // Transport state
    std::atomic<State> m_state{State::STOPPED};
    std::atomic<SyncMode> m_syncMode{SyncMode::INTERNAL};
    
    // Position
    std::atomic<int> m_currentBar{0};
    std::atomic<int> m_currentBeat{0};
    std::atomic<int> m_currentPulse{0};
    std::atomic<double> m_ppqPosition{0.0};
    
    // Loop state
    std::atomic<bool> m_isLooping{false};
    std::atomic<int> m_loopStartBar{0};
    std::atomic<int> m_loopEndBar{4};
    
    // Punch state
    std::atomic<bool> m_punchEnabled{false};
    std::atomic<int> m_punchInBar{0};
    std::atomic<int> m_punchOutBar{4};
    
    // Count-in
    std::atomic<int> m_countInBars{1};
    std::atomic<int> m_countInPulsesRemaining{0};
    
    // Time signature
    std::atomic<int> m_timeSignatureNum{4};
    std::atomic<int> m_timeSignatureDenom{4};
    
    // Listeners
    std::vector<Listener*> m_listeners;
    std::atomic<bool> m_isNotifying{false};
    
    //==========================================================================
    // Internal Methods
    
    /** Process loop boundaries */
    void processLooping();
    
    /** Process punch in/out */
    void processPunchInOut();
    
    /** Process count-in */
    void processCountIn();
    
    /** Update position from clock */
    void updatePosition(int totalPulses);
    
    /** Notify listeners */
    void notifyTransportStart();
    void notifyTransportStop();
    void notifyTransportPause();
    void notifyRecordingStart();
    void notifyRecordingStop();
    void notifyPositionChanged();
    void notifySyncModeChanged();
    void notifyLoopStateChanged();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Transport)
};

} // namespace HAM