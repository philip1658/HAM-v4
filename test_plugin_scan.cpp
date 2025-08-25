/*
  Test program to verify plugin scanning works correctly
*/

#include <JuceHeader.h>
#include <iostream>

class PluginScanTest : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override { return "Plugin Scan Test"; }
    const juce::String getApplicationVersion() override { return "1.0"; }
    bool moreThanOneInstanceAllowed() override { return true; }

    void initialise(const juce::String&) override
    {
        std::cout << "\n=== HAM Plugin Scan Test ===\n" << std::endl;
        
        // Initialize the plugin format manager
        juce::AudioPluginFormatManager formatManager;
        formatManager.addDefaultFormats();
        
        std::cout << "Initialized " << formatManager.getNumFormats() << " plugin formats:" << std::endl;
        
        for (int i = 0; i < formatManager.getNumFormats(); ++i)
        {
            if (auto* format = formatManager.getFormat(i))
            {
                std::cout << "  " << (i + 1) << ". " << format->getName() << std::endl;
                
                // Get search paths
                auto searchPaths = format->getDefaultLocationsToSearch();
                std::cout << "     Search paths:" << std::endl;
                
                for (int j = 0; j < searchPaths.getNumPaths(); ++j)
                {
                    auto path = searchPaths[j];
                    std::cout << "       - " << path.getFullPathName();
                    
                    if (path.exists())
                    {
                        if (path.isDirectory())
                        {
                            auto numFiles = path.getNumberOfChildFiles(juce::File::findFilesAndDirectories);
                            std::cout << " [EXISTS - " << numFiles << " items]";
                        }
                        else
                        {
                            std::cout << " [EXISTS - single file]";
                        }
                    }
                    else
                    {
                        std::cout << " [NOT FOUND]";
                    }
                    std::cout << std::endl;
                }
            }
        }
        
        // Now do actual scanning
        std::cout << "\n=== Starting Plugin Scan ===\n" << std::endl;
        
        juce::KnownPluginList pluginList;
        int totalFound = 0;
        
        for (int formatIndex = 0; formatIndex < formatManager.getNumFormats(); ++formatIndex)
        {
            auto* format = formatManager.getFormat(formatIndex);
            if (!format) continue;
            
            std::cout << "\nScanning " << format->getName() << " plugins..." << std::endl;
            
            auto searchPaths = format->getDefaultLocationsToSearch();
            
            // Create scanner
            auto deadMansPedal = juce::File::getSpecialLocation(juce::File::tempDirectory)
                                    .getChildFile("HAM_test_scan.lock");
            
            juce::PluginDirectoryScanner scanner(
                pluginList,
                *format,
                searchPaths,
                true,  // recursive
                deadMansPedal
            );
            
            juce::String pluginName;
            bool finished = false;
            int formatCount = 0;
            
            while (!finished)
            {
                finished = scanner.scanNextFile(true, pluginName);
                
                if (!pluginName.isEmpty())
                {
                    formatCount++;
                    totalFound++;
                    std::cout << "  ✓ Found: " << pluginName << std::endl;
                }
            }
            
            if (formatCount == 0)
            {
                std::cout << "  (No " << format->getName() << " plugins found)" << std::endl;
            }
            else
            {
                std::cout << "  Total " << format->getName() << ": " << formatCount << std::endl;
            }
            
            deadMansPedal.deleteFile();
        }
        
        std::cout << "\n=== Scan Complete ===" << std::endl;
        std::cout << "Total plugins found: " << totalFound << std::endl;
        std::cout << "\nPlugin List:" << std::endl;
        
        for (const auto& plugin : pluginList.getTypes())
        {
            std::cout << "  • " << plugin.name 
                      << " by " << plugin.manufacturerName
                      << " (" << plugin.pluginFormatName << ")"
                      << std::endl;
        }
        
        quit();
    }

    void shutdown() override {}
};

START_JUCE_APPLICATION(PluginScanTest)