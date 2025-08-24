/*
  ==============================================================================

    PluginBrowser.h
    Plugin browser UI component for selecting VST/AU plugins
    
    This is a stub implementation to fix compilation.
    Full implementation pending.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <functional>

namespace HAM::UI
{

/**
 * PluginBrowser - UI for browsing and selecting audio plugins
 * 
 * This is a basic stub to allow compilation.
 * TODO: Implement full plugin browsing functionality
 */
class PluginBrowser : public juce::Component
{
public:
    PluginBrowser()
    {
        // TODO: Implement plugin scanning and listing
        setSize(600, 400);
    }
    
    ~PluginBrowser() override = default;
    
    // Callback when a plugin is chosen
    std::function<void(const juce::PluginDescription&)> onPluginChosen;
    
    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff1e1e1e));
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(juce::FontOptions(16.0f)));
        g.drawText("Plugin Browser (Not Implemented)", 
                   getLocalBounds(), 
                   juce::Justification::centred);
    }
    
    void resized() override
    {
        // TODO: Layout plugin list and controls
    }
    
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginBrowser)
};

} // namespace HAM::UI