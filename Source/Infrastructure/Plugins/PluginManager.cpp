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
    // Initialize supported plugin formats
    m_formatManager.addDefaultFormats();
    
    // Create bridge process manager
    m_bridgeProcess = std::make_unique<BridgeProcess>();
}

PluginManager::~PluginManager()
{
    // Clean up all resources in proper order
    clearAllPlugins();
    terminateBridge();
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

} // namespace HAM