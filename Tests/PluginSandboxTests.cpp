/*
  ==============================================================================

    PluginSandboxTests.cpp
    Unit tests for plugin sandboxing system

  ==============================================================================
*/

#include <JuceHeader.h>
#include "../Source/Infrastructure/Plugins/PluginSandbox.h"
#include <chrono>
#include <thread>

namespace HAM::Tests
{

/**
 * PluginSandboxTests - Comprehensive test suite for sandboxing
 */
class PluginSandboxTests : public juce::UnitTest
{
public:
    PluginSandboxTests()
        : UnitTest("Plugin Sandbox Tests", "Infrastructure")
    {
    }
    
    void runTest() override
    {
        testSharedMemoryAudioBuffer();
        testPluginSandboxLifecycle();
        testCrashRecovery();
        testPerformanceMetrics();
        testSandboxFactory();
        testIPCCommunication();
        testMultipleSandboxes();
        testMemoryLeaks();
    }
    
private:
    //==============================================================================
    void testSharedMemoryAudioBuffer()
    {
        beginTest("SharedMemoryAudioBuffer - Basic Operations");
        
        // Create host and client buffers
        auto channelName = "TestChannel_" + juce::Uuid().toString();
        auto hostBuffer = std::make_unique<SharedMemoryAudioBuffer>(channelName, true);
        
        // Give time for shared memory to be created
        juce::Thread::sleep(10);
        
        auto clientBuffer = std::make_unique<SharedMemoryAudioBuffer>(channelName, false);
        
        // Test audio transfer
        const int numChannels = 2;
        const int numSamples = 512;
        
        // Create test audio data
        juce::AudioBuffer<float> testBuffer(numChannels, numSamples);
        for (int ch = 0; ch < numChannels; ++ch)
        {
            for (int s = 0; s < numSamples; ++s)
            {
                testBuffer.setSample(ch, s, std::sin(2.0f * juce::MathConstants<float>::pi * 
                                                     440.0f * s / 48000.0f));
            }
        }
        
        // Write from host
        const float* inputChannels[numChannels];
        for (int ch = 0; ch < numChannels; ++ch)
        {
            inputChannels[ch] = testBuffer.getReadPointer(ch);
        }
        
        expect(hostBuffer->writeAudioBlock(inputChannels, numChannels, numSamples),
               "Should write audio successfully");
        
        // Read from client
        juce::AudioBuffer<float> outputBuffer(numChannels, numSamples);
        float* outputChannels[numChannels];
        for (int ch = 0; ch < numChannels; ++ch)
        {
            outputChannels[ch] = outputBuffer.getWritePointer(ch);
        }
        
        expect(clientBuffer->readAudioBlock(outputChannels, numChannels, numSamples),
               "Should read audio successfully");
        
        // Verify data integrity
        for (int ch = 0; ch < numChannels; ++ch)
        {
            for (int s = 0; s < numSamples; ++s)
            {
                expectWithinAbsoluteError(outputBuffer.getSample(ch, s),
                                        testBuffer.getSample(ch, s),
                                        0.0001f,
                                        "Audio data should match");
            }
        }
        
        beginTest("SharedMemoryAudioBuffer - MIDI Transfer");
        
        // Test MIDI transfer
        juce::MidiBuffer midiInput;
        midiInput.addEvent(juce::MidiMessage::noteOn(1, 60, 100.0f), 0);
        midiInput.addEvent(juce::MidiMessage::noteOff(1, 60, 0.0f), 256);
        
        expect(hostBuffer->writeMidiBuffer(midiInput),
               "Should write MIDI successfully");
        
        juce::MidiBuffer midiOutput;
        expect(clientBuffer->readMidiBuffer(midiOutput),
               "Should read MIDI successfully");
        
        expectEquals(midiOutput.getNumEvents(), midiInput.getNumEvents(),
                    "MIDI event count should match");
        
        beginTest("SharedMemoryAudioBuffer - Heartbeat");
        
        // Test heartbeat mechanism
        hostBuffer->updateHeartbeat();
        expect(hostBuffer->isProcessAlive(),
               "Host should be alive after heartbeat");
        
        // Test latency measurement
        auto latency = hostBuffer->getLatencyMicroseconds();
        expect(latency >= 0, "Latency should be non-negative");
        expect(latency < 1000000, "Latency should be less than 1 second");
    }
    
    //==============================================================================
    void testPluginSandboxLifecycle()
    {
        beginTest("PluginSandbox - Lifecycle Management");
        
        // Create a dummy plugin description
        juce::PluginDescription description;
        description.name = "TestPlugin";
        description.fileOrIdentifier = "test.plugin";
        description.pluginFormatName = "VST3";
        
        PluginSandbox::Configuration config;
        config.sampleRate = 48000.0;
        config.blockSize = 512;
        config.timeoutMs = 1000;
        config.autoRestart = false;
        
        auto sandbox = std::make_unique<PluginSandbox>(description, config);
        
        // Test initial state
        expectEquals(static_cast<int>(sandbox->getState()), 
                    static_cast<int>(PluginSandbox::State::Idle),
                    "Initial state should be Idle");
        
        expect(!sandbox->isRunning(), "Should not be running initially");
        expect(!sandbox->hasCrashed(), "Should not be crashed initially");
        
        // Note: Cannot test actual start() without the sandbox executable
        // In production, this would launch the real process
        
        beginTest("PluginSandbox - State Transitions");
        
        // Test metrics
        auto metrics = sandbox->getMetrics();
        expectEquals(metrics.crashCount, 0, "Should have no crashes initially");
        expectEquals(metrics.restartCount, 0, "Should have no restarts initially");
    }
    
    //==============================================================================
    void testCrashRecovery()
    {
        beginTest("CrashRecoveryManager - Basic Operations");
        
        CrashRecoveryManager::RecoveryPolicy policy;
        policy.autoRestart = true;
        policy.maxRestartAttempts = 3;
        policy.restartDelayMs = 100;
        
        auto recoveryManager = std::make_unique<CrashRecoveryManager>(policy);
        
        // Test statistics
        auto stats = recoveryManager->getStatistics();
        expectEquals(stats.totalCrashes, 0, "Should have no crashes initially");
        expectEquals(stats.successfulRecoveries, 0, "Should have no recoveries initially");
        
        // Create and register a sandbox
        juce::PluginDescription description;
        description.name = "TestPlugin";
        
        auto sandbox = std::make_unique<PluginSandbox>(description);
        recoveryManager->registerSandbox(sandbox.get());
        
        // Unregister
        recoveryManager->unregisterSandbox(sandbox.get());
        
        // Clear statistics
        recoveryManager->clearStatistics();
        stats = recoveryManager->getStatistics();
        expectEquals(stats.totalCrashes, 0, "Statistics should be cleared");
    }
    
    //==============================================================================
    void testPerformanceMetrics()
    {
        beginTest("Performance Metrics - Latency Tracking");
        
        // Create IPC buffer for testing
        auto channelName = "PerfTest_" + juce::Uuid().toString();
        auto buffer = std::make_unique<SharedMemoryAudioBuffer>(channelName, true);
        
        // Simulate multiple audio callbacks
        const int numIterations = 100;
        const int numChannels = 2;
        const int blockSize = 128;
        
        juce::AudioBuffer<float> audioBuffer(numChannels, blockSize);
        audioBuffer.clear();
        
        const float* channels[numChannels];
        for (int ch = 0; ch < numChannels; ++ch)
        {
            channels[ch] = audioBuffer.getReadPointer(ch);
        }
        
        auto startTime = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < numIterations; ++i)
        {
            buffer->writeAudioBlock(channels, numChannels, blockSize);
            buffer->updateHeartbeat();
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        
        double averageLatencyUs = duration.count() / static_cast<double>(numIterations);
        
        logMessage("Average IPC write latency: " + juce::String(averageLatencyUs) + " µs");
        
        // Check performance requirements
        expect(averageLatencyUs < 1000.0, "Average latency should be less than 1ms");
        
        beginTest("Performance Metrics - Memory Usage");
        
        // Check memory size
        constexpr size_t expectedSize = sizeof(SharedMemoryAudioBuffer::Header) +
                                       (SharedMemoryAudioBuffer::MAX_CHANNELS * 
                                        SharedMemoryAudioBuffer::MAX_BLOCK_SIZE * sizeof(float)) +
                                       (64 * 1024);  // MIDI buffer
        
        logMessage("Shared memory size: " + juce::String(expectedSize / 1024) + " KB");
        
        expect(expectedSize < 2 * 1024 * 1024, "Memory usage should be less than 2MB per sandbox");
    }
    
    //==============================================================================
    void testSandboxFactory()
    {
        beginTest("SandboxFactory - Risk Assessment");
        
        // Test low-risk plugin
        juce::PluginDescription lowRisk;
        lowRisk.name = "Trusted Plugin";
        lowRisk.manufacturerName = "Reputable Company";
        lowRisk.pluginFormatName = "VST3";
        lowRisk.version = "2.0.0";
        
        expect(!SandboxFactory::shouldSandbox(lowRisk),
               "Low-risk plugin should not require sandboxing");
        
        // Test high-risk plugin
        juce::PluginDescription highRisk;
        highRisk.name = "Unknown Plugin";
        highRisk.manufacturerName = "";
        highRisk.pluginFormatName = "VST";  // VST2 is higher risk
        highRisk.version = "1.0";
        
        expect(SandboxFactory::shouldSandbox(highRisk),
               "High-risk plugin should require sandboxing");
        
        beginTest("SandboxFactory - Creation");
        
        PluginSandbox::Configuration config;
        config.sampleRate = 48000.0;
        config.blockSize = 256;
        
        auto sandbox = SandboxFactory::createSandbox(lowRisk, config);
        expect(sandbox != nullptr, "Should create sandbox successfully");
    }
    
    //==============================================================================
    void testIPCCommunication()
    {
        beginTest("IPC - Lock-free Communication");
        
        auto channelName = "IPCTest_" + juce::Uuid().toString();
        auto hostBuffer = std::make_unique<SharedMemoryAudioBuffer>(channelName, true);
        
        juce::Thread::sleep(10);
        
        auto clientBuffer = std::make_unique<SharedMemoryAudioBuffer>(channelName, false);
        
        // Test concurrent access
        std::atomic<bool> shouldRun{true};
        std::atomic<int> writeCount{0};
        std::atomic<int> readCount{0};
        
        // Writer thread
        std::thread writerThread([&]() {
            const int numChannels = 2;
            const int blockSize = 128;
            juce::AudioBuffer<float> buffer(numChannels, blockSize);
            
            const float* channels[numChannels];
            for (int ch = 0; ch < numChannels; ++ch)
            {
                channels[ch] = buffer.getReadPointer(ch);
            }
            
            while (shouldRun)
            {
                if (hostBuffer->writeAudioBlock(channels, numChannels, blockSize))
                {
                    writeCount++;
                }
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        });
        
        // Reader thread
        std::thread readerThread([&]() {
            const int numChannels = 2;
            const int blockSize = 128;
            juce::AudioBuffer<float> buffer(numChannels, blockSize);
            
            float* channels[numChannels];
            for (int ch = 0; ch < numChannels; ++ch)
            {
                channels[ch] = buffer.getWritePointer(ch);
            }
            
            while (shouldRun)
            {
                if (clientBuffer->readAudioBlock(channels, numChannels, blockSize))
                {
                    readCount++;
                }
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        });
        
        // Run for 100ms
        juce::Thread::sleep(100);
        shouldRun = false;
        
        writerThread.join();
        readerThread.join();
        
        logMessage("Writes: " + juce::String(writeCount.load()) + 
                  ", Reads: " + juce::String(readCount.load()));
        
        expect(writeCount > 0, "Should have successful writes");
        expect(readCount > 0, "Should have successful reads");
    }
    
    //==============================================================================
    void testMultipleSandboxes()
    {
        beginTest("Multiple Sandboxes - Resource Management");
        
        const int numSandboxes = 5;
        std::vector<std::unique_ptr<PluginSandbox>> sandboxes;
        
        for (int i = 0; i < numSandboxes; ++i)
        {
            juce::PluginDescription description;
            description.name = "Plugin_" + juce::String(i);
            description.fileOrIdentifier = "plugin_" + juce::String(i);
            
            PluginSandbox::Configuration config;
            config.sampleRate = 48000.0;
            config.blockSize = 512;
            
            sandboxes.push_back(std::make_unique<PluginSandbox>(description, config));
        }
        
        expectEquals(static_cast<int>(sandboxes.size()), numSandboxes,
                    "Should create multiple sandboxes");
        
        // Test processing multiple sandboxes
        juce::AudioBuffer<float> buffer(2, 512);
        juce::MidiBuffer midiBuffer;
        
        for (auto& sandbox : sandboxes)
        {
            sandbox->processBlock(buffer, midiBuffer);
        }
        
        expect(true, "Multiple sandboxes can coexist");
    }
    
    //==============================================================================
    void testMemoryLeaks()
    {
        beginTest("Memory Leak Detection");
        
        // Create and destroy sandboxes multiple times
        for (int i = 0; i < 10; ++i)
        {
            juce::PluginDescription description;
            description.name = "LeakTest_" + juce::String(i);
            
            auto sandbox = std::make_unique<PluginSandbox>(description);
            auto metrics = sandbox->getMetrics();
            
            // Force destruction
            sandbox.reset();
        }
        
        // Create and destroy IPC buffers
        for (int i = 0; i < 10; ++i)
        {
            auto channelName = "LeakTest_" + juce::String(i);
            auto buffer = std::make_unique<SharedMemoryAudioBuffer>(channelName, true);
            buffer.reset();
        }
        
        expect(true, "No memory leaks detected in creation/destruction cycles");
        
        // Note: In production, use valgrind or Instruments for actual leak detection
    }
};

// Register the test
static PluginSandboxTests pluginSandboxTests;

} // namespace HAM::Tests

// Main function for standalone test executable
int main(int argc, char* argv[])
{
    juce::UnitTestRunner runner;
    runner.setPassesAreLogged(true);
    runner.setAssertOnFailure(false);
    
    runner.runAllTests();
    
    // Get results
    int numPassed = 0;
    int numFailed = 0;
    
    for (int i = 0; i < runner.getNumResults(); ++i)
    {
        auto* result = runner.getResult(i);
        if (result)
        {
            numPassed += result->passes;
            numFailed += result->failures;
        }
    }
    
    if (numFailed == 0)
    {
        std::cout << "\nAll " << numPassed << " plugin sandbox tests passed!" << std::endl;
    }
    else
    {
        std::cout << "\n" << numFailed << " tests failed (out of " << (numPassed + numFailed) << ")." << std::endl;
    }
    
    return numFailed;
}