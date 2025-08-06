/*
  ==============================================================================

    ChannelManagerTests.cpp
    Tests for the ChannelManager buffer management and priority system

  ==============================================================================
*/

#include <JuceHeader.h>
#include "../Source/Domain/Services/ChannelManager.h"

using namespace HAM;

class ChannelManagerTests : public juce::UnitTest
{
public:
    ChannelManagerTests() : juce::UnitTest("ChannelManager Tests") {}
    
    void runTest() override
    {
        beginTest("Buffer Allocation");
        testBufferAllocation();
        
        beginTest("Priority Management");
        testPriorityManagement();
        
        beginTest("Buffer Pool Recycling");
        testBufferPoolRecycling();
        
        beginTest("Event Merging");
        testEventMerging();
        
        beginTest("Voice Stealing");
        testVoiceStealing();
        
        beginTest("Conflict Resolution");
        testConflictResolution();
        
        beginTest("Performance Optimization");
        testPerformanceOptimization();
        
        beginTest("Emergency Cleanup");
        testEmergencyCleanup();
    }
    
private:
    void testBufferAllocation()
    {
        ChannelManager manager;
        
        // Test basic allocation
        bool result = manager.assignTrackBuffer(0, ChannelManager::TrackPriority::Normal);
        expect(result, "Should allocate buffer for track 0");
        expect(manager.hasActiveBuffer(0), "Track 0 should have active buffer");
        expect(manager.getActiveBufferCount() == 1, "Should have 1 active buffer");
        
        // Test multiple allocations
        for (int i = 1; i < 10; ++i)
        {
            result = manager.assignTrackBuffer(i, ChannelManager::TrackPriority::Normal);
            expect(result, "Should allocate buffer for track " + juce::String(i));
        }
        expect(manager.getActiveBufferCount() == 10, "Should have 10 active buffers");
        
        // Test duplicate allocation (should return true, already allocated)
        result = manager.assignTrackBuffer(0, ChannelManager::TrackPriority::High);
        expect(result, "Should return true for already allocated track");
        expect(manager.getActiveBufferCount() == 10, "Count shouldn't change");
        
        // Test release
        manager.releaseTrackBuffer(0);
        expect(!manager.hasActiveBuffer(0), "Track 0 should not have buffer");
        expect(manager.getActiveBufferCount() == 9, "Should have 9 active buffers");
        
        // Test invalid track indices
        result = manager.assignTrackBuffer(-1, ChannelManager::TrackPriority::Normal);
        expect(!result, "Should not allocate for negative index");
        
        result = manager.assignTrackBuffer(200, ChannelManager::TrackPriority::Normal);
        expect(!result, "Should not allocate for out-of-range index");
    }
    
    void testPriorityManagement()
    {
        ChannelManager manager;
        
        // Assign tracks with different priorities
        manager.assignTrackBuffer(0, ChannelManager::TrackPriority::Critical);
        manager.assignTrackBuffer(1, ChannelManager::TrackPriority::High);
        manager.assignTrackBuffer(2, ChannelManager::TrackPriority::Normal);
        manager.assignTrackBuffer(3, ChannelManager::TrackPriority::Low);
        manager.assignTrackBuffer(4, ChannelManager::TrackPriority::Background);
        
        // Test priority retrieval
        auto assignment0 = manager.getTrackAssignment(0);
        expect(assignment0.priority == ChannelManager::TrackPriority::Critical,
               "Track 0 should have Critical priority");
        
        auto assignment4 = manager.getTrackAssignment(4);
        expect(assignment4.priority == ChannelManager::TrackPriority::Background,
               "Track 4 should have Background priority");
        
        // Test priority update
        manager.setTrackPriority(2, ChannelManager::TrackPriority::High);
        auto assignment2 = manager.getTrackAssignment(2);
        expect(assignment2.priority == ChannelManager::TrackPriority::High,
               "Track 2 priority should be updated to High");
        
        // Test tracks by priority
        auto sortedTracks = manager.getTracksByPriority();
        expect(sortedTracks.size() == 5, "Should have 5 active tracks");
        expect(sortedTracks[0] == 0, "First track should be Critical priority");
        expect(sortedTracks[4] == 4, "Last track should be Background priority");
    }
    
    void testBufferPoolRecycling()
    {
        ChannelManager manager;
        
        // Fill up the buffer pool
        const int maxBuffers = ChannelManager::MAX_BUFFER_POOL_SIZE;
        for (int i = 0; i < maxBuffers; ++i)
        {
            bool result = manager.assignTrackBuffer(i, 
                i == 0 ? ChannelManager::TrackPriority::Critical :
                ChannelManager::TrackPriority::Normal);
            expect(result, "Should allocate buffer " + juce::String(i));
        }
        
        expect(manager.getActiveBufferCount() == maxBuffers, 
               "Should have max buffers allocated");
        expect(manager.getAvailableBufferSlots() == 0,
               "Should have no available slots");
        
        // Try to allocate one more - should recycle
        bool result = manager.assignTrackBuffer(maxBuffers, 
                                               ChannelManager::TrackPriority::High);
        expect(result, "Should recycle buffer for new high-priority track");
        
        // Check that a non-critical buffer was recycled
        bool criticalStillActive = manager.hasActiveBuffer(0);
        expect(criticalStillActive, "Critical priority track should not be recycled");
        
        // Find which track was recycled
        int recycledTrack = -1;
        for (int i = 1; i < maxBuffers; ++i)
        {
            if (!manager.hasActiveBuffer(i))
            {
                recycledTrack = i;
                break;
            }
        }
        expect(recycledTrack > 0, "A normal priority track should have been recycled");
    }
    
    void testEventMerging()
    {
        ChannelManager manager;
        juce::MidiBuffer outputBuffer;
        
        // Create test events with different priorities
        std::vector<ChannelManager::PrioritizedEvent> events;
        
        // Critical priority note
        events.push_back({
            juce::MidiMessage::noteOn(1, 60, (juce::uint8)100),
            0,
            ChannelManager::TrackPriority::Critical,
            0,
            1.0f
        });
        
        // Low priority note at same time
        events.push_back({
            juce::MidiMessage::noteOn(1, 61, (juce::uint8)80),
            1,
            ChannelManager::TrackPriority::Low,
            0,
            1.0f
        });
        
        // High priority note later
        events.push_back({
            juce::MidiMessage::noteOn(1, 62, (juce::uint8)90),
            2,
            ChannelManager::TrackPriority::High,
            10,
            1.0f
        });
        
        // Merge events
        manager.mergeTrackEvents(events, outputBuffer, 100);
        
        // Check output
        int eventCount = 0;
        for (const auto meta : outputBuffer)
        {
            eventCount++;
        }
        expect(eventCount == 3, "All 3 events should be in output");
        
        // Test with limit
        outputBuffer.clear();
        manager.mergeTrackEvents(events, outputBuffer, 2);
        
        eventCount = 0;
        for (const auto meta : outputBuffer)
        {
            eventCount++;
        }
        expect(eventCount == 2, "Should be limited to 2 events");
    }
    
    void testVoiceStealing()
    {
        ChannelManager manager;
        juce::MidiBuffer outputBuffer;
        
        // Create events that exceed polyphony
        std::vector<ChannelManager::PrioritizedEvent> events;
        
        // Add 4 notes from different tracks with different priorities
        events.push_back({
            juce::MidiMessage::noteOn(1, 60, (juce::uint8)100),
            0,
            ChannelManager::TrackPriority::Critical,
            0,
            1.0f
        });
        
        events.push_back({
            juce::MidiMessage::noteOn(1, 61, (juce::uint8)80),
            1,
            ChannelManager::TrackPriority::Normal,
            5,
            1.0f
        });
        
        events.push_back({
            juce::MidiMessage::noteOn(1, 62, (juce::uint8)70),
            2,
            ChannelManager::TrackPriority::Low,
            10,
            1.0f
        });
        
        // This should steal from the low priority note
        events.push_back({
            juce::MidiMessage::noteOn(1, 63, (juce::uint8)90),
            3,
            ChannelManager::TrackPriority::High,
            15,
            1.0f
        });
        
        // Merge with max polyphony of 3
        manager.mergeWithVoiceStealing(events, outputBuffer, 3);
        
        // Check for note-off of stolen voice
        bool foundStolenNoteOff = false;
        for (const auto meta : outputBuffer)
        {
            if (meta.getMessage().isNoteOff() && 
                meta.getMessage().getNoteNumber() == 62)  // Low priority note
            {
                foundStolenNoteOff = true;
            }
        }
        expect(foundStolenNoteOff, "Should have note-off for stolen voice");
        
        // Check stats
        auto stats = manager.getPerformanceStats();
        expect(stats.voicesStolen > 0, "Should have stolen at least one voice");
    }
    
    void testConflictResolution()
    {
        ChannelManager manager;
        
        // Setup tracks with different priorities
        manager.assignTrackBuffer(0, ChannelManager::TrackPriority::Low);
        manager.assignTrackBuffer(1, ChannelManager::TrackPriority::High);
        manager.assignTrackBuffer(2, ChannelManager::TrackPriority::Critical);
        manager.assignTrackBuffer(3, ChannelManager::TrackPriority::Normal);
        
        // Test conflict resolution
        std::vector<int> conflictingTracks = {0, 1, 2, 3};
        int winner = manager.resolveResourceConflict(conflictingTracks);
        expect(winner == 2, "Critical priority track should win conflict");
        
        // Test without critical track
        conflictingTracks = {0, 1, 3};
        winner = manager.resolveResourceConflict(conflictingTracks);
        expect(winner == 1, "High priority track should win");
        
        // Test voice stealing selection
        std::vector<int> activeTracks = {0, 1, 2, 3};
        int victim = manager.selectVoiceToSteal(activeTracks);
        expect(victim != 2, "Should not steal from critical priority");
        expect(victim == 0 || victim == 3, "Should steal from lower priority");
        
        // Check stats
        auto stats = manager.getPerformanceStats();
        expect(stats.conflictsResolved >= 2, "Should have resolved conflicts");
    }
    
    void testPerformanceOptimization()
    {
        ChannelManager manager;
        
        // Allocate some buffers
        for (int i = 0; i < 10; ++i)
        {
            manager.assignTrackBuffer(i, ChannelManager::TrackPriority::Normal);
        }
        
        int initialCount = manager.getActiveBufferCount();
        expect(initialCount == 10, "Should have 10 active buffers");
        
        // Simulate time passing and optimize
        juce::Thread::sleep(100);  // Small delay
        manager.optimizeBufferAllocation();
        
        // Should still have all buffers (not enough time passed)
        expect(manager.getActiveBufferCount() == initialCount,
               "Buffers should not be released yet");
        
        // Test different allocation strategies
        manager.setAllocationStrategy(ChannelManager::AllocationStrategy::PreAllocated);
        manager.optimizeBufferAllocation();
        
        manager.setAllocationStrategy(ChannelManager::AllocationStrategy::Dynamic);
        manager.optimizeBufferAllocation();
        
        manager.setAllocationStrategy(ChannelManager::AllocationStrategy::Pooled);
        manager.optimizeBufferAllocation();
    }
    
    void testEmergencyCleanup()
    {
        ChannelManager manager;
        
        // Allocate many buffers
        for (int i = 0; i < 20; ++i)
        {
            manager.assignTrackBuffer(i, ChannelManager::TrackPriority::Normal);
        }
        
        // Perform emergency cleanup
        manager.performEmergencyCleanup();
        
        // Should have cleaned up inactive buffers
        // (In this test, they're all just allocated but not used)
        int remainingBuffers = manager.getActiveBufferCount();
        expect(remainingBuffers < 20, "Some buffers should be cleaned up");
        
        // Test buffer overflow handling
        manager.assignTrackBuffer(50, ChannelManager::TrackPriority::High);
        auto assignment = manager.getTrackAssignment(50);
        expect(assignment.priority == ChannelManager::TrackPriority::High,
               "Should have high priority initially");
        
        // Simulate overflow
        manager.handleBufferOverflow(50);
        assignment = manager.getTrackAssignment(50);
        expect(static_cast<int>(assignment.priority) > 
               static_cast<int>(ChannelManager::TrackPriority::High),
               "Priority should be lowered after overflow");
        
        // Reset stats and check
        auto statsBefore = manager.getPerformanceStats();
        manager.resetPerformanceStats();
        auto statsAfter = manager.getPerformanceStats();
        
        expect(statsAfter.bufferAllocations == 0, "Stats should be reset");
        expect(statsAfter.conflictsResolved == 0, "Stats should be reset");
        expect(statsAfter.voicesStolen == 0, "Stats should be reset");
    }
};

// Register the test
static ChannelManagerTests channelManagerTests;

// Main function to run tests
int main()
{
    juce::UnitTestRunner runner;
    runner.runAllTests();
    
    // Print results
    int numPassed = 0;
    int numFailed = 0;
    
    for (int i = 0; i < runner.getNumResults(); ++i)
    {
        auto* result = runner.getResult(i);
        if (result != nullptr)
        {
            numPassed += result->passes;
            numFailed += result->failures;
        }
    }
    
    std::cout << "\n========================================\n";
    std::cout << "ChannelManager Test Results:\n";
    std::cout << "  Passed: " << numPassed << "\n";
    std::cout << "  Failed: " << numFailed << "\n";
    std::cout << "========================================\n";
    
    return numFailed > 0 ? 1 : 0;
}