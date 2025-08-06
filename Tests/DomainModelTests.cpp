/*
  ==============================================================================

    DomainModelTests.cpp
    Unit tests for domain models

  ==============================================================================
*/

#include <JuceHeader.h>
#include "../Source/Domain/Models/Stage.h"
#include "../Source/Domain/Models/Track.h"
#include "../Source/Domain/Models/Pattern.h"
#include "../Source/Domain/Models/Scale.h"

using namespace HAM;

//==============================================================================
class StageTests : public juce::UnitTest
{
public:
    StageTests() : UnitTest("Stage Model Tests") {}
    
    void runTest() override
    {
        beginTest("Stage Default Values");
        {
            Stage stage;
            expect(stage.getPitch() == 60);
            expect(stage.getGate() == 0.5f);
            expect(stage.getVelocity() == 100);
            expect(stage.getPulseCount() == 1);
            expect(stage.getProbability() == 100.0f);
        }
        
        beginTest("Stage Parameter Setting");
        {
            Stage stage;
            stage.setPitch(72);
            expect(stage.getPitch() == 72);
            
            stage.setGate(0.75f);
            expectWithinAbsoluteError(stage.getGate(), 0.75f, 0.001f);
            
            stage.setVelocity(64);
            expect(stage.getVelocity() == 64);
        }
        
        beginTest("Stage Ratcheting");
        {
            Stage stage;
            stage.setRatchetCount(0, 4);
            expect(stage.getRatchetCount(0) == 4);
            
            stage.setRatchetCount(3, 8);
            expect(stage.getRatchetCount(3) == 8);
        }
        
        beginTest("Stage Serialization");
        {
            Stage stage;
            stage.setPitch(65);
            stage.setGate(0.8f);
            stage.setVelocity(110);
            stage.setSlide(true);
            
            auto tree = stage.toValueTree();
            
            Stage loaded;
            loaded.fromValueTree(tree);
            
            expect(loaded.getPitch() == 65);
            expectWithinAbsoluteError(loaded.getGate(), 0.8f, 0.001f);
            expect(loaded.getVelocity() == 110);
            expect(loaded.hasSlide() == true);
        }
    }
};

//==============================================================================
class TrackTests : public juce::UnitTest
{
public:
    TrackTests() : UnitTest("Track Model Tests") {}
    
    void runTest() override
    {
        beginTest("Track Default Values");
        {
            Track track;
            expect(track.getName() == "Track");
            expect(track.getMidiChannel() == 1);
            expect(track.getLength() == 8);
            expect(track.getVoiceMode() == VoiceMode::MONO);
        }
        
        beginTest("Track Stage Access");
        {
            Track track;
            auto& stage = track.getStage(0);
            stage.setPitch(48);
            expect(track.getStage(0).getPitch() == 48);
        }
        
        beginTest("Track MIDI Configuration");
        {
            Track track;
            track.setMidiChannel(5);
            expect(track.getMidiChannel() == 5);
            
            track.setVoiceMode(VoiceMode::POLY);
            expect(track.getVoiceMode() == VoiceMode::POLY);
            
            track.setMaxVoices(8);
            expect(track.getMaxVoices() == 8);
        }
        
        beginTest("Track Serialization");
        {
            Track track;
            track.setName("Test Track");
            track.setMidiChannel(3);
            track.setOctaveOffset(2);
            track.getStage(0).setPitch(67);
            
            auto tree = track.toValueTree();
            
            Track loaded;
            loaded.fromValueTree(tree);
            
            expect(loaded.getName() == "Test Track");
            expect(loaded.getMidiChannel() == 3);
            expect(loaded.getOctaveOffset() == 2);
            expect(loaded.getStage(0).getPitch() == 67);
        }
    }
};

//==============================================================================
class PatternTests : public juce::UnitTest
{
public:
    PatternTests() : UnitTest("Pattern Model Tests") {}
    
    void runTest() override
    {
        beginTest("Pattern Default Values");
        {
            Pattern pattern;
            expect(pattern.getName() == "New Pattern");
            expectWithinAbsoluteError(pattern.getBPM(), 120.0f, 0.001f);
            expect(pattern.getTrackCount() == 1);  // Default track
        }
        
        beginTest("Pattern Track Management");
        {
            Pattern pattern;
            int trackIndex = pattern.addTrack();
            expect(trackIndex == 1);
            expect(pattern.getTrackCount() == 2);
            
            auto track = pattern.getTrack(trackIndex);
            expect(track != nullptr);
            
            pattern.removeTrack(trackIndex);
            expect(pattern.getTrackCount() == 1);
        }
        
        beginTest("Pattern Snapshot Management");
        {
            Pattern pattern;
            pattern.setBPM(140.0f);
            
            int snapIndex = pattern.captureSnapshot("Test Snap");
            expect(snapIndex == 0);
            
            pattern.setBPM(100.0f);
            expectWithinAbsoluteError(pattern.getBPM(), 100.0f, 0.001f);
            
            pattern.recallSnapshot(snapIndex);
            expectWithinAbsoluteError(pattern.getBPM(), 140.0f, 0.001f);
        }
    }
};

//==============================================================================
class ScaleTests : public juce::UnitTest
{
public:
    ScaleTests() : UnitTest("Scale Model Tests") {}
    
    void runTest() override
    {
        beginTest("Scale Chromatic");
        {
            Scale chromatic = Scale::Chromatic;
            expect(chromatic.isChromatic());
            expect(chromatic.getSize() == 12);
            expect(chromatic.contains(60, 0));  // Any note is in chromatic
        }
        
        beginTest("Scale Major");
        {
            Scale major = Scale::Major;
            expect(!major.isChromatic());
            expect(major.getSize() == 7);
            
            // C major scale from C (60)
            expect(major.contains(60, 60));  // C
            expect(major.contains(62, 60));  // D
            expect(major.contains(64, 60));  // E
            expect(!major.contains(61, 60)); // C# not in C major
        }
        
        beginTest("Scale Quantization");
        {
            Scale major = Scale::Major;
            
            // Quantize C# to D in C major (C# is equidistant from C and D, round up to D)
            expect(major.quantize(61, 60) == 62);
            
            // Quantize D# to E in C major (D# is equidistant from D and E, round up to E)
            expect(major.quantize(63, 60) == 64);
        }
        
        beginTest("Scale Degree Calculation");
        {
            Scale major = Scale::Major;
            
            expect(major.getDegree(60, 60) == 0);  // C is degree 0
            expect(major.getDegree(62, 60) == 1);  // D is degree 1
            expect(major.getDegree(64, 60) == 2);  // E is degree 2
            expect(major.getDegree(61, 60) == -1); // C# not in scale
        }
    }
};

//==============================================================================
static StageTests stageTests;
static TrackTests trackTests;
static PatternTests patternTests;
static ScaleTests scaleTests;

//==============================================================================
int main(int argc, char* argv[])
{
    juce::UnitTestRunner runner;
    runner.runAllTests();
    
    int numPassed = 0;
    int numFailed = 0;
    
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