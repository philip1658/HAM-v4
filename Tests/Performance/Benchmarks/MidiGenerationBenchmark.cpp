#include <benchmark/benchmark.h>
#include "../Core/BenchmarkHelpers.h"
#include "../Core/PerformanceMetrics.h"
#include "Domain/Processors/MidiEventGenerator.h"
#include "Domain/Models/Pattern.h"
#include "Domain/Models/Track.h"
#include "Domain/Engines/GateEngine.h"
#include "Domain/Engines/PitchEngine.h"

using namespace HAM;
using namespace HAM::Performance;

/**
 * Benchmark MIDI event generation performance
 * Critical for maintaining real-time performance with multiple tracks
 */
static void BM_MidiEventGenerator_Generate(benchmark::State& state) {
    MidiEventGenerator generator;
    
    // Create test track with pattern
    Track track;
    track.setChannel(1);
    track.setEnabled(true);
    
    auto pattern = std::make_shared<Pattern>();
    pattern->setLength(state.range(0)); // Variable pattern length
    pattern->setDivision(Pattern::Division::Sixteenth);
    
    // Fill pattern with alternating gates
    for (int i = 0; i < pattern->getLength(); ++i) {
        auto& stage = pattern->getStage(i);
        stage.gate = (i % 2 == 0);
        stage.pitch = 60 + (i % 12);
        stage.velocity = 80 + (i % 48);
        stage.probability = 1.0f;
    }
    
    track.setPattern(pattern);
    
    LatencyMonitor latency_monitor;
    
    for (auto _ : state) {
        juce::MidiBuffer buffer;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // Simulate generating events for one buffer
        for (int pos = 0; pos < pattern->getLength(); ++pos) {
            generator.generateMidiEvents(track, pos, buffer, pos * 4);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        
        double latency = std::chrono::duration<double, std::milli>(end - start).count();
        latency_monitor.recordLatency(latency);
        
        benchmark::DoNotOptimize(buffer);
    }
    
    auto metrics = latency_monitor.getMetrics();
    state.counters["generation_time_ms"] = metrics.mean;
    state.counters["jitter_ms"] = latency_monitor.getJitter();
    state.counters["pattern_length"] = state.range(0);
}
BENCHMARK(BM_MidiEventGenerator_Generate)
    ->Arg(8)
    ->Arg(16)
    ->Arg(32)
    ->Arg(64)
    ->Unit(benchmark::kMicrosecond);

/**
 * Benchmark multi-track MIDI generation
 */
static void BM_MidiGeneration_MultiTrack(benchmark::State& state) {
    int num_tracks = state.range(0);
    std::vector<Track> tracks(num_tracks);
    std::vector<MidiEventGenerator> generators(num_tracks);
    
    // Setup tracks with different patterns
    for (int t = 0; t < num_tracks; ++t) {
        tracks[t].setChannel(t + 1);
        tracks[t].setEnabled(true);
        
        auto pattern = std::make_shared<Pattern>();
        pattern->setLength(16);
        pattern->setDivision(Pattern::Division::Sixteenth);
        
        // Create varied patterns
        for (int i = 0; i < 16; ++i) {
            auto& stage = pattern->getStage(i);
            stage.gate = ((i + t) % 3 != 0); // Different gate patterns
            stage.pitch = 48 + t * 2 + (i % 24);
            stage.velocity = 64 + (i * t) % 64;
        }
        
        tracks[t].setPattern(pattern);
    }
    
    for (auto _ : state) {
        juce::MidiBuffer combined_buffer;
        
        // Generate MIDI for all tracks
        for (int t = 0; t < num_tracks; ++t) {
            for (int pos = 0; pos < 16; ++pos) {
                generators[t].generateMidiEvents(tracks[t], pos, combined_buffer, pos * 4);
            }
        }
        
        benchmark::DoNotOptimize(combined_buffer);
    }
    
    state.counters["tracks"] = num_tracks;
    state.counters["events_per_track"] = 16;
    state.SetComplexityN(num_tracks);
}
BENCHMARK(BM_MidiGeneration_MultiTrack)
    ->Arg(1)
    ->Arg(4)
    ->Arg(8)
    ->Arg(16)
    ->Complexity()
    ->Unit(benchmark::kMicrosecond);

/**
 * Benchmark GateEngine processing
 */
static void BM_GateEngine_Process(benchmark::State& state) {
    GateEngine engine;
    
    std::vector<Stage> stages(state.range(0));
    for (size_t i = 0; i < stages.size(); ++i) {
        stages[i].gate = (i % 3 != 0);
        stages[i].probability = 0.8f;
        stages[i].gateLength = 0.5f;
    }
    
    for (auto _ : state) {
        for (const auto& stage : stages) {
            bool should_trigger = engine.shouldTrigger(stage);
            benchmark::DoNotOptimize(should_trigger);
            
            if (should_trigger) {
                float length = engine.calculateGateLength(stage, 120.0);
                benchmark::DoNotOptimize(length);
            }
        }
    }
    
    state.counters["stages"] = state.range(0);
}
BENCHMARK(BM_GateEngine_Process)
    ->Arg(8)
    ->Arg(16)
    ->Arg(32)
    ->Arg(64)
    ->Unit(benchmark::kNanosecond);

/**
 * Benchmark PitchEngine processing
 */
static void BM_PitchEngine_Process(benchmark::State& state) {
    PitchEngine engine;
    
    // Create a test scale
    Scale scale;
    scale.addNote(0);   // C
    scale.addNote(2);   // D
    scale.addNote(4);   // E
    scale.addNote(5);   // F
    scale.addNote(7);   // G
    scale.addNote(9);   // A
    scale.addNote(11);  // B
    
    std::vector<Stage> stages(state.range(0));
    for (size_t i = 0; i < stages.size(); ++i) {
        stages[i].pitch = 60 + (i % 24);
        stages[i].pitchBend = (i % 4 == 0) ? 0.5f : 0.0f;
        stages[i].octaveShift = (i / 12) - 1;
    }
    
    for (auto _ : state) {
        for (const auto& stage : stages) {
            int midi_note = engine.calculateMidiNote(stage, scale, 60);
            benchmark::DoNotOptimize(midi_note);
            
            if (stage.pitchBend != 0.0f) {
                int pitch_bend = engine.calculatePitchBend(stage.pitchBend);
                benchmark::DoNotOptimize(pitch_bend);
            }
        }
    }
    
    state.counters["stages"] = state.range(0);
}
BENCHMARK(BM_PitchEngine_Process)
    ->Arg(8)
    ->Arg(16)
    ->Arg(32)
    ->Arg(64)
    ->Unit(benchmark::kNanosecond);

/**
 * Benchmark MIDI event sorting and merging
 */
static void BM_MidiBuffer_Merge(benchmark::State& state) {
    int num_buffers = state.range(0);
    
    std::vector<juce::MidiBuffer> buffers(num_buffers);
    
    // Fill each buffer with events
    for (int b = 0; b < num_buffers; ++b) {
        for (int i = 0; i < 16; ++i) {
            buffers[b].addEvent(
                juce::MidiMessage::noteOn(b + 1, 60 + i, (uint8_t)(80 + i)),
                i * 32
            );
        }
    }
    
    for (auto _ : state) {
        juce::MidiBuffer merged;
        
        for (const auto& buffer : buffers) {
            merged.addEvents(buffer, 0, -1, 0);
        }
        
        benchmark::DoNotOptimize(merged);
    }
    
    state.counters["buffers"] = num_buffers;
    state.counters["events_per_buffer"] = 16;
}
BENCHMARK(BM_MidiBuffer_Merge)
    ->Arg(1)
    ->Arg(4)
    ->Arg(8)
    ->Arg(16)
    ->Unit(benchmark::kMicrosecond);

/**
 * Benchmark MIDI timing precision
 */
static void BM_MidiTiming_Precision(benchmark::State& state) {
    MidiEventGenerator generator;
    Track track;
    track.setChannel(1);
    track.setEnabled(true);
    
    auto pattern = std::make_shared<Pattern>();
    pattern->setLength(16);
    pattern->setDivision(Pattern::Division::Sixteenth);
    
    LatencyMonitor timing_monitor;
    
    const double sample_rate = 48000.0;
    const double tempo = 120.0;
    const double samples_per_tick = sample_rate / (tempo / 60.0 * 4.0); // 16th notes
    
    for (auto _ : state) {
        juce::MidiBuffer buffer;
        
        for (int i = 0; i < 16; ++i) {
            int expected_sample = static_cast<int>(i * samples_per_tick);
            
            auto start = std::chrono::high_resolution_clock::now();
            generator.generateMidiEvents(track, i, buffer, expected_sample);
            auto end = std::chrono::high_resolution_clock::now();
            
            // Check timing precision
            for (const auto metadata : buffer) {
                int actual_sample = metadata.samplePosition;
                double timing_error = std::abs(actual_sample - expected_sample) / sample_rate * 1000.0; // ms
                timing_monitor.recordLatency(timing_error);
            }
            
            buffer.clear();
        }
    }
    
    auto metrics = timing_monitor.getMetrics();
    state.counters["timing_error_mean_ms"] = metrics.mean;
    state.counters["timing_error_max_ms"] = metrics.max;
    state.counters["midi_jitter_ms"] = timing_monitor.getJitter();
    
    // Check against threshold
    if (timing_monitor.getJitter() > PerformanceThresholds::MAX_MIDI_JITTER_MS) {
        state.SkipWithError("MIDI jitter exceeds 0.1ms threshold");
    }
}
HAM_BENCHMARK(BM_MidiTiming_Precision);