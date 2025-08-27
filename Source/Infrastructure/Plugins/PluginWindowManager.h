/*
  ==============================================================================

    PluginWindowManager.h
    Manages plugin editor windows with proper lifecycle and positioning
    Ensures clean opening/closing and remembers window positions

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <memory>
#include <map>

namespace HAM
{

/**
 * PluginWindow - Custom window for hosting plugin editors
 * Handles proper cleanup and position saving
 */
class PluginWindow : public juce::DocumentWindow
{
public:
    using WindowID = std::pair<int, int>; // trackIndex, pluginIndex
    
    PluginWindow(const juce::String& name,
                 juce::AudioPluginInstance* plugin,
                 WindowID windowId,
                 std::function<void(WindowID)> onCloseCallback)
        : DocumentWindow(name, juce::Colours::darkgrey, DocumentWindow::allButtons)
        , m_plugin(plugin)
        , m_windowId(windowId)
        , m_onClose(onCloseCallback)
    {
        setUsingNativeTitleBar(true);
        setResizable(true, false);
        setResizeLimits(400, 300, 2000, 2000);
        
        // Create and set plugin editor
        if (m_plugin && m_plugin->hasEditor())
        {
            if (auto* editor = m_plugin->createEditorIfNeeded())
            {
                setContentOwned(editor, true);
                
                // Set initial size based on editor's preferred size
                setSize(editor->getWidth(), editor->getHeight());
            }
        }
    }
    
    ~PluginWindow() override
    {
        // Don't call clearContentComponent() here during destruction
        // JUCE will handle cleanup properly
    }
    
    void closeButtonPressed() override
    {
        // Save window position
        saveWindowPosition();
        
        // Clear content before notifying to avoid crashes
        clearContentComponent();
        
        // Notify manager
        if (m_onClose)
            m_onClose(m_windowId);
    }
    
    void moved() override
    {
        DocumentWindow::moved();
        saveWindowPosition();
    }
    
    void resized() override
    {
        DocumentWindow::resized();
        saveWindowPosition();
    }
    
    WindowID getWindowId() const { return m_windowId; }
    
    void saveWindowPosition()
    {
        // Save position to settings
        auto settings = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                           .getChildFile("HAM")
                           .getChildFile("window_positions.xml");
        
        settings.getParentDirectory().createDirectory();
        
        auto xml = juce::XmlDocument::parse(settings);
        if (!xml)
            xml = std::make_unique<juce::XmlElement>("WindowPositions");
        
        auto windowKey = "plugin_" + juce::String(m_windowId.first) + "_" + juce::String(m_windowId.second);
        
        if (auto* windowElement = xml->getChildByName(windowKey))
        {
            windowElement->setAttribute("x", getX());
            windowElement->setAttribute("y", getY());
            windowElement->setAttribute("width", getWidth());
            windowElement->setAttribute("height", getHeight());
        }
        else
        {
            auto newElement = xml->createNewChildElement(windowKey);
            newElement->setAttribute("x", getX());
            newElement->setAttribute("y", getY());
            newElement->setAttribute("width", getWidth());
            newElement->setAttribute("height", getHeight());
        }
        
        xml->writeTo(settings);
    }
    
    void restoreWindowPosition()
    {
        auto settings = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                           .getChildFile("HAM")
                           .getChildFile("window_positions.xml");
        
        if (auto xml = juce::XmlDocument::parse(settings))
        {
            auto windowKey = "plugin_" + juce::String(m_windowId.first) + "_" + juce::String(m_windowId.second);
            
            if (auto* windowElement = xml->getChildByName(windowKey))
            {
                auto x = windowElement->getIntAttribute("x", 100);
                auto y = windowElement->getIntAttribute("y", 100);
                auto width = windowElement->getIntAttribute("width", getWidth());
                auto height = windowElement->getIntAttribute("height", getHeight());
                
                setBounds(x, y, width, height);
            }
            else
            {
                // Default position - cascade windows
                centreWithSize(getWidth(), getHeight());
            }
        }
        else
        {
            centreWithSize(getWidth(), getHeight());
        }
    }
    
private:
    juce::AudioPluginInstance* m_plugin;
    WindowID m_windowId;
    std::function<void(WindowID)> m_onClose;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginWindow)
};

/**
 * PluginWindowManager - Manages all plugin editor windows
 * 
 * Features:
 * - Singleton access for global window management
 * - Automatic cleanup on plugin removal
 * - Window position persistence
 * - Focus management
 */
class PluginWindowManager
{
public:
    using WindowID = PluginWindow::WindowID;
    
    //==============================================================================
    // Singleton Access
    static PluginWindowManager& getInstance()
    {
        static PluginWindowManager instance;
        return instance;
    }
    
    // Delete copy/move constructors for singleton
    PluginWindowManager(const PluginWindowManager&) = delete;
    PluginWindowManager& operator=(const PluginWindowManager&) = delete;
    PluginWindowManager(PluginWindowManager&&) = delete;
    PluginWindowManager& operator=(PluginWindowManager&&) = delete;
    
    //==============================================================================
    // Window Management
    
    /**
     * Open or focus a plugin editor window
     * @param trackIndex The track containing the plugin
     * @param pluginIndex The plugin index in the chain (-1 for instrument)
     * @param plugin The plugin instance
     * @param pluginName The display name for the window
     * @return True if window was opened/focused successfully
     */
    bool openPluginWindow(int trackIndex, int pluginIndex,
                         juce::AudioPluginInstance* plugin,
                         const juce::String& pluginName)
    {
        DBG("PluginWindowManager::openPluginWindow called");
        DBG("  Track: " << trackIndex << ", Plugin: " << pluginIndex);
        DBG("  Plugin Name: " << pluginName);
        
        if (!plugin)
        {
            DBG("  ERROR: Plugin pointer is null!");
            return false;
        }
        
        DBG("  Plugin has editor: " << (plugin->hasEditor() ? "YES" : "NO"));
        
        WindowID windowId(trackIndex, pluginIndex);
        
        // Check if window already exists
        if (auto it = m_windows.find(windowId); it != m_windows.end())
        {
            DBG("  Window already exists, bringing to front");
            // Bring existing window to front
            if (it->second)
            {
                it->second->toFront(true);
                return true;
            }
        }
        
        DBG("  Creating new window...");
        // Create new window
        auto windowName = pluginName + " [Track " + juce::String(trackIndex + 1) + "]";
        
        auto window = std::make_unique<PluginWindow>(
            windowName,
            plugin,
            windowId,
            [this](WindowID id) { onWindowClosed(id); }
        );
        
        if (window->getContentComponent())
        {
            DBG("  Window created successfully with content");
            window->restoreWindowPosition();
            window->setVisible(true);
            window->toFront(true);
            
            m_windows[windowId] = std::move(window);
            return true;
        }
        else
        {
            DBG("  ERROR: Failed to create window content (no editor component)");
        }
        
        return false;
    }
    
    /**
     * Close a plugin window
     * @param trackIndex The track containing the plugin
     * @param pluginIndex The plugin index in the chain (-1 for instrument)
     */
    void closePluginWindow(int trackIndex, int pluginIndex)
    {
        WindowID windowId(trackIndex, pluginIndex);
        
        if (auto it = m_windows.find(windowId); it != m_windows.end())
        {
            if (it->second)
            {
                it->second->closeButtonPressed();
            }
            m_windows.erase(it);
        }
    }
    
    /**
     * Close all windows for a track
     * @param trackIndex The track index
     */
    void closeTrackWindows(int trackIndex)
    {
        auto it = m_windows.begin();
        while (it != m_windows.end())
        {
            if (it->first.first == trackIndex)
            {
                if (it->second)
                {
                    it->second->closeButtonPressed();
                }
                it = m_windows.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
    
    /**
     * Close all plugin windows
     */
    void closeAllWindows()
    {
        // Create a copy of window IDs to avoid iterator invalidation
        std::vector<WindowID> windowIds;
        windowIds.reserve(m_windows.size());
        
        for (const auto& [id, window] : m_windows)
        {
            windowIds.push_back(id);
        }
        
        // Close each window safely
        for (const auto& id : windowIds)
        {
            auto it = m_windows.find(id);
            if (it != m_windows.end() && it->second)
            {
                // Clear content first to avoid Cocoa crashes
                it->second->clearContentComponent();
                it->second->setVisible(false);
            }
        }
        
        // Clear the map after all windows are closed
        m_windows.clear();
    }
    
    /**
     * Check if a plugin window is open
     * @param trackIndex The track containing the plugin
     * @param pluginIndex The plugin index in the chain (-1 for instrument)
     * @return True if window exists and is visible
     */
    bool isWindowOpen(int trackIndex, int pluginIndex) const
    {
        WindowID windowId(trackIndex, pluginIndex);
        
        if (auto it = m_windows.find(windowId); it != m_windows.end())
        {
            return it->second && it->second->isVisible();
        }
        
        return false;
    }
    
    /**
     * Get the number of open windows
     * @return Count of open plugin windows
     */
    size_t getOpenWindowCount() const
    {
        return m_windows.size();
    }
    
    /**
     * Bring all windows to front
     */
    void bringAllToFront()
    {
        for (auto& [id, window] : m_windows)
        {
            if (window)
            {
                window->toFront(false);
            }
        }
    }
    
    /**
     * Minimize all windows
     */
    void minimizeAll()
    {
        for (auto& [id, window] : m_windows)
        {
            if (window)
            {
                window->setMinimised(true);
            }
        }
    }
    
private:
    //==============================================================================
    // Private constructor/destructor for singleton
    PluginWindowManager() = default;
    
    ~PluginWindowManager()
    {
        // During shutdown, just clear the window map
        // Don't try to interact with windows as they may be partially destroyed
        m_windows.clear();
    }
    
    //==============================================================================
    // Internal Methods
    
    void onWindowClosed(WindowID windowId)
    {
        // Window is closing, remove from map
        // Use iterator to safely erase during iteration
        auto it = m_windows.find(windowId);
        if (it != m_windows.end())
        {
            m_windows.erase(it);
        }
    }
    
    //==============================================================================
    // Internal Data
    
    std::map<WindowID, std::unique_ptr<PluginWindow>> m_windows;
};

} // namespace HAM