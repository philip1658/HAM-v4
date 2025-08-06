# HAM Architecture Documentation

## 🏛️ System Architecture Overview

HAM follows Domain-Driven Design (DDD) principles with a clean, layered architecture that separates business logic from technical implementation details.

```
┌─────────────────────────────────────────────────────┐
│                  Presentation Layer                  │
│         (UI Components, Views, ViewModels)          │
├─────────────────────────────────────────────────────┤
│                 Application Layer                    │
│         (Commands, Use Cases, Services)             │
├─────────────────────────────────────────────────────┤
│                    Domain Layer                      │
│        (Models, Engines, Business Logic)            │
├─────────────────────────────────────────────────────┤
│                Infrastructure Layer                  │
│      (JUCE, Audio, MIDI, Persistence, Debug)       │
└─────────────────────────────────────────────────────┘
```

## 📁 Layer Descriptions

### Domain Layer (Core Business Logic)

The heart of HAM - pure C++ classes with no framework dependencies.

#### Models
```cpp
// Domain/Models/Track.h
class Track {
    TrackId m_id;
    VoiceMode m_voiceMode;      // Mono or Poly
    int m_division;              // Clock division (1-64)
    float m_swing;               // -50% to +50%
    int m_octaveOffset;          // -4 to +4
    float m_pulseLength;         // 0.0 to 1.0
    int m_midiChannel;           // Dynamic MIDI channel assignment
    std::array<Stage, 8> m_stages;
    AccumulatorState m_accumulator;
    ScaleDefinition m_scale;
    PatternSnapshot m_currentSnapshot;
    
public:
    // Business logic methods
    void advanceToNextStage();
    MidiEvent generateCurrentEvent();
    void applyAccumulator();
};

// Domain/Models/Stage.h
class Stage {
    int m_pitch;                 // MIDI note number
    float m_gate;                // 0.0 to 1.0
    int m_velocity;              // 0 to 127
    int m_pulseCount;            // 1 to 8
    std::array<int, 8> m_ratchets; // Ratchet counts per pulse
    GateType m_gateType;         // MULTIPLE, HOLD, SINGLE, REST
    
    // HAM Editor parameters
    ModulationSettings m_modulation;
    std::vector<CCMapping> m_ccMappings;
    float m_probability;         // 0% to 100%
};

// Domain/Models/Snapshot.h
class PatternSnapshot {
    SnapshotId m_id;
    String m_name;
    TimeStamp m_timestamp;
    std::vector<Stage> m_stages;
    TrackSettings m_trackSettings;
    float m_morphWeight;         // 0.0-1.0 for morphing
    
public:
    void morphTo(const PatternSnapshot& target, float amount);
    PatternSnapshot interpolate(const PatternSnapshot& other, float t);
};
```

#### Engines

Core processing engines that handle the sequencer logic:

```cpp
// Domain/Engines/VoiceManager.h
class VoiceManager {
    enum class VoiceMode { Mono, Poly };
    static constexpr int MAX_VOICES = 16;
    
    struct Voice {
        int noteNumber;
        float velocity;
        int startSample;
        int remainingSamples;
        bool isActive;
        TrackId trackId;
    };
    
    std::array<Voice, MAX_VOICES> m_voices;
    VoiceMode m_mode;
    
public:
    // Voice allocation and management
    Voice* allocateVoice(int note, float velocity);
    void releaseVoice(Voice* voice);
    void processVoices(int numSamples);
    void stealOldestVoice();  // For poly mode overflow
};

// Domain/Engines/MasterClock.h
class MasterClock {
    static constexpr int PPQN = 24;  // Pulses per quarter note
    
    double m_sampleRate;
    double m_bpm;
    int m_samplesPerPulse;
    int m_currentSample;
    std::atomic<bool> m_isRunning;
    
public:
    // Sample-accurate timing
    void processBlock(int numSamples);
    bool shouldTriggerPulse(int sampleOffset);
    double getPhase() const;
    void setSyncSource(SyncSource source);
};

// Domain/Engines/AccumulatorEngine.h
class AccumulatorEngine {
    enum class Mode { PerStage, PerPulse, PerRatchet };
    enum class ResetMode { Manual, StageLoop, PatternChange };
    
    int m_currentOffset;         // In scale degrees
    int m_stepSize;              // How much to accumulate
    Mode m_mode;
    ResetMode m_resetMode;
    
public:
    int calculatePitchOffset(const Stage& stage, int pulseIndex);
    void reset();
    void step();
};

// Domain/Engines/AsyncPatternEngine.h
class AsyncPatternEngine {
    enum class SwitchMode { Immediate, Quantized, Manual };
    
    PatternId m_currentPattern;
    PatternId m_nextPattern;
    PatternSnapshot m_patternMemory[64];  // Scene slots
    SwitchMode m_switchMode;
    int m_quantizationGrid;      // 1, 4, 8, 16 beats
    
public:
    void queuePatternSwitch(PatternId pattern, SwitchMode mode);
    void recallSnapshot(int slot);
    void saveSnapshot(int slot, const String& name);
    void morphToSnapshot(int slot, float amount, bool quantized);
    bool isPatternSwitchPending() const;
};
```

### Application Layer (Use Cases & Commands)

Orchestrates domain objects and implements business rules.

```cpp
// Application/Commands/ModifyStageCommand.h
class ModifyStageCommand : public Command {
    TrackId m_trackId;
    int m_stageIndex;
    Stage m_oldState;
    Stage m_newState;
    
public:
    void execute() override;
    void undo() override;
    bool canMergeWith(const Command* other) override;
};

// Application/Services/ProjectService.h
class ProjectService {
    std::unique_ptr<Project> m_currentProject;
    CommandHistory m_history;
    
public:
    // High-level operations
    void createNewProject();
    void loadProject(const File& file);
    void saveProject();
    void addTrack(const TrackConfig& config);
    void modifyStage(TrackId track, int stage, const StageData& data);
};
```

### Infrastructure Layer (Technical Implementation)

Handles all framework-specific and I/O operations.

```cpp
// Infrastructure/Audio/AudioEngine.cpp
class AudioEngine : public juce::AudioProcessor {
    MasterClock m_clock;
    SequencerEngine m_sequencer;
    MidiRouter m_midiRouter;
    
    // Lock-free communication with UI
    juce::AbstractFifo m_messageQueue;
    std::array<Message, 1024> m_messages;
    
public:
    void processBlock(juce::AudioBuffer<float>& buffer,
                     juce::MidiBuffer& midiMessages) override {
        // Real-time safe processing
        const int numSamples = buffer.getNumSamples();
        
        // Process clock
        m_clock.processBlock(numSamples);
        
        // Generate MIDI events
        for (int sample = 0; sample < numSamples; ++sample) {
            if (m_clock.shouldTriggerPulse(sample)) {
                auto events = m_sequencer.processNextPulse();
                m_midiRouter.routeEvents(events, midiMessages, sample);
            }
        }
        
        // Process UI messages (lock-free)
        processMessageQueue();
    }
};
```

### Presentation Layer (UI Components)

Completely swappable UI layer using MVVM pattern.

```cpp
// Presentation/ViewModels/TrackViewModel.h
class TrackViewModel : public juce::ChangeBroadcaster {
    Track* m_track;  // Domain model reference
    ProjectService* m_projectService;
    
    // UI-specific state
    bool m_isSelected;
    bool m_isMuted;
    bool m_isSolo;
    
public:
    // UI operations that translate to domain commands
    void setPitch(int stageIndex, int pitch) {
        m_projectService->modifyStage(
            m_track->getId(), 
            stageIndex, 
            StageData{.pitch = pitch}
        );
        sendChangeMessage();
    }
};

// Presentation/Components/StageCard.cpp
class StageCard : public juce::Component {
    std::unique_ptr<juce::Slider> m_pitchSlider;
    std::unique_ptr<juce::Slider> m_gateSlider;
    std::unique_ptr<juce::Slider> m_velocitySlider;
    std::unique_ptr<juce::TextButton> m_hamButton;
    
    TrackViewModel* m_viewModel;
    int m_stageIndex;
    
public:
    void sliderValueChanged(juce::Slider* slider) override {
        if (slider == m_pitchSlider.get()) {
            m_viewModel->setPitch(m_stageIndex, slider->getValue());
        }
    }
};
```

## 🔄 Data Flow

### UI → Engine Communication (Lock-free)

```cpp
// Message structure for lock-free queue
struct Message {
    enum Type { 
        SetPitch, SetGate, SetVelocity, 
        ChangePattern, UpdateTempo 
    };
    
    Type type;
    TrackId trackId;
    int stageIndex;
    float value;
};

// In UI thread
void StageCard::sliderValueChanged(Slider* slider) {
    Message msg{
        .type = Message::SetPitch,
        .trackId = m_trackId,
        .stageIndex = m_stageIndex,
        .value = slider->getValue()
    };
    
    audioEngine->pushMessage(msg);  // Lock-free push
}

// In audio thread
void AudioEngine::processMessageQueue() {
    Message msg;
    while (m_messageQueue.pop(msg)) {
        // Apply changes atomically
        switch (msg.type) {
            case Message::SetPitch:
                m_sequencer.setPitch(msg.trackId, msg.stageIndex, msg.value);
                break;
            // ... other message types
        }
    }
}
```

### Engine → UI Updates

```cpp
// Using atomic values for real-time feedback
class SequencerState {
    std::atomic<int> m_currentStage[MAX_TRACKS];
    std::atomic<float> m_clockPhase;
    std::atomic<bool> m_gateStates[MAX_TRACKS][8];
    
public:
    // Called from audio thread
    void updateStage(TrackId track, int stage) {
        m_currentStage[track].store(stage, std::memory_order_release);
    }
    
    // Called from UI thread (30 FPS timer)
    int getCurrentStage(TrackId track) const {
        return m_currentStage[track].load(std::memory_order_acquire);
    }
};
```

## 🎯 Design Patterns Used

### 1. Command Pattern
- All user actions are commands
- Enables undo/redo functionality
- Provides action history

### 2. Observer Pattern
- ViewModels observe domain models
- UI components observe ViewModels
- Loose coupling between layers

### 3. Repository Pattern
- Abstract persistence details
- Swap between JSON/Binary/Database

### 4. Factory Pattern
- Create domain objects consistently
- Handle complex initialization

### 5. Strategy Pattern
- Different gate types
- Various accumulator modes
- Pluggable voice allocation

### 6. Memento Pattern
- Pattern snapshots for undo/redo
- Scene memory slots
- State preservation across sessions

### 7. State Machine Pattern
- Async pattern switching logic
- Scene transition management
- Morphing state interpolation

## 🔌 Plugin Architecture

### VST3/AU Hosting

```cpp
// Infrastructure/PluginHost.h
class PluginHost {
    struct PluginSlot {
        std::unique_ptr<juce::AudioPluginInstance> instance;
        juce::AudioBuffer<float> audioBuffer;
        juce::MidiBuffer midiBuffer;
        TrackId associatedTrack;
        int assignedChannel;         // Dynamic channel assignment
    };
    
    std::vector<PluginSlot> m_plugins;
    juce::AudioPluginFormatManager m_formatManager;
    ChannelManager m_channelManager;
    
public:
    void loadPlugin(const String& path, TrackId track, int preferredChannel = -1);
    void processPlugin(PluginSlot& slot, int numSamples);
    void showPluginUI(PluginSlot& slot);
    int assignMidiChannel(TrackId track);
};

// Domain/Services/ChannelManager.h
class ChannelManager {
    std::array<TrackId, 16> m_channelAssignments;  // MIDI channels 1-16
    std::set<int> m_availableChannels;
    
public:
    int assignChannel(TrackId track, int preferredChannel = -1);
    void releaseChannel(int channel);
    int getChannel(TrackId track) const;
    bool isChannelAvailable(int channel) const;
    void redistributeChannels();  // Optimize channel usage
};
```

## 🧪 Testing Strategy

### Unit Tests (Domain Layer)
```cpp
TEST_CASE("VoiceManager allocates voices correctly") {
    VoiceManager manager(VoiceMode::Poly);
    
    auto* voice1 = manager.allocateVoice(60, 100);
    REQUIRE(voice1 != nullptr);
    REQUIRE(voice1->noteNumber == 60);
    
    // Test voice stealing
    for (int i = 0; i < 16; ++i) {
        manager.allocateVoice(60 + i, 100);
    }
    
    auto* voice17 = manager.allocateVoice(77, 100);
    REQUIRE(voice17 != nullptr);  // Should steal oldest
}
```

### Integration Tests (Application Layer)
```cpp
TEST_CASE("ProjectService saves and loads correctly") {
    ProjectService service;
    service.createNewProject();
    service.addTrack(TrackConfig{.name = "Lead"});
    
    File testFile("test_project.ham");
    service.saveProject(testFile);
    
    ProjectService loadedService;
    loadedService.loadProject(testFile);
    
    REQUIRE(loadedService.getTrackCount() == 1);
}
```

## 🔐 Thread Safety

### Audio Thread Rules
1. **No allocations**: All buffers pre-allocated
2. **No locks**: Use lock-free data structures
3. **No blocking**: All operations O(1) or bounded
4. **Atomic operations**: For parameter updates

### Safe Communication Patterns
```cpp
// Good: Lock-free single producer, single consumer
juce::AbstractFifo fifo(1024);
std::array<Message, 1024> buffer;

// Good: Atomic parameter updates
std::atomic<float> m_tempo{120.0f};

// Good: Triple buffering for complex state
class TripleBuffer<PatternData> {
    std::array<PatternData, 3> buffers;
    std::atomic<int> readIndex{0};
    std::atomic<int> writeIndex{1};
    int processingIndex{2};
};

// Bad: Mutex in audio thread
std::mutex m_mutex;  // NEVER in processBlock!

// Bad: Dynamic allocation
std::vector<Note> notes;  // Pre-allocate instead!
```

## 📊 Performance Considerations

### Memory Layout
```cpp
// Good: Cache-friendly layout
struct alignas(64) TrackData {  // Cache line aligned
    // Hot data together
    int currentStage;
    float currentPhase;
    bool isPlaying;
    
    // Padding to prevent false sharing
    char padding[40];
    
    // Cold data
    std::string name;
    Color trackColor;
};
```

### SIMD Optimization Opportunities
```cpp
// Future optimization for ratchet processing
void processRatchets(float* gates, int count) {
    // Current: scalar processing
    for (int i = 0; i < count; ++i) {
        gates[i] *= 0.5f;
    }
    
    // Future: SIMD with JUCE
    // juce::FloatVectorOperations::multiply(gates, 0.5f, count);
}
```

## 🚀 Future Architecture Considerations

### Planned Enhancements
1. **Plugin Sandboxing**: Separate process for plugin hosting
2. **Distributed Processing**: Multi-core sequencer engine
3. **Network Sync**: Ableton Link integration
4. **Modular UI**: Hot-swappable UI themes/layouts
5. **Advanced Pattern Engine**: AI-assisted pattern generation
6. **Cloud Integration**: Pattern sharing and collaborative editing

### Scalability Path
```
Current: Async Pattern Engine with Multi-Channel MIDI
    ↓
Phase 1: Separate UI/Engine processes with CI/CD
    ↓
Phase 2: Distributed track processing
    ↓
Phase 3: Cloud preset/pattern sharing with automated testing
    ↓
Phase 4: AI-enhanced pattern morphing and generation
```

### Testing Architecture
```
Local Development
    ↓
Automated Unit Tests
    ↓
Integration Tests with Screenshot Validation
    ↓
CI/CD Pipeline with Log Parsing
    ↓
Manual Approval Gates
    ↓
Production Release
```

## 🤖 Automated Testing Integration

### CI/CD Architecture

```cpp
// Infrastructure/Testing/ScreenshotValidator.h
class ScreenshotValidator {
    struct ValidationResult {
        bool passed;
        float similarity;
        String failureReason;
        Image differenceImage;
    };
    
public:
    ValidationResult validateUI(const Image& screenshot, const String& testName);
    void updateBaseline(const Image& newBaseline, const String& testName);
    bool pixelPerfectMatch(const Image& expected, const Image& actual);
};

// Infrastructure/Testing/LogParser.h
class LogParser {
    enum class LogLevel { Debug, Info, Warning, Error, Critical };
    
    struct LogEntry {
        TimeStamp timestamp;
        LogLevel level;
        String component;
        String message;
        std::map<String, String> metadata;
    };
    
public:
    std::vector<LogEntry> parseLogFile(const File& logFile);
    bool detectPerformanceRegression(const std::vector<LogEntry>& logs);
    bool validateExpectedBehavior(const std::vector<LogEntry>& logs, const String& testCase);
};
```

---

*This architecture document is a living document and will be updated as HAM evolves.*