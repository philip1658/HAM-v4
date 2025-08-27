#include <JuceHeader.h>
#include "../Core/PerformanceMetrics.h"
#include "Domain/Clock/MasterClock.h"
#include "Domain/Processors/MidiEventGenerator.h"
#include <random>

using namespace HAM;
using namespace HAM::Performance;

/**
 * Test MIDI timing jitter to ensure <0.1ms requirement
 */
class JitterTest : public juce::UnitTest {
public:
    JitterTest() : UnitTest("MIDI Jitter Test") {}
    
    void runTest() override {
        beginTest("Clock Tick Jitter");
        testClockJitter();
        
        beginTest("MIDI Event Generation Jitter");
        testMidiGenerationJitter();
        
        beginTest("Pattern Transition Jitter");
        testPatternTransitionJitter();
        
        beginTest("Tempo Change Jitter");
        testTempoChangeJitter();
        
        beginTest("Multi-Track Synchronization");
        testMultiTrackSync();
    }
    
private:
    void testClockJitter() {
        MasterClock clock;
        clock.setSampleRate(48000.0);
        clock.setTempo(120.0);
        
        LatencyMonitor jitterMonitor;
        
        // Expected samples per tick at 120 BPM, 96 PPQ
        const double samplesPerTick = 48000.0 / (120.0 / 60.0 * 96.0);
        
        clock.start();
        
        std::vector<int64_t> tickTimes;
        int64_t lastTick = 0;
        
        // Collect tick times
        for (int i = 0; i < 1000; ++i) {
            clock.advance(512);
            int64_t currentTick = clock.getPositionInSamples();
            
            if (currentTick != lastTick) {
                tickTimes.push_back(currentTick);
                
                if (tickTimes.size() > 1) {
                    // Calculate interval
                    int64_t interval = tickTimes.back() - tickTimes[tickTimes.size() - 2];
                    double expectedInterval = samplesPerTick;
                    double jitterSamples = std::abs(interval - expectedInterval);
                    double jitterMs = (jitterSamples / 48000.0) * 1000.0;
                    
                    jitterMonitor.recordLatency(jitterMs);
                }
                
                lastTick = currentTick;
            }
        }
        
        clock.stop();
        
        auto metrics = jitterMonitor.getMetrics();
        double jitter = jitterMonitor.getJitter();
        
        logMessage("Clock jitter: " + String(jitter, 4) + " ms");
        logMessage("Max deviation: " + String(metrics.max, 4) + " ms");
        
        expect(jitter < PerformanceThresholds::MAX_MIDI_JITTER_MS,
               "Clock jitter exceeds 0.1ms threshold");
        expect(metrics.max < PerformanceThresholds::MAX_MIDI_JITTER_MS * 2,
               "Maximum clock deviation too high");
    }
    
    void testMidiGenerationJitter() {
        MidiEventGenerator generator;
        Track track;
        track.setChannel(1);
        track.setEnabled(true);
        
        auto pattern = std::make_shared<Pattern>();
        pattern->setLength(16);
        pattern->setDivision(Pattern::Division::Sixteenth);
        
        for (int i = 0; i < 16; ++i) {
            pattern->getStage(i).gate = true;
            pattern->getStage(i).pitch = 60;
        }
        
        track.setPattern(pattern);
        
        LatencyMonitor jitterMonitor;
        const double samplesPerStep = 48000.0 / (120.0 / 60.0 * 4.0); // 16th notes
        
        // Generate events and measure timing
        for (int iteration = 0; iteration < 100; ++iteration) {
            for (int step = 0; step < 16; ++step) {
                juce::MidiBuffer buffer;
                int expectedSample = static_cast<int>(step * samplesPerStep);
                
                auto start = std::chrono::high_resolution_clock::now();
                generator.generateMidiEvents(track, step, buffer, expectedSample);
                auto end = std::chrono::high_resolution_clock::now();
                
                // Check actual sample position
                for (const auto metadata : buffer) {
                    int actualSample = metadata.samplePosition;
                    double deviationSamples = std::abs(actualSample - expectedSample);
                    double deviationMs = (deviationSamples / 48000.0) * 1000.0;
                    jitterMonitor.recordLatency(deviationMs);
                }
            }
        }
        
        double jitter = jitterMonitor.getJitter();
        logMessage("MIDI generation jitter: " + String(jitter, 4) + " ms");
        
        expect(jitter < PerformanceThresholds::MAX_MIDI_JITTER_MS,
               "MIDI generation jitter exceeds threshold");
    }
    
    void testPatternTransitionJitter() {
        // Test jitter when switching between patterns
        MasterClock clock;
        clock.setSampleRate(48000.0);
        clock.setTempo(120.0);
        
        std::vector<std::shared_ptr<Pattern>> patterns;
        for (int i = 0; i < 4; ++i) {
            auto pattern = std::make_shared<Pattern>();
            pattern->setLength(16);
            pattern->setDivision(Pattern::Division::Sixteenth);
            patterns.push_back(pattern);
        }
        
        LatencyMonitor transitionMonitor;
        clock.start();
        
        int currentPattern = 0;
        int64_t lastTransitionSample = 0;
        
        for (int i = 0; i < 1000; ++i) {
            clock.advance(512);
            
            // Check for pattern boundary
            int64_t currentBar = clock.getPositionInSamples() / (48000 * 2); // 2 sec per bar at 120 BPM
            int newPattern = currentBar % 4;
            
            if (newPattern != currentPattern) {
                int64_t transitionSample = clock.getPositionInSamples();
                
                if (lastTransitionSample > 0) {
                    int64_t interval = transitionSample - lastTransitionSample;
                    double expectedInterval = 48000.0 * 2.0; // One bar
                    double jitterSamples = std::abs(interval - expectedInterval);
                    double jitterMs = (jitterSamples / 48000.0) * 1000.0;
                    transitionMonitor.recordLatency(jitterMs);
                }
                
                lastTransitionSample = transitionSample;
                currentPattern = newPattern;
            }
        }
        
        clock.stop();
        
        double jitter = transitionMonitor.getJitter();
        logMessage("Pattern transition jitter: " + String(jitter, 4) + " ms");
        
        expect(jitter < PerformanceThresholds::MAX_MIDI_JITTER_MS,
               "Pattern transition jitter exceeds threshold");
    }
    
    void testTempoChangeJitter() {
        MasterClock clock;
        clock.setSampleRate(48000.0);
        
        LatencyMonitor jitterMonitor;
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> tempoDist(60.0, 180.0);
        
        clock.start();
        
        for (int i = 0; i < 100; ++i) {
            double newTempo = tempoDist(gen);
            
            auto start = std::chrono::high_resolution_clock::now();
            clock.setTempo(newTempo);
            clock.advance(512);
            auto end = std::chrono::high_resolution_clock::now();
            
            double changeTimeMs = std::chrono::duration<double, std::milli>(end - start).count();
            jitterMonitor.recordLatency(changeTimeMs);
        }
        
        clock.stop();
        
        auto metrics = jitterMonitor.getMetrics();
        logMessage("Tempo change timing: mean " + String(metrics.mean, 4) + 
                  " ms, max " + String(metrics.max, 4) + " ms");
        
        expect(metrics.max < 1.0, "Tempo changes cause excessive delay");
    }
    
    void testMultiTrackSync() {
        // Test synchronization between multiple tracks
        const int numTracks = 8;
        std::vector<MidiEventGenerator> generators(numTracks);
        std::vector<Track> tracks(numTracks);
        
        for (int i = 0; i < numTracks; ++i) {
            tracks[i].setChannel(i + 1);
            tracks[i].setEnabled(true);
            
            auto pattern = std::make_shared<Pattern>();
            pattern->setLength(16);
            pattern->setDivision(Pattern::Division::Sixteenth);
            
            // All tracks have events on beat 1
            pattern->getStage(0).gate = true;
            pattern->getStage(0).pitch = 60 + i;
            
            tracks[i].setPattern(pattern);
        }
        
        LatencyMonitor syncMonitor;
        
        for (int iteration = 0; iteration < 100; ++iteration) {
            std::vector<int> eventTimes;
            
            // Generate events for all tracks
            for (int t = 0; t < numTracks; ++t) {
                juce::MidiBuffer buffer;
                generators[t].generateMidiEvents(tracks[t], 0, buffer, 0);
                
                // Collect event times
                for (const auto metadata : buffer) {
                    eventTimes.push_back(metadata.samplePosition);
                }
            }
            
            // Check synchronization
            if (!eventTimes.empty()) {
                int minTime = *std::min_element(eventTimes.begin(), eventTimes.end());
                int maxTime = *std::max_element(eventTimes.begin(), eventTimes.end());
                
                double deviationSamples = maxTime - minTime;
                double deviationMs = (deviationSamples / 48000.0) * 1000.0;
                syncMonitor.recordLatency(deviationMs);
            }
        }
        
        auto metrics = syncMonitor.getMetrics();
        logMessage("Multi-track sync deviation: " + String(metrics.max, 4) + " ms");
        
        expect(metrics.max < PerformanceThresholds::MAX_MIDI_JITTER_MS,
               "Multi-track synchronization exceeds jitter threshold");
    }
};

static JitterTest jitterTest;