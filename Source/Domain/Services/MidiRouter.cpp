/*
  ==============================================================================

    MidiRouter.cpp
    Implementation of MIDI routing system

  ==============================================================================
*/

#include "MidiRouter.h"

namespace HAM {

//==============================================================================
MidiRouter::MidiRouter()
{
    // Initialize track buffers lazily to save memory
    // They'll be created as needed when tracks are used
}

//==============================================================================
// Track Buffer Management

void MidiRouter::clearAllBuffers()
{
    for (auto& buffer : m_trackBuffers)
    {
        if (buffer)
        {
            buffer->clear();
        }
    }
    
    resetStats();
}

void MidiRouter::clearTrackBuffer(int trackIndex)
{
    if (trackIndex >= 0 && trackIndex < MAX_TRACKS)
    {
        if (m_trackBuffers[trackIndex])
        {
            m_trackBuffers[trackIndex]->clear();
        }
    }
}

void MidiRouter::addEventToTrack(int trackIndex, const juce::MidiMessage& message, 
                                int sampleOffset)
{
    if (trackIndex < 0 || trackIndex >= MAX_TRACKS)
        return;
    
    // Ensure buffer exists
    ensureTrackBufferExists(trackIndex);
    
    auto& buffer = m_trackBuffers[trackIndex];
    if (!buffer || !buffer->enabled.load())
        return;
    
    // Write to lock-free FIFO
    int start1, size1, start2, size2;
    buffer->fifo.prepareToWrite(1, start1, size1, start2, size2);
    
    if (size1 > 0)
    {
        buffer->eventBuffer[start1] = { message, trackIndex, sampleOffset, false };
        buffer->fifo.finishedWrite(1);
        buffer->pendingEvents.fetch_add(1);
    }
    else
    {
        // Buffer full - track dropped event
        m_stats.eventsDropped.fetch_add(1);
    }
}

//==============================================================================
// Processing

void MidiRouter::processBlock(juce::MidiBuffer& outputBuffer, int numSamples)
{
    // Clear output buffer
    outputBuffer.clear();
    
    // Process each track
    int activeCount = 0;
    for (int i = 0; i < MAX_TRACKS; ++i)
    {
        if (m_trackBuffers[i] && m_trackBuffers[i]->enabled.load())
        {
            if (m_trackBuffers[i]->pendingEvents.load() > 0)
            {
                routeTrackEvents(i, outputBuffer, numSamples);
                activeCount++;
            }
        }
    }
    
    m_stats.activeTrackCount.store(activeCount);
}

void MidiRouter::routeTrackEvents(int trackIndex, juce::MidiBuffer& outputBuffer, 
                                 int numSamples)
{
    if (trackIndex < 0 || trackIndex >= MAX_TRACKS)
        return;
    
    auto& buffer = m_trackBuffers[trackIndex];
    if (!buffer)
        return;
    
    // Read all pending events from track buffer
    int numReady = buffer->fifo.getNumReady();
    if (numReady == 0)
        return;
    
    int start1, size1, start2, size2;
    buffer->fifo.prepareToRead(numReady, start1, size1, start2, size2);
    
    // Process first block
    for (int i = 0; i < size1; ++i)
    {
        const auto& event = buffer->eventBuffer[start1 + i];
        
        // Route to channel 1
        auto routedMessage = routeToChannel(event.message, OUTPUT_CHANNEL);
        int sampleOffset = juce::jlimit(0, numSamples - 1, event.sampleOffset);
        outputBuffer.addEvent(routedMessage, sampleOffset);
        
        // Add debug copy on channel 16
        if (m_debugEnabled.load())
        {
            addDebugEvent(event.message, trackIndex, outputBuffer, sampleOffset);
        }
        
        m_stats.totalEventsRouted.fetch_add(1);
    }
    
    // Process second block (if FIFO wrapped)
    for (int i = 0; i < size2; ++i)
    {
        const auto& event = buffer->eventBuffer[start2 + i];
        
        // Route to channel 1
        auto routedMessage = routeToChannel(event.message, OUTPUT_CHANNEL);
        int sampleOffset = juce::jlimit(0, numSamples - 1, event.sampleOffset);
        outputBuffer.addEvent(routedMessage, sampleOffset);
        
        // Add debug copy on channel 16
        if (m_debugEnabled.load())
        {
            addDebugEvent(event.message, trackIndex, outputBuffer, sampleOffset);
        }
        
        m_stats.totalEventsRouted.fetch_add(1);
    }
    
    buffer->fifo.finishedRead(size1 + size2);
    buffer->pendingEvents.fetch_sub(size1 + size2);
}

//==============================================================================
// Configuration

void MidiRouter::setTrackEnabled(int trackIndex, bool enabled)
{
    if (trackIndex >= 0 && trackIndex < MAX_TRACKS)
    {
        ensureTrackBufferExists(trackIndex);
        if (m_trackBuffers[trackIndex])
        {
            m_trackBuffers[trackIndex]->enabled.store(enabled);
        }
    }
}

bool MidiRouter::isTrackEnabled(int trackIndex) const
{
    if (trackIndex >= 0 && trackIndex < MAX_TRACKS)
    {
        if (m_trackBuffers[trackIndex])
        {
            return m_trackBuffers[trackIndex]->enabled.load();
        }
    }
    return false;
}

void MidiRouter::setTrackPriority(int trackIndex, int priority)
{
    if (trackIndex >= 0 && trackIndex < MAX_TRACKS)
    {
        ensureTrackBufferExists(trackIndex);
        if (m_trackBuffers[trackIndex])
        {
            m_trackBuffers[trackIndex]->priority.store(priority);
        }
    }
}

//==============================================================================
// Statistics

void MidiRouter::resetStats()
{
    m_stats.totalEventsRouted.store(0);
    m_stats.eventsDropped.store(0);
    m_stats.debugEventsSent.store(0);
    m_stats.activeTrackCount.store(0);
}

//==============================================================================
// Debug Helpers

int MidiRouter::getPendingEventCount(int trackIndex) const
{
    if (trackIndex >= 0 && trackIndex < MAX_TRACKS)
    {
        if (m_trackBuffers[trackIndex])
        {
            return m_trackBuffers[trackIndex]->pendingEvents.load();
        }
    }
    return 0;
}

bool MidiRouter::hasPendingEvents() const
{
    for (int i = 0; i < MAX_TRACKS; ++i)
    {
        if (getPendingEventCount(i) > 0)
            return true;
    }
    return false;
}

//==============================================================================
// Internal Methods

void MidiRouter::ensureTrackBufferExists(int trackIndex)
{
    if (trackIndex >= 0 && trackIndex < MAX_TRACKS)
    {
        if (!m_trackBuffers[trackIndex])
        {
            m_trackBuffers[trackIndex] = std::make_unique<TrackBuffer>();
        }
    }
}

juce::MidiMessage MidiRouter::routeToChannel(const juce::MidiMessage& message, 
                                            int channel) const
{
    // Create new message with target channel
    if (message.isNoteOn())
    {
        return juce::MidiMessage::noteOn(channel, 
                                        message.getNoteNumber(),
                                        message.getVelocity());
    }
    else if (message.isNoteOff())
    {
        return juce::MidiMessage::noteOff(channel,
                                         message.getNoteNumber(),
                                         message.getVelocity());
    }
    else if (message.isController())
    {
        return juce::MidiMessage::controllerEvent(channel,
                                                 message.getControllerNumber(),
                                                 message.getControllerValue());
    }
    else if (message.isPitchWheel())
    {
        return juce::MidiMessage::pitchWheel(channel,
                                            message.getPitchWheelValue());
    }
    else if (message.isAftertouch())
    {
        return juce::MidiMessage::aftertouchChange(channel,
                                                  message.getNoteNumber(),
                                                  message.getAfterTouchValue());
    }
    else if (message.isChannelPressure())
    {
        return juce::MidiMessage::channelPressureChange(channel,
                                                       message.getChannelPressureValue());
    }
    else if (message.isProgramChange())
    {
        return juce::MidiMessage::programChange(channel,
                                               message.getProgramChangeNumber());
    }
    
    // For other message types, return as-is with channel set
    auto newMessage = message;
    newMessage.setChannel(channel);
    return newMessage;
}

void MidiRouter::addDebugEvent(const juce::MidiMessage& originalMessage,
                              int trackIndex,
                              juce::MidiBuffer& outputBuffer,
                              int sampleOffset)
{
    // Create debug message on channel 16 with track info
    // We can encode track index in velocity or CC for debugging
    auto debugMessage = routeToChannel(originalMessage, DEBUG_CHANNEL);
    
    // Add a small offset to avoid exact collision with main event
    int debugOffset = juce::jmin(sampleOffset + 1, outputBuffer.getLastEventTime());
    outputBuffer.addEvent(debugMessage, debugOffset);
    
    // Also send a CC message with track index for identification
    auto trackIdMessage = juce::MidiMessage::controllerEvent(DEBUG_CHANNEL,
                                                            120, // CC120 for track ID
                                                            trackIndex % 128);
    outputBuffer.addEvent(trackIdMessage, debugOffset);
    
    m_stats.debugEventsSent.fetch_add(1);
}

} // namespace HAM