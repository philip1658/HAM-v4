/*
  ==============================================================================

    MasterClock.h
    Master timing system with sample-accurate clock generation
    Provides 24 PPQN (Pulses Per Quarter Note) timing

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <functional>
#include <vector>
#include "TimingConstants.h"

namespace HAM {

//==============================================================================
/**
    Sample-accurate master clock with 24 PPQN resolution
    
    Generates timing pulses for sequencing with minimal jitter (<0.1ms).
    Supports dynamic BPM changes without glitches and provides
    clock division calculations for various time signatures.
*/
class MasterClock
{
public:
    //==========================================================================
    // Clock Listener Interface
    class Listener
    {
    public:
        virtual ~Listener() = default;
        
        /** Called on each 24 PPQN pulse */
        virtual void onClockPulse(int pulseNumber) = 0;
        
        /** Called when clock starts */
        virtual void onClockStart() = 0;
        
        /** Called when clock stops */
        virtual void onClockStop() = 0;
        
        /** Called when clock position resets */
        virtual void onClockReset() = 0;
        
        /** Called when BPM changes */
        virtual void onTempoChanged(float newBPM) = 0;
    };
    
    //==========================================================================
    // Clock Divisions (multipliers of base 24 PPQN)
    enum class Division
    {
        Whole       = 96,   // 4 quarter notes
        DottedHalf  = 72,   // 3 quarter notes
        Half        = 48,   // 2 quarter notes
        Triplet     = 32,   // Triplet quarter
        Quarter     = 24,   // 1 quarter note (base)
        Eighth      = 12,   // 1/2 quarter
        Sixteenth   = 6,    // 1/4 quarter
        ThirtySecond = 3    // 1/8 quarter
    };
    
    //==========================================================================
    // Construction
    MasterClock();
    ~MasterClock();
    
    //==========================================================================
    // Transport Control
    
    /** Start the clock from current position */
    void start();
    
    /** Stop the clock (maintains position) */
    void stop();
    
    /** Reset clock to beginning */
    void reset();
    
    /** Check if clock is running */
    bool isRunning() const { return m_isRunning.load(); }
    
    //==========================================================================
    // Tempo Control
    
    /** Set tempo in BPM (20-999) */
    void setBPM(float bpm);
    
    /** Get current tempo */
    float getBPM() const { return m_bpm.load(); }
    
    /** Set sample rate */
    void setSampleRate(double sampleRate) { m_sampleRate.store(sampleRate); }
    
    /** Enable/disable glide between tempo changes */
    void setTempoGlideEnabled(bool enabled) { m_tempoGlideEnabled = enabled; }
    
    /** Set tempo glide time in milliseconds */
    void setTempoGlideTime(float ms) { m_tempoGlideMs = ms; }
    
    //==========================================================================
    // Sample-Accurate Processing
    
    /** Process audio block and generate clock pulses
        Called from audio thread - must be lock-free!
        @param sampleRate Current sample rate
        @param numSamples Number of samples to process
    */
    void processBlock(double sampleRate, int numSamples);
    
    //==========================================================================
    // Clock Query
    
    /** Get current pulse number (0-95 for 4/4) */
    int getCurrentPulse() const { return m_currentPulse.load(); }
    
    /** Get current bar number */
    int getCurrentBar() const { return m_currentBar.load(); }
    
    /** Get current beat within bar (0-3 for 4/4) */
    int getCurrentBeat() const { return m_currentBeat.load(); }
    
    /** Get samples until next pulse */
    int getSamplesUntilNextPulse(double sampleRate) const;
    
    /** Get phase within current pulse (0.0-1.0) */
    float getPulsePhase() const { return m_pulsePhase.load(); }
    
    //==========================================================================
    // Clock Division Helpers
    
    /** Calculate samples per clock division */
    static double getSamplesPerDivision(Division div, float bpm, double sampleRate);
    
    /** Check if current pulse aligns with division */
    bool isOnDivision(Division div) const;
    
    /** Get next pulse that aligns with division */
    int getNextDivisionPulse(Division div) const;
    
    //==========================================================================
    // Listener Management
    
    /** Add clock listener (called from message thread only) */
    void addListener(Listener* listener);
    
    /** Remove clock listener (called from message thread only) */
    void removeListener(Listener* listener);
    
    //==========================================================================
    // Sync & MIDI Clock
    
    /** Process incoming MIDI clock (for external sync) */
    void processMidiClock();
    
    /** Process MIDI start message */
    void processMidiStart();
    
    /** Process MIDI stop message */
    void processMidiStop();
    
    /** Process MIDI continue message */
    void processMidiContinue();
    
    /** Enable/disable external MIDI clock sync */
    void setExternalSyncEnabled(bool enabled) { m_externalSyncEnabled = enabled; }
    
    /** Check if using external sync */
    bool isExternalSyncEnabled() const { return m_externalSyncEnabled; }
    
    /** Apply drift compensation by adjusting sample counter
        @param sampleOffset Compensation in samples (can be negative)
    */
    void applyDriftCompensation(double sampleOffset);
    
private:
    //==========================================================================
    // Internal State
    
    // Transport state
    std::atomic<bool> m_isRunning{false};
    std::atomic<float> m_bpm{TimingConstants::DEFAULT_BPM};
    std::atomic<float> m_targetBpm{TimingConstants::DEFAULT_BPM};
    
    // Clock position
    std::atomic<int> m_currentPulse{0};      // 0-23 within beat
    std::atomic<int> m_currentBeat{0};       // 0-3 within bar
    std::atomic<int> m_currentBar{0};        // Bar number
    std::atomic<float> m_pulsePhase{0.0f};   // 0-1 within pulse
    
    // High-precision sample-accurate timing
    TimingConstants::PreciseSampleCount m_preciseSamplesPerPulse{0};
    TimingConstants::PreciseSampleCount m_preciseSampleCounter{0};
    double m_lastSampleRate{TimingConstants::FALLBACK_SAMPLE_RATE};
    
    // Tempo glide
    bool m_tempoGlideEnabled{false};
    float m_tempoGlideMs{TimingConstants::DEFAULT_TEMPO_GLIDE_MS};
    float m_currentGlideBpm{TimingConstants::DEFAULT_BPM};
    float m_glideIncrement{0.0f};
    int m_glideSamplesRemaining{0};
    
    // Sample rate
    std::atomic<double> m_sampleRate{TimingConstants::DEFAULT_SAMPLE_RATE};
    
    // External sync
    bool m_externalSyncEnabled{false};
    int m_midiClockCounter{0};
    int64_t m_lastMidiClockTime{0};
    double m_midiClockInterval{0.0};
    
    // Listeners (only accessed from message thread for add/remove)
    std::vector<Listener*> m_listeners;
    std::atomic<bool> m_isNotifying{false};
    
    // Message thread callback queue
    std::function<void()> m_pendingListenerCall;
    
    //==========================================================================
    // Internal Methods
    
    /** Calculate samples per pulse for current BPM */
    void updateSamplesPerPulse(double sampleRate);
    
    /** Advance clock by one pulse */
    void advancePulse();
    
    /** Notify listeners on message thread */
    void notifyClockPulse(int pulse);
    void notifyClockStart();
    void notifyClockStop();
    void notifyClockReset();
    void notifyTempoChanged(float bpm);
    
    /** Apply tempo glide */
    void processTempoGlide(int numSamples);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterClock)
};

} // namespace HAM