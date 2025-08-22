/*
  ==============================================================================

    AccumulatorEngine.cpp
    Accumulator implementation for pitch transposition

  ==============================================================================
*/

#include "AccumulatorEngine.h"
#include "../Models/Track.h"
#include <algorithm>
#include <cmath>  // For std::round and std::abs

namespace HAM
{

//==============================================================================
AccumulatorEngine::AccumulatorEngine()
{
    // Initialize with sensible defaults
    m_currentValue.store(0);
    m_initialValue.store(0);
    m_stepSize.store(1);
    m_mode.store(static_cast<int>(AccumulatorMode::PER_STAGE));
    m_resetStrategy.store(static_cast<int>(ResetStrategy::LOOP_END));
    m_resetThreshold.store(8);
    m_minValue.store(-24);
    m_maxValue.store(24);
    m_wrapMode.store(false);
    m_pendingReset.store(false);
    
    // Initialize tracking state
    m_stepsSinceReset.store(0);
    m_lastStageIndex.store(-1);
    m_lastPulseIndex.store(-1);
    m_lastRatchetIndex.store(-1);
}

//==============================================================================
int AccumulatorEngine::processAccumulator(int stageIndex, 
                                         int pulseIndex, 
                                         int ratchetIndex,
                                         int incrementValue)
{
    // Check for pending reset
    if (m_pendingReset.load())
    {
        m_currentValue.store(m_initialValue.load());
        m_stepsSinceReset.store(0);
        m_pendingReset.store(false);
    }
    
    // Check if we should accumulate based on mode
    if (shouldAccumulate(stageIndex, pulseIndex, ratchetIndex))
    {
        int stepSize = m_stepSize.load();
        AccumulatorMode mode = getMode();
        
        // Handle PENDULUM mode specially
        if (mode == AccumulatorMode::PENDULUM)
        {
            int currentValue = m_currentValue.load();
            bool goingUp = m_pendulumDirection.load();
            int pendMin = m_pendulumMin.load();
            int pendMax = m_pendulumMax.load();
            
            // Calculate new value based on direction
            int newValue;
            if (goingUp)
            {
                newValue = currentValue + (incrementValue * stepSize);
                // Check if we hit the max and need to reverse
                if (newValue >= pendMax)
                {
                    newValue = pendMax;
                    m_pendulumDirection.store(false); // Change direction
                }
            }
            else
            {
                newValue = currentValue - (incrementValue * stepSize);
                // Check if we hit the min and need to reverse
                if (newValue <= pendMin)
                {
                    newValue = pendMin;
                    m_pendulumDirection.store(true); // Change direction
                }
            }
            
            m_currentValue.store(newValue);
        }
        else
        {
            // Normal accumulation for other modes
            int newValue = m_currentValue.load() + (incrementValue * stepSize);
            
            // Apply limits
            newValue = applyLimits(newValue);
            
            m_currentValue.store(newValue);
        }
        
        m_stepsSinceReset.fetch_add(1);
        
        // Check reset conditions
        checkResetConditions();
    }
    
    // Update tracking
    m_lastStageIndex.store(stageIndex);
    m_lastPulseIndex.store(pulseIndex);
    m_lastRatchetIndex.store(ratchetIndex);
    
    return m_currentValue.load();
}

//==============================================================================
void AccumulatorEngine::increment(int amount)
{
    int newValue = m_currentValue.load() + amount;
    newValue = applyLimits(newValue);
    m_currentValue.store(newValue);
    m_stepsSinceReset.fetch_add(1);
    checkResetConditions();
}

//==============================================================================
void AccumulatorEngine::reset(bool immediate)
{
    if (immediate)
    {
        m_currentValue.store(m_initialValue.load());
        m_stepsSinceReset.store(0);
        m_pendingReset.store(false);
        
        // Reset tracking state
        m_lastStageIndex.store(-1);
        m_lastPulseIndex.store(-1);
        m_lastRatchetIndex.store(-1);
        
        // Reset pendulum direction
        m_pendulumDirection.store(true); // Start going up
    }
    else
    {
        m_pendingReset.store(true);
    }
}

//==============================================================================
void AccumulatorEngine::setValueLimits(int min, int max)
{
    // Ensure valid range
    if (min > max)
        std::swap(min, max);
    
    m_minValue.store(min);
    m_maxValue.store(max);
    
    // Apply limits to current value
    int current = m_currentValue.load();
    current = applyLimits(current);
    m_currentValue.store(current);
}

//==============================================================================
AccumulatorEngine::AccumulatorState AccumulatorEngine::getState() const
{
    AccumulatorState state;
    state.currentValue = m_currentValue.load();
    state.stepsSinceReset = m_stepsSinceReset.load();
    state.lastStageProcessed = m_lastStageIndex.load();
    state.lastPulseProcessed = m_lastPulseIndex.load();
    state.pendingReset = m_pendingReset.load();
    return state;
}

//==============================================================================
void AccumulatorEngine::setState(const AccumulatorState& state)
{
    m_currentValue.store(state.currentValue);
    m_stepsSinceReset.store(state.stepsSinceReset);
    m_lastStageIndex.store(state.lastStageProcessed);
    m_lastPulseIndex.store(state.lastPulseProcessed);
    m_pendingReset.store(state.pendingReset);
}

//==============================================================================
void AccumulatorEngine::notifyLoopEnd()
{
    ResetStrategy strategy = getResetStrategy();
    if (strategy == ResetStrategy::LOOP_END)
    {
        reset(false); // Reset on next step
    }
}

//==============================================================================
bool AccumulatorEngine::shouldAccumulate(int stageIndex, int pulseIndex, int ratchetIndex)
{
    AccumulatorMode mode = getMode();
    
    switch (mode)
    {
        case AccumulatorMode::PER_STAGE:
            // Accumulate when stage changes
            return stageIndex != m_lastStageIndex.load();
            
        case AccumulatorMode::PER_PULSE:
            // Accumulate when pulse changes
            return (stageIndex != m_lastStageIndex.load()) ||
                   (pulseIndex != m_lastPulseIndex.load());
            
        case AccumulatorMode::PER_RATCHET:
            // Accumulate for every ratchet
            return (stageIndex != m_lastStageIndex.load()) ||
                   (pulseIndex != m_lastPulseIndex.load()) ||
                   (ratchetIndex != m_lastRatchetIndex.load());
            
        case AccumulatorMode::PENDULUM:
            // Accumulate when stage changes (like PER_STAGE)
            // Direction is handled in processAccumulator
            return stageIndex != m_lastStageIndex.load();
            
        case AccumulatorMode::MANUAL:
            // Only accumulate on manual trigger
            return false;
            
        default:
            return false;
    }
}

//==============================================================================
void AccumulatorEngine::checkResetConditions()
{
    ResetStrategy strategy = getResetStrategy();
    
    switch (strategy)
    {
        case ResetStrategy::STAGE_COUNT:
        {
            int threshold = m_resetThreshold.load();
            if (m_stepsSinceReset.load() >= threshold)
            {
                reset(false); // Reset on next step
            }
            break;
        }
            
        case ResetStrategy::VALUE_LIMIT:
        {
            int current = m_currentValue.load();
            int min = m_minValue.load();
            int max = m_maxValue.load();
            
            if (current <= min || current >= max)
            {
                reset(false); // Reset on next step
            }
            break;
        }
            
        case ResetStrategy::NEVER:
        case ResetStrategy::LOOP_END:
        case ResetStrategy::MANUAL:
        default:
            // No automatic reset
            break;
    }
}

//==============================================================================
int AccumulatorEngine::applyLimits(int value)
{
    int min = m_minValue.load();
    int max = m_maxValue.load();
    
    // Validate range
    if (min >= max)
    {
        // Invalid range, return min
        jassert(false);  // This shouldn't happen
        return min;
    }
    
    if (m_wrapMode.load())
    {
        // Wrap around with safe modulo
        int range = max - min + 1;
        if (range <= 0)
            return min;
        
        // More efficient wrapping using modulo
        if (value < min)
        {
            int offset = min - value;
            int wraps = (offset / range) + 1;
            value += wraps * range;
        }
        else if (value > max)
        {
            int offset = value - max;
            int wraps = (offset / range) + 1;
            value -= wraps * range;
        }
        
        // Final safety check
        jassert(value >= min && value <= max);
        return value;
    }
    else
    {
        // Clamp to limits
        return std::clamp(value, min, max);
    }
}

//==============================================================================
void AccumulatorEngine::setPendulumRange(int min, int max)
{
    // Ensure valid range
    if (min > max)
        std::swap(min, max);
    
    m_pendulumMin.store(min);
    m_pendulumMax.store(max);
    
    // If current value is outside range, clamp it
    int current = m_currentValue.load();
    if (current < min)
    {
        m_currentValue.store(min);
        m_pendulumDirection.store(true); // Go up
    }
    else if (current > max)
    {
        m_currentValue.store(max);
        m_pendulumDirection.store(false); // Go down
    }
}

//==============================================================================
// TrackAccumulator implementation
//==============================================================================

TrackAccumulator::TrackAccumulator()
{
}

int TrackAccumulator::processTrackAccumulator(
    const Track* track,
    int currentStage,
    int pulseInStage,
    int ratchetInPulse)
{
    if (!track)
        return 0;
    
    // Get accumulator settings from track (if available)
    // Use consistent increment value
    int incrementValue = 1;
    
    // Add validation
    jassert(currentStage >= 0 && currentStage < 128);  // Reasonable stage limit
    jassert(pulseInStage >= 0 && pulseInStage < 128);  // Reasonable pulse limit
    jassert(ratchetInPulse >= 0 && ratchetInPulse < 16); // Reasonable ratchet limit
    
    // Check if track has accumulator enabled
    if (track->getAccumulatorMode() != AccumulatorMode::OFF)
    {
        // Map track accumulator mode to engine mode
        switch (track->getAccumulatorMode())
        {
            case AccumulatorMode::STAGE:
                m_accumulator.setMode(AccumulatorEngine::AccumulatorMode::PER_STAGE);
                break;
            case AccumulatorMode::PULSE:
                m_accumulator.setMode(AccumulatorEngine::AccumulatorMode::PER_PULSE);
                break;
            case AccumulatorMode::RATCHET:
                m_accumulator.setMode(AccumulatorEngine::AccumulatorMode::PER_RATCHET);
                break;
            case AccumulatorMode::PENDULUM:
                m_accumulator.setMode(AccumulatorEngine::AccumulatorMode::PENDULUM);
                break;
            default:
                break;
        }
        
        // Process accumulator
        return m_accumulator.processAccumulator(currentStage, pulseInStage, ratchetInPulse, incrementValue);
    }
    
    return 0;
}

void TrackAccumulator::reset()
{
    m_accumulator.reset(true);
}

void TrackAccumulator::notifyLoopEnd()
{
    m_accumulator.notifyLoopEnd();
}

} // namespace HAM