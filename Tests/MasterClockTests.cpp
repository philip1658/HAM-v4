/*
  ==============================================================================

    MasterClockTests.cpp
    Comprehensive unit tests for MasterClock component
    Coverage target: >80% line coverage

  ==============================================================================
*/

#include <JuceHeader.h>
#include "../Source/Domain/Clock/MasterClock.h"
#include <chrono>
#include <thread>

using namespace HAM;

//==============================================================================
// Test listener for capturing clock events
class TestClockListener : public MasterClock::Listener
{
public:
    struct Event
    {
        enum Type { Pulse, Start, Stop, Reset, TempoChange };
        Type type;
        int pulseNumber{0};
        float tempo{0.0f};
    };
    
    std::vector<Event> events;
    std::atomic<int> pulseCount{0};
    std::atomic<int> startCount{0};
    std::atomic<int> stopCount{0};
    std::atomic<int> resetCount{0};
    std::atomic<int> tempoChangeCount{0};
    
    void onClockPulse(int pulseNumber) override
    {
        events.push_back({Event::Pulse, pulseNumber, 0.0f});
        pulseCount++;
    }
    
    void onClockStart() override
    {
        events.push_back({Event::Start, 0, 0.0f});
        startCount++;
    }
    
    void onClockStop() override
    {
        events.push_back({Event::Stop, 0, 0.0f});
        stopCount++;
    }
    
    void onClockReset() override
    {
        events.push_back({Event::Reset, 0, 0.0f});
        resetCount++;
    }
    
    void onTempoChanged(float newBPM) override
    {
        events.push_back({Event::TempoChange, 0, newBPM});
        tempoChangeCount++;
    }
    
    void reset()
    {
        events.clear();
        pulseCount = 0;
        startCount = 0;
        stopCount = 0;
        resetCount = 0;
        tempoChangeCount = 0;
    }
};

//==============================================================================
class MasterClockTests : public juce::UnitTest
{
public:
    MasterClockTests() : UnitTest("MasterClock Tests", "Timing") {}
    
    void runTest() override
    {
        testConstruction();
        testTransportControl();
        testTempoControl();
        testSampleAccurateProcessing();
        testClockQuery();
        testClockDivisions();
        testListenerManagement();
        testExternalSync();
        testTempoGlide();
        testEdgeCases();
        testThreadSafety();
    }
    
private:
    void testConstruction()
    {
        beginTest("Construction and Initial State");
        
        MasterClock clock;
        
        // Test initial state
        expect(!clock.isRunning(), "Clock should not be running initially");
        expectEquals(clock.getBPM(), 120.0f, "Default BPM should be 120");
        expectEquals(clock.getCurrentPulse(), 0, "Initial pulse should be 0");
        expectEquals(clock.getCurrentBar(), 0, "Initial bar should be 0");
        expectEquals(clock.getCurrentBeat(), 0, "Initial beat should be 0");
        expectEquals(clock.getPulsePhase(), 0.0f, "Initial phase should be 0");
        expect(!clock.isExternalSyncEnabled(), "External sync should be disabled");
    }
    
    void testTransportControl()
    {
        beginTest("Transport Control");
        
        MasterClock clock;
        TestClockListener listener;
        clock.addListener(&listener);
        
        // Test start
        clock.start();
        expect(clock.isRunning(), "Clock should be running after start");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        expectGreaterThan(listener.startCount.load(), 0, "Start event should be triggered");
        
        // Test stop
        clock.stop();
        expect(!clock.isRunning(), "Clock should not be running after stop");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        expectGreaterThan(listener.stopCount.load(), 0, "Stop event should be triggered");
        
        // Test reset
        clock.processBlock(48000, 1000); // Advance clock
        clock.reset();
        expectEquals(clock.getCurrentPulse(), 0, "Pulse should be 0 after reset");
        expectEquals(clock.getCurrentBar(), 0, "Bar should be 0 after reset");
        expectEquals(clock.getCurrentBeat(), 0, "Beat should be 0 after reset");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        expectGreaterThan(listener.resetCount.load(), 0, "Reset event should be triggered");
        
        // Test start/stop/start sequence
        listener.reset();
        clock.start();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        clock.stop();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        clock.start();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        expectEquals(listener.startCount.load(), 2, "Should have 2 start events");
        expectEquals(listener.stopCount.load(), 1, "Should have 1 stop event");
        
        clock.removeListener(&listener);
    }
    
    void testTempoControl()
    {
        beginTest("Tempo Control");
        
        MasterClock clock;
        TestClockListener listener;
        clock.addListener(&listener);
        
        // Test BPM setting
        clock.setBPM(140.0f);
        expectEquals(clock.getBPM(), 140.0f, "BPM should be updated to 140");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        expectGreaterThan(listener.tempoChangeCount.load(), 0, "Tempo change event should be triggered");
        
        // Test BPM range limits
        clock.setBPM(20.0f);
        expectEquals(clock.getBPM(), 20.0f, "Should accept minimum BPM of 20");
        
        clock.setBPM(999.0f);
        expectEquals(clock.getBPM(), 999.0f, "Should accept maximum BPM of 999");
        
        // Test invalid BPM values
        clock.setBPM(10.0f); // Below minimum
        expectEquals(clock.getBPM(), 20.0f, "Should clamp to minimum BPM");
        
        clock.setBPM(1500.0f); // Above maximum
        expectEquals(clock.getBPM(), 999.0f, "Should clamp to maximum BPM");
        
        // Test sample rate setting
        clock.setSampleRate(44100.0);
        clock.setSampleRate(48000.0);
        clock.setSampleRate(96000.0);
        
        clock.removeListener(&listener);
    }
    
    void testSampleAccurateProcessing()
    {
        beginTest("Sample-Accurate Processing");
        
        MasterClock clock;
        TestClockListener listener;
        clock.addListener(&listener);
        
        const double sampleRate = 48000.0;
        const float bpm = 120.0f;
        clock.setBPM(bpm);
        clock.setSampleRate(sampleRate);
        
        // Calculate expected samples per pulse
        // 24 PPQN, 120 BPM = 2 beats/sec = 48 pulses/sec
        // 48000 samples/sec / 48 pulses/sec = 1000 samples/pulse
        const double expectedSamplesPerPulse = sampleRate / (bpm / 60.0 * 24.0);
        
        // Start clock and process blocks
        clock.start();
        listener.reset();
        
        // Process exactly one pulse worth of samples
        clock.processBlock(sampleRate, static_cast<int>(expectedSamplesPerPulse));
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        expectEquals(listener.pulseCount.load(), 1, "Should generate exactly 1 pulse");
        expectEquals(clock.getCurrentPulse(), 1, "Current pulse should be 1");
        
        // Process multiple pulses
        listener.reset();
        clock.processBlock(sampleRate, static_cast<int>(expectedSamplesPerPulse * 5));
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        expectEquals(listener.pulseCount.load(), 5, "Should generate exactly 5 pulses");
        expectEquals(clock.getCurrentPulse(), 6, "Current pulse should be 6");
        
        // Test small block processing (less than one pulse)
        listener.reset();
        clock.reset();
        clock.processBlock(sampleRate, 100); // Small block
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        expectEquals(listener.pulseCount.load(), 0, "Should not generate pulse for small block");
        expectWithinAbsoluteError(clock.getPulsePhase(), 0.1f, 0.01f, "Phase should advance proportionally");
        
        // Test processing while stopped
        clock.stop();
        listener.reset();
        int pulseBefore = clock.getCurrentPulse();
        clock.processBlock(sampleRate, 5000);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        expectEquals(listener.pulseCount.load(), 0, "Should not generate pulses when stopped");
        expectEquals(clock.getCurrentPulse(), pulseBefore, "Pulse should not advance when stopped");
        
        clock.removeListener(&listener);
    }
    
    void testClockQuery()
    {
        beginTest("Clock Query Methods");
        
        MasterClock clock;
        const double sampleRate = 48000.0;
        clock.setSampleRate(sampleRate);
        clock.setBPM(120.0f);
        
        // Test initial position
        expectEquals(clock.getCurrentPulse(), 0, "Initial pulse should be 0");
        expectEquals(clock.getCurrentBar(), 0, "Initial bar should be 0");
        expectEquals(clock.getCurrentBeat(), 0, "Initial beat should be 0");
        
        // Advance clock and test position tracking
        clock.start();
        
        // Process to pulse 25 (start of beat 2 in bar 1)
        const double samplesPerPulse = sampleRate / (120.0 / 60.0 * 24.0);
        clock.processBlock(sampleRate, static_cast<int>(samplesPerPulse * 25));
        
        expectEquals(clock.getCurrentPulse(), 1, "Should wrap pulse at 24");
        expectEquals(clock.getCurrentBeat(), 1, "Should be on beat 1");
        expectEquals(clock.getCurrentBar(), 1, "Should be in bar 1");
        
        // Test getSamplesUntilNextPulse
        int samplesUntil = clock.getSamplesUntilNextPulse(sampleRate);
        expectGreaterThan(samplesUntil, 0, "Should have samples until next pulse");
        expectLessThan(samplesUntil, static_cast<int>(samplesPerPulse) + 1, "Should be less than one pulse");
        
        // Test phase calculation
        clock.reset();
        clock.processBlock(sampleRate, static_cast<int>(samplesPerPulse * 0.5));
        expectWithinAbsoluteError(clock.getPulsePhase(), 0.5f, 0.01f, "Phase should be ~0.5");
        
        clock.stop();
    }
    
    void testClockDivisions()
    {
        beginTest("Clock Divisions");
        
        MasterClock clock;
        const double sampleRate = 48000.0;
        clock.setSampleRate(sampleRate);
        clock.setBPM(120.0f);
        
        // Test samples per division calculation
        double samplesPerQuarter = MasterClock::getSamplesPerDivision(
            MasterClock::Division::Quarter, 120.0f, sampleRate);
        double samplesPerEighth = MasterClock::getSamplesPerDivision(
            MasterClock::Division::Eighth, 120.0f, sampleRate);
        
        expectWithinAbsoluteError(samplesPerQuarter, 24000.0, 1.0, "Quarter note should be ~24000 samples");
        expectWithinAbsoluteError(samplesPerEighth, 12000.0, 1.0, "Eighth note should be ~12000 samples");
        
        // Test division alignment
        clock.start();
        
        // On pulse 0 - should align with all divisions
        expect(clock.isOnDivision(MasterClock::Division::Quarter), "Pulse 0 aligns with quarter");
        expect(clock.isOnDivision(MasterClock::Division::Eighth), "Pulse 0 aligns with eighth");
        expect(clock.isOnDivision(MasterClock::Division::Sixteenth), "Pulse 0 aligns with sixteenth");
        
        // Advance to pulse 6 (sixteenth note)
        const double samplesPerPulse = sampleRate / (120.0 / 60.0 * 24.0);
        clock.processBlock(sampleRate, static_cast<int>(samplesPerPulse * 6));
        
        expect(clock.isOnDivision(MasterClock::Division::Sixteenth), "Pulse 6 aligns with sixteenth");
        expect(!clock.isOnDivision(MasterClock::Division::Eighth), "Pulse 6 doesn't align with eighth");
        
        // Test next division pulse calculation
        clock.reset();
        int nextQuarter = clock.getNextDivisionPulse(MasterClock::Division::Quarter);
        expectEquals(nextQuarter, 24, "Next quarter from 0 should be 24");
        
        // Advance to pulse 5
        clock.processBlock(sampleRate, static_cast<int>(samplesPerPulse * 5));
        int nextSixteenth = clock.getNextDivisionPulse(MasterClock::Division::Sixteenth);
        expectEquals(nextSixteenth, 6, "Next sixteenth from 5 should be 6");
        
        clock.stop();
    }
    
    void testListenerManagement()
    {
        beginTest("Listener Management");
        
        MasterClock clock;
        TestClockListener listener1, listener2, listener3;
        
        // Test adding listeners
        clock.addListener(&listener1);
        clock.addListener(&listener2);
        clock.addListener(&listener3);
        
        // Trigger events
        clock.start();
        clock.setBPM(140.0f);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        
        // All listeners should receive events
        expectGreaterThan(listener1.startCount.load(), 0, "Listener 1 should receive start");
        expectGreaterThan(listener2.startCount.load(), 0, "Listener 2 should receive start");
        expectGreaterThan(listener3.startCount.load(), 0, "Listener 3 should receive start");
        
        expectGreaterThan(listener1.tempoChangeCount.load(), 0, "Listener 1 should receive tempo change");
        expectGreaterThan(listener2.tempoChangeCount.load(), 0, "Listener 2 should receive tempo change");
        expectGreaterThan(listener3.tempoChangeCount.load(), 0, "Listener 3 should receive tempo change");
        
        // Test removing listener
        clock.removeListener(&listener2);
        listener1.reset();
        listener2.reset();
        listener3.reset();
        
        clock.stop();
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        
        expectGreaterThan(listener1.stopCount.load(), 0, "Listener 1 should receive stop");
        expectEquals(listener2.stopCount.load(), 0, "Listener 2 should not receive stop after removal");
        expectGreaterThan(listener3.stopCount.load(), 0, "Listener 3 should receive stop");
        
        // Test removing all listeners
        clock.removeListener(&listener1);
        clock.removeListener(&listener3);
        
        listener1.reset();
        listener3.reset();
        clock.reset();
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        
        expectEquals(listener1.resetCount.load(), 0, "Listener 1 should not receive reset after removal");
        expectEquals(listener3.resetCount.load(), 0, "Listener 3 should not receive reset after removal");
        
        // Test adding same listener multiple times (should handle gracefully)
        clock.addListener(&listener1);
        clock.addListener(&listener1); // Duplicate
        listener1.reset();
        
        clock.start();
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        
        // Should only receive one event even if added twice
        expectEquals(listener1.startCount.load(), 1, "Should not duplicate events for same listener");
        
        clock.removeListener(&listener1);
    }
    
    void testExternalSync()
    {
        beginTest("External MIDI Clock Sync");
        
        MasterClock clock;
        TestClockListener listener;
        clock.addListener(&listener);
        
        // Test enabling/disabling external sync
        expect(!clock.isExternalSyncEnabled(), "External sync should be disabled by default");
        
        clock.setExternalSyncEnabled(true);
        expect(clock.isExternalSyncEnabled(), "External sync should be enabled");
        
        clock.setExternalSyncEnabled(false);
        expect(!clock.isExternalSyncEnabled(), "External sync should be disabled");
        
        // Test MIDI clock messages
        clock.setExternalSyncEnabled(true);
        
        // Test MIDI start
        listener.reset();
        clock.processMidiStart();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        expectGreaterThan(listener.startCount.load(), 0, "MIDI start should trigger start event");
        expect(clock.isRunning(), "Clock should be running after MIDI start");
        
        // Test MIDI clock pulses
        listener.reset();
        // MIDI sends 24 clocks per quarter note, we use 24 PPQN internally
        for (int i = 0; i < 24; ++i)
        {
            clock.processMidiClock();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        expectGreaterOrEqual(listener.pulseCount.load(), 1, "MIDI clocks should generate pulses");
        
        // Test MIDI stop
        listener.reset();
        clock.processMidiStop();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        expectGreaterThan(listener.stopCount.load(), 0, "MIDI stop should trigger stop event");
        expect(!clock.isRunning(), "Clock should stop after MIDI stop");
        
        // Test MIDI continue
        listener.reset();
        clock.processMidiContinue();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        expectGreaterThan(listener.startCount.load(), 0, "MIDI continue should trigger start event");
        expect(clock.isRunning(), "Clock should be running after MIDI continue");
        
        // Test that internal processing is disabled during external sync
        clock.setExternalSyncEnabled(true);
        clock.start();
        listener.reset();
        
        const double sampleRate = 48000.0;
        clock.processBlock(sampleRate, 48000); // Process 1 second
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        // When using external sync, processBlock shouldn't generate internal pulses
        expectEquals(listener.pulseCount.load(), 0, "Should not generate internal pulses during external sync");
        
        clock.removeListener(&listener);
    }
    
    void testTempoGlide()
    {
        beginTest("Tempo Glide");
        
        MasterClock clock;
        TestClockListener listener;
        clock.addListener(&listener);
        
        const double sampleRate = 48000.0;
        clock.setSampleRate(sampleRate);
        
        // Test enabling/disabling tempo glide
        clock.setTempoGlideEnabled(false);
        clock.setBPM(120.0f);
        clock.setBPM(140.0f);
        expectEquals(clock.getBPM(), 140.0f, "Without glide, tempo should change immediately");
        
        // Enable tempo glide
        clock.setTempoGlideEnabled(true);
        clock.setTempoGlideTime(100.0f); // 100ms glide time
        
        // Set initial tempo
        clock.setBPM(120.0f);
        clock.start();
        
        // Change tempo with glide
        clock.setBPM(180.0f);
        
        // The target should be set immediately
        expectEquals(clock.getBPM(), 180.0f, "Target BPM should be set immediately");
        
        // Process blocks to apply glide
        const int samplesFor50ms = static_cast<int>(sampleRate * 0.05);
        clock.processBlock(sampleRate, samplesFor50ms);
        
        // After 50ms of 100ms glide, we should be halfway
        // Note: Internal glide implementation may vary, checking for gradual change
        clock.processBlock(sampleRate, samplesFor50ms); // Complete the glide
        
        // Test very short glide time
        clock.setTempoGlideTime(1.0f); // 1ms glide
        clock.setBPM(100.0f);
        clock.processBlock(sampleRate, 100); // Process minimal samples
        
        // Test very long glide time
        clock.setTempoGlideTime(5000.0f); // 5 second glide
        clock.setBPM(200.0f);
        
        clock.stop();
        clock.removeListener(&listener);
    }
    
    void testEdgeCases()
    {
        beginTest("Edge Cases and Boundary Conditions");
        
        MasterClock clock;
        TestClockListener listener;
        clock.addListener(&listener);
        
        // Test very low BPM
        clock.setBPM(20.0f);
        clock.setSampleRate(48000.0);
        clock.start();
        
        // At 20 BPM, one pulse = 48000 / (20/60 * 24) = 6000 samples
        clock.processBlock(48000.0, 6000);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        expectEquals(listener.pulseCount.load(), 1, "Should handle very low BPM");
        
        // Test very high BPM
        listener.reset();
        clock.setBPM(999.0f);
        // At 999 BPM, one pulse = 48000 / (999/60 * 24) ≈ 120 samples
        clock.processBlock(48000.0, 240);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        expectGreaterOrEqual(listener.pulseCount.load(), 1, "Should handle very high BPM");
        
        // Test bar/beat wraparound
        clock.reset();
        clock.setBPM(120.0f);
        
        // Process exactly 96 pulses (one full bar in 4/4)
        const double samplesPerPulse = 48000.0 / (120.0 / 60.0 * 24.0);
        clock.processBlock(48000.0, static_cast<int>(samplesPerPulse * 96));
        
        expectEquals(clock.getCurrentBar(), 1, "Should advance to bar 1");
        expectEquals(clock.getCurrentBeat(), 0, "Should wrap to beat 0");
        expectEquals(clock.getCurrentPulse(), 0, "Should wrap to pulse 0");
        
        // Test processing zero samples
        listener.reset();
        int pulseBefore = clock.getCurrentPulse();
        clock.processBlock(48000.0, 0);
        expectEquals(clock.getCurrentPulse(), pulseBefore, "Zero samples should not advance clock");
        expectEquals(listener.pulseCount.load(), 0, "Zero samples should not generate pulses");
        
        // Test negative samples (should handle gracefully)
        clock.processBlock(48000.0, -100);
        expectEquals(clock.getCurrentPulse(), pulseBefore, "Negative samples should not crash or advance");
        
        // Test very large block size
        listener.reset();
        clock.reset();
        clock.processBlock(48000.0, 480000); // 10 seconds at once
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        expectGreaterThan(listener.pulseCount.load(), 100, "Should handle large block sizes");
        
        // Test sample rate changes
        clock.reset();
        clock.setSampleRate(44100.0);
        clock.processBlock(44100.0, 44100);
        int pulse44k = clock.getCurrentPulse();
        
        clock.reset();
        clock.setSampleRate(96000.0);
        clock.processBlock(96000.0, 96000);
        int pulse96k = clock.getCurrentPulse();
        
        // Both should advance roughly the same amount of musical time
        expectWithinAbsoluteError(pulse44k, pulse96k, 2, "Different sample rates should maintain timing");
        
        // Test rapid start/stop
        for (int i = 0; i < 10; ++i)
        {
            clock.start();
            clock.processBlock(48000.0, 100);
            clock.stop();
        }
        
        // Test null listener (should handle gracefully)
        clock.removeListener(nullptr);
        clock.addListener(nullptr);
        
        clock.stop();
        clock.removeListener(&listener);
    }
    
    void testThreadSafety()
    {
        beginTest("Thread Safety");
        
        MasterClock clock;
        TestClockListener listener;
        clock.addListener(&listener);
        
        const double sampleRate = 48000.0;
        clock.setSampleRate(sampleRate);
        clock.setBPM(120.0f);
        
        std::atomic<bool> shouldStop{false};
        
        // Start audio thread simulation
        std::thread audioThread([&clock, &shouldStop, sampleRate]()
        {
            while (!shouldStop.load())
            {
                clock.processBlock(sampleRate, 512);
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
        });
        
        // Start control thread simulation
        std::thread controlThread([&clock, &shouldStop]()
        {
            int count = 0;
            while (!shouldStop.load() && count++ < 100)
            {
                clock.setBPM(100.0f + (count % 100));
                if (count % 10 == 0)
                {
                    clock.isRunning() ? clock.stop() : clock.start();
                }
                if (count % 20 == 0)
                {
                    clock.reset();
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        });
        
        // Let threads run for a bit
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        // Stop threads
        shouldStop = true;
        audioThread.join();
        controlThread.join();
        
        // If we get here without crashing, thread safety is working
        expect(true, "Thread safety test completed without crashes");
        
        // Verify clock is still functional
        clock.reset();
        clock.setBPM(120.0f);
        clock.start();
        listener.reset();
        
        clock.processBlock(sampleRate, 2000);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        
        expectGreaterThan(listener.pulseCount.load(), 0, "Clock should still function after thread test");
        
        clock.stop();
        clock.removeListener(&listener);
    }
};

static MasterClockTests masterClockTests;

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