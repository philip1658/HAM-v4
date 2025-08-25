/*
  ==============================================================================

    PluginManager.cpp
    Plugin resource management implementation

  ==============================================================================
*/

#include "PluginManager.h"

namespace HAM
{

//==============================================================================
PluginManager::PluginManager()
{
    try
    {
        // Initialize supported plugin formats
        m_formatManager.addDefaultFormats();
        
        // Create bridge process manager
        m_bridgeProcess = std::make_unique<BridgeProcess>();
    }
    catch (const std::exception& e)
    {
        DBG("PluginManager constructor exception: " << e.what());
        // Continue with minimal functionality
    }
    catch (...)
    {
        DBG("PluginManager constructor unknown exception");
        // Continue with minimal functionality
    }
}

PluginManager::~PluginManager()
{
    // Stop timer-based scanning
    stopTimer();
    
    // Stop any ongoing scan
    if (m_scanThread && m_scanThread->joinable())
    {
        m_isScanning = false;
        m_scanThread->join();
    }
    
    // Clean up all resources in proper order
    clearAllPlugins();
    terminateBridge();
}

//==============================================================================
// Plugin Scanning

void PluginManager::initialise()
{
    try
    {
        // Cache all directory paths in main thread for thread safety
        cacheDirectoryPaths();
        
        // Create application data directories if they don't exist
        if (m_cachedPaths.isValid)
        {
            if (m_cachedPaths.appDataDir.isDirectory() || m_cachedPaths.appDataDir.createDirectory())
            {
                DBG("PluginManager: App data directory ready: " << m_cachedPaths.appDataDir.getFullPathName());
            }
            
            // Load previously scanned plugins from settings
            if (m_cachedPaths.pluginListFile.existsAsFile())
            {
                try
                {
                    auto xml = juce::XmlDocument::parse(m_cachedPaths.pluginListFile);
                    if (xml)
                    {
                        m_knownPluginList.recreateFromXml(*xml);
                        DBG("PluginManager: Loaded " << m_knownPluginList.getNumTypes() << " plugins from cache");
                    }
                }
                catch (...)
                {
                    DBG("PluginManager: Could not load plugin cache - starting fresh");
                }
            }
        }
        else
        {
            DBG("PluginManager: Could not cache directory paths - plugin scanning disabled");
        }
    }
    catch (const std::exception& e)
    {
        DBG("PluginManager::initialise exception: " << e.what());
    }
    catch (...)
    {
        DBG("PluginManager::initialise unknown exception - continuing with defaults");
    }
}

void PluginManager::cacheDirectoryPaths()
{
    // This MUST be called from the main thread
    // Cache all paths that would normally be accessed from background thread
    
    try
    {
        // Try to get app data directory
        try 
        {
            m_cachedPaths.appDataDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                                           .getChildFile("HAM");
        }
        catch (...)
        {
            // Fallback to home directory
            m_cachedPaths.appDataDir = juce::File::getSpecialLocation(juce::File::userHomeDirectory)
                                           .getChildFile(".ham");
            DBG("PluginManager: Using fallback directory: " << m_cachedPaths.appDataDir.getFullPathName());
        }
        
        // Cache plugin list file path
        m_cachedPaths.pluginListFile = m_cachedPaths.appDataDir.getChildFile("plugin_list.xml");
        
        // Cache plugin search paths
        #if JUCE_MAC
        m_cachedPaths.searchPaths.add(juce::File("/Library/Audio/Plug-Ins/VST3"));
        m_cachedPaths.searchPaths.add(juce::File::getSpecialLocation(juce::File::userHomeDirectory)
                                         .getChildFile("Library/Audio/Plug-Ins/VST3"));
        m_cachedPaths.searchPaths.add(juce::File("/Library/Audio/Plug-Ins/Components"));
        m_cachedPaths.searchPaths.add(juce::File::getSpecialLocation(juce::File::userHomeDirectory)
                                         .getChildFile("Library/Audio/Plug-Ins/Components"));
        #elif JUCE_WINDOWS
        m_cachedPaths.searchPaths.add(juce::File("C:/Program Files/Common Files/VST3"));
        m_cachedPaths.searchPaths.add(juce::File("C:/Program Files (x86)/Common Files/VST3"));
        #endif
        
        // Add format-specific paths
        for (int i = 0; i < m_formatManager.getNumFormats(); ++i)
        {
            auto* format = m_formatManager.getFormat(i);
            if (format)
            {
                format->searchPathsForPlugins(m_cachedPaths.searchPaths, true);
            }
        }
        
        m_cachedPaths.isValid = true;
        DBG("PluginManager: Successfully cached all directory paths");
    }
    catch (const std::exception& e)
    {
        DBG("PluginManager::cacheDirectoryPaths exception: " << e.what());
        m_cachedPaths.isValid = false;
    }
    catch (...)
    {
        DBG("PluginManager::cacheDirectoryPaths unknown exception");
        m_cachedPaths.isValid = false;
    }
}

void PluginManager::startSandboxedScan(bool async)
{
    try
    {
        // Check if we have valid cached paths
        if (!m_cachedPaths.isValid)
        {
            DBG("PluginManager: Cannot start scan - no valid cached paths");
            return;
        }
        
        // Stop any existing scan
        if (m_isScanning)
        {
            stopTimer();  // Stop timer-based scanning
            m_isScanning = false;
            if (m_scanThread && m_scanThread->joinable())
            {
                m_scanThread->join();
            }
        }
        
        // Reset progress
        m_scanProgress = 0;
        m_scanTotal = 0;
        m_currentPlugin = "";
        
        // Use cached search paths - no file system access in background thread!
        const juce::FileSearchPath& searchPath = m_cachedPaths.searchPaths;
        
        // Create scanner with cached paths
        try
        {
            for (int i = 0; i < m_formatManager.getNumFormats(); ++i)
            {
                auto* format = m_formatManager.getFormat(i);
                
                if (format)
                {
                    // Create scanner for this format using cached paths
                    m_scanner = std::make_unique<juce::PluginDirectoryScanner>(
                        m_knownPluginList,
                        *format,
                        searchPath,
                        true,  // recursive search
                        juce::File()  // no dead man's pedal file for now
                    );
                    break; // Just use the first format for now (typically VST3)
                }
            }
        }
        catch (const std::exception& e)
        {
            DBG("PluginManager: Error creating scanner: " << e.what());
            m_scanner.reset();
            return;
        }
        
        // Get total number of files to scan
        m_scanTotal = searchPath.getNumPaths() * 100;  // Estimate
    
    if (async)
    {
        // Use timer-based scanning on the message thread for safety
        // This avoids all the thread-safety issues with file system access
        m_isScanning = true;
        
        DBG("PluginManager: Starting timer-based plugin scan");
        
        // Start timer to scan incrementally on message thread
        // Scan a few plugins every 50ms to avoid blocking UI
        startTimer(50);
    }
    else
    {
        // Synchronous scan (blocks)
        m_isScanning = true;
        juce::String pluginName;
        
        while (!m_scanner->scanNextFile(true, pluginName))
        {
            m_currentPlugin = pluginName;
            m_scanProgress++;
        }
        
        savePluginList();
        m_isScanning = false;
        m_scanner.reset();
    }
    }
    catch (const std::exception& e)
    {
        DBG("PluginManager::startSandboxedScan exception: " << e.what());
        m_isScanning = false;
        m_scanner.reset();
    }
    catch (...)
    {
        DBG("PluginManager::startSandboxedScan unknown exception");
        m_isScanning = false;
        m_scanner.reset();
    }
}

bool PluginManager::isScanning() const
{
    return m_isScanning.load();
}

PluginManager::ScanProgress PluginManager::getProgress() const
{
    ScanProgress progress;
    progress.total = m_scanTotal.load();
    progress.scanned = m_scanProgress.load();
    progress.current = m_currentPlugin;
    return progress;
}

void PluginManager::savePluginList()
{
    try
    {
        // Use cached paths - this method should only be called from main thread
        if (!m_cachedPaths.isValid)
        {
            DBG("PluginManager: Cannot save - no valid cached paths");
            return;
        }
        
        const juce::File& settingsFile = m_cachedPaths.pluginListFile;
        
        // Make sure parent directory exists
        if (!settingsFile.getParentDirectory().exists())
        {
            settingsFile.getParentDirectory().createDirectory();
        }
        
        if (auto xml = m_knownPluginList.createXml())
        {
            if (!xml->writeTo(settingsFile))
            {
                DBG("PluginManager: Could not save plugin list to: " << settingsFile.getFullPathName());
            }
            else
            {
                DBG("PluginManager: Successfully saved plugin list");
            }
        }
    }
    catch (const std::exception& e)
    {
        DBG("PluginManager::savePluginList exception: " << e.what());
    }
    catch (...)
    {
        DBG("PluginManager::savePluginList unknown exception - plugin list not saved");
    }
}

//==============================================================================
// Plugin Management

bool PluginManager::addInstrumentPlugin(int trackIndex,
                                       std::unique_ptr<juce::AudioPluginInstance> instance,
                                       const juce::PluginDescription& description)
{
    if (!instance)
        return false;
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Remove any existing instrument plugin for this track
    m_instrumentPlugins.erase(trackIndex);
    
    // Add new plugin with RAII wrapper
    m_instrumentPlugins[trackIndex] = std::make_unique<PluginInstance>(
        std::move(instance), description);
    
    return true;
}

bool PluginManager::addEffectPlugin(int trackIndex,
                                   std::unique_ptr<juce::AudioPluginInstance> instance,
                                   const juce::PluginDescription& description,
                                   int position)
{
    if (!instance)
        return false;
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Get or create effect chain for track
    auto& effectChain = m_effectPlugins[trackIndex];
    
    // Create plugin wrapper
    auto plugin = std::make_unique<PluginInstance>(std::move(instance), description);
    
    // Insert at specified position or end
    if (position < 0 || position >= static_cast<int>(effectChain.size()))
    {
        effectChain.push_back(std::move(plugin));
    }
    else
    {
        effectChain.insert(effectChain.begin() + position, std::move(plugin));
    }
    
    return true;
}

bool PluginManager::removeInstrumentPlugin(int trackIndex)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_instrumentPlugins.find(trackIndex);
    if (it != m_instrumentPlugins.end())
    {
        // unique_ptr automatically handles cleanup
        m_instrumentPlugins.erase(it);
        return true;
    }
    
    return false;
}

bool PluginManager::removeEffectPlugin(int trackIndex, int effectIndex)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_effectPlugins.find(trackIndex);
    if (it != m_effectPlugins.end())
    {
        auto& effectChain = it->second;
        if (effectIndex >= 0 && effectIndex < static_cast<int>(effectChain.size()))
        {
            // unique_ptr automatically handles cleanup
            effectChain.erase(effectChain.begin() + effectIndex);
            
            // Remove empty chains
            if (effectChain.empty())
            {
                m_effectPlugins.erase(it);
            }
            
            return true;
        }
    }
    
    return false;
}

PluginManager::PluginInstance* PluginManager::getInstrumentPlugin(int trackIndex)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_instrumentPlugins.find(trackIndex);
    if (it != m_instrumentPlugins.end())
    {
        return it->second.get();
    }
    
    return nullptr;
}

std::vector<PluginManager::PluginInstance*> PluginManager::getEffectPlugins(int trackIndex)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::vector<PluginInstance*> result;
    
    auto it = m_effectPlugins.find(trackIndex);
    if (it != m_effectPlugins.end())
    {
        for (const auto& plugin : it->second)
        {
            result.push_back(plugin.get());
        }
    }
    
    return result;
}

void PluginManager::clearTrackPlugins(int trackIndex)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Remove instrument plugin
    m_instrumentPlugins.erase(trackIndex);
    
    // Remove effect plugins
    m_effectPlugins.erase(trackIndex);
}

void PluginManager::clearAllPlugins()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Clear all plugins - unique_ptr handles cleanup automatically
    m_instrumentPlugins.clear();
    m_effectPlugins.clear();
}

//==============================================================================
// Bridge Process Management

bool PluginManager::launchPluginBridge(const juce::String& formatName,
                                      const juce::String& identifier,
                                      bool useRosetta)
{
    auto bridge = findPluginHostBridge();
    if (!bridge.existsAsFile())
    {
        DBG("PluginManager: Bridge executable not found");
        return false;
    }
    
    // Build command with proper escaping
    const int ipcPort = 53621;  // Fixed port for MVP
    auto escapeArg = [](const juce::String& s) {
        return juce::String("\"") + s.replace("\"", "\\\"") + "\"";
    };
    
    juce::String cmd = bridge.getFullPathName()
                     + " --format=" + escapeArg(formatName)
                     + " --identifier=" + escapeArg(identifier)
                     + " --port=" + juce::String(ipcPort);
    
#if JUCE_MAC
    if (useRosetta)
    {
        cmd = "/usr/bin/arch -x86_64 " + cmd;
    }
#endif
    
    // Launch bridge process
    if (m_bridgeProcess->launch(cmd))
    {
        DBG("PluginManager: Bridge launched successfully");
        return true;
    }
    
    DBG("PluginManager: Failed to launch bridge");
    return false;
}

void PluginManager::terminateBridge()
{
    if (m_bridgeProcess)
    {
        m_bridgeProcess->terminate();
    }
}

bool PluginManager::isBridgeRunning() const
{
    return m_bridgeProcess && m_bridgeProcess->isRunning();
}

//==============================================================================
// Resource Statistics

PluginManager::ResourceStats PluginManager::getResourceStats() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    ResourceStats stats;
    stats.instrumentCount = m_instrumentPlugins.size();
    
    size_t totalEffects = 0;
    size_t activeEditors = 0;
    
    for (const auto& [trackIndex, plugin] : m_instrumentPlugins)
    {
        if (plugin && plugin->getEditor())
        {
            activeEditors++;
        }
    }
    
    for (const auto& [trackIndex, effectChain] : m_effectPlugins)
    {
        totalEffects += effectChain.size();
        for (const auto& effect : effectChain)
        {
            if (effect && effect->getEditor())
            {
                activeEditors++;
            }
        }
    }
    
    stats.effectCount = totalEffects;
    stats.totalPluginCount = stats.instrumentCount + stats.effectCount;
    stats.activeEditorCount = activeEditors;
    stats.bridgeActive = isBridgeRunning();
    
    return stats;
}

//==============================================================================
// Helper Methods

juce::File PluginManager::findPluginHostBridge()
{
    auto exe = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
    
    // Navigate up directory tree to project root
    auto dir = exe;
    for (int i = 0; i < 6; ++i)
    {
        dir = dir.getParentDirectory();
    }
    
    // Check candidate locations
    auto candidateApp = dir.getChildFile("PluginHostBridge_artefacts")
                          .getChildFile("Release")
                          .getChildFile("PluginHostBridge.app")
                          .getChildFile("Contents")
                          .getChildFile("MacOS")
                          .getChildFile("PluginHostBridge");
    
    if (candidateApp.existsAsFile())
        return candidateApp;
    
    auto candidateBin = dir.getChildFile("bin")
                          .getChildFile("PluginHostBridge");
    
    if (candidateBin.existsAsFile())
        return candidateBin;
    
    return {};
}

void PluginManager::timerCallback()
{
    // This runs on the message thread, so it's safe to access file system
    
    if (!m_isScanning || !m_scanner)
    {
        stopTimer();
        return;
    }
    
    try
    {
        juce::String pluginName;
        
        // Scan a few plugins per timer callback to avoid blocking UI
        for (int i = 0; i < 3; ++i)  // Scan up to 3 plugins per tick
        {
            if (!m_scanner)
                break;
                
            bool scanComplete = m_scanner->scanNextFile(false, pluginName);
            
            if (!pluginName.isEmpty())
            {
                m_currentPlugin = pluginName;
                m_scanProgress++;
                DBG("PluginManager: Scanning " << pluginName);
            }
            
            if (scanComplete)
            {
                DBG("PluginManager: Scan complete! Found " << m_knownPluginList.getNumTypes() << " plugins");
                stopTimer();
                m_isScanning = false;
                m_scanner.reset();
                
                // Save the results
                savePluginList();
                break;
            }
        }
    }
    catch (const std::exception& e)
    {
        DBG("PluginManager: Timer scan exception: " << e.what());
        // Continue scanning despite errors
    }
    catch (...)
    {
        DBG("PluginManager: Timer scan unknown exception");
        // Continue scanning despite errors
    }
}

} // namespace HAM