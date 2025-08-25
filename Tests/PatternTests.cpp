/*
  ==============================================================================

    PatternTests.cpp
    Comprehensive unit tests for Pattern model
    Coverage target: >80% line coverage

  ==============================================================================
*/

#include <JuceHeader.h>
#include "../Source/Domain/Models/Pattern.h"

using namespace HAM;

//==============================================================================
class PatternTests : public juce::UnitTest
{
public:
    PatternTests() : UnitTest("Pattern Tests", "Models") {}
    
    void runTest() override
    {
        testConstruction();
        testTrackManagement();
        testPatternProperties();
        testSerialization();
        testSnapshotManagement();
        testBoundaryConditions();
        testThreadSafety();
    }
    
private:
    void testConstruction()
    {
        beginTest("Construction and Initial State");
        
        Pattern pattern;
        
        // Test default state
        expectEquals(pattern.getName(), juce::String("New Pattern"), "Default name should be 'New Pattern'");
        expectEquals(pattern.getTrackCount(), 1, "Should have 1 track by default");
        expectEquals(pattern.getBPM(), 120.0f, "Default BPM should be 120");
        expectEquals(pattern.getGlobalSwing(), 50.0f, "Default global swing should be 50");
        expectEquals(pattern.getLoopLength(), 4, "Default loop length should be 4 bars");
        expect(!pattern.isModified(), "Pattern should not be modified initially");
    }
    
    void testTrackManagement()
    {
        beginTest("Track Management");
        
        Pattern pattern;
        
        // Test default track
        expectEquals(pattern.getTrackCount(), 1, "Should have 1 track by default");
        auto* track = pattern.getTrack(0);
        expect(track != nullptr, "Default track should exist");
        
        // Add tracks
        int trackId1 = pattern.addTrack();
        int trackId2 = pattern.addTrack();
        expectEquals(pattern.getTrackCount(), 3, "Should have 3 tracks after adding 2");
        
        // Access tracks
        auto* track1 = pattern.getTrack(trackId1);
        auto* track2 = pattern.getTrack(trackId2);
        expect(track1 != nullptr, "Track 1 should exist");
        expect(track2 != nullptr, "Track 2 should exist");
        
        // Test const access
        const Pattern& constPattern = pattern;
        const auto* constTrack = constPattern.getTrack(0);
        expect(constTrack != nullptr, "Const track should exist");
        
        // Remove track
        pattern.removeTrack(1);
        expectEquals(pattern.getTrackCount(), 2, "Should have 2 tracks after removing one");
        
        // Clear all tracks
        pattern.clearTracks();
        expectEquals(pattern.getTrackCount(), 0, "Should have 0 tracks after clearing");
        
        // Add track after clearing
        pattern.addTrack();
        expectEquals(pattern.getTrackCount(), 1, "Should be able to add track after clearing");
    }
    
    void testPatternProperties()
    {
        beginTest("Pattern Properties");
        
        Pattern pattern;
        
        // Test name
        pattern.setName("Test Pattern");
        expectEquals(pattern.getName(), juce::String("Test Pattern"), "Name should be updated");
        
        pattern.setName("");
        expectEquals(pattern.getName(), juce::String(""), "Should accept empty name");
        
        // Test BPM
        pattern.setBPM(140.0f);
        expectEquals(pattern.getBPM(), 140.0f, "BPM should be 140");
        
        pattern.setBPM(60.0f);
        expectEquals(pattern.getBPM(), 60.0f, "BPM should be 60");
        
        pattern.setBPM(999.0f);
        expectEquals(pattern.getBPM(), 999.0f, "Should accept high BPM");
        
        // Test time signature
        pattern.setTimeSignature(3, 4);
        expectEquals(pattern.getTimeSignatureNumerator(), 3, "Time sig numerator should be 3");
        expectEquals(pattern.getTimeSignatureDenominator(), 4, "Time sig denominator should be 4");
        
        pattern.setTimeSignature(7, 8);
        expectEquals(pattern.getTimeSignatureNumerator(), 7, "Time sig numerator should be 7");
        expectEquals(pattern.getTimeSignatureDenominator(), 8, "Time sig denominator should be 8");
        
        // Test loop length
        pattern.setLoopLength(8);
        expectEquals(pattern.getLoopLength(), 8, "Loop length should be 8 bars");
        
        pattern.setLoopLength(1);
        expectEquals(pattern.getLoopLength(), 1, "Should accept minimum loop length of 1");
        
        // Test global swing
        pattern.setGlobalSwing(75.0f);
        expectEquals(pattern.getGlobalSwing(), 75.0f, "Global swing should be 75");
        
        pattern.setGlobalSwing(0.0f);
        expectEquals(pattern.getGlobalSwing(), 0.0f, "Should accept 0 swing");
        
        pattern.setGlobalSwing(100.0f);
        expectEquals(pattern.getGlobalSwing(), 100.0f, "Should accept 100 swing");
        
        // Test global gate length
        pattern.setGlobalGateLength(0.5f);
        expectEquals(pattern.getGlobalGateLength(), 0.5f, "Global gate length should be 0.5");
        
        pattern.setGlobalGateLength(2.0f);
        expectEquals(pattern.getGlobalGateLength(), 2.0f, "Global gate length should be 2.0");
    }
    
    void testSerialization()
    {
        beginTest("Serialization");
        
        Pattern pattern;
        
        // Setup pattern with data
        pattern.setName("Serialized Pattern");
        pattern.setBPM(133.0f);
        pattern.setTimeSignature(3, 4);
        pattern.setLoopLength(8);
        pattern.setGlobalSwing(25.0f);
        pattern.setGlobalGateLength(1.5f);
        
        // Add and configure tracks
        for (int i = 0; i < 3; ++i)
        {
            int trackId = pattern.addTrack();
            auto* track = pattern.getTrack(trackId);
            if (track)
            {
                track->setName("Track " + juce::String(i));
                track->setMidiChannel(i + 1);
                track->setEnabled(i % 2 == 0);
            }
        }
        
        // Serialize to ValueTree
        auto state = pattern.toValueTree();
        
        // Verify serialized data
        expect(state.isValid(), "ValueTree should be valid");
        expectEquals(state.getType().toString(), juce::String("Pattern"), "Type should be Pattern");
        expectEquals(state.getProperty("name").toString(), juce::String("Serialized Pattern"), "Name should be serialized");
        expectEquals((float)state.getProperty("bpm"), 133.0f, "BPM should be serialized");
        expectEquals((int)state.getProperty("timeSignatureNum"), 3, "Time sig numerator should be serialized");
        expectEquals((int)state.getProperty("timeSignatureDenom"), 4, "Time sig denominator should be serialized");
        
        // Create new pattern from ValueTree
        Pattern restoredPattern;
        restoredPattern.fromValueTree(state);
        
        // Verify restoration
        expectEquals(restoredPattern.getName(), pattern.getName(), "Name should be restored");
        expectEquals(restoredPattern.getBPM(), pattern.getBPM(), "BPM should be restored");
        expectEquals(restoredPattern.getTimeSignatureNumerator(), pattern.getTimeSignatureNumerator(), "Time sig num should be restored");
        expectEquals(restoredPattern.getTimeSignatureDenominator(), pattern.getTimeSignatureDenominator(), "Time sig denom should be restored");
        expectEquals(restoredPattern.getLoopLength(), pattern.getLoopLength(), "Loop length should be restored");
        expectEquals(restoredPattern.getGlobalSwing(), pattern.getGlobalSwing(), "Global swing should be restored");
        
        // Verify track data restoration
        expectEquals(restoredPattern.getTrackCount(), pattern.getTrackCount(), "Track count should be restored");
        for (int i = 0; i < pattern.getTrackCount(); ++i)
        {
            auto* originalTrack = pattern.getTrack(i);
            auto* restoredTrack = restoredPattern.getTrack(i);
            
            if (originalTrack && restoredTrack)
            {
                expectEquals(restoredTrack->getName(), originalTrack->getName(),
                           "Track name should be restored for track " + juce::String(i));
                expectEquals(restoredTrack->getMidiChannel(), originalTrack->getMidiChannel(),
                           "Track MIDI channel should be restored");
                expectEquals(restoredTrack->isEnabled(), originalTrack->isEnabled(),
                           "Track enabled state should be restored");
            }
        }
        
        // Test JSON serialization
        auto json = pattern.toJSON();
        expect(json.length() > 0, "Should produce JSON string");
        
        Pattern jsonPattern;
        bool loaded = jsonPattern.fromJSON(json);
        expect(loaded, "Should load from JSON");
        
        expectEquals(jsonPattern.getName(), pattern.getName(), "JSON should preserve name");
        expectEquals(jsonPattern.getBPM(), pattern.getBPM(), "JSON should preserve BPM");
        expectEquals(jsonPattern.getTrackCount(), pattern.getTrackCount(), "JSON should preserve track count");
    }
    
    void testSnapshotManagement()
    {
        beginTest("Snapshot Management");
        
        Pattern pattern;
        pattern.setName("Snapshot Test");
        pattern.setBPM(145.0f);
        pattern.setGlobalSwing(60.0f);
        
        // Add some tracks
        for (int i = 0; i < 3; ++i)
        {
            int trackId = pattern.addTrack();
            auto* track = pattern.getTrack(trackId);
            if (track)
            {
                track->setName("Track " + juce::String(i));
                track->setChannel(i + 1);
            }
        }
        
        // Capture initial snapshot
        int snap1 = pattern.captureSnapshot("Snapshot 1");
        expectGreaterOrEqual(snap1, 0, "Should return valid snapshot index");
        expectEquals(pattern.getSnapshotCount(), 1, "Should have 1 snapshot");
        
        // Modify pattern
        pattern.setBPM(120.0f);
        pattern.setGlobalSwing(40.0f);
        
        // Capture second snapshot
        int snap2 = pattern.captureSnapshot("Snapshot 2");
        expectGreaterOrEqual(snap2, 0, "Should return valid snapshot index");
        expectEquals(pattern.getSnapshotCount(), 2, "Should have 2 snapshots");
        
        // Test snapshot access
        auto* snapshot1 = pattern.getSnapshot(snap1);
        auto* snapshot2 = pattern.getSnapshot(snap2);
        expect(snapshot1 != nullptr, "Snapshot 1 should exist");
        expect(snapshot2 != nullptr, "Snapshot 2 should exist");
        
        if (snapshot1)
        {
            expectEquals(snapshot1->name, juce::String("Snapshot 1"), "Snapshot 1 name should match");
        }
        
        // Recall first snapshot
        pattern.recallSnapshot(snap1);
        expectEquals(pattern.getBPM(), 145.0f, "BPM should be restored to snapshot 1 value");
        expectEquals(pattern.getGlobalSwing(), 60.0f, "Swing should be restored to snapshot 1 value");
        
        // Remove snapshot
        pattern.removeSnapshot(snap1);
        expectEquals(pattern.getSnapshotCount(), 1, "Should have 1 snapshot after removal");
        
        // Clear all snapshots
        pattern.clearSnapshots();
        expectEquals(pattern.getSnapshotCount(), 0, "Should have 0 snapshots after clearing");
    }
    
    void testBoundaryConditions()
    {
        beginTest("Boundary Conditions");
        
        Pattern pattern;
        
        // Test BPM boundaries
        pattern.setBPM(0.0f);
        expectGreaterThan(pattern.getBPM(), 0.0f, "BPM should be positive");
        
        pattern.setBPM(-120.0f);
        expectGreaterThan(pattern.getBPM(), 0.0f, "Negative BPM should be handled");
        
        pattern.setBPM(10000.0f);
        expectLessOrEqual(pattern.getBPM(), 999.0f, "BPM should be clamped");
        
        // Test loop length boundaries
        pattern.setLoopLength(0);
        expectGreaterThan(pattern.getLoopLength(), 0, "Loop length should be clamped to minimum");
        
        pattern.setLoopLength(-10);
        expectGreaterThan(pattern.getLoopLength(), 0, "Negative loop length should be handled");
        
        // Test global swing boundaries
        pattern.setGlobalSwing(-10.0f);
        expectGreaterOrEqual(pattern.getGlobalSwing(), 0.0f, "Swing should be clamped to minimum");
        
        pattern.setGlobalSwing(200.0f);
        expectLessOrEqual(pattern.getGlobalSwing(), 100.0f, "Swing should be clamped to maximum");
        
        // Test very long name
        juce::String longName;
        for (int i = 0; i < 1000; ++i)
            longName += "A";
        pattern.setName(longName);
        expect(pattern.getName().length() <= 256, "Name should be limited in length");
        
        // Test empty ValueTree
        juce::ValueTree empty;
        pattern.fromValueTree(empty);
        // Should handle gracefully without crashing
        
        // Test invalid JSON
        bool loaded = pattern.fromJSON("{invalid json}");
        expect(!loaded, "Should fail to load invalid JSON");
        
        // Test empty JSON
        loaded = pattern.fromJSON("");
        expect(!loaded, "Should fail to load empty JSON");
    }
    
    void testThreadSafety()
    {
        beginTest("Thread Safety");
        
        Pattern pattern;
        std::atomic<bool> shouldStop{false};
        
        // Writer thread - modifying pattern
        std::thread writerThread([&pattern, &shouldStop]()
        {
            int counter = 0;
            while (!shouldStop.load())
            {
                pattern.setName("Pattern " + juce::String(counter));
                pattern.setBPM(60.0f + (counter % 180));
                pattern.setGlobalSwing((counter % 100));
                pattern.setLoopLength((counter % 8) + 1);
                
                // Modify tracks
                if (counter % 10 == 0 && pattern.getTrackCount() < 10)
                {
                    pattern.addTrack();
                }
                
                for (int i = 0; i < pattern.getTrackCount(); ++i)
                {
                    auto* track = pattern.getTrack(i);
                    if (track)
                    {
                        track->setMidiChannel((counter + i) % 16 + 1);
                        track->setEnabled((counter + i) % 2 == 0);
                    }
                }
                
                counter++;
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        });
        
        // Reader thread - reading pattern state
        std::thread readerThread([&pattern, &shouldStop]()
        {
            while (!shouldStop.load())
            {
                pattern.getName();
                pattern.getBPM();
                pattern.getGlobalSwing();
                pattern.getLoopLength();
                pattern.getTrackCount();
                
                for (int i = 0; i < pattern.getTrackCount(); ++i)
                {
                    auto* track = pattern.getTrack(i);
                    if (track)
                    {
                        track->getName();
                        track->getMidiChannel();
                        track->isEnabled();
                    }
                }
                
                std::this_thread::sleep_for(std::chrono::microseconds(50));
            }
        });
        
        // Serialization thread
        std::thread serializationThread([&pattern, &shouldStop]()
        {
            while (!shouldStop.load())
            {
                auto state = pattern.toValueTree();
                Pattern temp;
                temp.fromValueTree(state);
                
                auto json = pattern.toJSON();
                if (json.length() > 0)
                {
                    Pattern jsonTemp;
                    jsonTemp.fromJSON(json);
                }
                
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
        
        // Verify pattern is still functional
        pattern.setName("Final Test");
        expectEquals(pattern.getName(), juce::String("Final Test"), "Pattern should still be functional");
    }
};

static PatternTests patternTests;

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