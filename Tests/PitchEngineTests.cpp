/*
  ==============================================================================

    PitchEngineTests.cpp
    Unit tests for PitchEngine

  ==============================================================================
*/

#include <JuceHeader.h>
#include "../Source/Domain/Engines/PitchEngine.h"
#include "../Source/Domain/Models/Stage.h"
#include "../Source/Domain/Models/Scale.h"
#include "../Source/Domain/Models/Track.h"

using namespace HAM;

class PitchEngineTests : public juce::UnitTest
{
public:
    PitchEngineTests() : juce::UnitTest("PitchEngine Tests", "Engines") {}
    
    void runTest() override
    {
        testScaleQuantization();
        testChromaticMode();
        testOctaveOffsets();
        testNoteRangeLimiting();
        testChordQuantization();
        testCustomScale();
        testPitchProcessing();
        testTrackPitchProcessor();
    }
    
private:
    void testScaleQuantization()
    {
        beginTest("Scale Quantization");
        
        PitchEngine engine;
        
        // Set C Major scale
        engine.setScale(Scale::Major);
        engine.setRootNote(60); // C4
        engine.setQuantizationMode(PitchEngine::QuantizationMode::SCALE);
        
        // Test quantization of various notes
        // C# (61) should quantize to D (62) when snapping up
        expect(engine.quantizeToScale(61, true) == 62);
        
        // C# (61) should quantize to C (60) when snapping down
        expect(engine.quantizeToScale(61, false) == 60);
        
        // E (64) is in scale, should remain unchanged
        expect(engine.quantizeToScale(64, true) == 64);
        
        // F# (66) should quantize to G (67) when snapping up
        expect(engine.quantizeToScale(66, true) == 67);
        
        // Test with different octaves
        expect(engine.quantizeToScale(73, true) == 74); // C#5 -> D5
        expect(engine.quantizeToScale(49, false) == 48); // C#3 -> C3
    }
    
    void testChromaticMode()
    {
        beginTest("Chromatic Mode");
        
        PitchEngine engine;
        engine.setQuantizationMode(PitchEngine::QuantizationMode::CHROMATIC);
        
        Stage stage;
        stage.setPitch(65); // F4
        
        auto result = engine.processPitch(stage, 0, 0);
        
        // In chromatic mode, pitch should pass through unchanged
        expect(result.midiNote == 65);
        expect(!result.wasQuantized);
    }
    
    void testOctaveOffsets()
    {
        beginTest("Octave Offsets");
        
        PitchEngine engine;
        
        // Test positive octave offset
        expect(engine.applyOctaveOffset(60, 1) == 72); // C4 + 1 octave = C5
        expect(engine.applyOctaveOffset(60, 2) == 84); // C4 + 2 octaves = C6
        
        // Test negative octave offset
        expect(engine.applyOctaveOffset(60, -1) == 48); // C4 - 1 octave = C3
        expect(engine.applyOctaveOffset(60, -2) == 36); // C4 - 2 octaves = C2
        
        // Test no offset
        expect(engine.applyOctaveOffset(60, 0) == 60);
    }
    
    void testNoteRangeLimiting()
    {
        beginTest("Note Range Limiting");
        
        PitchEngine engine;
        
        // Test MIDI range limiting
        expect(engine.limitToMidiRange(-10) == 0);
        expect(engine.limitToMidiRange(150) == 127);
        expect(engine.limitToMidiRange(60) == 60);
        
        // Test custom range limiting
        engine.setNoteRange(48, 72); // C3 to C5
        
        expect(engine.getMinNote() == 48);
        expect(engine.getMaxNote() == 72);
        
        expect(engine.limitToMidiRange(40) == 48);
        expect(engine.limitToMidiRange(80) == 72);
        expect(engine.limitToMidiRange(60) == 60);
    }
    
    void testChordQuantization()
    {
        beginTest("Chord Quantization");
        
        PitchEngine engine;
        engine.setQuantizationMode(PitchEngine::QuantizationMode::CHORD);
        
        // Set C Major triad (C, E, G)
        int chordTones[] = {60, 64, 67};
        engine.setChordTones(chordTones, 3);
        
        // Test chord quantization through processPitch
        Stage stage;
        stage.setPitch(62); // D
        auto result = engine.processPitch(stage, 0, 0);
        expect(result.midiNote == 60 || result.midiNote == 64); // Should quantize to C or E
        
        stage.setPitch(65); // F
        result = engine.processPitch(stage, 0, 0);
        expect(result.midiNote == 64 || result.midiNote == 67); // Should quantize to E or G
        
        // Test clearing chord tones
        engine.clearChordTones();
        engine.setQuantizationMode(PitchEngine::QuantizationMode::CHROMATIC);
        result = engine.processPitch(stage, 0, 0);
        expect(result.midiNote == 65); // Should pass through unchanged
    }
    
    void testCustomScale()
    {
        beginTest("Custom Scale");
        
        PitchEngine engine;
        engine.setQuantizationMode(PitchEngine::QuantizationMode::CUSTOM);
        engine.setRootNote(60); // C4
        
        // Create pentatonic scale (0, 2, 4, 7, 9)
        int intervals[] = {0, 2, 4, 7, 9};
        engine.setCustomScale(intervals, 5);
        
        // Test custom scale through processPitch
        Stage stage;
        stage.setPitch(61); // C#
        auto result = engine.processPitch(stage, 0, 0);
        expect(result.midiNote == 60 || result.midiNote == 62); // Should quantize to C or D
        
        stage.setPitch(65); // F
        result = engine.processPitch(stage, 0, 0);
        expect(result.midiNote == 64 || result.midiNote == 67); // Should quantize to E or G
    }
    
    void testPitchProcessing()
    {
        beginTest("Pitch Processing");
        
        PitchEngine engine;
        
        // Set up scale
        engine.setScale(Scale::Minor);
        engine.setRootNote(57); // A3
        engine.setQuantizationMode(PitchEngine::QuantizationMode::SCALE);
        
        // Set transposition
        engine.setTransposition(2); // Up 2 semitones
        
        // Create a stage
        Stage stage;
        stage.setPitch(60); // C4
        stage.setOctave(1); // Up one octave
        stage.setVelocity(100);
        stage.setPitchBend(0.5f);
        
        // Process pitch
        auto result = engine.processPitch(stage, 0, 3); // With accumulator offset of 3
        
        // Expected: 60 + 0 + 3 + 2 (transposition) + 12 (octave) = 77
        // Then quantized to A minor scale
        expect(result.midiNote >= 0 && result.midiNote <= 127);
        expect(result.wasQuantized);
        expectWithinAbsoluteError(result.pitchBend, 0.5f, 0.001f);
    }
    
    void testTrackPitchProcessor()
    {
        beginTest("Track Pitch Processor");
        
        TrackPitchProcessor processor;
        
        // Create a track
        auto track = std::make_unique<Track>();
        track->setMidiChannel(1);
        
        // Set up first stage
        Stage& stage = track->getStage(0);
        stage.setPitch(4); // Relative pitch (4 semitones above base)
        stage.setOctave(0);
        stage.setVelocity(80);
        
        // Update scale
        processor.updateScale(Scale::Major);
        processor.getPitchEngine().setRootNote(60); // C Major
        
        // Set to chromatic mode first to ensure pitch passes through unchanged
        processor.getPitchEngine().setQuantizationMode(PitchEngine::QuantizationMode::CHROMATIC);
        
        // Process pitch for stage 0
        auto result = processor.processTrackPitch(track.get(), 0, 0);
        
        // In chromatic mode, base (60) + stage pitch (4) = 64 (E)
        expectEquals(result.midiNote, 64);
        
        // Test with accumulator offset
        result = processor.processTrackPitch(track.get(), 0, 5);
        expect(result.midiNote > 64); // Should be transposed up
        
        // Test reset
        processor.reset();
        
        // Test with null track
        result = processor.processTrackPitch(nullptr, 0, 0);
        expect(result.midiNote == 60); // Default value
        expect(!result.wasQuantized);
    }
};

static PitchEngineTests pitchEngineTests;

// Main function for console app
int main(int argc, char* argv[])
{
    juce::UnitTestRunner runner;
    runner.runAllTests();
    
    // Return 0 if all tests passed, 1 if any failed
    int numFailures = 0;
    for (int i = 0; i < runner.getNumResults(); ++i)
    {
        if (runner.getResult(i)->failures > 0)
            numFailures += runner.getResult(i)->failures;
    }
    
    return numFailures > 0 ? 1 : 0;
}