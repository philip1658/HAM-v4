/*
  ==============================================================================

    AccumulatorEngine.h
    Accumulator engine for pitch transposition in HAM sequencer
    Provides cumulative pitch offsets per stage/pulse/ratchet

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../Models/Stage.h"
#include <atomic>
#include <array>

namespace HAM
{

/**
 * Accumulator engine for cumulative pitch transposition
 * Thread-safe for real-time audio processing
 */
class AccumulatorEngine
{
public:
    //==============================================================================
    // Configuration
    
    enum class AccumulatorMode
    {
        PER_STAGE,      // Accumulate once per stage
        PER_PULSE,      // Accumulate for each pulse
        PER_RATCHET,    // Accumulate for each ratchet
        PENDULUM,       // Accumulate up then down (ping-pong)
        MANUAL          // Only accumulate on manual trigger
    };
    
    enum class ResetStrategy
    {
        NEVER,          // Never reset automatically
        LOOP_END,       // Reset at pattern loop
        STAGE_COUNT,    // Reset after N stages
        VALUE_LIMIT,    // Reset when exceeding limit
        MANUAL          // Only reset on manual trigger
    };
    
    struct AccumulatorState
    {
        int currentValue;           // Current accumulator value
        int stepsSinceReset;       // Steps since last reset
        int lastStageProcessed;   // Last stage index processed
        int lastPulseProcessed;   // Last pulse index processed
        bool pendingReset;         // Reset pending for next step
    };
    
    //==============================================================================
    // Constructor/Destructor
    
    AccumulatorEngine();
    ~AccumulatorEngine() = default;
    
    //==============================================================================
    // Accumulator Processing
    
    /**
     * Process accumulator for current position
     * @param stageIndex Current stage (0-7)
     * @param pulseIndex Current pulse within stage (0-7)
     * @param ratchetIndex Current ratchet within pulse (0-7)
     * @param incrementValue Value to accumulate
     * @return Current accumulator value after processing
     */
    int processAccumulator(int stageIndex, 
                          int pulseIndex, 
                          int ratchetIndex,
                          int incrementValue = 1);
    
    /**
     * Get current accumulator value without processing
     * @return Current accumulator value
     */
    int getCurrentValue() const { return m_currentValue.load(); }
    
    /**
     * Manually increment accumulator
     * @param amount Amount to add (can be negative)
     */
    void increment(int amount = 1);
    
    /**
     * Reset accumulator to initial value
     * @param immediate If true, reset immediately; if false, reset on next step
     */
    void reset(bool immediate = true);
    
    //==============================================================================
    // Configuration
    
    /**
     * Set accumulator mode
     * @param mode The accumulation mode
     */
    void setMode(AccumulatorMode mode) { m_mode.store(static_cast<int>(mode)); }
    
    /**
     * Get accumulator mode
     * @return Current accumulation mode
     */
    AccumulatorMode getMode() const { return static_cast<AccumulatorMode>(m_mode.load()); }
    
    /**
     * Set reset strategy
     * @param strategy The reset strategy
     */
    void setResetStrategy(ResetStrategy strategy) { m_resetStrategy.store(static_cast<int>(strategy)); }
    
    /**
     * Get reset strategy
     * @return Current reset strategy
     */
    ResetStrategy getResetStrategy() const { return static_cast<ResetStrategy>(m_resetStrategy.load()); }
    
    /**
     * Set reset threshold (for STAGE_COUNT or VALUE_LIMIT strategies)
     * @param threshold The threshold value
     */
    void setResetThreshold(int threshold) { m_resetThreshold.store(threshold); }
    
    /**
     * Get reset threshold
     * @return Current reset threshold
     */
    int getResetThreshold() const { return m_resetThreshold.load(); }
    
    //==============================================================================
    // Range Control
    
    /**
     * Set value limits
     * @param min Minimum accumulator value
     * @param max Maximum accumulator value
     */
    void setValueLimits(int min, int max);
    
    /**
     * Get minimum value
     * @return Minimum accumulator value
     */
    int getMinValue() const { return m_minValue.load(); }
    
    /**
     * Get maximum value
     * @return Maximum accumulator value
     */
    int getMaxValue() const { return m_maxValue.load(); }
    
    /**
     * Set wrap mode (when exceeding limits)
     * @param wrap If true, wrap around; if false, clamp to limits
     */
    void setWrapMode(bool wrap) { m_wrapMode.store(wrap); }
    
    /**
     * Get wrap mode
     * @return True if wrapping enabled
     */
    bool isWrapMode() const { return m_wrapMode.load(); }
    
    //==============================================================================
    // State Management
    
    /**
     * Get current state snapshot
     * @return AccumulatorState structure
     */
    AccumulatorState getState() const;
    
    /**
     * Restore from state snapshot
     * @param state The state to restore
     */
    void setState(const AccumulatorState& state);
    
    /**
     * Mark pattern loop completed (for LOOP_END reset strategy)
     */
    void notifyLoopEnd();
    
    //==============================================================================
    // Real-time safe parameter updates
    
    void setInitialValue(int value) { m_initialValue.store(value); }
    int getInitialValue() const { return m_initialValue.load(); }
    
    void setStepSize(int size) { m_stepSize.store(size); }
    int getStepSize() const { return m_stepSize.load(); }
    
    //==============================================================================
    // Pendulum mode configuration
    
    void setPendulumRange(int min, int max);
    int getPendulumMin() const { return m_pendulumMin.load(); }
    int getPendulumMax() const { return m_pendulumMax.load(); }
    bool getPendulumDirection() const { return m_pendulumDirection.load(); }
    
private:
    //==============================================================================
    // Internal state (all atomic for thread safety)
    
    std::atomic<int> m_currentValue{0};
    std::atomic<int> m_initialValue{0};
    std::atomic<int> m_stepSize{1};
    std::atomic<int> m_mode{0};
    std::atomic<int> m_resetStrategy{0};
    std::atomic<int> m_resetThreshold{8};
    std::atomic<int> m_minValue{-24};
    std::atomic<int> m_maxValue{24};
    std::atomic<bool> m_wrapMode{false};
    std::atomic<bool> m_pendingReset{false};
    
    // Tracking state
    std::atomic<int> m_stepsSinceReset{0};
    std::atomic<int> m_lastStageIndex{-1};
    std::atomic<int> m_lastPulseIndex{-1};
    std::atomic<int> m_lastRatchetIndex{-1};
    
    // Pendulum mode state
    std::atomic<bool> m_pendulumDirection{true}; // true = up, false = down
    std::atomic<int> m_pendulumMin{0};
    std::atomic<int> m_pendulumMax{8};
    
    //==============================================================================
    // Internal helpers
    
    bool shouldAccumulate(int stageIndex, int pulseIndex, int ratchetIndex);
    void checkResetConditions();
    int applyLimits(int value);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AccumulatorEngine)
};

//==============================================================================
/**
 * Track accumulator - manages accumulator for a track
 */
class TrackAccumulator
{
public:
    TrackAccumulator();
    ~TrackAccumulator() = default;
    
    /**
     * Process accumulator for current track position
     * @param track The track being processed
     * @param currentStage Current stage index (0-7)
     * @param pulseInStage Current pulse within stage (0-7)
     * @param ratchetInPulse Current ratchet within pulse (0-7)
     * @return Current accumulator value
     */
    int processTrackAccumulator(
        const class Track* track,
        int currentStage,
        int pulseInStage,
        int ratchetInPulse);
    
    /**
     * Reset accumulator
     */
    void reset();
    
    /**
     * Notify pattern loop completed
     */
    void notifyLoopEnd();
    
    /** Get the accumulator engine for configuration */
    AccumulatorEngine& getEngine() { return m_accumulator; }
    
private:
    AccumulatorEngine m_accumulator;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackAccumulator)
};

} // namespace HAM