/*
  ==============================================================================

    MainComponent.h
    Main component for HAM sequencer with Pulse UI integration

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <memory>

//==============================================================================
/**
 * Main component for HAM sequencer
 * Uses Pulse UI component library with message-based architecture
 */
class MainComponent : public juce::Component
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    //==============================================================================
    void paint(juce::Graphics&) override;
    void paintOverChildren(juce::Graphics&) override;
    void resized() override;

private:
    // Forward declaration for implementation
    class Impl;
    std::unique_ptr<Impl> m_impl;
    
    // Look and feel
    juce::LookAndFeel_V4 m_pulseLookAndFeel;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};