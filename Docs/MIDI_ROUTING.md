# MIDI Routing Documentation

## 🎹 MIDI Signal Flow Overview

HAM's advanced MIDI routing system provides flexible multi-channel assignment with intelligent channel management, supporting both traditional channel-1 routing and dynamic channel allocation for complex setups.

```
┌──────────────┐     ┌──────────────┐     ┌──────────────┐
│ Master Clock │────▶│  Sequencer   │────▶│ Voice Manager│
└──────────────┘     │    Engine    │     └──────────────┘
                     └──────────────┘              │
                            │                      │
                     ┌──────▼──────┐      ┌───────▼──────┐
                     │ MIDI Router │      │ Note Events  │
                     └──────────────┘      └──────────────┘
                            │                      │
        ┌───────────────────┼───────────────────┼─┐
        │                   │                   │ │
   ┌────▼────┐        ┌────▼────┐        ┌────▼─▼──┐
   │ Track 1 │        │ Track 2 │        │ Track N │
   │ Buffer  │        │ Buffer  │        │ Buffer  │
   └────┬────┘        └────┬────┘        └────┬────┘
        │                  │                  │
   ┌────▼────┐        ┌────▼────┐        ┌────▼────┐
   │Plugin 1 │        │Plugin 2 │        │Plugin N │
   │(Chan 1) │        │(Chan 1) │        │(Chan 1) │
   └─────────┘        └─────────┘        └─────────┘
```

## 📊 Buffer Management

### Track Buffer System

Each track maintains its own MIDI buffer to ensure clean separation and prevent crosstalk:

```cpp
class MidiRouter {
private:
    // Each track gets its own buffer with flexible channel assignment
    struct TrackMidiBuffer {
        juce::MidiBuffer buffer;
        TrackId trackId;
        bool isMuted;
        bool isSolo;
        int outputChannel;  // Dynamic channel assignment (1-16)
        ChannelMode channelMode;  // Fixed, Auto, or Preferred
    };
    
    std::vector<TrackMidiBuffer> m_trackBuffers;
    
public:
    void routeEvent(const MidiEvent& event, int samplePosition) {
        // Find the target track buffer
        auto& trackBuffer = m_trackBuffers[event.trackId];
        
        // Get assigned channel for this track
        int targetChannel = m_channelManager.getChannel(event.trackId);
        
        auto message = juce::MidiMessage::noteOn(
            targetChannel,  // Dynamic channel assignment
            event.noteNumber,
            event.velocity
        );
        
        trackBuffer.buffer.addEvent(message, samplePosition);
    }
};
```

### Buffer Allocation Strategy

```cpp
// Pre-allocate buffers to avoid real-time allocation
class MidiBufferPool {
    static constexpr size_t BUFFER_SIZE = 1024;  // Events per buffer
    static constexpr size_t MAX_TRACKS = 128;
    
    struct PooledBuffer {
        std::array<MidiEvent, BUFFER_SIZE> events;
        std::atomic<int> writeIndex{0};
        std::atomic<int> readIndex{0};
    };
    
    std::array<PooledBuffer, MAX_TRACKS> m_pool;
    
public:
    // Lock-free buffer acquisition
    PooledBuffer* getBuffer(TrackId id) {
        return &m_pool[id];
    }
};
```

## 🎵 Voice Management

### Mono Mode Implementation

```cpp
class MonoVoiceHandler {
    struct MonoVoice {
        int currentNote = -1;
        int velocity = 0;
        bool isActive = false;
    };
    
    MonoVoice m_voice;
    
public:
    MidiBuffer processMono(const Stage& stage, int samplePosition) {
        MidiBuffer output;
        
        // Kill previous note if active
        if (m_voice.isActive) {
            output.addEvent(
                MidiMessage::noteOff(1, m_voice.currentNote),
                samplePosition
            );
        }
        
        // Start new note
        m_voice.currentNote = stage.pitch;
        m_voice.velocity = stage.velocity;
        m_voice.isActive = true;
        
        output.addEvent(
            MidiMessage::noteOn(1, stage.pitch, stage.velocity),
            samplePosition
        );
        
        return output;
    }
};
```

### Poly Mode Implementation

```cpp
class PolyVoiceHandler {
    static constexpr int MAX_VOICES = 16;
    
    struct Voice {
        int noteNumber = -1;
        int velocity = 0;
        int startTime = 0;
        int duration = 0;
        bool isActive = false;
    };
    
    std::array<Voice, MAX_VOICES> m_voices;
    int m_nextVoiceIndex = 0;
    
public:
    MidiBuffer processPoly(const Stage& stage, int samplePosition) {
        MidiBuffer output;
        
        // Find free voice or steal oldest
        Voice* voice = findFreeVoice();
        if (!voice) {
            voice = stealOldestVoice(output, samplePosition);
        }
        
        // Allocate new voice
        voice->noteNumber = stage.pitch;
        voice->velocity = stage.velocity;
        voice->startTime = samplePosition;
        voice->duration = calculateDuration(stage);
        voice->isActive = true;
        
        output.addEvent(
            MidiMessage::noteOn(1, stage.pitch, stage.velocity),
            samplePosition
        );
        
        return output;
    }
    
private:
    Voice* findFreeVoice() {
        for (auto& voice : m_voices) {
            if (!voice.isActive) return &voice;
        }
        return nullptr;
    }
    
    Voice* stealOldestVoice(MidiBuffer& output, int samplePosition) {
        // Find oldest active voice
        Voice* oldest = &m_voices[0];
        for (auto& voice : m_voices) {
            if (voice.isActive && voice.startTime < oldest->startTime) {
                oldest = &voice;
            }
        }
        
        // Send note off for stolen voice
        output.addEvent(
            MidiMessage::noteOff(1, oldest->noteNumber),
            samplePosition
        );
        
        return oldest;
    }
};
```

## 🔄 Ratchet Processing

### Gate Type Behaviors

```cpp
enum class GateType {
    MULTIPLE,  // Individual gate per ratchet
    HOLD,      // Single sustained gate
    SINGLE,    // Gate on first ratchet only
    REST       // No gate output
};

class RatchetProcessor {
    struct RatchetEvent {
        int ratchetIndex;
        float gateLength;
        bool shouldTrigger;
    };
    
public:
    std::vector<RatchetEvent> processRatchets(
        const Stage& stage, 
        int pulseIndex,
        int samplesPerPulse
    ) {
        std::vector<RatchetEvent> events;
        int ratchetCount = stage.ratchets[pulseIndex];
        
        if (ratchetCount == 0 || stage.gateType == GateType::REST) {
            return events;  // No output
        }
        
        int samplesPerRatchet = samplesPerPulse / ratchetCount;
        
        switch (stage.gateType) {
            case GateType::MULTIPLE:
                // Create gate for each ratchet
                for (int i = 0; i < ratchetCount; ++i) {
                    events.push_back({
                        .ratchetIndex = i,
                        .gateLength = samplesPerRatchet * stage.gate,
                        .shouldTrigger = true
                    });
                }
                break;
                
            case GateType::HOLD:
                // Single gate spanning all ratchets
                events.push_back({
                    .ratchetIndex = 0,
                    .gateLength = samplesPerPulse * stage.gate,
                    .shouldTrigger = true
                });
                // Create retriggers without note-off
                for (int i = 1; i < ratchetCount; ++i) {
                    events.push_back({
                        .ratchetIndex = i,
                        .gateLength = 0,  // No gate, just retrigger
                        .shouldTrigger = false
                    });
                }
                break;
                
            case GateType::SINGLE:
                // Only first ratchet gets gate
                events.push_back({
                    .ratchetIndex = 0,
                    .gateLength = samplesPerRatchet * stage.gate,
                    .shouldTrigger = true
                });
                break;
        }
        
        return events;
    }
};
```

### Gate Stretching

When gate stretching is enabled, ratchets are distributed evenly across the pulse duration:

```cpp
class GateStretchProcessor {
    bool m_stretchEnabled = false;
    
public:
    int calculateRatchetTiming(
        int ratchetIndex, 
        int ratchetCount,
        int samplesPerPulse,
        bool stretchEnabled
    ) {
        if (!stretchEnabled) {
            // Normal: ratchets at start of pulse
            int samplesPerRatchet = samplesPerPulse / ratchetCount;
            return ratchetIndex * samplesPerRatchet;
        } else {
            // Stretched: distributed across pulse
            float stretchFactor = (float)samplesPerPulse / (float)ratchetCount;
            return (int)(ratchetIndex * stretchFactor);
        }
    }
};
```

## 🎛️ MIDI CC Routing

### HAM Editor CC Mappings

```cpp
class CCRouter {
    struct CCMapping {
        int ccNumber;
        float minValue;
        float maxValue;
        ModulationSource source;
        TrackId targetTrack;
        int targetChannel;  // For hardware outputs
    };
    
    std::vector<CCMapping> m_mappings;
    
public:
    void processCCAutomation(
        const Stage& stage,
        MidiBuffer& buffer,
        int samplePosition
    ) {
        for (const auto& mapping : stage.ccMappings) {
            float value = calculateModulation(mapping.source, stage);
            int ccValue = (int)(value * 127.0f);
            
            auto ccMessage = MidiMessage::controllerEvent(
                1,  // Always channel 1 for plugins
                mapping.ccNumber,
                ccValue
            );
            
            buffer.addEvent(ccMessage, samplePosition);
        }
    }
};
```

## 🔍 MIDI Monitor Integration

### Debug Channel Architecture

```cpp
#ifdef DEBUG_MIDI_MONITOR
class MidiMonitor {
    static constexpr int DEBUG_CHANNEL = 16;
    
    struct MonitorEvent {
        MidiMessage expected;   // What UI says should happen
        MidiMessage actual;     // What actually was sent
        int sampleTime;
        float timingError;      // In milliseconds
    };
    
    std::vector<MonitorEvent> m_events;
    
public:
    void logEvent(
        const MidiMessage& expected,
        const MidiMessage& actual,
        int samplePosition,
        double sampleRate
    ) {
        // Duplicate to debug channel
        MidiMessage debugMsg = actual;
        debugMsg.setChannel(DEBUG_CHANNEL);
        
        // Calculate timing difference
        float expectedTime = expected.getTimeStamp();
        float actualTime = samplePosition / sampleRate * 1000.0;
        float error = actualTime - expectedTime;
        
        m_events.push_back({
            .expected = expected,
            .actual = actual,
            .sampleTime = samplePosition,
            .timingError = error
        });
        
        // Highlight timing issues
        if (std::abs(error) > 0.1) {  // > 0.1ms error
            DBG("TIMING ERROR: " << error << "ms on note " 
                << actual.getNoteNumber());
        }
    }
    
    // UI can read this for display
    const std::vector<MonitorEvent>& getEvents() const {
        return m_events;
    }
};
#endif
```

## 📈 Performance Optimizations

### Lock-free MIDI Event Queue

```cpp
template<size_t SIZE>
class LockFreeMidiQueue {
    struct Event {
        MidiMessage message;
        int samplePosition;
        std::atomic<bool> ready{false};
    };
    
    std::array<Event, SIZE> m_buffer;
    std::atomic<size_t> m_writeIndex{0};
    std::atomic<size_t> m_readIndex{0};
    
public:
    bool push(const MidiMessage& msg, int position) {
        size_t write = m_writeIndex.load(std::memory_order_acquire);
        size_t nextWrite = (write + 1) % SIZE;
        
        if (nextWrite == m_readIndex.load(std::memory_order_acquire)) {
            return false;  // Queue full
        }
        
        m_buffer[write].message = msg;
        m_buffer[write].samplePosition = position;
        m_buffer[write].ready.store(true, std::memory_order_release);
        
        m_writeIndex.store(nextWrite, std::memory_order_release);
        return true;
    }
    
    bool pop(MidiMessage& msg, int& position) {
        size_t read = m_readIndex.load(std::memory_order_acquire);
        
        if (read == m_writeIndex.load(std::memory_order_acquire)) {
            return false;  // Queue empty
        }
        
        while (!m_buffer[read].ready.load(std::memory_order_acquire)) {
            // Spin-wait for write to complete
        }
        
        msg = m_buffer[read].message;
        position = m_buffer[read].samplePosition;
        m_buffer[read].ready.store(false, std::memory_order_release);
        
        m_readIndex.store((read + 1) % SIZE, std::memory_order_release);
        return true;
    }
};
```

## 🎚️ External MIDI Configuration

### User-Configurable Routing

```cpp
class ExternalMidiConfig {
    struct RouteConfig {
        TrackId sourceTrack;
        String destinationDevice;
        int destinationChannel;  // 1-16 or -1 for auto
        ChannelMode channelMode; // Fixed, Auto, Preferred
        bool enabled;
        int priority;            // For channel allocation priority
    };
    
    std::vector<RouteConfig> m_routes;
    
public:
    void routeExternal(
        const MidiBuffer& trackBuffer,
        TrackId trackId
    ) {
        for (const auto& route : m_routes) {
            if (route.sourceTrack == trackId && route.enabled) {
                // Send to external device
                sendToDevice(
                    trackBuffer,
                    route.destinationDevice,
                    route.destinationChannel
                );
            }
        }
    }
};
```

## 🔧 MIDI Timing Precision

### Sample-Accurate Event Scheduling

```cpp
class MidiScheduler {
    struct ScheduledEvent {
        MidiMessage message;
        int64_t sampleTime;  // Absolute sample position
        TrackId track;
    };
    
    // Priority queue for scheduled events
    std::priority_queue<
        ScheduledEvent,
        std::vector<ScheduledEvent>,
        std::greater<ScheduledEvent>
    > m_eventQueue;
    
    int64_t m_currentSample = 0;
    
public:
    void scheduleEvent(
        const MidiMessage& msg,
        int samplesInFuture,
        TrackId track
    ) {
        m_eventQueue.push({
            .message = msg,
            .sampleTime = m_currentSample + samplesInFuture,
            .track = track
        });
    }
    
    void processScheduledEvents(
        MidiBuffer& output,
        int numSamples
    ) {
        int64_t endSample = m_currentSample + numSamples;
        
        while (!m_eventQueue.empty()) {
            const auto& event = m_eventQueue.top();
            
            if (event.sampleTime >= endSample) {
                break;  // Event is in future
            }
            
            int sampleOffset = event.sampleTime - m_currentSample;
            output.addEvent(event.message, sampleOffset);
            
            m_eventQueue.pop();
        }
        
        m_currentSample = endSample;
    }
};
```

## 📊 MIDI Statistics & Analysis

### Performance Metrics

```cpp
class MidiStatistics {
    struct TrackStats {
        std::atomic<int> noteOnCount{0};
        std::atomic<int> noteOffCount{0};
        std::atomic<int> ccCount{0};
        std::atomic<float> averageVelocity{0};
        std::atomic<float> timingJitter{0};
    };
    
    std::array<TrackStats, MAX_TRACKS> m_stats;
    
public:
    void updateStats(TrackId track, const MidiMessage& msg) {
        if (msg.isNoteOn()) {
            m_stats[track].noteOnCount++;
            updateAverageVelocity(track, msg.getVelocity());
        } else if (msg.isNoteOff()) {
            m_stats[track].noteOffCount++;
        } else if (msg.isController()) {
            m_stats[track].ccCount++;
        }
    }
    
    // For UI display
    String getStatsString(TrackId track) {
        auto& stats = m_stats[track];
        return String::formatted(
            "Notes: %d | Velocity: %.1f | Jitter: %.3fms",
            stats.noteOnCount.load(),
            stats.averageVelocity.load(),
            stats.timingJitter.load()
        );
    }
};
```

## 🔄 Advanced Channel Management

### Dynamic Channel Assignment

```cpp
class ChannelManager {
    enum class ChannelMode {
        Fixed,      // User-specified channel, never changes
        Auto,       // System assigns available channel
        Preferred,  // Prefer specified channel, fallback to auto
        MPE        // MPE mode with channel per voice
    };
    
    struct ChannelAssignment {
        TrackId track;
        int channel;
        ChannelMode mode;
        int priority;        // 1-10, higher = more important
        TimeStamp lastUsed;
        bool isActive;
    };
    
    std::array<ChannelAssignment, 16> m_assignments;
    std::priority_queue<ChannelRequest> m_pendingRequests;
    
public:
    int requestChannel(TrackId track, ChannelMode mode, int preferred = -1);
    void releaseChannel(TrackId track);
    void redistributeChannels();  // Optimize based on usage patterns
    bool canAssignChannel(int channel, TrackId track) const;
    void setChannelPriority(TrackId track, int priority);
};
```

### MPE (MIDI Polyphonic Expression) Support

```cpp
class MPEChannelManager {
    static constexpr int MPE_MASTER_CHANNEL = 1;
    static constexpr int MPE_VOICE_CHANNELS_START = 2;
    static constexpr int MAX_MPE_VOICES = 15;
    
    struct MPEVoice {
        int channel;        // 2-16 for voice channels
        int noteNumber;
        float pitchBend;
        float pressure;     // Channel pressure
        float timbre;       // CC74
        bool isActive;
    };
    
    std::array<MPEVoice, MAX_MPE_VOICES> m_mpeVoices;
    
public:
    int allocateMPEVoice(int noteNumber);
    void releaseMPEVoice(int channel);
    void updateMPEExpression(int channel, float pitchBend, float pressure, float timbre);
    void sendMPEConfiguration();
};
```

### Async Pattern Engine Integration

```cpp
class AsyncPatternMidiRouter {
    enum class TransitionMode {
        Immediate,    // Switch immediately
        Quantized,    // Wait for beat/bar boundary
        Crossfade,    // Gradually morph between patterns
        Layered       // Play both patterns simultaneously
    };
    
    struct PatternTransition {
        PatternId fromPattern;
        PatternId toPattern;
        TransitionMode mode;
        float crossfadeAmount;  // 0.0-1.0
        int quantizationGrid;   // 1, 4, 8, 16 beats
        bool isActive;
    };
    
    PatternTransition m_currentTransition;
    
public:
    void initiatePatternSwitch(PatternId newPattern, TransitionMode mode);
    void updateCrossfade(float amount);
    bool isTransitionComplete() const;
    void processTransition(MidiBuffer& buffer, int numSamples);
};
```

---

*This MIDI routing documentation covers the advanced multi-channel signal flow, async pattern switching, and voice management systems in HAM.*