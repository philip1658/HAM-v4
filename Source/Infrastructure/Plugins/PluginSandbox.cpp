/*
  ==============================================================================

    PluginSandbox.cpp
    Out-of-process plugin hosting implementation

  ==============================================================================
*/

#include "PluginSandbox.h"
#include <signal.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

namespace HAM
{

//==============================================================================
// SharedMemoryAudioBuffer Implementation

SharedMemoryAudioBuffer::SharedMemoryAudioBuffer(const juce::String& name, bool isHost)
    : m_name(name)
    , m_isHost(isHost)
    , m_sharedMemorySize(TOTAL_SIZE)
{
    // Create shared memory name for POSIX (must be short on macOS)
    // macOS has a limit of 31 characters for shared memory names
    juce::String shortName = m_name.substring(0, 20);
    juce::String shmName = "/" + shortName;
    
    if (m_isHost)
    {
        // Host creates the shared memory
        // Remove any existing shared memory with this name
        shm_unlink(shmName.toRawUTF8());
        
        // Create new shared memory
        m_sharedMemoryFd = shm_open(shmName.toRawUTF8(), O_CREAT | O_RDWR, 0666);
        if (m_sharedMemoryFd < 0)
        {
            DBG("Failed to create shared memory: " << strerror(errno));
            return;
        }
        
        // Set the size
        if (ftruncate(m_sharedMemoryFd, m_sharedMemorySize) < 0)
        {
            DBG("Failed to set shared memory size: " << strerror(errno));
            close(m_sharedMemoryFd);
            m_sharedMemoryFd = -1;
            return;
        }
    }
    else
    {
        // Client attaches to existing shared memory
        // Retry connection with timeout
        int attempts = 0;
        while (attempts < 50)
        {
            m_sharedMemoryFd = shm_open(shmName.toRawUTF8(), O_RDWR, 0666);
            if (m_sharedMemoryFd >= 0)
                break;
                
            juce::Thread::sleep(100);  // Wait 100ms between attempts
            attempts++;
        }
        
        if (m_sharedMemoryFd < 0)
        {
            DBG("Failed to open shared memory: " << strerror(errno));
            return;
        }
    }
    
    // Map the shared memory
    m_sharedMemoryPtr = mmap(nullptr, m_sharedMemorySize, 
                            PROT_READ | PROT_WRITE, MAP_SHARED, 
                            m_sharedMemoryFd, 0);
    
    if (m_sharedMemoryPtr == MAP_FAILED)
    {
        DBG("Failed to map shared memory: " << strerror(errno));
        close(m_sharedMemoryFd);
        m_sharedMemoryFd = -1;
        m_sharedMemoryPtr = nullptr;
        return;
    }
    
    // Map memory regions
    uint8_t* base = static_cast<uint8_t*>(m_sharedMemoryPtr);
    m_header = reinterpret_cast<Header*>(base);
    m_audioData = reinterpret_cast<float*>(base + sizeof(Header));
    m_midiData = base + sizeof(Header) + (MAX_CHANNELS * MAX_BLOCK_SIZE * sizeof(float));
    
    // Initialize header if we're the host
    if (m_isHost)
    {
        new (m_header) Header();  // Placement new for atomic initialization
        m_header->isAlive = true;
        updateHeartbeat();
    }
}

SharedMemoryAudioBuffer::~SharedMemoryAudioBuffer()
{
    if (m_header && m_isHost)
    {
        m_header->isAlive = false;
    }
    
    // Unmap shared memory
    if (m_sharedMemoryPtr && m_sharedMemoryPtr != MAP_FAILED)
    {
        munmap(m_sharedMemoryPtr, m_sharedMemorySize);
    }
    
    // Close file descriptor
    if (m_sharedMemoryFd >= 0)
    {
        close(m_sharedMemoryFd);
    }
    
    // Remove shared memory if we're the host
    if (m_isHost)
    {
        juce::String shortName = m_name.substring(0, 20);
        juce::String shmName = "/" + shortName;
        shm_unlink(shmName.toRawUTF8());
    }
}

bool SharedMemoryAudioBuffer::writeAudioBlock(const float** inputChannels, 
                                             int numChannels, int numSamples)
{
    if (!m_header || !m_audioData) return false;
    
    // Update configuration
    m_header->numChannels = numChannels;
    m_header->blockSize = numSamples;
    
    // Calculate buffer position (simple double buffering)
    uint64_t writePos = m_header->writePosition.load();
    int bufferIndex = writePos % 2;  // Alternate between two buffers
    
    // Copy audio data (lock-free write)
    float* destBase = m_audioData + (bufferIndex * MAX_CHANNELS * MAX_BLOCK_SIZE);
    for (int ch = 0; ch < numChannels && ch < MAX_CHANNELS; ++ch)
    {
        float* dest = destBase + (ch * MAX_BLOCK_SIZE);
        const float* src = inputChannels[ch];
        
        // Use memcpy for better performance
        std::memcpy(dest, src, numSamples * sizeof(float));
    }
    
    // Update write position (atomic increment)
    m_header->writePosition.fetch_add(1, std::memory_order_release);
    
    // Update heartbeat
    updateHeartbeat();
    
    return true;
}

bool SharedMemoryAudioBuffer::readAudioBlock(float** outputChannels, 
                                            int numChannels, int numSamples)
{
    if (!m_header || !m_audioData) return false;
    
    // Check if new data is available
    uint64_t readPos = m_header->readPosition.load(std::memory_order_acquire);
    uint64_t writePos = m_header->writePosition.load(std::memory_order_acquire);
    
    if (readPos >= writePos)
    {
        // No new data, output silence
        for (int ch = 0; ch < numChannels; ++ch)
        {
            std::memset(outputChannels[ch], 0, numSamples * sizeof(float));
        }
        return false;
    }
    
    // Read from the appropriate buffer
    int bufferIndex = readPos % 2;
    float* srcBase = m_audioData + (bufferIndex * MAX_CHANNELS * MAX_BLOCK_SIZE);
    
    int channelsToRead = std::min(numChannels, m_header->numChannels.load());
    int samplesToRead = std::min(numSamples, m_header->blockSize.load());
    
    // Copy audio data
    for (int ch = 0; ch < channelsToRead; ++ch)
    {
        float* src = srcBase + (ch * MAX_BLOCK_SIZE);
        float* dest = outputChannels[ch];
        std::memcpy(dest, src, samplesToRead * sizeof(float));
    }
    
    // Clear any remaining channels
    for (int ch = channelsToRead; ch < numChannels; ++ch)
    {
        std::memset(outputChannels[ch], 0, numSamples * sizeof(float));
    }
    
    // Update read position
    m_header->readPosition.fetch_add(1, std::memory_order_release);
    
    // Calculate latency
    auto now = juce::Time::getHighResolutionTicks();
    int64_t latencyUs = juce::Time::highResolutionTicksToSeconds(
        now - m_header->lastHeartbeat) * 1000000;
    
    m_header->totalLatencyUs.fetch_add(latencyUs);
    m_header->latencySamples.fetch_add(1);
    
    return true;
}

bool SharedMemoryAudioBuffer::writeMidiBuffer(const juce::MidiBuffer& midiBuffer)
{
    if (!m_midiData) return false;
    
    // Simple MIDI serialization
    size_t offset = 0;
    
    // Write number of events
    uint32_t numEvents = midiBuffer.getNumEvents();
    std::memcpy(m_midiData + offset, &numEvents, sizeof(numEvents));
    offset += sizeof(numEvents);
    
    // Write each MIDI event
    for (const auto metadata : midiBuffer)
    {
        // Write timestamp
        int32_t timestamp = metadata.samplePosition;
        std::memcpy(m_midiData + offset, &timestamp, sizeof(timestamp));
        offset += sizeof(timestamp);
        
        // Write message size
        uint32_t size = metadata.numBytes;
        std::memcpy(m_midiData + offset, &size, sizeof(size));
        offset += sizeof(size);
        
        // Write message data
        std::memcpy(m_midiData + offset, metadata.data, size);
        offset += size;
        
        // Safety check
        if (offset > 64 * 1024 - 1024) break;  // Leave some buffer
    }
    
    return true;
}

bool SharedMemoryAudioBuffer::readMidiBuffer(juce::MidiBuffer& midiBuffer)
{
    if (!m_midiData) return false;
    
    midiBuffer.clear();
    size_t offset = 0;
    
    // Read number of events
    uint32_t numEvents = 0;
    std::memcpy(&numEvents, m_midiData + offset, sizeof(numEvents));
    offset += sizeof(numEvents);
    
    // Read each MIDI event
    for (uint32_t i = 0; i < numEvents; ++i)
    {
        // Read timestamp
        int32_t timestamp = 0;
        std::memcpy(&timestamp, m_midiData + offset, sizeof(timestamp));
        offset += sizeof(timestamp);
        
        // Read message size
        uint32_t size = 0;
        std::memcpy(&size, m_midiData + offset, sizeof(size));
        offset += sizeof(size);
        
        // Read message data
        if (size > 0 && size <= 256)  // Sanity check
        {
            midiBuffer.addEvent(m_midiData + offset, size, timestamp);
        }
        offset += size;
        
        // Safety check
        if (offset > 64 * 1024) break;
    }
    
    return true;
}

void SharedMemoryAudioBuffer::updateHeartbeat()
{
    if (m_header)
    {
        m_header->lastHeartbeat = juce::Time::getHighResolutionTicks();
    }
}

bool SharedMemoryAudioBuffer::isProcessAlive() const
{
    if (!m_header) return false;
    
    // Check if process has sent heartbeat recently
    auto now = juce::Time::getHighResolutionTicks();
    auto lastBeat = m_header->lastHeartbeat.load();
    
    double secondsSinceLastBeat = juce::Time::highResolutionTicksToSeconds(now - lastBeat);
    
    // Consider dead if no heartbeat for 1 second
    return m_header->isAlive && secondsSinceLastBeat < 1.0;
}

int64_t SharedMemoryAudioBuffer::getLatencyMicroseconds() const
{
    if (!m_header) return 0;
    
    int samples = m_header->latencySamples.load();
    if (samples == 0) return 0;
    
    return m_header->totalLatencyUs.load() / samples;
}

//==============================================================================
// PluginSandbox Implementation

PluginSandbox::PluginSandbox(const juce::PluginDescription& description,
                           const Configuration& config)
    : m_description(description)
    , m_config(config)
{
    m_ipcChannelName = generateIPCChannelName();
}

PluginSandbox::~PluginSandbox()
{
    stop();
}

bool PluginSandbox::start()
{
    if (m_state != State::Idle && m_state != State::Crashed)
    {
        return false;
    }
    
    m_state = State::Starting;
    
    // Create shared memory buffer
    m_audioBuffer = std::make_unique<SharedMemoryAudioBuffer>(m_ipcChannelName, true);
    
    // Launch sandbox process
    if (!launchProcess())
    {
        m_state = State::Idle;
        return false;
    }
    
    // Start health monitoring
    m_shouldMonitor = true;
    m_monitorThread = std::make_unique<std::thread>([this]() { monitorHealth(); });
    
    // Wait for process to be ready (with timeout)
    auto startTime = std::chrono::steady_clock::now();
    while (m_state == State::Starting)
    {
        if (m_audioBuffer->isProcessAlive())
        {
            m_state = State::Running;
            return true;
        }
        
        auto elapsed = std::chrono::steady_clock::now() - startTime;
        if (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() > m_config.timeoutMs)
        {
            DBG("PluginSandbox: Timeout waiting for process to start");
            stop();
            return false;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    return m_state == State::Running;
}

void PluginSandbox::stop()
{
    m_state = State::Stopping;
    
    // Stop monitoring
    m_shouldMonitor = false;
    if (m_monitorThread && m_monitorThread->joinable())
    {
        m_monitorThread->join();
    }
    
    // Terminate process
    if (m_process && m_process->isRunning())
    {
        m_process->kill();
    }
    
    // Clean up resources
    m_process.reset();
    m_audioBuffer.reset();
    
    m_state = State::Idle;
}

bool PluginSandbox::restart()
{
    stop();
    
    // Reset restart counter if this is manual
    if (m_state != State::Crashed)
    {
        m_restartAttempts = 0;
    }
    
    return start();
}

void PluginSandbox::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiBuffer)
{
    if (m_state != State::Running || !m_audioBuffer)
    {
        // Output silence if not running
        buffer.clear();
        midiBuffer.clear();
        return;
    }
    
    auto startTime = juce::Time::getHighResolutionTicks();
    
    // Send audio to sandbox
    const float* inputChannels[buffer.getNumChannels()];
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        inputChannels[ch] = buffer.getReadPointer(ch);
    }
    
    m_audioBuffer->writeAudioBlock(inputChannels, buffer.getNumChannels(), buffer.getNumSamples());
    m_audioBuffer->writeMidiBuffer(midiBuffer);
    
    // Wait for processed audio (with very short timeout for real-time safety)
    // In practice, this should be nearly instantaneous due to shared memory
    float* outputChannels[buffer.getNumChannels()];
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        outputChannels[ch] = buffer.getWritePointer(ch);
    }
    
    if (!m_audioBuffer->readAudioBlock(outputChannels, buffer.getNumChannels(), buffer.getNumSamples()))
    {
        // No data received - output silence
        buffer.clear();
    }
    
    // Read processed MIDI
    midiBuffer.clear();
    m_audioBuffer->readMidiBuffer(midiBuffer);
    
    // Track latency
    auto endTime = juce::Time::getHighResolutionTicks();
    int64_t latencyUs = juce::Time::highResolutionTicksToSeconds(endTime - startTime) * 1000000;
    
    m_totalLatency.fetch_add(latencyUs);
    m_latencySamples.fetch_add(1);
    
    if (latencyUs > m_maxLatency)
    {
        m_maxLatency = latencyUs;
    }
}

void PluginSandbox::setParameter(int parameterIndex, float value)
{
    // TODO: Implement parameter control via IPC
    juce::ignoreUnused(parameterIndex, value);
}

float PluginSandbox::getParameter(int parameterIndex) const
{
    // TODO: Implement parameter query via IPC
    juce::ignoreUnused(parameterIndex);
    return 0.0f;
}

bool PluginSandbox::hasEditor() const
{
    return m_description.hasSharedContainer;
}

void PluginSandbox::showEditor()
{
    // TODO: Implement editor showing via IPC
}

void PluginSandbox::closeEditor()
{
    // TODO: Implement editor closing via IPC
}

PluginSandbox::Metrics PluginSandbox::getMetrics() const
{
    Metrics metrics;
    
    int samples = m_latencySamples.load();
    if (samples > 0)
    {
        metrics.averageLatencyUs = m_totalLatency.load() / samples;
    }
    
    metrics.maxLatencyUs = m_maxLatency.load();
    metrics.crashCount = m_crashCount.load();
    metrics.restartCount = m_restartAttempts.load();
    
    // TODO: Calculate CPU usage based on process activity
    // m_sandboxProcess member needs to be properly implemented
    if (false) // Disabled until m_sandboxProcess is properly defined
    {
        auto currentTime = juce::Time::getHighResolutionTicks();
        auto timeDelta = (currentTime - m_lastCpuMeasureTime) / (double)juce::Time::getHighResolutionTicksPerSecond();
        
        if (timeDelta > 0.1) // Update every 100ms
        {
            // Basic CPU estimation based on message processing activity
            auto messagesPerSecond = m_messagesSentCount.load() / std::max(0.1, timeDelta);
            m_estimatedCpuUsage = std::min(100.0f, static_cast<float>(messagesPerSecond * 0.1)); // Rough estimation
            
            m_lastCpuMeasureTime = currentTime;
            m_messagesSentCount = 0;
        }
        metrics.cpuUsage = m_estimatedCpuUsage;
    }
    else
    {
        metrics.cpuUsage = 0.0f;
    }
    
    return metrics;
}

bool PluginSandbox::launchProcess()
{
    auto sandboxExe = findSandboxExecutable();
    if (!sandboxExe.existsAsFile())
    {
        DBG("PluginSandbox: Sandbox executable not found: " << sandboxExe.getFullPathName());
        return false;
    }
    
    // Build command line
    juce::StringArray args;
    args.add(sandboxExe.getFullPathName());
    args.add("--plugin");
    args.add(m_description.fileOrIdentifier);
    args.add("--ipc");
    args.add(m_ipcChannelName);
    args.add("--samplerate");
    args.add(juce::String(m_config.sampleRate));
    args.add("--blocksize");
    args.add(juce::String(m_config.blockSize));
    
    if (m_config.useRosetta)
    {
        args.add("--rosetta");
    }
    
    // Launch process
    m_process = std::make_unique<juce::ChildProcess>();
    
    if (!m_process->start(args))
    {
        DBG("PluginSandbox: Failed to launch process");
        m_process.reset();
        return false;
    }
    
    DBG("PluginSandbox: Process launched successfully");
    return true;
}

void PluginSandbox::monitorHealth()
{
    while (m_shouldMonitor)
    {
        if (m_state == State::Running)
        {
            // Check if process is still alive
            bool processRunning = m_process && m_process->isRunning();
            bool ipcAlive = m_audioBuffer && m_audioBuffer->isProcessAlive();
            
            if (!processRunning || !ipcAlive)
            {
                handleCrash(processRunning ? "IPC timeout" : "Process terminated");
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void PluginSandbox::handleCrash(const juce::String& reason)
{
    DBG("PluginSandbox: Crash detected - " << reason);
    
    m_state = State::Crashed;
    m_crashCount.fetch_add(1);
    
    // Notify callback
    if (m_crashCallback)
    {
        m_crashCallback(reason);
    }
    
    // Attempt auto-restart if configured
    if (m_config.autoRestart && m_restartAttempts < m_config.maxRestartAttempts)
    {
        m_restartAttempts.fetch_add(1);
        DBG("PluginSandbox: Attempting restart " << m_restartAttempts << "/" << m_config.maxRestartAttempts);
        
        // Wait a bit before restarting
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        if (restart())
        {
            DBG("PluginSandbox: Restart successful");
        }
        else
        {
            DBG("PluginSandbox: Restart failed");
        }
    }
}

juce::String PluginSandbox::generateIPCChannelName() const
{
    // Generate unique IPC channel name
    return "HAM_Sandbox_" + juce::Uuid().toString();
}

juce::File PluginSandbox::findSandboxExecutable() const
{
    // Look for sandbox executable in build directory
    auto appDir = juce::File::getSpecialLocation(juce::File::currentApplicationFile).getParentDirectory();
    
    // Check various possible locations
    std::vector<juce::File> locations = {
        appDir.getChildFile("PluginSandboxHost"),
        appDir.getParentDirectory().getChildFile("PluginSandboxHost"),
        juce::File::getCurrentWorkingDirectory().getChildFile("build/PluginSandboxHost"),
        juce::File::getCurrentWorkingDirectory().getChildFile("build/Debug/PluginSandboxHost"),
        juce::File::getCurrentWorkingDirectory().getChildFile("build/Release/PluginSandboxHost"),
    };
    
    for (const auto& location : locations)
    {
        if (location.existsAsFile())
        {
            return location;
        }
    }
    
    return juce::File();
}

//==============================================================================
// SandboxedPluginHost Implementation

SandboxedPluginHost::SandboxedPluginHost()
{
    // Add supported formats
    m_formatManager.addDefaultFormats();
    
    // Install signal handlers for crash detection
    installSignalHandlers();
}

SandboxedPluginHost::~SandboxedPluginHost()
{
    if (m_plugin)
    {
        m_plugin->releaseResources();
    }
}

bool SandboxedPluginHost::initialise(const juce::String& pluginPath, 
                                    const juce::String& ipcChannelName)
{
    // Connect to shared memory
    m_ipcBuffer = std::make_unique<SharedMemoryAudioBuffer>(ipcChannelName, false);
    
    // Load the plugin
    juce::OwnedArray<juce::PluginDescription> typesFound;
    
    for (int i = 0; i < m_formatManager.getNumFormats(); ++i)
    {
        auto* format = m_formatManager.getFormat(i);
        format->findAllTypesForFile(typesFound, pluginPath);
    }
    
    if (typesFound.isEmpty())
    {
        DBG("SandboxedPluginHost: No plugins found in: " << pluginPath);
        return false;
    }
    
    // Create plugin instance
    juce::String errorMessage;
    m_plugin = m_formatManager.createPluginInstance(
        *typesFound[0], 48000.0, 512, errorMessage);
    
    if (!m_plugin)
    {
        DBG("SandboxedPluginHost: Failed to create plugin: " << errorMessage);
        return false;
    }
    
    DBG("SandboxedPluginHost: Plugin loaded successfully");
    return true;
}

void SandboxedPluginHost::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    if (m_plugin)
    {
        m_plugin->prepareToPlay(sampleRate, samplesPerBlock);
    }
}

void SandboxedPluginHost::releaseResources()
{
    if (m_plugin)
    {
        m_plugin->releaseResources();
    }
}

void SandboxedPluginHost::processBlock(juce::AudioBuffer<float>& buffer, 
                                      juce::MidiBuffer& midiBuffer)
{
    if (!m_plugin || !m_ipcBuffer) return;
    
    // Read input from shared memory
    float* channels[buffer.getNumChannels()];
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        channels[ch] = buffer.getWritePointer(ch);
    }
    
    m_ipcBuffer->readAudioBlock(channels, buffer.getNumChannels(), buffer.getNumSamples());
    m_ipcBuffer->readMidiBuffer(midiBuffer);
    
    // Process through plugin
    m_plugin->processBlock(buffer, midiBuffer);
    
    // Write output to shared memory
    const float* outputChannels[buffer.getNumChannels()];
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        outputChannels[ch] = buffer.getReadPointer(ch);
    }
    
    m_ipcBuffer->writeAudioBlock(outputChannels, buffer.getNumChannels(), buffer.getNumSamples());
    m_ipcBuffer->writeMidiBuffer(midiBuffer);
}

void SandboxedPluginHost::installSignalHandlers()
{
    signal(SIGSEGV, signalHandler);
    signal(SIGABRT, signalHandler);
    signal(SIGFPE, signalHandler);
    signal(SIGILL, signalHandler);
    signal(SIGBUS, signalHandler);
}

void SandboxedPluginHost::signalHandler(int signal)
{
    const char* signalName = "Unknown";
    switch (signal)
    {
        case SIGSEGV: signalName = "SIGSEGV"; break;
        case SIGABRT: signalName = "SIGABRT"; break;
        case SIGFPE:  signalName = "SIGFPE"; break;
        case SIGILL:  signalName = "SIGILL"; break;
        case SIGBUS:  signalName = "SIGBUS"; break;
    }
    
    // Log crash (avoid complex operations in signal handler)
    write(STDERR_FILENO, "Crash: ", 7);
    write(STDERR_FILENO, signalName, strlen(signalName));
    write(STDERR_FILENO, "\n", 1);
    
    // Exit with error code
    _exit(128 + signal);
}

//==============================================================================
// CrashRecoveryManager Implementation

CrashRecoveryManager::CrashRecoveryManager(RecoveryPolicy policy)
    : m_policy(policy)
{
    // Start recovery worker thread
    m_recoveryThread = std::make_unique<std::thread>([this]() { recoveryWorker(); });
}

CrashRecoveryManager::~CrashRecoveryManager()
{
    m_shouldRun = false;
    if (m_recoveryThread && m_recoveryThread->joinable())
    {
        m_recoveryThread->join();
    }
}

void CrashRecoveryManager::registerSandbox(PluginSandbox* sandbox)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_sandboxes.push_back(sandbox);
    
    // Set crash callback
    sandbox->setCrashCallback([this, sandbox](const juce::String& error) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stats.totalCrashes++;
        m_stats.crashLog.push_back(juce::Time::getCurrentTime().toString(true, true) + 
                                  " - " + error);
        
        if (m_policy.autoRestart)
        {
            attemptRecovery(sandbox);
        }
    });
}

void CrashRecoveryManager::unregisterSandbox(PluginSandbox* sandbox)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_sandboxes.erase(
        std::remove(m_sandboxes.begin(), m_sandboxes.end(), sandbox),
        m_sandboxes.end());
}

void CrashRecoveryManager::recoverSandbox(PluginSandbox* sandbox)
{
    attemptRecovery(sandbox);
}

CrashRecoveryManager::Stats CrashRecoveryManager::getStatistics() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_stats;
}

void CrashRecoveryManager::clearStatistics()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stats = Stats();
}

void CrashRecoveryManager::recoveryWorker()
{
    while (m_shouldRun)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        // Check health of all registered sandboxes
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto* sandbox : m_sandboxes)
        {
            if (sandbox->hasCrashed() && m_policy.autoRestart)
            {
                attemptRecovery(sandbox);
            }
        }
    }
}

void CrashRecoveryManager::attemptRecovery(PluginSandbox* sandbox)
{
    DBG("CrashRecoveryManager: Attempting recovery");
    
    // Wait before restart
    std::this_thread::sleep_for(std::chrono::milliseconds(m_policy.restartDelayMs));
    
    // Try to restart
    if (sandbox->restart())
    {
        m_stats.successfulRecoveries++;
        DBG("CrashRecoveryManager: Recovery successful");
    }
    else
    {
        m_stats.failedRecoveries++;
        DBG("CrashRecoveryManager: Recovery failed");
        
        if (m_policy.notifyUser)
        {
            // TODO: Notify user of failed recovery
        }
    }
}

//==============================================================================
// SandboxFactory Implementation

std::unique_ptr<juce::AudioPluginInstance> SandboxFactory::createPlugin(
    const juce::PluginDescription& description,
    double sampleRate,
    int blockSize,
    HostingMode mode)
{
    if (mode == HostingMode::ForceDirect)
    {
        // Direct hosting only
        juce::AudioPluginFormatManager formatManager;
        formatManager.addDefaultFormats();
        
        juce::String errorMessage;
        return formatManager.createPluginInstance(description, sampleRate, blockSize, errorMessage);
    }
    
    if (mode == HostingMode::PreferSandboxed || mode == HostingMode::ForceSandboxed)
    {
        // Try sandboxed hosting
        if (shouldSandbox(description))
        {
            // TODO: Create sandboxed wrapper that implements AudioPluginInstance interface
            DBG("SandboxFactory: Sandboxed hosting not yet fully implemented");
        }
        
        if (mode == HostingMode::ForceSandboxed)
        {
            // No fallback allowed
            return nullptr;
        }
    }
    
    // Fallback to direct hosting
    juce::AudioPluginFormatManager formatManager;
    formatManager.addDefaultFormats();
    
    juce::String errorMessage;
    return formatManager.createPluginInstance(description, sampleRate, blockSize, errorMessage);
}

std::unique_ptr<PluginSandbox> SandboxFactory::createSandbox(
    const juce::PluginDescription& description,
    const PluginSandbox::Configuration& config)
{
    return std::make_unique<PluginSandbox>(description, config);
}

bool SandboxFactory::shouldSandbox(const juce::PluginDescription& description)
{
    // Assess risk level
    int riskLevel = assessPluginRisk(description);
    
    // Sandbox high-risk plugins
    return riskLevel > 5;
}

int SandboxFactory::assessPluginRisk(const juce::PluginDescription& description)
{
    int risk = 0;
    
    // Unknown manufacturer
    if (description.manufacturerName.isEmpty() || 
        description.manufacturerName == "Unknown")
    {
        risk += 3;
    }
    
    // Old plugin version
    if (description.version.containsIgnoreCase("1.0") ||
        description.version.isEmpty())
    {
        risk += 2;
    }
    
    // VST2 plugins are generally less stable
    if (description.pluginFormatName == "VST")
    {
        risk += 2;
    }
    
    // Check for known problematic plugins
    juce::StringArray problematicPlugins = {
        "CrashyPlugin",
        "UnstableSynth",
        // Add known problematic plugins here
    };
    
    for (const auto& problematic : problematicPlugins)
    {
        if (description.name.containsIgnoreCase(problematic))
        {
            risk += 5;
        }
    }
    
    return risk;
}

} // namespace HAM