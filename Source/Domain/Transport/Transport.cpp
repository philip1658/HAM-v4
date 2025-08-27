/*
  ==============================================================================

    Transport.cpp
    Implementation of the transport control system

  ==============================================================================
*/

#include "Transport.h"
#include <algorithm>
#include <thread>

namespace HAM {

//==============================================================================
Transport::Transport(MasterClock& clock)
    : m_clock(clock)
{
}

Transport::~Transport()
{
    // Ensure stopped
    stop(true);
    
    // Wait for any ongoing notifications to complete, then clear listeners
    while (m_isNotifying.load()) {
        std::this_thread::yield();
    }
    m_listeners.clear();
}

//==============================================================================
// Transport Control

void Transport::play()
{
    State currentState = m_state.load();
    juce::Logger::writeToLog("Transport::play() called - current state: " + juce::String(static_cast<int>(currentState)));
    
    // If already playing, ensure clock is also running (fix desync issues)
    if (currentState == State::PLAYING)
    {
        juce::Logger::writeToLog("Transport: Already playing, checking clock state");
        // Ensure clock is actually running
        if (m_syncMode == SyncMode::INTERNAL && !m_clock.isRunning())
        {
            juce::Logger::writeToLog("Transport: WARNING - Transport playing but clock stopped! Restarting clock.");
            m_clock.start();
        }
        return;
    }
    
    // Try to transition from STOPPED to PLAYING
    State expected = State::STOPPED;
    if (m_state.compare_exchange_strong(expected, State::PLAYING))
    {
        juce::Logger::writeToLog("Transport: State changed from STOPPED to PLAYING");
        // Start internal clock if using internal sync
        if (m_syncMode == SyncMode::INTERNAL)
        {
            juce::Logger::writeToLog("Transport: Starting internal clock");
            m_clock.start();
            
            // Verify clock actually started
            if (!m_clock.isRunning())
            {
                juce::Logger::writeToLog("Transport: ERROR - Clock failed to start! Retrying...");
                m_clock.stop(); // Clear any bad state
                m_clock.reset(); // Reset position
                m_clock.start(); // Try again
                
                if (!m_clock.isRunning())
                {
                    juce::Logger::writeToLog("Transport: CRITICAL - Clock won't start! Reverting transport state.");
                    m_state.store(State::STOPPED);
                    return;
                }
            }
            juce::Logger::writeToLog("Transport: Clock confirmed running");
        }
        
        notifyTransportStart();
        return;
    }
    
    // Try to resume from PAUSED
    expected = State::PAUSED;
    if (m_state.compare_exchange_strong(expected, State::PLAYING))
    {
        juce::Logger::writeToLog("Transport: State changed from PAUSED to PLAYING");
        if (m_syncMode == SyncMode::INTERNAL)
        {
            juce::Logger::writeToLog("Transport: Resuming internal clock");
            m_clock.start();
            
            // Verify clock actually started
            if (!m_clock.isRunning())
            {
                juce::Logger::writeToLog("Transport: ERROR - Clock failed to resume! Retrying...");
                m_clock.stop();
                m_clock.start();
            }
        }
        
        notifyTransportStart();
        return;
    }
    
    // If we're in any other state, log it but don't fail silently
    currentState = m_state.load();
    juce::Logger::writeToLog("Transport: Cannot play from current state: " + juce::String(static_cast<int>(currentState)));
}

void Transport::stop(bool returnToZero)
{
    State currentState = m_state.load();
    
    // Stop recording if active
    if (currentState == State::RECORDING)
    {
        stopRecording();
    }
    
    // Stop transport
    if (currentState != State::STOPPED)
    {
        m_state.store(State::STOPPED);
        
        // Stop internal clock
        if (m_syncMode == SyncMode::INTERNAL)
        {
            m_clock.stop();
        }
        
        // Return to zero if requested
        if (returnToZero)
        {
            this->returnToZero();
        }
        
        notifyTransportStop();
    }
}

void Transport::pause()
{
    State expected = State::PLAYING;
    if (m_state.compare_exchange_strong(expected, State::PAUSED))
    {
        // Pause internal clock
        if (m_syncMode == SyncMode::INTERNAL)
        {
            m_clock.stop();
        }
        
        notifyTransportPause();
        return;
    }
    
    // Also pause recording
    expected = State::RECORDING;
    if (m_state.compare_exchange_strong(expected, State::PAUSED))
    {
        if (m_syncMode == SyncMode::INTERNAL)
        {
            m_clock.stop();
        }
        
        notifyTransportPause();
    }
}

void Transport::togglePlayStop()
{
    if (isPlaying())
    {
        stop(false);
    }
    else
    {
        play();
    }
}

void Transport::record(bool useCountIn, int countInBars)
{
    if (useCountIn && countInBars > 0)
    {
        // Start count-in
        m_state.store(State::COUNT_IN);
        m_countInPulsesRemaining.store(countInBars * 96);  // 96 pulses per bar
        
        // Start clock for count-in
        if (m_syncMode == SyncMode::INTERNAL)
        {
            m_clock.start();
        }
    }
    else
    {
        // Start recording immediately
        m_state.store(State::RECORDING);
        
        if (m_syncMode == SyncMode::INTERNAL)
        {
            m_clock.start();
        }
        
        notifyRecordingStart();
    }
}

void Transport::stopRecording()
{
    State expected = State::RECORDING;
    if (m_state.compare_exchange_strong(expected, State::PLAYING))
    {
        notifyRecordingStop();
    }
}

//==============================================================================
// Position Control

void Transport::setPosition(int bar, int beat, int pulse)
{
    // Clamp values
    bar = std::max(0, bar);
    beat = juce::jlimit(0, m_timeSignatureNum.load() - 1, beat);
    pulse = juce::jlimit(0, 23, pulse);
    
    // Update position
    m_currentBar.store(bar);
    m_currentBeat.store(beat);
    m_currentPulse.store(pulse);
    
    // Calculate PPQ position
    int totalPulses = (bar * 96) + (beat * 24) + pulse;
    m_ppqPosition.store(totalPulses / 24.0);
    
    // Reset clock position if using internal sync
    if (m_syncMode == SyncMode::INTERNAL && !isPlaying())
    {
        m_clock.reset();
    }
    
    notifyPositionChanged();
}

void Transport::returnToZero()
{
    setPosition(0, 0, 0);
}

void Transport::moveByBars(int bars)
{
    int newBar = std::max(0, m_currentBar.load() + bars);
    setPosition(newBar, 0, 0);
}

Transport::Position Transport::getCurrentPosition() const
{
    Position pos;
    pos.bar = m_currentBar.load();
    pos.beat = m_currentBeat.load();
    pos.pulse = m_currentPulse.load();
    pos.ppqPosition = m_ppqPosition.load();
    pos.numerator = m_timeSignatureNum.load();
    pos.denominator = m_timeSignatureDenom.load();
    pos.bpm = m_clock.getBPM();
    pos.loopStartBar = m_loopStartBar.load();
    pos.loopEndBar = m_loopEndBar.load();
    pos.isLooping = m_isLooping.load();
    pos.isRecording = (m_state.load() == State::RECORDING);
    pos.isPunching = m_punchEnabled.load();
    pos.punchInBar = m_punchInBar.load();
    pos.punchOutBar = m_punchOutBar.load();
    return pos;
}

juce::String Transport::getPositionString() const
{
    return juce::String::formatted("%03d.%d.%02d",
        m_currentBar.load() + 1,  // Display bars starting from 1
        m_currentBeat.load() + 1,  // Display beats starting from 1
        m_currentPulse.load());
}

//==============================================================================
// Loop Control

void Transport::setLooping(bool shouldLoop)
{
    if (m_isLooping.exchange(shouldLoop) != shouldLoop)
    {
        notifyLoopStateChanged();
    }
}

void Transport::setLoopPoints(int startBar, int endBar)
{
    // Ensure valid loop points
    startBar = std::max(0, startBar);
    endBar = std::max(startBar + 1, endBar);  // At least 1 bar loop
    
    m_loopStartBar.store(startBar);
    m_loopEndBar.store(endBar);
}

//==============================================================================
// Punch In/Out

void Transport::setPunchEnabled(bool enabled)
{
    m_punchEnabled.store(enabled);
}

void Transport::setPunchPoints(int inBar, int outBar)
{
    inBar = std::max(0, inBar);
    outBar = std::max(inBar + 1, outBar);
    
    m_punchInBar.store(inBar);
    m_punchOutBar.store(outBar);
}

//==============================================================================
// Sync Control

void Transport::setSyncMode(SyncMode mode)
{
    SyncMode oldMode = m_syncMode.exchange(mode);
    
    if (oldMode != mode)
    {
        // Handle sync mode change
        if (mode == SyncMode::INTERNAL)
        {
            // Switch to internal clock
            m_clock.setExternalSyncEnabled(false);
            
            // Start clock if transport is playing
            if (isPlaying())
            {
                m_clock.start();
            }
        }
        else if (mode == SyncMode::MIDI_CLOCK)
        {
            // Switch to MIDI clock sync
            m_clock.setExternalSyncEnabled(true);
            
            // Stop internal clock
            m_clock.stop();
            
            // Wait for external sync
            if (isPlaying())
            {
                m_state.store(State::WAITING_FOR_SYNC);
            }
        }
        // Add other sync modes here (Ableton Link, MTC, etc.)
        
        notifySyncModeChanged();
    }
}

bool Transport::isExternalSync() const
{
    SyncMode mode = m_syncMode.load();
    return mode != SyncMode::INTERNAL;
}

void Transport::resync()
{
    if (isExternalSync())
    {
        // Reset position and wait for sync
        returnToZero();
        
        if (isPlaying())
        {
            m_state.store(State::WAITING_FOR_SYNC);
        }
    }
}

//==============================================================================
// Time Signature

void Transport::setTimeSignature(int numerator, int denominator)
{
    // Validate time signature
    numerator = juce::jlimit(1, 16, numerator);
    denominator = juce::jlimit(1, 16, denominator);
    
    // Ensure denominator is power of 2
    int closestPowerOf2 = 1;
    while (closestPowerOf2 < denominator)
        closestPowerOf2 *= 2;
    
    if (closestPowerOf2 > 16)
        closestPowerOf2 = 16;
    
    m_timeSignatureNum.store(numerator);
    m_timeSignatureDenom.store(closestPowerOf2);
}

//==============================================================================
// Listener Management

void Transport::addListener(Listener* listener)
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

void Transport::removeListener(Listener* listener)
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
// Clock Callback

void Transport::processClock(int totalPulses)
{
    // Only process if playing or recording
    State currentState = m_state.load();
    if (currentState != State::PLAYING && 
        currentState != State::RECORDING &&
        currentState != State::COUNT_IN)
    {
        return;
    }
    
    // Handle count-in
    if (currentState == State::COUNT_IN)
    {
        processCountIn();
        
        // Don't advance position during count-in
        if (m_state.load() == State::COUNT_IN)
            return;
    }
    
    // Update position
    updatePosition(totalPulses);
    
    // Process looping
    if (m_isLooping.load())
    {
        processLooping();
    }
    
    // Process punch in/out
    if (m_punchEnabled.load() && currentState == State::RECORDING)
    {
        processPunchInOut();
    }
    
    // Notify position change
    notifyPositionChanged();
}

//==============================================================================
// Internal Methods

void Transport::processLooping()
{
    int currentBar = m_currentBar.load();
    int loopEnd = m_loopEndBar.load();
    int loopStart = m_loopStartBar.load();
    
    // Check if we've reached the loop end
    if (currentBar >= loopEnd)
    {
        // Jump back to loop start
        setPosition(loopStart, 0, 0);
    }
}

void Transport::processPunchInOut()
{
    int currentBar = m_currentBar.load();
    int punchIn = m_punchInBar.load();
    int punchOut = m_punchOutBar.load();
    State currentState = m_state.load();
    
    // Check punch in
    if (currentBar >= punchIn && currentBar < punchOut)
    {
        if (currentState == State::PLAYING)
        {
            // Start recording at punch in
            m_state.store(State::RECORDING);
            notifyRecordingStart();
        }
    }
    // Check punch out
    else if (currentBar >= punchOut)
    {
        if (currentState == State::RECORDING)
        {
            // Stop recording at punch out
            m_state.store(State::PLAYING);
            notifyRecordingStop();
        }
    }
}

void Transport::processCountIn()
{
    int remaining = m_countInPulsesRemaining.load();
    
    if (remaining > 0)
    {
        remaining--;
        m_countInPulsesRemaining.store(remaining);
        
        // Check if count-in is complete
        if (remaining == 0)
        {
            // Start recording
            m_state.store(State::RECORDING);
            notifyRecordingStart();
        }
    }
}

void Transport::updatePosition(int totalPulses)
{
    // Calculate bar, beat, pulse from total pulses
    int bar = totalPulses / 96;
    int remaining = totalPulses % 96;
    int beat = remaining / 24;
    int pulse = remaining % 24;
    
    // Update position
    m_currentBar.store(bar);
    m_currentBeat.store(beat);
    m_currentPulse.store(pulse);
    m_ppqPosition.store(totalPulses / 24.0);
}

//==============================================================================
// Notifications

void Transport::notifyTransportStart()
{
    juce::MessageManager::callAsync([this]()
    {
        m_isNotifying.store(true);
        
        for (auto* listener : m_listeners)
        {
            if (listener != nullptr)
                listener->onTransportStart();
        }
        
        m_isNotifying.store(false);
    });
}

void Transport::notifyTransportStop()
{
    juce::MessageManager::callAsync([this]()
    {
        m_isNotifying.store(true);
        
        for (auto* listener : m_listeners)
        {
            if (listener != nullptr)
                listener->onTransportStop();
        }
        
        m_isNotifying.store(false);
    });
}

void Transport::notifyTransportPause()
{
    juce::MessageManager::callAsync([this]()
    {
        m_isNotifying.store(true);
        
        for (auto* listener : m_listeners)
        {
            if (listener != nullptr)
                listener->onTransportPause();
        }
        
        m_isNotifying.store(false);
    });
}

void Transport::notifyRecordingStart()
{
    juce::MessageManager::callAsync([this]()
    {
        m_isNotifying.store(true);
        
        for (auto* listener : m_listeners)
        {
            if (listener != nullptr)
                listener->onRecordingStart();
        }
        
        m_isNotifying.store(false);
    });
}

void Transport::notifyRecordingStop()
{
    juce::MessageManager::callAsync([this]()
    {
        m_isNotifying.store(true);
        
        for (auto* listener : m_listeners)
        {
            if (listener != nullptr)
                listener->onRecordingStop();
        }
        
        m_isNotifying.store(false);
    });
}

void Transport::notifyPositionChanged()
{
    Position pos = getCurrentPosition();
    
    juce::MessageManager::callAsync([this, pos]()
    {
        m_isNotifying.store(true);
        
        for (auto* listener : m_listeners)
        {
            if (listener != nullptr)
                listener->onPositionChanged(pos);
        }
        
        m_isNotifying.store(false);
    });
}

void Transport::notifySyncModeChanged()
{
    SyncMode mode = m_syncMode.load();
    
    juce::MessageManager::callAsync([this, mode]()
    {
        m_isNotifying.store(true);
        
        for (auto* listener : m_listeners)
        {
            if (listener != nullptr)
                listener->onSyncModeChanged(mode);
        }
        
        m_isNotifying.store(false);
    });
}

void Transport::notifyLoopStateChanged()
{
    bool looping = m_isLooping.load();
    
    juce::MessageManager::callAsync([this, looping]()
    {
        m_isNotifying.store(true);
        
        for (auto* listener : m_listeners)
        {
            if (listener != nullptr)
                listener->onLoopStateChanged(looping);
        }
        
        m_isNotifying.store(false);
    });
}

} // namespace HAM