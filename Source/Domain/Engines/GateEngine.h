/*
  ==============================================================================

    GateEngine.h
    Gate processing engine for HAM sequencer
    Handles gate types: MULTIPLE, HOLD, SINGLE, REST
    Supports ratcheting up to 8 subdivisions per pulse

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../Models/Stage.h"
#include <array>
#include <atomic>
#include <functional>
#include <random>

namespace HAM
{

/**
 * Gate engine for processing gate types and ratchets
 * Thread-safe for real-time audio processing
 */
class GateEngine
{
public:
    //==============================================================================
    // Gate Configuration
    
    enum class GateType
    {
        SUSTAINED,  // Single sustained note for entire stage duration (DEFAULT)
        MULTIPLE,   // Individual gate per ratchet
        HOLD,       // Single sustained gate across pulse
        SINGLE,     // Gate on first ratchet only
        REST        // No gate output
    };
    
    struct GateEvent
    {
        bool isNoteOn;
        int sampleOffset;
        float velocity;
        int ratchetIndex;
    };
    
    struct RatchetPattern
    {
        std::array<int, 8> subdivisions;  // 1-8 ratchets per pulse
        std::array<float, 8> velocities;  // Per-ratchet velocity
        std::array<float, 8> probabilities; // Per-ratchet probability
        int pulseCount;                     // Number of pulses (1-8)
    };
    
    //==============================================================================
    // Constructor/Destructor
    
    GateEngine();
    ~GateEngine() = default;
    
    //==============================================================================
    // Gate Processing
    
    /**
     * Process gate for a stage
     * @param stage The stage to process
     * @param pulseIndex Current pulse within stage (0-7)
     * @param sampleRate Current sample rate
     * @param samplesPerPulse Samples in one pulse
     * @return Vector of gate events for this pulse
     */
    std::vector<GateEvent> processStageGate(const Stage& stage, 
                                           int pulseIndex,
                                           double sampleRate,
                                           int samplesPerPulse);
    
    /**
     * Calculate gate length in samples
     * @param gateLength Normalized gate length (0.0-1.0)
     * @param samplesPerPulse Total samples in pulse
     * @param gateType Type of gate
     * @return Gate length in samples
     */
    int calculateGateLength(float gateLength, 
                          int samplesPerPulse,
                          GateType gateType);
    
    /**
     * Apply swing to gate timing
     * @param sampleOffset Original sample offset
     * @param swingAmount Swing amount (-50% to +50%)
     * @param isEvenBeat Whether this is an even beat
     * @return Adjusted sample offset
     */
    int applySwing(int sampleOffset, 
                  float swingAmount, 
                  bool isEvenBeat);
    
    //==============================================================================
    // Gate Pattern Management
    
    /**
     * Generate ratchet pattern for a pulse
     * @param ratchetCount Number of ratchets (1-8)
     * @param pulseLength Length of pulse in samples
     * @return Array of sample offsets for each ratchet
     */
    std::vector<int> generateRatchetPattern(int ratchetCount, 
                                           int pulseLength);
    
    /**
     * Check if gate should trigger based on probability
     * @param probability Probability value (0.0-1.0)
     * @return True if gate should trigger
     */
    bool shouldTrigger(float probability);
    
    //==============================================================================
    // Gate Morphing (for pattern interpolation)
    
    /**
     * Interpolate between two gate patterns
     * @param from Source gate pattern
     * @param to Target gate pattern
     * @param amount Morph amount (0.0-1.0)
     * @return Interpolated gate pattern
     */
    RatchetPattern morphGatePatterns(const RatchetPattern& from,
                                    const RatchetPattern& to,
                                    float amount);
    
    //==============================================================================
    // Configuration
    
    /** Set gate stretching mode */
    void setGateStretching(bool enabled) { m_gateStretchingEnabled.store(enabled); }
    
    /** Get gate stretching mode */
    bool isGateStretchingEnabled() const { return m_gateStretchingEnabled.load(); }
    
    /** Set minimum gate length in milliseconds */
    void setMinimumGateLength(float ms) { m_minGateLengthMs.store(ms); }
    
    /** Get minimum gate length */
    float getMinimumGateLength() const { return m_minGateLengthMs.load(); }
    
    //==============================================================================
    // Real-time safe parameter updates
    
    void setGlobalGateLength(float length) { m_globalGateLength.store(length); }
    float getGlobalGateLength() const { return m_globalGateLength.load(); }
    
    void setGlobalSwing(float swing) { m_globalSwing.store(swing); }
    float getGlobalSwing() const { return m_globalSwing.load(); }
    
private:
    //==============================================================================
    // Internal state (all atomic for thread safety)
    
    std::atomic<bool> m_gateStretchingEnabled{false};
    std::atomic<float> m_minGateLengthMs{1.0f};
    std::atomic<float> m_globalGateLength{0.9f};
    std::atomic<float> m_globalSwing{0.0f};
    
    // Random number generator for probability
    std::mt19937 m_randomGenerator;
    std::uniform_real_distribution<float> m_distribution{0.0f, 1.0f};
    
    //==============================================================================
    // Internal helpers
    
    GateType getGateTypeFromStage(const Stage& stage) const;
    float getEffectiveVelocity(const Stage& stage, int ratchetIndex) const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GateEngine)
};

//==============================================================================
/**
 * Track gate processor - manages gates for all 8 stages
 */
class TrackGateProcessor
{
public:
    TrackGateProcessor();
    ~TrackGateProcessor() = default;
    
    /**
     * Process all gates for a track
     * @param track The track to process
     * @param currentStage Current stage index (0-7)
     * @param pulseInStage Current pulse within stage (0-7)
     * @param sampleRate Current sample rate
     * @param samplesPerPulse Samples per pulse
     * @return Combined gate events for the track
     */
    std::vector<GateEngine::GateEvent> processTrackGates(
        const class Track* track,
        int currentStage,
        int pulseInStage,
        double sampleRate,
        int samplesPerPulse);
    
    /** Reset all gate states */
    void reset();
    
    /** Get the gate engine for configuration */
    GateEngine& getGateEngine() { return m_gateEngine; }
    
private:
    GateEngine m_gateEngine;
    
    // Track state for gate processing
    int m_lastProcessedStage{-1};
    int m_lastProcessedPulse{-1};
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackGateProcessor)
};

} // namespace HAM