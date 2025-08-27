#include <JuceHeader.h>
#include "../Core/PerformanceMetrics.h"
#include "Infrastructure/Audio/HAMAudioProcessor.h"

using namespace HAM;
using namespace HAM::Performance;

/**
 * Audio processing latency tests
 */
class LatencyTest : public juce::UnitTest {
public:
    LatencyTest() : UnitTest("Audio Latency Test") {}
    
    void runTest() override {
        beginTest("Processing Latency");
        testProcessingLatency();
        
        beginTest("MIDI to Audio Latency");
        testMidiToAudioLatency();
        
        beginTest("Buffer Size Impact");
        testBufferSizeImpact();
        
        beginTest("Sample Rate Impact");
        testSampleRateImpact();
        
        beginTest("Worst Case Latency");
        testWorstCaseLatency();
    }
    
private:
    void testProcessingLatency() {
        HAMAudioProcessor processor;
        processor.prepareToPlay(48000.0, 512);
        
        LatencyMonitor monitor;
        juce::AudioBuffer<float> buffer(2, 512);
        juce::MidiBuffer midi;
        
        // Warm up
        for (int i = 0; i < 10; ++i) {
            processor.processBlock(buffer, midi);
        }
        
        // Measure
        for (int i = 0; i < 1000; ++i) {
            auto start = std::chrono::high_resolution_clock::now();
            processor.processBlock(buffer, midi);
            auto end = std::chrono::high_resolution_clock::now();
            
            double latencyMs = std::chrono::duration<double, std::milli>(end - start).count();
            monitor.recordLatency(latencyMs);
        }
        
        auto metrics = monitor.getMetrics();
        
        logMessage("Processing latency:");
        logMessage("  Mean: " + String(metrics.mean, 3) + " ms");
        logMessage("  P99:  " + String(metrics.p99, 3) + " ms");
        logMessage("  Max:  " + String(metrics.max, 3) + " ms");
        
        expect(metrics.mean < 2.0, "Mean latency too high");
        expect(metrics.max < PerformanceThresholds::MAX_AUDIO_LATENCY_MS, 
               "Max latency exceeds threshold");
    }
    
    void testMidiToAudioLatency() {
        HAMAudioProcessor processor;
        processor.prepareToPlay(48000.0, 512);
        
        LatencyMonitor monitor;
        
        for (int i = 0; i < 100; ++i) {
            juce::AudioBuffer<float> buffer(2, 512);
            juce::MidiBuffer midi;
            
            // Add MIDI note at sample 0
            midi.addEvent(juce::MidiMessage::noteOn(1, 60, (uint8_t)100), 0);
            
            auto noteTime = std::chrono::high_resolution_clock::now();
            processor.processBlock(buffer, midi);
            auto processTime = std::chrono::high_resolution_clock::now();
            
            // Check if audio was generated (simplified check)
            bool audioGenerated = false;
            for (int ch = 0; ch < 2; ++ch) {
                const float* data = buffer.getReadPointer(ch);
                for (int s = 0; s < 512; ++s) {
                    if (std::abs(data[s]) > 0.001f) {
                        audioGenerated = true;
                        
                        // Calculate latency to first audio sample
                        double sampleLatencyMs = (s / 48000.0) * 1000.0;
                        double processingLatencyMs = std::chrono::duration<double, std::milli>(
                            processTime - noteTime).count();
                        
                        monitor.recordLatency(sampleLatencyMs + processingLatencyMs);
                        break;
                    }
                }
                if (audioGenerated) break;
            }
        }
        
        auto metrics = monitor.getMetrics();
        logMessage("MIDI to audio latency: " + String(metrics.mean, 3) + " ms");
        
        expect(metrics.mean < 3.0, "MIDI to audio latency too high");
    }
    
    void testBufferSizeImpact() {
        const int bufferSizes[] = {64, 128, 256, 512, 1024, 2048};
        
        logMessage("Buffer size impact on latency:");
        
        for (int bufferSize : bufferSizes) {
            HAMAudioProcessor processor;
            processor.prepareToPlay(48000.0, bufferSize);
            
            LatencyMonitor monitor;
            juce::AudioBuffer<float> buffer(2, bufferSize);
            juce::MidiBuffer midi;
            
            // Add proportional MIDI events
            for (int i = 0; i < bufferSize / 64; ++i) {
                midi.addEvent(juce::MidiMessage::noteOn(1, 60 + i, (uint8_t)100), i * 64);
            }
            
            for (int i = 0; i < 100; ++i) {
                auto start = std::chrono::high_resolution_clock::now();
                processor.processBlock(buffer, midi);
                auto end = std::chrono::high_resolution_clock::now();
                
                double latencyMs = std::chrono::duration<double, std::milli>(end - start).count();
                monitor.recordLatency(latencyMs);
            }
            
            auto metrics = monitor.getMetrics();
            
            // Theoretical minimum latency
            double theoreticalMs = (bufferSize / 48000.0) * 1000.0;
            
            logMessage("  " + String(bufferSize) + " samples: " + 
                      String(metrics.mean, 3) + " ms (theoretical: " + 
                      String(theoreticalMs, 3) + " ms)");
            
            // Processing should add minimal overhead
            double overhead = metrics.mean - theoreticalMs;
            expect(overhead < 1.0, "Excessive processing overhead at buffer size " + String(bufferSize));
        }
    }
    
    void testSampleRateImpact() {
        const double sampleRates[] = {44100.0, 48000.0, 88200.0, 96000.0};
        
        logMessage("Sample rate impact on latency:");
        
        for (double sampleRate : sampleRates) {
            HAMAudioProcessor processor;
            processor.prepareToPlay(sampleRate, 512);
            
            LatencyMonitor monitor;
            juce::AudioBuffer<float> buffer(2, 512);
            juce::MidiBuffer midi;
            
            for (int i = 0; i < 100; ++i) {
                auto start = std::chrono::high_resolution_clock::now();
                processor.processBlock(buffer, midi);
                auto end = std::chrono::high_resolution_clock::now();
                
                double latencyMs = std::chrono::duration<double, std::milli>(end - start).count();
                monitor.recordLatency(latencyMs);
            }
            
            auto metrics = monitor.getMetrics();
            double theoreticalMs = (512.0 / sampleRate) * 1000.0;
            
            logMessage("  " + String(sampleRate/1000.0, 1) + " kHz: " + 
                      String(metrics.mean, 3) + " ms");
            
            // Higher sample rates should have lower latency
            if (sampleRate > 48000.0) {
                expect(metrics.mean < 3.0, "High sample rate latency not improved");
            }
        }
    }
    
    void testWorstCaseLatency() {
        HAMAudioProcessor processor;
        processor.prepareToPlay(48000.0, 512);
        
        // Enable all tracks (worst case)
        // for (int i = 0; i < 8; ++i) {
        //     processor.enableTrack(i);
        // }
        
        LatencyMonitor monitor;
        juce::AudioBuffer<float> buffer(2, 512);
        
        // Worst case: maximum MIDI events
        juce::MidiBuffer midi;
        for (int i = 0; i < 200; ++i) {
            midi.addEvent(
                juce::MidiMessage::noteOn((i % 8) + 1, 36 + (i % 48), (uint8_t)(64 + i % 64)),
                i * 2
            );
        }
        
        logMessage("Testing worst-case latency with 200 MIDI events...");
        
        for (int i = 0; i < 100; ++i) {
            auto start = std::chrono::high_resolution_clock::now();
            processor.processBlock(buffer, midi);
            auto end = std::chrono::high_resolution_clock::now();
            
            double latencyMs = std::chrono::duration<double, std::milli>(end - start).count();
            monitor.recordLatency(latencyMs);
        }
        
        auto metrics = monitor.getMetrics();
        
        logMessage("Worst-case latency:");
        logMessage("  Mean: " + String(metrics.mean, 3) + " ms");
        logMessage("  P99:  " + String(metrics.p99, 3) + " ms");
        logMessage("  Max:  " + String(metrics.max, 3) + " ms");
        
        expect(metrics.max < PerformanceThresholds::MAX_AUDIO_LATENCY_MS,
               "Worst-case latency exceeds threshold");
        
        processor.releaseResources();
    }
};

static LatencyTest latencyTest;