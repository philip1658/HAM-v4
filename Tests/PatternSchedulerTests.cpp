/*
  ==============================================================================

    PatternSchedulerTests.cpp
    Comprehensive unit tests for PatternScheduler component
    Coverage target: >80% line coverage

  ==============================================================================
*/

#include <JuceHeader.h>
#include "../Source/Domain/Processors/PatternScheduler.h"
#include "../Source/Domain/Models/Pattern.h"

using namespace HAM;

//==============================================================================
class PatternSchedulerTests : public juce::UnitTest
{
public:
    PatternSchedulerTests() : UnitTest("PatternScheduler Tests", "Processors") {}
    
    void runTest() override
    {
        testConstruction();
        testPatternQueueing();
        testTransitionTiming();
        testQueueManagement();
        testSchedulingModes();
        testPatternValidation();
        testCallbacks();
        testEdgeCases();
        testThreadSafety();
    }
    
private:
    void testConstruction()
    {
        beginTest("Construction and Initial State");
        
        PatternScheduler scheduler;
        
        // Test initial state
        expectEquals(scheduler.getCurrentPatternIndex(), 0, "Should start with pattern 0");
        expect(!scheduler.hasQueuedPattern(), "Should have no queued pattern initially");
        expectEquals(scheduler.getQueuedPatternIndex(), -1, "Queued index should be -1");
        expectEquals(scheduler.getTransitionMode(), 
                    PatternScheduler::TransitionMode::NextBar,
                    "Default transition should be NextBar");
    }
    
    void testPatternQueueing()
    {
        beginTest("Pattern Queueing");
        
        PatternScheduler scheduler;
        
        // Queue a pattern
        scheduler.queuePattern(5);
        expect(scheduler.hasQueuedPattern(), "Should have queued pattern");
        expectEquals(scheduler.getQueuedPatternIndex(), 5, "Queued index should be 5");
        
        // Queue another pattern (should replace)
        scheduler.queuePattern(3);
        expectEquals(scheduler.getQueuedPatternIndex(), 3, "Queued index should be updated to 3");
        
        // Clear queue
        scheduler.clearQueue();
        expect(!scheduler.hasQueuedPattern(), "Queue should be cleared");
        expectEquals(scheduler.getQueuedPatternIndex(), -1, "Queued index should be -1");
        
        // Queue same pattern as current
        scheduler.setCurrentPattern(2);
        scheduler.queuePattern(2);
        expect(scheduler.hasQueuedPattern(), "Should allow queueing current pattern");
        
        // Test immediate transition
        scheduler.queuePattern(7, PatternScheduler::TransitionMode::Immediate);
        scheduler.processTransition(0, 0); // Any position
        expectEquals(scheduler.getCurrentPatternIndex(), 7, "Should transition immediately");
        expect(!scheduler.hasQueuedPattern(), "Queue should be cleared after transition");
    }
    
    void testTransitionTiming()
    {
        beginTest("Transition Timing");
        
        PatternScheduler scheduler;
        
        // Test NextBeat transition
        scheduler.setTransitionMode(PatternScheduler::TransitionMode::NextBeat);
        scheduler.queuePattern(1);
        
        // Not on beat boundary
        scheduler.processTransition(5, 0); // Pulse 5 of beat 0
        expectEquals(scheduler.getCurrentPatternIndex(), 0, "Should not transition mid-beat");
        
        // On beat boundary
        scheduler.processTransition(0, 1); // Pulse 0 of beat 1
        expectEquals(scheduler.getCurrentPatternIndex(), 1, "Should transition on beat");
        
        // Test NextBar transition
        scheduler.setCurrentPattern(0);
        scheduler.setTransitionMode(PatternScheduler::TransitionMode::NextBar);
        scheduler.queuePattern(2);
        
        // Not on bar boundary
        scheduler.processTransition(0, 1); // Beat 1
        expectEquals(scheduler.getCurrentPatternIndex(), 0, "Should not transition mid-bar");
        
        // On bar boundary
        scheduler.processTransition(0, 0); // Beat 0 (new bar)
        expectEquals(scheduler.getCurrentPatternIndex(), 2, "Should transition on bar");
        
        // Test Next2Bars transition
        scheduler.setCurrentPattern(0);
        scheduler.setTransitionMode(PatternScheduler::TransitionMode::Next2Bars);
        scheduler.queuePattern(3);
        
        int barCount = 0;
        for (int bar = 0; bar < 3; ++bar)
        {
            for (int beat = 0; beat < 4; ++beat)
            {
                scheduler.processTransition(0, beat);
                if (scheduler.getCurrentPatternIndex() == 3)
                {
                    barCount = bar;
                    break;
                }
            }
            if (scheduler.getCurrentPatternIndex() == 3) break;
        }
        
        expectGreaterOrEqual(barCount, 1, "Should transition after at least 2 bars");
        
        // Test Next4Bars transition
        scheduler.setCurrentPattern(0);
        scheduler.setTransitionMode(PatternScheduler::TransitionMode::Next4Bars);
        scheduler.queuePattern(4);
        
        for (int i = 0; i < 5 * 4; ++i) // 5 bars worth of beats
        {
            scheduler.processTransition(0, i % 4);
        }
        
        expectEquals(scheduler.getCurrentPatternIndex(), 4, "Should transition after 4 bars");
    }
    
    void testQueueManagement()
    {
        beginTest("Queue Management");
        
        PatternScheduler scheduler;
        
        // Test queue priority
        scheduler.queuePattern(1);
        scheduler.queuePattern(2, PatternScheduler::TransitionMode::Immediate);
        scheduler.processTransition(0, 0);
        expectEquals(scheduler.getCurrentPatternIndex(), 2, "Immediate should take priority");
        
        // Test multiple queues
        scheduler.queuePattern(3);
        scheduler.queuePattern(4);
        scheduler.queuePattern(5);
        expectEquals(scheduler.getQueuedPatternIndex(), 5, "Latest queue should override");
        
        // Test cancel transition
        scheduler.queuePattern(6);
        scheduler.cancelTransition();
        expect(!scheduler.hasQueuedPattern(), "Transition should be cancelled");
        
        // Test queue validation
        scheduler.queuePattern(-1); // Invalid index
        expect(!scheduler.hasQueuedPattern() || scheduler.getQueuedPatternIndex() >= 0,
               "Should handle invalid pattern index");
        
        scheduler.queuePattern(1000); // Very large index
        expect(scheduler.hasQueuedPattern(), "Should accept large indices");
    }
    
    void testSchedulingModes()
    {
        beginTest("Scheduling Modes");
        
        PatternScheduler scheduler;
        
        // Test all transition modes
        const auto modes = {
            PatternScheduler::TransitionMode::Immediate,
            PatternScheduler::TransitionMode::NextPulse,
            PatternScheduler::TransitionMode::NextBeat,
            PatternScheduler::TransitionMode::NextBar,
            PatternScheduler::TransitionMode::Next2Bars,
            PatternScheduler::TransitionMode::Next4Bars,
            PatternScheduler::TransitionMode::Next8Bars,
            PatternScheduler::TransitionMode::Next16Bars
        };
        
        for (auto mode : modes)
        {
            scheduler.setTransitionMode(mode);
            expectEquals(scheduler.getTransitionMode(), mode, "Mode should be set");
            
            scheduler.setCurrentPattern(0);
            scheduler.queuePattern(10);
            
            // Process enough transitions to trigger
            for (int i = 0; i < 100; ++i)
            {
                scheduler.processTransition(i % 24, i / 24);
                if (scheduler.getCurrentPatternIndex() == 10)
                    break;
            }
            
            if (mode == PatternScheduler::TransitionMode::Immediate)
            {
                expectEquals(scheduler.getCurrentPatternIndex(), 10,
                           "Should have transitioned with mode");
            }
        }
        
        // Test default mode
        scheduler.setDefaultTransitionMode(PatternScheduler::TransitionMode::NextBeat);
        expectEquals(scheduler.getDefaultTransitionMode(),
                    PatternScheduler::TransitionMode::NextBeat,
                    "Default mode should be set");
    }
    
    void testPatternValidation()
    {
        beginTest("Pattern Validation");
        
        PatternScheduler scheduler;
        
        // Set up pattern validator
        scheduler.setPatternValidator([](int index) {
            return index >= 0 && index < 8; // Only allow 0-7
        });
        
        // Test valid pattern
        scheduler.queuePattern(5);
        expect(scheduler.hasQueuedPattern(), "Valid pattern should be queued");
        
        // Test invalid pattern
        scheduler.queuePattern(10);
        expect(!scheduler.hasQueuedPattern() || scheduler.getQueuedPatternIndex() < 8,
               "Invalid pattern should be rejected or clamped");
        
        // Remove validator
        scheduler.setPatternValidator(nullptr);
        scheduler.queuePattern(10);
        expect(scheduler.hasQueuedPattern(), "Should accept any pattern without validator");
    }
    
    void testCallbacks()
    {
        beginTest("Callbacks");
        
        PatternScheduler scheduler;
        
        int transitionCount = 0;
        int lastOldPattern = -1;
        int lastNewPattern = -1;
        
        // Set transition callback
        scheduler.setTransitionCallback([&](int oldPattern, int newPattern) {
            transitionCount++;
            lastOldPattern = oldPattern;
            lastNewPattern = newPattern;
        });
        
        // Trigger transition
        scheduler.setCurrentPattern(0);
        scheduler.queuePattern(5, PatternScheduler::TransitionMode::Immediate);
        scheduler.processTransition(0, 0);
        
        expectEquals(transitionCount, 1, "Callback should be called once");
        expectEquals(lastOldPattern, 0, "Should pass old pattern");
        expectEquals(lastNewPattern, 5, "Should pass new pattern");
        
        // Test queue callback
        int queueCount = 0;
        int lastQueuedPattern = -1;
        
        scheduler.setQueueCallback([&](int pattern) {
            queueCount++;
            lastQueuedPattern = pattern;
        });
        
        scheduler.queuePattern(7);
        expectEquals(queueCount, 1, "Queue callback should be called");
        expectEquals(lastQueuedPattern, 7, "Should pass queued pattern");
        
        // Test with null callbacks
        scheduler.setTransitionCallback(nullptr);
        scheduler.setQueueCallback(nullptr);
        
        scheduler.queuePattern(8, PatternScheduler::TransitionMode::Immediate);
        scheduler.processTransition(0, 0);
        // Should not crash with null callbacks
    }
    
    void testEdgeCases()
    {
        beginTest("Edge Cases");
        
        PatternScheduler scheduler;
        
        // Test rapid queueing
        for (int i = 0; i < 100; ++i)
        {
            scheduler.queuePattern(i);
        }
        expectEquals(scheduler.getQueuedPatternIndex(), 99, "Should handle rapid queueing");
        
        // Test transition during transition
        scheduler.setCurrentPattern(0);
        scheduler.queuePattern(1, PatternScheduler::TransitionMode::Immediate);
        scheduler.queuePattern(2, PatternScheduler::TransitionMode::Immediate);
        scheduler.processTransition(0, 0);
        expectEquals(scheduler.getCurrentPatternIndex(), 2, "Should handle overlapping transitions");
        
        // Test negative pulse/beat values
        scheduler.processTransition(-1, -1);
        // Should handle gracefully
        
        // Test very large pulse/beat values
        scheduler.processTransition(10000, 10000);
        // Should handle gracefully
        
        // Test clearing non-existent queue
        scheduler.clearQueue();
        scheduler.clearQueue(); // Double clear
        expect(!scheduler.hasQueuedPattern(), "Double clear should be safe");
        
        // Test canceling non-existent transition
        scheduler.cancelTransition();
        scheduler.cancelTransition(); // Double cancel
        // Should handle gracefully
        
        // Test pattern index boundaries
        scheduler.setCurrentPattern(-10);
        expectGreaterOrEqual(scheduler.getCurrentPatternIndex(), 0, "Should clamp negative index");
        
        scheduler.setCurrentPattern(INT_MAX);
        // Should handle large index
        
        // Test transition at exact boundaries
        scheduler.setTransitionMode(PatternScheduler::TransitionMode::NextBar);
        scheduler.setCurrentPattern(0);
        scheduler.queuePattern(5);
        
        // Exactly at bar boundary
        scheduler.processTransition(0, 0);
        expectEquals(scheduler.getCurrentPatternIndex(), 5, "Should transition at exact boundary");
    }
    
    void testThreadSafety()
    {
        beginTest("Thread Safety");
        
        PatternScheduler scheduler;
        std::atomic<bool> shouldStop{false};
        std::atomic<int> transitionCount{0};
        
        // Set callback to count transitions
        scheduler.setTransitionCallback([&transitionCount](int, int) {
            transitionCount++;
        });
        
        // Audio thread - processing transitions
        std::thread audioThread([&scheduler, &shouldStop]()
        {
            int pulse = 0;
            int beat = 0;
            
            while (!shouldStop.load())
            {
                scheduler.processTransition(pulse, beat);
                
                pulse++;
                if (pulse >= 24)
                {
                    pulse = 0;
                    beat++;
                    if (beat >= 4) beat = 0;
                }
                
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        });
        
        // Control thread - queueing patterns
        std::thread controlThread([&scheduler, &shouldStop]()
        {
            int pattern = 0;
            const auto modes = {
                PatternScheduler::TransitionMode::Immediate,
                PatternScheduler::TransitionMode::NextBeat,
                PatternScheduler::TransitionMode::NextBar
            };
            
            while (!shouldStop.load())
            {
                for (auto mode : modes)
                {
                    scheduler.queuePattern(pattern++, mode);
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    
                    if (pattern % 5 == 0)
                    {
                        scheduler.clearQueue();
                    }
                }
            }
        });
        
        // Query thread - reading state
        std::thread queryThread([&scheduler, &shouldStop]()
        {
            while (!shouldStop.load())
            {
                scheduler.getCurrentPatternIndex();
                scheduler.getQueuedPatternIndex();
                scheduler.hasQueuedPattern();
                scheduler.getTransitionMode();
                
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
        
        // Verify scheduler is still functional
        scheduler.setCurrentPattern(0);
        scheduler.queuePattern(100, PatternScheduler::TransitionMode::Immediate);
        scheduler.processTransition(0, 0);
        expectEquals(scheduler.getCurrentPatternIndex(), 100, "Scheduler should still be functional");
        
        expectGreaterThan(transitionCount.load(), 0, "Should have processed some transitions");
    }
};

static PatternSchedulerTests patternSchedulerTests;

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