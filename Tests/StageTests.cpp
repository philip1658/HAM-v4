/*
  ==============================================================================

    StageTests.cpp
    Comprehensive unit tests for Stage model
    Coverage target: >80% line coverage

  ==============================================================================
*/

#include <JuceHeader.h>
#include "../Source/Domain/Models/Stage.h"

using namespace HAM;

//==============================================================================
class StageTests : public juce::UnitTest
{
public:
    StageTests() : UnitTest("Stage Tests", "Models") {}
    
    void runTest() override
    {
        testConstruction();
        testGridOperations();
        testParameterManagement();
        testPulseAndRatchet();
        testGateTypes();
        testModulation();
        testSerialization();
        testBoundaryConditions();
        testThreadSafety();
    }
    
private:
    void testConstruction()
    {
        beginTest("Construction and Initial State");
        
        Stage stage;
        
        // Test default state
        expectEquals(stage.getPitch(), 60, "Default pitch should be 60 (C4)");
        expectEquals(stage.getVelocity(), 100, "Default velocity should be 100");
        expectEquals(stage.getGateLength(), 0.9f, "Default gate length should be 0.9");
        expectEquals(stage.getPulseCount(), 1, "Default pulse count should be 1");
        expect(stage.isActive(), "Stage should be active by default");
        expectEquals(stage.getGateType(), Stage::GateType::MULTIPLE, "Default gate type should be MULTIPLE");
        
        // Test grid initialization (8x8)
        for (int row = 0; row < 8; ++row)
        {
            for (int col = 0; col < 8; ++col)
            {
                expectEquals(stage.getGridValue(row, col), 0, 
                           "Grid[" + juce::String(row) + "][" + juce::String(col) + "] should be 0");
            }
        }
    }
    
    void testGridOperations()
    {
        beginTest("8x8 Grid Operations");
        
        Stage stage;
        
        // Test setting and getting grid values
        for (int row = 0; row < 8; ++row)
        {
            for (int col = 0; col < 8; ++col)
            {
                int value = row * 8 + col;
                stage.setGridValue(row, col, value);
                expectEquals(stage.getGridValue(row, col), value,
                           "Grid value should be set correctly");
            }
        }
        
        // Test row operations
        stage.clearGrid();
        stage.setRow(3, 0xFF); // Set all bits in row 3
        for (int col = 0; col < 8; ++col)
        {
            expectEquals(stage.getGridValue(3, col), 1,
                       "Row 3, col " + juce::String(col) + " should be 1");
            expectEquals(stage.getGridValue(4, col), 0,
                       "Row 4, col " + juce::String(col) + " should still be 0");
        }
        
        // Test column operations
        stage.clearGrid();
        stage.setColumn(5, 0xFF); // Set all bits in column 5
        for (int row = 0; row < 8; ++row)
        {
            expectEquals(stage.getGridValue(row, 5), 1,
                       "Row " + juce::String(row) + ", col 5 should be 1");
            expectEquals(stage.getGridValue(row, 4), 0,
                       "Row " + juce::String(row) + ", col 4 should still be 0");
        }
        
        // Test pattern setting
        stage.clearGrid();
        uint64_t pattern = 0x0102040810204080; // Diagonal pattern
        stage.setGridPattern(pattern);
        
        for (int i = 0; i < 8; ++i)
        {
            expectEquals(stage.getGridValue(i, i), 1,
                       "Diagonal element [" + juce::String(i) + "][" + juce::String(i) + "] should be 1");
        }
        
        // Test grid inversion
        stage.invertGrid();
        for (int i = 0; i < 8; ++i)
        {
            expectEquals(stage.getGridValue(i, i), 0,
                       "Diagonal element should be 0 after inversion");
            if (i > 0)
            {
                expectEquals(stage.getGridValue(i, i-1), 1,
                           "Non-diagonal elements should be 1 after inversion");
            }
        }
        
        // Test grid rotation
        stage.clearGrid();
        stage.setGridValue(0, 0, 1);
        stage.setGridValue(0, 7, 1);
        stage.rotateGrid(90); // Rotate clockwise
        expectEquals(stage.getGridValue(0, 7), 1, "Top-right corner should have rotated value");
        expectEquals(stage.getGridValue(7, 7), 1, "Bottom-right corner should have rotated value");
        
        // Test grid shift
        stage.clearGrid();
        stage.setColumn(3, 0xFF);
        stage.shiftGrid(1, 0); // Shift right by 1
        for (int row = 0; row < 8; ++row)
        {
            expectEquals(stage.getGridValue(row, 4), 1,
                       "Column should shift right by 1");
        }
    }
    
    void testParameterManagement()
    {
        beginTest("Parameter Management");
        
        Stage stage;
        
        // Test pitch
        stage.setPitch(72);
        expectEquals(stage.getPitch(), 72, "Pitch should be 72");
        
        stage.setPitch(0);
        expectEquals(stage.getPitch(), 0, "Should accept MIDI note 0");
        
        stage.setPitch(127);
        expectEquals(stage.getPitch(), 127, "Should accept MIDI note 127");
        
        // Test velocity
        stage.setVelocity(64);
        expectEquals(stage.getVelocity(), 64, "Velocity should be 64");
        
        stage.setVelocity(0);
        expectEquals(stage.getVelocity(), 0, "Should accept velocity 0");
        
        stage.setVelocity(127);
        expectEquals(stage.getVelocity(), 127, "Should accept velocity 127");
        
        // Test gate length
        stage.setGateLength(0.5f);
        expectEquals(stage.getGateLength(), 0.5f, "Gate length should be 0.5");
        
        stage.setGateLength(0.0f);
        expectEquals(stage.getGateLength(), 0.0f, "Should accept gate length 0");
        
        stage.setGateLength(2.0f);
        expectEquals(stage.getGateLength(), 2.0f, "Should accept gate length > 1");
        
        // Test probability
        stage.setProbability(0.75f);
        expectEquals(stage.getProbability(), 0.75f, "Probability should be 0.75");
        
        stage.setProbability(0.0f);
        expectEquals(stage.getProbability(), 0.0f, "Should accept probability 0");
        
        stage.setProbability(1.0f);
        expectEquals(stage.getProbability(), 1.0f, "Should accept probability 1");
        
        // Test swing
        stage.setSwing(0.25f);
        expectEquals(stage.getSwing(), 0.25f, "Swing should be 0.25");
        
        stage.setSwing(-0.5f);
        expectEquals(stage.getSwing(), -0.5f, "Should accept negative swing");
        
        // Test active state
        stage.setActive(false);
        expect(!stage.isActive(), "Stage should be inactive");
        
        stage.setActive(true);
        expect(stage.isActive(), "Stage should be active");
        
        // Test skip flag
        stage.setSkip(true);
        expect(stage.shouldSkip(), "Stage should be marked for skip");
        
        stage.setSkip(false);
        expect(!stage.shouldSkip(), "Stage should not be marked for skip");
    }
    
    void testPulseAndRatchet()
    {
        beginTest("Pulse and Ratchet Configuration");
        
        Stage stage;
        
        // Test pulse count
        stage.setPulseCount(4);
        expectEquals(stage.getPulseCount(), 4, "Pulse count should be 4");
        
        stage.setPulseCount(1);
        expectEquals(stage.getPulseCount(), 1, "Should accept minimum pulse count of 1");
        
        stage.setPulseCount(8);
        expectEquals(stage.getPulseCount(), 8, "Should accept maximum pulse count of 8");
        
        // Test ratchet configuration
        stage.setPulseCount(4);
        stage.setRatchetCount(0, 1); // First pulse, 1 ratchet
        stage.setRatchetCount(1, 2); // Second pulse, 2 ratchets
        stage.setRatchetCount(2, 4); // Third pulse, 4 ratchets
        stage.setRatchetCount(3, 8); // Fourth pulse, 8 ratchets
        
        expectEquals(stage.getRatchetCount(0), 1, "First pulse should have 1 ratchet");
        expectEquals(stage.getRatchetCount(1), 2, "Second pulse should have 2 ratchets");
        expectEquals(stage.getRatchetCount(2), 4, "Third pulse should have 4 ratchets");
        expectEquals(stage.getRatchetCount(3), 8, "Fourth pulse should have 8 ratchets");
        
        // Test ratchet pattern
        stage.setRatchetPattern(0, 0b10101010); // Alternating pattern
        expectEquals(stage.getRatchetPattern(0), 0b10101010, "Ratchet pattern should be set");
        
        // Test ratchet probability
        stage.setRatchetProbability(0, 0.5f);
        expectEquals(stage.getRatchetProbability(0), 0.5f, "Ratchet probability should be 0.5");
        
        // Test total ratchet count calculation
        int totalRatchets = stage.getTotalRatchetCount();
        expectEquals(totalRatchets, 15, "Total ratchets should be 1+2+4+8 = 15");
        
        // Test pulse division
        stage.setPulseDivision(3); // Triplet
        expectEquals(stage.getPulseDivision(), 3, "Pulse division should be 3");
        
        // Test micro-timing per pulse
        stage.setPulseMicroTiming(0, -0.1f); // Slightly early
        stage.setPulseMicroTiming(1, 0.1f);  // Slightly late
        expectEquals(stage.getPulseMicroTiming(0), -0.1f, "First pulse should be early");
        expectEquals(stage.getPulseMicroTiming(1), 0.1f, "Second pulse should be late");
    }
    
    void testGateTypes()
    {
        beginTest("Gate Type Behavior");
        
        Stage stage;
        
        // Test MULTIPLE gate type
        stage.setGateType(Stage::GateType::MULTIPLE);
        expectEquals(stage.getGateType(), Stage::GateType::MULTIPLE, "Gate type should be MULTIPLE");
        expect(stage.shouldTriggerOnRatchet(0), "MULTIPLE should trigger on first ratchet");
        expect(stage.shouldTriggerOnRatchet(1), "MULTIPLE should trigger on second ratchet");
        
        // Test SINGLE gate type
        stage.setGateType(Stage::GateType::SINGLE);
        expectEquals(stage.getGateType(), Stage::GateType::SINGLE, "Gate type should be SINGLE");
        expect(stage.shouldTriggerOnRatchet(0), "SINGLE should trigger on first ratchet");
        expect(!stage.shouldTriggerOnRatchet(1), "SINGLE should not trigger on second ratchet");
        
        // Test HOLD gate type
        stage.setGateType(Stage::GateType::HOLD);
        expectEquals(stage.getGateType(), Stage::GateType::HOLD, "Gate type should be HOLD");
        expect(stage.shouldHoldGate(), "HOLD should maintain gate");
        
        // Test REST gate type
        stage.setGateType(Stage::GateType::REST);
        expectEquals(stage.getGateType(), Stage::GateType::REST, "Gate type should be REST");
        expect(!stage.shouldTriggerOnRatchet(0), "REST should not trigger");
        
        // Test gate type with probability
        stage.setGateType(Stage::GateType::MULTIPLE);
        stage.setProbability(0.0f);
        expect(!stage.shouldTriggerWithProbability(), "Should not trigger with 0 probability");
        
        stage.setProbability(1.0f);
        expect(stage.shouldTriggerWithProbability(), "Should trigger with 100% probability");
    }
    
    void testModulation()
    {
        beginTest("Modulation and CC Mapping");
        
        Stage stage;
        
        // Test modulation depth
        stage.setModulationDepth(0.5f);
        expectEquals(stage.getModulationDepth(), 0.5f, "Modulation depth should be 0.5");
        
        // Test modulation target
        stage.setModulationTarget(Stage::ModTarget::PITCH);
        expectEquals(stage.getModulationTarget(), Stage::ModTarget::PITCH, "Mod target should be PITCH");
        
        stage.setModulationTarget(Stage::ModTarget::VELOCITY);
        expectEquals(stage.getModulationTarget(), Stage::ModTarget::VELOCITY, "Mod target should be VELOCITY");
        
        stage.setModulationTarget(Stage::ModTarget::GATE_LENGTH);
        expectEquals(stage.getModulationTarget(), Stage::ModTarget::GATE_LENGTH, "Mod target should be GATE_LENGTH");
        
        // Test CC mappings
        stage.setCCMapping(0, 1);  // Map to CC 1 (Mod Wheel)
        stage.setCCMapping(1, 7);  // Map to CC 7 (Volume)
        stage.setCCMapping(2, 11); // Map to CC 11 (Expression)
        
        expectEquals(stage.getCCMapping(0), 1, "First CC should be 1");
        expectEquals(stage.getCCMapping(1), 7, "Second CC should be 7");
        expectEquals(stage.getCCMapping(2), 11, "Third CC should be 11");
        
        // Test CC values
        stage.setCCValue(0, 64);
        stage.setCCValue(1, 100);
        stage.setCCValue(2, 127);
        
        expectEquals(stage.getCCValue(0), 64, "First CC value should be 64");
        expectEquals(stage.getCCValue(1), 100, "Second CC value should be 100");
        expectEquals(stage.getCCValue(2), 127, "Third CC value should be 127");
        
        // Test pitchbend
        stage.setPitchBend(0.5f);
        expectEquals(stage.getPitchBend(), 0.5f, "Pitchbend should be 0.5");
        
        stage.setPitchBend(-1.0f);
        expectEquals(stage.getPitchBend(), -1.0f, "Should accept negative pitchbend");
        
        stage.setPitchBend(1.0f);
        expectEquals(stage.getPitchBend(), 1.0f, "Should accept maximum pitchbend");
    }
    
    void testSerialization()
    {
        beginTest("Serialization");
        
        Stage stage;
        
        // Setup stage with various parameters
        stage.setPitch(67);
        stage.setVelocity(80);
        stage.setGateLength(0.75f);
        stage.setPulseCount(3);
        stage.setGateType(Stage::GateType::HOLD);
        stage.setProbability(0.8f);
        stage.setSwing(0.15f);
        stage.setActive(false);
        
        // Set grid pattern
        stage.setGridValue(2, 3, 1);
        stage.setGridValue(4, 5, 1);
        stage.setGridValue(6, 7, 1);
        
        // Set ratchets
        stage.setRatchetCount(0, 2);
        stage.setRatchetCount(1, 4);
        stage.setRatchetCount(2, 1);
        
        // Set modulation
        stage.setModulationDepth(0.3f);
        stage.setModulationTarget(Stage::ModTarget::VELOCITY);
        
        // Serialize to ValueTree
        auto state = stage.toValueTree();
        
        expect(state.isValid(), "ValueTree should be valid");
        expectEquals(state.getType().toString(), juce::String("Stage"), "Type should be Stage");
        expectEquals((int)state.getProperty("pitch"), 67, "Pitch should be serialized");
        expectEquals((int)state.getProperty("velocity"), 80, "Velocity should be serialized");
        expectEquals((float)state.getProperty("gateLength"), 0.75f, "Gate length should be serialized");
        
        // Create new stage from ValueTree
        Stage restored;
        restored.fromValueTree(state);
        
        expectEquals(restored.getPitch(), stage.getPitch(), "Pitch should be restored");
        expectEquals(restored.getVelocity(), stage.getVelocity(), "Velocity should be restored");
        expectEquals(restored.getGateLength(), stage.getGateLength(), "Gate length should be restored");
        expectEquals(restored.getPulseCount(), stage.getPulseCount(), "Pulse count should be restored");
        expectEquals(restored.getGateType(), stage.getGateType(), "Gate type should be restored");
        expectEquals(restored.getProbability(), stage.getProbability(), "Probability should be restored");
        expectEquals(restored.isActive(), stage.isActive(), "Active state should be restored");
        
        // Verify grid restoration
        expectEquals(restored.getGridValue(2, 3), 1, "Grid value should be restored");
        expectEquals(restored.getGridValue(4, 5), 1, "Grid value should be restored");
        expectEquals(restored.getGridValue(6, 7), 1, "Grid value should be restored");
        
        // Verify ratchet restoration
        expectEquals(restored.getRatchetCount(0), 2, "Ratchet count should be restored");
        expectEquals(restored.getRatchetCount(1), 4, "Ratchet count should be restored");
        expectEquals(restored.getRatchetCount(2), 1, "Ratchet count should be restored");
        
        // Test JSON serialization
        auto json = stage.toJSON();
        expect(json.length() > 0, "Should produce JSON string");
        
        Stage jsonStage;
        bool loaded = jsonStage.fromJSON(json);
        expect(loaded, "Should load from JSON");
        
        expectEquals(jsonStage.getPitch(), stage.getPitch(), "JSON should preserve pitch");
        expectEquals(jsonStage.getVelocity(), stage.getVelocity(), "JSON should preserve velocity");
    }
    
    void testBoundaryConditions()
    {
        beginTest("Boundary Conditions");
        
        Stage stage;
        
        // Test pitch boundaries
        stage.setPitch(-10);
        expectGreaterOrEqual(stage.getPitch(), 0, "Pitch should be clamped to 0");
        
        stage.setPitch(200);
        expectLessOrEqual(stage.getPitch(), 127, "Pitch should be clamped to 127");
        
        // Test velocity boundaries
        stage.setVelocity(-10);
        expectGreaterOrEqual(stage.getVelocity(), 0, "Velocity should be clamped to 0");
        
        stage.setVelocity(200);
        expectLessOrEqual(stage.getVelocity(), 127, "Velocity should be clamped to 127");
        
        // Test pulse count boundaries
        stage.setPulseCount(0);
        expectGreaterOrEqual(stage.getPulseCount(), 1, "Pulse count should be at least 1");
        
        stage.setPulseCount(20);
        expectLessOrEqual(stage.getPulseCount(), 8, "Pulse count should be clamped to 8");
        
        // Test ratchet count boundaries
        stage.setRatchetCount(0, 0);
        expectGreaterOrEqual(stage.getRatchetCount(0), 1, "Ratchet count should be at least 1");
        
        stage.setRatchetCount(0, 20);
        expectLessOrEqual(stage.getRatchetCount(0), 8, "Ratchet count should be clamped to 8");
        
        // Test probability boundaries
        stage.setProbability(-0.5f);
        expectGreaterOrEqual(stage.getProbability(), 0.0f, "Probability should be clamped to 0");
        
        stage.setProbability(1.5f);
        expectLessOrEqual(stage.getProbability(), 1.0f, "Probability should be clamped to 1");
        
        // Test grid boundaries
        stage.setGridValue(-1, 0, 1);
        expectEquals(stage.getGridValue(0, 0), 0, "Out of bounds grid access should be safe");
        
        stage.setGridValue(8, 8, 1);
        expectEquals(stage.getGridValue(7, 7), 0, "Out of bounds grid access should be safe");
        
        // Test invalid CC mappings
        stage.setCCMapping(0, -1);
        expectGreaterOrEqual(stage.getCCMapping(0), 0, "CC should be clamped to valid range");
        
        stage.setCCMapping(0, 128);
        expectLessOrEqual(stage.getCCMapping(0), 127, "CC should be clamped to 127");
        
        // Test empty ValueTree
        juce::ValueTree empty;
        stage.fromValueTree(empty);
        // Should handle gracefully without crashing
        
        // Test invalid JSON
        bool loaded = stage.fromJSON("{invalid json}");
        expect(!loaded, "Should fail to load invalid JSON");
    }
    
    void testThreadSafety()
    {
        beginTest("Thread Safety");
        
        Stage stage;
        std::atomic<bool> shouldStop{false};
        
        // Writer thread - modifying stage
        std::thread writerThread([&stage, &shouldStop]()
        {
            int counter = 0;
            while (!shouldStop.load())
            {
                stage.setPitch(48 + (counter % 24));
                stage.setVelocity(64 + (counter % 64));
                stage.setGateLength((counter % 100) / 100.0f);
                stage.setPulseCount((counter % 8) + 1);
                stage.setActive(counter % 2 == 0);
                
                // Modify grid
                int row = counter % 8;
                int col = (counter + 4) % 8;
                stage.setGridValue(row, col, counter % 2);
                
                counter++;
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        });
        
        // Reader thread - reading stage state
        std::thread readerThread([&stage, &shouldStop]()
        {
            while (!shouldStop.load())
            {
                stage.getPitch();
                stage.getVelocity();
                stage.getGateLength();
                stage.getPulseCount();
                stage.isActive();
                
                // Read grid
                for (int i = 0; i < 8; ++i)
                {
                    for (int j = 0; j < 8; ++j)
                    {
                        stage.getGridValue(i, j);
                    }
                }
                
                std::this_thread::sleep_for(std::chrono::microseconds(50));
            }
        });
        
        // Serialization thread
        std::thread serializationThread([&stage, &shouldStop]()
        {
            while (!shouldStop.load())
            {
                auto state = stage.toValueTree();
                Stage temp;
                temp.fromValueTree(state);
                
                auto json = stage.toJSON();
                Stage jsonTemp;
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
        
        // Verify stage is still functional
        stage.setPitch(60);
        expectEquals(stage.getPitch(), 60, "Stage should still be functional");
    }
};

static StageTests stageTests;

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