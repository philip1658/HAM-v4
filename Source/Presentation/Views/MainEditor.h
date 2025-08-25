/*
  ==============================================================================

    MainEditor.h
    Main editor window for HAM sequencer
    Bridges AudioProcessor with the MainComponent UI system

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../../MainComponent.h"
#include "../Core/AppController.h"
#include "../Core/UICoordinator.h"
#include "Infrastructure/Audio/HAMAudioProcessor.h"

namespace HAM
{

/**
 * Main editor window for HAM sequencer AudioProcessor
 * This bridges the AudioProcessor with the existing MainComponent UI system
 */
class MainEditor : public juce::AudioProcessorEditor
{
public:
    MainEditor(HAMAudioProcessor& processor)
        : AudioProcessorEditor(processor)
        , m_processor(processor)
    {
        // Create the main UI component
        m_mainComponent = std::make_unique<MainComponent>();
        addAndMakeVisible(m_mainComponent.get());
        
        // Set initial size (can be made resizable later)
        setSize(1200, 800);
        setResizable(true, true);
        setResizeLimits(800, 600, 2400, 1600);
        
        // Connect the UI to the processor
        connectToProcessor();
        
        // Connect the processor to the UICoordinator for MixerView
        if (m_mainComponent->getUICoordinator())
        {
            m_mainComponent->getUICoordinator()->setAudioProcessor(&processor);
        }
    }
    
    ~MainEditor() override
    {
        // Clean disconnect from processor
        disconnectFromProcessor();
    }
    
    void paint(juce::Graphics& g) override
    {
        // Background is handled by MainComponent
        g.fillAll(juce::Colours::black);
    }
    
    void resized() override
    {
        if (m_mainComponent)
        {
            m_mainComponent->setBounds(getLocalBounds());
        }
    }
    
private:
    void connectToProcessor()
    {
        // Future: Connect AppController to HAMAudioProcessor message system
        // This will allow the UI to send messages to the audio processor
        // and receive status updates back
    }
    
    void disconnectFromProcessor()
    {
        // Future: Disconnect from processor message system
    }
    
    HAMAudioProcessor& m_processor;
    std::unique_ptr<MainComponent> m_mainComponent;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainEditor)
};

} // namespace HAM