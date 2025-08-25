/*
  ==============================================================================

    TrackTests.cpp
    Comprehensive unit tests for Track model
    Coverage target: >80% line coverage

  ==============================================================================
*/

#include <JuceHeader.h>
#include "../Source/Domain/Models/Track.h"
#include "../Source/Domain/Models/Stage.h"

using namespace HAM;

//==============================================================================
class TrackTests : public juce::UnitTest
{
public:
    TrackTests() : UnitTest("Track Tests", "Models") {}
    
    void runTest() override
    {
        testConstruction();
        testStageManagement();
        testTrackProperties();
        testMidiConfiguration();
        testVoiceMode();
        testPlaybackControl();
        testModulation();
        testSerialization();
        testBoundaryConditions();
        testThreadSafety();
    }
    
private:
    void testConstruction()
    {
        beginTest("Construction and Initial State");
        
        Track track;
        
        // Test default state
        expectEquals(track.getName(), juce::String("Track"), "Default name should be 'Track'");
        expectEquals(track.getChannel(), 1, "Default MIDI channel should be 1");
        expect(track.isActive(), "Track should be active by default");
        expect(!track.isMuted(), "Track should not be muted by default");
        expect(!track.isSoloed(), "Track should not be soloed by default");
        expectEquals(track.getStageCount(), 8, "Should have 8 stages by default");
        expectEquals(track.getVolume(), 1.0f, "Default volume should be 1.0");
        expectEquals(track.getPan(), 0.0f, "Default pan should be centered");
        expectEquals(track.getVoiceMode(), Track::VoiceMode::POLY, "Default voice mode should be POLY");
    }
    
    void testStageManagement()
    {
        beginTest("Stage Management");
        
        Track track;
        
        // Test default stages
        expectEquals(track.getStageCount(), 8, "Should have 8 stages");
        
        for (int i = 0; i < 8; ++i)
        {
            auto* stage = track.getStage(i);
            expect(stage != nullptr, "Stage " + juce::String(i) + " should exist");
            
            if (stage)
            {
                // Modify stage
                stage->setPitch(60 + i);
                stage->setVelocity(100 - i * 10);
                stage->setGateLength(0.5f + i * 0.05f);
                
                // Verify changes persist
                auto* sameStage = track.getStage(i);
                expectEquals(sameStage->getPitch(), 60 + i, "Pitch should be set");
                expectEquals(sameStage->getVelocity(), 100 - i * 10, "Velocity should be set");
                expectEquals(sameStage->getGateLength(), 0.5f + i * 0.05f, "Gate length should be set");
            }
        }
        
        // Test const access
        const Track& constTrack = track;
        for (int i = 0; i < 8; ++i)
        {
            const auto* constStage = constTrack.getStage(i);
            expect(constStage != nullptr, "Const stage " + juce::String(i) + " should exist");
        }
        
        // Test out of bounds access
        auto* invalidStage = track.getStage(-1);
        expect(invalidStage == nullptr, "Negative index should return nullptr");
        
        invalidStage = track.getStage(8);
        expect(invalidStage == nullptr, "Out of bounds index should return nullptr");
        
        // Test current stage tracking
        track.setCurrentStage(3);
        expectEquals(track.getCurrentStageIndex(), 3, "Current stage should be 3");
        
        auto* currentStage = track.getCurrentStage();
        expect(currentStage == track.getStage(3), "Should return correct current stage");
        
        // Test stage advancement
        track.advanceStage();
        expectEquals(track.getCurrentStageIndex(), 4, "Should advance to stage 4");
        
        // Test wrap around
        track.setCurrentStage(7);
        track.advanceStage();
        expectEquals(track.getCurrentStageIndex(), 0, "Should wrap to stage 0");
        
        // Test reset
        track.resetStagePosition();
        expectEquals(track.getCurrentStageIndex(), 0, "Should reset to stage 0");
    }
    
    void testTrackProperties()
    {
        beginTest("Track Properties");
        
        Track track;
        
        // Test name
        track.setName("Lead Synth");
        expectEquals(track.getName(), juce::String("Lead Synth"), "Name should be updated");
        
        track.setName("");
        expectEquals(track.getName(), juce::String(""), "Should accept empty name");
        
        // Test color
        juce::Colour testColor(255, 128, 64);
        track.setColor(testColor);
        expectEquals(track.getColor(), testColor, "Color should be set");
        
        // Test volume
        track.setVolume(0.75f);
        expectEquals(track.getVolume(), 0.75f, "Volume should be 0.75");
        
        track.setVolume(0.0f);
        expectEquals(track.getVolume(), 0.0f, "Should accept minimum volume");
        
        track.setVolume(2.0f);
        expectEquals(track.getVolume(), 2.0f, "Should accept volume > 1");
        
        // Test pan
        track.setPan(-1.0f);
        expectEquals(track.getPan(), -1.0f, "Should accept full left pan");
        
        track.setPan(1.0f);
        expectEquals(track.getPan(), 1.0f, "Should accept full right pan");
        
        track.setPan(0.0f);
        expectEquals(track.getPan(), 0.0f, "Should accept center pan");
        
        // Test active state
        track.setActive(false);
        expect(!track.isActive(), "Track should be inactive");
        
        track.setActive(true);
        expect(track.isActive(), "Track should be active");
        
        // Test mute
        track.setMuted(true);
        expect(track.isMuted(), "Track should be muted");
        
        track.setMuted(false);
        expect(!track.isMuted(), "Track should be unmuted");
        
        // Test solo
        track.setSoloed(true);
        expect(track.isSoloed(), "Track should be soloed");
        
        track.setSoloed(false);
        expect(!track.isSoloed(), "Track should be unsoloed");
        
        // Test record arm
        track.setRecordArmed(true);
        expect(track.isRecordArmed(), "Track should be record armed");
        
        track.setRecordArmed(false);
        expect(!track.isRecordArmed(), "Track should not be record armed");
    }
    
    void testMidiConfiguration()
    {
        beginTest("MIDI Configuration");
        
        Track track;
        
        // Test MIDI channel
        track.setChannel(5);
        expectEquals(track.getChannel(), 5, "MIDI channel should be 5");
        
        track.setChannel(1);
        expectEquals(track.getChannel(), 1, "Should accept channel 1");
        
        track.setChannel(16);
        expectEquals(track.getChannel(), 16, "Should accept channel 16");
        
        // Test MIDI output port
        track.setMidiOutputPort(2);
        expectEquals(track.getMidiOutputPort(), 2, "MIDI output port should be 2");
        
        // Test MIDI input filter
        track.setMidiInputEnabled(true);
        expect(track.isMidiInputEnabled(), "MIDI input should be enabled");
        
        track.setMidiInputChannel(10);
        expectEquals(track.getMidiInputChannel(), 10, "MIDI input channel should be 10");
        
        // Test note range
        track.setNoteRangeLow(36);  // C2
        track.setNoteRangeHigh(84); // C6
        expectEquals(track.getNoteRangeLow(), 36, "Low note range should be 36");
        expectEquals(track.getNoteRangeHigh(), 84, "High note range should be 84");
        
        // Test velocity range
        track.setVelocityRangeLow(40);
        track.setVelocityRangeHigh(120);
        expectEquals(track.getVelocityRangeLow(), 40, "Low velocity range should be 40");
        expectEquals(track.getVelocityRangeHigh(), 120, "High velocity range should be 120");
        
        // Test transpose
        track.setTranspose(7);
        expectEquals(track.getTranspose(), 7, "Transpose should be +7 semitones");
        
        track.setTranspose(-12);
        expectEquals(track.getTranspose(), -12, "Transpose should be -12 semitones");
        
        // Test octave shift
        track.setOctaveShift(2);
        expectEquals(track.getOctaveShift(), 2, "Octave shift should be +2");
        
        track.setOctaveShift(-1);
        expectEquals(track.getOctaveShift(), -1, "Octave shift should be -1");
    }
    
    void testVoiceMode()
    {
        beginTest("Voice Mode Configuration");
        
        Track track;
        
        // Test POLY mode
        track.setVoiceMode(Track::VoiceMode::POLY);
        expectEquals(track.getVoiceMode(), Track::VoiceMode::POLY, "Voice mode should be POLY");
        expectEquals(track.getMaxPolyphony(), 16, "Default polyphony should be 16");
        
        track.setMaxPolyphony(8);
        expectEquals(track.getMaxPolyphony(), 8, "Max polyphony should be 8");
        
        // Test MONO mode
        track.setVoiceMode(Track::VoiceMode::MONO);
        expectEquals(track.getVoiceMode(), Track::VoiceMode::MONO, "Voice mode should be MONO");
        expect(track.getGlideEnabled(), "Glide should be available in MONO mode");
        
        track.setGlideTime(100.0f);
        expectEquals(track.getGlideTime(), 100.0f, "Glide time should be 100ms");
        
        track.setGlideEnabled(false);
        expect(!track.getGlideEnabled(), "Glide should be disabled");
        
        // Test UNISON mode
        track.setVoiceMode(Track::VoiceMode::UNISON);
        expectEquals(track.getVoiceMode(), Track::VoiceMode::UNISON, "Voice mode should be UNISON");
        
        track.setUnisonVoices(4);
        expectEquals(track.getUnisonVoices(), 4, "Unison voices should be 4");
        
        track.setUnisonDetune(0.1f);
        expectEquals(track.getUnisonDetune(), 0.1f, "Unison detune should be 0.1");
        
        track.setUnisonSpread(0.5f);
        expectEquals(track.getUnisonSpread(), 0.5f, "Unison spread should be 0.5");
        
        // Test voice stealing
        track.setVoiceStealingEnabled(true);
        expect(track.isVoiceStealingEnabled(), "Voice stealing should be enabled");
        
        track.setVoiceStealingMode(Track::StealingMode::OLDEST);
        expectEquals(track.getVoiceStealingMode(), Track::StealingMode::OLDEST, "Stealing mode should be OLDEST");
        
        track.setVoiceStealingMode(Track::StealingMode::NEWEST);
        expectEquals(track.getVoiceStealingMode(), Track::StealingMode::NEWEST, "Stealing mode should be NEWEST");
        
        track.setVoiceStealingMode(Track::StealingMode::QUIETEST);
        expectEquals(track.getVoiceStealingMode(), Track::StealingMode::QUIETEST, "Stealing mode should be QUIETEST");
    }
    
    void testPlaybackControl()
    {
        beginTest("Playback Control");
        
        Track track;
        
        // Test play direction
        track.setPlayDirection(Track::Direction::FORWARD);
        expectEquals(track.getPlayDirection(), Track::Direction::FORWARD, "Direction should be FORWARD");
        
        track.setPlayDirection(Track::Direction::BACKWARD);
        expectEquals(track.getPlayDirection(), Track::Direction::BACKWARD, "Direction should be BACKWARD");
        
        track.setPlayDirection(Track::Direction::PINGPONG);
        expectEquals(track.getPlayDirection(), Track::Direction::PINGPONG, "Direction should be PINGPONG");
        
        track.setPlayDirection(Track::Direction::RANDOM);
        expectEquals(track.getPlayDirection(), Track::Direction::RANDOM, "Direction should be RANDOM");
        
        // Test loop settings
        track.setLoopEnabled(true);
        expect(track.isLoopEnabled(), "Loop should be enabled");
        
        track.setLoopStart(2);
        track.setLoopEnd(6);
        expectEquals(track.getLoopStart(), 2, "Loop start should be stage 2");
        expectEquals(track.getLoopEnd(), 6, "Loop end should be stage 6");
        
        // Test clock division
        track.setClockDivision(Track::Division::EIGHTH);
        expectEquals(track.getClockDivision(), Track::Division::EIGHTH, "Clock division should be EIGHTH");
        
        track.setClockDivision(Track::Division::QUARTER_TRIPLET);
        expectEquals(track.getClockDivision(), Track::Division::QUARTER_TRIPLET, "Clock division should be QUARTER_TRIPLET");
        
        // Test swing
        track.setSwing(0.25f);
        expectEquals(track.getSwing(), 0.25f, "Swing should be 0.25");
        
        // Test shuffle
        track.setShuffleEnabled(true);
        expect(track.isShuffleEnabled(), "Shuffle should be enabled");
        
        track.setShuffleAmount(0.67f);
        expectEquals(track.getShuffleAmount(), 0.67f, "Shuffle amount should be 0.67");
        
        // Test probability
        track.setProbability(0.8f);
        expectEquals(track.getProbability(), 0.8f, "Probability should be 0.8");
        
        // Test humanization
        track.setHumanizeEnabled(true);
        expect(track.isHumanizeEnabled(), "Humanize should be enabled");
        
        track.setHumanizeTiming(0.05f);
        expectEquals(track.getHumanizeTiming(), 0.05f, "Humanize timing should be 0.05");
        
        track.setHumanizeVelocity(0.1f);
        expectEquals(track.getHumanizeVelocity(), 0.1f, "Humanize velocity should be 0.1");
    }
    
    void testModulation()
    {
        beginTest("Modulation and Automation");
        
        Track track;
        
        // Test LFO
        track.setLFOEnabled(true);
        expect(track.isLFOEnabled(), "LFO should be enabled");
        
        track.setLFORate(2.0f);
        expectEquals(track.getLFORate(), 2.0f, "LFO rate should be 2.0 Hz");
        
        track.setLFODepth(0.5f);
        expectEquals(track.getLFODepth(), 0.5f, "LFO depth should be 0.5");
        
        track.setLFOShape(Track::LFOShape::SINE);
        expectEquals(track.getLFOShape(), Track::LFOShape::SINE, "LFO shape should be SINE");
        
        track.setLFOShape(Track::LFOShape::TRIANGLE);
        expectEquals(track.getLFOShape(), Track::LFOShape::TRIANGLE, "LFO shape should be TRIANGLE");
        
        track.setLFOShape(Track::LFOShape::SQUARE);
        expectEquals(track.getLFOShape(), Track::LFOShape::SQUARE, "LFO shape should be SQUARE");
        
        track.setLFOShape(Track::LFOShape::RANDOM);
        expectEquals(track.getLFOShape(), Track::LFOShape::RANDOM, "LFO shape should be RANDOM");
        
        // Test envelope
        track.setEnvelopeEnabled(true);
        expect(track.isEnvelopeEnabled(), "Envelope should be enabled");
        
        track.setEnvelopeAttack(10.0f);
        track.setEnvelopeDecay(50.0f);
        track.setEnvelopeSustain(0.7f);
        track.setEnvelopeRelease(200.0f);
        
        expectEquals(track.getEnvelopeAttack(), 10.0f, "Attack should be 10ms");
        expectEquals(track.getEnvelopeDecay(), 50.0f, "Decay should be 50ms");
        expectEquals(track.getEnvelopeSustain(), 0.7f, "Sustain should be 0.7");
        expectEquals(track.getEnvelopeRelease(), 200.0f, "Release should be 200ms");
        
        // Test automation lanes
        track.setAutomationLaneEnabled(0, true);
        expect(track.isAutomationLaneEnabled(0), "Automation lane 0 should be enabled");
        
        track.setAutomationTarget(0, Track::AutoTarget::VOLUME);
        expectEquals(track.getAutomationTarget(0), Track::AutoTarget::VOLUME, "Auto target should be VOLUME");
        
        track.setAutomationTarget(1, Track::AutoTarget::PAN);
        expectEquals(track.getAutomationTarget(1), Track::AutoTarget::PAN, "Auto target should be PAN");
        
        track.setAutomationTarget(2, Track::AutoTarget::CUTOFF);
        expectEquals(track.getAutomationTarget(2), Track::AutoTarget::CUTOFF, "Auto target should be CUTOFF");
    }
    
    void testSerialization()
    {
        beginTest("Serialization");
        
        Track track;
        
        // Setup track with various properties
        track.setName("Test Track");
        track.setChannel(5);
        track.setVolume(0.8f);
        track.setPan(-0.3f);
        track.setMuted(true);
        track.setSoloed(false);
        track.setVoiceMode(Track::VoiceMode::MONO);
        track.setPlayDirection(Track::Direction::PINGPONG);
        
        // Configure stages
        for (int i = 0; i < 4; ++i)
        {
            auto* stage = track.getStage(i);
            if (stage)
            {
                stage->setPitch(60 + i * 2);
                stage->setVelocity(80 + i * 5);
            }
        }
        
        // Serialize to ValueTree
        auto state = track.toValueTree();
        
        expect(state.isValid(), "ValueTree should be valid");
        expectEquals(state.getType().toString(), juce::String("Track"), "Type should be Track");
        expectEquals(state.getProperty("name").toString(), juce::String("Test Track"), "Name should be serialized");
        expectEquals((int)state.getProperty("channel"), 5, "Channel should be serialized");
        
        // Create new track from ValueTree
        Track restored;
        restored.fromValueTree(state);
        
        expectEquals(restored.getName(), track.getName(), "Name should be restored");
        expectEquals(restored.getChannel(), track.getChannel(), "Channel should be restored");
        expectEquals(restored.getVolume(), track.getVolume(), "Volume should be restored");
        expectEquals(restored.getPan(), track.getPan(), "Pan should be restored");
        expectEquals(restored.isMuted(), track.isMuted(), "Mute state should be restored");
        expectEquals(restored.isSoloed(), track.isSoloed(), "Solo state should be restored");
        expectEquals(restored.getVoiceMode(), track.getVoiceMode(), "Voice mode should be restored");
        expectEquals(restored.getPlayDirection(), track.getPlayDirection(), "Play direction should be restored");
        
        // Verify stage restoration
        for (int i = 0; i < 4; ++i)
        {
            auto* originalStage = track.getStage(i);
            auto* restoredStage = restored.getStage(i);
            
            if (originalStage && restoredStage)
            {
                expectEquals(restoredStage->getPitch(), originalStage->getPitch(),
                           "Stage " + juce::String(i) + " pitch should be restored");
                expectEquals(restoredStage->getVelocity(), originalStage->getVelocity(),
                           "Stage " + juce::String(i) + " velocity should be restored");
            }
        }
        
        // Test JSON serialization
        auto json = track.toJSON();
        expect(json.length() > 0, "Should produce JSON string");
        
        Track jsonTrack;
        bool loaded = jsonTrack.fromJSON(json);
        expect(loaded, "Should load from JSON");
        
        expectEquals(jsonTrack.getName(), track.getName(), "JSON should preserve name");
        expectEquals(jsonTrack.getChannel(), track.getChannel(), "JSON should preserve channel");
    }
    
    void testBoundaryConditions()
    {
        beginTest("Boundary Conditions");
        
        Track track;
        
        // Test MIDI channel boundaries
        track.setChannel(0);
        expectGreaterOrEqual(track.getChannel(), 1, "Channel should be clamped to 1");
        
        track.setChannel(17);
        expectLessOrEqual(track.getChannel(), 16, "Channel should be clamped to 16");
        
        // Test volume boundaries
        track.setVolume(-0.5f);
        expectGreaterOrEqual(track.getVolume(), 0.0f, "Volume should be clamped to 0");
        
        track.setVolume(10.0f);
        expectLessOrEqual(track.getVolume(), 2.0f, "Volume should be reasonable");
        
        // Test pan boundaries
        track.setPan(-2.0f);
        expectGreaterOrEqual(track.getPan(), -1.0f, "Pan should be clamped to -1");
        
        track.setPan(2.0f);
        expectLessOrEqual(track.getPan(), 1.0f, "Pan should be clamped to 1");
        
        // Test stage index boundaries
        track.setCurrentStage(-1);
        expectGreaterOrEqual(track.getCurrentStageIndex(), 0, "Stage index should be clamped to 0");
        
        track.setCurrentStage(10);
        expectLessThan(track.getCurrentStageIndex(), 8, "Stage index should be clamped");
        
        // Test loop boundaries
        track.setLoopStart(-1);
        expectGreaterOrEqual(track.getLoopStart(), 0, "Loop start should be clamped to 0");
        
        track.setLoopEnd(10);
        expectLessOrEqual(track.getLoopEnd(), 7, "Loop end should be clamped to 7");
        
        track.setLoopStart(6);
        track.setLoopEnd(3);
        expect(track.getLoopStart() <= track.getLoopEnd(), "Loop range should be valid");
        
        // Test polyphony boundaries
        track.setMaxPolyphony(0);
        expectGreaterOrEqual(track.getMaxPolyphony(), 1, "Polyphony should be at least 1");
        
        track.setMaxPolyphony(200);
        expectLessOrEqual(track.getMaxPolyphony(), 64, "Polyphony should be reasonable");
        
        // Test very long name
        juce::String longName;
        for (int i = 0; i < 1000; ++i)
            longName += "A";
        track.setName(longName);
        expect(track.getName().length() <= 256, "Name should be limited in length");
        
        // Test empty ValueTree
        juce::ValueTree empty;
        track.fromValueTree(empty);
        // Should handle gracefully without crashing
        
        // Test invalid JSON
        bool loaded = track.fromJSON("{invalid json}");
        expect(!loaded, "Should fail to load invalid JSON");
    }
    
    void testThreadSafety()
    {
        beginTest("Thread Safety");
        
        Track track;
        std::atomic<bool> shouldStop{false};
        
        // Writer thread - modifying track
        std::thread writerThread([&track, &shouldStop]()
        {
            int counter = 0;
            while (!shouldStop.load())
            {
                track.setName("Track " + juce::String(counter));
                track.setChannel((counter % 16) + 1);
                track.setVolume((counter % 100) / 100.0f);
                track.setPan(((counter % 200) - 100) / 100.0f);
                track.setMuted(counter % 2 == 0);
                track.setSoloed(counter % 3 == 0);
                track.setCurrentStage(counter % 8);
                
                // Modify stages
                auto* stage = track.getStage(counter % 8);
                if (stage)
                {
                    stage->setPitch(48 + (counter % 24));
                    stage->setVelocity(64 + (counter % 64));
                }
                
                counter++;
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        });
        
        // Reader thread - reading track state
        std::thread readerThread([&track, &shouldStop]()
        {
            while (!shouldStop.load())
            {
                track.getName();
                track.getChannel();
                track.getVolume();
                track.getPan();
                track.isMuted();
                track.isSoloed();
                track.getCurrentStageIndex();
                
                for (int i = 0; i < 8; ++i)
                {
                    auto* stage = track.getStage(i);
                    if (stage)
                    {
                        stage->getPitch();
                        stage->getVelocity();
                    }
                }
                
                std::this_thread::sleep_for(std::chrono::microseconds(50));
            }
        });
        
        // Serialization thread
        std::thread serializationThread([&track, &shouldStop]()
        {
            while (!shouldStop.load())
            {
                auto state = track.toValueTree();
                Track temp;
                temp.fromValueTree(state);
                
                auto json = track.toJSON();
                Track jsonTemp;
                jsonTemp.fromJSON(json);
                
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
        
        // Let threads run
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        // Stop threads
        shouldStop = true;
        writerThread.join();
        readerThread.join();
        serializationThread.join();
        
        // If we get here without crashing, thread safety is working
        expect(true, "Thread safety test completed without crashes");
        
        // Verify track is still functional
        track.setName("Final Test");
        expectEquals(track.getName(), juce::String("Final Test"), "Track should still be functional");
    }
};

static TrackTests trackTests;

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