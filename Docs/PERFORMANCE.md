# Performance Optimization Guide

## 🎯 Performance Targets

### Hardware Target
- **Platform**: Apple Silicon M3 Max (128GB RAM)
- **CPU Usage**: < 5% at 48kHz with loaded plugin
- **Multi-Channel CPU**: < 15% with 16 active MIDI channels
- **Memory**: < 200MB base, < 500MB with plugins
- **Snapshot Memory**: < 50MB for 64 pattern snapshots
- **Latency**: < 0.1ms MIDI timing jitter
- **Pattern Switch**: < 1ms quantized scene switching
- **Buffer Sizes**: 64-512 samples (1.3-10.6ms @ 48kHz)

## ⚡ Real-Time Audio Constraints

### The Golden Rules

```cpp
// NEVER in audio thread:
❌ new/delete or malloc/free
❌ std::mutex, std::lock_guard, or any locks
❌ File I/O or network operations
❌ String operations or formatting
❌ Unbounded loops or recursion
❌ Virtual function calls (when avoidable)
❌ Exception throwing/catching
❌ Pattern morphing calculations (pre-compute)
❌ Channel reallocation during playback

// ALWAYS in audio thread:
✅ Pre-allocated buffers
✅ Lock-free data structures
✅ Atomic operations for parameters
✅ Bounded, predictable operations
✅ Cache-friendly memory access
✅ SIMD when beneficial
✅ Pre-computed pattern snapshots
✅ Cached channel assignments
```

### Audio Callback Budget

```cpp
// Time available per callback
// Budget = BufferSize / SampleRate

// Examples at 48kHz:
// 64 samples   = 1.33ms  (TIGHT! Studio recording)
// 128 samples  = 2.67ms  (Recommended minimum)
// 256 samples  = 5.33ms  (Live performance)
// 512 samples  = 10.67ms (Mixing/mastering)

class AudioEngine {
    void processBlock(AudioBuffer& buffer, MidiBuffer& midi) {
        // Measure performance
        juce::ScopedHighResolutionTimer timer("processBlock");
        
        const int numSamples = buffer.getNumSamples();
        const double msAvailable = numSamples / 48000.0 * 1000.0;
        
        // Do work...
        
        // Check if we're within budget
        if (timer.getElapsedMilliseconds() > msAvailable * 0.8) {
            DBG("WARNING: Using 80% of audio budget!");
        }
    }
};
```

## 🔒 Lock-Free Programming

### Lock-Free Message Queue

```cpp
template<typename T, size_t Size>
class LockFreeQueue {
    struct Node {
        std::atomic<T> data;
        std::atomic<bool> ready{false};
    };
    
    std::array<Node, Size> m_buffer;
    std::atomic<size_t> m_writeIndex{0};
    std::atomic<size_t> m_readIndex{0};
    
public:
    bool push(const T& item) {
        size_t write = m_writeIndex.load(std::memory_order_relaxed);
        size_t nextWrite = (write + 1) % Size;
        
        // Check if full
        if (nextWrite == m_readIndex.load(std::memory_order_acquire)) {
            return false;
        }
        
        // Write data
        m_buffer[write].data.store(item, std::memory_order_relaxed);
        m_buffer[write].ready.store(true, std::memory_order_release);
        
        // Update write index
        m_writeIndex.store(nextWrite, std::memory_order_release);
        return true;
    }
    
    bool pop(T& item) {
        size_t read = m_readIndex.load(std::memory_order_relaxed);
        
        // Check if empty
        if (read == m_writeIndex.load(std::memory_order_acquire)) {
            return false;
        }
        
        // Wait for data to be ready
        while (!m_buffer[read].ready.load(std::memory_order_acquire)) {
            // Minimal spin-wait
            std::this_thread::yield();
        }
        
        // Read data
        item = m_buffer[read].data.load(std::memory_order_relaxed);
        m_buffer[read].ready.store(false, std::memory_order_release);
        
        // Update read index
        m_readIndex.store((read + 1) % Size, std::memory_order_release);
        return true;
    }
};
```

### Atomic Parameter Updates

```cpp
class Parameters {
    // Use atomic for real-time safe parameter updates
    std::atomic<float> m_tempo{120.0f};
    std::atomic<int> m_division{4};
    std::atomic<bool> m_isPlaying{false};
    
public:
    // Called from UI thread
    void setTempo(float bpm) {
        m_tempo.store(bpm, std::memory_order_release);
    }
    
    // Called from audio thread
    float getTempo() const {
        return m_tempo.load(std::memory_order_acquire);
    }
};
```

## 💾 Memory Management

### Pre-Allocation Strategy

```cpp
class MemoryPool {
    // Pre-allocate all memory at startup
    static constexpr size_t MAX_EVENTS = 10000;
    static constexpr size_t MAX_VOICES = 256;
    
    struct Pools {
        // Event pool
        std::array<MidiEvent, MAX_EVENTS> eventPool;
        std::atomic<size_t> eventIndex{0};
        
        // Voice pool
        std::array<Voice, MAX_VOICES> voicePool;
        std::atomic<size_t> voiceIndex{0};
        
        // Reset pools (NOT in audio thread!)
        void reset() {
            eventIndex = 0;
            voiceIndex = 0;
        }
    };
    
    // Triple buffering for pool swapping
    std::array<Pools, 3> m_pools;
    std::atomic<int> m_readIndex{0};
    std::atomic<int> m_writeIndex{1};
    int m_prepareIndex{2};
    
public:
    // Audio thread: get event from pool
    MidiEvent* allocateEvent() {
        auto& pool = m_pools[m_readIndex.load()];
        size_t index = pool.eventIndex.fetch_add(1);
        
        if (index >= MAX_EVENTS) {
            return nullptr;  // Pool exhausted
        }
        
        return &pool.eventPool[index];
    }
};
```

### Cache-Friendly Data Layout

```cpp
// Bad: Random memory access
struct BadLayout {
    std::vector<Track*> tracks;  // Pointer chasing
    std::map<int, Stage> stages; // Tree traversal
};

// Good: Contiguous memory access
struct GoodLayout {
    // Group hot data together
    struct alignas(64) TrackData {  // Cache line aligned
        // Hot path data (accessed every audio callback)
        int currentStage;
        float phase;
        bool isPlaying;
        std::array<float, 8> stagePitches;
        
        // Padding to prevent false sharing
        char padding[24];
        
        // Cold data (rarely accessed)
        std::string name;
        Color trackColor;
    };
    
    // Contiguous array for cache efficiency
    std::array<TrackData, MAX_TRACKS> tracks;
};
```

## 🚀 SIMD Optimizations

### Using JUCE's SIMD Helpers

```cpp
class SIMDProcessor {
    void processGates(float* gates, int count) {
        // Scalar version (slow)
        for (int i = 0; i < count; ++i) {
            gates[i] *= 0.5f;
        }
        
        // SIMD version using JUCE (fast)
        juce::FloatVectorOperations::multiply(gates, 0.5f, count);
    }
    
    void addWithGain(float* dest, const float* src, float gain, int count) {
        // JUCE handles SIMD internally
        juce::FloatVectorOperations::addWithMultiply(dest, src, gain, count);
    }
    
    float findPeak(const float* data, int count) {
        // Vectorized peak finding
        auto range = juce::FloatVectorOperations::findMinAndMax(data, count);
        return std::max(std::abs(range.getStart()), std::abs(range.getEnd()));
    }
};
```

### Custom SIMD Operations

```cpp
#if JUCE_USE_SIMD
class CustomSIMD {
    using Vec = juce::dsp::SIMDRegister<float>;
    
    void processRatchets(float* output, int count) {
        // Process 4 floats at a time on most architectures
        const int vecSize = Vec::SIMDNumElements;
        const int vecCount = count / vecSize;
        
        Vec multiplier(0.707f);  // -3dB for each ratchet
        
        for (int i = 0; i < vecCount; ++i) {
            Vec data = Vec::fromRawArray(output + i * vecSize);
            data = data * multiplier;
            data.copyToRawArray(output + i * vecSize);
        }
        
        // Handle remaining samples
        for (int i = vecCount * vecSize; i < count; ++i) {
            output[i] *= 0.707f;
        }
    }
};
#endif
```

## 📊 Profiling & Measurement

### Performance Counters

```cpp
class PerformanceMonitor {
    juce::PerformanceCounter m_processBlockCounter{"ProcessBlock", 100};
    juce::PerformanceCounter m_midiEventCounter{"MIDI Events", 1000};
    
    struct Stats {
        std::atomic<float> averageCpu{0.0f};
        std::atomic<float> peakCpu{0.0f};
        std::atomic<int> xruns{0};
        std::atomic<float> latencyMs{0.0f};
    };
    
    Stats m_stats;
    
public:
    void startMeasurement() {
        m_processBlockCounter.start();
    }
    
    void endMeasurement() {
        m_processBlockCounter.stop();
        
        // Update statistics
        float cpu = getCurrentCpuUsage();
        m_stats.averageCpu = m_stats.averageCpu * 0.95f + cpu * 0.05f;
        
        if (cpu > m_stats.peakCpu) {
            m_stats.peakCpu = cpu;
        }
    }
    
    String getStatsString() const {
        return String::formatted(
            "CPU: %.1f%% (peak: %.1f%%) | XRuns: %d | Latency: %.2fms",
            m_stats.averageCpu.load(),
            m_stats.peakCpu.load(),
            m_stats.xruns.load(),
            m_stats.latencyMs.load()
        );
    }
};
```

### Timing Analysis

```cpp
class TimingAnalyzer {
    struct TimingData {
        int64_t startTime;
        int64_t endTime;
        String operation;
    };
    
    std::array<TimingData, 1000> m_timings;
    std::atomic<int> m_timingIndex{0};
    
public:
    class ScopedTimer {
        TimingAnalyzer& m_analyzer;
        TimingData m_data;
        
    public:
        ScopedTimer(TimingAnalyzer& analyzer, const String& op)
            : m_analyzer(analyzer) {
            m_data.operation = op;
            m_data.startTime = juce::Time::getHighResolutionTicks();
        }
        
        ~ScopedTimer() {
            m_data.endTime = juce::Time::getHighResolutionTicks();
            m_analyzer.addTiming(m_data);
        }
    };
    
    void addTiming(const TimingData& data) {
        int index = m_timingIndex.fetch_add(1) % m_timings.size();
        m_timings[index] = data;
    }
    
    void generateReport() {
        // Analyze timing data
        std::map<String, double> averages;
        
        for (const auto& timing : m_timings) {
            double ms = (timing.endTime - timing.startTime) / 1000000.0;
            averages[timing.operation] += ms;
        }
        
        // Output report
        for (const auto& [op, total] : averages) {
            DBG(op << ": " << (total / m_timings.size()) << "ms average");
        }
    }
};
```

## 🔧 Optimization Techniques

### Branch Prediction Optimization

```cpp
// Bad: Unpredictable branches
void processBad(const Stage& stage) {
    if (stage.probability > random()) {  // Random branch
        if (stage.gateType == MULTIPLE) {
            // Process...
        } else if (stage.gateType == HOLD) {
            // Process...
        }
        // More branches...
    }
}

// Good: Predictable branches and lookup tables
void processGood(const Stage& stage) {
    // Use lookup table instead of branches
    static const std::array<GateProcessor*, 4> processors = {
        &multipleProcessor,
        &holdProcessor,
        &singleProcessor,
        &restProcessor
    };
    
    // Predictable branch (probability often 100%)
    bool shouldProcess = stage.probability >= 100 || 
                         (stage.probability > m_randomTable[m_randomIndex++]);
    
    // Branchless selection
    int processorIndex = static_cast<int>(stage.gateType);
    processors[processorIndex]->process(stage);
}
```

### Loop Optimization

```cpp
// Bad: Poor cache usage
void processTracksBad() {
    for (int s = 0; s < 8; ++s) {
        for (int t = 0; t < numTracks; ++t) {
            processStage(tracks[t].stages[s]);
        }
    }
}

// Good: Better cache locality
void processTracksGood() {
    for (int t = 0; t < numTracks; ++t) {
        // Process all stages of one track together
        for (int s = 0; s < 8; ++s) {
            processStage(tracks[t].stages[s]);
        }
    }
}

// Best: Vectorized with prefetching
void processTracksBest() {
    for (int t = 0; t < numTracks; ++t) {
        // Prefetch next track data
        if (t + 1 < numTracks) {
            __builtin_prefetch(&tracks[t + 1], 0, 3);
        }
        
        // Process current track
        processTrackVectorized(tracks[t]);
    }
}
```

## 🎚️ CPU Throttling Strategies

### Dynamic Quality Adjustment

```cpp
class DynamicQuality {
    enum class QualityLevel {
        Ultra,   // All features enabled
        High,    // Most features, some optimizations
        Medium,  // Balanced quality/performance
        Low      // Maximum performance
    };
    
    std::atomic<QualityLevel> m_currentLevel{QualityLevel::High};
    
public:
    void adjustQuality(float cpuUsage) {
        if (cpuUsage > 80.0f) {
            // Reduce quality
            if (m_currentLevel == QualityLevel::Ultra) {
                m_currentLevel = QualityLevel::High;
            } else if (m_currentLevel == QualityLevel::High) {
                m_currentLevel = QualityLevel::Medium;
            }
        } else if (cpuUsage < 40.0f) {
            // Increase quality
            if (m_currentLevel == QualityLevel::Medium) {
                m_currentLevel = QualityLevel::High;
            } else if (m_currentLevel == QualityLevel::High) {
                m_currentLevel = QualityLevel::Ultra;
            }
        }
    }
    
    bool shouldProcessFeature(const String& feature) {
        switch (m_currentLevel.load()) {
            case QualityLevel::Ultra:
                return true;
                
            case QualityLevel::High:
                return feature != "ComplexModulation";
                
            case QualityLevel::Medium:
                return feature != "ComplexModulation" && 
                       feature != "HighQualityInterpolation";
                
            case QualityLevel::Low:
                return feature == "CoreSequencing" || 
                       feature == "BasicMIDI";
        }
        return true;
    }
};
```

## 📉 Common Performance Pitfalls

### Pitfall 1: Memory Allocation
```cpp
// ❌ BAD: Allocation in audio thread
void processBlockBad(MidiBuffer& midi) {
    std::vector<MidiMessage> events;  // ALLOCATION!
    events.push_back(noteOn);         // REALLOCATION!
}

// ✅ GOOD: Pre-allocated buffer
class Processor {
    std::array<MidiMessage, 1024> m_eventBuffer;
    int m_eventCount = 0;
    
    void processBlockGood(MidiBuffer& midi) {
        m_eventBuffer[m_eventCount++] = noteOn;  // No allocation
    }
};
```

### Pitfall 2: Mutex Usage
```cpp
// ❌ BAD: Mutex in audio thread
std::mutex m_mutex;
void processBlockBad() {
    std::lock_guard<std::mutex> lock(m_mutex);  // BLOCKS!
    // Process...
}

// ✅ GOOD: Lock-free alternative
std::atomic<Parameters> m_params;
void processBlockGood() {
    Parameters params = m_params.load();  // Non-blocking
    // Process...
}
```

### Pitfall 3: Virtual Functions
```cpp
// ❌ BAD: Virtual dispatch in hot path
class Processor {
    virtual void process() = 0;  // Virtual overhead
};

// ✅ GOOD: Static polymorphism
template<typename ProcessorType>
void processOptimized(ProcessorType& processor) {
    processor.process();  // Inlined, no vtable lookup
}
```

## 🏁 Performance Checklist

Before deploying, ensure:

- [ ] No allocations in audio thread
- [ ] All loops are bounded
- [ ] No blocking operations
- [ ] CPU usage < 5% on target hardware
- [ ] MIDI jitter < 0.1ms
- [ ] No xruns at minimum buffer size
- [ ] Memory usage stable over time
- [ ] Profile data shows no hotspots
- [ ] All real-time constraints met

## 🎭 Advanced Feature Performance

### Pattern Morphing Optimization

```cpp
class PatternMorphOptimizer {
    // Pre-computed interpolation tables
    struct MorphCache {
        std::array<float, 128> pitchLUT;     // Pitch interpolation lookup
        std::array<float, 128> velocityLUT;  // Velocity interpolation
        std::array<float, 64> gateLUT;       // Gate length interpolation
        bool isValid;
        TimeStamp lastUpdate;
    };
    
    std::unordered_map<SnapshotPair, MorphCache> m_morphCache;
    
public:
    // Pre-compute morph interpolation (NOT in audio thread)
    void precomputeMorph(const PatternSnapshot& from, 
                        const PatternSnapshot& to) {
        auto key = std::make_pair(from.getId(), to.getId());
        auto& cache = m_morphCache[key];
        
        // Pre-calculate all interpolation values
        for (int i = 0; i < 128; ++i) {
            cache.pitchLUT[i] = calculatePitchInterpolation(from, to, i / 127.0f);
            cache.velocityLUT[i] = calculateVelocityInterpolation(from, to, i / 127.0f);
        }
        
        cache.isValid = true;
        cache.lastUpdate = getCurrentTime();
    }
    
    // Fast morph lookup (audio thread safe)
    Stage morphStage(const Stage& from, const Stage& to, float amount) {
        int lutIndex = static_cast<int>(amount * 127.0f);
        // Use pre-computed values...
    }
};
```

### Multi-Channel Performance

```cpp
class MultiChannelOptimizer {
    // Channel usage statistics for optimization
    struct ChannelStats {
        std::atomic<int> eventCount{0};
        std::atomic<float> avgCpuUsage{0.0f};
        std::atomic<int> conflicts{0};
        TimeStamp lastOptimization;
    };
    
    std::array<ChannelStats, 16> m_channelStats;
    
public:
    void optimizeChannelDistribution() {
        // Analyze usage patterns
        // Redistribute channels based on activity
        // Balance CPU load across channels
    }
    
    bool shouldReallocateChannels() const {
        // Check if reallocation would improve performance
        float totalCpuImprovement = 0.0f;
        // Calculate potential gains...
        return totalCpuImprovement > 5.0f;  // 5% improvement threshold
    }
};
```

### Async Pattern Engine Performance

```cpp
class AsyncPatternPerformance {
    // Lock-free pattern queue for scene switching
    struct PatternSwitch {
        PatternId targetPattern;
        TransitionMode mode;
        int quantizationPoint;
        std::atomic<bool> ready{false};
    };
    
    LockFreeQueue<PatternSwitch, 64> m_switchQueue;
    std::atomic<bool> m_switchPending{false};
    
public:
    // Non-blocking pattern switch request
    bool requestPatternSwitch(PatternId pattern, TransitionMode mode) {
        PatternSwitch sw{pattern, mode, calculateQuantizationPoint()};
        bool success = m_switchQueue.push(sw);
        if (success) {
            m_switchPending = true;
        }
        return success;
    }
    
    // Audio thread processing
    void processPatternSwitches(int numSamples) {
        if (!m_switchPending.load()) return;
        
        PatternSwitch sw;
        if (m_switchQueue.pop(sw)) {
            // Process switch with minimal CPU impact
            executePatternSwitch(sw);
        }
    }
};
```

### CI/CD Performance Testing

```cpp
class AutomatedPerformanceTesting {
    struct BenchmarkResult {
        String testName;
        float cpuUsage;
        int memoryUsage;
        float latency;
        bool passed;
        String failureReason;
    };
    
public:
    std::vector<BenchmarkResult> runFullBenchmarkSuite() {
        std::vector<BenchmarkResult> results;
        
        // Test scenarios
        results.push_back(benchmarkBasicPlayback());
        results.push_back(benchmarkMultiChannelRouting());
        results.push_back(benchmarkPatternMorphing());
        results.push_back(benchmarkAsyncPatternSwitch());
        results.push_back(benchmarkHighTrackCount());
        results.push_back(benchmarkPluginHosting());
        
        return results;
    }
    
    bool validatePerformanceRegression(const std::vector<BenchmarkResult>& current,
                                      const std::vector<BenchmarkResult>& baseline) {
        // Compare results and flag regressions
        for (size_t i = 0; i < current.size() && i < baseline.size(); ++i) {
            if (current[i].cpuUsage > baseline[i].cpuUsage * 1.1f) {  // 10% regression
                return false;
            }
        }
        return true;
    }
};
```

---

*Performance optimization is an ongoing process. Profile regularly and optimize based on real-world usage patterns. New features like multi-channel routing and pattern morphing require continuous performance validation.*