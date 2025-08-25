/*
  ==============================================================================

    PatternScheduler.cpp
    Implementation of pattern scheduling and transitions

  ==============================================================================
*/

#include "PatternScheduler.h"

namespace HAM {

PatternScheduler::PatternScheduler()
    : m_currentPatternIndex(0)
    , m_queuedPatternIndex(-1)
    , m_transitionMode(TransitionMode::NextBar)
    , m_defaultTransitionMode(TransitionMode::NextBar)
    , m_pendingTransitionPulse(-1)
    , m_pendingTransitionBeat(-1)
{
}

PatternScheduler::~PatternScheduler() = default;

void PatternScheduler::queuePattern(int patternIndex, TransitionMode mode)
{
    m_queuedPatternIndex = patternIndex;
    m_transitionMode = mode;
    
    // Calculate transition timing
    calculateTransitionTiming(mode);
    
    // Notify callback
    if (m_queueCallback)
    {
        m_queueCallback(patternIndex);
    }
}

void PatternScheduler::clearQueue()
{
    m_queuedPatternIndex = -1;
    m_pendingTransitionPulse = -1;
    m_pendingTransitionBeat = -1;
}

void PatternScheduler::cancelTransition()
{
    clearQueue();
}

void PatternScheduler::processTransition(int currentPulse, int currentBeat)
{
    if (!hasQueuedPattern())
        return;
    
    bool shouldTransition = false;
    
    switch (m_transitionMode)
    {
        case TransitionMode::Immediate:
            shouldTransition = true;
            break;
            
        case TransitionMode::NextPulse:
            shouldTransition = true; // Transition on any pulse when queued
            break;
            
        case TransitionMode::NextBeat:
            shouldTransition = (currentPulse == 0); // Start of beat
            break;
            
        case TransitionMode::NextBar:
            shouldTransition = (currentBeat == 0 && currentPulse == 0); // Start of bar
            break;
            
        case TransitionMode::Next2Bars:
            // Check if we've reached the 2-bar boundary
            if (m_pendingTransitionBeat >= 0)
            {
                shouldTransition = (currentBeat == 0 && currentPulse == 0 && 
                                  m_barsElapsed >= 2);
            }
            break;
            
        case TransitionMode::Next4Bars:
            if (m_pendingTransitionBeat >= 0)
            {
                shouldTransition = (currentBeat == 0 && currentPulse == 0 && 
                                  m_barsElapsed >= 4);
            }
            break;
            
        case TransitionMode::Next8Bars:
            if (m_pendingTransitionBeat >= 0)
            {
                shouldTransition = (currentBeat == 0 && currentPulse == 0 && 
                                  m_barsElapsed >= 8);
            }
            break;
            
        case TransitionMode::Next16Bars:
            if (m_pendingTransitionBeat >= 0)
            {
                shouldTransition = (currentBeat == 0 && currentPulse == 0 && 
                                  m_barsElapsed >= 16);
            }
            break;
    }
    
    // Count bars for longer transitions
    if (currentBeat == 0 && currentPulse == 0)
    {
        m_barsElapsed++;
    }
    
    if (shouldTransition)
    {
        executeTransition();
    }
}

void PatternScheduler::setCurrentPattern(int patternIndex)
{
    int oldPattern = m_currentPatternIndex.load();
    m_currentPatternIndex = patternIndex;
    
    if (m_transitionCallback)
    {
        m_transitionCallback(oldPattern, patternIndex);
    }
}

void PatternScheduler::setTransitionMode(TransitionMode mode)
{
    m_transitionMode = mode;
}

void PatternScheduler::setDefaultTransitionMode(TransitionMode mode)
{
    m_defaultTransitionMode = mode;
}

void PatternScheduler::setPatternValidator(PatternValidator validator)
{
    m_patternValidator = validator;
}

void PatternScheduler::setTransitionCallback(TransitionCallback callback)
{
    m_transitionCallback = callback;
}

void PatternScheduler::setQueueCallback(QueueCallback callback)
{
    m_queueCallback = callback;
}

void PatternScheduler::calculateTransitionTiming(TransitionMode mode)
{
    // Reset bar counter when queueing
    m_barsElapsed = 0;
    
    // Store the transition requirements
    switch (mode)
    {
        case TransitionMode::Immediate:
        case TransitionMode::NextPulse:
            m_pendingTransitionPulse = 0; // Any pulse
            m_pendingTransitionBeat = -1;
            break;
            
        case TransitionMode::NextBeat:
            m_pendingTransitionPulse = 0; // Start of beat
            m_pendingTransitionBeat = -1; // Any beat
            break;
            
        case TransitionMode::NextBar:
        case TransitionMode::Next2Bars:
        case TransitionMode::Next4Bars:
        case TransitionMode::Next8Bars:
        case TransitionMode::Next16Bars:
            m_pendingTransitionPulse = 0;
            m_pendingTransitionBeat = 0; // Start of bar
            break;
    }
}

void PatternScheduler::executeTransition()
{
    if (!hasQueuedPattern())
        return;
    
    // Validate pattern if validator is set
    if (m_patternValidator)
    {
        if (!m_patternValidator(m_queuedPatternIndex))
        {
            clearQueue();
            return;
        }
    }
    
    int oldPattern = m_currentPatternIndex.load();
    int newPattern = m_queuedPatternIndex.load();
    
    m_currentPatternIndex = newPattern;
    clearQueue();
    
    // Notify callback
    if (m_transitionCallback)
    {
        m_transitionCallback(oldPattern, newPattern);
    }
}

} // namespace HAM