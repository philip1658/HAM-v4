// SPDX-License-Identifier: MIT
#include <JuceHeader.h>
#include <iostream>

static juce::File getSettingsDir()
{
    auto dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                   .getChildFile("CloneHAM");
    if (!dir.isDirectory()) dir.createDirectory();
    return dir;
}

static juce::File getPluginsXmlFile()    { return getSettingsDir().getChildFile("Plugins.xml"); }
static juce::File getDeadMansPedalFile() { return getSettingsDir().getChildFile("DeadMansPedal.txt"); }
static juce::File getScanStatusFile()    { return getSettingsDir().getChildFile("ScanStatus.json"); }

struct Progress
{
    juce::String current;
    int scanned = 0;
    int total   = 0;
    bool isScanning = true;

    juce::var toVar() const
    {
        juce::DynamicObject::Ptr o = new juce::DynamicObject();
        o->setProperty("current", current);
        o->setProperty("scanned", scanned);
        o->setProperty("total", total);
        o->setProperty("isScanning", isScanning);
        return o.get();
    }
};

static void writeStatus(const Progress& p)
{
    getScanStatusFile().replaceWithText(juce::JSON::toString(p.toVar()));
}

int main (int argc, char** argv)
{
    // Initialize JUCE
    juce::ScopedJuceInitialiser_GUI init;
    
    std::cout << "=== HAM Plugin Scanner v0.1.0 ===" << std::endl;
    std::cout << "Scanning for VST3 and AudioUnit plugins..." << std::endl;
    
    // Create format manager and add ALL default formats
    juce::AudioPluginFormatManager formatManager;
    formatManager.addDefaultFormats();  // THIS IS THE KEY! Uses JUCE's default paths
    
    std::cout << "Available plugin formats: " << formatManager.getNumFormats() << std::endl;
    for (int i = 0; i < formatManager.getNumFormats(); ++i)
    {
        if (auto* format = formatManager.getFormat(i))
        {
            std::cout << "  - " << format->getName().toStdString() << std::endl;
        }
    }
    
    // Load existing plugin list if it exists
    juce::KnownPluginList pluginList;
    if (auto f = getPluginsXmlFile(); f.existsAsFile())
    {
        juce::XmlDocument doc(f);
        if (auto xml = doc.getDocumentElement())
        {
            pluginList.recreateFromXml(*xml);
            std::cout << "Loaded " << pluginList.getNumTypes() 
                     << " plugins from cache" << std::endl;
        }
    }
    
    // Prepare dead man's pedal for timeout protection
    auto deadMansPedal = getDeadMansPedalFile();
    deadMansPedal.replaceWithText("scanning");
    
    Progress progress;
    int totalScanned = 0;
    
    // Scan each format separately
    for (int formatIndex = 0; formatIndex < formatManager.getNumFormats(); ++formatIndex)
    {
        auto* format = formatManager.getFormat(formatIndex);
        if (!format) continue;
        
        std::cout << "\nScanning " << format->getName().toStdString() << " plugins..." << std::endl;
        
        // Get default search paths for this format
        juce::FileSearchPath searchPaths = format->getDefaultLocationsToSearch();
        
        // Also add common locations that might be missed
        #if JUCE_MAC
        if (format->getName().contains("VST3"))
        {
            searchPaths.add(juce::File("/Library/Audio/Plug-Ins/VST3"));
            searchPaths.add(juce::File::getSpecialLocation(juce::File::userHomeDirectory)
                              .getChildFile("Library/Audio/Plug-Ins/VST3"));
        }
        else if (format->getName().contains("AudioUnit"))
        {
            searchPaths.add(juce::File("/Library/Audio/Plug-Ins/Components"));
            searchPaths.add(juce::File::getSpecialLocation(juce::File::userHomeDirectory)
                              .getChildFile("Library/Audio/Plug-Ins/Components"));
        }
        #elif JUCE_WINDOWS
        if (format->getName().contains("VST3"))
        {
            searchPaths.add(juce::File("C:/Program Files/Common Files/VST3"));
            searchPaths.add(juce::File("C:/Program Files (x86)/Common Files/VST3"));
        }
        #endif
        
        std::cout << "Search paths:" << std::endl;
        for (int i = 0; i < searchPaths.getNumPaths(); ++i)
        {
            std::cout << "  - " << searchPaths[i].getFullPathName().toStdString() << std::endl;
        }
        
        // Create scanner for this format
        juce::PluginDirectoryScanner scanner(
            pluginList,
            *format,
            searchPaths,
            true,  // recursive
            deadMansPedal,
            false  // don't skip files if already in list (rescan all)
        );
        
        juce::String pluginBeingScanned;
        
        // Scan all plugins for this format
        while (true)
        {
            // Update dead man's pedal to prevent timeout
            deadMansPedal.replaceWithText("scanning");
            
            // Try to scan next file
            bool finished = scanner.scanNextFile(true, pluginBeingScanned);
            
            if (!pluginBeingScanned.isEmpty())
            {
                std::cout << "  Found: " << pluginBeingScanned.toStdString() << std::endl;
                totalScanned++;
                
                progress.current = pluginBeingScanned;
                progress.scanned = totalScanned;
                writeStatus(progress);
                
                // Save after each plugin found (in case of crash)
                if (auto xml = pluginList.createXml())
                {
                    xml->writeTo(getPluginsXmlFile());
                }
            }
            
            if (finished)
            {
                std::cout << "Finished scanning " << format->getName().toStdString() << std::endl;
                break;
            }
        }
    }
    
    // Final save
    if (auto xml = pluginList.createXml())
    {
        xml->writeTo(getPluginsXmlFile());
        std::cout << "\nTotal plugins found: " << pluginList.getNumTypes() << std::endl;
        
        // List all found plugins
        std::cout << "\nPlugin List:" << std::endl;
        for (const auto& desc : pluginList.getTypes())
        {
            std::cout << "  - " << desc.name.toStdString() 
                     << " by " << desc.manufacturerName.toStdString()
                     << " (" << desc.pluginFormatName.toStdString() << ")"
                     << std::endl;
        }
    }
    
    // Mark scanning as complete
    progress.isScanning = false;
    progress.total = totalScanned;
    writeStatus(progress);
    
    // Clean up dead man's pedal
    deadMansPedal.deleteFile();
    
    std::cout << "\nScanning complete!" << std::endl;
    return 0;
}