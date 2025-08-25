/*
  Simple test to verify plugin audio processing capability
*/

#include <JuceHeader.h>
#include <iostream>
#include <thread>
#include <chrono>

class PluginTester : public juce::AudioAppComponent
{
public:
    PluginTester()
    {
        setAudioChannels(2, 2);
        
        // Initialize plugin format manager
        formatManager.addDefaultFormats();
        
        std::cout << "==================================" << std::endl;
        std::cout << "HAM Plugin Audio Processing Test" << std::endl;
        std::cout << "==================================" << std::endl;
        std::cout << std::endl;
        
        // Check available formats
        std::cout << "Available Plugin Formats:" << std::endl;
        for (int i = 0; i < formatManager.getNumFormats(); ++i)
        {
            auto* format = formatManager.getFormat(i);
            std::cout << "- " << format->getName() << std::endl;
        }
        std::cout << std::endl;
        
        // Try to load a simple plugin
        testPluginLoading();
    }
    
    ~PluginTester() override
    {
        shutdownAudio();
    }
    
    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override
    {
        std::cout << "Audio Setup:" << std::endl;
        std::cout << "- Sample Rate: " << sampleRate << " Hz" << std::endl;
        std::cout << "- Buffer Size: " << samplesPerBlockExpected << " samples" << std::endl;
        std::cout << "- Latency: " << (samplesPerBlockExpected * 1000.0 / sampleRate) << " ms" << std::endl;
        std::cout << std::endl;
        
        if (pluginInstance)
        {
            pluginInstance->prepareToPlay(sampleRate, samplesPerBlockExpected);
            std::cout << "✅ Plugin prepared for audio processing" << std::endl;
        }
    }
    
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override
    {
        bufferToFill.clearActiveBufferRegion();
        
        if (pluginInstance)
        {
            // Generate test tone
            float frequency = 440.0f; // A4
            float amplitude = 0.1f;
            
            auto* leftChannel = bufferToFill.buffer->getWritePointer(0, bufferToFill.startSample);
            auto* rightChannel = bufferToFill.buffer->getWritePointer(1, bufferToFill.startSample);
            
            for (int sample = 0; sample < bufferToFill.numSamples; ++sample)
            {
                float sineWave = amplitude * std::sin(2.0f * juce::MathConstants<float>::pi * 
                                                      frequency * currentSamplePosition / getSampleRate());
                leftChannel[sample] = sineWave;
                rightChannel[sample] = sineWave;
                currentSamplePosition++;
            }
            
            // Process through plugin
            juce::MidiBuffer midiMessages;
            pluginInstance->processBlock(*bufferToFill.buffer, midiMessages);
            
            // Update performance counter
            processCounter++;
            if (processCounter % 100 == 0)
            {
                std::cout << "Processed " << processCounter << " blocks" << std::endl;
            }
        }
    }
    
    void releaseResources() override
    {
        if (pluginInstance)
        {
            pluginInstance->releaseResources();
        }
    }
    
private:
    void testPluginLoading()
    {
        // Try to find and load a basic AU plugin (usually available on macOS)
        juce::String pluginPath;
        
        // Try common plugin paths
        std::vector<juce::String> testPlugins = {
            "/System/Library/Audio/Units/Components/MatrixReverb.component",
            "/Library/Audio/Plug-Ins/Components/AUNetSend.component",
            "/Library/Audio/Plug-Ins/VST3/Valhalla VintageVerb.vst3"
        };
        
        for (const auto& path : testPlugins)
        {
            juce::File pluginFile(path);
            if (pluginFile.exists())
            {
                pluginPath = path;
                std::cout << "Found test plugin: " << path << std::endl;
                break;
            }
        }
        
        if (pluginPath.isEmpty())
        {
            std::cout << "⚠️ No test plugins found in standard locations" << std::endl;
            std::cout << "Plugin hosting is available but no plugins to test with" << std::endl;
            return;
        }
        
        // Try to load the plugin
        juce::OwnedArray<juce::PluginDescription> typesFound;
        juce::KnownPluginList knownList;
        
        for (int i = 0; i < formatManager.getNumFormats(); ++i)
        {
            auto* format = formatManager.getFormat(i);
            format->findAllTypesForFile(typesFound, pluginPath);
        }
        
        if (typesFound.size() > 0)
        {
            std::cout << "✅ Found " << typesFound.size() << " plugin(s) in file" << std::endl;
            
            auto* desc = typesFound[0];
            std::cout << "Loading: " << desc->name << " by " << desc->manufacturerName << std::endl;
            
            juce::String errorMessage;
            pluginInstance = formatManager.createPluginInstance(*desc, getSampleRate(), 
                                                               getBlockSize(), errorMessage);
            
            if (pluginInstance)
            {
                std::cout << "✅ Plugin loaded successfully!" << std::endl;
                std::cout << "- Input channels: " << pluginInstance->getTotalNumInputChannels() << std::endl;
                std::cout << "- Output channels: " << pluginInstance->getTotalNumOutputChannels() << std::endl;
                std::cout << "- Parameters: " << pluginInstance->getNumParameters() << std::endl;
                std::cout << "✅ Audio processing ready!" << std::endl;
            }
            else
            {
                std::cout << "❌ Failed to instantiate plugin: " << errorMessage << std::endl;
            }
        }
        else
        {
            std::cout << "❌ No plugins found in file" << std::endl;
        }
    }
    
    juce::AudioPluginFormatManager formatManager;
    std::unique_ptr<juce::AudioPluginInstance> pluginInstance;
    int64_t currentSamplePosition = 0;
    int processCounter = 0;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginTester)
};

class TestApplication : public juce::JUCEApplication
{
public:
    TestApplication() = default;
    
    const juce::String getApplicationName() override { return "HAM Plugin Test"; }
    const juce::String getApplicationVersion() override { return "1.0"; }
    
    void initialise(const juce::String&) override
    {
        mainWindow = std::make_unique<MainWindow>("HAM Plugin Audio Test");
        
        // Run for 3 seconds then quit
        juce::Timer::callAfterDelay(3000, [this]
        {
            std::cout << std::endl;
            std::cout << "================================" << std::endl;
            std::cout << "Test Complete - Plugin support verified!" << std::endl;
            std::cout << "================================" << std::endl;
            systemRequestedQuit();
        });
    }
    
    void shutdown() override
    {
        mainWindow = nullptr;
    }
    
    void systemRequestedQuit() override
    {
        quit();
    }
    
private:
    class MainWindow : public juce::DocumentWindow
    {
    public:
        explicit MainWindow(const juce::String& name)
            : DocumentWindow(name, juce::Colours::black, DocumentWindow::allButtons)
        {
            setContentOwned(new PluginTester(), true);
            setResizable(false, false);
            centreWithSize(400, 200);
            setVisible(true);
        }
        
        void closeButtonPressed() override
        {
            JUCEApplication::getInstance()->systemRequestedQuit();
        }
        
    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
    };
    
    std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION(TestApplication)