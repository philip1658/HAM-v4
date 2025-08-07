/*
  ==============================================================================

    MainComponent.h
    Main component for HAM sequencer

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
 * Main component for HAM sequencer
 * Will contain the full UI once implemented
 */
class MainComponent : public juce::Component
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    //==============================================================================
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    //==============================================================================
    // TODO: Add HAM sequencer components here
    // - Transport controls
    // - Stage grid
    // - Track sidebar
    // - Pattern manager
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};