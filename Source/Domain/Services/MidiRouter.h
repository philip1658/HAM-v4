/*
  ==============================================================================

    MidiRouter.h
    MIDI routing system for multi-track sequencer output.
    Routes all track MIDI to channel 1 for plugins, with debug on channel 16.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <memory>
#include "../../Application/Configuration.h"
#include "../Types/MidiRoutingTypes.h"

namespace HAM {

//==============================================================================
/**
    MIDI Router for managing multi-track MIDI output.
    
    Features:
    - Per-track MIDI buffers for clean separation
    - All tracks output on channel 1 (for plugin compatibility)
    - Debug monitoring on channel 16
    - Lock-free operation for real-time safety
    - Support for up to 128 tracks
*/
class MidiRouter
{
public:
    //==========================================================================
    // Constants
    static constexpr int MAX_TRACKS = 128;
    static constexpr int OUTPUT_CHANNEL = 1;    // All tracks output on ch1
    static constexpr int BUFFER_SIZE = 512;     // Events per track buffer
    
    //==========================================================================
    // Track Event for routing
    struct TrackEvent
    {
        juce::MidiMessage message;
        int trackIndex{0};
        int sampleOffset{0};
        bool processed{false};
    };
    
    //==========================================================================
    // Construction
    MidiRouter();
    ~MidiRouter() = default;
    
    //==========================================================================
    // Track Buffer Management
    
    /** Clear all track buffers */
    void clearAllBuffers();
    
    /** Clear specific track buffer */
    void clearTrackBuffer(int trackIndex);
    
    /** Add event to track buffer (lock-free) */
    void addEventToTrack(int trackIndex, const juce::MidiMessage& message, 
                        int sampleOffset);
    
    //==========================================================================
    // Processing
    
    /** Process all track buffers into output buffer */
    void processBlock(juce::MidiBuffer& outputBuffer, int numSamples);
    
    /** Route track events to channel 1 and debug channel */
    void routeTrackEvents(int trackIndex, juce::MidiBuffer& outputBuffer, 
                         int numSamples);
    
    //==========================================================================
    // Configuration
    
    /** Enable/disable debug channel output */
    void setDebugChannelEnabled(bool enabled) { m_debugEnabled = enabled; }
    bool isDebugChannelEnabled() const { return m_debugEnabled; }
    
    /** Set external MIDI output device */
    void setExternalMidiOutput(juce::MidiOutput* output) { m_externalMidiOutput = output; }
    juce::MidiOutput* getExternalMidiOutput() const { return m_externalMidiOutput; }
    
    /** Set MIDI routing mode for a track */
    void setTrackMidiRoutingMode(int trackIndex, MidiRoutingMode mode);
    MidiRoutingMode getTrackMidiRoutingMode(int trackIndex) const;
    
    /** Enable/disable specific track */
    void setTrackEnabled(int trackIndex, bool enabled);
    bool isTrackEnabled(int trackIndex) const;
    
    /** Set track priority for channel allocation */
    void setTrackPriority(int trackIndex, int priority);
    
    //==========================================================================
    // Statistics
    
    struct Stats
    {
        std::atomic<int> totalEventsRouted{0};
        std::atomic<int> eventsDropped{0};
        std::atomic<int> debugEventsSent{0};
        std::atomic<int> activeTrackCount{0};
    };
    
    // Get snapshot of stats (can't copy atomics directly)
    struct StatsSnapshot
    {
        int totalEventsRouted;
        int eventsDropped;
        int debugEventsSent;
        int activeTrackCount;
    };
    
    StatsSnapshot getStats() const 
    { 
        return { 
            m_stats.totalEventsRouted.load(),
            m_stats.eventsDropped.load(),
            m_stats.debugEventsSent.load(),
            m_stats.activeTrackCount.load()
        };
    }
    void resetStats();
    
    //==========================================================================
    // Debug Helpers
    
    /** Get pending events for a track (for debugging) */
    int getPendingEventCount(int trackIndex) const;
    
    /** Check if any track has pending events */
    bool hasPendingEvents() const;
    
private:
    //==========================================================================
    // Per-Track State
    struct TrackBuffer
    {
        // Lock-free FIFO for track events
        juce::AbstractFifo fifo{BUFFER_SIZE};
        std::array<TrackEvent, BUFFER_SIZE> eventBuffer;
        
        // Track configuration
        std::atomic<bool> enabled{true};
        std::atomic<int> priority{0};
        std::atomic<int> pendingEvents{0};
        
        // Clear the buffer
        void clear()
        {
            fifo.reset();
            pendingEvents.store(0);
        }
    };
    
    //==========================================================================
    // Internal State
    
    // Track buffers
    std::array<std::unique_ptr<TrackBuffer>, MAX_TRACKS> m_trackBuffers;
    
    // External MIDI output
    juce::MidiOutput* m_externalMidiOutput = nullptr;
    
    // Track routing modes
    std::array<MidiRoutingMode, MAX_TRACKS> m_trackRoutingModes;
    
    // Configuration
    // Default: enabled for test visibility; UI can toggle off in app
    std::atomic<bool> m_debugEnabled{true};
    
    // Statistics
    mutable Stats m_stats;
    
    //==========================================================================
    // Internal Methods
    
    /** Initialize track buffer if needed */
    void ensureTrackBufferExists(int trackIndex);
    
    /** Convert message to output channel */
    juce::MidiMessage routeToChannel(const juce::MidiMessage& message, 
                                    int channel) const;
    
    /** Add debug event */
    void addDebugEvent(const juce::MidiMessage& originalMessage,
                      int trackIndex,
                      juce::MidiBuffer& outputBuffer,
                      int sampleOffset);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiRouter)
};

} // namespace HAM