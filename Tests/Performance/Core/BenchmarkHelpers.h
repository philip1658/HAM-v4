#pragma once

#include <benchmark/benchmark.h>
#include <JuceHeader.h>
#include "PerformanceMetrics.h"
#include <random>
#include <vector>

namespace HAM::Performance {

/**
 * Helper utilities for benchmarking HAM components
 */
class BenchmarkHelpers {
public:
    /**
     * Generate realistic MIDI test data
     */
    static juce::MidiBuffer generateTestMidiBuffer(int numEvents, double sampleRate, int bufferSize) {
        juce::MidiBuffer buffer;
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> note_dist(36, 84); // C2 to C6
        std::uniform_int_distribution<> vel_dist(1, 127);
        std::uniform_int_distribution<> time_dist(0, bufferSize - 1);
        
        for (int i = 0; i < numEvents; ++i) {
            int samplePos = time_dist(gen);
            int note = note_dist(gen);
            int velocity = vel_dist(gen);
            
            if (i % 2 == 0) {
                buffer.addEvent(juce::MidiMessage::noteOn(1, note, (uint8_t)velocity), samplePos);
            } else {
                buffer.addEvent(juce::MidiMessage::noteOff(1, note, (uint8_t)velocity), samplePos);
            }
        }
        
        return buffer;
    }
    
    /**
     * Generate test audio buffer
     */
    static juce::AudioBuffer<float> generateTestAudioBuffer(int numChannels, int numSamples) {
        juce::AudioBuffer<float> buffer(numChannels, numSamples);
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        
        for (int ch = 0; ch < numChannels; ++ch) {
            float* channelData = buffer.getWritePointer(ch);
            for (int i = 0; i < numSamples; ++i) {
                channelData[i] = dist(gen) * 0.1f; // Low amplitude noise
            }
        }
        
        return buffer;
    }
    
    /**
     * Fixture for audio processor benchmarks
     */
    class AudioProcessorFixture : public benchmark::Fixture {
    protected:
        static constexpr double SAMPLE_RATE = 48000.0;
        static constexpr int BUFFER_SIZE = 512;
        static constexpr int NUM_CHANNELS = 2;
        
        juce::AudioBuffer<float> audioBuffer;
        juce::MidiBuffer midiBuffer;
        
        void SetUp(const benchmark::State& state) override {
            audioBuffer = generateTestAudioBuffer(NUM_CHANNELS, BUFFER_SIZE);
            midiBuffer = generateTestMidiBuffer(state.range(0), SAMPLE_RATE, BUFFER_SIZE);
        }
        
        void TearDown(const benchmark::State&) override {
            audioBuffer = juce::AudioBuffer<float>();
            midiBuffer.clear();
        }
    };
    
    /**
     * Measure CPU usage during benchmark
     */
    class CPUUsageReporter {
    public:
        CPUUsageReporter(benchmark::State& state) : state_(state) {
            monitor_.startMeasurement();
        }
        
        ~CPUUsageReporter() {
            monitor_.endMeasurement();
            auto metrics = monitor_.getMetrics();
            state_.counters["cpu_usage_percent"] = metrics.mean;
            state_.counters["cpu_usage_max_percent"] = metrics.max;
        }
        
    private:
        benchmark::State& state_;
        CPUMonitor monitor_;
    };
    
    /**
     * Measure memory usage during benchmark
     */
    class MemoryUsageReporter {
    public:
        MemoryUsageReporter(benchmark::State& state) : state_(state) {
            initial_stats_ = monitor_.getStats();
        }
        
        ~MemoryUsageReporter() {
            auto final_stats = monitor_.getStats();
            state_.counters["memory_allocated_mb"] = 
                (final_stats.current_bytes - initial_stats_.current_bytes) / (1024.0 * 1024.0);
            state_.counters["memory_peak_mb"] = 
                final_stats.peak_bytes / (1024.0 * 1024.0);
            state_.counters["allocations"] = 
                final_stats.allocation_count - initial_stats_.allocation_count;
        }
        
    private:
        benchmark::State& state_;
        MemoryMonitor monitor_;
        MemoryMonitor::MemoryStats initial_stats_;
    };
    
    /**
     * Custom benchmark reporter for HAM metrics
     */
    class HAMBenchmarkReporter : public benchmark::ConsoleReporter {
    public:
        bool ReportContext(const Context& context) override {
            std::cout << "HAM Performance Benchmark Suite\n";
            std::cout << "================================\n";
            std::cout << "CPU Threshold: " << PerformanceThresholds::MAX_CPU_USAGE_PERCENT << "%\n";
            std::cout << "MIDI Jitter Threshold: " << PerformanceThresholds::MAX_MIDI_JITTER_MS << "ms\n";
            std::cout << "Audio Latency Threshold: " << PerformanceThresholds::MAX_AUDIO_LATENCY_MS << "ms\n";
            std::cout << "Memory Threshold: " << PerformanceThresholds::MAX_MEMORY_MB << "MB\n\n";
            return ConsoleReporter::ReportContext(context);
        }
        
        void ReportRuns(const std::vector<Run>& reports) override {
            ConsoleReporter::ReportRuns(reports);
            
            // Check for threshold violations
            for (const auto& run : reports) {
                checkThresholdViolations(run);
            }
        }
        
    private:
        void checkThresholdViolations(const Run& run) {
            bool violation = false;
            std::stringstream warnings;
            
            auto cpu_it = run.counters.find("cpu_usage_percent");
            if (cpu_it != run.counters.end()) {
                if (cpu_it->second.value > PerformanceThresholds::MAX_CPU_USAGE_PERCENT) {
                    warnings << "  ⚠️ CPU usage exceeds threshold: " 
                            << cpu_it->second.value << "% > " 
                            << PerformanceThresholds::MAX_CPU_USAGE_PERCENT << "%\n";
                    violation = true;
                }
            }
            
            auto jitter_it = run.counters.find("midi_jitter_ms");
            if (jitter_it != run.counters.end()) {
                if (jitter_it->second.value > PerformanceThresholds::MAX_MIDI_JITTER_MS) {
                    warnings << "  ⚠️ MIDI jitter exceeds threshold: " 
                            << jitter_it->second.value << "ms > " 
                            << PerformanceThresholds::MAX_MIDI_JITTER_MS << "ms\n";
                    violation = true;
                }
            }
            
            if (violation) {
                std::cout << "\n🔴 Threshold Violations for " << run.benchmark_name() << ":\n";
                std::cout << warnings.str();
            }
        }
    };
};

/**
 * Macro for registering HAM-specific benchmarks with performance monitoring
 */
#define HAM_BENCHMARK(func) \
    BENCHMARK(func)->Unit(benchmark::kMicrosecond)->Iterations(1000)

#define HAM_BENCHMARK_WITH_ARG(func, arg) \
    BENCHMARK(func)->Arg(arg)->Unit(benchmark::kMicrosecond)->Iterations(1000)

#define HAM_BENCHMARK_RANGE(func, min, max) \
    BENCHMARK(func)->RangeMultiplier(2)->Range(min, max)->Unit(benchmark::kMicrosecond)

} // namespace HAM::Performance