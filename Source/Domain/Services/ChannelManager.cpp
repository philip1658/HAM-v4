/*
  ==============================================================================

    ChannelManager.cpp
    Implementation of buffer management and priority-based event merging

  ==============================================================================
*/

#include "ChannelManager.h"
#include <algorithm>

namespace HAM {

//==============================================================================
ChannelManager::ChannelManager()
{
    // Initialize buffer pool based on strategy
    if (m_allocationStrategy == AllocationStrategy::PreAllocated)
    {
        for (auto& slot : m_bufferPool)
        {
            slot.buffer = std::make_unique<juce::MidiBuffer>();
        }
    }
    
    // Initialize track assignments
    for (int i = 0; i < MAX_TRACKS; ++i)
    {
        m_trackAssignments[i].trackIndex = i;
        m_trackAssignments[i].bufferIndex = -1;
        m_trackAssignments[i].isActive = false;
    }
}

//==============================================================================
// Track Management

bool ChannelManager::assignTrackBuffer(int trackIndex, TrackPriority priority)
{
    if (trackIndex < 0 || trackIndex >= MAX_TRACKS)
        return false;
    
    // Check if already assigned
    if (m_trackAssignments[trackIndex].bufferIndex >= 0)
        return true;
    
    // Update timing to ensure current time is valid
    updateTiming();
    
    // Find free buffer slot
    int bufferIndex = findFreeBufferSlot();
    
    // If no free slot, try recycling
    if (bufferIndex < 0)
    {
        bufferIndex = recycleLRUBuffer();
    }
    
    if (bufferIndex < 0)
        return false;  // No buffers available
    
    // Assign buffer to track
    auto& slot = m_bufferPool[bufferIndex];
    slot.inUse.store(true);
    slot.assignedTrack.store(trackIndex);
    slot.lastAccessTime = m_currentTime;
    
    // Create buffer if needed (lazy allocation)
    if (!slot.buffer)
    {
        slot.buffer = std::make_unique<juce::MidiBuffer>();
        m_stats.bufferAllocations.fetch_add(1);
    }
    
    // Update track assignment
    m_trackAssignments[trackIndex].bufferIndex = bufferIndex;
    m_trackAssignments[trackIndex].priority = priority;
    m_trackAssignments[trackIndex].isActive = true;
    m_trackAssignments[trackIndex].lastActivityTime = static_cast<int>(m_currentTime);
    
    m_activeBufferCount.fetch_add(1);
    return true;
}

void ChannelManager::releaseTrackBuffer(int trackIndex)
{
    if (trackIndex < 0 || trackIndex >= MAX_TRACKS)
        return;
    
    auto& assignment = m_trackAssignments[trackIndex];
    if (assignment.bufferIndex < 0)
        return;
    
    // Release buffer slot
    auto& slot = m_bufferPool[assignment.bufferIndex];
    slot.inUse.store(false);
    slot.assignedTrack.store(-1);
    slot.eventCount.store(0);
    
    if (slot.buffer)
    {
        slot.buffer->clear();
    }
    
    // Clear assignment
    assignment.bufferIndex = -1;
    assignment.isActive = false;
    assignment.voiceCount = 0;
    
    m_activeBufferCount.fetch_sub(1);
    m_stats.bufferDeallocations.fetch_add(1);
}

void ChannelManager::setTrackPriority(int trackIndex, TrackPriority priority)
{
    if (trackIndex >= 0 && trackIndex < MAX_TRACKS)
    {
        m_trackAssignments[trackIndex].priority = priority;
    }
}

ChannelManager::TrackAssignment ChannelManager::getTrackAssignment(int trackIndex) const
{
    if (trackIndex >= 0 && trackIndex < MAX_TRACKS)
    {
        return m_trackAssignments[trackIndex];
    }
    return {};
}

//==============================================================================
// Buffer Pool Management

void ChannelManager::setAllocationStrategy(AllocationStrategy strategy)
{
    m_allocationStrategy.store(strategy);
    
    // Pre-allocate if switching to that strategy
    if (strategy == AllocationStrategy::PreAllocated)
    {
        for (auto& slot : m_bufferPool)
        {
            if (!slot.buffer)
            {
                slot.buffer = std::make_unique<juce::MidiBuffer>();
            }
        }
    }
}

int ChannelManager::getAvailableBufferSlots() const
{
    int available = 0;
    for (const auto& slot : m_bufferPool)
    {
        if (!slot.inUse.load())
            available++;
    }
    return available;
}

void ChannelManager::optimizeBufferAllocation()
{
    updateTiming();
    
    // Release buffers that haven't been used recently
    const juce::int64 inactivityThreshold = 5000;  // 5 seconds
    
    for (int i = 0; i < MAX_TRACKS; ++i)
    {
        auto& assignment = m_trackAssignments[i];
        if (assignment.isActive && assignment.bufferIndex >= 0)
        {
            auto& slot = m_bufferPool[assignment.bufferIndex];
            if (m_currentTime - slot.lastAccessTime > inactivityThreshold)
            {
                releaseTrackBuffer(i);
            }
        }
    }
    
    // Deallocate unused buffers if using dynamic strategy
    if (m_allocationStrategy == AllocationStrategy::Dynamic)
    {
        for (auto& slot : m_bufferPool)
        {
            if (!slot.inUse.load() && slot.buffer)
            {
                slot.buffer.reset();
            }
        }
    }
}

//==============================================================================
// Event Merging

void ChannelManager::mergeTrackEvents(const std::vector<PrioritizedEvent>& events,
                                      juce::MidiBuffer& outputBuffer,
                                      int maxEvents)
{
    // Clear output
    outputBuffer.clear();
    
    if (events.empty())
        return;
    
    // Sort events by priority and importance
    std::vector<PrioritizedEvent> sortedEvents = events;
    std::sort(sortedEvents.begin(), sortedEvents.end(),
              [this](const PrioritizedEvent& a, const PrioritizedEvent& b)
              {
                  // First sort by sample offset (timing is critical)
                  if (a.sampleOffset != b.sampleOffset)
                      return a.sampleOffset < b.sampleOffset;
                  
                  // Then by priority
                  if (a.priority != b.priority)
                      return static_cast<int>(a.priority) < static_cast<int>(b.priority);
                  
                  // Finally by importance
                  return calculateEventImportance(a) > calculateEventImportance(b);
              });
    
    // Add events up to max limit
    int eventsAdded = 0;
    for (const auto& event : sortedEvents)
    {
        if (eventsAdded >= maxEvents)
        {
            m_stats.eventsDropped.fetch_add(1);
            continue;
        }
        
        outputBuffer.addEvent(event.message, event.sampleOffset);
        eventsAdded++;
    }
}

void ChannelManager::mergeWithVoiceStealing(const std::vector<PrioritizedEvent>& events,
                                           juce::MidiBuffer& outputBuffer,
                                           int maxPolyphony)
{
    outputBuffer.clear();
    
    // Track active notes per track
    struct ActiveNote
    {
        int trackIndex;
        int noteNumber;
        TrackPriority priority;
        int startTime;
    };
    
    std::vector<ActiveNote> activeNotes;
    activeNotes.reserve(maxPolyphony);
    
    // Process events in time order
    std::vector<PrioritizedEvent> sortedEvents = events;
    std::sort(sortedEvents.begin(), sortedEvents.end(),
              [](const PrioritizedEvent& a, const PrioritizedEvent& b)
              {
                  return a.sampleOffset < b.sampleOffset;
              });
    
    for (const auto& event : sortedEvents)
    {
        if (event.message.isNoteOn())
        {
            // Check if we need to steal a voice
            if (activeNotes.size() >= static_cast<size_t>(maxPolyphony))
            {
                // Find lowest priority note to steal
                auto victimIt = std::min_element(activeNotes.begin(), activeNotes.end(),
                    [](const ActiveNote& a, const ActiveNote& b)
                    {
                        // Steal from lower priority first
                        if (a.priority != b.priority)
                            return static_cast<int>(a.priority) > static_cast<int>(b.priority);
                        // Then oldest note
                        return a.startTime < b.startTime;
                    });
                
                if (victimIt != activeNotes.end())
                {
                    // Only steal if new note has equal or higher priority
                    if (static_cast<int>(event.priority) <= static_cast<int>(victimIt->priority))
                    {
                        // Send note off for stolen voice
                        auto noteOff = juce::MidiMessage::noteOff(1, victimIt->noteNumber);
                        outputBuffer.addEvent(noteOff, event.sampleOffset);
                        
                        // Replace with new note
                        victimIt->trackIndex = event.trackIndex;
                        victimIt->noteNumber = event.message.getNoteNumber();
                        victimIt->priority = event.priority;
                        victimIt->startTime = event.sampleOffset;
                        
                        m_stats.voicesStolen.fetch_add(1);
                    }
                    else
                    {
                        // New note has lower priority, drop it
                        m_stats.eventsDropped.fetch_add(1);
                        continue;
                    }
                }
            }
            else
            {
                // Add new active note
                activeNotes.push_back({
                    event.trackIndex,
                    event.message.getNoteNumber(),
                    event.priority,
                    event.sampleOffset
                });
            }
        }
        else if (event.message.isNoteOff())
        {
            // Remove from active notes
            auto it = std::find_if(activeNotes.begin(), activeNotes.end(),
                [&event](const ActiveNote& note)
                {
                    return note.trackIndex == event.trackIndex &&
                           note.noteNumber == event.message.getNoteNumber();
                });
            
            if (it != activeNotes.end())
            {
                activeNotes.erase(it);
            }
        }
        
        // Add event to output
        outputBuffer.addEvent(event.message, event.sampleOffset);
    }
}

//==============================================================================
// Conflict Resolution

int ChannelManager::resolveResourceConflict(const std::vector<int>& trackIndices)
{
    if (trackIndices.empty())
        return -1;
    
    // Find highest priority track
    int bestTrack = -1;
    TrackPriority bestPriority = TrackPriority::Background;
    
    for (int trackIndex : trackIndices)
    {
        if (trackIndex >= 0 && trackIndex < MAX_TRACKS)
        {
            const auto& assignment = m_trackAssignments[trackIndex];
            if (assignment.isActive && 
                static_cast<int>(assignment.priority) < static_cast<int>(bestPriority))
            {
                bestPriority = assignment.priority;
                bestTrack = trackIndex;
            }
        }
    }
    
    m_stats.conflictsResolved.fetch_add(1);
    return bestTrack;
}

void ChannelManager::handleBufferOverflow(int trackIndex)
{
    if (trackIndex < 0 || trackIndex >= MAX_TRACKS)
        return;
    
    auto& assignment = m_trackAssignments[trackIndex];
    
    // Lower priority if it's causing problems
    if (assignment.priority != TrackPriority::Background)
    {
        int newPriority = static_cast<int>(assignment.priority) + 1;
        assignment.priority = static_cast<TrackPriority>(
            std::min(newPriority, static_cast<int>(TrackPriority::Background))
        );
    }
    
    // Clear buffer if assigned
    if (assignment.bufferIndex >= 0)
    {
        auto& slot = m_bufferPool[assignment.bufferIndex];
        if (slot.buffer)
        {
            slot.buffer->clear();
            slot.eventCount.store(0);
        }
    }
}

int ChannelManager::selectVoiceToSteal(const std::vector<int>& activeTrackIndices)
{
    if (activeTrackIndices.empty())
        return -1;
    
    // Find track with lowest priority and most voices
    int victimTrack = -1;
    TrackPriority lowestPriority = TrackPriority::Critical;
    int maxVoices = 0;
    
    for (int trackIndex : activeTrackIndices)
    {
        if (trackIndex >= 0 && trackIndex < MAX_TRACKS)
        {
            const auto& assignment = m_trackAssignments[trackIndex];
            
            // Skip critical priority tracks
            if (assignment.priority == TrackPriority::Critical)
                continue;
            
            // Prefer tracks with lower priority or more voices
            if (static_cast<int>(assignment.priority) > static_cast<int>(lowestPriority) ||
                (assignment.priority == lowestPriority && assignment.voiceCount > maxVoices))
            {
                lowestPriority = assignment.priority;
                maxVoices = assignment.voiceCount;
                victimTrack = trackIndex;
            }
        }
    }
    
    return victimTrack;
}

//==============================================================================
// Performance Monitoring

ChannelManager::PerformanceSnapshot ChannelManager::getPerformanceStats() const
{
    return {
        m_stats.bufferAllocations.load(),
        m_stats.bufferDeallocations.load(),
        m_stats.conflictsResolved.load(),
        m_stats.voicesStolen.load(),
        m_stats.eventsDropped.load(),
        m_stats.totalEventsProcessed.load(),
        m_stats.averageMergeTime.load()
    };
}

void ChannelManager::resetPerformanceStats()
{
    m_stats.bufferAllocations.store(0);
    m_stats.bufferDeallocations.store(0);
    m_stats.conflictsResolved.store(0);
    m_stats.voicesStolen.store(0);
    m_stats.eventsDropped.store(0);
    m_stats.totalEventsProcessed.store(0);
    m_stats.averageMergeTime.store(0.0f);
}

//==============================================================================
// Utilities

std::vector<int> ChannelManager::getTracksByPriority() const
{
    std::vector<int> tracks;
    tracks.reserve(MAX_TRACKS);
    
    for (int i = 0; i < MAX_TRACKS; ++i)
    {
        if (m_trackAssignments[i].isActive)
        {
            tracks.push_back(i);
        }
    }
    
    // Sort by priority
    std::sort(tracks.begin(), tracks.end(),
              [this](int a, int b)
              {
                  return static_cast<int>(m_trackAssignments[a].priority) <
                         static_cast<int>(m_trackAssignments[b].priority);
              });
    
    return tracks;
}

bool ChannelManager::hasActiveBuffer(int trackIndex) const
{
    if (trackIndex >= 0 && trackIndex < MAX_TRACKS)
    {
        return m_trackAssignments[trackIndex].bufferIndex >= 0 &&
               m_trackAssignments[trackIndex].isActive;
    }
    return false;
}

void ChannelManager::performEmergencyCleanup()
{
    updateTiming();
    
    // Release all inactive buffers
    // In emergency situations, be more aggressive about cleanup
    for (int i = 0; i < MAX_TRACKS; ++i)
    {
        auto& assignment = m_trackAssignments[i];
        if (assignment.bufferIndex >= 0)
        {
            auto& slot = m_bufferPool[assignment.bufferIndex];
            
            // Check if truly inactive (no events)
            // In emergency cleanup, we don't wait for time threshold
            if (slot.eventCount.load() == 0)
            {
                releaseTrackBuffer(i);
            }
        }
    }
    
    // Clear all buffer contents
    for (auto& slot : m_bufferPool)
    {
        if (slot.buffer && !slot.inUse.load())
        {
            slot.buffer->clear();
        }
    }
}

//==============================================================================
// Internal methods

int ChannelManager::findFreeBufferSlot() const
{
    for (int i = 0; i < MAX_BUFFER_POOL_SIZE; ++i)
    {
        if (!m_bufferPool[i].inUse.load())
        {
            return i;
        }
    }
    return -1;
}

int ChannelManager::recycleLRUBuffer()
{
    updateTiming();
    
    // Find least recently used buffer
    int lruIndex = -1;
    juce::int64 oldestTime = m_currentTime;
    TrackPriority lowestPriority = TrackPriority::Critical;
    
    for (int i = 0; i < MAX_BUFFER_POOL_SIZE; ++i)
    {
        const auto& slot = m_bufferPool[i];
        if (slot.inUse.load())
        {
            int trackIndex = slot.assignedTrack.load();
            if (trackIndex >= 0 && trackIndex < MAX_TRACKS)
            {
                const auto& assignment = m_trackAssignments[trackIndex];
                
                // Don't recycle critical priority buffers
                if (assignment.priority == TrackPriority::Critical)
                    continue;
                
                // Find oldest or lowest priority
                if (static_cast<int>(assignment.priority) > static_cast<int>(lowestPriority) ||
                    (assignment.priority == lowestPriority && slot.lastAccessTime < oldestTime))
                {
                    lowestPriority = assignment.priority;
                    oldestTime = slot.lastAccessTime;
                    lruIndex = i;
                }
            }
        }
    }
    
    // Release the LRU buffer
    if (lruIndex >= 0)
    {
        int trackToRelease = m_bufferPool[lruIndex].assignedTrack.load();
        if (trackToRelease >= 0)
        {
            releaseTrackBuffer(trackToRelease);
        }
        return lruIndex;
    }
    
    return -1;
}

float ChannelManager::calculateEventImportance(const PrioritizedEvent& event) const
{
    float importance = event.importance;
    
    // Note-on events are more important
    if (event.message.isNoteOn())
    {
        importance *= 1.5f;
        
        // Higher velocity = more important
        importance *= (0.5f + 0.5f * (event.message.getVelocity() / 127.0f));
    }
    
    // CC and pitch bend are important for expression
    if (event.message.isController() || event.message.isPitchWheel())
    {
        importance *= 1.2f;
    }
    
    // Track priority affects importance
    importance *= (5.0f - static_cast<float>(event.priority)) / 5.0f;
    
    return importance;
}

void ChannelManager::updateTiming()
{
    m_currentTime = juce::Time::currentTimeMillis();
}

void ChannelManager::processMidiBuffer(juce::MidiBuffer& midiBuffer, int numSamples)
{
    // For now, just pass through the buffer
    // Full implementation would handle channel routing and merging
    
    // Update statistics
    m_stats.totalEventsProcessed.fetch_add(midiBuffer.getNumEvents());
}

} // namespace HAM