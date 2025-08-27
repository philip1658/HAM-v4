/*
  ==============================================================================

    PluginSandboxHostMain.cpp
    Separate process for hosting sandboxed plugins
    This executable runs plugins in isolation to prevent crashes

  ==============================================================================
*/

#include <JuceHeader.h>
#include "../../Source/Infrastructure/Plugins/PluginSandbox.h"

//==============================================================================
/**
 * SandboxHostApplication - Main application for plugin sandbox process
 */
class SandboxHostApplication : public juce::JUCEApplication
{
public:
    SandboxHostApplication() = default;
    
    const juce::String getApplicationName() override { return "HAM Plugin Sandbox Host"; }
    const juce::String getApplicationVersion() override { return "1.0.0"; }
    bool moreThanOneInstanceAllowed() override { return true; }
    
    void initialise(const juce::String& commandLine) override
    {
        // Parse command line arguments
        juce::StringArray args = juce::StringArray::fromTokens(commandLine, true);
        
        juce::String pluginPath;
        juce::String ipcChannelName;
        double sampleRate = 48000.0;
        int blockSize = 512;
        bool useRosetta = false;
        
        for (int i = 0; i < args.size(); ++i)
        {
            if (args[i] == "--plugin" && i + 1 < args.size())
            {
                pluginPath = args[++i];
            }
            else if (args[i] == "--ipc" && i + 1 < args.size())
            {
                ipcChannelName = args[++i];
            }
            else if (args[i] == "--samplerate" && i + 1 < args.size())
            {
                sampleRate = args[++i].getDoubleValue();
            }
            else if (args[i] == "--blocksize" && i + 1 < args.size())
            {
                blockSize = args[++i].getIntValue();
            }
            else if (args[i] == "--rosetta")
            {
                useRosetta = true;
            }
        }
        
        // Validate arguments
        if (pluginPath.isEmpty() || ipcChannelName.isEmpty())
        {
            DBG("PluginSandboxHost: Missing required arguments");
            DBG("Usage: PluginSandboxHost --plugin <path> --ipc <channel> [--samplerate <rate>] [--blocksize <size>]");
            quit();
            return;
        }
        
        DBG("PluginSandboxHost: Starting with plugin: " << pluginPath);
        DBG("  IPC Channel: " << ipcChannelName);
        DBG("  Sample Rate: " << sampleRate);
        DBG("  Block Size: " << blockSize);
        
        // Create the sandboxed host
        m_host = std::make_unique<HAM::SandboxedPluginHost>();
        
        if (!m_host->initialise(pluginPath, ipcChannelName))
        {
            DBG("PluginSandboxHost: Failed to initialize");
            quit();
            return;
        }
        
        // Prepare audio
        m_host->prepareToPlay(sampleRate, blockSize);
        
        // Create a dummy audio device manager for processing
        m_deviceManager = std::make_unique<juce::AudioDeviceManager>();
        
        // Set up audio callback
        m_audioCallback = std::make_unique<AudioCallback>(*m_host);
        
        // Initialize with null audio (we're just processing, not playing)
        auto result = m_deviceManager->initialise(
            2,     // input channels
            2,     // output channels
            nullptr,  // no saved state
            true,     // select default device
            {},       // preferred device
            nullptr   // no setup options
        );
        
        if (result.isNotEmpty())
        {
            DBG("PluginSandboxHost: Audio initialization warning: " << result);
            // Continue anyway - we're mainly interested in processing
        }
        
        // Add audio callback
        m_deviceManager->addAudioCallback(m_audioCallback.get());
        
        // Start processing loop
        m_processThread = std::make_unique<ProcessThread>(*m_host, blockSize);
        m_processThread->startThread(juce::Thread::Priority::high);
        
        DBG("PluginSandboxHost: Running...");
    }
    
    void shutdown() override
    {
        DBG("PluginSandboxHost: Shutting down");
        
        // Stop processing
        if (m_processThread)
        {
            m_processThread->stopThread(1000);
        }
        
        // Remove audio callback
        if (m_deviceManager && m_audioCallback)
        {
            m_deviceManager->removeAudioCallback(m_audioCallback.get());
        }
        
        // Clean up
        m_audioCallback.reset();
        m_deviceManager.reset();
        m_processThread.reset();
        m_host.reset();
    }
    
    void systemRequestedQuit() override
    {
        quit();
    }
    
private:
    //==============================================================================
    /**
     * AudioCallback - Handles audio device callbacks
     */
    class AudioCallback : public juce::AudioIODeviceCallback
    {
    public:
        explicit AudioCallback(HAM::SandboxedPluginHost& host)
            : m_host(host)
        {
        }
        
        void audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                             int numInputChannels,
                                             float* const* outputChannelData,
                                             int numOutputChannels,
                                             int numSamples,
                                             const juce::AudioIODeviceCallbackContext& context) override
        {
            // Create buffers
            juce::AudioBuffer<float> buffer(numOutputChannels, numSamples);
            
            // Copy input if available
            if (numInputChannels > 0 && inputChannelData != nullptr)
            {
                for (int ch = 0; ch < std::min(numInputChannels, numOutputChannels); ++ch)
                {
                    buffer.copyFrom(ch, 0, inputChannelData[ch], numSamples);
                }
            }
            
            // Process
            juce::MidiBuffer midiBuffer;
            m_host.processBlock(buffer, midiBuffer);
            
            // Copy to output
            for (int ch = 0; ch < numOutputChannels; ++ch)
            {
                if (outputChannelData[ch] != nullptr)
                {
                    std::memcpy(outputChannelData[ch], buffer.getReadPointer(ch), 
                              numSamples * sizeof(float));
                }
            }
        }
        
        void audioDeviceAboutToStart(juce::AudioIODevice* device) override
        {
            if (device)
            {
                m_host.prepareToPlay(device->getCurrentSampleRate(),
                                   device->getCurrentBufferSizeSamples());
            }
        }
        
        void audioDeviceStopped() override
        {
            m_host.releaseResources();
        }
        
    private:
        HAM::SandboxedPluginHost& m_host;
    };
    
    //==============================================================================
    /**
     * ProcessThread - Main processing thread for the sandbox
     */
    class ProcessThread : public juce::Thread
    {
    public:
        ProcessThread(HAM::SandboxedPluginHost& host, int blockSize)
            : Thread("SandboxProcessThread")
            , m_host(host)
            , m_blockSize(blockSize)
        {
        }
        
        void run() override
        {
            // Set thread priority for real-time audio
            setPriority(Priority::high);
            
            // Create processing buffer
            juce::AudioBuffer<float> buffer(2, m_blockSize);
            juce::MidiBuffer midiBuffer;
            
            while (!threadShouldExit())
            {
                // Process audio block through IPC
                m_host.processBlock(buffer, midiBuffer);
                
                // Small sleep to prevent CPU spinning
                // In real implementation, this would be driven by audio callbacks
                wait(1);
            }
        }
        
    private:
        HAM::SandboxedPluginHost& m_host;
        int m_blockSize;
    };
    
    //==============================================================================
    std::unique_ptr<HAM::SandboxedPluginHost> m_host;
    std::unique_ptr<juce::AudioDeviceManager> m_deviceManager;
    std::unique_ptr<AudioCallback> m_audioCallback;
    std::unique_ptr<ProcessThread> m_processThread;
};

//==============================================================================
// Main entry point

START_JUCE_APPLICATION(SandboxHostApplication)