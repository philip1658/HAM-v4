/*
  ==============================================================================

    PresetManager.cpp
    Implementation of preset management system

  ==============================================================================
*/

#include "PresetManager.h"

namespace HAM
{

//==============================================================================
PresetManager::PresetManager()
{
    // Set up preset directories
    auto appDataDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
    
    m_userPresetDirectory = appDataDir.getChildFile("HAM").getChildFile("Presets").getChildFile("User");
    m_factoryPresetDirectory = appDataDir.getChildFile("HAM").getChildFile("Presets").getChildFile("Factory");
    
    // Create directories if they don't exist
    m_userPresetDirectory.createDirectory();
    m_factoryPresetDirectory.createDirectory();
    
    // Create default categories
    m_categories = { "User", "Bass", "Lead", "Pad", "Drums", "FX", "Arp", "Sequence" };
}

PresetManager::~PresetManager() = default;

//==============================================================================
// Plugin Preset Management

bool PresetManager::savePluginPreset(juce::AudioPluginInstance* pluginInstance,
                                     const juce::String& presetName,
                                     const juce::String& category)
{
    if (!pluginInstance || presetName.isEmpty())
        return false;
    
    juce::ScopedLock lock(m_lock);
    
    // Get plugin state
    juce::MemoryBlock stateData;
    pluginInstance->getStateInformation(stateData);
    
    if (stateData.getSize() == 0)
        return false;
    
    // Create preset
    PluginPreset preset;
    preset.name = presetName;
    preset.category = category;
    preset.pluginName = pluginInstance->getPluginDescription().name;
    preset.pluginFormat = pluginInstance->getPluginDescription().pluginFormatName;
    preset.stateData = stateData;
    preset.lastModified = juce::Time::getCurrentTime();
    preset.isFactory = false;
    
    // Save to file
    auto presetFile = getPluginPresetFile(presetName);
    
    // Create preset data
    juce::ValueTree presetTree("PluginPreset");
    presetTree.setProperty("name", preset.name, nullptr);
    presetTree.setProperty("category", preset.category, nullptr);
    presetTree.setProperty("pluginName", preset.pluginName, nullptr);
    presetTree.setProperty("pluginFormat", preset.pluginFormat, nullptr);
    presetTree.setProperty("lastModified", preset.lastModified.toISO8601(true), nullptr);
    
    // Add state data as base64
    auto base64 = juce::Base64::toBase64(stateData.getData(), stateData.getSize());
    presetTree.setProperty("stateData", base64, nullptr);
    
    // Save to XML
    auto xml = presetTree.createXml();
    if (xml && xml->writeTo(presetFile))
    {
        // Add to cache
        m_pluginPresets.push_back(preset);
        return true;
    }
    
    return false;
}

bool PresetManager::loadPluginPreset(juce::AudioPluginInstance* pluginInstance,
                                     const juce::String& presetName)
{
    if (!pluginInstance || presetName.isEmpty())
        return false;
    
    juce::ScopedLock lock(m_lock);
    
    // Find preset in cache
    for (const auto& preset : m_pluginPresets)
    {
        if (preset.name == presetName)
        {
            // Check if plugin matches
            if (preset.pluginName != pluginInstance->getPluginDescription().name)
            {
                DBG("Warning: Loading preset from different plugin");
            }
            
            // Load state
            pluginInstance->setStateInformation(preset.stateData.getData(),
                                              static_cast<int>(preset.stateData.getSize()));
            return true;
        }
    }
    
    // Try loading from file
    auto presetFile = getPluginPresetFile(presetName);
    if (presetFile.existsAsFile())
    {
        auto xml = juce::XmlDocument::parse(presetFile);
        if (xml)
        {
            auto presetTree = juce::ValueTree::fromXml(*xml);
            if (presetTree.hasType("PluginPreset"))
            {
                // Get state data from base64
                auto base64 = presetTree.getProperty("stateData").toString();
                juce::MemoryOutputStream stream;
                juce::Base64::convertFromBase64(stream, base64);
                
                // Load state
                pluginInstance->setStateInformation(stream.getData(),
                                                  static_cast<int>(stream.getDataSize()));
                return true;
            }
        }
    }
    
    return false;
}

std::vector<PresetManager::PluginPreset> PresetManager::getPluginPresets(
    const juce::String& pluginName,
    const juce::String& pluginFormat)
{
    juce::ScopedLock lock(m_lock);
    
    std::vector<PluginPreset> results;
    
    for (const auto& preset : m_pluginPresets)
    {
        bool matches = true;
        
        if (pluginName.isNotEmpty() && preset.pluginName != pluginName)
            matches = false;
        
        if (pluginFormat.isNotEmpty() && preset.pluginFormat != pluginFormat)
            matches = false;
        
        if (matches)
            results.push_back(preset);
    }
    
    return results;
}

bool PresetManager::deletePluginPreset(const juce::String& presetName)
{
    juce::ScopedLock lock(m_lock);
    
    // Remove from cache
    m_pluginPresets.erase(
        std::remove_if(m_pluginPresets.begin(), m_pluginPresets.end(),
                      [&](const PluginPreset& p) { return p.name == presetName && !p.isFactory; }),
        m_pluginPresets.end());
    
    // Delete file
    auto presetFile = getPluginPresetFile(presetName);
    return presetFile.deleteFile();
}

//==============================================================================
// Pattern Preset Management

bool PresetManager::savePatternPreset(const juce::ValueTree& patternData,
                                      const juce::String& presetName,
                                      const juce::String& category)
{
    if (!patternData.isValid() || presetName.isEmpty())
        return false;
    
    juce::ScopedLock lock(m_lock);
    
    // Create preset
    PatternPreset preset;
    preset.name = presetName;
    preset.category = category;
    preset.patternData = patternData;
    preset.lastModified = juce::Time::getCurrentTime();
    preset.isFactory = false;
    
    // Save to file
    auto presetFile = getPatternPresetFile(presetName);
    
    // Create preset wrapper
    juce::ValueTree presetTree("PatternPreset");
    presetTree.setProperty("name", preset.name, nullptr);
    presetTree.setProperty("category", preset.category, nullptr);
    presetTree.setProperty("lastModified", preset.lastModified.toISO8601(true), nullptr);
    presetTree.addChild(patternData, -1, nullptr);
    
    // Save to XML
    auto xml = presetTree.createXml();
    if (xml && xml->writeTo(presetFile))
    {
        // Add to cache
        m_patternPresets.push_back(preset);
        return true;
    }
    
    return false;
}

juce::ValueTree PresetManager::loadPatternPreset(const juce::String& presetName)
{
    if (presetName.isEmpty())
        return {};
    
    juce::ScopedLock lock(m_lock);
    
    // Find preset in cache
    for (const auto& preset : m_patternPresets)
    {
        if (preset.name == presetName)
            return preset.patternData;
    }
    
    // Try loading from file
    auto presetFile = getPatternPresetFile(presetName);
    if (presetFile.existsAsFile())
    {
        auto xml = juce::XmlDocument::parse(presetFile);
        if (xml)
        {
            auto presetTree = juce::ValueTree::fromXml(*xml);
            if (presetTree.hasType("PatternPreset"))
            {
                // Return the pattern data (first child)
                if (presetTree.getNumChildren() > 0)
                    return presetTree.getChild(0);
            }
        }
    }
    
    return {};
}

//==============================================================================
// Preset Library Management

void PresetManager::initialize()
{
    rescanPresets();
}

void PresetManager::rescanPresets()
{
    juce::ScopedLock lock(m_lock);
    
    // Clear cache
    m_pluginPresets.clear();
    m_patternPresets.clear();
    
    // Scan user presets
    loadPresetsFromDirectory(m_userPresetDirectory, false);
    
    // Scan factory presets
    loadPresetsFromDirectory(m_factoryPresetDirectory, true);
}

void PresetManager::loadPresetsFromDirectory(const juce::File& directory, bool isFactory)
{
    // Find all preset files
    for (const auto& file : directory.findChildFiles(juce::File::findFiles, true, "*.hampreset"))
    {
        auto xml = juce::XmlDocument::parse(file);
        if (!xml)
            continue;
        
        auto tree = juce::ValueTree::fromXml(*xml);
        
        if (tree.hasType("PluginPreset"))
        {
            PluginPreset preset;
            preset.name = tree.getProperty("name");
            preset.category = tree.getProperty("category");
            preset.pluginName = tree.getProperty("pluginName");
            preset.pluginFormat = tree.getProperty("pluginFormat");
            preset.lastModified = juce::Time::fromISO8601(tree.getProperty("lastModified").toString());
            preset.isFactory = isFactory;
            
            // Load state data from base64
            auto base64 = tree.getProperty("stateData").toString();
            juce::MemoryOutputStream stream;
            juce::Base64::convertFromBase64(stream, base64);
            preset.stateData.setSize(stream.getDataSize());
            preset.stateData.copyFrom(stream.getData(), 0, stream.getDataSize());
            
            m_pluginPresets.push_back(preset);
        }
        else if (tree.hasType("PatternPreset"))
        {
            PatternPreset preset;
            preset.name = tree.getProperty("name");
            preset.category = tree.getProperty("category");
            preset.lastModified = juce::Time::fromISO8601(tree.getProperty("lastModified").toString());
            preset.isFactory = isFactory;
            
            if (tree.getNumChildren() > 0)
                preset.patternData = tree.getChild(0);
            
            m_patternPresets.push_back(preset);
        }
    }
}

//==============================================================================
// Helper Methods

juce::File PresetManager::getPluginPresetFile(const juce::String& presetName) const
{
    auto filename = juce::File::createLegalFileName(presetName) + ".hampreset";
    return m_userPresetDirectory.getChildFile("Plugins").getChildFile(filename);
}

juce::File PresetManager::getPatternPresetFile(const juce::String& presetName) const
{
    auto filename = juce::File::createLegalFileName(presetName) + ".hampreset";
    return m_userPresetDirectory.getChildFile("Patterns").getChildFile(filename);
}

juce::File PresetManager::getUserPresetDirectory() const
{
    return m_userPresetDirectory;
}

juce::File PresetManager::getFactoryPresetDirectory() const
{
    return m_factoryPresetDirectory;
}

std::vector<juce::String> PresetManager::getCategories() const
{
    juce::ScopedLock lock(m_lock);
    return m_categories;
}

bool PresetManager::createCategory(const juce::String& categoryName)
{
    if (categoryName.isEmpty())
        return false;
    
    juce::ScopedLock lock(m_lock);
    
    // Check if already exists
    if (std::find(m_categories.begin(), m_categories.end(), categoryName) != m_categories.end())
        return false;
    
    m_categories.push_back(categoryName);
    return true;
}

} // namespace HAM