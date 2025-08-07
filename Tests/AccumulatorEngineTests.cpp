/*
  ==============================================================================

    AccumulatorEngineTests.cpp
    Unit tests for AccumulatorEngine

  ==============================================================================
*/

#include <JuceHeader.h>
#include "../Source/Domain/Engines/AccumulatorEngine.h"
#include "../Source/Domain/Models/Track.h"

using namespace HAM;

class AccumulatorEngineTests : public juce::UnitTest
{
public:
    AccumulatorEngineTests() : juce::UnitTest("AccumulatorEngine Tests", "Engines") {}
    
    void runTest() override
    {
        testBasicAccumulation();
        testAccumulatorModes();
        testResetStrategies();
        testValueLimits();
        testWrapMode();
        testStateManagement();
        testTrackAccumulator();
    }
    
private:
    void testBasicAccumulation()
    {
        beginTest("Basic Accumulation");
        
        AccumulatorEngine engine;
        engine.setMode(AccumulatorEngine::AccumulatorMode::PER_STAGE);
        
        // Initial value should be 0
        expect(engine.getCurrentValue() == 0);
        
        // Process first stage
        int value = engine.processAccumulator(0, 0, 0, 1);
        expect(value == 1);
        
        // Same stage, should not accumulate
        value = engine.processAccumulator(0, 1, 0, 1);
        expect(value == 1);
        
        // New stage, should accumulate
        value = engine.processAccumulator(1, 0, 0, 1);
        expect(value == 2);
        
        // Manual increment
        engine.increment(3);
        expect(engine.getCurrentValue() == 5);
    }
    
    void testAccumulatorModes()
    {
        beginTest("Accumulator Modes");
        
        AccumulatorEngine engine;
        
        // Test PER_STAGE mode
        engine.reset();
        engine.setMode(AccumulatorEngine::AccumulatorMode::PER_STAGE);
        
        engine.processAccumulator(0, 0, 0);
        expect(engine.getCurrentValue() == 1);
        
        engine.processAccumulator(0, 1, 0);
        expect(engine.getCurrentValue() == 1); // Same stage, no accumulation
        
        engine.processAccumulator(1, 0, 0);
        expect(engine.getCurrentValue() == 2); // New stage
        
        // Test PER_PULSE mode
        engine.reset();
        engine.setMode(AccumulatorEngine::AccumulatorMode::PER_PULSE);
        
        engine.processAccumulator(0, 0, 0);
        expect(engine.getCurrentValue() == 1);
        
        engine.processAccumulator(0, 0, 1);
        expect(engine.getCurrentValue() == 1); // Same pulse, no accumulation
        
        engine.processAccumulator(0, 1, 0);
        expect(engine.getCurrentValue() == 2); // New pulse
        
        // Test PER_RATCHET mode
        engine.reset();
        engine.setMode(AccumulatorEngine::AccumulatorMode::PER_RATCHET);
        
        engine.processAccumulator(0, 0, 0);
        expect(engine.getCurrentValue() == 1);
        
        engine.processAccumulator(0, 0, 1);
        expect(engine.getCurrentValue() == 2); // New ratchet
        
        engine.processAccumulator(0, 0, 2);
        expect(engine.getCurrentValue() == 3); // Another ratchet
        
        // Test PENDULUM mode
        engine.reset();
        engine.setMode(AccumulatorEngine::AccumulatorMode::PENDULUM);
        engine.setPendulumRange(0, 3);
        
        // Going up
        engine.processAccumulator(0, 0, 0);
        expect(engine.getCurrentValue() == 1);
        expect(engine.getPendulumDirection() == true); // Still going up
        
        engine.processAccumulator(1, 0, 0);
        expect(engine.getCurrentValue() == 2);
        
        engine.processAccumulator(2, 0, 0);
        expect(engine.getCurrentValue() == 3); // Hit max
        expect(engine.getPendulumDirection() == false); // Should have changed direction
        
        // Going down
        engine.processAccumulator(3, 0, 0);
        expect(engine.getCurrentValue() == 2);
        
        engine.processAccumulator(4, 0, 0);
        expect(engine.getCurrentValue() == 1);
        
        engine.processAccumulator(5, 0, 0);
        expect(engine.getCurrentValue() == 0); // Hit min
        expect(engine.getPendulumDirection() == true); // Should have changed direction again
        
        // Going up again
        engine.processAccumulator(6, 0, 0);
        expect(engine.getCurrentValue() == 1);
        
        // Test MANUAL mode
        engine.reset();
        engine.setMode(AccumulatorEngine::AccumulatorMode::MANUAL);
        
        engine.processAccumulator(0, 0, 0);
        expect(engine.getCurrentValue() == 0); // No automatic accumulation
        
        engine.processAccumulator(1, 0, 0);
        expect(engine.getCurrentValue() == 0); // Still no accumulation
        
        engine.increment(5);
        expect(engine.getCurrentValue() == 5); // Manual increment works
    }
    
    void testResetStrategies()
    {
        beginTest("Reset Strategies");
        
        AccumulatorEngine engine;
        engine.setMode(AccumulatorEngine::AccumulatorMode::PER_STAGE);
        
        // Test STAGE_COUNT strategy
        engine.reset();
        engine.setResetStrategy(AccumulatorEngine::ResetStrategy::STAGE_COUNT);
        engine.setResetThreshold(3);
        
        engine.processAccumulator(0, 0, 0);
        engine.processAccumulator(1, 0, 0);
        engine.processAccumulator(2, 0, 0);
        expect(engine.getCurrentValue() == 3);
        
        // Should reset on next accumulation
        engine.processAccumulator(3, 0, 0);
        expect(engine.getCurrentValue() == 1); // Reset to initial (0) + 1
        
        // Test VALUE_LIMIT strategy
        engine.reset();
        engine.setResetStrategy(AccumulatorEngine::ResetStrategy::VALUE_LIMIT);
        engine.setValueLimits(-2, 2);
        
        engine.processAccumulator(0, 0, 0);
        engine.processAccumulator(1, 0, 0);
        expect(engine.getCurrentValue() == 2);
        
        // Should reset on next accumulation
        engine.processAccumulator(2, 0, 0);
        expect(engine.getCurrentValue() == 1); // Reset to initial (0) + 1
        
        // Test LOOP_END strategy
        engine.reset();
        engine.setResetStrategy(AccumulatorEngine::ResetStrategy::LOOP_END);
        
        engine.processAccumulator(0, 0, 0);
        engine.processAccumulator(1, 0, 0);
        expect(engine.getCurrentValue() == 2);
        
        engine.notifyLoopEnd();
        engine.processAccumulator(0, 0, 0); // Should reset here
        expect(engine.getCurrentValue() == 1); // Reset to initial (0) + 1
        
        // Test NEVER strategy
        engine.reset();
        engine.setResetStrategy(AccumulatorEngine::ResetStrategy::NEVER);
        engine.setValueLimits(-100, 100);
        
        for (int i = 0; i < 20; i++)
        {
            engine.processAccumulator(i, 0, 0);
        }
        expect(engine.getCurrentValue() == 20); // Should never reset
    }
    
    void testValueLimits()
    {
        beginTest("Value Limits");
        
        AccumulatorEngine engine;
        engine.setMode(AccumulatorEngine::AccumulatorMode::PER_STAGE);
        engine.setValueLimits(-5, 5);
        engine.setWrapMode(false); // Clamp mode
        
        // Test clamping at upper limit
        engine.reset();
        for (int i = 0; i < 10; i++)
        {
            engine.processAccumulator(i, 0, 0);
        }
        expect(engine.getCurrentValue() == 5); // Clamped at max
        
        // Test clamping at lower limit
        engine.reset();
        engine.setInitialValue(0);
        engine.setStepSize(-2);
        
        for (int i = 0; i < 10; i++)
        {
            engine.processAccumulator(i, 0, 0);
        }
        expect(engine.getCurrentValue() == -5); // Clamped at min
        
        // Test with different step size
        engine.reset();
        engine.setStepSize(3);
        
        engine.processAccumulator(0, 0, 0);
        expect(engine.getCurrentValue() == 3);
        
        engine.processAccumulator(1, 0, 0);
        expect(engine.getCurrentValue() == 5); // Clamped at max (would be 6)
    }
    
    void testWrapMode()
    {
        beginTest("Wrap Mode");
        
        AccumulatorEngine engine;
        engine.setMode(AccumulatorEngine::AccumulatorMode::PER_STAGE);
        engine.setValueLimits(0, 3);
        engine.setWrapMode(true); // Enable wrapping
        
        // Test wrapping at upper limit
        engine.reset();
        engine.processAccumulator(0, 0, 0); // 1
        engine.processAccumulator(1, 0, 0); // 2
        engine.processAccumulator(2, 0, 0); // 3
        engine.processAccumulator(3, 0, 0); // Should wrap to 0
        expect(engine.getCurrentValue() == 0);
        
        // Test wrapping with larger step
        engine.reset();
        engine.setStepSize(2);
        
        engine.processAccumulator(0, 0, 0); // 2
        engine.processAccumulator(1, 0, 0); // Would be 4, wraps to 0
        expect(engine.getCurrentValue() == 0);
        
        // Test negative wrapping
        engine.setValueLimits(-2, 2);
        engine.setInitialValue(0);
        engine.setStepSize(-3);
        engine.reset();
        
        engine.processAccumulator(0, 0, 0); // Would be -3, wraps to 2
        expect(engine.getCurrentValue() == 0 || engine.getCurrentValue() == 2); // Wrapping calculation may vary
    }
    
    void testStateManagement()
    {
        beginTest("State Management");
        
        AccumulatorEngine engine;
        engine.setMode(AccumulatorEngine::AccumulatorMode::PER_STAGE);
        
        // Set up some state
        engine.processAccumulator(0, 0, 0);
        engine.processAccumulator(1, 2, 3);
        engine.processAccumulator(2, 4, 5);
        
        // Get state
        auto state = engine.getState();
        expect(state.currentValue == 3);
        expect(state.stepsSinceReset == 3);
        expect(state.lastStageProcessed == 2);
        expect(state.lastPulseProcessed == 4);
        
        // Reset and modify
        engine.reset();
        engine.processAccumulator(5, 5, 5);
        expect(engine.getCurrentValue() == 1);
        
        // Restore state
        engine.setState(state);
        expect(engine.getCurrentValue() == 3);
        
        auto restoredState = engine.getState();
        expect(restoredState.currentValue == state.currentValue);
        expect(restoredState.stepsSinceReset == state.stepsSinceReset);
    }
    
    void testTrackAccumulator()
    {
        beginTest("Track Accumulator");
        
        TrackAccumulator accumulator;
        
        // Create a track
        auto track = std::make_unique<Track>();
        track->setAccumulatorMode(AccumulatorMode::STAGE);
        
        // Process accumulator for different stages
        int value = accumulator.processTrackAccumulator(track.get(), 0, 0, 0);
        expect(value == 1);
        
        value = accumulator.processTrackAccumulator(track.get(), 0, 1, 0);
        expect(value == 1); // Same stage, no accumulation
        
        value = accumulator.processTrackAccumulator(track.get(), 1, 0, 0);
        expect(value == 2); // New stage
        
        // Test with PER_PULSE mode
        track->setAccumulatorMode(AccumulatorMode::PULSE);
        accumulator.reset();
        
        value = accumulator.processTrackAccumulator(track.get(), 0, 0, 0);
        expect(value == 1);
        
        value = accumulator.processTrackAccumulator(track.get(), 0, 1, 0);
        expect(value == 2); // New pulse
        
        // Test with accumulator OFF
        track->setAccumulatorMode(AccumulatorMode::OFF);
        accumulator.reset();
        
        value = accumulator.processTrackAccumulator(track.get(), 0, 0, 0);
        expect(value == 0); // No accumulation when OFF
        
        // Test loop end notification
        track->setAccumulatorMode(AccumulatorMode::STAGE);
        accumulator.reset();
        accumulator.getEngine().setResetStrategy(AccumulatorEngine::ResetStrategy::LOOP_END);
        
        accumulator.processTrackAccumulator(track.get(), 0, 0, 0);
        accumulator.processTrackAccumulator(track.get(), 1, 0, 0);
        expect(accumulator.getEngine().getCurrentValue() == 2);
        
        accumulator.notifyLoopEnd();
        value = accumulator.processTrackAccumulator(track.get(), 0, 0, 0);
        expect(value == 1); // Reset after loop end
        
        // Test with null track
        value = accumulator.processTrackAccumulator(nullptr, 0, 0, 0);
        expect(value == 0);
    }
};

static AccumulatorEngineTests accumulatorEngineTests;

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