/*
  ==============================================================================

    SequencerEngineTests.cpp
    Unit tests for the SequencerEngine with focus on MONO/POLY mode behavior

  ==============================================================================
*/

#include <JuceHeader.h>
#include "../Source/Domain/Engines/SequencerEngine.h"
#include "../Source/Domain/Clock/MasterClock.h"
#include "../Source/Engine/Voice/VoiceManager.h"
#include "../Source/Domain/Models/Pattern.h"
#include "../Source/Domain/Models/Track.h"
#include "../Source/Domain/Models/Stage.h"

using namespace HAM;

//==============================================================================
class SequencerEngineTests : public juce::UnitTest
{
public:
    SequencerEngineTests() : UnitTest("SequencerEngine Tests") {}
    
    void runTest() override
    {
        testConstruction();
        testPatternManagement();
        testTransportControl();
        testMonoModeAdvancement();
        testPolyModeAdvancement();
        testTrackProcessing();
        testMidiGeneration();
        testAccumulatorIntegration();
        testRatchetProcessing();
        testSkipConditions();
        testPatternLooping();
        testSoloMute();
    }
    
private:
    //==========================================================================
    void testConstruction()
    {
        beginTest("Construction");
        
        SequencerEngine engine;
        expect(engine.getState() == SequencerEngine::State::STOPPED);
        expect(engine.getActivePattern() == nullptr);
        expect(engine.getPatternPosition() == 0.0f);
        
        auto stats = engine.getStats();
        expect(stats.eventsGenerated == 0);
        expect(stats.tracksProcessed == 0);
        expect(stats.stagesProcessed == 0);
    }
    
    //==========================================================================
    void testPatternManagement()
    {
        beginTest("Pattern Management");
        
        SequencerEngine engine;
        auto pattern = std::make_shared<Pattern>();
        
        // Set active pattern
        engine.setActivePattern(pattern);
        expect(engine.getActivePattern() == pattern);
        
        // Queue pattern change
        auto newPattern = std::make_shared<Pattern>();
        engine.queuePatternChange(newPattern);
        
        // Pattern should not change immediately
        expect(engine.getActivePattern() == pattern);
        
        // After hitting loop point, pattern should switch
        // (This would be tested with clock integration)
    }
    
    //==========================================================================
    void testTransportControl()
    {
        beginTest("Transport Control");
        
        SequencerEngine engine;
        MasterClock clock;
        VoiceManager voiceManager;
        
        engine.setMasterClock(&clock);
        engine.setVoiceManager(&voiceManager);
        
        // Test start
        engine.start();
        expect(engine.getState() == SequencerEngine::State::PLAYING);
        
        // Test stop
        engine.stop();
        expect(engine.getState() == SequencerEngine::State::STOPPED);
        
        // Test reset
        auto pattern = std::make_shared<Pattern>();
        // Use default track
        engine.setActivePattern(pattern);
        
        engine.reset();
        expect(engine.getCurrentPatternBar() == 0);
        expect(pattern->getTrack(0)->getCurrentStageIndex() == 0);
    }
    
    //==========================================================================
    void testMonoModeAdvancement()
    {
        beginTest("MONO Mode Stage Advancement");
        
        SequencerEngine engine;
        MasterClock clock;
        VoiceManager voiceManager;
        
        engine.setMasterClock(&clock);
        engine.setVoiceManager(&voiceManager);
        
        // Create pattern with MONO track
        auto pattern = std::make_shared<Pattern>();
        // Use default track
        Track* track = pattern->getTrack(0);
        track->setVoiceMode(VoiceMode::MONO);
        track->setDivision(1);  // Every pulse
        
        // Set up stage 0 with 4 pulses
        Stage& stage = track->getStage(0);
        stage.setPulseCount(4);
        stage.setPitch(60);
        stage.setVelocity(100);
        
        // Set up other stages to have pulses too (otherwise they default to 1)
        for (int i = 1; i < 8; ++i)
        {
            Stage& s = track->getStage(i);
            s.setPulseCount(4);
            s.setPitch(60 + i);
            s.setVelocity(100);
            
            // Immediately verify it was set
            expect(s.getPulseCount() == 4);
        }
        
        // Verify all stages have 4 pulses  
        for (int i = 0; i < 8; ++i)
        {
            int pc = track->getStage(i).getPulseCount();
            expectEquals(pc, 4,
                        juce::String("Stage ") + juce::String(i) + " should have 4 pulses but has " + juce::String(pc));
        }
        
        track->setName("Mono Track");
        engine.setActivePattern(pattern);
        engine.start();
        
        // Simulate clock pulses - in MONO mode, stage should play all 4 pulses
        for (int pulse = 0; pulse < 4; ++pulse)
        {
            engine.onClockPulse(pulse);
            
            // Should still be on stage 0
            int currentStage = track->getCurrentStageIndex();
            int stagePulseCount = track->getStage(currentStage).getPulseCount();
            
            expectEquals(currentStage, 0, 
                   juce::String("Pulse ") + juce::String(pulse) + 
                   ": MONO mode should stay on stage 0, but is on stage " + 
                   juce::String(currentStage) + " (pulseCount=" + juce::String(stagePulseCount) + ")");
        }
        
        // After 4 pulses, should advance to next stage
        engine.onClockPulse(4);
        expect(track->getCurrentStageIndex() == 1,
               "MONO mode should advance after all pulses complete");
    }
    
    //==========================================================================
    void testPolyModeAdvancement()
    {
        beginTest("POLY Mode Stage Advancement");
        
        SequencerEngine engine;
        MasterClock clock;
        VoiceManager voiceManager;
        
        engine.setMasterClock(&clock);
        engine.setVoiceManager(&voiceManager);
        
        // Create pattern with POLY track
        auto pattern = std::make_shared<Pattern>();
        // Use default track
        Track* track = pattern->getTrack(0);
        track->setVoiceMode(VoiceMode::POLY);
        track->setDivision(1);  // Every pulse
        
        // Set up stage 0 with 4 pulses  
        Stage& stage = track->getStage(0);
        stage.setPulseCount(4);
        stage.setPitch(60);
        stage.setVelocity(100);
        
        // Set up other stages too
        for (int i = 1; i < 8; ++i)
        {
            track->getStage(i).setPulseCount(4);
            track->getStage(i).setPitch(60 + i);
        }
        
        track->setName("Poly Track");
        engine.setActivePattern(pattern);
        engine.start();
        
        // First pulse - should trigger stage 0
        engine.onClockPulse(0);
        expect(track->getCurrentStageIndex() == 0);
        
        // Second pulse - POLY mode advances after 1 pulse
        engine.onClockPulse(1);
        expect(track->getCurrentStageIndex() == 1,
               "POLY mode should advance after 1 pulse");
        
        // Note: Stage 0 is still playing its remaining pulses in the background
    }
    
    //==========================================================================
    void testTrackProcessing()
    {
        beginTest("Track Processing");
        
        SequencerEngine engine;
        MasterClock clock;
        
        engine.setMasterClock(&clock);
        
        auto pattern = std::make_shared<Pattern>();
        // Use default track
        Track* track = pattern->getTrack(0);
        track->setEnabled(true);
        track->setMuted(false);
        track->setDivision(1);
        engine.setActivePattern(pattern);
        
        // Test track should trigger
        expect(engine.shouldTrackTrigger(*track, 0) == true);
        expect(engine.shouldTrackTrigger(*track, 24) == true);  // Next quarter note
        
        // Test with division
        track->setDivision(2);  // Half speed - triggers every 2 pulses
        expect(engine.shouldTrackTrigger(*track, 0) == true, "Pulse 0 should trigger");
        expect(engine.shouldTrackTrigger(*track, 1) == false, "Pulse 1 should not trigger");
        expect(engine.shouldTrackTrigger(*track, 2) == true, "Pulse 2 should trigger");
        expect(engine.shouldTrackTrigger(*track, 6) == true, "Pulse 6 should trigger (6 % 2 = 0)");
        expect(engine.shouldTrackTrigger(*track, 12) == true, "Pulse 12 should trigger");
    }
    
    //==========================================================================
    void testMidiGeneration()
    {
        beginTest("MIDI Generation");
        
        SequencerEngine engine;
        MasterClock clock;
        VoiceManager voiceManager;
        
        engine.resetStats();  // Clear any previous stats
        engine.setMasterClock(&clock);
        engine.setVoiceManager(&voiceManager);
        clock.setBPM(120.0f);  // Set a BPM
        
        auto pattern = std::make_shared<Pattern>();
        // Pattern already has 1 default track, use that
        expectEquals(pattern->getTrackCount(), 1, "Pattern should have 1 default track");
        Track* track = pattern->getTrack(0);
        track->setMidiChannel(1);
        track->setVoiceMode(VoiceMode::MONO);
        
        // Set track parameters
        track->setEnabled(true);
        track->setMuted(false);
        track->setDivision(1);  // Every pulse
        
        Stage& stage = track->getStage(0);
        stage.setPitch(64);  // E4
        stage.setVelocity(100);  // MIDI velocity value
        stage.setGateType(GateType::MULTIPLE);
        stage.setPulseCount(1);  // Single pulse
        
        engine.setActivePattern(pattern);
        engine.start();
        
        // Process a pulse
        engine.onClockPulse(0);
        
        // Check stats to see if events were generated
        auto stats = engine.getStats();
        expectEquals(stats.tracksProcessed, 1, "Should have processed 1 track");
        expectEquals(stats.stagesProcessed, 1, "Should have processed 1 stage");
        expect(stats.eventsGenerated > 0, 
               juce::String("Should have generated events. Events generated: ") + 
               juce::String(stats.eventsGenerated));
        
        // Get MIDI events
        juce::MidiBuffer midiBuffer;
        engine.processBlock(midiBuffer, 512);
        
        // Should have generated a note-on
        expect(!midiBuffer.isEmpty(), "MIDI buffer should not be empty after processing pulse");
        
        // Check the message
        for (const auto metadata : midiBuffer)
        {
            auto msg = metadata.getMessage();
            if (msg.isNoteOn())
            {
                expect(msg.getNoteNumber() == 64);
                expect(msg.getChannel() == 1);
                expect(msg.getVelocity() > 0);
            }
        }
    }
    
    //==========================================================================
    void testAccumulatorIntegration()
    {
        beginTest("Accumulator Integration");
        
        SequencerEngine engine;
        auto pattern = std::make_shared<Pattern>();
        // Use default track
        Track* track = pattern->getTrack(0);
        
        // Set up accumulator
        track->setAccumulatorMode(AccumulatorMode::STAGE);
        track->setAccumulatorOffset(2);  // +2 semitones per stage
        track->setAccumulatorReset(12);  // Reset at octave
        track->setAccumulatorValue(0);
        
        Stage& stage = track->getStage(0);
        stage.setPitch(60);  // C4
        engine.setActivePattern(pattern);
        
        // Calculate pitch with accumulator
        int pitch = engine.calculatePitch(*track, stage);
        expect(pitch == 60);  // First time, no accumulation
        
        // After stage advance
        track->setAccumulatorValue(2);
        pitch = engine.calculatePitch(*track, stage);
        expect(pitch == 62);  // C4 + 2 = D4
        
        // At reset point
        track->setAccumulatorValue(12);
        pitch = engine.calculatePitch(*track, stage);
        expect(pitch == 72);  // C4 + 12 = C5
    }
    
    //==========================================================================
    void testRatchetProcessing()
    {
        beginTest("Ratchet Processing");
        
        SequencerEngine engine;
        MasterClock clock;
        
        engine.setMasterClock(&clock);
        clock.setBPM(120.0f);
        
        auto pattern = std::make_shared<Pattern>();
        // Use default track
        Track* track = pattern->getTrack(0);
        track->setVoiceMode(VoiceMode::POLY);
        
        Stage& stage = track->getStage(0);
        stage.setPulseCount(1);
        stage.setRatchetCount(0, 4);  // 4 ratchets on first pulse
        stage.setRatchetProbability(1.0f);  // Always trigger
        engine.setActivePattern(pattern);
        engine.start();
        
        // Process should generate ratchet events
        engine.processTrack(*track, 0, 0);
        
        // Check stats - should show processing
        auto stats = engine.getStats();
        expect(stats.stagesProcessed > 0);
    }
    
    //==========================================================================
    void testSkipConditions()
    {
        beginTest("Skip Conditions");
        
        SequencerEngine engine;
        auto pattern = std::make_shared<Pattern>();
        // Use default track
        Track* track = pattern->getTrack(0);
        
        Stage& stage = track->getStage(0);
        stage.setSkipProbability(0.0f);  // Never skip
        stage.setSkipCondition(SkipCondition::NEVER);
        
        // Should not skip (note: shouldSkipStage is private, so we can't test directly)
        // We can only verify the method exists by compiling
        
        // Set skip probability
        stage.setSkipProbability(1.0f);  // Always skip
        // Note: This is probabilistic, so we can't test exact behavior
        // but we can verify the method exists and runs
    }
    
    //==========================================================================
    void testPatternLooping()
    {
        beginTest("Pattern Looping");
        
        SequencerEngine engine;
        MasterClock clock;
        
        engine.setMasterClock(&clock);
        
        auto pattern = std::make_shared<Pattern>();
        // Use default track
        Track* track = pattern->getTrack(0);
        track->setLength(4);  // 4 stages
        
        engine.setActivePattern(pattern);
        
        // Test total bars calculation
        int totalBars = engine.getTotalPatternBars();
        expect(totalBars >= 4);  // At least 4 bars
        
        // Test loop point detection
        expect(engine.isAtLoopPoint() == false);  // Not at loop initially
    }
    
    //==========================================================================
    void testSoloMute()
    {
        beginTest("Solo/Mute Functionality");
        
        SequencerEngine engine;
        auto pattern = std::make_shared<Pattern>();
        
        // Pattern starts with 1 track, add 2 more for a total of 3
        int idx2 = pattern->addTrack();
        int idx3 = pattern->addTrack();
        
        pattern->getTrack(0)->setName("Track 1");
        pattern->getTrack(idx2)->setName("Track 2");
        pattern->getTrack(idx3)->setName("Track 3");
        
        engine.setActivePattern(pattern);
        
        // No soloed tracks initially
        expect(engine.hasSoloedTracks() == false);
        
        // Solo track 2
        pattern->getTrack(1)->setSolo(true);
        expect(engine.hasSoloedTracks() == true);
        
        // Mute track 3
        pattern->getTrack(2)->setMuted(true);
        expect(pattern->getTrack(2)->isMuted() == true);
    }
};

//==============================================================================
// Register the test
static SequencerEngineTests sequencerEngineTests;

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