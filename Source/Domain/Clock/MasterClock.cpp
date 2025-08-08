/*
  ==============================================================================

    MasterClock.cpp
    Implementation of the sample-accurate master clock

  ==============================================================================
*/

#include "MasterClock.h"
#include <algorithm>
#include <cmath>
#include <thread>

namespace HAM {

//==============================================================================
MasterClock::MasterClock()
{
    // Initialize with default 120 BPM
    updateSamplesPerPulse(44100.0);
}

MasterClock::~MasterClock()
{
    // Ensure clock is stopped
    stop();
    
    // Wait for any ongoing notifications to complete, then clear listeners
    while (m_isNotifying.load()) {
        // Brief spin wait for notifications to complete
        std::this_thread::yield();
    }
    m_listeners.clear();
}

//==============================================================================
// Transport Control

void MasterClock::start()
{
    bool expected = false;
    if (m_isRunning.compare_exchange_strong(expected, true))
    {
        notifyClockStart();
    }
}

void MasterClock::stop()
{
    bool expected = true;
    if (m_isRunning.compare_exchange_strong(expected, false))
    {
        notifyClockStop();
    }
}

void MasterClock::reset()
{
    m_currentPulse.store(0);
    m_currentBeat.store(0);
    m_currentBar.store(0);
    m_pulsePhase.store(0.0f);
    m_sampleCounter = 0.0;
    m_midiClockCounter = 0;
    
    notifyClockReset();
}

//==============================================================================
// Tempo Control

void MasterClock::setBPM(float bpm)
{
    // Clamp BPM to reasonable range
    bpm = std::clamp(bpm, 20.0f, 999.0f);
    
    if (m_tempoGlideEnabled && m_isRunning.load())
    {
        // Set up glide to new tempo
        m_targetBpm.store(bpm);
        
        float currentBpm = m_currentGlideBpm;
        float difference = bpm - currentBpm;
        
        // Calculate glide parameters
        int glideSamples = static_cast<int>((m_tempoGlideMs / 1000.0) * m_lastSampleRate);
        if (glideSamples > 0)
        {
            m_glideIncrement = difference / static_cast<float>(glideSamples);
            m_glideSamplesRemaining = glideSamples;
        }
        else
        {
            // Instant change if glide time is too short
            m_bpm.store(bpm);
            m_currentGlideBpm = bpm;
            updateSamplesPerPulse(m_lastSampleRate);
            notifyTempoChanged(bpm);
        }
    }
    else
    {
        // Instant tempo change
        m_bpm.store(bpm);
        m_targetBpm.store(bpm);
        m_currentGlideBpm = bpm;
        updateSamplesPerPulse(m_lastSampleRate);
        notifyTempoChanged(bpm);
    }
}

//==============================================================================
// Sample-Accurate Processing

void MasterClock::processBlock(double sampleRate, int numSamples)
{
    if (!m_isRunning.load())
        return;
    
    // Update sample rate if changed
    if (std::abs(sampleRate - m_lastSampleRate) > 0.01)
    {
        m_lastSampleRate = sampleRate;
        updateSamplesPerPulse(sampleRate);
    }
    
    // Process tempo glide if active
    if (m_glideSamplesRemaining > 0)
    {
        processTempoGlide(numSamples);
    }
    
    // Process samples and generate clock pulses
    int samplesProcessed = 0;
    
    while (samplesProcessed < numSamples)
    {
        // Calculate samples until next pulse
        double samplesUntilPulse = m_samplesPerPulse - m_sampleCounter;
        int samplesToProcess = std::min(
            static_cast<int>(std::ceil(samplesUntilPulse)),
            numSamples - samplesProcessed
        );
        
        // Advance sample counter
        m_sampleCounter += samplesToProcess;
        samplesProcessed += samplesToProcess;
        
        // Update pulse phase
        float phase = static_cast<float>(m_sampleCounter / m_samplesPerPulse);
        m_pulsePhase.store(std::clamp(phase, 0.0f, 1.0f));
        
        // Check if we've reached a pulse boundary
        if (m_sampleCounter >= m_samplesPerPulse)
        {
            // Reset counter for next pulse
            m_sampleCounter -= m_samplesPerPulse;
            
            // Advance pulse and notify listeners
            advancePulse();
        }
    }
}

void MasterClock::processTempoGlide(int numSamples)
{
    int samplesToProcess = std::min(m_glideSamplesRemaining, numSamples);
    
    // Apply glide increment
    m_currentGlideBpm += m_glideIncrement * static_cast<float>(samplesToProcess);
    m_glideSamplesRemaining -= samplesToProcess;
    
    // Update BPM and recalculate pulse timing
    m_bpm.store(m_currentGlideBpm);
    updateSamplesPerPulse(m_lastSampleRate);
    
    // Check if glide is complete
    if (m_glideSamplesRemaining <= 0)
    {
        m_bpm.store(m_targetBpm.load());
        m_currentGlideBpm = m_targetBpm.load();
        updateSamplesPerPulse(m_lastSampleRate);
        notifyTempoChanged(m_bpm.load());
    }
}

//==============================================================================
// Clock Query

int MasterClock::getSamplesUntilNextPulse(double sampleRate) const
{
    if (!m_isRunning.load())
        return 0;
    
    double samplesRemaining = m_samplesPerPulse - m_sampleCounter;
    return static_cast<int>(std::ceil(samplesRemaining));
}

bool MasterClock::isOnDivision(Division div) const
{
    int divPulses = static_cast<int>(div);
    int totalPulses = (m_currentBar.load() * 96) + 
                      (m_currentBeat.load() * 24) + 
                      m_currentPulse.load();
    
    return (totalPulses % divPulses) == 0;
}

int MasterClock::getNextDivisionPulse(Division div) const
{
    int divPulses = static_cast<int>(div);
    int totalPulses = (m_currentBar.load() * 96) + 
                      (m_currentBeat.load() * 24) + 
                      m_currentPulse.load();
    
    int nextDivision = ((totalPulses / divPulses) + 1) * divPulses;
    return nextDivision % 24;  // Return pulse within beat
}

double MasterClock::getSamplesPerDivision(Division div, float bpm, double sampleRate)
{
    // Calculate samples per quarter note
    double samplesPerQuarter = (60.0 / bpm) * sampleRate;
    
    // Scale by division
    double divisionFactor = static_cast<double>(div) / 24.0;
    return samplesPerQuarter * divisionFactor;
}

//==============================================================================
// Listener Management

void MasterClock::addListener(Listener* listener)
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

void MasterClock::removeListener(Listener* listener)
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
// MIDI Clock Sync

void MasterClock::processMidiClock()
{
    if (!m_externalSyncEnabled)
        return;
    
    // MIDI clock is 24 ppq - same as our internal resolution
    if (m_isRunning.load())
    {
        // Calculate tempo from MIDI clock interval
        int64_t currentTime = juce::Time::getHighResolutionTicks();
        
        if (m_lastMidiClockTime > 0)
        {
            double interval = juce::Time::highResolutionTicksToSeconds(currentTime - m_lastMidiClockTime);
            
            // Smooth the interval calculation
            if (m_midiClockInterval > 0)
            {
                m_midiClockInterval = (m_midiClockInterval * 0.9) + (interval * 0.1);
            }
            else
            {
                m_midiClockInterval = interval;
            }
            
            // Calculate BPM from interval (24 clocks per quarter note)
            double bpm = 60.0 / (m_midiClockInterval * 24.0);
            
            // Only update if BPM is reasonable
            if (bpm > 20.0 && bpm < 999.0)
            {
                m_bpm.store(static_cast<float>(bpm));
                updateSamplesPerPulse(m_lastSampleRate);
            }
        }
        
        m_lastMidiClockTime = currentTime;
        
        // Advance clock
        advancePulse();
    }
    
    m_midiClockCounter++;
}

void MasterClock::processMidiStart()
{
    if (m_externalSyncEnabled)
    {
        reset();
        start();
    }
}

void MasterClock::processMidiStop()
{
    if (m_externalSyncEnabled)
    {
        stop();
    }
}

void MasterClock::processMidiContinue()
{
    if (m_externalSyncEnabled)
    {
        start();
    }
}

//==============================================================================
// Internal Methods

void MasterClock::updateSamplesPerPulse(double sampleRate)
{
    // Calculate samples per pulse (24 ppq)
    // 60 seconds / BPM = seconds per beat
    // seconds per beat / 24 = seconds per pulse
    // seconds per pulse * sample rate = samples per pulse
    
    double secondsPerBeat = 60.0 / static_cast<double>(m_bpm.load());
    double secondsPerPulse = secondsPerBeat / 24.0;
    m_samplesPerPulse = secondsPerPulse * sampleRate;
}

void MasterClock::advancePulse()
{
    int pulse = m_currentPulse.load();
    int beat = m_currentBeat.load();
    int bar = m_currentBar.load();
    
    // Advance pulse
    pulse++;
    
    // Check for beat boundary (24 pulses per beat)
    if (pulse >= 24)
    {
        pulse = 0;
        beat++;
        
        // Check for bar boundary (4 beats per bar)
        if (beat >= 4)
        {
            beat = 0;
            bar++;
        }
    }
    
    // Update atomic values
    m_currentPulse.store(pulse);
    m_currentBeat.store(beat);
    m_currentBar.store(bar);
    
    // Notify listeners
    int totalPulse = (bar * 96) + (beat * 24) + pulse;
    notifyClockPulse(totalPulse);
}

void MasterClock::notifyClockPulse(int pulse)
{
    // Post to message thread to avoid blocking audio thread
    juce::MessageManager::callAsync([this, pulse]()
    {
        // Set notification flag (prevents listener list modification during notification)
        m_isNotifying.store(true);
        
        for (auto* listener : m_listeners)
        {
            if (listener != nullptr)
                listener->onClockPulse(pulse);
        }
        
        // Clear notification flag
        m_isNotifying.store(false);
    });
}

void MasterClock::notifyClockStart()
{
    juce::MessageManager::callAsync([this]()
    {
        m_isNotifying.store(true);
        
        for (auto* listener : m_listeners)
        {
            if (listener != nullptr)
                listener->onClockStart();
        }
        
        m_isNotifying.store(false);
    });
}

void MasterClock::notifyClockStop()
{
    juce::MessageManager::callAsync([this]()
    {
        m_isNotifying.store(true);
        
        for (auto* listener : m_listeners)
        {
            if (listener != nullptr)
                listener->onClockStop();
        }
        
        m_isNotifying.store(false);
    });
}

void MasterClock::notifyClockReset()
{
    juce::MessageManager::callAsync([this]()
    {
        m_isNotifying.store(true);
        
        for (auto* listener : m_listeners)
        {
            if (listener != nullptr)
                listener->onClockReset();
        }
        
        m_isNotifying.store(false);
    });
}

void MasterClock::notifyTempoChanged(float bpm)
{
    juce::MessageManager::callAsync([this, bpm]()
    {
        m_isNotifying.store(true);
        
        for (auto* listener : m_listeners)
        {
            if (listener != nullptr)
                listener->onTempoChanged(bpm);
        }
        
        m_isNotifying.store(false);
    });
}

} // namespace HAM