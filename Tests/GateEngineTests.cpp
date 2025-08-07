/*
  ==============================================================================

    GateEngineTests.cpp
    Unit tests for GateEngine

  ==============================================================================
*/

#include <JuceHeader.h>
#include "../Source/Domain/Engines/GateEngine.h"
#include "../Source/Domain/Models/Stage.h"
#include "../Source/Domain/Models/Track.h"

using namespace HAM;

class GateEngineTests : public juce::UnitTest
{
public:
    GateEngineTests()
        : UnitTest("GateEngine Tests", "Engines")
    {
    }
    
    void runTest() override
    {
        testGateTypes();
        testRatcheting();
        testGateLength();
        testSwing();
        testProbability();
        testGateMorphing();
        testTrackProcessor();
    }
    
private:
    void testGateTypes()
    {
        beginTest("Gate Types");
        
        GateEngine engine;
        Stage stage;
        
        const double sampleRate = 48000.0;
        const int samplesPerPulse = 12000; // 250ms at 48kHz
        
        // Test MULTIPLE gate type
        {
            stage.setGateType(0); // MULTIPLE
            stage.setRatchetCount(0, 4); // 4 ratchets on first pulse
            stage.setProbability(1.0f);
            
            auto events = engine.processStageGate(stage, 0, sampleRate, samplesPerPulse);
            
            // Should have 8 events (4 note-ons + 4 note-offs)
            expect(events.size() == 8, "MULTIPLE should generate 8 events for 4 ratchets");
            
            int noteOns = 0, noteOffs = 0;
            for (const auto& event : events)
            {
                if (event.isNoteOn) noteOns++;
                else noteOffs++;
            }
            expect(noteOns == 4, "Should have 4 note-ons");
            expect(noteOffs == 4, "Should have 4 note-offs");
        }
        
        // Test HOLD gate type
        {
            stage.setGateType(1); // HOLD
            stage.setRatchetCount(0, 4); // 4 ratchets but should be one long gate
            
            auto events = engine.processStageGate(stage, 0, sampleRate, samplesPerPulse);
            
            // Should have 2 events (1 note-on + 1 note-off)
            expect(events.size() == 2, "HOLD should generate 2 events regardless of ratchets");
            expect(events[0].isNoteOn == true, "First event should be note-on");
            expect(events[1].isNoteOn == false, "Second event should be note-off");
            expect(events[1].sampleOffset > events[0].sampleOffset, "Note-off should come after note-on");
        }
        
        // Test SINGLE gate type
        {
            stage.setGateType(2); // SINGLE
            stage.setRatchetCount(0, 4); // 4 ratchets but only first triggers
            
            auto events = engine.processStageGate(stage, 0, sampleRate, samplesPerPulse);
            
            // Should have 2 events (1 note-on + 1 note-off)
            expect(events.size() == 2, "SINGLE should generate 2 events");
            expect(events[0].ratchetIndex == 0, "Should only trigger on first ratchet");
        }
        
        // Test REST gate type
        {
            stage.setGateType(3); // REST
            stage.setRatchetCount(0, 4);
            
            auto events = engine.processStageGate(stage, 0, sampleRate, samplesPerPulse);
            
            // Should have no events
            expect(events.size() == 0, "REST should generate no events");
        }
    }
    
    void testRatcheting()
    {
        beginTest("Ratcheting");
        
        GateEngine engine;
        
        // Test ratchet pattern generation
        {
            auto offsets = engine.generateRatchetPattern(4, 1000);
            expect(offsets.size() == 4, "Should generate 4 offsets");
            expect(offsets[0] == 0, "First ratchet at offset 0");
            expect(offsets[1] == 250, "Second ratchet at 250");
            expect(offsets[2] == 500, "Third ratchet at 500");
            expect(offsets[3] == 750, "Fourth ratchet at 750");
        }
        
        // Test single ratchet
        {
            auto offsets = engine.generateRatchetPattern(1, 1000);
            expect(offsets.size() == 1, "Single ratchet pattern");
            expect(offsets[0] == 0, "Single ratchet at start");
        }
        
        // Test max ratchets
        {
            auto offsets = engine.generateRatchetPattern(8, 800);
            expect(offsets.size() == 8, "Should generate 8 offsets");
            
            // Check even spacing
            for (int i = 1; i < 8; ++i)
            {
                int spacing = offsets[i] - offsets[i-1];
                expect(spacing == 100, "Even spacing of 100 samples");
            }
        }
    }
    
    void testGateLength()
    {
        beginTest("Gate Length Calculation");
        
        GateEngine engine;
        
        // Test normal gate length
        {
            int length = engine.calculateGateLength(0.5f, 1000, GateEngine::GateType::MULTIPLE);
            expect(length == 500, "50% gate length should be 500 samples");
        }
        
        // Test minimum gate length
        {
            engine.setMinimumGateLength(10.0f); // 10ms minimum
            int length = engine.calculateGateLength(0.001f, 1000, GateEngine::GateType::MULTIPLE);
            int minSamples = static_cast<int>((10.0f / 1000.0f) * 48000.0f);
            expect(length >= minSamples, "Should respect minimum gate length");
        }
        
        // Test gate stretching for HOLD type
        {
            engine.setGateStretching(true);
            int length = engine.calculateGateLength(0.5f, 1000, GateEngine::GateType::HOLD);
            expect(length == 999, "Stretched HOLD gate should be full pulse minus 1");
        }
        
        // Test maximum gate length
        {
            int length = engine.calculateGateLength(2.0f, 1000, GateEngine::GateType::MULTIPLE);
            expect(length == 999, "Gate should not exceed pulse length");
        }
    }
    
    void testSwing()
    {
        beginTest("Swing Application");
        
        GateEngine engine;
        
        // Test no swing
        {
            int offset = engine.applySwing(100, 0.0f, false);
            expect(offset == 100, "No swing should not change offset");
        }
        
        // Test positive swing on odd beat
        {
            int offset = engine.applySwing(100, 0.5f, false);
            expect(offset > 100, "Positive swing should delay odd beats");
        }
        
        // Test negative swing on odd beat
        {
            int offset = engine.applySwing(100, -0.5f, false);
            expect(offset < 100, "Negative swing should advance odd beats");
        }
        
        // Test swing doesn't affect even beats
        {
            int offset = engine.applySwing(100, 0.5f, true);
            expect(offset == 100, "Swing should not affect even beats");
        }
    }
    
    void testProbability()
    {
        beginTest("Probability Testing");
        
        GateEngine engine;
        
        // Test always trigger
        {
            int triggers = 0;
            for (int i = 0; i < 100; ++i)
            {
                if (engine.shouldTrigger(1.0f))
                    triggers++;
            }
            expect(triggers == 100, "100% probability should always trigger");
        }
        
        // Test never trigger
        {
            int triggers = 0;
            for (int i = 0; i < 100; ++i)
            {
                if (engine.shouldTrigger(0.0f))
                    triggers++;
            }
            expect(triggers == 0, "0% probability should never trigger");
        }
        
        // Test 50% probability (statistical)
        {
            int triggers = 0;
            for (int i = 0; i < 1000; ++i)
            {
                if (engine.shouldTrigger(0.5f))
                    triggers++;
            }
            // Should be around 500, allow for variance
            expect(triggers > 400 && triggers < 600, 
                   "50% probability should trigger roughly half the time");
        }
    }
    
    void testGateMorphing()
    {
        beginTest("Gate Pattern Morphing");
        
        GateEngine engine;
        
        GateEngine::RatchetPattern pattern1;
        pattern1.pulseCount = 4;
        for (int i = 0; i < 8; ++i)
        {
            pattern1.subdivisions[i] = 1;
            pattern1.velocities[i] = 0.5f;
            pattern1.probabilities[i] = 1.0f;
        }
        
        GateEngine::RatchetPattern pattern2;
        pattern2.pulseCount = 8;
        for (int i = 0; i < 8; ++i)
        {
            pattern2.subdivisions[i] = 4;
            pattern2.velocities[i] = 1.0f;
            pattern2.probabilities[i] = 0.5f;
        }
        
        // Test 0% morph (should be pattern1)
        {
            auto result = engine.morphGatePatterns(pattern1, pattern2, 0.0f);
            expect(result.pulseCount == 4, "0% morph should use pattern1 pulse count");
            expect(result.subdivisions[0] == 1, "0% morph should use pattern1 subdivisions");
            expectWithinAbsoluteError(result.velocities[0], 0.5f, 0.01f, 
                                     "0% morph should use pattern1 velocities");
        }
        
        // Test 100% morph (should be pattern2)
        {
            auto result = engine.morphGatePatterns(pattern1, pattern2, 1.0f);
            expect(result.pulseCount == 8, "100% morph should use pattern2 pulse count");
            expect(result.subdivisions[0] == 4, "100% morph should use pattern2 subdivisions");
            expectWithinAbsoluteError(result.velocities[0], 1.0f, 0.01f,
                                     "100% morph should use pattern2 velocities");
        }
        
        // Test 50% morph
        {
            auto result = engine.morphGatePatterns(pattern1, pattern2, 0.5f);
            expect(result.pulseCount == 6, "50% morph should interpolate pulse count");
            expectWithinAbsoluteError(result.velocities[0], 0.75f, 0.01f,
                                     "50% morph should interpolate velocities");
            expectWithinAbsoluteError(result.probabilities[0], 0.75f, 0.01f,
                                     "50% morph should interpolate probabilities");
        }
    }
    
    void testTrackProcessor()
    {
        beginTest("Track Gate Processor");
        
        TrackGateProcessor processor;
        Track track;
        
        // Setup track with stages
        for (int i = 0; i < 8; ++i)
        {
            Stage& stage = track.getStage(i);
            stage.setPitch(60 + i);
            stage.setGateType(0); // MULTIPLE
            stage.setRatchetCount(0, 2); // 2 ratchets
            stage.setProbability(1.0f);
        }
        
        // Process first stage
        {
            auto events = processor.processTrackGates(&track, 0, 0, 48000.0, 1000);
            expect(events.size() > 0, "Should generate events for stage 0");
        }
        
        // Test duplicate prevention
        {
            auto events = processor.processTrackGates(&track, 0, 0, 48000.0, 1000);
            expect(events.size() == 0, "Should not reprocess same stage/pulse");
        }
        
        // Process different pulse
        {
            auto events = processor.processTrackGates(&track, 0, 1, 48000.0, 1000);
            expect(events.size() > 0, "Should generate events for different pulse");
        }
        
        // Test reset
        {
            processor.reset();
            auto events = processor.processTrackGates(&track, 0, 0, 48000.0, 1000);
            expect(events.size() > 0, "Should generate events after reset");
        }
    }
};

static GateEngineTests gateEngineTests;

//==============================================================================
// Main test runner
int main(int, char*[])
{
    juce::UnitTestRunner runner;
    runner.runAllTests();
    
    int numPassed = 0;
    int numFailed = 0;
    
    for (int i = 0; i < runner.getNumResults(); ++i)
    {
        auto* result = runner.getResult(i);
        numPassed += result->passes;
        numFailed += result->failures;
    }
    
    DBG("===========================================");
    DBG("GateEngine Test Results:");
    DBG("Tests Passed: " << numPassed);
    DBG("Tests Failed: " << numFailed);
    DBG("===========================================");
    
    return numFailed > 0 ? 1 : 0;
}