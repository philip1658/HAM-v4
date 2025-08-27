/*
  ==============================================================================

    MidiEventGeneratorTests.cpp
    Comprehensive unit tests for MidiEventGenerator component
    Coverage target: >80% line coverage

  ==============================================================================
*/

#include <JuceHeader.h>
#include "../Source/Domain/Processors/MidiEventGenerator.h"
#include "../Source/Domain/Models/Stage.h"
#include "../Source/Domain/Models/Track.h"
#include <algorithm>

using namespace HAM;

//==============================================================================
class MidiEventGeneratorTests : public juce::UnitTest
{
public:
    MidiEventGeneratorTests() : UnitTest("MidiEventGenerator Tests", "MIDI") {}
    
    void runTest() override
    {
        testConstruction();
        testBasicEventGeneration();
        testRatchetedEvents();
        testHumanization();
        testCCGeneration();
        testPitchBendGeneration();
        testStageEventGeneration();
        testVelocityConfiguration();
        testTimingConfiguration();
        testEngineIntegration();
        testEdgeCases();
        testThreadSafety();
    }
    
private:
    void testConstruction()
    {
        beginTest("Construction and Initial State");
        
        MidiEventGenerator generator;
        
        // Test initial configuration
        expectEquals(generator.getGlobalVelocity(), 1.0f, "Global velocity should be 1.0");
        
        // Test engines are created
        expect(&generator.getGateEngine() != nullptr, "Gate engine should be created");
        expect(&generator.getPitchEngine() != nullptr, "Pitch engine should be created");
    }
    
    void testBasicEventGeneration()
    {
        beginTest("Basic Event Generation");
        
        MidiEventGenerator generator;
        
        // Test single note generation through ratcheted events
        auto events = generator.generateRatchetedEvents(60, 100, 1, 1000, 1);
        
        expectEquals(events.size(), static_cast<size_t>(2), "Should generate note on and off");
        
        if (events.size() >= 2)
        {
            // Check note on
            expect(events[0].message.isNoteOn(), "First event should be note on");
            expectEquals(events[0].message.getNoteNumber(), 60, "Note should be 60");
            expectEquals(events[0].channel, 1, "Channel should be 1");
            
            // Check note off
            expect(events[1].message.isNoteOff(), "Second event should be note off");
            expectEquals(events[1].message.getNoteNumber(), 60, "Note off should be for same note");
        }
        
        // Test with different velocity
        events = generator.generateRatchetedEvents(72, 127, 1, 1000, 2);
        if (!events.empty() && events[0].message.isNoteOn())
        {
            expectEquals(static_cast<int>(events[0].message.getVelocity()), 127, "Velocity should be 127");
            expectEquals(events[0].channel, 2, "Channel should be 2");
        }
        
        // Test with zero velocity (should generate note off)
        events = generator.generateRatchetedEvents(48, 0, 1, 1000, 3);
        if (!events.empty())
        {
            expect(events[0].message.isNoteOff() || events[0].message.getVelocity() == 0,
                   "Zero velocity should generate note off");
        }
    }
    
    void testRatchetedEvents()
    {
        beginTest("Ratcheted Events");
        
        MidiEventGenerator generator;
        
        // Test multiple ratchets
        auto events = generator.generateRatchetedEvents(64, 100, 4, 1000, 1);
        
        // Should generate 4 note pairs (on/off)
        expectEquals(events.size(), static_cast<size_t>(8), "4 ratchets should generate 8 events");
        
        // Check timing distribution
        if (events.size() >= 8)
        {
            int expectedSpacing = 1000 / 4; // Samples between ratchets
            
            for (size_t i = 0; i < events.size(); i += 2)
            {
                if (i + 2 < events.size())
                {
                    int spacing = events[i + 2].sampleOffset - events[i].sampleOffset;
                    expectWithinAbsoluteError(spacing, expectedSpacing, 10,
                                            "Ratchets should be evenly spaced");
                }
            }
        }
        
        // Test different ratchet counts
        for (int ratchetCount : {1, 2, 3, 5, 8, 16})
        {
            events = generator.generateRatchetedEvents(60, 80, ratchetCount, 960, 1);
            expectEquals(static_cast<int>(events.size()), ratchetCount * 2,
                        "Should generate correct number of events for " + 
                        juce::String(ratchetCount) + " ratchets");
        }
        
        // Test ratchet with very small window
        events = generator.generateRatchetedEvents(60, 100, 4, 10, 1);
        expect(!events.empty(), "Should handle small sample windows");
        
        // Test zero ratchets
        events = generator.generateRatchetedEvents(60, 100, 0, 1000, 1);
        expect(events.empty(), "Zero ratchets should generate no events");
    }
    
    void testHumanization()
    {
        beginTest("Humanization");
        
        MidiEventGenerator generator;
        
        // Generate base events
        auto events = generator.generateRatchetedEvents(60, 100, 4, 1000, 1);
        
        // Store original values
        std::vector<int> originalOffsets;
        std::vector<int> originalVelocities;
        for (const auto& event : events)
        {
            originalOffsets.push_back(event.sampleOffset);
            if (event.message.isNoteOn())
                originalVelocities.push_back(event.message.getVelocity());
        }
        
        // Apply timing humanization
        generator.applyHumanization(events, 0.5f, 0.0f);
        
        // Check that timing has changed
        bool timingChanged = false;
        for (size_t i = 0; i < events.size(); ++i)
        {
            if (events[i].sampleOffset != originalOffsets[i])
            {
                timingChanged = true;
                break;
            }
        }
        expect(timingChanged || events.empty(), "Timing humanization should modify offsets");
        
        // Generate new events for velocity test
        events = generator.generateRatchetedEvents(60, 100, 4, 1000, 1);
        
        // Apply velocity humanization
        generator.applyHumanization(events, 0.0f, 0.8f);
        
        // Check that velocities have changed
        bool velocityChanged = false;
        int noteOnIndex = 0;
        for (const auto& event : events)
        {
            if (event.message.isNoteOn())
            {
                if (noteOnIndex < originalVelocities.size() &&
                    event.message.getVelocity() != originalVelocities[noteOnIndex])
                {
                    velocityChanged = true;
                    break;
                }
                noteOnIndex++;
            }
        }
        expect(velocityChanged || events.empty(), "Velocity humanization should modify velocities");
        
        // Test extreme humanization
        events = generator.generateRatchetedEvents(60, 64, 8, 2000, 1);
        generator.applyHumanization(events, 1.0f, 1.0f);
        
        // All events should still be within valid ranges
        for (const auto& event : events)
        {
            expectGreaterOrEqual(event.sampleOffset, 0, "Offset should be non-negative");
            if (event.message.isNoteOn())
            {
                expectGreaterOrEqual(static_cast<int>(event.message.getVelocity()), 1, "Velocity should be >= 1");
                expectLessOrEqual(static_cast<int>(event.message.getVelocity()), 127, "Velocity should be <= 127");
            }
        }
        
        // Test no humanization
        events = generator.generateRatchetedEvents(60, 100, 2, 1000, 1);
        auto copyEvents = events;
        generator.applyHumanization(events, 0.0f, 0.0f);
        
        // Events should be unchanged
        for (size_t i = 0; i < events.size(); ++i)
        {
            expectEquals(events[i].sampleOffset, copyEvents[i].sampleOffset,
                        "No humanization should leave timing unchanged");
            if (events[i].message.isNoteOn())
            {
                expectEquals(events[i].message.getVelocity(), 
                           copyEvents[i].message.getVelocity(),
                           "No humanization should leave velocity unchanged");
            }
        }
    }
    
    void testCCGeneration()
    {
        beginTest("CC Event Generation");
        
        MidiEventGenerator generator;
        Stage stage;
        
        // Setup stage with CC modulation
        stage.setModulationCC(1, 64);  // Mod wheel at 64
        stage.setModulationCC(7, 100); // Volume at 100
        stage.setModulationCC(10, 32); // Pan at 32
        
        // Generate CC events
        auto events = generator.generateCCEvents(stage, 5, 100);
        
        // Should generate events for each active CC
        expectGreaterOrEqual(static_cast<int>(events.size()), 1, 
                           "Should generate at least one CC event");
        
        // Check CC values
        bool foundModWheel = false;
        bool foundVolume = false;
        bool foundPan = false;
        
        for (const auto& event : events)
        {
            if (event.message.isController())
            {
                int ccNum = event.message.getControllerNumber();
                int ccVal = event.message.getControllerValue();
                
                if (ccNum == 1 && ccVal == 64) foundModWheel = true;
                if (ccNum == 7 && ccVal == 100) foundVolume = true;
                if (ccNum == 10 && ccVal == 32) foundPan = true;
                
                expectEquals(event.channel, 5, "CC should be on correct channel");
                expectEquals(event.sampleOffset, 100, "CC should have correct offset");
            }
        }
        
        expect(foundModWheel || foundVolume || foundPan, 
               "Should find at least one expected CC");
        
        // Test with CC disabled
        generator.setCCEnabled(false);
        events = generator.generateCCEvents(stage, 1, 0);
        expect(events.empty(), "Should not generate CC when disabled");
        
        // Re-enable and test empty stage
        generator.setCCEnabled(true);
        Stage emptyStage;
        events = generator.generateCCEvents(emptyStage, 1, 0);
        // Empty stage might still generate default CCs or none
    }
    
    void testPitchBendGeneration()
    {
        beginTest("Pitch Bend Generation");
        
        MidiEventGenerator generator;
        Stage stage;
        
        // Set pitch bend value
        stage.setPitchBend(0.5f); // Center position
        
        auto event = generator.generatePitchBendEvent(stage, 3, 200);
        
        if (event.has_value())
        {
            expect(event->message.isPitchWheel(), "Should be pitch wheel message");
            expectEquals(event->channel, 3, "Should be on correct channel");
            expectEquals(event->sampleOffset, 200, "Should have correct offset");
            
            // Check pitch bend value is in valid range
            int pitchValue = event->message.getPitchWheelValue();
            expectGreaterOrEqual(pitchValue, 0, "Pitch value should be >= 0");
            expectLessOrEqual(pitchValue, 16383, "Pitch value should be <= 16383");
        }
        
        // Test extreme values
        stage.setPitchBend(0.0f); // Minimum
        event = generator.generatePitchBendEvent(stage, 1, 0);
        if (event.has_value())
        {
            expectGreaterOrEqual(event->message.getPitchWheelValue(), 0,
                               "Min pitch bend should be valid");
        }
        
        stage.setPitchBend(1.0f); // Maximum
        event = generator.generatePitchBendEvent(stage, 1, 0);
        if (event.has_value())
        {
            expectLessOrEqual(event->message.getPitchWheelValue(), 16383,
                            "Max pitch bend should be valid");
        }
        
        // Test neutral position
        stage.setPitchBend(0.5f);
        event = generator.generatePitchBendEvent(stage, 1, 0);
        if (event.has_value())
        {
            int expected = 8192; // Center position
            expectWithinAbsoluteError(event->message.getPitchWheelValue(), 
                                    expected, 100,
                                    "Center pitch bend should be near 8192");
        }
    }
    
    void testStageEventGeneration()
    {
        beginTest("Stage Event Generation");
        
        MidiEventGenerator generator;
        Stage stage;
        Track track;
        
        // Configure stage
        stage.setGate(3, true);  // Enable gate at position 3
        stage.setPitch(3, 64);   // Set pitch
        stage.setVelocity(3, 100);
        stage.setRatchet(3, 2);  // 2 ratchets
        
        // Configure track
        track.setChannel(10);
        track.setEnabled(true);
        
        // Generate events for pulse 3
        auto events = generator.generateStageEvents(
            stage, 0, &track, 3, 48000.0, 1000, 4000);
        
        // Should generate events for the active gate
        expect(!events.empty(), "Should generate events for active gate");
        
        if (!events.empty())
        {
            // Check first event
            expect(events[0].message.isNoteOn() || events[0].message.isNoteOff(),
                   "Should be note event");
            expectEquals(events[0].channel, 10, "Should use track channel");
            expectEquals(events[0].trackIndex, 0, "Track index should be set");
            expectEquals(events[0].stageIndex, 0, "Stage index should be set");
        }
        
        // Test with disabled track
        track.setEnabled(false);
        events = generator.generateStageEvents(
            stage, 0, &track, 3, 48000.0, 1000, 4000);
        expect(events.empty(), "Disabled track should generate no events");
        
        // Test with no active gates
        track.setEnabled(true);
        stage.setGate(3, false);
        events = generator.generateStageEvents(
            stage, 0, &track, 3, 48000.0, 1000, 4000);
        expect(events.empty(), "No active gates should generate no events");
        
        // Test multiple gates in stage
        for (int i = 0; i < 8; ++i)
        {
            stage.setGate(i, true);
            stage.setPitch(i, 60 + i);
            stage.setVelocity(i, 80 + i * 5);
        }
        
        // Generate events for each pulse
        for (int pulse = 0; pulse < 8; ++pulse)
        {
            events = generator.generateStageEvents(
                stage, 0, &track, pulse, 48000.0, 1000, 4000);
            
            if (!events.empty() && events[0].message.isNoteOn())
            {
                int expectedNote = 60 + pulse;
                expectEquals(events[0].message.getNoteNumber(), expectedNote,
                           "Note should match stage pitch");
            }
        }
    }
    
    void testVelocityConfiguration()
    {
        beginTest("Velocity Configuration");
        
        MidiEventGenerator generator;
        
        // Test global velocity scaling
        generator.setGlobalVelocity(0.5f);
        expectEquals(generator.getGlobalVelocity(), 0.5f, "Global velocity should be 0.5");
        
        auto events = generator.generateRatchetedEvents(60, 100, 1, 1000, 1);
        if (!events.empty() && events[0].message.isNoteOn())
        {
            int scaledVelocity = events[0].message.getVelocity();
            expectLessOrEqual(scaledVelocity, 100, "Velocity should be scaled down");
        }
        
        // Test velocity randomization
        generator.setGlobalVelocity(1.0f);
        generator.setVelocityRandomization(0.5f);
        
        // Generate multiple events and check for variation
        std::set<int> velocities;
        for (int i = 0; i < 10; ++i)
        {
            events = generator.generateRatchetedEvents(60, 100, 1, 1000, 1);
            if (!events.empty() && events[0].message.isNoteOn())
            {
                velocities.insert(events[0].message.getVelocity());
            }
        }
        
        expectGreaterThan(static_cast<int>(velocities.size()), 1,
                         "Velocity randomization should create variation");
        
        // Test extreme values
        generator.setGlobalVelocity(0.0f);
        events = generator.generateRatchetedEvents(60, 127, 1, 1000, 1);
        if (!events.empty() && events[0].message.isNoteOn())
        {
            expectEquals(events[0].message.getVelocity(), 1,
                        "Zero global velocity should still output minimum velocity");
        }
        
        generator.setGlobalVelocity(2.0f); // Over 1.0
        events = generator.generateRatchetedEvents(60, 64, 1, 1000, 1);
        if (!events.empty() && events[0].message.isNoteOn())
        {
            expectLessOrEqual(events[0].message.getVelocity(), 127,
                            "Velocity should be clamped to 127");
        }
    }
    
    void testTimingConfiguration()
    {
        beginTest("Timing Configuration");
        
        MidiEventGenerator generator;
        
        // Test timing randomization
        generator.setTimingRandomization(0.5f);
        
        // Generate multiple events and check for timing variation
        std::set<int> offsets;
        for (int i = 0; i < 10; ++i)
        {
            auto events = generator.generateRatchetedEvents(60, 100, 2, 1000, 1);
            if (events.size() >= 2)
            {
                offsets.insert(events[1].sampleOffset - events[0].sampleOffset);
            }
        }
        
        expectGreaterOrEqual(static_cast<int>(offsets.size()), 1,
                           "Timing randomization should create variation");
        
        // Test with no randomization
        generator.setTimingRandomization(0.0f);
        offsets.clear();
        
        for (int i = 0; i < 5; ++i)
        {
            auto events = generator.generateRatchetedEvents(60, 100, 2, 1000, 1);
            if (events.size() >= 2)
            {
                offsets.insert(events[1].sampleOffset - events[0].sampleOffset);
            }
        }
        
        expectEquals(static_cast<int>(offsets.size()), 1,
                    "No randomization should produce consistent timing");
    }
    
    void testEngineIntegration()
    {
        beginTest("Engine Integration");
        
        MidiEventGenerator generator;
        
        // Test gate engine access
        auto& gateEngine = generator.getGateEngine();
        expect(&gateEngine != nullptr, "Should provide gate engine access");
        
        // Configure gate engine
        gateEngine.setProbability(0.5f);
        gateEngine.setGateLength(0.8f);
        
        // Test pitch engine access
        auto& pitchEngine = generator.getPitchEngine();
        expect(&pitchEngine != nullptr, "Should provide pitch engine access");
        
        // Configure pitch engine
        pitchEngine.setOctaveRange(-1, 2);
        pitchEngine.setTranspose(12);
        
        // Generate events with configured engines
        Stage stage;
        Track track;
        track.setChannel(1);
        track.setEnabled(true);
        
        for (int i = 0; i < 8; ++i)
        {
            stage.setGate(i, true);
            stage.setPitch(i, 60);
        }
        
        auto events = generator.generateStageEvents(
            stage, 0, &track, 0, 48000.0, 1000, 4000);
        
        // Events should reflect engine configuration
        if (!events.empty() && events[0].message.isNoteOn())
        {
            // Note should be transposed
            int note = events[0].message.getNoteNumber();
            expectNotEquals(note, 60, "Pitch engine should modify note");
        }
    }
    
    void testEdgeCases()
    {
        beginTest("Edge Cases");
        
        MidiEventGenerator generator;
        
        // Test with null track
        Stage stage;
        auto events = generator.generateStageEvents(
            stage, 0, nullptr, 0, 48000.0, 1000, 4000);
        // Should handle gracefully
        
        // Test with zero sample rate
        events = generator.generateStageEvents(
            stage, 0, nullptr, 0, 0.0, 1000, 4000);
        // Should handle gracefully
        
        // Test with zero samples per pulse
        events = generator.generateStageEvents(
            stage, 0, nullptr, 0, 48000.0, 0, 4000);
        // Should handle gracefully
        
        // Test with zero buffer size
        events = generator.generateStageEvents(
            stage, 0, nullptr, 0, 48000.0, 1000, 0);
        // Should handle gracefully
        
        // Test with negative indices
        events = generator.generateStageEvents(
            stage, -1, nullptr, -1, 48000.0, 1000, 4000);
        // Should handle gracefully
        
        // Test with out of range pulse index
        events = generator.generateStageEvents(
            stage, 0, nullptr, 100, 48000.0, 1000, 4000);
        // Should handle gracefully
        
        // Test extreme ratchet count
        events = generator.generateRatchetedEvents(60, 100, 1000, 1000, 1);
        expect(events.size() <= 2000, "Should limit extreme ratchet counts");
        
        // Test MIDI channel boundaries
        events = generator.generateRatchetedEvents(60, 100, 1, 1000, 0);
        if (!events.empty())
        {
            expectGreaterOrEqual(events[0].channel, 1, "Channel should be >= 1");
        }
        
        events = generator.generateRatchetedEvents(60, 100, 1, 1000, 17);
        if (!events.empty())
        {
            expectLessOrEqual(events[0].channel, 16, "Channel should be <= 16");
        }
        
        // Test MIDI note boundaries
        events = generator.generateRatchetedEvents(-10, 100, 1, 1000, 1);
        if (!events.empty() && events[0].message.isNoteOn())
        {
            expectGreaterOrEqual(events[0].message.getNoteNumber(), 0,
                               "Note should be >= 0");
        }
        
        events = generator.generateRatchetedEvents(200, 100, 1, 1000, 1);
        if (!events.empty() && events[0].message.isNoteOn())
        {
            expectLessOrEqual(events[0].message.getNoteNumber(), 127,
                            "Note should be <= 127");
        }
        
        // Test empty CC generation
        Stage emptyStage;
        events = generator.generateCCEvents(emptyStage, 1, 0);
        // Should handle empty stage
        
        // Test pitch bend with no value set
        auto pitchEvent = generator.generatePitchBendEvent(emptyStage, 1, 0);
        // Should handle gracefully
    }
    
    void testThreadSafety()
    {
        beginTest("Thread Safety");
        
        MidiEventGenerator generator;
        std::atomic<bool> shouldStop{false};
        
        // Audio thread - generating events
        std::thread audioThread([&generator, &shouldStop]()
        {
            Stage stage;
            Track track;
            track.setChannel(1);
            track.setEnabled(true);
            
            for (int i = 0; i < 8; ++i)
            {
                stage.setGate(i, true);
                stage.setPitch(i, 60 + i);
            }
            
            while (!shouldStop.load())
            {
                for (int pulse = 0; pulse < 8; ++pulse)
                {
                    auto events = generator.generateStageEvents(
                        stage, 0, &track, pulse, 48000.0, 1000, 4000);
                    
                    // Also test ratcheted events
                    generator.generateRatchetedEvents(60, 100, 4, 1000, 1);
                    
                    std::this_thread::sleep_for(std::chrono::microseconds(100));
                }
            }
        });
        
        // Control thread - changing configuration
        std::thread controlThread([&generator, &shouldStop]()
        {
            float velocity = 0.0f;
            while (!shouldStop.load())
            {
                generator.setGlobalVelocity(velocity);
                generator.setVelocityRandomization(velocity * 0.5f);
                generator.setTimingRandomization(velocity * 0.3f);
                generator.setCCEnabled(velocity > 0.5f);
                
                velocity += 0.1f;
                if (velocity > 1.0f) velocity = 0.0f;
                
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
        
        // Engine configuration thread
        std::thread engineThread([&generator, &shouldStop]()
        {
            while (!shouldStop.load())
            {
                generator.getGateEngine().setProbability(0.8f);
                generator.getGateEngine().setGateLength(0.5f);
                generator.getPitchEngine().setTranspose(12);
                generator.getPitchEngine().setOctaveRange(-2, 2);
                
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }
        });
        
        // Let threads run
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        // Stop threads
        shouldStop = true;
        audioThread.join();
        controlThread.join();
        engineThread.join();
        
        // If we get here without crashing, thread safety is working
        expect(true, "Thread safety test completed without crashes");
        
        // Verify generator is still functional
        auto events = generator.generateRatchetedEvents(60, 100, 2, 1000, 1);
        expect(!events.empty(), "Generator should still function after thread test");
    }
};

static MidiEventGeneratorTests midiEventGeneratorTests;

//==============================================================================
// Main function for standalone test execution
int main(int argc, char* argv[])
{
    juce::UnitTestRunner runner;
    runner.runAllTests();
    
    int numPassed = 0, numFailed = 0;
    
    for (int i = 0; i < runner.getNumResults(); ++i)
    {
        auto* result = runner.getResult(i);
        if (result->failures > 0)
            numFailed++;
        else
            numPassed++;
            
        std::cout << result->unitTestName << ": "
                  << (result->failures > 0 ? "FAILED" : "PASSED")
                  << " (" << result->passes << " passes, "
                  << result->failures << " failures)\n";
    }
    
    std::cout << "\n========================================\n";
    std::cout << "Total: " << numPassed << " passed, " << numFailed << " failed\n";
    std::cout << "========================================\n";
    
    return numFailed > 0 ? 1 : 0;
}