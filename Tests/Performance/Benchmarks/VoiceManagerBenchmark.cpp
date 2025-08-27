#include <benchmark/benchmark.h>
#include "../Core/BenchmarkHelpers.h"
#include "Domain/Engines/VoiceManager.h"

using namespace HAM;
using namespace HAM::Performance;

/**
 * Benchmark voice allocation and management
 */
static void BM_VoiceManager_Allocation(benchmark::State& state) {
    VoiceManager manager;
    manager.setMode(VoiceManager::Mode::Poly);
    manager.setMaxVoices(state.range(0));
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> note_dist(36, 84);
    std::uniform_int_distribution<> vel_dist(1, 127);
    
    for (auto _ : state) {
        // Simulate note on/off sequence
        for (int i = 0; i < 32; ++i) {
            int note = note_dist(gen);
            int velocity = vel_dist(gen);
            
            if (i % 3 == 0) {
                // Note on
                int voice = manager.allocateVoice(note, velocity);
                benchmark::DoNotOptimize(voice);
            } else if (i % 3 == 1) {
                // Note off
                manager.releaseVoice(note);
            }
        }
        
        // Reset for next iteration
        manager.reset();
    }
    
    state.counters["max_voices"] = state.range(0);
}
BENCHMARK(BM_VoiceManager_Allocation)
    ->Arg(1)   // Mono
    ->Arg(4)   // Limited poly
    ->Arg(8)   // Standard poly
    ->Arg(16)  // Full poly
    ->Unit(benchmark::kMicrosecond);

/**
 * Benchmark voice stealing algorithm
 */
static void BM_VoiceManager_VoiceStealing(benchmark::State& state) {
    VoiceManager manager;
    manager.setMode(VoiceManager::Mode::Poly);
    manager.setMaxVoices(8); // Limited voices to force stealing
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> note_dist(36, 84);
    
    for (auto _ : state) {
        // Fill all voices
        for (int i = 0; i < 16; ++i) { // More notes than voices
            int note = note_dist(gen);
            int voice = manager.allocateVoice(note, 80);
            benchmark::DoNotOptimize(voice);
        }
        
        manager.reset();
    }
    
    state.counters["notes_played"] = 16;
    state.counters["available_voices"] = 8;
}
HAM_BENCHMARK(BM_VoiceManager_VoiceStealing);

/**
 * Benchmark different voice modes
 */
static void BM_VoiceManager_Modes(benchmark::State& state) {
    VoiceManager manager;
    auto mode = static_cast<VoiceManager::Mode>(state.range(0));
    manager.setMode(mode);
    
    const char* mode_names[] = {"Mono", "Poly", "Unison", "Chord"};
    state.SetLabel(mode_names[state.range(0)]);
    
    for (auto _ : state) {
        // Typical note sequence
        manager.allocateVoice(60, 100);
        manager.allocateVoice(64, 100);
        manager.allocateVoice(67, 100);
        
        manager.releaseVoice(60);
        manager.allocateVoice(62, 80);
        
        manager.releaseVoice(64);
        manager.releaseVoice(67);
        manager.releaseVoice(62);
        
        manager.reset();
    }
}
BENCHMARK(BM_VoiceManager_Modes)
    ->Arg(0)  // Mono
    ->Arg(1)  // Poly
    ->Arg(2)  // Unison
    ->Arg(3)  // Chord
    ->Unit(benchmark::kNanosecond);

/**
 * Benchmark voice parameter updates
 */
static void BM_VoiceManager_ParameterUpdate(benchmark::State& state) {
    VoiceManager manager;
    manager.setMode(VoiceManager::Mode::Poly);
    manager.setMaxVoices(16);
    
    // Allocate some voices
    std::vector<int> voices;
    for (int i = 0; i < 8; ++i) {
        voices.push_back(manager.allocateVoice(60 + i, 100));
    }
    
    for (auto _ : state) {
        // Update voice parameters
        for (int voice : voices) {
            manager.setVoicePitchBend(voice, 0.5f);
            manager.setVoiceModulation(voice, 0.7f);
            manager.setVoicePanning(voice, 0.0f);
        }
    }
    
    state.counters["active_voices"] = voices.size();
}
HAM_BENCHMARK(BM_VoiceManager_ParameterUpdate);

/**
 * Benchmark getActiveVoices query
 */
static void BM_VoiceManager_GetActiveVoices(benchmark::State& state) {
    VoiceManager manager;
    manager.setMode(VoiceManager::Mode::Poly);
    manager.setMaxVoices(16);
    
    // Allocate voices
    int num_voices = state.range(0);
    for (int i = 0; i < num_voices; ++i) {
        manager.allocateVoice(48 + i, 80);
    }
    
    for (auto _ : state) {
        auto active = manager.getActiveVoices();
        benchmark::DoNotOptimize(active);
    }
    
    state.counters["voices"] = num_voices;
}
BENCHMARK(BM_VoiceManager_GetActiveVoices)
    ->Arg(0)
    ->Arg(4)
    ->Arg(8)
    ->Arg(16)
    ->Unit(benchmark::kNanosecond);

/**
 * Benchmark voice priority handling
 */
static void BM_VoiceManager_Priority(benchmark::State& state) {
    VoiceManager manager;
    manager.setMode(VoiceManager::Mode::Poly);
    manager.setMaxVoices(4); // Limited voices
    
    for (auto _ : state) {
        // High priority notes
        manager.allocateVoice(60, 127, 1.0f); // Max priority
        manager.allocateVoice(64, 100, 0.8f);
        manager.allocateVoice(67, 80, 0.6f);
        manager.allocateVoice(72, 60, 0.4f);
        
        // Low priority note should steal lowest priority voice
        int stolen = manager.allocateVoice(48, 127, 0.9f);
        benchmark::DoNotOptimize(stolen);
        
        manager.reset();
    }
}
HAM_BENCHMARK(BM_VoiceManager_Priority);