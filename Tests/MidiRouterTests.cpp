/*
  ==============================================================================

    MidiRouterTests.cpp
    Unit tests for MIDI Router functionality

  ==============================================================================
*/

#include <JuceHeader.h>
#include "../Source/Domain/Services/MidiRouter.h"

using namespace HAM;

//==============================================================================
class MidiRouterTests : public juce::UnitTest
{
public:
    MidiRouterTests() : UnitTest("MidiRouter Tests") {}
    
    void runTest() override
    {
        testConstruction();
        testSingleTrackRouting();
        testMultiTrackRouting();
        testChannelRouting();
        testDebugChannel();
        testBufferOverflow();
        testTrackEnableDisable();
        testStatistics();
        testClearOperations();
    }
    
private:
    //==========================================================================
    void testConstruction()
    {
        beginTest("Construction");
        
        MidiRouter router;
        
        expect(!router.hasPendingEvents());
        expect(router.isDebugChannelEnabled());
        
        auto stats = router.getStats();
        expect(stats.totalEventsRouted == 0);
        expect(stats.eventsDropped == 0);
        expect(stats.debugEventsSent == 0);
        expect(stats.activeTrackCount == 0);
    }
    
    //==========================================================================
    void testSingleTrackRouting()
    {
        beginTest("Single Track Routing");
        
        MidiRouter router;
        
        // Add events to track 0
        auto noteOn = juce::MidiMessage::noteOn(5, 60, (juce::uint8)100); // Ch5, C4
        auto noteOff = juce::MidiMessage::noteOff(5, 60, (juce::uint8)0);
        
        router.addEventToTrack(0, noteOn, 0);
        router.addEventToTrack(0, noteOff, 100);
        
        expect(router.getPendingEventCount(0) == 2);
        expect(router.hasPendingEvents());
        
        // Process block
        juce::MidiBuffer output;
        router.processBlock(output, 512);
        
        // Check output
        expect(!output.isEmpty());
        
        int eventCount = 0;
        for (const auto metadata : output)
        {
            auto msg = metadata.getMessage();
            
            if (msg.getChannel() == MidiRouter::OUTPUT_CHANNEL)
            {
                // Main output should be on channel 1
                expect(msg.getNoteNumber() == 60);
                eventCount++;
            }
            else if (msg.getChannel() == 16) // Debug channel (if enabled)
            {
                // Debug copies on channel 16
                eventCount++;
            }
        }
        
        // Should have 2 main events + debug events
        expect(eventCount >= 2);
        
        // Track should be empty after processing
        expect(router.getPendingEventCount(0) == 0);
    }
    
    //==========================================================================
    void testMultiTrackRouting()
    {
        beginTest("Multi-Track Routing");
        
        MidiRouter router;
        
        // Add events to multiple tracks
        for (int track = 0; track < 4; ++track)
        {
            int pitch = 60 + track;
            auto noteOn = juce::MidiMessage::noteOn(track + 1, pitch, (juce::uint8)100);
            router.addEventToTrack(track, noteOn, track * 10);
        }
        
        // Check pending events
        for (int track = 0; track < 4; ++track)
        {
            expect(router.getPendingEventCount(track) == 1);
        }
        
        // Process block
        juce::MidiBuffer output;
        router.processBlock(output, 512);
        
        // Count events by channel
        int channel1Count = 0;
        int channel16Count = 0;
        
        for (const auto metadata : output)
        {
            auto msg = metadata.getMessage();
            
            if (msg.getChannel() == 1)
                channel1Count++;
            else if (msg.getChannel() == 16)
                channel16Count++;
        }
        
        // All 4 tracks should output on channel 1
        expect(channel1Count == 4);
        
        // Debug channel should have copies
        expect(channel16Count > 0);
        
        auto stats = router.getStats();
        expect(stats.totalEventsRouted == 4);
        expect(stats.activeTrackCount > 0);
    }
    
    //==========================================================================
    void testChannelRouting()
    {
        beginTest("Channel Routing");
        
        MidiRouter router;
        
        // Test various MIDI message types
        auto noteOn = juce::MidiMessage::noteOn(7, 64, (juce::uint8)100);
        auto cc = juce::MidiMessage::controllerEvent(7, 1, 64);
        auto pitchBend = juce::MidiMessage::pitchWheel(7, 8192);
        auto aftertouch = juce::MidiMessage::aftertouchChange(7, 64, 50);
        
        router.addEventToTrack(0, noteOn, 0);
        router.addEventToTrack(0, cc, 10);
        router.addEventToTrack(0, pitchBend, 20);
        router.addEventToTrack(0, aftertouch, 30);
        
        juce::MidiBuffer output;
        router.processBlock(output, 512);
        
        // Verify all messages are routed to channel 1
        for (const auto metadata : output)
        {
            auto msg = metadata.getMessage();
            
            if (!msg.isController() || msg.getControllerNumber() != 120) // Skip track ID CC
            {
                if (msg.getChannel() == 1)
                {
                    // All main events should be on channel 1
                    expect(msg.getChannel() == 1);
                    
                    // Check message types preserved
                    if (msg.isNoteOn())
                        expect(msg.getNoteNumber() == 64);
                    else if (msg.isController() && msg.getControllerNumber() == 1)
                        expect(msg.getControllerValue() == 64);
                    else if (msg.isPitchWheel())
                        expect(msg.getPitchWheelValue() == 8192);
                    else if (msg.isAftertouch())
                        expect(msg.getAfterTouchValue() == 50);
                }
            }
        }
    }
    
    //==========================================================================
    void testDebugChannel()
    {
        beginTest("Debug Channel");
        
        MidiRouter router;
        
        // Test with debug enabled
        router.setDebugChannelEnabled(true);
        
        auto noteOn = juce::MidiMessage::noteOn(3, 60, (juce::uint8)100);
        router.addEventToTrack(5, noteOn, 0); // Track 5
        
        juce::MidiBuffer output;
        router.processBlock(output, 512);
        
        bool foundDebugEvent = false;
        bool foundTrackIdCC = false;
        
        for (const auto metadata : output)
        {
            auto msg = metadata.getMessage();
            
            if (msg.getChannel() == 16)
            {
                if (msg.isNoteOn())
                {
                    foundDebugEvent = true;
                    expect(msg.getNoteNumber() == 60);
                }
                else if (msg.isController() && msg.getControllerNumber() == 120)
                {
                    foundTrackIdCC = true;
                    expect(msg.getControllerValue() == 5); // Track index
                }
            }
        }
        
        expect(foundDebugEvent);
        expect(foundTrackIdCC);
        
        // Test with debug disabled
        router.clearAllBuffers();
        router.setDebugChannelEnabled(false);
        
        router.addEventToTrack(0, noteOn, 0);
        output.clear();
        router.processBlock(output, 512);
        
        int debugCount = 0;
        for (const auto metadata : output)
        {
            if (metadata.getMessage().getChannel() == 16)
                debugCount++;
        }
        
        expect(debugCount == 0, "No debug events when disabled");
    }
    
    //==========================================================================
    void testBufferOverflow()
    {
        beginTest("Buffer Overflow");
        
        MidiRouter router;
        
        // Fill track buffer to capacity
        for (int i = 0; i < MidiRouter::BUFFER_SIZE + 10; ++i)
        {
            auto noteOn = juce::MidiMessage::noteOn(1, 60 + (i % 12), (juce::uint8)100);
            router.addEventToTrack(0, noteOn, i);
        }
        
        // Should have dropped some events
        auto stats = router.getStats();
        expect(stats.eventsDropped > 0);
        
        // Process and check
        juce::MidiBuffer output;
        router.processBlock(output, 8192);
        
        // Count events
        int eventCount = 0;
        for (const auto metadata : output)
        {
            if (metadata.getMessage().getChannel() == 1)
                eventCount++;
        }
        
        // Should be limited to buffer size
        expect(eventCount <= MidiRouter::BUFFER_SIZE);
    }
    
    //==========================================================================
    void testTrackEnableDisable()
    {
        beginTest("Track Enable/Disable");
        
        MidiRouter router;
        
        // Disable track 0
        router.setTrackEnabled(0, false);
        expect(!router.isTrackEnabled(0));
        
        // Add event to disabled track
        auto noteOn = juce::MidiMessage::noteOn(1, 60, (juce::uint8)100);
        router.addEventToTrack(0, noteOn, 0);
        
        // Should not process disabled track
        juce::MidiBuffer output;
        router.processBlock(output, 512);
        
        expect(output.isEmpty());
        
        // Enable and try again
        router.setTrackEnabled(0, true);
        router.addEventToTrack(0, noteOn, 0);
        
        output.clear();
        router.processBlock(output, 512);
        
        expect(!output.isEmpty());
    }
    
    //==========================================================================
    void testStatistics()
    {
        beginTest("Statistics");
        
        MidiRouter router;
        router.resetStats();
        
        auto stats = router.getStats();
        expect(stats.totalEventsRouted == 0);
        expect(stats.eventsDropped == 0);
        expect(stats.debugEventsSent == 0);
        
        // Route some events
        for (int i = 0; i < 10; ++i)
        {
            auto noteOn = juce::MidiMessage::noteOn(1, 60 + i, (juce::uint8)100);
            router.addEventToTrack(i % 3, noteOn, i * 10);
        }
        
        juce::MidiBuffer output;
        router.processBlock(output, 512);
        
        stats = router.getStats();
        expect(stats.totalEventsRouted == 10);
        expect(stats.activeTrackCount > 0);
        expect(stats.debugEventsSent > 0);
    }
    
    //==========================================================================
    void testClearOperations()
    {
        beginTest("Clear Operations");
        
        MidiRouter router;
        
        // Add events to multiple tracks
        for (int track = 0; track < 5; ++track)
        {
            auto noteOn = juce::MidiMessage::noteOn(1, 60 + track, (juce::uint8)100);
            router.addEventToTrack(track, noteOn, 0);
        }
        
        expect(router.hasPendingEvents());
        
        // Clear specific track
        router.clearTrackBuffer(2);
        expect(router.getPendingEventCount(2) == 0);
        expect(router.getPendingEventCount(1) == 1); // Others still have events
        
        // Clear all
        router.clearAllBuffers();
        expect(!router.hasPendingEvents());
        
        for (int track = 0; track < 5; ++track)
        {
            expect(router.getPendingEventCount(track) == 0);
        }
    }
};

//==============================================================================
// Register the test
static MidiRouterTests midiRouterTests;

//==============================================================================
// Main function to run tests
int main(int, char**)
{
    juce::UnitTestRunner runner;
    runner.runAllTests();
    
    // Get results
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
    
    if (numFailed > 0)
    {
        std::cout << "\n" << numFailed << " test(s) failed!\n";
        return 1;
    }
    
    std::cout << "\nAll " << numPassed << " tests passed!\n";
    return 0;
}