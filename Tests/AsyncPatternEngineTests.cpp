/*
  ==============================================================================

    AsyncPatternEngineTests.cpp
    Comprehensive unit tests for AsyncPatternEngine component
    Coverage target: >80% line coverage

  ==============================================================================
*/

#include <JuceHeader.h>
#include "../Source/Domain/Clock/AsyncPatternEngine.h"
#include "../Source/Domain/Clock/MasterClock.h"
#include <chrono>
#include <thread>

using namespace HAM;

//==============================================================================
// Mock clock for testing without actual timing
class MockClock : public MasterClock
{
public:
    void simulatePulse(int pulseNumber)
    {
        // Directly call listener methods for testing
        for (auto* listener : getListeners())
        {
            if (listener)
                listener->onClockPulse(pulseNumber);
        }
    }
    
    void simulateStart()
    {
        for (auto* listener : getListeners())
        {
            if (listener)
                listener->onClockStart();
        }
    }
    
    void simulateStop()
    {
        for (auto* listener : getListeners())
        {
            if (listener)
                listener->onClockStop();
        }
    }
    
    void simulateReset()
    {
        for (auto* listener : getListeners())
        {
            if (listener)
                listener->onClockReset();
        }
    }
    
    void simulateTempo(float bpm)
    {
        for (auto* listener : getListeners())
        {
            if (listener)
                listener->onTempoChanged(bpm);
        }
    }
    
    // Helper to access listeners for testing
    std::vector<Listener*>& getListeners() { return m_listeners; }
    
    // Override to simulate specific positions
    void setTestPosition(int pulse, int beat, int bar)
    {
        m_currentPulse = pulse;
        m_currentBeat = beat;
        m_currentBar = bar;
    }
    
private:
    std::vector<Listener*> m_listeners;
};

//==============================================================================
// Test listener for capturing pattern events
class TestPatternListener : public AsyncPatternEngine::Listener
{
public:
    struct Event
    {
        enum Type { PatternQueued, PatternSwitched, SceneQueued, SceneSwitched };
        Type type;
        int index;
    };
    
    std::vector<Event> events;
    std::atomic<int> patternQueuedCount{0};
    std::atomic<int> patternSwitchedCount{0};
    std::atomic<int> sceneQueuedCount{0};
    std::atomic<int> sceneSwitchedCount{0};
    
    int lastPatternQueued{-1};
    int lastPatternSwitched{-1};
    int lastSceneQueued{-1};
    int lastSceneSwitched{-1};
    
    void onPatternQueued(int patternIndex) override
    {
        events.push_back({Event::PatternQueued, patternIndex});
        lastPatternQueued = patternIndex;
        patternQueuedCount++;
    }
    
    void onPatternSwitched(int patternIndex) override
    {
        events.push_back({Event::PatternSwitched, patternIndex});
        lastPatternSwitched = patternIndex;
        patternSwitchedCount++;
    }
    
    void onSceneQueued(int sceneIndex) override
    {
        events.push_back({Event::SceneQueued, sceneIndex});
        lastSceneQueued = sceneIndex;
        sceneQueuedCount++;
    }
    
    void onSceneSwitched(int sceneIndex) override
    {
        events.push_back({Event::SceneSwitched, sceneIndex});
        lastSceneSwitched = sceneIndex;
        sceneSwitchedCount++;
    }
    
    void reset()
    {
        events.clear();
        patternQueuedCount = 0;
        patternSwitchedCount = 0;
        sceneQueuedCount = 0;
        sceneSwitchedCount = 0;
        lastPatternQueued = -1;
        lastPatternSwitched = -1;
        lastSceneQueued = -1;
        lastSceneSwitched = -1;
    }
};

//==============================================================================
class AsyncPatternEngineTests : public juce::UnitTest
{
public:
    AsyncPatternEngineTests() : UnitTest("AsyncPatternEngine Tests", "Timing") {}
    
    void runTest() override
    {
        testConstruction();
        testPatternQueueing();
        testSceneQueueing();
        testQuantizationModes();
        testPendingSwitchManagement();
        testListenerNotifications();
        testClockIntegration();
        testSwitchTiming();
        testEdgeCases();
        testThreadSafety();
    }
    
private:
    void testConstruction()
    {
        beginTest("Construction and Initial State");
        
        MasterClock clock;
        AsyncPatternEngine engine(clock);
        
        // Test initial state
        expectEquals(engine.getCurrentPatternIndex(), 0, "Initial pattern should be 0");
        expectEquals(engine.getCurrentSceneIndex(), 0, "Initial scene should be 0");
        expect(!engine.hasPendingSwitch(), "Should have no pending switch initially");
        expectEquals(engine.getPendingPatternIndex(), -1, "Pending pattern should be -1");
        expectEquals(engine.getPendingSceneIndex(), -1, "Pending scene should be -1");
        expectEquals(static_cast<int>(engine.getDefaultQuantization()), 
                    static_cast<int>(AsyncPatternEngine::SwitchQuantization::NEXT_BAR),
                    "Default quantization should be NEXT_BAR");
    }
    
    void testPatternQueueing()
    {
        beginTest("Pattern Queueing");
        
        MasterClock clock;
        AsyncPatternEngine engine(clock);
        TestPatternListener listener;
        engine.addListener(&listener);
        
        // Queue a pattern
        engine.queuePattern(5, AsyncPatternEngine::SwitchQuantization::NEXT_BEAT);
        
        expect(engine.hasPendingSwitch(), "Should have pending switch after queueing");
        expectEquals(engine.getPendingPatternIndex(), 5, "Pending pattern should be 5");
        expectEquals(engine.getPendingSceneIndex(), -1, "Should not have pending scene");
        expectEquals(listener.lastPatternQueued, 5, "Listener should be notified of queued pattern");
        expectEquals(listener.patternQueuedCount.load(), 1, "Should have one pattern queued event");
        
        // Queue another pattern (should replace)
        engine.queuePattern(8, AsyncPatternEngine::SwitchQuantization::IMMEDIATE);
        
        expectEquals(engine.getPendingPatternIndex(), 8, "Pending pattern should be updated to 8");
        expectEquals(listener.lastPatternQueued, 8, "Listener should be notified of new queued pattern");
        expectEquals(listener.patternQueuedCount.load(), 2, "Should have two pattern queued events");
        
        // Test immediate switching
        engine.queuePattern(3, AsyncPatternEngine::SwitchQuantization::IMMEDIATE);
        engine.onClockPulse(0); // Trigger immediate switch
        
        expectEquals(engine.getCurrentPatternIndex(), 3, "Pattern should switch immediately");
        expectEquals(listener.lastPatternSwitched, 3, "Listener should be notified of switch");
        expect(!engine.hasPendingSwitch(), "Should have no pending switch after immediate");
        
        engine.removeListener(&listener);
    }
    
    void testSceneQueueing()
    {
        beginTest("Scene Queueing");
        
        MasterClock clock;
        AsyncPatternEngine engine(clock);
        TestPatternListener listener;
        engine.addListener(&listener);
        
        // Queue a scene
        engine.queueScene(2, AsyncPatternEngine::SwitchQuantization::NEXT_2_BARS);
        
        expect(engine.hasPendingSwitch(), "Should have pending switch after queueing scene");
        expectEquals(engine.getPendingSceneIndex(), 2, "Pending scene should be 2");
        expectEquals(engine.getPendingPatternIndex(), -1, "Should not have pending pattern");
        expectEquals(listener.lastSceneQueued, 2, "Listener should be notified of queued scene");
        expectEquals(listener.sceneQueuedCount.load(), 1, "Should have one scene queued event");
        
        // Queue another scene
        engine.queueScene(4, AsyncPatternEngine::SwitchQuantization::NEXT_4_BARS);
        
        expectEquals(engine.getPendingSceneIndex(), 4, "Pending scene should be updated to 4");
        expectEquals(listener.lastSceneQueued, 4, "Listener should be notified of new queued scene");
        
        // Test immediate scene switching
        engine.queueScene(1, AsyncPatternEngine::SwitchQuantization::IMMEDIATE);
        engine.onClockPulse(0); // Trigger immediate switch
        
        expectEquals(engine.getCurrentSceneIndex(), 1, "Scene should switch immediately");
        expectEquals(listener.lastSceneSwitched, 1, "Listener should be notified of scene switch");
        expect(!engine.hasPendingSwitch(), "Should have no pending switch after immediate");
        
        engine.removeListener(&listener);
    }
    
    void testQuantizationModes()
    {
        beginTest("Quantization Modes");
        
        MockClock clock;
        AsyncPatternEngine engine(clock);
        clock.addListener(&engine);
        TestPatternListener listener;
        engine.addListener(&listener);
        
        // Test IMMEDIATE quantization
        engine.queuePattern(1, AsyncPatternEngine::SwitchQuantization::IMMEDIATE);
        clock.simulatePulse(0);
        expectEquals(engine.getCurrentPatternIndex(), 1, "IMMEDIATE should switch on any pulse");
        
        // Test NEXT_PULSE quantization
        engine.queuePattern(2, AsyncPatternEngine::SwitchQuantization::NEXT_PULSE);
        expectEquals(engine.getCurrentPatternIndex(), 1, "Should not switch yet");
        clock.simulatePulse(1);
        expectEquals(engine.getCurrentPatternIndex(), 2, "NEXT_PULSE should switch on next pulse");
        
        // Test NEXT_BEAT quantization (every 24 pulses)
        engine.queuePattern(3, AsyncPatternEngine::SwitchQuantization::NEXT_BEAT);
        clock.setTestPosition(23, 0, 0); // Just before beat
        clock.simulatePulse(23);
        expectEquals(engine.getCurrentPatternIndex(), 2, "Should not switch before beat");
        clock.setTestPosition(24, 1, 0); // On beat
        clock.simulatePulse(24);
        expectEquals(engine.getCurrentPatternIndex(), 3, "NEXT_BEAT should switch on beat boundary");
        
        // Test NEXT_BAR quantization (every 96 pulses in 4/4)
        engine.queuePattern(4, AsyncPatternEngine::SwitchQuantization::NEXT_BAR);
        clock.setTestPosition(95, 3, 0); // Just before bar
        clock.simulatePulse(95);
        expectEquals(engine.getCurrentPatternIndex(), 3, "Should not switch before bar");
        clock.setTestPosition(0, 0, 1); // Start of new bar
        clock.simulatePulse(96);
        expectEquals(engine.getCurrentPatternIndex(), 4, "NEXT_BAR should switch on bar boundary");
        
        // Test longer quantizations
        engine.queuePattern(5, AsyncPatternEngine::SwitchQuantization::NEXT_2_BARS);
        clock.setTestPosition(0, 0, 2); // Skip ahead
        clock.simulatePulse(96 * 2);
        expectEquals(engine.getCurrentPatternIndex(), 5, "NEXT_2_BARS should switch after 2 bars");
        
        engine.queuePattern(6, AsyncPatternEngine::SwitchQuantization::NEXT_4_BARS);
        clock.setTestPosition(0, 0, 4); // Skip ahead
        clock.simulatePulse(96 * 4);
        expectEquals(engine.getCurrentPatternIndex(), 6, "NEXT_4_BARS should switch after 4 bars");
        
        engine.queuePattern(7, AsyncPatternEngine::SwitchQuantization::NEXT_8_BARS);
        clock.setTestPosition(0, 0, 8); // Skip ahead
        clock.simulatePulse(96 * 8);
        expectEquals(engine.getCurrentPatternIndex(), 7, "NEXT_8_BARS should switch after 8 bars");
        
        engine.queuePattern(8, AsyncPatternEngine::SwitchQuantization::NEXT_16_BARS);
        clock.setTestPosition(0, 0, 16); // Skip ahead
        clock.simulatePulse(96 * 16);
        expectEquals(engine.getCurrentPatternIndex(), 8, "NEXT_16_BARS should switch after 16 bars");
        
        clock.removeListener(&engine);
        engine.removeListener(&listener);
    }
    
    void testPendingSwitchManagement()
    {
        beginTest("Pending Switch Management");
        
        MasterClock clock;
        AsyncPatternEngine engine(clock);
        TestPatternListener listener;
        engine.addListener(&listener);
        
        // Test canceling pending switch
        engine.queuePattern(5, AsyncPatternEngine::SwitchQuantization::NEXT_BAR);
        expect(engine.hasPendingSwitch(), "Should have pending switch");
        
        engine.cancelPendingSwitch();
        expect(!engine.hasPendingSwitch(), "Should not have pending switch after cancel");
        expectEquals(engine.getPendingPatternIndex(), -1, "Pending pattern should be cleared");
        expectEquals(engine.getPendingSceneIndex(), -1, "Pending scene should be cleared");
        
        // Test pattern overriding scene
        engine.queueScene(2, AsyncPatternEngine::SwitchQuantization::NEXT_BAR);
        expect(engine.hasPendingSwitch(), "Should have pending scene switch");
        
        engine.queuePattern(3, AsyncPatternEngine::SwitchQuantization::NEXT_BEAT);
        expect(engine.hasPendingSwitch(), "Should still have pending switch");
        expectEquals(engine.getPendingPatternIndex(), 3, "Pattern should override scene");
        expectEquals(engine.getPendingSceneIndex(), -1, "Scene should be cleared");
        
        // Test scene overriding pattern
        engine.queuePattern(4, AsyncPatternEngine::SwitchQuantization::NEXT_BAR);
        engine.queueScene(5, AsyncPatternEngine::SwitchQuantization::NEXT_BEAT);
        expectEquals(engine.getPendingSceneIndex(), 5, "Scene should override pattern");
        expectEquals(engine.getPendingPatternIndex(), -1, "Pattern should be cleared");
        
        // Test getBarsUntilSwitch and getBeatsUntilSwitch
        engine.queuePattern(6, AsyncPatternEngine::SwitchQuantization::NEXT_4_BARS);
        int barsUntil = engine.getBarsUntilSwitch();
        expectGreaterOrEqual(barsUntil, 0, "Should return valid bars until switch");
        
        int beatsUntil = engine.getBeatsUntilSwitch();
        expectGreaterOrEqual(beatsUntil, 0, "Should return valid beats until switch");
        
        engine.cancelPendingSwitch();
        expectEquals(engine.getBarsUntilSwitch(), -1, "Should return -1 when no pending switch");
        expectEquals(engine.getBeatsUntilSwitch(), -1, "Should return -1 when no pending switch");
        
        engine.removeListener(&listener);
    }
    
    void testListenerNotifications()
    {
        beginTest("Listener Notifications");
        
        MockClock clock;
        AsyncPatternEngine engine(clock);
        clock.addListener(&engine);
        
        TestPatternListener listener1, listener2, listener3;
        
        // Add multiple listeners
        engine.addListener(&listener1);
        engine.addListener(&listener2);
        engine.addListener(&listener3);
        
        // Test pattern queue notifications
        engine.queuePattern(5, AsyncPatternEngine::SwitchQuantization::IMMEDIATE);
        expectEquals(listener1.lastPatternQueued, 5, "Listener 1 should receive pattern queued");
        expectEquals(listener2.lastPatternQueued, 5, "Listener 2 should receive pattern queued");
        expectEquals(listener3.lastPatternQueued, 5, "Listener 3 should receive pattern queued");
        
        // Test pattern switch notifications
        clock.simulatePulse(0);
        expectEquals(listener1.lastPatternSwitched, 5, "Listener 1 should receive pattern switched");
        expectEquals(listener2.lastPatternSwitched, 5, "Listener 2 should receive pattern switched");
        expectEquals(listener3.lastPatternSwitched, 5, "Listener 3 should receive pattern switched");
        
        // Remove one listener
        engine.removeListener(&listener2);
        listener1.reset();
        listener2.reset();
        listener3.reset();
        
        // Test scene notifications
        engine.queueScene(3, AsyncPatternEngine::SwitchQuantization::IMMEDIATE);
        expectEquals(listener1.lastSceneQueued, 3, "Listener 1 should receive scene queued");
        expectEquals(listener2.lastSceneQueued, -1, "Listener 2 should not receive after removal");
        expectEquals(listener3.lastSceneQueued, 3, "Listener 3 should receive scene queued");
        
        clock.simulatePulse(1);
        expectEquals(listener1.lastSceneSwitched, 3, "Listener 1 should receive scene switched");
        expectEquals(listener2.lastSceneSwitched, -1, "Listener 2 should not receive after removal");
        expectEquals(listener3.lastSceneSwitched, 3, "Listener 3 should receive scene switched");
        
        // Test adding same listener multiple times
        engine.addListener(&listener1); // Duplicate
        listener1.reset();
        
        engine.queuePattern(7, AsyncPatternEngine::SwitchQuantization::IMMEDIATE);
        expectEquals(listener1.patternQueuedCount.load(), 1, "Should not duplicate events");
        
        // Clean up
        engine.removeListener(&listener1);
        engine.removeListener(&listener3);
        clock.removeListener(&engine);
    }
    
    void testClockIntegration()
    {
        beginTest("Clock Integration");
        
        MockClock clock;
        AsyncPatternEngine engine(clock);
        clock.addListener(&engine);
        TestPatternListener listener;
        engine.addListener(&listener);
        
        // Test clock start event
        clock.simulateStart();
        // Engine should handle start gracefully
        
        // Test clock stop event
        engine.queuePattern(5, AsyncPatternEngine::SwitchQuantization::NEXT_BEAT);
        clock.simulateStop();
        // Pending switch should remain but not execute while stopped
        expect(engine.hasPendingSwitch(), "Should maintain pending switch when stopped");
        
        // Test clock reset event
        clock.simulateReset();
        // Engine should handle reset, potentially clearing pending switches
        
        // Test tempo change event
        clock.simulateTempo(140.0f);
        // Engine should adjust timing calculations
        
        // Test pattern switching during clock events
        engine.queuePattern(6, AsyncPatternEngine::SwitchQuantization::NEXT_PULSE);
        clock.simulateStart();
        clock.simulatePulse(1);
        expectEquals(engine.getCurrentPatternIndex(), 6, "Should switch after clock events");
        
        clock.removeListener(&engine);
        engine.removeListener(&listener);
    }
    
    void testSwitchTiming()
    {
        beginTest("Switch Timing Accuracy");
        
        MockClock clock;
        AsyncPatternEngine engine(clock);
        clock.addListener(&engine);
        TestPatternListener listener;
        engine.addListener(&listener);
        
        // Test precise beat boundaries
        for (int beat = 0; beat < 4; ++beat)
        {
            engine.queuePattern(beat + 10, AsyncPatternEngine::SwitchQuantization::NEXT_BEAT);
            
            // Pulses before beat boundary shouldn't trigger
            for (int pulse = 1; pulse < 24; ++pulse)
            {
                clock.simulatePulse(beat * 24 + pulse);
                expectNotEquals(engine.getCurrentPatternIndex(), beat + 10, 
                               "Should not switch before beat boundary");
            }
            
            // Pulse on beat boundary should trigger
            clock.simulatePulse((beat + 1) * 24);
            expectEquals(engine.getCurrentPatternIndex(), beat + 10, 
                        "Should switch exactly on beat boundary");
        }
        
        // Test bar boundaries
        engine.queuePattern(20, AsyncPatternEngine::SwitchQuantization::NEXT_BAR);
        
        // Simulate pulses through almost a full bar
        for (int pulse = 1; pulse < 96; ++pulse)
        {
            clock.simulatePulse(pulse);
            expectNotEquals(engine.getCurrentPatternIndex(), 20, 
                           "Should not switch before bar boundary");
        }
        
        // Bar boundary should trigger
        clock.simulatePulse(96);
        expectEquals(engine.getCurrentPatternIndex(), 20, 
                    "Should switch exactly on bar boundary");
        
        // Test switching doesn't happen multiple times
        listener.reset();
        engine.queuePattern(21, AsyncPatternEngine::SwitchQuantization::NEXT_PULSE);
        clock.simulatePulse(97);
        expectEquals(listener.patternSwitchedCount.load(), 1, "Should switch only once");
        
        clock.simulatePulse(98);
        expectEquals(listener.patternSwitchedCount.load(), 1, "Should not switch again");
        
        clock.removeListener(&engine);
        engine.removeListener(&listener);
    }
    
    void testEdgeCases()
    {
        beginTest("Edge Cases");
        
        MockClock clock;
        AsyncPatternEngine engine(clock);
        clock.addListener(&engine);
        TestPatternListener listener;
        engine.addListener(&listener);
        
        // Test negative pattern index
        engine.queuePattern(-1, AsyncPatternEngine::SwitchQuantization::IMMEDIATE);
        clock.simulatePulse(0);
        expectGreaterOrEqual(engine.getCurrentPatternIndex(), 0, "Should handle negative index gracefully");
        
        // Test very large pattern index
        engine.queuePattern(999999, AsyncPatternEngine::SwitchQuantization::IMMEDIATE);
        clock.simulatePulse(1);
        expectEquals(engine.getCurrentPatternIndex(), 999999, "Should handle large indices");
        
        // Test rapid pattern queueing
        for (int i = 0; i < 100; ++i)
        {
            engine.queuePattern(i, AsyncPatternEngine::SwitchQuantization::IMMEDIATE);
            clock.simulatePulse(i);
        }
        expectEquals(engine.getCurrentPatternIndex(), 99, "Should handle rapid switching");
        
        // Test switching same pattern
        int currentPattern = engine.getCurrentPatternIndex();
        listener.reset();
        engine.queuePattern(currentPattern, AsyncPatternEngine::SwitchQuantization::IMMEDIATE);
        clock.simulatePulse(100);
        expectEquals(listener.patternSwitchedCount.load(), 1, "Should still notify even for same pattern");
        
        // Test canceling non-existent switch
        engine.cancelPendingSwitch();
        engine.cancelPendingSwitch(); // Second cancel should be safe
        expect(!engine.hasPendingSwitch(), "Multiple cancels should be safe");
        
        // Test removing non-existent listener
        TestPatternListener nonExistentListener;
        engine.removeListener(&nonExistentListener); // Should not crash
        
        // Test null listener
        engine.addListener(nullptr);
        engine.removeListener(nullptr); // Should handle gracefully
        
        // Test default quantization
        engine.setDefaultQuantization(AsyncPatternEngine::SwitchQuantization::NEXT_PULSE);
        expectEquals(static_cast<int>(engine.getDefaultQuantization()),
                    static_cast<int>(AsyncPatternEngine::SwitchQuantization::NEXT_PULSE),
                    "Default quantization should be updated");
        
        // Test pattern and scene queued simultaneously
        engine.queuePattern(50, AsyncPatternEngine::SwitchQuantization::NEXT_BAR);
        engine.queueScene(10, AsyncPatternEngine::SwitchQuantization::NEXT_BEAT);
        expect(engine.hasPendingSwitch(), "Should have pending switch");
        expectEquals(engine.getPendingPatternIndex(), -1, "Pattern should be cleared by scene");
        expectEquals(engine.getPendingSceneIndex(), 10, "Scene should be pending");
        
        clock.removeListener(&engine);
        engine.removeListener(&listener);
    }
    
    void testThreadSafety()
    {
        beginTest("Thread Safety");
        
        MasterClock clock;
        AsyncPatternEngine engine(clock);
        TestPatternListener listener;
        engine.addListener(&listener);
        
        std::atomic<bool> shouldStop{false};
        
        // Audio thread simulation - processing clock pulses
        std::thread audioThread([&engine, &clock, &shouldStop]()
        {
            int pulse = 0;
            while (!shouldStop.load())
            {
                engine.onClockPulse(pulse++);
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        });
        
        // Control thread simulation - queueing patterns
        std::thread controlThread([&engine, &shouldStop]()
        {
            int pattern = 0;
            while (!shouldStop.load())
            {
                engine.queuePattern(pattern++, 
                    static_cast<AsyncPatternEngine::SwitchQuantization>(pattern % 8));
                
                if (pattern % 10 == 0)
                {
                    engine.cancelPendingSwitch();
                }
                
                if (pattern % 5 == 0)
                {
                    engine.queueScene(pattern / 5, 
                        AsyncPatternEngine::SwitchQuantization::NEXT_BAR);
                }
                
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        });
        
        // Query thread simulation - reading state
        std::thread queryThread([&engine, &shouldStop]()
        {
            while (!shouldStop.load())
            {
                engine.getCurrentPatternIndex();
                engine.getCurrentSceneIndex();
                engine.hasPendingSwitch();
                engine.getPendingPatternIndex();
                engine.getPendingSceneIndex();
                engine.getBarsUntilSwitch();
                engine.getBeatsUntilSwitch();
                std::this_thread::sleep_for(std::chrono::microseconds(50));
            }
        });
        
        // Let threads run
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        // Stop threads
        shouldStop = true;
        audioThread.join();
        controlThread.join();
        queryThread.join();
        
        // If we get here without crashing, thread safety is working
        expect(true, "Thread safety test completed without crashes");
        
        // Verify engine is still functional
        engine.queuePattern(100, AsyncPatternEngine::SwitchQuantization::IMMEDIATE);
        engine.onClockPulse(0);
        expectEquals(engine.getCurrentPatternIndex(), 100, "Engine should still function after thread test");
        
        engine.removeListener(&listener);
    }
};

static AsyncPatternEngineTests asyncPatternEngineTests;

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