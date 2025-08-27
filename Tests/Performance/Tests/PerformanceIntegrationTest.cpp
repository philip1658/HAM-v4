#include <JuceHeader.h>
#include "../Core/PerformanceMetrics.h"
#include "Infrastructure/Audio/HAMAudioProcessor.h"
#include <thread>
#include <chrono>

using namespace HAM;
using namespace HAM::Performance;

/**
 * Integration test that runs the full HAM system and measures performance
 */
class PerformanceIntegrationTest : public juce::UnitTest {
public:
    PerformanceIntegrationTest() : UnitTest("Performance Integration Test") {}
    
    void runTest() override {
        beginTest("Full System Performance Test");
        
        // Create processor
        HAMAudioProcessor processor;
        const double sampleRate = 48000.0;
        const int bufferSize = 512;
        
        processor.prepareToPlay(sampleRate, bufferSize);
        
        // Reset performance monitoring
        resetPerformanceMonitoring();
        
        // Create test buffers
        juce::AudioBuffer<float> audioBuffer(2, bufferSize);
        juce::MidiBuffer midiBuffer;
        
        // Add some test MIDI events
        for (int i = 0; i < 10; ++i) {
            midiBuffer.addEvent(
                juce::MidiMessage::noteOn(1, 60 + i, (uint8_t)100),
                i * 50
            );
        }
        
        // Monitoring
        CPUMonitor cpuMonitor;
        MemoryMonitor memoryMonitor;
        LatencyMonitor audioLatencyMonitor;
        LatencyMonitor midiLatencyMonitor;
        
        // Run for 1 second worth of audio
        const int numBuffers = static_cast<int>(sampleRate / bufferSize);
        
        logMessage("Running performance test for 1 second of audio...");
        
        cpuMonitor.startMeasurement();
        
        for (int i = 0; i < numBuffers; ++i) {
            auto startTime = std::chrono::high_resolution_clock::now();
            
            // Process audio
            processor.processBlock(audioBuffer, midiBuffer);
            
            auto endTime = std::chrono::high_resolution_clock::now();
            
            // Measure latency
            double latencyMs = std::chrono::duration<double, std::milli>(
                endTime - startTime).count();
            audioLatencyMonitor.recordLatency(latencyMs);
            
            // Clear MIDI buffer for next iteration
            midiBuffer.clear();
            
            // Add occasional MIDI events
            if (i % 10 == 0) {
                midiBuffer.addEvent(
                    juce::MidiMessage::noteOn(1, 60 + (i % 12), (uint8_t)100),
                    0
                );
                midiLatencyMonitor.recordLatency(latencyMs);
            }
        }
        
        cpuMonitor.endMeasurement();
        
        // Get metrics
        auto cpuMetrics = cpuMonitor.getMetrics();
        auto memoryStats = memoryMonitor.getStats();
        auto audioMetrics = audioLatencyMonitor.getMetrics();
        auto midiMetrics = midiLatencyMonitor.getMetrics();
        double midiJitter = midiLatencyMonitor.getJitter();
        
        // Log results
        logMessage("=== Performance Results ===");
        logMessage(String("CPU Usage: ") + String(cpuMetrics.mean, 2) + "%");
        logMessage(String("CPU Max: ") + String(cpuMetrics.max, 2) + "%");
        logMessage(String("Memory Peak: ") + String(memoryStats.peak_bytes / 1024.0 / 1024.0, 2) + " MB");
        logMessage(String("Audio Latency Mean: ") + String(audioMetrics.mean, 3) + " ms");
        logMessage(String("Audio Latency Max: ") + String(audioMetrics.max, 3) + " ms");
        logMessage(String("MIDI Jitter: ") + String(midiJitter, 4) + " ms");
        
        // Check against thresholds
        expect(cpuMetrics.max < PerformanceThresholds::MAX_CPU_USAGE_PERCENT,
               "CPU usage exceeds threshold");
        
        expect(audioMetrics.max < PerformanceThresholds::MAX_AUDIO_LATENCY_MS,
               "Audio latency exceeds threshold");
        
        expect(midiJitter < PerformanceThresholds::MAX_MIDI_JITTER_MS,
               "MIDI jitter exceeds threshold");
        
        expect((memoryStats.peak_bytes / 1024.0 / 1024.0) < PerformanceThresholds::MAX_MEMORY_MB,
               "Memory usage exceeds threshold");
        
        processor.releaseResources();
    }
};

/**
 * Stress test with maximum load
 */
class StressTest : public juce::UnitTest {
public:
    StressTest() : UnitTest("Performance Stress Test") {}
    
    void runTest() override {
        beginTest("Maximum Load Stress Test");
        
        HAMAudioProcessor processor;
        processor.prepareToPlay(48000.0, 512);
        
        // Enable all tracks
        // processor.setAllTracksEnabled(true);
        
        juce::AudioBuffer<float> audioBuffer(2, 512);
        juce::MidiBuffer midiBuffer;
        
        // Fill with maximum MIDI events
        for (int i = 0; i < 200; ++i) {
            midiBuffer.addEvent(
                juce::MidiMessage::noteOn(1 + (i % 16), 36 + (i % 48), (uint8_t)(64 + i % 64)),
                i * 2
            );
        }
        
        CPUMonitor cpuMonitor;
        cpuMonitor.startMeasurement();
        
        // Process under stress
        for (int i = 0; i < 100; ++i) {
            processor.processBlock(audioBuffer, midiBuffer);
        }
        
        cpuMonitor.endMeasurement();
        
        auto cpuMetrics = cpuMonitor.getMetrics();
        
        logMessage("Stress Test CPU Usage: " + String(cpuMetrics.max, 2) + "%");
        
        // Even under stress, should not exceed reasonable limits
        expect(cpuMetrics.max < PerformanceThresholds::MAX_CPU_USAGE_PERCENT * 2,
               "CPU usage under stress is excessive");
        
        processor.releaseResources();
    }
};

/**
 * Thread safety test
 */
class ThreadSafetyTest : public juce::UnitTest {
public:
    ThreadSafetyTest() : UnitTest("Thread Safety Performance Test") {}
    
    void runTest() override {
        beginTest("Concurrent Access Test");
        
        HAMAudioProcessor processor;
        processor.prepareToPlay(48000.0, 512);
        
        std::atomic<bool> stopFlag{false};
        ThreadContentionMonitor contentionMonitor;
        
        // UI thread simulator
        std::thread uiThread([&processor, &stopFlag, &contentionMonitor]() {
            while (!stopFlag) {
                // Simulate UI operations
                // processor.getParameter(0);
                // processor.setParameter(0, 0.5f);
                
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        });
        
        // Audio processing
        juce::AudioBuffer<float> audioBuffer(2, 512);
        juce::MidiBuffer midiBuffer;
        
        auto startTime = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < 1000; ++i) {
            processor.processBlock(audioBuffer, midiBuffer);
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        
        stopFlag = true;
        uiThread.join();
        
        double totalTimeMs = std::chrono::duration<double, std::milli>(
            endTime - startTime).count();
        
        double avgTimePerBuffer = totalTimeMs / 1000.0;
        
        logMessage("Average time per buffer with concurrent UI: " + 
                  String(avgTimePerBuffer, 3) + " ms");
        
        expect(avgTimePerBuffer < 5.0, "Processing too slow with concurrent access");
        
        auto contentionStats = contentionMonitor.getStats();
        expect(contentionStats.total_contentions == 0, "Thread contention detected");
        
        processor.releaseResources();
    }
};

// Register tests
static PerformanceIntegrationTest performanceTest;
static StressTest stressTest;
static ThreadSafetyTest threadSafetyTest;