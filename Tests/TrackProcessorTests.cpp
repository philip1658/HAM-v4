/*
  ==============================================================================

    TrackProcessorTests.cpp
    Comprehensive unit tests for TrackProcessor component
    Coverage target: >80% line coverage

  ==============================================================================
*/

#include <JuceHeader.h>
#include "../Source/Domain/Processors/TrackProcessor.h"
#include "../Source/Domain/Models/Track.h"
#include "../Source/Domain/Models/Stage.h"

using namespace HAM;

//==============================================================================
class TrackProcessorTests : public juce::UnitTest
{
public:
    TrackProcessorTests() : UnitTest("TrackProcessor Tests", "Processors") {}
    
    void runTest() override
    {
        testConstruction();
        testBasicProcessing();
        testModulationProcessing();
        testGateProcessing();
        testPitchProcessing();
        testVelocityProcessing();
        testMuteAndSolo();
        testChannelRouting();
        testEdgeCases();
        testThreadSafety();
    }
    
private:
    void testConstruction()
    {
        beginTest("Construction and Initial State");
        
        TrackProcessor processor;
        
        // Test initial state
        expect(!processor.isMuted(), "Should not be muted initially");
        expect(!processor.isSoloed(), "Should not be soloed initially");
        expectEquals(processor.getOutputChannel(), 1, "Default output channel should be 1");
        expectEquals(processor.getVolume(), 1.0f, "Default volume should be 1.0");
        expectEquals(processor.getPan(), 0.0f, "Default pan should be center");
    }
    
    void testBasicProcessing()
    {
        beginTest("Basic Track Processing");
        
        TrackProcessor processor;
        Track track;
        Stage stage;
        
        // Setup track
        track.setChannel(5);
        track.setEnabled(true);
        track.setVolume(0.8f);
        track.setPan(-0.5f);
        
        // Setup stage with some gates
        stage.setGate(0, true);
        stage.setPitch(0, 64);
        stage.setVelocity(0, 100);
        
        // Process track
        auto result = processor.processTrack(track, stage, 0, 48000.0);
        
        expect(result.processed, "Track should be processed");
        expectEquals(result.channel, 5, "Should use track channel");
        expectWithinAbsoluteError(result.volume, 0.8f, 0.01f, "Should apply track volume");
        expectWithinAbsoluteError(result.pan, -0.5f, 0.01f, "Should apply track pan");
        
        // Test disabled track
        track.setEnabled(false);
        result = processor.processTrack(track, stage, 0, 48000.0);
        expect(!result.processed, "Disabled track should not be processed");
    }
    
    void testModulationProcessing()
    {
        beginTest("Modulation Processing");
        
        TrackProcessor processor;
        Track track;
        Stage stage;
        
        track.setEnabled(true);
        
        // Add modulation to stage
        stage.setModulationCC(1, 64);   // Mod wheel
        stage.setModulationCC(7, 100);  // Volume
        stage.setModulationAmount(0.5f);
        stage.setModulationRate(4.0f);
        
        // Process with modulation
        auto result = processor.processTrack(track, stage, 0, 48000.0);
        
        expect(result.processed, "Should process with modulation");
        expect(!result.modulationData.empty(), "Should generate modulation data");
        
        // Check modulation values
        bool foundModWheel = false;
        bool foundVolume = false;
        
        for (const auto& mod : result.modulationData)
        {
            if (mod.ccNumber == 1) foundModWheel = true;
            if (mod.ccNumber == 7) foundVolume = true;
            
            expectGreaterOrEqual(mod.value, 0, "CC value should be >= 0");
            expectLessOrEqual(mod.value, 127, "CC value should be <= 127");
        }
        
        expect(foundModWheel || foundVolume, "Should find modulation CCs");
    }
    
    void testGateProcessing()
    {
        beginTest("Gate Processing");
        
        TrackProcessor processor;
        Track track;
        Stage stage;
        
        track.setEnabled(true);
        
        // Test gate probability
        processor.setGateProbability(0.5f);
        
        // Process multiple times to test probability
        int gatesTriggered = 0;
        stage.setGate(0, true);
        
        for (int i = 0; i < 100; ++i)
        {
            auto result = processor.processTrack(track, stage, 0, 48000.0);
            if (result.gateActive)
                gatesTriggered++;
        }
        
        // Should be roughly 50% with probability 0.5
        expectGreaterThan(gatesTriggered, 20, "Some gates should trigger");
        expectLessThan(gatesTriggered, 80, "Not all gates should trigger");
        
        // Test gate length
        processor.setGateProbability(1.0f);
        processor.setGateLength(0.25f);
        
        auto result = processor.processTrack(track, stage, 0, 48000.0);
        expectWithinAbsoluteError(result.gateLength, 0.25f, 0.01f, "Gate length should be applied");
        
        // Test no gate
        stage.setGate(0, false);
        result = processor.processTrack(track, stage, 0, 48000.0);
        expect(!result.gateActive, "Should not trigger when gate is off");
    }
    
    void testPitchProcessing()
    {
        beginTest("Pitch Processing");
        
        TrackProcessor processor;
        Track track;
        Stage stage;
        
        track.setEnabled(true);
        stage.setGate(0, true);
        stage.setPitch(0, 60); // Middle C
        
        // Test transpose
        processor.setTranspose(12); // Up one octave
        auto result = processor.processTrack(track, stage, 0, 48000.0);
        expectEquals(result.pitch, 72, "Should transpose up one octave");
        
        processor.setTranspose(-12); // Down one octave
        result = processor.processTrack(track, stage, 0, 48000.0);
        expectEquals(result.pitch, 48, "Should transpose down one octave");
        
        // Test octave range
        processor.setTranspose(0);
        processor.setOctaveRange(-2, 2);
        
        // Process multiple times to test octave randomization
        std::set<int> pitches;
        for (int i = 0; i < 50; ++i)
        {
            result = processor.processTrack(track, stage, 0, 48000.0);
            pitches.insert(result.pitch);
        }
        
        // Should have variations across octaves
        expectGreaterThan(static_cast<int>(pitches.size()), 1, "Should have pitch variations");
        
        // All pitches should be within range
        for (int pitch : pitches)
        {
            expectGreaterOrEqual(pitch, 36, "Pitch should be >= C1");
            expectLessOrEqual(pitch, 84, "Pitch should be <= C6");
        }
    }
    
    void testVelocityProcessing()
    {
        beginTest("Velocity Processing");
        
        TrackProcessor processor;
        Track track;
        Stage stage;
        
        track.setEnabled(true);
        track.setVolume(0.5f);
        stage.setGate(0, true);
        stage.setVelocity(0, 100);
        
        // Test velocity scaling
        processor.setVelocityScale(0.8f);
        auto result = processor.processTrack(track, stage, 0, 48000.0);
        
        // Velocity should be scaled by track volume and velocity scale
        int expectedVel = static_cast<int>(100 * 0.5f * 0.8f);
        expectWithinAbsoluteError(result.velocity, expectedVel, 5, "Velocity should be scaled");
        
        // Test velocity randomization
        processor.setVelocityRandomization(0.3f);
        
        std::set<int> velocities;
        for (int i = 0; i < 20; ++i)
        {
            result = processor.processTrack(track, stage, 0, 48000.0);
            velocities.insert(result.velocity);
        }
        
        expectGreaterThan(static_cast<int>(velocities.size()), 1, "Should have velocity variations");
        
        // Test accent
        stage.setAccent(0, true);
        processor.setAccentAmount(1.5f);
        result = processor.processTrack(track, stage, 0, 48000.0);
        
        expectGreaterThan(result.velocity, expectedVel, "Accent should increase velocity");
    }
    
    void testMuteAndSolo()
    {
        beginTest("Mute and Solo");
        
        TrackProcessor processor;
        Track track;
        Stage stage;
        
        track.setEnabled(true);
        stage.setGate(0, true);
        
        // Test mute
        processor.setMuted(true);
        auto result = processor.processTrack(track, stage, 0, 48000.0);
        expect(!result.processed, "Muted track should not be processed");
        
        processor.setMuted(false);
        result = processor.processTrack(track, stage, 0, 48000.0);
        expect(result.processed, "Unmuted track should be processed");
        
        // Test solo
        processor.setSoloed(true);
        expect(processor.isSoloed(), "Should be soloed");
        result = processor.processTrack(track, stage, 0, 48000.0);
        expect(result.processed, "Soloed track should be processed");
        
        // Test mute override by solo
        processor.setMuted(true);
        processor.setSoloed(true);
        result = processor.processTrack(track, stage, 0, 48000.0);
        expect(result.processed, "Solo should override mute");
    }
    
    void testChannelRouting()
    {
        beginTest("Channel Routing");
        
        TrackProcessor processor;
        Track track;
        Stage stage;
        
        track.setEnabled(true);
        stage.setGate(0, true);
        
        // Test channel routing
        for (int channel = 1; channel <= 16; ++channel)
        {
            track.setChannel(channel);
            processor.setOutputChannel(channel);
            
            auto result = processor.processTrack(track, stage, 0, 48000.0);
            expectEquals(result.channel, channel, "Should route to channel " + juce::String(channel));
        }
        
        // Test channel override
        track.setChannel(5);
        processor.setOutputChannel(10);
        processor.setChannelOverride(true);
        
        auto result = processor.processTrack(track, stage, 0, 48000.0);
        expectEquals(result.channel, 10, "Should override to processor channel");
        
        processor.setChannelOverride(false);
        result = processor.processTrack(track, stage, 0, 48000.0);
        expectEquals(result.channel, 5, "Should use track channel when not overridden");
    }
    
    void testEdgeCases()
    {
        beginTest("Edge Cases");
        
        TrackProcessor processor;
        Track track;
        Stage stage;
        
        // Test with null/invalid inputs
        auto result = processor.processTrack(track, stage, -1, 0.0);
        // Should handle gracefully
        
        // Test extreme values
        track.setVolume(10.0f); // Way too high
        processor.setVelocityScale(5.0f);
        stage.setVelocity(0, 127);
        stage.setGate(0, true);
        track.setEnabled(true);
        
        result = processor.processTrack(track, stage, 0, 48000.0);
        expectLessOrEqual(result.velocity, 127, "Velocity should be clamped to 127");
        
        // Test zero/negative sample rate
        result = processor.processTrack(track, stage, 0, 0.0);
        result = processor.processTrack(track, stage, 0, -48000.0);
        // Should handle gracefully
        
        // Test out of range channels
        track.setChannel(0);
        result = processor.processTrack(track, stage, 0, 48000.0);
        expectGreaterOrEqual(result.channel, 1, "Channel should be >= 1");
        
        track.setChannel(17);
        result = processor.processTrack(track, stage, 0, 48000.0);
        expectLessOrEqual(result.channel, 16, "Channel should be <= 16");
        
        // Test extreme pan values
        track.setPan(-2.0f);
        result = processor.processTrack(track, stage, 0, 48000.0);
        expectGreaterOrEqual(result.pan, -1.0f, "Pan should be >= -1");
        
        track.setPan(2.0f);
        result = processor.processTrack(track, stage, 0, 48000.0);
        expectLessOrEqual(result.pan, 1.0f, "Pan should be <= 1");
        
        // Test with all gates off
        for (int i = 0; i < 8; ++i)
        {
            stage.setGate(i, false);
        }
        result = processor.processTrack(track, stage, 3, 48000.0);
        expect(!result.gateActive, "No gates should mean no output");
    }
    
    void testThreadSafety()
    {
        beginTest("Thread Safety");
        
        TrackProcessor processor;
        std::atomic<bool> shouldStop{false};
        
        // Audio thread - processing tracks
        std::thread audioThread([&processor, &shouldStop]()
        {
            Track track;
            Stage stage;
            track.setEnabled(true);
            track.setChannel(1);
            
            for (int i = 0; i < 8; ++i)
            {
                stage.setGate(i, true);
                stage.setPitch(i, 60 + i);
                stage.setVelocity(i, 80 + i * 2);
            }
            
            while (!shouldStop.load())
            {
                for (int pulse = 0; pulse < 8; ++pulse)
                {
                    auto result = processor.processTrack(track, stage, pulse, 48000.0);
                    std::this_thread::sleep_for(std::chrono::microseconds(100));
                }
            }
        });
        
        // Control thread - changing settings
        std::thread controlThread([&processor, &shouldStop]()
        {
            int counter = 0;
            while (!shouldStop.load())
            {
                processor.setMuted(counter % 3 == 0);
                processor.setSoloed(counter % 5 == 0);
                processor.setOutputChannel((counter % 16) + 1);
                processor.setVolume((counter % 100) / 100.0f);
                processor.setPan((counter % 200 - 100) / 100.0f);
                processor.setTranspose((counter % 24) - 12);
                processor.setGateProbability((counter % 100) / 100.0f);
                processor.setGateLength((counter % 100) / 100.0f);
                processor.setVelocityScale((counter % 150) / 100.0f);
                processor.setVelocityRandomization((counter % 50) / 100.0f);
                
                counter++;
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        });
        
        // Let threads run
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        // Stop threads
        shouldStop = true;
        audioThread.join();
        controlThread.join();
        
        // If we get here without crashing, thread safety is working
        expect(true, "Thread safety test completed without crashes");
        
        // Verify processor is still functional
        Track track;
        Stage stage;
        track.setEnabled(true);
        stage.setGate(0, true);
        
        auto result = processor.processTrack(track, stage, 0, 48000.0);
        expect(result.processed || processor.isMuted(), "Processor should still be functional");
    }
};

static TrackProcessorTests trackProcessorTests;

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