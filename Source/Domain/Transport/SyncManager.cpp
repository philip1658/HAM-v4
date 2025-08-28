/*
  ==============================================================================

    SyncManager.cpp
    Implementation of the sync management system

  ==============================================================================
*/

#include "SyncManager.h"
#include "../Clock/TimingConstants.h"
#include <chrono>

namespace HAM {

//==============================================================================
SyncManager::SyncManager(MasterClock& masterClock, Transport& transport)
    : m_masterClock(masterClock)
    , m_transport(transport)
{
    // Register as clock listener
    m_masterClock.addListener(this);
}

SyncManager::~SyncManager()
{
    // Unregister from clock
    m_masterClock.removeListener(this);
    
    // Close MIDI devices
    closeMidiInput();
    closeMidiOutput();
}

//==============================================================================
// MIDI Clock Input

void SyncManager::setMidiClockInputEnabled(bool enabled)
{
    m_receiveMidiClock = enabled;
    
    if (enabled)
    {
        // Switch transport to MIDI clock sync
        m_transport.setSyncMode(Transport::SyncMode::MIDI_CLOCK);
        m_masterClock.setExternalSyncEnabled(true);
    }
    else
    {
        // Switch back to internal sync
        m_transport.setSyncMode(Transport::SyncMode::INTERNAL);
        m_masterClock.setExternalSyncEnabled(false);
        
        // Reset statistics
        m_midiClockCounter = 0;
        m_totalClocksReceived = 0;
    }
}

void SyncManager::processMidiMessage(const juce::MidiMessage& message)
{
    if (!m_receiveMidiClock)
        return;
    
    const uint8_t* data = message.getRawData();
    
    switch (data[0])
    {
        case MIDI_CLOCK_TICK:
            processMidiClockTick();
            break;
            
        case MIDI_CLOCK_START:
            // Reset position and start
            m_transport.returnToZero();
            m_transport.play();
            m_midiClockCounter = 0;
            m_masterClock.processMidiStart();
            break;
            
        case MIDI_CLOCK_STOP:
            m_transport.stop(false);  // Don't return to zero
            m_masterClock.processMidiStop();
            break;
            
        case MIDI_CLOCK_CONTINUE:
            m_transport.play();
            m_masterClock.processMidiContinue();
            break;
            
        case MIDI_SONG_POSITION:
            if (message.getRawDataSize() >= 3)
            {
                // Song position is in 16th notes (6 MIDI clocks each)
                int sixteenths = (data[2] << 7) | data[1];
                m_songPositionSixteenths = sixteenths;
                
                // Convert to bars and beats (assuming 4/4)
                int bar = sixteenths / 16;
                int beat = (sixteenths % 16) / 4;
                m_transport.setPosition(bar, beat, 0);
            }
            break;
    }
}

void SyncManager::setMidiInputDevice(const juce::String& deviceName)
{
    if (deviceName != m_midiInputDevice)
    {
        closeMidiInput();
        
        if (deviceName.isNotEmpty())
        {
            openMidiInput(deviceName);
        }
        
        m_midiInputDevice = deviceName;
    }
}

//==============================================================================
// MIDI Clock Output

void SyncManager::setMidiClockOutputEnabled(bool enabled)
{
    m_sendMidiClock = enabled;
    
    if (!enabled && m_midiOutput)
    {
        // Send stop if disabling
        sendMidiStop();
    }
}

void SyncManager::setMidiOutputDevice(const juce::String& deviceName)
{
    if (deviceName != m_midiOutputDevice)
    {
        closeMidiOutput();
        
        if (deviceName.isNotEmpty())
        {
            openMidiOutput(deviceName);
        }
        
        m_midiOutputDevice = deviceName;
    }
}

void SyncManager::sendMidiStart()
{
    if (m_midiOutput)
    {
        m_midiOutput->sendMessageNow(juce::MidiMessage(MIDI_CLOCK_START));
    }
}

void SyncManager::sendMidiStop()
{
    if (m_midiOutput)
    {
        m_midiOutput->sendMessageNow(juce::MidiMessage(MIDI_CLOCK_STOP));
    }
}

void SyncManager::sendMidiContinue()
{
    if (m_midiOutput)
    {
        m_midiOutput->sendMessageNow(juce::MidiMessage(MIDI_CLOCK_CONTINUE));
    }
}

void SyncManager::sendSongPosition(int sixteenthNotes)
{
    if (m_midiOutput)
    {
        // Song position is 14-bit value (LSB, MSB)
        uint8_t lsb = sixteenthNotes & 0x7F;
        uint8_t msb = (sixteenthNotes >> 7) & 0x7F;
        
        juce::MidiMessage msg(MIDI_SONG_POSITION, lsb, msb);
        m_midiOutput->sendMessageNow(msg);
    }
}

//==============================================================================
// Ableton Link (Preparation)

void SyncManager::setLinkEnabled(bool enabled)
{
    m_linkEnabled = enabled;
    
    if (enabled)
    {
        // TODO: Initialize Link session
        // For now, just prepare the infrastructure
        m_transport.setSyncMode(Transport::SyncMode::ABLETON_LINK);
    }
    else
    {
        // TODO: Disable Link session
        m_transport.setSyncMode(Transport::SyncMode::INTERNAL);
    }
}

void SyncManager::alignLinkPhase()
{
    // TODO: Implement Link phase alignment
    // This will force all Link peers to align to the same phase
}

//==============================================================================
// Host Sync

void SyncManager::processHostPlayhead(const juce::AudioPlayHead::PositionInfo& info)
{
    if (!m_hostSyncEnabled)
        return;
    
    // Update transport position from host
    if (info.getPpqPosition().hasValue())
    {
        double ppq = *info.getPpqPosition();
        int totalPulses = static_cast<int>(ppq * 24.0);  // Convert to 24 PPQN
        
        int bar = totalPulses / 96;
        int remaining = totalPulses % 96;
        int beat = remaining / 24;
        int pulse = remaining % 24;
        
        m_transport.setPosition(bar, beat, pulse);
    }
    
    // Update BPM from host
    if (info.getBpm().hasValue())
    {
        m_masterClock.setBPM(static_cast<float>(*info.getBpm()));
    }
    
    // Update transport state from host
    if (info.getIsPlaying())
    {
        if (!m_transport.isPlaying())
            m_transport.play();
    }
    else
    {
        if (m_transport.isPlaying())
            m_transport.stop(false);
    }
    
    // Handle recording state
    if (info.getIsRecording())
    {
        if (!m_transport.isRecording())
            m_transport.record(false, 0);
    }
    else
    {
        if (m_transport.isRecording())
            m_transport.stopRecording();
    }
    
    // Update loop points
    if (info.getIsLooping() && info.getLoopPoints().hasValue())
    {
        auto loop = *info.getLoopPoints();
        int startBar = static_cast<int>(loop.ppqStart / 4.0);  // Convert from quarters to bars
        int endBar = static_cast<int>(loop.ppqEnd / 4.0);
        
        m_transport.setLoopPoints(startBar, endBar);
        m_transport.setLooping(true);
    }
}

//==============================================================================
// Sync Status

SyncManager::Status SyncManager::getStatus() const
{
    Status status;
    status.isReceivingMidiClock = m_receiveMidiClock;
    status.isSendingMidiClock = m_sendMidiClock;
    status.isLinkEnabled = m_linkEnabled;
    status.isLinkConnected = false;  // TODO: Implement with Link
    status.linkPeerCount = getLinkPeerCount();
    status.isMTCEnabled = m_mtcEnabled;
    status.isHostSyncEnabled = m_hostSyncEnabled;
    status.externalBPM = m_estimatedExternalBPM;
    status.internalBPM = m_masterClock.getBPM();
    status.clockDrift = m_clockDrift;
    status.droppedClocks = m_droppedClocks;
    status.totalClocksReceived = m_totalClocksReceived;
    return status;
}

void SyncManager::resetStatistics()
{
    m_droppedClocks = 0;
    m_totalClocksReceived = 0;
    m_clockDrift = 0.0;
    m_driftAccumulator = 0.0;
}

bool SyncManager::hasValidExternalClock() const
{
    // Consider clock valid if we've received ticks recently
    if (!m_receiveMidiClock)
        return false;
    
    int64_t now = juce::Time::getHighResolutionTicks();
    double timeSinceLastClock = juce::Time::highResolutionTicksToSeconds(now - m_lastMidiClockTime);
    
    // Clock is valid if we received a tick within last 100ms
    return timeSinceLastClock < 0.1;
}

void SyncManager::setDriftCompensationStrength(float strength)
{
    m_driftCompensationStrength = juce::jlimit(0.0f, 1.0f, strength);
}

//==============================================================================
// MasterClock::Listener implementation

void SyncManager::onClockPulse(int pulseNumber)
{
    // Send MIDI clock if enabled
    if (m_sendMidiClock && m_midiOutput)
    {
        sendMidiClockTick();
    }
    
    // Update transport position
    m_transport.processClock(pulseNumber);
}

void SyncManager::onClockStart()
{
    if (m_sendMidiClock)
    {
        sendMidiStart();
    }
}

void SyncManager::onClockStop()
{
    if (m_sendMidiClock)
    {
        sendMidiStop();
    }
}

void SyncManager::onClockReset()
{
    if (m_sendMidiClock)
    {
        sendSongPosition(0);
    }
}

void SyncManager::onTempoChanged(float newBPM)
{
    // Tempo change notification
    // Could be used to update Link tempo or other sync sources
}

//==============================================================================
// MidiInputCallback implementation

void SyncManager::handleIncomingMidiMessage(juce::MidiInput* source,
                                            const juce::MidiMessage& message)
{
    processMidiMessage(message);
}

//==============================================================================
// Internal Methods

void SyncManager::processMidiClockTick()
{
    m_totalClocksReceived++;
    m_midiClockCounter++;
    
    // Calculate timing
    int64_t now = juce::Time::getHighResolutionTicks();
    
    if (m_lastMidiClockTime > 0)
    {
        double interval = juce::Time::highResolutionTicksToSeconds(now - m_lastMidiClockTime);
        
        // Smooth the interval calculation
        if (m_midiClockInterval > 0)
        {
            m_midiClockInterval = (m_midiClockInterval * 0.9) + (interval * 0.1);
        }
        else
        {
            m_midiClockInterval = interval;
        }
        
        // Calculate BPM
        calculateExternalBPM();
        
        // Apply drift compensation if enabled
        if (m_driftCompensationEnabled)
        {
            applyDriftCompensation();
        }
    }
    
    m_lastMidiClockTime = now;
    
    // Process clock in master clock
    m_masterClock.processMidiClock();
}

void SyncManager::calculateExternalBPM()
{
    if (m_midiClockInterval <= 0) return;
    
    // Calculate BPM using timing constants for better accuracy
    double rawBpm = TimingConstants::calculateBPMFromInterval(m_midiClockInterval);
    
    // Enhanced validation with tighter constraints
    if (rawBpm < TimingConstants::MIN_BPM || rawBpm > TimingConstants::MAX_BPM)
    {
        // Invalid BPM - increment dropped clock counter
        m_droppedClocks++;
        return;
    }
    
    // Additional stability check - reject large jumps that could be jitter
    if (m_estimatedExternalBPM > 0)
    {
        double percentChange = std::abs(rawBpm - m_estimatedExternalBPM) / m_estimatedExternalBPM;
        if (percentChange > 0.1)  // Reject changes > 10%
        {
            // Large change detected - could be jitter or tempo change
            // Apply stricter filtering for stability
            rawBpm = m_estimatedExternalBPM + (rawBpm - m_estimatedExternalBPM) * 0.1;
        }
    }
    
    // Apply sophisticated smoothing filter to reduce jitter impact
    if (m_estimatedExternalBPM <= 0)
    {
        // First BPM reading - accept it directly
        m_estimatedExternalBPM = static_cast<float>(rawBpm);
    }
    else
    {
        // Apply exponential smoothing with dynamic filtering strength
        double smoothingFactor = TimingConstants::BPM_SMOOTHING_FACTOR;
        
        // Use stronger smoothing for small changes (reduce jitter)
        double bpmDelta = std::abs(rawBpm - m_estimatedExternalBPM);
        if (bpmDelta < 1.0)
        {
            smoothingFactor *= 0.5;  // Stronger smoothing for small jitter
        }
        
        // Apply filtered update
        double smoothedBpm = m_estimatedExternalBPM * (1.0 - smoothingFactor) + rawBpm * smoothingFactor;
        m_estimatedExternalBPM = static_cast<float>(smoothedBpm);
    }
}

void SyncManager::applyDriftCompensation()
{
    // Calculate expected vs actual interval
    double expectedInterval = 60.0 / (m_estimatedExternalBPM * 24.0);
    double drift = m_midiClockInterval - expectedInterval;
    
    // Accumulate drift
    m_driftAccumulator += drift;
    
    // Apply compensation based on strength
    double compensation = m_driftAccumulator * m_driftCompensationStrength;
    
    // Update clock drift for monitoring
    m_clockDrift = m_driftAccumulator * 1000.0;  // Convert to ms
    
    // Apply compensation to master clock timing to maintain sync
    if (std::abs(compensation) > 0.0001)  // Only apply significant compensation
    {
        // Calculate sample offset compensation
        double sampleRate = 48000.0;  // TODO: Get from master clock
        double sampleCompensation = compensation * sampleRate;
        
        // Apply gradual compensation to prevent audible glitches
        // Limit compensation to prevent large timing jumps
        sampleCompensation = std::clamp(sampleCompensation, -10.0, 10.0);
        
        // Apply compensation through MasterClock interface
        if (std::abs(sampleCompensation) >= 1.0)
        {
            // Apply drift compensation to master clock timing
            m_masterClock.applyDriftCompensation(sampleCompensation);
            
            // Reset accumulator after applying compensation
            m_driftAccumulator *= (1.0 - m_driftCompensationStrength);
        }
    }
}

void SyncManager::sendMidiClockTick()
{
    if (m_midiOutput)
    {
        m_midiOutput->sendMessageNow(juce::MidiMessage(MIDI_CLOCK_TICK));
    }
}

bool SyncManager::openMidiInput(const juce::String& deviceName)
{
    auto devices = juce::MidiInput::getAvailableDevices();
    
    for (auto& device : devices)
    {
        if (device.name == deviceName)
        {
            m_midiInput = juce::MidiInput::openDevice(device.identifier, this);
            
            if (m_midiInput)
            {
                m_midiInput->start();
                return true;
            }
        }
    }
    
    return false;
}

bool SyncManager::openMidiOutput(const juce::String& deviceName)
{
    auto devices = juce::MidiOutput::getAvailableDevices();
    
    for (auto& device : devices)
    {
        if (device.name == deviceName)
        {
            m_midiOutput = juce::MidiOutput::openDevice(device.identifier);
            return m_midiOutput != nullptr;
        }
    }
    
    return false;
}

void SyncManager::closeMidiInput()
{
    if (m_midiInput)
    {
        m_midiInput->stop();
        m_midiInput.reset();
    }
}

void SyncManager::closeMidiOutput()
{
    if (m_midiOutput)
    {
        // Send stop before closing
        if (m_sendMidiClock)
        {
            sendMidiStop();
        }
        
        m_midiOutput.reset();
    }
}


} // namespace HAM