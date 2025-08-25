/*
  ==============================================================================

    PluginManager.h
    Plugin resource management with RAII and proper memory handling
    Ensures no memory leaks in plugin hosting

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <atomic>

namespace HAM
{

/**
 * PluginManager - Manages plugin instances with proper RAII
 * 
 * Features:
 * - Automatic cleanup of plugin instances
 * - Thread-safe plugin management
 * - Support for instrument and effect plugins per track
 * - Bridge process lifecycle management
 */
class PluginManager : private juce::Timer
{
public:
    //==============================================================================
    // Singleton Access
    
    static PluginManager& instance()
    {
        static PluginManager instance;
        return instance;
    }
    
    // Delete copy/move constructors for singleton
    PluginManager(const PluginManager&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;
    PluginManager(PluginManager&&) = delete;
    PluginManager& operator=(PluginManager&&) = delete;
    
    //==============================================================================
    // Plugin Instance Wrapper with RAII
    class PluginInstance
    {
    public:
        PluginInstance(std::unique_ptr<juce::AudioPluginInstance> instance,
                      const juce::PluginDescription& description)
            : m_instance(std::move(instance))
            , m_description(description)
            , m_isActive(true)
        {
            jassert(m_instance != nullptr);
        }
        
        ~PluginInstance()
        {
            // Ensure proper cleanup
            if (m_editor)
            {
                m_editor.reset();
            }
            
            if (m_instance)
            {
                m_instance->releaseResources();
                m_instance.reset();
            }
        }
        
        // Non-copyable, moveable
        PluginInstance(const PluginInstance&) = delete;
        PluginInstance& operator=(const PluginInstance&) = delete;
        PluginInstance(PluginInstance&&) = default;
        PluginInstance& operator=(PluginInstance&&) = default;
        
        juce::AudioPluginInstance* getInstance() { return m_instance.get(); }
        const juce::AudioPluginInstance* getInstance() const { return m_instance.get(); }
        
        juce::AudioProcessorEditor* getEditor()
        {
            if (!m_editor && m_instance)
            {
                m_editor.reset(m_instance->createEditor());
            }
            return m_editor.get();
        }
        
        void closeEditor()
        {
            m_editor.reset();
        }
        
        const juce::PluginDescription& getDescription() const { return m_description; }
        bool isActive() const { return m_isActive; }
        void setActive(bool active) { m_isActive = active; }
        
    private:
        std::unique_ptr<juce::AudioPluginInstance> m_instance;
        std::unique_ptr<juce::AudioProcessorEditor> m_editor;
        juce::PluginDescription m_description;
        bool m_isActive;
    };
    
    //==============================================================================
    // Bridge Process Manager
    class BridgeProcess
    {
    public:
        BridgeProcess() = default;
        
        ~BridgeProcess()
        {
            terminate();
        }
        
        bool launch(const juce::String& command)
        {
            terminate();  // Clean up any existing process
            
            m_process = std::make_unique<juce::ChildProcess>();
            if (m_process->start(command))
            {
                return true;
            }
            
            m_process.reset();
            return false;
        }
        
        void terminate()
        {
            if (m_process && m_process->isRunning())
            {
                m_process->kill();
            }
            m_process.reset();
        }
        
        bool isRunning() const
        {
            return m_process && m_process->isRunning();
        }
        
    private:
        std::unique_ptr<juce::ChildProcess> m_process;
    };
    
    //==============================================================================
    // Plugin Scanning Interface (for compatibility with Main.cpp)
    
    struct ScanProgress
    {
        int total{0};
        int scanned{0};
        juce::String current;
    };
    
    /** Initialize plugin scanning system */
    void initialise();
    
    /** Start sandboxed plugin scan */
    void startSandboxedScan(bool async);
    
    /** Check if currently scanning */
    bool isScanning() const;
    
    /** Get scan progress */
    ScanProgress getProgress() const;
    
    /** Get known plugins list */
    juce::KnownPluginList& getKnownPluginList() { return m_knownPluginList; }
    const juce::KnownPluginList& getKnownPluginList() const { return m_knownPluginList; }
    
    //==============================================================================
    // Plugin Instantiation
    
    /**
     * Create a plugin instance synchronously
     * @param description The plugin description
     * @param sampleRate The sample rate
     * @param blockSize The audio block size
     * @return The created plugin instance (or nullptr on failure)
     */
    std::unique_ptr<juce::AudioPluginInstance> createPluginInstance(
        const juce::PluginDescription& description,
        double sampleRate,
        int blockSize);
    
    /**
     * Create a plugin instance asynchronously (required for AudioUnit v3)
     * @param description The plugin description
     * @param sampleRate The sample rate
     * @param blockSize The audio block size
     * @param callback Callback with instance and error message
     */
    void createPluginInstanceAsync(
        const juce::PluginDescription& description,
        double sampleRate,
        int blockSize,
        std::function<void(std::unique_ptr<juce::AudioPluginInstance>, const juce::String&)> callback);
    
    //==============================================================================
    // Plugin Management
    
    /**
     * Add an instrument plugin to a track
     * @param trackIndex The track to add the plugin to
     * @param instance The plugin instance (ownership transferred)
     * @param description The plugin description
     * @return True if successfully added
     */
    bool addInstrumentPlugin(int trackIndex,
                           std::unique_ptr<juce::AudioPluginInstance> instance,
                           const juce::PluginDescription& description);
    
    /**
     * Add an effect plugin to a track
     * @param trackIndex The track to add the effect to
     * @param instance The plugin instance (ownership transferred)
     * @param description The plugin description
     * @param position Insert position in effect chain (-1 for end)
     * @return True if successfully added
     */
    bool addEffectPlugin(int trackIndex,
                        std::unique_ptr<juce::AudioPluginInstance> instance,
                        const juce::PluginDescription& description,
                        int position = -1);
    
    /**
     * Remove instrument plugin from track
     * @param trackIndex The track index
     * @return True if plugin was removed
     */
    bool removeInstrumentPlugin(int trackIndex);
    
    /**
     * Remove effect plugin from track
     * @param trackIndex The track index
     * @param effectIndex The effect index in the chain
     * @return True if plugin was removed
     */
    bool removeEffectPlugin(int trackIndex, int effectIndex);
    
    /**
     * Get instrument plugin for track
     * @param trackIndex The track index
     * @return Pointer to plugin instance (or nullptr if none)
     */
    PluginInstance* getInstrumentPlugin(int trackIndex);
    
    /**
     * Get effect plugins for track
     * @param trackIndex The track index
     * @return Vector of effect plugin instances
     */
    std::vector<PluginInstance*> getEffectPlugins(int trackIndex);
    
    /**
     * Clear all plugins for a track
     * @param trackIndex The track index
     */
    void clearTrackPlugins(int trackIndex);
    
    /**
     * Clear all plugins
     */
    void clearAllPlugins();
    
    //==============================================================================
    // Bridge Process Management
    
    /**
     * Launch plugin editor in bridge process
     * @param formatName The plugin format
     * @param identifier The plugin identifier
     * @param useRosetta Use Rosetta for x86_64 plugins on Apple Silicon
     * @return True if bridge launched successfully
     */
    bool launchPluginBridge(const juce::String& formatName,
                           const juce::String& identifier,
                           bool useRosetta = false);
    
    /**
     * Terminate any running bridge process
     */
    void terminateBridge();
    
    /**
     * Check if bridge is running
     * @return True if bridge process is active
     */
    bool isBridgeRunning() const;
    
    //==============================================================================
    // Resource Statistics
    
    struct ResourceStats
    {
        size_t totalPluginCount{0};
        size_t instrumentCount{0};
        size_t effectCount{0};
        size_t activeEditorCount{0};
        bool bridgeActive{false};
    };
    
    ResourceStats getResourceStats() const;
    
private:
    //==============================================================================
    // Private constructor/destructor for singleton
    PluginManager();
    ~PluginManager();
    
    //==============================================================================
    // Internal Data
    
    // Thread safety
    mutable std::mutex m_mutex;
    
    // Plugin storage - using unique_ptr for automatic cleanup
    std::unordered_map<int, std::unique_ptr<PluginInstance>> m_instrumentPlugins;
    std::unordered_map<int, std::vector<std::unique_ptr<PluginInstance>>> m_effectPlugins;
    
    // Bridge process management
    std::unique_ptr<BridgeProcess> m_bridgeProcess;
    
    // Plugin format manager (shared)
    juce::AudioPluginFormatManager m_formatManager;
    
    // Known plugins list
    juce::KnownPluginList m_knownPluginList;
    
    // Scanning state
    std::atomic<bool> m_isScanning{false};
    std::atomic<int> m_scanTotal{0};
    std::atomic<int> m_scanProgress{0};
    juce::String m_currentPlugin;
    
    // Plugin scanner (for async scanning)
    std::unique_ptr<juce::PluginDirectoryScanner> m_scanner;
    std::unique_ptr<std::thread> m_scanThread;
    int m_currentFormatIndex{0};  // Track which format we're scanning
    
    // Plugin list file location
    juce::File m_pluginListFile;
    
    // Audio processor graph for plugin chains
    std::unique_ptr<juce::AudioProcessorGraph> m_processorGraph;
    
    //==============================================================================
    // Helper Methods
    
    static juce::File findPluginHostBridge();
    juce::File findPluginScanner();
    void savePluginList();
    void performInternalScan();
    void loadPluginList();
    
    // Timer callback for safe scanning on message thread
    void timerCallback() override;
    
    // Note: Non-copyable is enforced by deleted constructors above
};

} // namespace HAM