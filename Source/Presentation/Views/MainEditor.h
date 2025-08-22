/*
  ==============================================================================

    MainEditor.h
    Main editor window for HAM sequencer
    Placeholder for future UI implementation

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

namespace HAM
{

class HAMAudioProcessor;

/**
 * Main editor window for HAM sequencer
 * This is a placeholder that will be expanded as the UI is
 * connected to the audio processor and the HAM Editor panel is added.
 */
class MainEditor : public juce::AudioProcessorEditor
{
public:
    MainEditor(HAMAudioProcessor& processor)
        : AudioProcessorEditor(processor)
    {
        setSize(1200, 600);
    }
    
    ~MainEditor() override = default;
    
    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::black);
        g.setColour(juce::Colours::white);
        g.drawText("HAM Sequencer - Audio Processor Connected", 
                   getLocalBounds(), juce::Justification::centred);
    }
    
    void resized() override {}
    
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainEditor)
};

} // namespace HAM