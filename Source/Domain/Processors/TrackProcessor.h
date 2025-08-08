/*
  ==============================================================================

    TrackProcessor.h
    Handles track-level logic: stage advancement, directions, skip conditions
    Extracted from SequencerEngine for better separation of concerns

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../Models/Track.h"
#include "../Models/Stage.h"
#include <atomic>
#include <memory>
#include <random>

namespace HAM
{

/**
 * Processes track state and stage advancement
 * Thread-safe for real-time audio processing
 */
class TrackProcessor
{
public:
    //==============================================================================
    // Direction modes for stage progression
    enum class Direction
    {
        FORWARD,        // Normal 1->8 progression
        REVERSE,        // Reverse 8->1 progression
        PING_PONG,      // Forward then reverse
        RANDOM,         // Random stage selection
        PENDULUM,       // Like ping-pong but skips ends on reverse
        SPIRAL          // Jump pattern (1,5,2,6,3,7,4,8)
    };
    
    //==============================================================================
    // Skip conditions for stages
    enum class SkipCondition
    {
        NONE,           // Never skip
        PROBABILITY,    // Skip based on probability
        EVERY_N,        // Skip every N iterations
        GATE_REST,      // Skip if gate type is REST
        MANUAL          // Skip based on manual flag
    };
    
    //==============================================================================
    // Constructor/Destructor
    
    TrackProcessor();
    ~TrackProcessor() = default;
    
    //==============================================================================
    // Stage Processing
    
    /**
     * Calculate next stage index based on direction mode
     * @param currentStage Current stage index (0-7)
     * @param track The track being processed
     * @return Next stage index
     */
    int calculateNextStage(int currentStage, const Track* track);
    
    /**
     * Check if stage should be skipped
     * @param stageIndex Stage to check (0-7)
     * @param track The track being processed
     * @return True if stage should be skipped
     */
    bool shouldSkipStage(int stageIndex, const Track* track);
    
    /**
     * Get effective pulse count for a stage (considering mono/poly mode)
     * @param stage The stage to process
     * @param voiceMode Current voice mode
     * @return Number of pulses to play
     */
    int getEffectivePulseCount(const Stage& stage, VoiceMode voiceMode);
    
    /**
     * Process stage transition
     * @param fromStage Previous stage index
     * @param toStage New stage index
     * @param track The track being processed
     */
    void processStageTransition(int fromStage, int toStage, Track* track);
    
    //==============================================================================
    // Direction Management
    
    /** Set playback direction */
    void setDirection(Direction dir) { m_direction.store(static_cast<int>(dir)); }
    
    /** Get current direction */
    Direction getDirection() const { return static_cast<Direction>(m_direction.load()); }
    
    /** Reset direction state (for ping-pong, etc.) */
    void resetDirectionState();
    
    //==============================================================================
    // Skip Management
    
    /** Set skip condition mode */
    void setSkipCondition(SkipCondition condition) { m_skipCondition.store(static_cast<int>(condition)); }
    
    /** Get skip condition */
    SkipCondition getSkipCondition() const { return static_cast<SkipCondition>(m_skipCondition.load()); }
    
    /** Set skip probability (0.0-1.0) */
    void setSkipProbability(float prob) { m_skipProbability.store(prob); }
    
    /** Set skip interval for EVERY_N mode */
    void setSkipInterval(int interval) { m_skipInterval.store(interval); }
    
    //==============================================================================
    // State Management
    
    /** Reset all processor state */
    void reset();
    
    /** Get current stage index */
    int getCurrentStage() const { return m_currentStage.load(); }
    
    /** Set current stage (for jump/reset) */
    void setCurrentStage(int stage);
    
    /** Get total stages processed */
    int getStagesProcessed() const { return m_stagesProcessed.load(); }
    
private:
    //==============================================================================
    // Internal state (atomic for thread safety)
    
    std::atomic<int> m_currentStage{0};
    std::atomic<int> m_direction{0};
    std::atomic<int> m_skipCondition{0};
    std::atomic<float> m_skipProbability{0.0f};
    std::atomic<int> m_skipInterval{4};
    std::atomic<int> m_stagesProcessed{0};
    
    // Direction state
    std::atomic<bool> m_pingPongForward{true};
    std::atomic<int> m_pendulumStep{0};
    
    // Skip state
    std::atomic<int> m_skipCounter{0};
    
    // Random number generation for random direction and skip probability
    std::mt19937 m_randomGenerator;
    std::uniform_int_distribution<int> m_stageDistribution{0, 7};
    std::uniform_real_distribution<float> m_probabilityDistribution{0.0f, 1.0f};
    
    //==============================================================================
    // Internal helpers
    
    int processForwardDirection(int currentStage);
    int processReverseDirection(int currentStage);
    int processPingPongDirection(int currentStage);
    int processRandomDirection();
    int processPendulumDirection(int currentStage);
    int processSpiralDirection(int currentStage);
    
    bool checkProbabilitySkip(float probability);
    bool checkEveryNSkip();
    
    // Spiral pattern lookup table
    static constexpr std::array<int, 8> s_spiralPattern = {0, 4, 1, 5, 2, 6, 3, 7};
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackProcessor)
};

} // namespace HAM