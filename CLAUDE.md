# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

HAM (Hardware Accumulator Mode) is a MIDI sequencer inspired by Intellijel Metropolix, built with JUCE 8.0.4 and C++20. The project follows Domain-Driven Design with strict separation between business logic and technical implementation.

## Build Commands

```bash
# Quick build to Desktop (recommended)
./build.sh

# Manual debug build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j8

# Release build with optimizations
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j8

# Build with MIDI monitor (debug channel 16)
cmake .. -DDEBUG_MIDI_MONITOR=ON

# Run specific test
./build/Tests/AccumulatorEngineTests  # Example for AccumulatorEngine
./build/Tests/SequencerEngineTests    # Example for SequencerEngine

# Run all tests
ctest --test-dir build --output-on-failure
```

## Architecture Overview

### Domain-Driven Design Layers

1. **Domain Layer** (`Source/Domain/`)
   - Pure business logic with no JUCE dependencies
   - Key engines coordinate through message passing:
     - `SequencerEngine`: Coordinates pattern playback, delegates to processors
     - `MasterClock`: Sample-accurate 24 PPQN timing, notifies listeners
     - `VoiceManager`: Lock-free voice allocation with mono/poly modes
     - `AccumulatorEngine`: Pitch transposition with PENDULUM mode
     - `PitchEngine`: Scale quantization and pitch processing
     - `GateEngine`: Gate types (MULTIPLE/HOLD/SINGLE/REST) and ratchets

2. **Infrastructure Layer** (`Source/Infrastructure/`)
   - Technical implementations (future: AudioProcessor, PluginHost)
   - JUCE-specific code isolated here

3. **Presentation Layer** (`Source/UI/` - not yet implemented)
   - Future UI components will be completely swappable
   - ParameterBridge for thread-safe UI↔Engine communication

### Critical Design Patterns

#### SequencerEngine Refactoring (Important!)
The SequencerEngine was refactored to delegate responsibilities:
```cpp
// Instead of monolithic SequencerEngine handling everything:
SequencerEngine
  ├── TrackProcessor      // Track states, stage advancement, directions
  ├── MidiEventGenerator  // MIDI event creation, ratchets, velocity
  └── PatternScheduler    // Pattern transitions, chaining, morphing
```

#### Voice Mode Behavior (CRITICAL)
```cpp
// MONO MODE: Plays ALL pulses before advancing
if (voiceMode == VoiceMode::MONO) {
    // Stage 1: [pulse1, pulse2, pulse3] -> complete -> advance
    // Notes cut previous notes immediately
}

// POLY MODE: Advances after 1 pulse
if (voiceMode == VoiceMode::POLY) {
    // Stage 1: [pulse1] -> advance -> Stage 2: [pulse1] -> advance
    // Notes can overlap (up to 64 voices)
}
```

#### Lock-Free Audio Processing
```cpp
// ALWAYS in audio thread:
- No new/delete or malloc
- No std::mutex or locks  
- No string operations
- Use std::atomic for parameters
- Pre-allocated buffers
- juce::AbstractFifo for queues
```

## Current Development Status (ROADMAP)

- **Phase 1**: Foundation ✅ Complete
- **Phase 2**: Core Audio ✅ Complete (SequencerEngine, VoiceManager, MidiRouter, ChannelManager)
- **Phase 3**: Advanced Engines (In Progress)
  - 3.1 GateEngine ✅ Complete
  - 3.2 PitchEngine ✅ Complete  
  - 3.3 AccumulatorEngine ✅ Complete (100% test coverage)
  - 3.4 Pattern Morphing ❌ Not started
- **Phase 4**: Infrastructure ❌ Not started (AudioProcessor needed for UI connection)
- **Phase 5**: Basic UI ❌ Not started

## Testing Strategy

```bash
# Test naming convention
Tests/[EngineName]Tests.cpp

# Each engine has comprehensive tests
# AccumulatorEngine has 100% coverage as reference implementation

# Performance requirements:
- MIDI jitter < 0.1ms
- CPU < 5% with 1 track @ 48kHz
- 64 voice polyphony without dropouts
```

## Key Technical Decisions

1. **24 PPQN Clock Resolution**: Chosen for compatibility with hardware sequencers
2. **64 Voice Polyphony**: Increased from 16 for richer arrangements
3. **Lock-Free Design**: All audio processing uses atomics and lock-free queues
4. **Domain-First Development**: Business logic implemented and tested before UI
5. **Processor Delegation Pattern**: SequencerEngine delegates to specialized processors

## Common Development Tasks

### Adding a New Engine
1. Create interface in `Source/Domain/Engines/`
2. Write comprehensive tests in `Tests/[EngineName]Tests.cpp`
3. Implement with lock-free constraints
4. Integrate with SequencerEngine if needed

### Modifying Voice Behavior
- Check `VoiceManager::processVoiceMode()` for MONO/POLY logic
- Update `SequencerEngine::processTrack()` for stage advancement
- Test with `VoiceManagerTests` and `SequencerEngineTests`

### Working with MasterClock
- Clock is the timing source for everything
- Use `MasterClock::Listener` interface for notifications
- Never block in clock callbacks

## Performance Considerations

- Pre-allocate all buffers (see `m_midiEventBuffer.resize(1024)`)
- Use `juce::AbstractFifo` for thread-safe queues
- Profile with Instruments.app: `instruments -t "Time Profiler" ./HAM.app`
- Check allocations: `instruments -t "Allocations" ./HAM.app`

## JUCE-Specific Guidelines

Always use JUCE types where available:
- `juce::MidiMessage` instead of raw MIDI
- `juce::AudioBuffer` for audio processing
- `jassert()` instead of assert()
- `DBG()` instead of std::cout
- `juce::ValueTree` for state management

## Git Workflow

```bash
# Commit message format
git commit -m "feat: implement AccumulatorEngine with PENDULUM mode"
git commit -m "fix: resolve voice stealing in MONO mode"
git commit -m "test: add 100% coverage for PitchEngine"
git commit -m "refactor: extract TrackProcessor from SequencerEngine"
```

## Debug Tips

- Enable MIDI monitor: `cmake .. -DDEBUG_MIDI_MONITOR=ON`
- Check thread safety with Thread Sanitizer
- Use `DBG()` liberally - it's removed in Release builds
- Voice issues? Check `VoiceManager::getDebugString()`
- Timing issues? Enable `MasterClock::setDebugMode(true)`