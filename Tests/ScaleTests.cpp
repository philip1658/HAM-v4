/*
  ==============================================================================

    ScaleTests.cpp
    Comprehensive unit tests for Scale model
    Coverage target: >80% line coverage

  ==============================================================================
*/

#include <JuceHeader.h>
#include "../Source/Domain/Models/Scale.h"

using namespace HAM;

//==============================================================================
class ScaleTests : public juce::UnitTest
{
public:
    ScaleTests() : UnitTest("Scale Tests", "Models") {}
    
    void runTest() override
    {
        testConstruction();
        testBuiltInScales();
        testPitchQuantization();
        testScaleGeneration();
        testCustomScales();
        testSerialization();
        testBoundaryConditions();
        testPerformance();
    }
    
private:
    void testConstruction()
    {
        beginTest("Construction and Initial State");
        
        Scale scale;
        
        // Test default state
        expectEquals(scale.getName(), juce::String("Chromatic"), "Default scale should be Chromatic");
        expectEquals(scale.getRootNote(), 0, "Default root note should be 0 (C)");
        expectEquals(scale.getSize(), 12, "Chromatic scale should have 12 notes");
        expect(scale.isChromatic(), "Default scale should be chromatic");
        
        // Test scale degrees
        for (int i = 0; i < 12; ++i)
        {
            expectEquals(scale.getDegree(i), i, "Chromatic scale degree " + juce::String(i) + " should be " + juce::String(i));
        }
    }
    
    void testBuiltInScales()
    {
        beginTest("Built-in Scale Types");
        
        Scale scale;
        
        // Test Major scale
        scale.setScaleType(Scale::Type::Major);
        expectEquals(scale.getName(), juce::String("Major"), "Should be Major scale");
        expectEquals(scale.getSize(), 7, "Major scale should have 7 notes");
        
        // Verify Major scale intervals (W-W-H-W-W-W-H)
        const int majorIntervals[] = {0, 2, 4, 5, 7, 9, 11};
        for (int i = 0; i < 7; ++i)
        {
            expectEquals(scale.getDegree(i), majorIntervals[i], 
                        "Major scale degree " + juce::String(i) + " incorrect");
        }
        
        // Test Minor scale
        scale.setScaleType(Scale::Type::Minor);
        expectEquals(scale.getName(), juce::String("Minor"), "Should be Minor scale");
        expectEquals(scale.getSize(), 7, "Minor scale should have 7 notes");
        
        // Verify Minor scale intervals (W-H-W-W-H-W-W)
        const int minorIntervals[] = {0, 2, 3, 5, 7, 8, 10};
        for (int i = 0; i < 7; ++i)
        {
            expectEquals(scale.getDegree(i), minorIntervals[i], 
                        "Minor scale degree " + juce::String(i) + " incorrect");
        }
        
        // Test Pentatonic scale
        scale.setScaleType(Scale::Type::Pentatonic);
        expectEquals(scale.getName(), juce::String("Pentatonic"), "Should be Pentatonic scale");
        expectEquals(scale.getSize(), 5, "Pentatonic scale should have 5 notes");
        
        // Test Blues scale
        scale.setScaleType(Scale::Type::Blues);
        expectEquals(scale.getName(), juce::String("Blues"), "Should be Blues scale");
        expectEquals(scale.getSize(), 6, "Blues scale should have 6 notes");
        
        // Test Dorian mode
        scale.setScaleType(Scale::Type::Dorian);
        expectEquals(scale.getName(), juce::String("Dorian"), "Should be Dorian mode");
        expectEquals(scale.getSize(), 7, "Dorian mode should have 7 notes");
        
        // Test Mixolydian mode
        scale.setScaleType(Scale::Type::Mixolydian);
        expectEquals(scale.getName(), juce::String("Mixolydian"), "Should be Mixolydian mode");
        expectEquals(scale.getSize(), 7, "Mixolydian mode should have 7 notes");
        
        // Test Lydian mode
        scale.setScaleType(Scale::Type::Lydian);
        expectEquals(scale.getName(), juce::String("Lydian"), "Should be Lydian mode");
        expectEquals(scale.getSize(), 7, "Lydian mode should have 7 notes");
        
        // Test Phrygian mode
        scale.setScaleType(Scale::Type::Phrygian);
        expectEquals(scale.getName(), juce::String("Phrygian"), "Should be Phrygian mode");
        expectEquals(scale.getSize(), 7, "Phrygian mode should have 7 notes");
    }
    
    void testPitchQuantization()
    {
        beginTest("Pitch Quantization");
        
        Scale scale;
        scale.setScaleType(Scale::Type::Major);
        scale.setRootNote(0); // C Major
        
        // Test quantization to scale degrees
        expectEquals(scale.quantizePitch(60), 60, "C (60) should stay C in C Major");
        expectEquals(scale.quantizePitch(61), 60, "C# (61) should quantize down to C");
        expectEquals(scale.quantizePitch(62), 62, "D (62) should stay D in C Major");
        expectEquals(scale.quantizePitch(63), 62, "D# (63) should quantize down to D");
        expectEquals(scale.quantizePitch(64), 64, "E (64) should stay E in C Major");
        expectEquals(scale.quantizePitch(65), 65, "F (65) should stay F in C Major");
        expectEquals(scale.quantizePitch(66), 65, "F# (66) should quantize down to F");
        expectEquals(scale.quantizePitch(67), 67, "G (67) should stay G in C Major");
        expectEquals(scale.quantizePitch(68), 67, "G# (68) should quantize down to G");
        expectEquals(scale.quantizePitch(69), 69, "A (69) should stay A in C Major");
        expectEquals(scale.quantizePitch(70), 69, "A# (70) should quantize down to A");
        expectEquals(scale.quantizePitch(71), 71, "B (71) should stay B in C Major");
        
        // Test with different root note (D Major)
        scale.setRootNote(2); // D
        expectEquals(scale.quantizePitch(62), 62, "D should stay D in D Major");
        expectEquals(scale.quantizePitch(63), 62, "D# should quantize to D in D Major");
        expectEquals(scale.quantizePitch(64), 64, "E should stay E in D Major");
        expectEquals(scale.quantizePitch(65), 64, "F should quantize to E in D Major");
        expectEquals(scale.quantizePitch(66), 66, "F# should stay F# in D Major");
        
        // Test quantization across octaves
        expectEquals(scale.quantizePitch(74), 74, "D in next octave should stay D");
        expectEquals(scale.quantizePitch(50), 50, "D in previous octave should stay D");
        
        // Test with pentatonic scale (fewer notes)
        scale.setScaleType(Scale::Type::Pentatonic);
        scale.setRootNote(0); // C Pentatonic
        
        expectEquals(scale.quantizePitch(60), 60, "C should stay in C Pentatonic");
        expectEquals(scale.quantizePitch(61), 60, "C# should quantize to C");
        expectEquals(scale.quantizePitch(62), 62, "D should stay in C Pentatonic");
        expectEquals(scale.quantizePitch(63), 62, "D# should quantize to D");
        expectEquals(scale.quantizePitch(64), 64, "E should stay in C Pentatonic");
        expectEquals(scale.quantizePitch(65), 64, "F should quantize to E (no F in pentatonic)");
    }
    
    void testScaleGeneration()
    {
        beginTest("Scale Generation from Intervals");
        
        Scale scale;
        
        // Generate custom scale from intervals
        std::vector<int> intervals = {2, 2, 1, 2, 2, 2, 1}; // Major scale intervals
        scale.setFromIntervals(intervals);
        
        expectEquals(scale.getSize(), 7, "Generated scale should have 7 notes");
        
        // Verify generated degrees match major scale
        const int expectedDegrees[] = {0, 2, 4, 5, 7, 9, 11};
        for (int i = 0; i < 7; ++i)
        {
            expectEquals(scale.getDegree(i), expectedDegrees[i], 
                        "Generated degree " + juce::String(i) + " should match major scale");
        }
        
        // Generate whole tone scale
        std::vector<int> wholeTone = {2, 2, 2, 2, 2, 2};
        scale.setFromIntervals(wholeTone);
        
        expectEquals(scale.getSize(), 6, "Whole tone scale should have 6 notes");
        for (int i = 0; i < 6; ++i)
        {
            expectEquals(scale.getDegree(i), i * 2, 
                        "Whole tone degree " + juce::String(i) + " should be " + juce::String(i * 2));
        }
        
        // Generate diminished scale
        std::vector<int> diminished = {2, 1, 2, 1, 2, 1, 2, 1};
        scale.setFromIntervals(diminished);
        
        expectEquals(scale.getSize(), 8, "Diminished scale should have 8 notes");
    }
    
    void testCustomScales()
    {
        beginTest("Custom Scale Creation");
        
        Scale scale;
        
        // Create custom scale with specific degrees
        std::vector<int> customDegrees = {0, 3, 5, 7, 10}; // Custom pentatonic variation
        scale.setCustomScale("Custom Penta", customDegrees);
        
        expectEquals(scale.getName(), juce::String("Custom Penta"), "Custom scale name should be set");
        expectEquals(scale.getSize(), 5, "Custom scale should have 5 notes");
        
        for (int i = 0; i < 5; ++i)
        {
            expectEquals(scale.getDegree(i), customDegrees[i], 
                        "Custom degree " + juce::String(i) + " should match");
        }
        
        // Test quantization with custom scale
        scale.setRootNote(0);
        expectEquals(scale.quantizePitch(60), 60, "C should be in custom scale");
        expectEquals(scale.quantizePitch(61), 60, "C# should quantize to C");
        expectEquals(scale.quantizePitch(62), 60, "D should quantize to C (not in scale)");
        expectEquals(scale.quantizePitch(63), 63, "D# (Eb) should be in custom scale");
        
        // Create microtonal scale (quarter-tone)
        std::vector<float> microtonalDegrees = {0.0f, 0.5f, 2.0f, 3.5f, 5.0f, 7.0f, 9.5f, 11.0f};
        scale.setMicrotonalScale("Quarter-tone", microtonalDegrees);
        
        expectEquals(scale.getName(), juce::String("Quarter-tone"), "Microtonal scale name should be set");
        expect(scale.isMicrotonal(), "Scale should be marked as microtonal");
    }
    
    void testSerialization()
    {
        beginTest("Serialization");
        
        Scale scale;
        
        // Setup scale
        scale.setScaleType(Scale::Type::Dorian);
        scale.setRootNote(5); // F Dorian
        scale.setName("F Dorian");
        
        // Create custom scale for testing
        std::vector<int> customDegrees = {0, 2, 3, 6, 7, 9};
        scale.setCustomScale("My Custom", customDegrees);
        
        // Serialize to ValueTree
        auto state = scale.toValueTree();
        
        expect(state.isValid(), "ValueTree should be valid");
        expectEquals(state.getType().toString(), juce::String("Scale"), "Type should be Scale");
        expectEquals(state.getProperty("name").toString(), juce::String("My Custom"), "Name should be serialized");
        expectEquals((int)state.getProperty("rootNote"), 5, "Root note should be serialized");
        
        // Create new scale from ValueTree
        Scale restored;
        restored.fromValueTree(state);
        
        expectEquals(restored.getName(), scale.getName(), "Name should be restored");
        expectEquals(restored.getRootNote(), scale.getRootNote(), "Root note should be restored");
        expectEquals(restored.getSize(), scale.getSize(), "Size should be restored");
        
        for (int i = 0; i < scale.getSize(); ++i)
        {
            expectEquals(restored.getDegree(i), scale.getDegree(i), 
                        "Degree " + juce::String(i) + " should be restored");
        }
        
        // Test JSON serialization
        auto json = scale.toJSON();
        expect(json.length() > 0, "Should produce JSON string");
        
        Scale jsonScale;
        bool loaded = jsonScale.fromJSON(json);
        expect(loaded, "Should load from JSON");
        
        expectEquals(jsonScale.getName(), scale.getName(), "JSON should preserve name");
        expectEquals(jsonScale.getRootNote(), scale.getRootNote(), "JSON should preserve root note");
        expectEquals(jsonScale.getSize(), scale.getSize(), "JSON should preserve size");
    }
    
    void testBoundaryConditions()
    {
        beginTest("Boundary Conditions");
        
        Scale scale;
        
        // Test with extreme MIDI values
        scale.setScaleType(Scale::Type::Major);
        
        expectEquals(scale.quantizePitch(0), 0, "Should handle MIDI note 0");
        expectEquals(scale.quantizePitch(127), 127, "Should handle MIDI note 127");
        expectEquals(scale.quantizePitch(-1), 0, "Negative MIDI should clamp to 0");
        expectEquals(scale.quantizePitch(128), 127, "MIDI > 127 should clamp to 127");
        
        // Test with invalid root note
        scale.setRootNote(-5);
        expectGreaterOrEqual(scale.getRootNote(), 0, "Root note should be clamped to valid range");
        
        scale.setRootNote(15);
        expectLessOrEqual(scale.getRootNote(), 11, "Root note should be clamped to 0-11");
        
        // Test empty custom scale
        std::vector<int> empty;
        scale.setCustomScale("Empty", empty);
        expectGreaterThan(scale.getSize(), 0, "Empty scale should default to chromatic");
        
        // Test very large custom scale
        std::vector<int> large;
        for (int i = 0; i < 100; ++i)
        {
            large.push_back(i % 12);
        }
        scale.setCustomScale("Large", large);
        expectLessOrEqual(scale.getSize(), 12, "Scale size should be limited to 12");
        
        // Test invalid intervals
        std::vector<int> invalidIntervals = {0, 0, 0};
        scale.setFromIntervals(invalidIntervals);
        expectGreaterThan(scale.getSize(), 0, "Invalid intervals should default to chromatic");
        
        // Test very long scale name
        juce::String longName;
        for (int i = 0; i < 1000; ++i)
            longName += "A";
        scale.setName(longName);
        expect(scale.getName().length() <= 256, "Name should be limited in length");
    }
    
    void testPerformance()
    {
        beginTest("Performance");
        
        Scale scale;
        scale.setScaleType(Scale::Type::Major);
        
        // Test quantization performance
        const int iterations = 10000;
        auto startTime = juce::Time::getMillisecondCounterHiRes();
        
        for (int i = 0; i < iterations; ++i)
        {
            int pitch = (i % 128);
            scale.quantizePitch(pitch);
        }
        
        auto endTime = juce::Time::getMillisecondCounterHiRes();
        auto elapsed = endTime - startTime;
        
        // Should complete 10000 quantizations in under 10ms
        expectLessThan(elapsed, 10.0, "Quantization should be fast");
        
        // Test scale switching performance
        startTime = juce::Time::getMillisecondCounterHiRes();
        
        for (int i = 0; i < 1000; ++i)
        {
            scale.setScaleType(static_cast<Scale::Type>(i % 8));
        }
        
        endTime = juce::Time::getMillisecondCounterHiRes();
        elapsed = endTime - startTime;
        
        // Should complete 1000 scale switches in under 10ms
        expectLessThan(elapsed, 10.0, "Scale switching should be fast");
        
        // Test serialization performance
        startTime = juce::Time::getMillisecondCounterHiRes();
        
        for (int i = 0; i < 100; ++i)
        {
            auto state = scale.toValueTree();
            Scale temp;
            temp.fromValueTree(state);
        }
        
        endTime = juce::Time::getMillisecondCounterHiRes();
        elapsed = endTime - startTime;
        
        // Should complete 100 serialization round-trips in under 50ms
        expectLessThan(elapsed, 50.0, "Serialization should be reasonably fast");
    }
};

static ScaleTests scaleTests;

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