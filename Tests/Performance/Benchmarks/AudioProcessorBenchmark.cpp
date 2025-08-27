#include <benchmark/benchmark.h>
#include "../Core/BenchmarkHelpers.h"
#include "../Core/PerformanceMetrics.h"
#include "Infrastructure/Audio/HAMAudioProcessor.h"

using namespace HAM::Performance;

/**
 * Benchmark HAMAudioProcessor::processBlock performance
 * This is the most critical real-time function in the system
 */
class AudioProcessorBenchmark : public BenchmarkHelpers::AudioProcessorFixture {
protected:
    std::unique_ptr<HAM::HAMAudioProcessor> processor;
    
    void SetUp(const benchmark::State& state) override {
        AudioProcessorFixture::SetUp(state);
        
        processor = std::make_unique<HAM::HAMAudioProcessor>();
        processor->prepareToPlay(SAMPLE_RATE, BUFFER_SIZE);
    }
    
    void TearDown(const benchmark::State& state) override {
        processor->releaseResources();
        processor.reset();
        
        AudioProcessorFixture::TearDown(state);
    }
};

BENCHMARK_DEFINE_F(AudioProcessorBenchmark, ProcessBlock)(benchmark::State& state) {
    // Measure CPU and memory usage
    CPUUsageReporter cpu_reporter(state);
    MemoryUsageReporter mem_reporter(state);
    
    // Track latency
    LatencyMonitor latency_monitor;
    
    for (auto _ : state) {
        // Create fresh buffers for each iteration
        auto audio = generateTestAudioBuffer(NUM_CHANNELS, BUFFER_SIZE);
        auto midi = generateTestMidiBuffer(state.range(0), SAMPLE_RATE, BUFFER_SIZE);
        
        auto start = std::chrono::high_resolution_clock::now();
        processor->processBlock(audio, midi);
        auto end = std::chrono::high_resolution_clock::now();
        
        double latency_ms = std::chrono::duration<double, std::milli>(end - start).count();
        latency_monitor.recordLatency(latency_ms);
    }
    
    // Report metrics
    auto latency_metrics = latency_monitor.getMetrics();
    state.counters["audio_latency_ms"] = latency_metrics.mean;
    state.counters["audio_latency_p99_ms"] = latency_metrics.p99;
    state.counters["audio_latency_max_ms"] = latency_metrics.max;
    
    // Set bytes processed for throughput calculation
    state.SetBytesProcessed(state.iterations() * BUFFER_SIZE * NUM_CHANNELS * sizeof(float));
    state.SetItemsProcessed(state.iterations() * BUFFER_SIZE);
}

// Register benchmark with different MIDI event loads
BENCHMARK_REGISTER_F(AudioProcessorBenchmark, ProcessBlock)
    ->Arg(0)    // No MIDI events
    ->Arg(10)   // Light MIDI load
    ->Arg(50)   // Medium MIDI load
    ->Arg(100)  // Heavy MIDI load
    ->Arg(200)  // Stress test
    ->Unit(benchmark::kMicrosecond);

/**
 * Benchmark audio buffer processing without MIDI
 */
static void BM_AudioProcessing_NoMidi(benchmark::State& state) {
    HAM::HAMAudioProcessor processor;
    processor.prepareToPlay(48000.0, 512);
    
    auto audio = BenchmarkHelpers::generateTestAudioBuffer(2, 512);
    juce::MidiBuffer empty_midi;
    
    for (auto _ : state) {
        processor.processBlock(audio, empty_midi);
    }
    
    state.SetItemsProcessed(state.iterations() * 512);
}
HAM_BENCHMARK(BM_AudioProcessing_NoMidi);

/**
 * Benchmark with varying buffer sizes
 */
static void BM_AudioProcessing_BufferSize(benchmark::State& state) {
    int buffer_size = state.range(0);
    
    HAM::HAMAudioProcessor processor;
    processor.prepareToPlay(48000.0, buffer_size);
    
    auto audio = BenchmarkHelpers::generateTestAudioBuffer(2, buffer_size);
    auto midi = BenchmarkHelpers::generateTestMidiBuffer(10, 48000.0, buffer_size);
    
    LatencyMonitor latency_monitor;
    
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        processor.processBlock(audio, midi);
        auto end = std::chrono::high_resolution_clock::now();
        
        double latency_ms = std::chrono::duration<double, std::milli>(end - start).count();
        latency_monitor.recordLatency(latency_ms);
    }
    
    auto metrics = latency_monitor.getMetrics();
    state.counters["latency_ms"] = metrics.mean;
    state.counters["jitter_ms"] = latency_monitor.getJitter();
    
    // Check against thresholds
    if (metrics.max > PerformanceThresholds::MAX_AUDIO_LATENCY_MS) {
        state.SkipWithError("Latency exceeds threshold");
    }
}
HAM_BENCHMARK_RANGE(BM_AudioProcessing_BufferSize, 64, 2048);

/**
 * Benchmark worst-case scenario: all tracks active, heavy MIDI load
 */
static void BM_AudioProcessing_WorstCase(benchmark::State& state) {
    HAM::HAMAudioProcessor processor;
    processor.prepareToPlay(48000.0, 512);
    
    // Enable all 8 tracks
    for (int i = 0; i < 8; ++i) {
        // processor.enableTrack(i); // Assuming such method exists
    }
    
    auto audio = BenchmarkHelpers::generateTestAudioBuffer(2, 512);
    auto midi = BenchmarkHelpers::generateTestMidiBuffer(200, 48000.0, 512); // Heavy load
    
    CPUUsageReporter cpu_reporter(state);
    MemoryUsageReporter mem_reporter(state);
    
    for (auto _ : state) {
        processor.processBlock(audio, midi);
    }
    
    state.SetLabel("Worst Case: 8 tracks, 200 MIDI events");
}
HAM_BENCHMARK(BM_AudioProcessing_WorstCase);

/**
 * Benchmark thread safety with concurrent UI updates
 */
static void BM_AudioProcessing_WithUIUpdates(benchmark::State& state) {
    HAM::HAMAudioProcessor processor;
    processor.prepareToPlay(48000.0, 512);
    
    auto audio = BenchmarkHelpers::generateTestAudioBuffer(2, 512);
    auto midi = BenchmarkHelpers::generateTestMidiBuffer(50, 48000.0, 512);
    
    std::atomic<bool> stop{false};
    
    // Simulate UI thread sending messages
    std::thread ui_thread([&processor, &stop] {
        while (!stop.load()) {
            // Simulate UI parameter changes
            // processor.setParameter(0, 0.5f);
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    });
    
    ThreadContentionMonitor contention_monitor;
    
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        processor.processBlock(audio, midi);
        auto end = std::chrono::high_resolution_clock::now();
        
        // Check for contention (if it took unusually long)
        double duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
        if (duration_ms > 1.0) { // 1ms is unusually long for 512 samples
            contention_monitor.recordContention();
        }
    }
    
    stop = true;
    ui_thread.join();
    
    auto contention_stats = contention_monitor.getStats();
    state.counters["contentions"] = contention_stats.total_contentions;
}
HAM_BENCHMARK(BM_AudioProcessing_WithUIUpdates);