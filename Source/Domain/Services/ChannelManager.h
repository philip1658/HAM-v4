/*
  ==============================================================================

    ChannelManager.h
    Manages internal track buffer allocation, priority-based merging, and
    resource conflict resolution for the multi-track sequencer.
    
    All tracks ultimately output to MIDI channel 1 for plugin compatibility,
    but this manager handles the internal organization before merging.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <memory>
#include <vector>

namespace HAM {

//==============================================================================
/**
    Channel Manager for intelligent buffer management and event merging.
    
    Responsibilities:
    - Dynamic buffer pool allocation for active tracks
    - Priority-based event merging when multiple tracks fire simultaneously
    - Voice stealing coordination across tracks
    - Resource conflict resolution
    - Performance optimization for many tracks
*/
class ChannelManager
{
public:
    //==========================================================================
    // Constants
    static constexpr int MAX_TRACKS = 128;
    static constexpr int MAX_BUFFER_POOL_SIZE = 32;  // Active buffer pool
    static constexpr int MAX_EVENTS_PER_BLOCK = 1024;
    
    //==========================================================================
    // Priority levels for track merging
    enum class TrackPriority
    {
        Critical = 0,    // Never steal voices from these (e.g., lead)
        High = 1,        // Prefer to keep these playing
        Normal = 2,      // Standard priority
        Low = 3,         // First to lose voices if needed
        Background = 4   // Ambient/pad tracks
    };
    
    //==========================================================================
    // Buffer allocation strategy
    enum class AllocationStrategy
    {
        Dynamic,         // Allocate/deallocate as needed
        PreAllocated,    // Keep all buffers ready
        Pooled          // Use fixed pool with recycling
    };
    
    //==========================================================================
    // Track assignment info
    struct TrackAssignment
    {
        int trackIndex{-1};
        int bufferIndex{-1};
        TrackPriority priority{TrackPriority::Normal};
        bool isActive{false};
        int voiceCount{0};
        int lastActivityTime{0};
    };
    
    //==========================================================================
    // Event with priority info for merging
    struct PrioritizedEvent
    {
        juce::MidiMessage message;
        int trackIndex{0};
        TrackPriority priority{TrackPriority::Normal};
        int sampleOffset{0};
        float importance{1.0f};  // For fine-grained sorting
    };
    
    //==========================================================================
    // Construction
    ChannelManager();
    ~ChannelManager() = default;
    
    //==========================================================================
    // Track Management
    
    /** Assign a buffer to a track */
    bool assignTrackBuffer(int trackIndex, TrackPriority priority = TrackPriority::Normal);
    
    /** Release a track's buffer back to pool */
    void releaseTrackBuffer(int trackIndex);
    
    /** Update track priority */
    void setTrackPriority(int trackIndex, TrackPriority priority);
    
    /** Get track's current assignment */
    TrackAssignment getTrackAssignment(int trackIndex) const;
    
    //==========================================================================
    // Buffer Pool Management
    
    /** Set allocation strategy */
    void setAllocationStrategy(AllocationStrategy strategy);
    
    /** Get number of active buffers */
    int getActiveBufferCount() const { return m_activeBufferCount.load(); }
    
    /** Get available buffer slots */
    int getAvailableBufferSlots() const;
    
    /** Optimize buffer allocation based on usage patterns */
    void optimizeBufferAllocation();
    
    //==========================================================================
    // Event Merging
    
    /** Merge events from multiple tracks with priority handling */
    void mergeTrackEvents(const std::vector<PrioritizedEvent>& events,
                         juce::MidiBuffer& outputBuffer,
                         int maxEvents = MAX_EVENTS_PER_BLOCK);
    
    /** Process MIDI buffer through channel manager */
    void processMidiBuffer(juce::MidiBuffer& midiBuffer, int numSamples);
    
    /** Smart merge with voice stealing across tracks */
    void mergeWithVoiceStealing(const std::vector<PrioritizedEvent>& events,
                                juce::MidiBuffer& outputBuffer,
                                int maxPolyphony);
    
    //==========================================================================
    // Conflict Resolution
    
    /** Resolve conflicts when multiple tracks need same resource */
    int resolveResourceConflict(const std::vector<int>& trackIndices);
    
    /** Handle buffer overflow situations */
    void handleBufferOverflow(int trackIndex);
    
    /** Steal voices intelligently across tracks */
    int selectVoiceToSteal(const std::vector<int>& activeTrackIndices);
    
    //==========================================================================
    // Performance Monitoring
    
    struct PerformanceStats
    {
        std::atomic<int> bufferAllocations{0};
        std::atomic<int> bufferDeallocations{0};
        std::atomic<int> conflictsResolved{0};
        std::atomic<int> voicesStolen{0};
        std::atomic<int> eventsDropped{0};
        std::atomic<int> totalEventsProcessed{0};
        std::atomic<float> averageMergeTime{0.0f};
    };
    
    struct PerformanceSnapshot
    {
        int bufferAllocations;
        int bufferDeallocations;
        int conflictsResolved;
        int voicesStolen;
        int eventsDropped;
        int totalEventsProcessed;
        float averageMergeTime;
    };
    
    PerformanceSnapshot getPerformanceStats() const;
    void resetPerformanceStats();
    
    //==========================================================================
    // Utilities
    
    /** Get tracks sorted by priority */
    std::vector<int> getTracksByPriority() const;
    
    /** Check if track has active buffer */
    bool hasActiveBuffer(int trackIndex) const;
    
    /** Emergency cleanup - release all inactive buffers */
    void performEmergencyCleanup();
    
private:
    //==========================================================================
    // Internal buffer pool
    struct BufferSlot
    {
        std::atomic<bool> inUse{false};
        std::atomic<int> assignedTrack{-1};
        std::atomic<int> eventCount{0};
        juce::int64 lastAccessTime{0};
        std::unique_ptr<juce::MidiBuffer> buffer;
    };
    
    //==========================================================================
    // Internal state
    
    // Track assignments
    std::array<TrackAssignment, MAX_TRACKS> m_trackAssignments;
    
    // Buffer pool
    std::array<BufferSlot, MAX_BUFFER_POOL_SIZE> m_bufferPool;
    std::atomic<int> m_activeBufferCount{0};
    
    // Configuration
    std::atomic<AllocationStrategy> m_allocationStrategy{AllocationStrategy::Pooled};
    
    // Performance stats
    mutable PerformanceStats m_stats;
    
    // Timing
    juce::int64 m_currentTime{0};
    
    //==========================================================================
    // Internal methods
    
    /** Find free buffer slot */
    int findFreeBufferSlot() const;
    
    /** Recycle least recently used buffer */
    int recycleLRUBuffer();
    
    /** Calculate event importance for sorting */
    float calculateEventImportance(const PrioritizedEvent& event) const;
    
    /** Update timing information */
    void updateTiming();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelManager)
};

} // namespace HAM