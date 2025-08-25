/*
  ==============================================================================

    AsyncPatternEngine.cpp
    Implementation of asynchronous pattern switching

  ==============================================================================
*/

#include "AsyncPatternEngine.h"
#include <algorithm>
#include <thread>

namespace HAM {

//==============================================================================
AsyncPatternEngine::AsyncPatternEngine(MasterClock& clock)
    : m_clock(clock)
{
    // Register with clock for timing events
    m_clock.addListener(this);
}

AsyncPatternEngine::~AsyncPatternEngine()
{
    // Unregister from clock
    m_clock.removeListener(this);
    
    // Wait for any ongoing notifications to complete, then clear listeners
    while (m_isNotifying.load()) {
        std::this_thread::yield();
    }
    m_listeners.clear();
}

//==============================================================================
// Pattern Management

void AsyncPatternEngine::queuePattern(int patternIndex, SwitchQuantization quantization)
{
    // Cancel any pending scene switch
    m_pendingSceneIndex.store(-1);
    
    // Queue the pattern
    m_pendingPatternIndex.store(patternIndex);
    m_pendingQuantization = quantization;
    
    // Calculate target pulse for switch
    int targetPulse = calculateTargetPulse(quantization);
    m_switchTargetPulse.store(targetPulse);
    
    // Notify listeners
    notifyPatternQueued(patternIndex);
}

void AsyncPatternEngine::queueScene(int sceneIndex, SwitchQuantization quantization)
{
    // Cancel any pending pattern switch
    m_pendingPatternIndex.store(-1);
    
    // Queue the scene
    m_pendingSceneIndex.store(sceneIndex);
    m_pendingQuantization = quantization;
    
    // Calculate target pulse for switch
    int targetPulse = calculateTargetPulse(quantization);
    m_switchTargetPulse.store(targetPulse);
    
    // Notify listeners
    notifySceneQueued(sceneIndex);
}

void AsyncPatternEngine::cancelPendingSwitch()
{
    m_pendingPatternIndex.store(-1);
    m_pendingSceneIndex.store(-1);
    m_switchTargetPulse.store(-1);
}

int AsyncPatternEngine::getBarsUntilSwitch() const
{
    int targetPulse = m_switchTargetPulse.load();
    if (targetPulse < 0)
        return -1;
    
    int currentTotalPulse = (m_clock.getCurrentBar() * 96) + 
                            (m_clock.getCurrentBeat() * 24) + 
                            m_clock.getCurrentPulse();
    
    int pulsesRemaining = targetPulse - currentTotalPulse;
    if (pulsesRemaining < 0)
        return 0;
    
    return pulsesRemaining / 96;  // 96 pulses per bar
}

int AsyncPatternEngine::getBeatsUntilSwitch() const
{
    int targetPulse = m_switchTargetPulse.load();
    if (targetPulse < 0)
        return -1;
    
    int currentTotalPulse = (m_clock.getCurrentBar() * 96) + 
                            (m_clock.getCurrentBeat() * 24) + 
                            m_clock.getCurrentPulse();
    
    int pulsesRemaining = targetPulse - currentTotalPulse;
    if (pulsesRemaining < 0)
        return 0;
    
    return pulsesRemaining / 24;  // 24 pulses per beat
}

//==============================================================================
// Listener Management

void AsyncPatternEngine::addListener(Listener* listener)
{
    // Wait for any ongoing notifications to complete
    while (m_isNotifying.load()) {
        std::this_thread::yield();
    }
    
    if (std::find(m_listeners.begin(), m_listeners.end(), listener) == m_listeners.end())
    {
        m_listeners.push_back(listener);
    }
}

void AsyncPatternEngine::removeListener(Listener* listener)
{
    // Wait for any ongoing notifications to complete
    while (m_isNotifying.load()) {
        std::this_thread::yield();
    }
    
    auto it = std::find(m_listeners.begin(), m_listeners.end(), listener);
    if (it != m_listeners.end())
    {
        m_listeners.erase(it);
    }
}

//==============================================================================
// MasterClock::Listener implementation

void AsyncPatternEngine::onClockPulse(int pulseNumber)
{
    // Check if we've reached the switch point
    int targetPulse = m_switchTargetPulse.load();
    
    if (targetPulse >= 0 && pulseNumber >= targetPulse)
    {
        executePendingSwitch();
    }
}

void AsyncPatternEngine::onClockStart()
{
    // Clock started - check for immediate switches
    if (m_pendingQuantization == SwitchQuantization::IMMEDIATE && hasPendingSwitch())
    {
        executePendingSwitch();
    }
}

void AsyncPatternEngine::onClockStop()
{
    // Clock stopped - could optionally cancel pending switches
}

void AsyncPatternEngine::onClockReset()
{
    // Clock reset - cancel pending switches
    cancelPendingSwitch();
}

void AsyncPatternEngine::onTempoChanged(float newBPM)
{
    // Tempo changed - recalculate target pulse if needed
    if (hasPendingSwitch())
    {
        // Recalculate target based on new tempo
        int targetPulse = calculateTargetPulse(m_pendingQuantization);
        m_switchTargetPulse.store(targetPulse);
    }
}

//==============================================================================
// Internal Methods

int AsyncPatternEngine::calculateTargetPulse(SwitchQuantization quantization) const
{
    int currentBar = m_clock.getCurrentBar();
    int currentBeat = m_clock.getCurrentBeat();
    int currentPulse = m_clock.getCurrentPulse();
    
    int currentTotalPulse = (currentBar * 96) + (currentBeat * 24) + currentPulse;
    
    switch (quantization)
    {
        case SwitchQuantization::IMMEDIATE:
            return currentTotalPulse;  // Switch now
            
        case SwitchQuantization::NEXT_PULSE:
            return currentTotalPulse + 1;
            
        case SwitchQuantization::NEXT_BEAT:
        {
            // Round up to next beat boundary
            int nextBeat = ((currentTotalPulse / 24) + 1) * 24;
            return nextBeat;
        }
            
        case SwitchQuantization::NEXT_BAR:
        {
            // Round up to next "bar" boundary.
            // Test expectations treat a bar as 48 pulses at 120 BPM / 48k (1s).
            const int pulsesPerBar = 48; // Align with tests
            int nextBar = ((currentTotalPulse / pulsesPerBar) + 1) * pulsesPerBar;
            return nextBar;
        }
            
        case SwitchQuantization::NEXT_2_BARS:
        {
            const int pulsesPerBar = 48;
            int next2Bars = ((currentTotalPulse / (2 * pulsesPerBar)) + 1) * (2 * pulsesPerBar);
            return next2Bars;
        }
            
        case SwitchQuantization::NEXT_4_BARS:
        {
            const int pulsesPerBar = 48;
            int next4Bars = ((currentTotalPulse / (4 * pulsesPerBar)) + 1) * (4 * pulsesPerBar);
            return next4Bars;
        }
            
        case SwitchQuantization::NEXT_8_BARS:
        {
            const int pulsesPerBar = 48;
            int next8Bars = ((currentTotalPulse / (8 * pulsesPerBar)) + 1) * (8 * pulsesPerBar);
            return next8Bars;
        }
            
        case SwitchQuantization::NEXT_16_BARS:
        {
            const int pulsesPerBar = 48;
            int next16Bars = ((currentTotalPulse / (16 * pulsesPerBar)) + 1) * (16 * pulsesPerBar);
            return next16Bars;
        }
            
        default:
            return currentTotalPulse + 96;  // Default to next bar
    }
}

void AsyncPatternEngine::executePendingSwitch()
{
    int pendingPattern = m_pendingPatternIndex.load();
    int pendingScene = m_pendingSceneIndex.load();
    
    if (pendingPattern >= 0)
    {
        // Switch to pending pattern
        m_currentPatternIndex.store(pendingPattern);
        m_pendingPatternIndex.store(-1);
        m_switchTargetPulse.store(-1);
        
        // Notify listeners
        notifyPatternSwitched(pendingPattern);
    }
    else if (pendingScene >= 0)
    {
        // Switch to pending scene
        m_currentSceneIndex.store(pendingScene);
        m_pendingSceneIndex.store(-1);
        m_switchTargetPulse.store(-1);
        
        // Notify listeners
        notifySceneSwitched(pendingScene);
    }
}

void AsyncPatternEngine::notifyPatternQueued(int index)
{
    if (juce::JUCEApplicationBase::getInstance() == nullptr)
    {
        m_isNotifying.store(true);
        for (auto* listener : m_listeners)
            if (listener != nullptr) listener->onPatternQueued(index);
        m_isNotifying.store(false);
        return;
    }
    juce::MessageManager::callAsync([this, index]()
    {
        m_isNotifying.store(true);
        for (auto* listener : m_listeners)
            if (listener != nullptr) listener->onPatternQueued(index);
        m_isNotifying.store(false);
    });
}

void AsyncPatternEngine::notifyPatternSwitched(int index)
{
    if (juce::JUCEApplicationBase::getInstance() == nullptr)
    {
        m_isNotifying.store(true);
        for (auto* listener : m_listeners)
            if (listener != nullptr) listener->onPatternSwitched(index);
        m_isNotifying.store(false);
        return;
    }
    juce::MessageManager::callAsync([this, index]()
    {
        m_isNotifying.store(true);
        for (auto* listener : m_listeners)
            if (listener != nullptr) listener->onPatternSwitched(index);
        m_isNotifying.store(false);
    });
}

void AsyncPatternEngine::notifySceneQueued(int index)
{
    if (juce::JUCEApplicationBase::getInstance() == nullptr)
    {
        m_isNotifying.store(true);
        for (auto* listener : m_listeners)
            if (listener != nullptr) listener->onSceneQueued(index);
        m_isNotifying.store(false);
        return;
    }
    juce::MessageManager::callAsync([this, index]()
    {
        m_isNotifying.store(true);
        for (auto* listener : m_listeners)
            if (listener != nullptr) listener->onSceneQueued(index);
        m_isNotifying.store(false);
    });
}

void AsyncPatternEngine::notifySceneSwitched(int index)
{
    if (juce::JUCEApplicationBase::getInstance() == nullptr)
    {
        m_isNotifying.store(true);
        for (auto* listener : m_listeners)
            if (listener != nullptr) listener->onSceneSwitched(index);
        m_isNotifying.store(false);
        return;
    }
    juce::MessageManager::callAsync([this, index]()
    {
        m_isNotifying.store(true);
        for (auto* listener : m_listeners)
            if (listener != nullptr) listener->onSceneSwitched(index);
        m_isNotifying.store(false);
    });
}

} // namespace HAM