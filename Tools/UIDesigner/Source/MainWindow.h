#pragma once

#include <JuceHeader.h>
#include <memory>

class ComponentGallery;

//==============================================================================
/**
 * Main window for HAM UI Designer
 * Development tool for designing and testing UI components
 */
class MainWindow : public juce::DocumentWindow
{
public:
    MainWindow(const juce::String& name);
    ~MainWindow() override;
    
    void closeButtonPressed() override;
    
private:
    std::unique_ptr<ComponentGallery> gallery;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
};