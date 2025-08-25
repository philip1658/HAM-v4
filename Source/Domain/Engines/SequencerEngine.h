/*
  ==============================================================================

    SequencerEngine.h
    Core sequencer engine that coordinates pattern playback, stage advancement,
    and track state management. Integrates with MasterClock for timing and
    VoiceManager for note handling.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <memory>
#include <vector>
#include <atomic>
#include "../Clock/MasterClock.h"
#include "../Models/Pattern.h"
#include "../Models/Track.h"
#include "../Models/Stage.h"
#include "VoiceManager.h"

namespace HAM {

//==============================================================================
/**
    Main sequencer engine that drives pattern playback.
    
    Manages stage advancement, track processing, and MIDI generation.
    Processes clock pulses from MasterClock and generates MIDI events
    based on pattern data. Integrates with VoiceManager for voice allocation.
*/
class SequencerEngine : public MasterClock::Listener
{
public:
    //==========================================================================
    // MIDI Event for output
    struct MidiEvent
    {
        juce::MidiMessage message;
        int sampleOffset{0};
        int trackIndex{0};
        int stageIndex{0};
    };
    
    //==========================================================================
    // Sequencer State
    enum class State
    {
        STOPPED,
        PLAYING,
        RECORDING,
        WAITING_TO_START  // Waiting for next bar/beat
    };
    
    //==========================================================================
    // Construction
    SequencerEngine();
    ~SequencerEngine() override;
    
    //==========================================================================
    // Pattern Management
    
    /** Set the active pattern to play */
    void setActivePattern(std::shared_ptr<Pattern> pattern);
    
    /** Get the active pattern */
    std::shared_ptr<Pattern> getActivePattern() const { return m_activePattern; }
    
    /** Queue a pattern change (happens on next loop) */
    void queuePatternChange(std::shared_ptr<Pattern> pattern);
    
    //==========================================================================
    // Transport Control
    
    /** Start sequencer playback */
    void start();
    
    /** Stop sequencer playback */
    void stop();
    
    /** Reset to beginning */
    void reset();
    
    /** Get current state */
    State getState() const { return m_state.load(); }
    
    //==========================================================================
    // Clock Integration
    
    /** Set master clock reference */
    void setMasterClock(MasterClock* clock);
    
    /** MasterClock::Listener interface */
    void onClockPulse(int pulseNumber) override;
    void onClockStart() override;
    void onClockStop() override;
    void onClockReset() override;
    void onTempoChanged(float newBPM) override;
    
    //==========================================================================
    // Voice Manager Integration
    
    /** Set voice manager for note allocation */
    void setVoiceManager(VoiceManager* voiceManager) { m_voiceManager = voiceManager; }
    
    //==========================================================================
    // MIDI Output
    
    /** Process and generate MIDI events for audio block */
    void processBlock(double sampleRate, int numSamples);
    
    /** Get pending MIDI events (thread-safe) */
    std::vector<MidiEvent> getPendingMidiEvents();
    
    /** Get and clear MIDI events */
    void getAndClearMidiEvents(std::vector<MidiEvent>& events);
    
    /** Set current pattern - safe wrapper for legacy code */
    void setPattern(std::shared_ptr<Pattern> pattern) { 
        // Safe delegation to setActivePattern
        setActivePattern(pattern);
    }
    
    //==========================================================================
    // Track Processing
    
    /** Process a single track's current stage */
    void processTrack(Track& track, int trackIndex, int pulseNumber);
    
    /** Advance track to next stage based on division */
    void advanceTrackStage(Track& track, int pulseNumber);
    
    /** Check if track should trigger on this pulse */
    bool shouldTrackTrigger(const Track& track, int pulseNumber) const;
    
    /** Generate MIDI for stage */
    void generateStageEvents(Track& track, const Stage& stage, 
                            int trackIndex, int stageIndex);
    
    //==========================================================================
    // Stage Processing
    
    /** Process ratchets for a stage */
    void processRatchets(const Stage& stage, Track& track, 
                        int trackIndex, int stageIndex);
    
    /** Calculate actual pitch including accumulator and scale */
    int calculatePitch(const Track& track, const Stage& stage) const;
    
    /** Apply swing to timing */
    int applySwing(int pulseNumber, float swingAmount) const;
    
    //==========================================================================
    // Pattern Management
    
    /** Handle pattern switching at loop points */
    void handlePatternSwitch();
    
    /** Check if at pattern loop point */
    bool isAtLoopPoint() const;
    
    //==========================================================================
    // State Query
    
    /** Get current position in pattern (0.0-1.0) */
    float getPatternPosition() const;
    
    /** Get current bar in pattern */
    int getCurrentPatternBar() const { return m_currentPatternBar.load(); }
    
    /** Get total bars in pattern */
    int getTotalPatternBars() const;
    
    /** Check if any track is soloed */
    bool hasSoloedTracks() const;
    
    //==========================================================================
    // Debug & Monitoring
    
    /** Get statistics for performance monitoring */
    struct Stats
    {
        int eventsGenerated{0};
        int tracksProcessed{0};
        int stagesProcessed{0};
        float cpuUsagePercent{0.0f};
        int64_t lastProcessTime{0};
    };
    
    Stats getStats() const { return m_stats; }
    void resetStats() { m_stats = Stats{}; }
    
private:
    //==========================================================================
    // Internal State
    
    // Pattern data
    std::shared_ptr<Pattern> m_activePattern;
    std::shared_ptr<Pattern> m_queuedPattern;
    
    // Transport state
    std::atomic<State> m_state{State::STOPPED};
    
    // Clock reference
    MasterClock* m_masterClock{nullptr};
    VoiceManager* m_voiceManager{nullptr};
    
    // Playback position
    std::atomic<int> m_currentPatternBar{0};
    std::atomic<int> m_lastProcessedPulse{-1};
    
    // Track states (pulse counters for each track)
    // Using array instead of vector because atomics can't be moved/copied
    std::array<std::atomic<int>, 128> m_trackPulseCounters;  // Max 128 tracks
    std::array<std::atomic<int>, 128> m_trackLastTriggerPulse;
    
    // MIDI event queue (lock-free)
    juce::AbstractFifo m_midiEventFifo{1024};
    std::vector<MidiEvent> m_midiEventBuffer;
    
    // Performance stats
    mutable Stats m_stats;
    
    //==========================================================================
    // Internal Methods
    
    /** Initialize track states for pattern */
    void initializeTrackStates();
    
    /** Queue MIDI event for output */
    void queueMidiEvent(const MidiEvent& event);
    
    /** Calculate samples until next pulse */
    int calculateSampleOffset(int pulseNumber, int numSamples) const;
    
    /** Handle track direction (forward, backward, pendulum, random) */
    int getNextStageIndex(const Track& track, int currentIndex) const;
    
    /** Apply accumulator to pitch */
    int applyAccumulator(const Track& track, int basePitch) const;
    
    /** Check skip conditions for stage */
    bool shouldSkipStage(const Stage& stage) const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SequencerEngine)
};

} // namespace HAM