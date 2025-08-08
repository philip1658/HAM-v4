/*
  ==============================================================================

    TrackProcessor.cpp
    Track processing implementation

  ==============================================================================
*/

#include "TrackProcessor.h"
#include <algorithm>

namespace HAM
{

//==============================================================================
TrackProcessor::TrackProcessor()
    : m_randomGenerator(std::random_device{}())
{
}

//==============================================================================
int TrackProcessor::calculateNextStage(int currentStage, const Track* track)
{
    if (!track) return 0;
    
    int nextStage = currentStage;
    Direction dir = getDirection();
    
    switch (dir)
    {
        case Direction::FORWARD:
            nextStage = processForwardDirection(currentStage);
            break;
            
        case Direction::REVERSE:
            nextStage = processReverseDirection(currentStage);
            break;
            
        case Direction::PING_PONG:
            nextStage = processPingPongDirection(currentStage);
            break;
            
        case Direction::RANDOM:
            nextStage = processRandomDirection();
            break;
            
        case Direction::PENDULUM:
            nextStage = processPendulumDirection(currentStage);
            break;
            
        case Direction::SPIRAL:
            nextStage = processSpiralDirection(currentStage);
            break;
    }
    
    // Check for skip conditions
    while (shouldSkipStage(nextStage, track))
    {
        // Try next stage in sequence
        int prevStage = nextStage;
        nextStage = calculateNextStage(nextStage, track);
        
        // Prevent infinite loop
        if (nextStage == prevStage)
        {
            // All stages are skipped, reset to stage 0
            nextStage = 0;
            break;
        }
    }
    
    m_currentStage.store(nextStage);
    m_stagesProcessed.fetch_add(1);
    
    return nextStage;
}

//==============================================================================
bool TrackProcessor::shouldSkipStage(int stageIndex, const Track* track)
{
    if (!track || stageIndex < 0 || stageIndex >= 8)
        return false;
    
    const Stage& stage = track->getStage(stageIndex);
    SkipCondition condition = getSkipCondition();
    
    switch (condition)
    {
        case SkipCondition::NONE:
            return false;
            
        case SkipCondition::PROBABILITY:
            return checkProbabilitySkip(m_skipProbability.load());
            
        case SkipCondition::EVERY_N:
            return checkEveryNSkip();
            
        case SkipCondition::GATE_REST:
            return stage.getGateTypeAsInt() == 3; // REST type
            
        case SkipCondition::MANUAL:
            return stage.shouldSkipOnFirstLoop();
            
        default:
            return false;
    }
}

//==============================================================================
int TrackProcessor::getEffectivePulseCount(const Stage& stage, VoiceMode voiceMode)
{
    int pulseCount = stage.getPulseCount();
    
    // In POLY mode, we only play 1 pulse before advancing
    // In MONO mode, we play all pulses
    if (voiceMode == VoiceMode::POLY)
    {
        return 1;
    }
    
    return pulseCount;
}

//==============================================================================
void TrackProcessor::processStageTransition(int fromStage, int toStage, Track* track)
{
    if (!track) return;
    
    // Could trigger stage transition effects here
    // For now, just update any transition-related state
    
    // Reset skip counter if needed
    if (getSkipCondition() == SkipCondition::EVERY_N)
    {
        m_skipCounter.fetch_add(1);
    }
}

//==============================================================================
void TrackProcessor::resetDirectionState()
{
    m_pingPongForward.store(true);
    m_pendulumStep.store(0);
}

//==============================================================================
void TrackProcessor::reset()
{
    m_currentStage.store(0);
    m_stagesProcessed.store(0);
    m_skipCounter.store(0);
    resetDirectionState();
}

//==============================================================================
void TrackProcessor::setCurrentStage(int stage)
{
    m_currentStage.store(std::clamp(stage, 0, 7));
}

//==============================================================================
// Private helper methods
//==============================================================================

int TrackProcessor::processForwardDirection(int currentStage)
{
    return (currentStage + 1) % 8;
}

int TrackProcessor::processReverseDirection(int currentStage)
{
    return (currentStage - 1 + 8) % 8;
}

int TrackProcessor::processPingPongDirection(int currentStage)
{
    bool forward = m_pingPongForward.load();
    
    if (forward)
    {
        if (currentStage >= 7)
        {
            m_pingPongForward.store(false);
            return 6; // Start going back
        }
        return currentStage + 1;
    }
    else
    {
        if (currentStage <= 0)
        {
            m_pingPongForward.store(true);
            return 1; // Start going forward
        }
        return currentStage - 1;
    }
}

int TrackProcessor::processRandomDirection()
{
    return m_stageDistribution(m_randomGenerator);
}

int TrackProcessor::processPendulumDirection(int currentStage)
{
    // Pendulum skips the ends on reverse
    // Pattern: 0,1,2,3,4,5,6,7,6,5,4,3,2,1,0,1...
    bool forward = m_pingPongForward.load();
    
    if (forward)
    {
        if (currentStage >= 7)
        {
            m_pingPongForward.store(false);
            return 6; // Skip end, go to 6
        }
        return currentStage + 1;
    }
    else
    {
        if (currentStage <= 0)
        {
            m_pingPongForward.store(true);
            return 1; // Skip end, go to 1
        }
        return currentStage - 1;
    }
}

int TrackProcessor::processSpiralDirection(int currentStage)
{
    // Find current position in spiral pattern
    auto it = std::find(s_spiralPattern.begin(), s_spiralPattern.end(), currentStage);
    if (it != s_spiralPattern.end())
    {
        size_t index = std::distance(s_spiralPattern.begin(), it);
        return s_spiralPattern[(index + 1) % 8];
    }
    
    // If not found (shouldn't happen), default to forward
    return (currentStage + 1) % 8;
}

bool TrackProcessor::checkProbabilitySkip(float probability)
{
    return m_probabilityDistribution(m_randomGenerator) < probability;
}

bool TrackProcessor::checkEveryNSkip()
{
    int interval = m_skipInterval.load();
    int counter = m_skipCounter.load();
    
    return (counter % interval) == (interval - 1);
}

} // namespace HAM