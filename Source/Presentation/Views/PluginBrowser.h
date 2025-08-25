/*
  ==============================================================================

    PluginBrowser.h
    Plugin browser UI component for selecting VST/AU plugins
    Basic functional implementation

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <functional>
#include "../../Infrastructure/Plugins/PluginManager.h"

namespace HAM::UI
{

/**
 * PluginBrowser - UI for browsing and selecting audio plugins
 * Basic implementation with plugin list and categories
 */
class PluginBrowser : public juce::Component,
                      public juce::ListBoxModel,
                      private juce::Timer
{
public:
    PluginBrowser()
    {
        // Setup plugin list
        m_pluginList.setModel(this);
        m_pluginList.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff1e1e1e));
        addAndMakeVisible(m_pluginList);
        
        // Setup category filter
        m_categoryFilter.addItem("All Plugins", 1);
        m_categoryFilter.addItem("Effects", 2);
        m_categoryFilter.addItem("Instruments", 3);
        m_categoryFilter.addItem("Favorites", 4);
        m_categoryFilter.setSelectedId(1);
        m_categoryFilter.onChange = [this] { updateFilteredList(); };
        addAndMakeVisible(m_categoryFilter);
        
        // Setup search box
        m_searchBox.setTextToShowWhenEmpty("Search plugins...", juce::Colours::grey);
        m_searchBox.onTextChange = [this] { updateFilteredList(); };
        addAndMakeVisible(m_searchBox);
        
        // Setup scan button
        m_scanButton.setButtonText("Scan for Plugins");
        m_scanButton.onClick = [this] { scanForPlugins(); };
        addAndMakeVisible(m_scanButton);
        
        // Add close button
        m_closeButton.setButtonText("Close");
        m_closeButton.onClick = [this] { 
            if (onCloseRequested) onCloseRequested();
        };
        addAndMakeVisible(m_closeButton);
        
        setSize(600, 400);
        
        // Initialize PluginManager
        PluginManager::instance().initialise();
        
        // Load cached plugin list if available
        loadPluginList();
        
        // If no plugins found, start a scan automatically
        if (m_allPlugins.isEmpty())
        {
            DBG("No cached plugins found, starting initial scan...");
            scanForPlugins();
        }
        
        // Start timer to update plugin list during scan
        startTimer(100); // Update every 100ms for faster UI updates
    }
    
    ~PluginBrowser() override = default;
    
    // Callback when a plugin is chosen
    std::function<void(const juce::PluginDescription&)> onPluginChosen;
    
    // Callback when close is requested
    std::function<void()> onCloseRequested;
    
    // ListBoxModel implementation
    int getNumRows() override
    {
        return m_filteredPlugins.size();
    }
    
    void paintListBoxItem(int rowNumber, juce::Graphics& g,
                          int width, int height, bool rowIsSelected) override
    {
        if (rowNumber >= 0 && rowNumber < m_filteredPlugins.size())
        {
            const auto& plugin = m_filteredPlugins[rowNumber];
            
            if (rowIsSelected)
                g.fillAll(juce::Colour(0xff3a3a3a));
            
            g.setColour(juce::Colours::white);
            g.setFont(juce::Font(juce::FontOptions(14.0f)));
            
            // Draw plugin name and manufacturer
            g.drawText(plugin.name + " - " + plugin.manufacturerName,
                      10, 0, width - 20, height,
                      juce::Justification::centredLeft);
        }
    }
    
    void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override
    {
        if (row >= 0 && row < m_filteredPlugins.size() && onPluginChosen)
        {
            onPluginChosen(m_filteredPlugins[row]);
        }
    }
    
    void paint(juce::Graphics& g) override
    {
        // Background
        g.fillAll(juce::Colour(0xff1e1e1e));
        
        // Draw border frame
        g.setColour(juce::Colour(0xff3a3a3a));
        g.drawRect(getLocalBounds(), 2);
        
        // Draw inner highlight
        g.setColour(juce::Colour(0xff4a4a4a));
        g.drawRect(getLocalBounds().reduced(1), 1);
    }
    
    void resized() override
    {
        auto bounds = getLocalBounds();
        
        // Top controls
        auto topBar = bounds.removeFromTop(40);
        m_categoryFilter.setBounds(topBar.removeFromLeft(150).reduced(5));
        m_searchBox.setBounds(topBar.removeFromLeft(200).reduced(5));
        m_scanButton.setBounds(topBar.removeFromLeft(120).reduced(5));
        m_closeButton.setBounds(topBar.removeFromRight(80).reduced(5));
        
        // Plugin list takes remaining space
        m_pluginList.setBounds(bounds.reduced(5));
    }
    
    // Timer callback for scan progress and list updates
    void timerCallback() override
    {
        auto& pluginManager = PluginManager::instance();
        
        if (pluginManager.isScanning())
        {
            // Update progress
            auto progress = pluginManager.getProgress();
            m_scanButton.setButtonText("Scanning... " + juce::String(progress.scanned) + "/" + progress.current);
            
            // Reload plugin list to show newly found plugins
            loadPluginList();
        }
        else if (m_wasScanning)
        {
            // Scan just completed
            m_scanButton.setButtonText("Scan for Plugins");
            m_scanButton.setEnabled(true);
            
            // Final reload of plugin list
            loadPluginList();
            
            // Show result message
            DBG("Scan complete. Found " << m_allPlugins.size() << " plugins");
        }
        
        m_wasScanning = pluginManager.isScanning();
    }
    
private:
    void scanForPlugins()
    {
        // Use PluginManager for sandboxed scanning
        auto& pluginManager = PluginManager::instance();
        
        if (pluginManager.isScanning())
        {
            // Already scanning - show progress
            auto progress = pluginManager.getProgress();
            m_scanButton.setButtonText("Scanning... " + juce::String(progress.scanned) + "/" + juce::String(progress.total));
            return;
        }
        
        // Start async scan
        m_scanButton.setButtonText("Scanning...");
        m_scanButton.setEnabled(false);
        
        pluginManager.startSandboxedScan(true); // async scan
        
        // Start timer to update progress
        startTimer(100); // Update every 100ms
    }
    
    void updateFilteredList()
    {
        m_filteredPlugins.clear();
        
        auto searchText = m_searchBox.getText().toLowerCase();
        auto categoryId = m_categoryFilter.getSelectedId();
        
        for (const auto& plugin : m_allPlugins)
        {
            bool matchesSearch = searchText.isEmpty() ||
                                plugin.name.toLowerCase().contains(searchText) ||
                                plugin.manufacturerName.toLowerCase().contains(searchText);
            
            bool matchesCategory = (categoryId == 1) || // All
                                  (categoryId == 2 && !plugin.isInstrument) || // Effects
                                  (categoryId == 3 && plugin.isInstrument); // Instruments
            
            if (matchesSearch && matchesCategory)
            {
                m_filteredPlugins.add(plugin);
            }
        }
        
        m_pluginList.updateContent();
    }
    
    void loadPluginList()
    {
        // Load plugins from PluginManager's known list
        auto& pluginManager = PluginManager::instance();
        const auto& knownList = pluginManager.getKnownPluginList();
        
        m_allPlugins.clear();
        for (const auto& plugin : knownList.getTypes())
        {
            m_allPlugins.add(plugin);
        }
        
        updateFilteredList();
    }
    
    void savePluginList()
    {
        // Save plugin list to preferences for faster loading
        // This would normally save to a file or database
    }
    
    juce::ListBox m_pluginList;
    juce::ComboBox m_categoryFilter;
    juce::TextEditor m_searchBox;
    juce::TextButton m_scanButton;
    juce::TextButton m_closeButton;
    
    juce::Array<juce::PluginDescription> m_allPlugins;
    juce::Array<juce::PluginDescription> m_filteredPlugins;
    bool m_wasScanning = false;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginBrowser)
};

} // namespace HAM::UI