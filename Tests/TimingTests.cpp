/*
  ==============================================================================

    TimingTests.cpp
    Unit tests for MasterClock and AsyncPatternEngine

  ==============================================================================
*/

#include <JuceHeader.h>
#include "../Source/Domain/Clock/MasterClock.h"
#include "../Source/Domain/Clock/AsyncPatternEngine.h"
#include <chrono>
#include <vector>

using namespace HAM;
using namespace std::chrono;

//==============================================================================
class TestClockListener : public MasterClock::Listener
{
public:
    std::vector<int> pulses;
    std::vector<float> bpmChanges;
    bool started = false;
    bool stopped = false;
    bool reset = false;
    
    void onClockPulse(int pulseNumber) override
    {
        pulses.push_back(pulseNumber);
    }
    
    void onClockStart() override { started = true; }
    void onClockStop() override { stopped = true; }
    void onClockReset() override { reset = true; }
    void onTempoChanged(float newBPM) override { bpmChanges.push_back(newBPM); }
    
    void clear()
    {
        pulses.clear();
        bpmChanges.clear();
        started = stopped = reset = false;
    }
};

//==============================================================================
class MasterClockTests : public juce::UnitTest
{
public:
    MasterClockTests() : UnitTest("MasterClock Tests") {}
    
    void runTest() override
    {
        beginTest("Clock Default State");
        {
            MasterClock clock;
            expect(!clock.isRunning());
            expectWithinAbsoluteError(clock.getBPM(), 120.0f, 0.001f);
            expect(clock.getCurrentPulse() == 0);
            expect(clock.getCurrentBeat() == 0);
            expect(clock.getCurrentBar() == 0);
        }
        
        beginTest("Clock Start/Stop");
        {
            MasterClock clock;
            TestClockListener listener;
            clock.addListener(&listener);
            
            clock.start();
            expect(clock.isRunning());
            juce::Thread::sleep(10);  // Wait for async callback
            expect(listener.started);
            
            clock.stop();
            expect(!clock.isRunning());
            juce::Thread::sleep(10);
            expect(listener.stopped);
            
            clock.removeListener(&listener);
        }
        
        beginTest("BPM Setting");
        {
            MasterClock clock;
            
            clock.setBPM(140.0f);
            expectWithinAbsoluteError(clock.getBPM(), 140.0f, 0.001f);
            
            // Test clamping
            clock.setBPM(10.0f);
            expectWithinAbsoluteError(clock.getBPM(), 20.0f, 0.001f);
            
            clock.setBPM(1000.0f);
            expectWithinAbsoluteError(clock.getBPM(), 999.0f, 0.001f);
        }
        
        beginTest("Sample-Accurate Processing");
        {
            MasterClock clock;
            TestClockListener listener;
            clock.addListener(&listener);
            
            clock.setBPM(120.0f);  // 120 BPM = 0.5 seconds per beat
            clock.start();
            
            double sampleRate = 48000.0;
            
            // At 120 BPM:
            // - 0.5 seconds per beat
            // - 0.5/24 = 0.0208333 seconds per pulse
            // - 0.0208333 * 48000 = 1000 samples per pulse
            
            // Process exactly 1000 samples - should generate 1 pulse
            listener.clear();
            clock.processBlock(sampleRate, 1000);
            
            // Small delay for async notification
            juce::Thread::sleep(10);
            
            // We should have advanced by 1 pulse
            expect(clock.getCurrentPulse() == 1);
            
            // Process another 2000 samples - should generate 2 more pulses
            clock.processBlock(sampleRate, 2000);
            juce::Thread::sleep(10);
            
            expect(clock.getCurrentPulse() == 3);
            
            clock.removeListener(&listener);
        }
        
        beginTest("Clock Division Calculations");
        {
            MasterClock clock;
            
            double sampleRate = 48000.0;
            float bpm = 120.0f;
            
            // Test division calculations
            double samplesPerQuarter = MasterClock::getSamplesPerDivision(
                MasterClock::Division::Quarter, bpm, sampleRate);
            
            // At 120 BPM, quarter note = 0.5 seconds = 24000 samples
            expectWithinAbsoluteError(samplesPerQuarter, 24000.0, 1.0);
            
            double samplesPerEighth = MasterClock::getSamplesPerDivision(
                MasterClock::Division::Eighth, bpm, sampleRate);
            
            // Eighth note = half of quarter = 12000 samples
            expectWithinAbsoluteError(samplesPerEighth, 12000.0, 1.0);
        }
        
        beginTest("Timing Jitter Test");
        {
            MasterClock clock;
            clock.setBPM(120.0f);
            clock.start();
            
            double sampleRate = 48000.0;
            int samplesPerPulse = 1000;  // At 120 BPM
            
            std::vector<high_resolution_clock::time_point> timestamps;
            
            // Process 10 pulses and measure timing
            for (int i = 0; i < 10; ++i)
            {
                auto startTime = high_resolution_clock::now();
                clock.processBlock(sampleRate, samplesPerPulse);
                timestamps.push_back(startTime);
            }
            
            // Calculate jitter (deviation from expected interval)
            if (timestamps.size() > 1)
            {
                double maxJitter = 0.0;
                
                for (size_t i = 1; i < timestamps.size(); ++i)
                {
                    auto interval = duration_cast<microseconds>(
                        timestamps[i] - timestamps[i-1]).count();
                    
                    // Expected interval in microseconds
                    double expected = (samplesPerPulse / sampleRate) * 1000000.0;
                    double jitter = std::abs(interval - expected);
                    
                    if (jitter > maxJitter)
                        maxJitter = jitter;
                }
                
                // Jitter should be < 100 microseconds (0.1ms)
                expect(maxJitter < 100.0, "Timing jitter exceeds 0.1ms");
            }
        }
    }
};

//==============================================================================
class AsyncPatternEngineTests : public juce::UnitTest
{
public:
    AsyncPatternEngineTests() : UnitTest("AsyncPatternEngine Tests") {}
    
    void runTest() override
    {
        beginTest("Pattern Queue and Switch");
        {
            MasterClock clock;
            AsyncPatternEngine engine(clock);
            
            expect(engine.getCurrentPatternIndex() == 0);
            expect(!engine.hasPendingSwitch());
            
            // Queue pattern switch
            engine.queuePattern(5, AsyncPatternEngine::SwitchQuantization::IMMEDIATE);
            expect(engine.hasPendingSwitch());
            expect(engine.getPendingPatternIndex() == 5);
            
            // Simulate immediate switch
            clock.start();
            clock.processBlock(48000.0, 1);
            juce::Thread::sleep(10);
            
            // Pattern should have switched
            expect(engine.getCurrentPatternIndex() == 5);
            expect(!engine.hasPendingSwitch());
        }
        
        beginTest("Quantized Pattern Switching");
        {
            MasterClock clock;
            AsyncPatternEngine engine(clock);
            
            clock.setBPM(120.0f);
            clock.start();
            
            // Queue switch for next bar
            engine.queuePattern(3, AsyncPatternEngine::SwitchQuantization::NEXT_BAR);
            
            // Process less than a bar worth of samples
            clock.processBlock(48000.0, 40000);  // Less than 48000 (1 bar at 120 BPM)
            juce::Thread::sleep(10);
            
            // Should still be pending
            expect(engine.hasPendingSwitch());
            expect(engine.getCurrentPatternIndex() == 0);
            
            // Process to complete the bar
            clock.processBlock(48000.0, 10000);
            juce::Thread::sleep(10);
            
            // Should have switched
            expect(!engine.hasPendingSwitch());
            expect(engine.getCurrentPatternIndex() == 3);
        }
        
        beginTest("Scene Switching");
        {
            MasterClock clock;
            AsyncPatternEngine engine(clock);
            
            expect(engine.getCurrentSceneIndex() == 0);
            
            // Queue scene switch
            engine.queueScene(2, AsyncPatternEngine::SwitchQuantization::NEXT_BEAT);
            expect(engine.getPendingSceneIndex() == 2);
            
            clock.setBPM(120.0f);
            clock.start();
            
            // Process one beat worth of samples (24 pulses)
            clock.processBlock(48000.0, 24000);  // 1 beat at 120 BPM
            juce::Thread::sleep(10);
            
            // Should have switched
            expect(engine.getCurrentSceneIndex() == 2);
            expect(!engine.hasPendingSwitch());
        }
        
        beginTest("Cancel Pending Switch");
        {
            MasterClock clock;
            AsyncPatternEngine engine(clock);
            
            engine.queuePattern(7, AsyncPatternEngine::SwitchQuantization::NEXT_BAR);
            expect(engine.hasPendingSwitch());
            
            engine.cancelPendingSwitch();
            expect(!engine.hasPendingSwitch());
            expect(engine.getPendingPatternIndex() == -1);
        }
    }
};

//==============================================================================
static MasterClockTests masterClockTests;
static AsyncPatternEngineTests asyncPatternEngineTests;

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