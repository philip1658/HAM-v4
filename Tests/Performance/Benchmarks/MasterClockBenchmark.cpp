#include <benchmark/benchmark.h>
#include "../Core/BenchmarkHelpers.h"
#include "../Core/PerformanceMetrics.h"
#include "Domain/Clock/MasterClock.h"
#include "Domain/Clock/AsyncPatternEngine.h"

using namespace HAM;
using namespace HAM::Performance;

/**
 * Benchmark MasterClock timing accuracy and performance
 * Critical for maintaining <0.1ms jitter requirement
 */
static void BM_MasterClock_Advance(benchmark::State& state) {
    MasterClock clock;
    clock.setSampleRate(48000.0);
    clock.setTempo(120.0);
    
    int samples_per_call = state.range(0);
    
    for (auto _ : state) {
        clock.advance(samples_per_call);
    }
    
    state.SetItemsProcessed(state.iterations() * samples_per_call);
    state.counters["samples_per_sec"] = benchmark::Counter(
        state.iterations() * samples_per_call, 
        benchmark::Counter::kIsRate
    );
}
HAM_BENCHMARK_RANGE(BM_MasterClock_Advance, 1, 2048);

/**
 * Benchmark clock with multiple listeners
 */
class TestClockListener : public MasterClock::Listener {
public:
    void onClockTick(int64_t ppqPosition, double bpm) override {
        tickCount++;
        lastPpq = ppqPosition;
        lastBpm = bpm;
    }
    
    void onBarStart(int barNumber) override {
        barCount++;
    }
    
    void onTransportStateChanged(bool isPlaying) override {
        transportChanges++;
    }
    
    std::atomic<int> tickCount{0};
    std::atomic<int> barCount{0};
    std::atomic<int> transportChanges{0};
    int64_t lastPpq{0};
    double lastBpm{0};
};

static void BM_MasterClock_MultipleListeners(benchmark::State& state) {
    MasterClock clock;
    clock.setSampleRate(48000.0);
    clock.setTempo(120.0);
    
    int num_listeners = state.range(0);
    std::vector<std::unique_ptr<TestClockListener>> listeners;
    
    for (int i = 0; i < num_listeners; ++i) {
        listeners.push_back(std::make_unique<TestClockListener>());
        clock.addListener(listeners.back().get());
    }
    
    clock.start();
    
    for (auto _ : state) {
        clock.advance(512);
    }
    
    clock.stop();
    
    // Clean up listeners
    for (auto& listener : listeners) {
        clock.removeListener(listener.get());
    }
    
    state.counters["listeners"] = num_listeners;
    state.SetComplexityN(num_listeners);
}
BENCHMARK(BM_MasterClock_MultipleListeners)
    ->Arg(1)
    ->Arg(4)
    ->Arg(8)
    ->Arg(16)
    ->Arg(32)
    ->Complexity()
    ->Unit(benchmark::kMicrosecond);

/**
 * Benchmark clock sync accuracy
 */
static void BM_MasterClock_SyncAccuracy(benchmark::State& state) {
    MasterClock clock;
    clock.setSampleRate(48000.0);
    clock.setTempo(120.0);
    
    LatencyMonitor jitter_monitor;
    
    clock.start();
    
    // Measure jitter between expected and actual tick times
    auto expected_samples_per_tick = 48000.0 / (120.0 / 60.0 * 96.0); // 96 PPQ
    double accumulated_error = 0.0;
    
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        clock.advance(512);
        auto end = std::chrono::high_resolution_clock::now();
        
        double actual_ms = std::chrono::duration<double, std::milli>(end - start).count();
        double expected_ms = (512.0 / 48000.0) * 1000.0;
        double jitter = std::abs(actual_ms - expected_ms);
        
        jitter_monitor.recordLatency(jitter);
        accumulated_error += jitter;
    }
    
    auto metrics = jitter_monitor.getMetrics();
    state.counters["jitter_mean_ms"] = metrics.mean;
    state.counters["jitter_max_ms"] = metrics.max;
    state.counters["jitter_p99_ms"] = metrics.p99;
    
    // Check against threshold
    if (metrics.max > PerformanceThresholds::MAX_MIDI_JITTER_MS) {
        state.SkipWithError("Jitter exceeds 0.1ms threshold");
    }
}
HAM_BENCHMARK(BM_MasterClock_SyncAccuracy);

/**
 * Benchmark AsyncPatternEngine with clock
 */
static void BM_AsyncPatternEngine_Process(benchmark::State& state) {
    MasterClock clock;
    clock.setSampleRate(48000.0);
    clock.setTempo(120.0);
    
    AsyncPatternEngine engine(&clock);
    
    // Create test patterns
    for (int i = 0; i < state.range(0); ++i) {
        auto pattern = std::make_shared<Pattern>();
        pattern->setLength(16);
        pattern->setDivision(Pattern::Division::Sixteenth);
        
        // Add some stages
        for (int j = 0; j < 8; ++j) {
            pattern->getStage(j).gate = (j % 2 == 0);
            pattern->getStage(j).pitch = 60 + j;
        }
        
        engine.addPattern(i % 8, pattern); // Distribute across tracks
    }
    
    clock.start();
    
    for (auto _ : state) {
        engine.processPatterns(512);
        clock.advance(512);
    }
    
    clock.stop();
    
    state.counters["patterns"] = state.range(0);
}
BENCHMARK(BM_AsyncPatternEngine_Process)
    ->Arg(1)
    ->Arg(8)
    ->Arg(16)
    ->Arg(32)
    ->Unit(benchmark::kMicrosecond);

/**
 * Benchmark tempo change handling
 */
static void BM_MasterClock_TempoChange(benchmark::State& state) {
    MasterClock clock;
    clock.setSampleRate(48000.0);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> tempo_dist(60.0, 180.0);
    
    for (auto _ : state) {
        double new_tempo = tempo_dist(gen);
        clock.setTempo(new_tempo);
        clock.advance(512);
    }
    
    state.counters["tempo_changes"] = state.iterations();
}
HAM_BENCHMARK(BM_MasterClock_TempoChange);

/**
 * Benchmark clock position seeking
 */
static void BM_MasterClock_Seek(benchmark::State& state) {
    MasterClock clock;
    clock.setSampleRate(48000.0);
    clock.setTempo(120.0);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> position_dist(0, 96 * 16); // 16 bars
    
    for (auto _ : state) {
        int64_t new_position = position_dist(gen);
        clock.setPositionInPPQ(new_position);
    }
    
    state.counters["seeks"] = state.iterations();
}
HAM_BENCHMARK(BM_MasterClock_Seek);