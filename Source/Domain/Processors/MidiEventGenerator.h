/*
  ==============================================================================

    MidiEventGenerator.h
    Generates MIDI events from stage data with ratcheting and velocity control
    Extracted from SequencerEngine for better separation of concerns

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../Models/Stage.h"
#include "../Models/Track.h"
#include "../Engines/GateEngine.h"
#include "../Engines/PitchEngine.h"
#include <vector>
#include <memory>
#include <mutex>
#include <optional>

namespace HAM
{

/**
 * Generates MIDI events from sequencer data
 * Thread-safe for real-time audio processing
 */
class MidiEventGenerator
{
public:
    //==============================================================================
    // MIDI Event structure
    
    struct MidiEvent
    {
        juce::MidiMessage message;
        int sampleOffset;       // Sample position in buffer
        int channel;            // MIDI channel (1-16)
        int trackIndex;         // Source track index
        int stageIndex;         // Source stage index
        int ratchetIndex;       // Ratchet subdivision index
        float velocity;         // Normalized velocity (0.0-1.0)
    };
    
    //==============================================================================
    // Constructor/Destructor
    
    MidiEventGenerator();
    ~MidiEventGenerator() = default;
    
    //==============================================================================
    // Event Generation
    
    /**
     * Generate MIDI events for a stage
     * @param stage The stage to process
     * @param stageIndex Stage index (0-7)
     * @param track The track containing the stage
     * @param pulseIndex Current pulse in stage (0-7)
     * @param sampleRate Current sample rate
     * @param samplesPerPulse Samples in one pulse
     * @param bufferSize Total buffer size
     * @return Vector of MIDI events
     */
    std::vector<MidiEvent> generateStageEvents(
        const Stage& stage,
        int stageIndex,
        const Track* track,
        int pulseIndex,
        double sampleRate,
        int samplesPerPulse,
        int bufferSize);
    
    /**
     * Generate ratcheted MIDI events
     * @param baseNote MIDI note number
     * @param velocity Base velocity (0-127)
     * @param ratchetCount Number of ratchets
     * @param samplesPerPulse Samples in pulse
     * @param channel MIDI channel
     * @return Vector of ratcheted events
     */
    std::vector<MidiEvent> generateRatchetedEvents(
        int baseNote,
        int velocity,
        int ratchetCount,
        int samplesPerPulse,
        int channel);
    
    
    /**
     * Apply humanization to events
     * @param events Events to humanize
     * @param timingAmount Timing variation (0.0-1.0)
     * @param velocityAmount Velocity variation (0.0-1.0)
     */
    void applyHumanization(std::vector<MidiEvent>& events,
                          float timingAmount,
                          float velocityAmount);
    
    //==============================================================================
    // CC Generation
    
    /**
     * Generate CC events from stage modulation
     * @param stage The stage with modulation data
     * @param channel MIDI channel
     * @param sampleOffset Sample position
     * @return Vector of CC events
     */
    std::vector<MidiEvent> generateCCEvents(const Stage& stage,
                                           int channel,
                                           int sampleOffset);
    
    /**
     * Generate pitch bend events
     * @param stage The stage with pitch bend data
     * @param channel MIDI channel
     * @param sampleOffset Sample position
     * @return Pitch bend event (if any)
     */
    std::optional<MidiEvent> generatePitchBendEvent(const Stage& stage,
                                                    int channel,
                                                    int sampleOffset);
    
    //==============================================================================
    // Configuration
    
    /** Set global velocity scaling */
    void setGlobalVelocity(float velocity) { m_globalVelocity.store(velocity); }
    
    /** Get global velocity */
    float getGlobalVelocity() const { return m_globalVelocity.load(); }
    
    /** Set velocity randomization amount */
    void setVelocityRandomization(float amount) { m_velocityRandom.store(amount); }
    
    /** Set timing randomization amount */
    void setTimingRandomization(float amount) { m_timingRandom.store(amount); }
    
    /** Enable/disable CC output */
    void setCCEnabled(bool enabled) { m_ccEnabled.store(enabled); }
    
    //==============================================================================
    // Buffer Overflow Management
    
    /**
     * Get queued events from previous buffer overflow and clear the queue
     * @param bufferSize Current buffer size
     * @return Vector of events that should be processed in current buffer
     */
    std::vector<MidiEvent> getAndClearQueuedEvents(int bufferSize);
    
    /**
     * Check if there are queued events waiting
     * @return True if events are queued
     */
    bool hasQueuedEvents() const { return !m_queuedEvents.empty(); }
    
    //==============================================================================
    // Helper Components
    
    /** Get gate engine for configuration */
    GateEngine& getGateEngine() { return *m_gateEngine; }
    
    /** Get pitch engine for configuration */
    PitchEngine& getPitchEngine() { return *m_pitchEngine; }
    
private:
    //==============================================================================
    // Internal components
    
    std::unique_ptr<GateEngine> m_gateEngine;
    std::unique_ptr<PitchEngine> m_pitchEngine;
    
    //==============================================================================
    // Configuration (atomic for thread safety)
    
    std::atomic<float> m_globalVelocity{1.0f};
    std::atomic<float> m_velocityRandom{0.0f};
    std::atomic<float> m_timingRandom{0.0f};
    std::atomic<bool> m_ccEnabled{true};
    
    //==============================================================================
    // Random generation
    
    std::mt19937 m_randomGenerator;
    std::uniform_real_distribution<float> m_distribution{0.0f, 1.0f};
    std::normal_distribution<float> m_normalDistribution{0.0f, 1.0f};
    
    // JUCE Random for thread-safe random number generation
    juce::Random m_random;
    
    //==============================================================================
    // Buffer overflow queue
    
    std::vector<MidiEvent> m_queuedEvents;
    mutable std::mutex m_queueMutex;  // Protect queue access
    
    //==============================================================================
    // Internal helpers
    
    int applyVelocityRandomization(int baseVelocity);
    int applyTimingRandomization(int sampleOffset, int maxOffset);
    
    MidiEvent createNoteOnEvent(int note, int velocity, int channel, int sampleOffset);
    MidiEvent createNoteOffEvent(int note, int channel, int sampleOffset);
    MidiEvent createCCEvent(int ccNumber, int value, int channel, int sampleOffset);
    MidiEvent createPitchBendEvent(int value, int channel, int sampleOffset);
    
    /** Queue event for next buffer if it falls outside current buffer */
    void queueEventIfOverflow(MidiEvent& event, int bufferSize);
    
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiEventGenerator)
};

} // namespace HAM