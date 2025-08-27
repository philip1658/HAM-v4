/*
  ==============================================================================

    PluginSandbox.h
    Out-of-process plugin hosting with crash protection
    Implements sandboxing to prevent plugin crashes from affecting HAM

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <memory>
#include <thread>
#include <chrono>

namespace HAM
{

/**
 * SharedMemoryAudioBuffer - Lock-free shared memory for audio data
 * Uses ring buffer design for zero-copy audio transfer
 */
class SharedMemoryAudioBuffer
{
public:
    static constexpr size_t MAX_CHANNELS = 32;
    static constexpr size_t MAX_BLOCK_SIZE = 8192;
    
    struct Header
    {
        std::atomic<uint64_t> writePosition{0};
        std::atomic<uint64_t> readPosition{0};
        std::atomic<int> numChannels{2};
        std::atomic<int> blockSize{512};
        std::atomic<bool> isAlive{true};
        std::atomic<int64_t> lastHeartbeat{0};
        
        // Performance metrics
        std::atomic<int64_t> totalLatencyUs{0};
        std::atomic<int> latencySamples{0};
    };
    
    SharedMemoryAudioBuffer(const juce::String& name, bool isHost);
    ~SharedMemoryAudioBuffer();
    
    // Audio data transfer (lock-free)
    bool writeAudioBlock(const float** inputChannels, int numChannels, int numSamples);
    bool readAudioBlock(float** outputChannels, int numChannels, int numSamples);
    
    // MIDI data transfer (lock-free)
    bool writeMidiBuffer(const juce::MidiBuffer& midiBuffer);
    bool readMidiBuffer(juce::MidiBuffer& midiBuffer);
    
    // Health monitoring
    void updateHeartbeat();
    bool isProcessAlive() const;
    int64_t getLatencyMicroseconds() const;
    
private:
    juce::String m_name;
    bool m_isHost;
    void* m_sharedMemoryPtr{nullptr};  // Raw pointer to shared memory
    size_t m_sharedMemorySize{0};
    int m_sharedMemoryFd{-1};  // File descriptor for shared memory (POSIX)
    Header* m_header{nullptr};
    float* m_audioData{nullptr};
    uint8_t* m_midiData{nullptr};
    
    static constexpr size_t TOTAL_SIZE = 
        sizeof(Header) + 
        (MAX_CHANNELS * MAX_BLOCK_SIZE * sizeof(float)) +
        (64 * 1024); // 64KB for MIDI data
};

//==============================================================================

/**
 * PluginSandbox - Manages sandboxed plugin instances
 * Runs plugins in separate processes with crash protection
 */
class PluginSandbox
{
public:
    enum class State
    {
        Idle,
        Starting,
        Running,
        Crashed,
        Stopping
    };
    
    struct Configuration
    {
        double sampleRate;
        int blockSize;
        bool useRosetta;  // For x86_64 plugins on Apple Silicon
        int timeoutMs;     // Process startup timeout
        bool autoRestart;   // Auto-restart on crash
        int maxRestartAttempts;
        
        Configuration() 
            : sampleRate(48000.0)
            , blockSize(512)
            , useRosetta(false)
            , timeoutMs(5000)
            , autoRestart(true)
            , maxRestartAttempts(3)
        {}
    };
    
    PluginSandbox(const juce::PluginDescription& description,
                  const Configuration& config = Configuration());
    ~PluginSandbox();
    
    // Process lifecycle
    bool start();
    void stop();
    bool restart();
    
    // Audio processing (real-time safe)
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiBuffer);
    
    // Plugin control
    void setParameter(int parameterIndex, float value);
    float getParameter(int parameterIndex) const;
    
    // Editor management
    bool hasEditor() const;
    void showEditor();
    void closeEditor();
    
    // State queries
    State getState() const { return m_state.load(); }
    bool isRunning() const { return m_state == State::Running; }
    bool hasCrashed() const { return m_state == State::Crashed; }
    
    // Performance metrics
    struct Metrics
    {
        int64_t averageLatencyUs{0};
        int64_t maxLatencyUs{0};
        int crashCount{0};
        int restartCount{0};
        float cpuUsage{0.0f};
    };
    
    Metrics getMetrics() const;
    
    // Crash recovery callback
    using CrashCallback = std::function<void(const juce::String& errorMessage)>;
    void setCrashCallback(CrashCallback callback) { m_crashCallback = callback; }
    
private:
    // Plugin information
    juce::PluginDescription m_description;
    Configuration m_config;
    
    // Process management
    std::unique_ptr<juce::ChildProcess> m_process;
    std::atomic<State> m_state{State::Idle};
    
    // Shared memory IPC
    std::unique_ptr<SharedMemoryAudioBuffer> m_audioBuffer;
    juce::String m_ipcChannelName;
    
    // Health monitoring
    std::unique_ptr<std::thread> m_monitorThread;
    std::atomic<bool> m_shouldMonitor{false};
    
    // Crash recovery
    std::atomic<int> m_crashCount{0};
    std::atomic<int> m_restartAttempts{0};
    CrashCallback m_crashCallback;
    
    // Performance tracking
    mutable std::atomic<int64_t> m_totalLatency{0};
    mutable std::atomic<int> m_latencySamples{0};
    mutable std::atomic<int64_t> m_maxLatency{0};
    
    // CPU usage tracking
    mutable std::atomic<int64_t> m_lastCpuMeasureTime{0};
    mutable std::atomic<int> m_messagesSentCount{0};
    mutable float m_estimatedCpuUsage{0.0f};
    
    // Helper methods
    bool launchProcess();
    void monitorHealth();
    void handleCrash(const juce::String& reason);
    juce::String generateIPCChannelName() const;
    juce::File findSandboxExecutable() const;
};

//==============================================================================

/**
 * SandboxedPluginHost - Host process for sandboxed plugins
 * This runs in the separate process and hosts the actual plugin
 */
class SandboxedPluginHost : public juce::AudioProcessor
{
public:
    SandboxedPluginHost();
    ~SandboxedPluginHost() override;
    
    // Initialize with plugin and IPC channel
    bool initialise(const juce::String& pluginPath, 
                   const juce::String& ipcChannelName);
    
    // AudioProcessor interface
    const juce::String getName() const override { return "SandboxedHost"; }
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer, 
                     juce::MidiBuffer& midiBuffer) override;
    
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    
    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}
    
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    
private:
    std::unique_ptr<juce::AudioPluginInstance> m_plugin;
    std::unique_ptr<SharedMemoryAudioBuffer> m_ipcBuffer;
    juce::AudioPluginFormatManager m_formatManager;
    
    // Signal handlers for crash detection
    static void installSignalHandlers();
    static void signalHandler(int signal);
};

//==============================================================================

/**
 * CrashRecoveryManager - Manages crash detection and recovery
 * Coordinates sandbox restart and state restoration
 */
class CrashRecoveryManager
{
public:
    struct RecoveryPolicy
    {
        bool autoRestart;
        int maxRestartAttempts;
        int restartDelayMs;
        bool saveStateBeforeCrash;
        bool notifyUser;
        
        RecoveryPolicy()
            : autoRestart(true)
            , maxRestartAttempts(3)
            , restartDelayMs(1000)
            , saveStateBeforeCrash(true)
            , notifyUser(true)
        {}
    };
    
    CrashRecoveryManager(RecoveryPolicy policy = RecoveryPolicy());
    ~CrashRecoveryManager();
    
    // Register a sandbox for monitoring
    void registerSandbox(PluginSandbox* sandbox);
    void unregisterSandbox(PluginSandbox* sandbox);
    
    // Manual recovery trigger
    void recoverSandbox(PluginSandbox* sandbox);
    
    // Statistics
    struct Stats
    {
        int totalCrashes{0};
        int successfulRecoveries{0};
        int failedRecoveries{0};
        std::vector<juce::String> crashLog;
    };
    
    Stats getStatistics() const;
    void clearStatistics();
    
private:
    RecoveryPolicy m_policy;
    std::vector<PluginSandbox*> m_sandboxes;
    mutable std::mutex m_mutex;
    
    // Recovery statistics
    mutable Stats m_stats;
    
    // Recovery worker thread
    std::unique_ptr<std::thread> m_recoveryThread;
    std::atomic<bool> m_shouldRun{true};
    
    void recoveryWorker();
    void attemptRecovery(PluginSandbox* sandbox);
};

//==============================================================================

/**
 * SandboxFactory - Factory for creating sandboxed or direct plugin instances
 * Provides fallback to direct hosting if sandboxing fails
 */
class SandboxFactory
{
public:
    enum class HostingMode
    {
        PreferSandboxed,   // Try sandbox first, fallback to direct
        ForceSandboxed,    // Only use sandbox (fail if not possible)
        ForceDirect        // Skip sandbox, use direct hosting
    };
    
    static std::unique_ptr<juce::AudioPluginInstance> createPlugin(
        const juce::PluginDescription& description,
        double sampleRate,
        int blockSize,
        HostingMode mode = HostingMode::PreferSandboxed);
    
    static std::unique_ptr<PluginSandbox> createSandbox(
        const juce::PluginDescription& description,
        const PluginSandbox::Configuration& config = PluginSandbox::Configuration());
    
    // Check if plugin should be sandboxed (based on risk assessment)
    static bool shouldSandbox(const juce::PluginDescription& description);
    
private:
    // Risk assessment for plugins
    static int assessPluginRisk(const juce::PluginDescription& description);
};

} // namespace HAM