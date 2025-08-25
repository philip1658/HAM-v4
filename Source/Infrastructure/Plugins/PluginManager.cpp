/*
  ==============================================================================

    PluginManager.cpp
    Plugin resource management implementation

  ==============================================================================
*/

#include "PluginManager.h"
#include <iostream>

namespace HAM
{

//==============================================================================
PluginManager::PluginManager()
{
    try
    {
        // Initialize supported plugin formats using JUCE's defaults
        DBG("PluginManager: Initializing plugin formats...");
        
        // THIS IS THE FIX: Use addDefaultFormats() instead of manual registration
        // This automatically adds all supported formats with correct settings
        m_formatManager.addDefaultFormats();
        
        DBG("PluginManager: Total formats available: " << m_formatManager.getNumFormats());
        
        // List all available formats for debugging
        for (int i = 0; i < m_formatManager.getNumFormats(); ++i)
        {
            if (auto* format = m_formatManager.getFormat(i))
            {
                DBG("  Format " << i << ": " << format->getName());
                
                // Show default paths for each format
                auto paths = format->getDefaultLocationsToSearch();
                for (int j = 0; j < paths.getNumPaths(); ++j)
                {
                    DBG("    Search path: " << paths[j].getFullPathName());
                }
            }
        }
        
        // Create bridge process manager (for out-of-process plugin hosting)
        m_bridgeProcess = std::make_unique<BridgeProcess>();
        
        // Initialize audio processor graph for plugin chains
        m_processorGraph = std::make_unique<juce::AudioProcessorGraph>();
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
        std::cout << "PluginManager::initialise() called" << std::endl;
        
        // FIXED: Use the correct location on macOS
        // On macOS, JUCE uses ~/Library/AppName instead of ~/Library/Application Support/AppName
        auto appDataDir = juce::File::getSpecialLocation(juce::File::userHomeDirectory)
                            .getChildFile("Library")
                            .getChildFile("CloneHAM");
        
        if (!appDataDir.exists())
        {
            appDataDir.createDirectory();
        }
        
        m_pluginListFile = appDataDir.getChildFile("Plugins.xml");
        
        DBG("Plugin list file: " << m_pluginListFile.getFullPathName());
        
        // Load previously scanned plugins from cache
        if (m_pluginListFile.existsAsFile())
        {
            try
            {
                DBG("Loading plugin cache from: " << m_pluginListFile.getFullPathName());
                auto xml = juce::XmlDocument::parse(m_pluginListFile);
                if (xml)
                {
                    m_knownPluginList.recreateFromXml(*xml);
                    DBG("PluginManager: Successfully loaded " << m_knownPluginList.getNumTypes() << " plugins from cache!");
                    
                    // Print first 10 loaded plugins for debugging
                    int count = 0;
                    for (const auto& desc : m_knownPluginList.getTypes())
                    {
                        if (count++ < 10)
                        {
                            DBG("  - " << desc.name << " by " << desc.manufacturerName 
                                << " (" << desc.pluginFormatName << ")");
                        }
                    }
                    if (m_knownPluginList.getNumTypes() > 10)
                    {
                        DBG("  ... and " << (m_knownPluginList.getNumTypes() - 10) << " more plugins");
                    }
                }
            }
            catch (...)
            {
                DBG("PluginManager: Could not load plugin cache - starting fresh");
            }
        }
        else
        {
            DBG("PluginManager: No plugin cache found - will scan on first use");
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

void PluginManager::startSandboxedScan(bool async)
{
    try
    {
        std::cout << "PluginManager::startSandboxedScan called, async=" << async << std::endl;
        DBG("PluginManager::startSandboxedScan - Starting plugin scan...");
        
        // Stop any existing scan
        if (m_isScanning)
        {
            DBG("PluginManager: Stopping existing scan...");
            stopTimer();
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
        
        // Launch external scanner process for safety
        auto scannerExe = findPluginScanner();
        if (scannerExe.existsAsFile())
        {
            DBG("PluginManager: Launching external scanner: " << scannerExe.getFullPathName());
            
            // Launch scanner process
            juce::ChildProcess scannerProcess;
            if (scannerProcess.start(scannerExe.getFullPathName()))
            {
                DBG("PluginManager: Scanner launched successfully");
                
                // Wait for scanner to complete (with timeout)
                if (async)
                {
                    // Start monitoring thread
                    m_isScanning = true;
                    auto scannerPath = scannerExe.getFullPathName();
                    m_scanThread = std::make_unique<std::thread>([this, scannerPath]() {
                        juce::ChildProcess localScanner;
                        localScanner.start(scannerPath);
                        localScanner.waitForProcessToFinish(60000);  // 60 second timeout
                        m_isScanning = false;
                        
                        // Reload plugin list after scan completes
                        juce::MessageManager::callAsync([this]() {
                            loadPluginList();
                        });
                    });
                }
                else
                {
                    // Wait synchronously
                    scannerProcess.waitForProcessToFinish(60000);
                    loadPluginList();
                }
                
                return;
            }
        }
        
        // Fallback to internal scanning if external scanner not available
        DBG("PluginManager: External scanner not found, using internal scanning");
        
        m_currentFormatIndex = 0;
        m_isScanning = true;
        
        if (async)
        {
            // Use timer-based scanning on message thread
            startTimer(50);  // Check every 50ms
        }
        else
        {
            // Synchronous internal scan
            performInternalScan();
        }
    }
    catch (const std::exception& e)
    {
        DBG("PluginManager::startSandboxedScan exception: " << e.what());
        m_isScanning = false;
    }
    catch (...)
    {
        DBG("PluginManager::startSandboxedScan unknown exception");
        m_isScanning = false;
    }
}

void PluginManager::performInternalScan()
{
    DBG("PluginManager: Starting internal plugin scan...");
    
    int totalFound = 0;
    
    // Scan all formats
    for (int formatIndex = 0; formatIndex < m_formatManager.getNumFormats(); ++formatIndex)
    {
        auto* format = m_formatManager.getFormat(formatIndex);
        if (!format) continue;
        
        DBG("\nScanning " << format->getName() << " plugins...");
        
        // Get the default search paths for this format
        auto searchPaths = format->getDefaultLocationsToSearch();
        
        DBG("Search paths for " << format->getName() << ":");
        for (int i = 0; i < searchPaths.getNumPaths(); ++i)
        {
            DBG("  - " << searchPaths[i].getFullPathName());
        }
        
        // Create dead man's pedal for timeout protection
        auto deadMansPedal = juce::File::getSpecialLocation(juce::File::tempDirectory)
                                .getChildFile("HAM_scan_" + format->getName() + ".lock");
        
        // Create the scanner with proper paths
        juce::PluginDirectoryScanner scanner(
            m_knownPluginList,
            *format,
            searchPaths,
            true,  // recursive search
            deadMansPedal
        );
        
        // Scan until no more plugins are found
        juce::String pluginName;
        bool finished = false;
        
        while (!finished)
        {
            // scanNextFile returns true when scanning is complete
            finished = scanner.scanNextFile(true, pluginName);
            
            if (!pluginName.isEmpty())
            {
                m_currentPlugin = pluginName;
                m_scanProgress++;
                totalFound++;
                DBG("  ✓ Found: " << pluginName);
            }
            
            // Check for failures
            auto failedFiles = scanner.getFailedFiles();
            if (!failedFiles.isEmpty())
            {
                DBG("  ✗ Failed to scan " << failedFiles.size() << " file(s)");
            }
        }
        
        // Clean up lock file
        deadMansPedal.deleteFile();
    }
    
    DBG("\nPlugin scan complete! Total plugins found: " << totalFound);
    DBG("Plugin list now contains " << m_knownPluginList.getNumTypes() << " plugins");
    
    // Save the results
    savePluginList();
    m_isScanning = false;
}

void PluginManager::loadPluginList()
{
    if (m_pluginListFile.existsAsFile())
    {
        try
        {
            auto xml = juce::XmlDocument::parse(m_pluginListFile);
            if (xml)
            {
                m_knownPluginList.recreateFromXml(*xml);
                DBG("PluginManager: Reloaded " << m_knownPluginList.getNumTypes() << " plugins after scan");
            }
        }
        catch (...)
        {
            DBG("PluginManager: Could not reload plugin list");
        }
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
        if (auto xml = m_knownPluginList.createXml())
        {
            if (!xml->writeTo(m_pluginListFile))
            {
                DBG("PluginManager: Could not save plugin list to: " << m_pluginListFile.getFullPathName());
            }
            else
            {
                DBG("PluginManager: Successfully saved " << m_knownPluginList.getNumTypes() << " plugins");
            }
        }
    }
    catch (const std::exception& e)
    {
        DBG("PluginManager::savePluginList exception: " << e.what());
    }
    catch (...)
    {
        DBG("PluginManager::savePluginList unknown exception");
    }
}

//==============================================================================
// Plugin Instantiation

std::unique_ptr<juce::AudioPluginInstance> PluginManager::createPluginInstance(
    const juce::PluginDescription& description,
    double sampleRate,
    int blockSize)
{
    juce::String errorMessage;
    
    // Try to create the plugin instance
    auto instance = m_formatManager.createPluginInstance(
        description,
        sampleRate,
        blockSize,
        errorMessage
    );
    
    if (!instance)
    {
        DBG("PluginManager: Failed to create plugin instance: " << errorMessage);
        return nullptr;
    }
    
    DBG("PluginManager: Successfully created instance of " << description.name);
    return instance;
}

void PluginManager::createPluginInstanceAsync(
    const juce::PluginDescription& description,
    double sampleRate,
    int blockSize,
    std::function<void(std::unique_ptr<juce::AudioPluginInstance>, const juce::String&)> callback)
{
    // Use JUCE's async instantiation (important for AudioUnit v3)
    m_formatManager.createPluginInstanceAsync(
        description,
        sampleRate,
        blockSize,
        [callback](std::unique_ptr<juce::AudioPluginInstance> instance, const juce::String& error) {
            callback(std::move(instance), error);
        }
    );
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

juce::File PluginManager::findPluginScanner()
{
    // Look for the PluginScanWorker executable
    auto exe = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
    
    // Check in build directory
    auto buildDir = exe.getParentDirectory().getParentDirectory();
    auto scanner = buildDir.getChildFile("PluginScanWorker");
    
    if (scanner.existsAsFile())
        return scanner;
    
    // Check in same directory as main app
    scanner = exe.getParentDirectory().getChildFile("PluginScanWorker");
    
    if (scanner.existsAsFile())
        return scanner;
    
    // Check in bin directory
    scanner = buildDir.getChildFile("bin").getChildFile("PluginScanWorker");
    
    if (scanner.existsAsFile())
        return scanner;
    
    return {};
}

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
    // Timer-based internal scanning
    if (!m_isScanning)
    {
        stopTimer();
        return;
    }
    
    // Check if we have a valid format to scan
    if (m_currentFormatIndex >= m_formatManager.getNumFormats())
    {
        // All formats scanned
        DBG("PluginManager: All formats scanned! Total plugins found: " << m_knownPluginList.getNumTypes());
        
        stopTimer();
        m_isScanning = false;
        savePluginList();
        return;
    }
    
    // Create scanner for current format if needed
    if (!m_scanner)
    {
        auto* format = m_formatManager.getFormat(m_currentFormatIndex);
        if (format)
        {
            DBG("\nPluginManager: Scanning " << format->getName() << " plugins...");
            
            auto searchPaths = format->getDefaultLocationsToSearch();
            
            // Log the paths we're searching
            DBG("Search paths:");
            for (int i = 0; i < searchPaths.getNumPaths(); ++i)
            {
                auto path = searchPaths[i];
                DBG("  - " << path.getFullPathName() 
                    << (path.exists() ? " [EXISTS]" : " [NOT FOUND]"));
            }
            auto deadMansPedal = juce::File::getSpecialLocation(juce::File::tempDirectory)
                                    .getChildFile("HAM_scan.lock");
            
            m_scanner = std::make_unique<juce::PluginDirectoryScanner>(
                m_knownPluginList,
                *format,
                searchPaths,
                true,
                deadMansPedal
            );
        }
    }
    
    // Scan a few plugins per timer callback
    if (m_scanner)
    {
        juce::String pluginName;
        
        for (int i = 0; i < 5; ++i)  // Scan up to 5 plugins per tick
        {
            bool finished = m_scanner->scanNextFile(true, pluginName);
            
            if (!pluginName.isEmpty())
            {
                m_currentPlugin = pluginName;
                m_scanProgress++;
                DBG("  Found: " << pluginName);
            }
            
            if (finished)
            {
                // Move to next format
                DBG("PluginManager: Format " << (m_currentFormatIndex + 1) << "/" 
                    << m_formatManager.getNumFormats() << " complete");
                
                m_scanner.reset();
                m_currentFormatIndex++;
                break;
            }
        }
    }
}

} // namespace HAM