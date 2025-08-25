/*
  ==============================================================================

    PresetManager.h
    Manages saving and loading of presets for plugins and patterns

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <memory>
#include <vector>

namespace HAM
{

/**
 * PresetManager - Handles saving and loading of presets
 * 
 * Features:
 * - Plugin preset management (save/load plugin states)
 * - Pattern preset management
 * - User preset library
 * - Factory preset support
 */
class PresetManager
{
public:
    //==============================================================================
    // Singleton Access
    
    static PresetManager& instance()
    {
        static PresetManager instance;
        return instance;
    }
    
    // Delete copy/move for singleton
    PresetManager(const PresetManager&) = delete;
    PresetManager& operator=(const PresetManager&) = delete;
    
    //==============================================================================
    // Preset Types
    
    struct PluginPreset
    {
        juce::String name;
        juce::String category;
        juce::String pluginName;
        juce::String pluginFormat;
        juce::MemoryBlock stateData;
        juce::Time lastModified;
        bool isFactory = false;
    };
    
    struct PatternPreset
    {
        juce::String name;
        juce::String category;
        juce::ValueTree patternData;
        juce::Time lastModified;
        bool isFactory = false;
    };
    
    //==============================================================================
    // Plugin Preset Management
    
    /**
     * Save plugin state as preset
     * @param pluginInstance The plugin to save
     * @param presetName Name for the preset
     * @param category Optional category
     * @return True if saved successfully
     */
    bool savePluginPreset(juce::AudioPluginInstance* pluginInstance,
                         const juce::String& presetName,
                         const juce::String& category = "User");
    
    /**
     * Load plugin preset
     * @param pluginInstance The plugin to load into
     * @param presetName Name of the preset
     * @return True if loaded successfully
     */
    bool loadPluginPreset(juce::AudioPluginInstance* pluginInstance,
                         const juce::String& presetName);
    
    /**
     * Get all presets for a specific plugin
     * @param pluginName Name of the plugin
     * @param pluginFormat Format (VST3, AU, etc)
     * @return Vector of matching presets
     */
    std::vector<PluginPreset> getPluginPresets(const juce::String& pluginName,
                                               const juce::String& pluginFormat = "");
    
    /**
     * Delete a plugin preset
     * @param presetName Name of the preset to delete
     * @return True if deleted successfully
     */
    bool deletePluginPreset(const juce::String& presetName);
    
    /**
     * Export plugin preset to file
     * @param presetName Name of the preset
     * @param exportFile File to export to
     * @return True if exported successfully
     */
    bool exportPluginPreset(const juce::String& presetName,
                           const juce::File& exportFile);
    
    /**
     * Import plugin preset from file
     * @param importFile File to import from
     * @param newName Optional new name for the preset
     * @return True if imported successfully
     */
    bool importPluginPreset(const juce::File& importFile,
                           const juce::String& newName = "");
    
    //==============================================================================
    // Pattern Preset Management
    
    /**
     * Save pattern as preset
     * @param patternData Pattern data as ValueTree
     * @param presetName Name for the preset
     * @param category Optional category
     * @return True if saved successfully
     */
    bool savePatternPreset(const juce::ValueTree& patternData,
                          const juce::String& presetName,
                          const juce::String& category = "User");
    
    /**
     * Load pattern preset
     * @param presetName Name of the preset
     * @return Pattern data as ValueTree (invalid if not found)
     */
    juce::ValueTree loadPatternPreset(const juce::String& presetName);
    
    /**
     * Get all pattern presets
     * @param category Optional category filter
     * @return Vector of pattern presets
     */
    std::vector<PatternPreset> getPatternPresets(const juce::String& category = "");
    
    /**
     * Delete a pattern preset
     * @param presetName Name of the preset to delete
     * @return True if deleted successfully
     */
    bool deletePatternPreset(const juce::String& presetName);
    
    //==============================================================================
    // Preset Library Management
    
    /**
     * Get preset directory for user presets
     * @return User preset directory
     */
    juce::File getUserPresetDirectory() const;
    
    /**
     * Get preset directory for factory presets
     * @return Factory preset directory
     */
    juce::File getFactoryPresetDirectory() const;
    
    /**
     * Scan preset directories for available presets
     */
    void rescanPresets();
    
    /**
     * Get all preset categories
     * @return Vector of category names
     */
    std::vector<juce::String> getCategories() const;
    
    /**
     * Create a new category
     * @param categoryName Name of the category
     * @return True if created successfully
     */
    bool createCategory(const juce::String& categoryName);
    
    //==============================================================================
    // Initialization
    
    /**
     * Initialize preset system
     */
    void initialize();
    
private:
    //==============================================================================
    // Private constructor for singleton
    PresetManager();
    ~PresetManager();
    
    //==============================================================================
    // Helper Methods
    
    juce::File getPluginPresetFile(const juce::String& presetName) const;
    juce::File getPatternPresetFile(const juce::String& presetName) const;
    void loadPresetsFromDirectory(const juce::File& directory, bool isFactory);
    
    //==============================================================================
    // Data
    
    std::vector<PluginPreset> m_pluginPresets;
    std::vector<PatternPreset> m_patternPresets;
    std::vector<juce::String> m_categories;
    
    juce::File m_userPresetDirectory;
    juce::File m_factoryPresetDirectory;
    
    mutable juce::CriticalSection m_lock;
};

} // namespace HAM