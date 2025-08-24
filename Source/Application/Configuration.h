/*
  ==============================================================================
  
    Configuration.h
    HAM Application Configuration Settings
    
    Manages all configurable settings for the application including
    debug options, performance settings, and user preferences.
    
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <optional>

namespace HAM::Application {

/**
 * Configuration singleton for managing application settings.
 * All settings are persisted to user preferences.
 */
class Configuration
{
public:
    // Get singleton instance
    static Configuration& getInstance()
    {
        static Configuration instance;
        return instance;
    }
    
    // Delete copy/move constructors
    Configuration(const Configuration&) = delete;
    Configuration& operator=(const Configuration&) = delete;
    Configuration(Configuration&&) = delete;
    Configuration& operator=(Configuration&&) = delete;
    
    // ===== Debug Settings =====
    
    /** Get debug MIDI channel (0 = disabled, 1-16 = channel number) */
    int getDebugMidiChannel() const { return m_debugMidiChannel; }
    
    /** Set debug MIDI channel (0 to disable, 1-16 for channel) */
    void setDebugMidiChannel(int channel) 
    { 
        m_debugMidiChannel = juce::jlimit(0, 16, channel);
        saveSettings();
    }
    
    /** Check if debug MIDI is enabled */
    bool isDebugMidiEnabled() const { return m_debugMidiChannel > 0; }
    
    // ===== Performance Settings =====
    
    /** Get audio buffer size */
    int getAudioBufferSize() const { return m_audioBufferSize; }
    
    /** Set audio buffer size */
    void setAudioBufferSize(int size) 
    { 
        m_audioBufferSize = size;
        saveSettings();
    }
    
    /** Get sample rate */
    double getSampleRate() const { return m_sampleRate; }
    
    /** Set sample rate */
    void setSampleRate(double rate) 
    { 
        m_sampleRate = rate;
        saveSettings();
    }
    
    /** Enable/disable performance statistics tracking */
    void setPerformanceStatsEnabled(bool enabled)
    {
        m_performanceStatsEnabled = enabled;
        saveSettings();
    }
    
    /** Check if performance stats are enabled */
    bool isPerformanceStatsEnabled() const { return m_performanceStatsEnabled; }
    
    // ===== MIDI Settings =====
    
    /** Get default MIDI output channel for all tracks */
    int getDefaultMidiOutputChannel() const { return m_defaultMidiOutputChannel; }
    
    /** Set default MIDI output channel */
    void setDefaultMidiOutputChannel(int channel)
    {
        m_defaultMidiOutputChannel = juce::jlimit(1, 16, channel);
        saveSettings();
    }
    
    // ===== UI Settings =====
    
    /** Get UI scale factor */
    float getUiScaleFactor() const { return m_uiScaleFactor; }
    
    /** Set UI scale factor */
    void setUiScaleFactor(float scale)
    {
        m_uiScaleFactor = juce::jlimit(0.5f, 2.0f, scale);
        saveSettings();
    }
    
    /** Get whether to show tooltips */
    bool getShowTooltips() const { return m_showTooltips; }
    
    /** Set whether to show tooltips */
    void setShowTooltips(bool show)
    {
        m_showTooltips = show;
        saveSettings();
    }
    
    // ===== File Paths =====
    
    /** Get last project directory */
    juce::File getLastProjectDirectory() const { return m_lastProjectDirectory; }
    
    /** Set last project directory */
    void setLastProjectDirectory(const juce::File& dir)
    {
        m_lastProjectDirectory = dir;
        saveSettings();
    }
    
    // ===== Load/Save =====
    
    /** Load settings from user preferences */
    void loadSettings()
    {
        auto* props = getProperties()->getUserSettings();
        if (props == nullptr)
            return;
        
        m_debugMidiChannel = props->getIntValue("debugMidiChannel", 0); // Default: disabled
        m_audioBufferSize = props->getIntValue("audioBufferSize", 128);
        m_sampleRate = props->getDoubleValue("sampleRate", 48000.0);
        m_performanceStatsEnabled = props->getBoolValue("performanceStatsEnabled", false);
        m_defaultMidiOutputChannel = props->getIntValue("defaultMidiOutputChannel", 1);
        m_uiScaleFactor = static_cast<float>(props->getDoubleValue("uiScaleFactor", 1.0));
        m_showTooltips = props->getBoolValue("showTooltips", true);
        
        auto lastDir = props->getValue("lastProjectDirectory");
        if (lastDir.isNotEmpty())
            m_lastProjectDirectory = juce::File(lastDir);
    }
    
    /** Save settings to user preferences */
    void saveSettings()
    {
        auto* props = getProperties()->getUserSettings();
        if (props == nullptr)
            return;
        
        props->setValue("debugMidiChannel", m_debugMidiChannel);
        props->setValue("audioBufferSize", m_audioBufferSize);
        props->setValue("sampleRate", m_sampleRate);
        props->setValue("performanceStatsEnabled", m_performanceStatsEnabled);
        props->setValue("defaultMidiOutputChannel", m_defaultMidiOutputChannel);
        props->setValue("uiScaleFactor", static_cast<double>(m_uiScaleFactor));
        props->setValue("showTooltips", m_showTooltips);
        
        if (m_lastProjectDirectory.exists())
            props->setValue("lastProjectDirectory", m_lastProjectDirectory.getFullPathName());
        
        props->saveIfNeeded();
    }
    
    /** Reset all settings to defaults */
    void resetToDefaults()
    {
        m_debugMidiChannel = 0; // Disabled by default
        m_audioBufferSize = 128;
        m_sampleRate = 48000.0;
        m_performanceStatsEnabled = false;
        m_defaultMidiOutputChannel = 1;
        m_uiScaleFactor = 1.0f;
        m_showTooltips = true;
        m_lastProjectDirectory = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
        
        saveSettings();
    }
    
private:
    // Private constructor for singleton
    Configuration()
    {
        loadSettings();
    }
    
    ~Configuration() = default;
    
    // Get application properties for persistence
    juce::ApplicationProperties* getProperties()
    {
        if (!m_properties)
        {
            juce::PropertiesFile::Options options;
            options.applicationName = "HAM";
            options.filenameSuffix = ".settings";
            options.osxLibrarySubFolder = "Application Support";
            options.folderName = "HAM";
            options.storageFormat = juce::PropertiesFile::storeAsXML;
            
            m_properties = std::make_unique<juce::ApplicationProperties>();
            m_properties->setStorageParameters(options);
        }
        return m_properties.get();
    }
    
    // Settings storage
    int m_debugMidiChannel = 0; // 0 = disabled, 1-16 = channel
    int m_audioBufferSize = 128;
    double m_sampleRate = 48000.0;
    bool m_performanceStatsEnabled = false;
    int m_defaultMidiOutputChannel = 1;
    float m_uiScaleFactor = 1.0f;
    bool m_showTooltips = true;
    juce::File m_lastProjectDirectory;
    
    // Properties for persistence
    std::unique_ptr<juce::ApplicationProperties> m_properties;
};

} // namespace HAM::Application