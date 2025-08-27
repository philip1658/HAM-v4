#include <benchmark/benchmark.h>
#include "../Core/BenchmarkHelpers.h"
#include "Domain/Models/Pattern.h"
#include "Domain/Processors/PatternScheduler.h"
#include "Domain/Processors/TrackProcessor.h"

using namespace HAM;
using namespace HAM::Performance;

/**
 * Benchmark pattern processing and scheduling
 */
static void BM_Pattern_StageAccess(benchmark::State& state) {
    Pattern pattern;
    pattern.setLength(state.range(0));
    
    // Initialize stages
    for (int i = 0; i < pattern.getLength(); ++i) {
        auto& stage = pattern.getStage(i);
        stage.gate = (i % 2 == 0);
        stage.pitch = 60 + (i % 12);
        stage.velocity = 64 + (i % 64);
    }
    
    for (auto _ : state) {
        // Simulate pattern playback
        for (int i = 0; i < pattern.getLength(); ++i) {
            const auto& stage = pattern.getStage(i);
            benchmark::DoNotOptimize(stage.gate);
            benchmark::DoNotOptimize(stage.pitch);
            benchmark::DoNotOptimize(stage.velocity);
        }
    }
    
    state.counters["stages"] = pattern.getLength();
    state.SetItemsProcessed(state.iterations() * pattern.getLength());
}
BENCHMARK(BM_Pattern_StageAccess)
    ->Arg(8)
    ->Arg(16)
    ->Arg(32)
    ->Arg(64)
    ->Unit(benchmark::kNanosecond);

/**
 * Benchmark pattern scheduler with multiple patterns
 */
static void BM_PatternScheduler_Schedule(benchmark::State& state) {
    PatternScheduler scheduler;
    int num_patterns = state.range(0);
    
    // Create patterns with different lengths and divisions
    for (int i = 0; i < num_patterns; ++i) {
        auto pattern = std::make_shared<Pattern>();
        pattern->setLength(8 + (i % 56)); // Varied lengths
        pattern->setDivision(static_cast<Pattern::Division>(i % 8));
        
        scheduler.addPattern(i % 8, pattern); // Distribute across tracks
    }
    
    for (auto _ : state) {
        // Simulate scheduling for one bar
        for (int tick = 0; tick < 96; ++tick) { // 96 PPQ
            auto scheduled = scheduler.getActivePatterns(tick);
            benchmark::DoNotOptimize(scheduled);
        }
    }
    
    state.counters["patterns"] = num_patterns;
}
BENCHMARK(BM_PatternScheduler_Schedule)
    ->Arg(1)
    ->Arg(8)
    ->Arg(16)
    ->Arg(32)
    ->Unit(benchmark::kMicrosecond);

/**
 * Benchmark track processor
 */
static void BM_TrackProcessor_Process(benchmark::State& state) {
    TrackProcessor processor;
    
    Track track;
    track.setEnabled(true);
    track.setChannel(1);
    
    auto pattern = std::make_shared<Pattern>();
    pattern->setLength(16);
    pattern->setDivision(Pattern::Division::Sixteenth);
    
    // Setup pattern
    for (int i = 0; i < 16; ++i) {
        auto& stage = pattern->getStage(i);
        stage.gate = (i % 3 != 0);
        stage.pitch = 60 + (i % 12);
        stage.velocity = 80;
        stage.probability = 0.9f;
    }
    
    track.setPattern(pattern);
    
    for (auto _ : state) {
        juce::MidiBuffer buffer;
        
        // Process one pattern cycle
        for (int step = 0; step < 16; ++step) {
            processor.processStep(track, step, buffer, step * 30);
        }
        
        benchmark::DoNotOptimize(buffer);
    }
    
    state.SetItemsProcessed(state.iterations() * 16);
}
HAM_BENCHMARK(BM_TrackProcessor_Process);

/**
 * Benchmark pattern mutation operations
 */
static void BM_Pattern_Mutation(benchmark::State& state) {
    Pattern pattern;
    pattern.setLength(16);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> stage_dist(0, 15);
    std::uniform_int_distribution<> pitch_dist(36, 84);
    std::uniform_real_distribution<float> prob_dist(0.0f, 1.0f);
    
    for (auto _ : state) {
        // Simulate pattern editing
        int stage_idx = stage_dist(gen);
        auto& stage = pattern.getStage(stage_idx);
        
        state.PauseTiming();
        int new_pitch = pitch_dist(gen);
        float new_prob = prob_dist(gen);
        state.ResumeTiming();
        
        stage.pitch = new_pitch;
        stage.probability = new_prob;
        stage.gate = !stage.gate;
    }
}
HAM_BENCHMARK(BM_Pattern_Mutation);

/**
 * Benchmark pattern chain processing
 */
static void BM_Pattern_Chain(benchmark::State& state) {
    int chain_length = state.range(0);
    std::vector<std::shared_ptr<Pattern>> chain;
    
    for (int i = 0; i < chain_length; ++i) {
        auto pattern = std::make_shared<Pattern>();
        pattern->setLength(16);
        pattern->setDivision(Pattern::Division::Sixteenth);
        
        // Each pattern has different content
        for (int j = 0; j < 16; ++j) {
            auto& stage = pattern->getStage(j);
            stage.gate = ((i + j) % 3 != 0);
            stage.pitch = 48 + i + j;
        }
        
        chain.push_back(pattern);
    }
    
    for (auto _ : state) {
        // Process entire chain
        for (const auto& pattern : chain) {
            for (int step = 0; step < pattern->getLength(); ++step) {
                const auto& stage = pattern->getStage(step);
                benchmark::DoNotOptimize(stage);
            }
        }
    }
    
    state.counters["chain_length"] = chain_length;
    state.counters["total_stages"] = chain_length * 16;
}
BENCHMARK(BM_Pattern_Chain)
    ->Arg(1)
    ->Arg(4)
    ->Arg(8)
    ->Arg(16)
    ->Unit(benchmark::kMicrosecond);

/**
 * Benchmark pattern probability calculations
 */
static void BM_Pattern_Probability(benchmark::State& state) {
    Pattern pattern;
    pattern.setLength(64);
    
    // Setup stages with varied probabilities
    for (int i = 0; i < 64; ++i) {
        auto& stage = pattern.getStage(i);
        stage.probability = static_cast<float>(i) / 64.0f;
        stage.gate = true;
    }
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    
    for (auto _ : state) {
        int triggered = 0;
        
        for (int i = 0; i < 64; ++i) {
            float rand = dist(gen);
            const auto& stage = pattern.getStage(i);
            
            if (stage.gate && rand <= stage.probability) {
                triggered++;
            }
        }
        
        benchmark::DoNotOptimize(triggered);
    }
    
    state.SetItemsProcessed(state.iterations() * 64);
}
HAM_BENCHMARK(BM_Pattern_Probability);