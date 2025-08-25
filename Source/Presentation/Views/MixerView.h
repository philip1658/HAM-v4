/*
  ==============================================================================

    MixerView.h
    Direct plugin management UI - based on PulseProject's working implementation
    Simplified architecture for immediate functionality

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../../Infrastructure/Audio/HAMAudioProcessor.h"
#include "../../Infrastructure/Plugins/PluginManager.h"
#include <memory>
#include <vector>

namespace HAM::UI
{

// Forward declaration
class LightweightPluginBrowser;

/**
 * MixerView - Direct plugin management without complex layers
 * Based on PulseProject's successful implementation
 */
class MixerView : public juce::Component,
                   public juce::Timer
{
public:
    explicit MixerView(HAMAudioProcessor& processor);
    ~MixerView() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;
    
private:
    /**
     * ChannelStrip - Single track with direct plugin loading
     * Mirrors PulseProject's approach
     */
    class ChannelStrip : public juce::Component,
                         public juce::Button::Listener,
                         public juce::Slider::Listener
    {
    public:
        ChannelStrip(int channelIndex, HAMAudioProcessor& processor, MixerView* parent);
        ~ChannelStrip() override = default;
        
        void paint(juce::Graphics& g) override;
        void resized() override;
        
        // UI callbacks
        void buttonClicked(juce::Button* button) override;
        void sliderValueChanged(juce::Slider* slider) override;
        
        // Update plugin display after loading
        void updatePluginDisplay(const juce::String& pluginName);
        
        // Get channel index
        int getChannelIndex() const { return m_channelIndex; }
        
    private:
        int m_channelIndex;
        HAMAudioProcessor& m_processor;
        MixerView* m_parent;
        
        // UI Components
        juce::Label m_channelLabel;
        juce::TextButton m_pluginSlot;
        juce::TextButton m_deleteButton{"X"};
        juce::TextButton m_bypassButton{"B"};
        juce::TextButton m_editButton{"E"};
        juce::Slider m_volumeSlider;
        juce::Slider m_panSlider;
        juce::TextButton m_muteButton{"M"};
        juce::TextButton m_soloButton{"S"};
        
        // State
        bool m_hasPlugin{false};
        juce::String m_loadedPluginName;
        
        // Helper methods
        void showPluginEditor();
        juce::Colour getTrackColor() const;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelStrip)
    };
    
    // Browser management - single instance like Pulse
    class BrowserWindowManager
    {
    public:
        BrowserWindowManager() = default;
        ~BrowserWindowManager();
        
        // Create or show existing browser window
        juce::DocumentWindow* createBrowserWindow();
        
        // Get browser component
        LightweightPluginBrowser* getBrowser() const;
        
        // Close browser
        void closeBrowser();
        
    private:
        std::unique_ptr<juce::DocumentWindow> m_browserWindow;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BrowserWindowManager)
    };
    
    // Main processor reference
    HAMAudioProcessor& m_processor;
    
    // Channel strips
    std::vector<std::unique_ptr<ChannelStrip>> m_channelStrips;
    
    // Browser manager
    BrowserWindowManager m_browserManager;
    
    // Layout components
    juce::Viewport m_viewport;
    juce::Component m_channelContainer;
    
    // Master section
    juce::Label m_masterLabel{"", "MASTER"};
    juce::Slider m_masterVolume;
    
    // Helper methods
    void createChannelStrips();
    void layoutChannels();
    
    // Called when plugin is selected in browser
    void onPluginSelected(int channelIndex, const juce::PluginDescription& desc);
    
    // Direct plugin loading - bypass complex architecture
    bool loadPluginDirect(int channelIndex, const juce::PluginDescription& desc);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerView)
};

/**
 * LightweightPluginBrowser - Simple plugin browser with cached list
 * Based on PulseProject's implementation
 */
class LightweightPluginBrowser : public juce::Component,
                                 public juce::ListBoxModel,
                                 private juce::Timer
{
public:
    LightweightPluginBrowser();
    ~LightweightPluginBrowser() override;
    
    // Callback when plugin is selected
    std::function<void(const juce::PluginDescription&)> onPluginSelected;
    
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // ListBoxModel overrides
    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics& g, 
                         int width, int height, bool rowIsSelected) override;
    void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override;
    
private:
    // UI Components
    juce::TextEditor m_searchBox;
    juce::ComboBox m_typeFilter;
    juce::ListBox m_pluginList;
    juce::TextButton m_loadButton{"Load Plugin"};
    juce::TextButton m_scanButton{"Rescan"};
    juce::Label m_statusLabel;
    
    // Plugin data
    juce::Array<juce::PluginDescription> m_allPlugins;
    juce::Array<juce::PluginDescription> m_filteredPlugins;
    
    // Methods
    void loadCachedPlugins();
    void filterPlugins();
    void loadSelectedPlugin();
    void startPluginScan();
    
    // Timer for scan updates
    void timerCallback() override;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LightweightPluginBrowser)
};

} // namespace HAM::UI