/*
  ==============================================================================

    PluginScanWorker.cpp
    Standalone plugin scanner executable for HAM
    Scans plugins in a separate process to prevent crashes

  ==============================================================================
*/

#include <JuceHeader.h>
#include <iostream>

class PluginScanWorker : public juce::JUCEApplication,
                         private juce::Timer
{
public:
    const juce::String getApplicationName() override { return "HAM Plugin Scanner"; }
    const juce::String getApplicationVersion() override { return "1.0"; }
    bool moreThanOneInstanceAllowed() override { return true; }

    void initialise(const juce::String& commandLine) override
    {
        // Parse command line
        auto args = juce::StringArray::fromTokens(commandLine, " ", "\"");
        
        if (args.contains("--help"))
        {
            showHelp();
            quit();
            return;
        }
        
        if (args.contains("--scan"))
        {
            int index = args.indexOf("--scan");
            if (index >= 0 && index + 1 < args.size())
            {
                scanSinglePlugin(args[index + 1]);
            }
            else
            {
                std::cerr << "Error: --scan requires a plugin path" << std::endl;
            }
            quit();
            return;
        }
        
        // Default: scan all plugins
        scanAllPlugins();
        quit();
    }
    
    void shutdown() override {}

private:
    void showHelp()
    {
        std::cout << "HAM Plugin Scanner" << std::endl;
        std::cout << "Usage:" << std::endl;
        std::cout << "  PluginScanWorker              - Scan all plugins" << std::endl;
        std::cout << "  PluginScanWorker --scan PATH  - Scan specific plugin" << std::endl;
        std::cout << "  PluginScanWorker --help       - Show this help" << std::endl;
    }
    
    void scanSinglePlugin(const juce::String& pluginPath)
    {
        juce::File pluginFile(pluginPath);
        
        if (!pluginFile.exists())
        {
            std::cerr << "SCAN_ERROR: Plugin not found: " << pluginPath << std::endl;
            return;
        }
        
        juce::AudioPluginFormatManager formatManager;
        formatManager.addDefaultFormats();
        
        // Try to identify the format
        for (int i = 0; i < formatManager.getNumFormats(); ++i)
        {
            auto* format = formatManager.getFormat(i);
            if (!format) continue;
            
            // Check if this format can handle the file
            if (format->fileMightContainThisPluginType(pluginPath))
            {
                juce::OwnedArray<juce::PluginDescription> descriptions;
                format->findAllTypesForFile(descriptions, pluginPath);
                
                for (auto* desc : descriptions)
                {
                    if (desc)
                    {
                        // Output the plugin description as XML
                        if (auto xml = desc->createXml())
                        {
                            std::cout << "PLUGIN_FOUND:" << xml->toString(juce::XmlElement::TextFormat()) << std::endl;
                        }
                    }
                }
            }
        }
        
        std::cout << "SCAN_COMPLETE:" << pluginPath << std::endl;
    }
    
    void scanAllPlugins()
    {
        std::cout << "Starting comprehensive plugin scan..." << std::endl;
        
        juce::AudioPluginFormatManager formatManager;
        formatManager.addDefaultFormats();
        
        juce::KnownPluginList pluginList;
        int totalFound = 0;
        
        // Get application data directory
        auto appDataDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                            .getChildFile("CloneHAM");
        
        if (!appDataDir.exists())
        {
            appDataDir.createDirectory();
        }
        
        auto pluginListFile = appDataDir.getChildFile("Plugins.xml");
        
        // Scan each format
        for (int formatIndex = 0; formatIndex < formatManager.getNumFormats(); ++formatIndex)
        {
            auto* format = formatManager.getFormat(formatIndex);
            if (!format) continue;
            
            std::cout << "Scanning " << format->getName().toStdString() << " plugins..." << std::endl;
            
            auto searchPaths = format->getDefaultLocationsToSearch();
            
            // Create scanner
            auto deadMansPedal = juce::File::getSpecialLocation(juce::File::tempDirectory)
                                    .getChildFile("HAM_scan.lock");
            
            juce::PluginDirectoryScanner scanner(
                pluginList,
                *format,
                searchPaths,
                true,  // recursive
                deadMansPedal
            );
            
            juce::String pluginName;
            bool finished = false;
            
            while (!finished)
            {
                finished = scanner.scanNextFile(true, pluginName);
                
                if (!pluginName.isEmpty())
                {
                    totalFound++;
                    std::cout << "  Found: " << pluginName.toStdString() << std::endl;
                }
            }
            
            deadMansPedal.deleteFile();
        }
        
        std::cout << "Scan complete. Total plugins found: " << totalFound << std::endl;
        
        // Save the plugin list
        if (auto xml = pluginList.createXml())
        {
            if (xml->writeTo(pluginListFile))
            {
                std::cout << "Plugin list saved to: " << pluginListFile.getFullPathName().toStdString() << std::endl;
            }
        }
    }
    
    void timerCallback() override {}
};

START_JUCE_APPLICATION(PluginScanWorker)