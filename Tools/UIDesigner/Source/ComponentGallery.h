#pragma once

#include <JuceHeader.h>
#include "UI/PulseComponentLibrary.h"
#include <memory>
#include <vector>

//==============================================================================
/**
 * Component Gallery with organized, scrollable view
 * All Pulse components neatly arranged without overlap
 */
class ComponentGallery : public juce::Component
{
public:
    ComponentGallery();
    ~ComponentGallery() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
private:
    //==============================================================================
    // Component container with scrolling
    class ScrollableComponentView : public juce::Component
    {
    public:
        ScrollableComponentView();
        
        void paint(juce::Graphics& g) override;
        void resized() override;
        
        // Add component sections
        void addSection(const juce::String& title, int yPosition);
        void addComponent(std::unique_ptr<juce::Component> comp, 
                         const juce::String& name,
                         int x, int y, int width, int height);
        
        int getTotalHeight() const { return totalHeight; }
        
    private:
        struct Section {
            juce::String title;
            int yPosition;
        };
        
        struct ComponentInfo {
            std::unique_ptr<juce::Component> component;
            juce::String name;
            juce::Rectangle<int> bounds;
        };
        
        std::vector<Section> sections;
        std::vector<ComponentInfo> components;
        int totalHeight = 0;
        
        // Grid helpers
        static constexpr int GRID_SIZE = 20;
        static constexpr int MARGIN = 40;
        static constexpr int SECTION_SPACING = 60;
    };
    
    //==============================================================================
    // Main components
    std::unique_ptr<juce::Viewport> viewport;
    std::unique_ptr<ScrollableComponentView> componentView;
    std::unique_ptr<HAM::PulseComponentLibrary> pulseLibrary;
    
    // Info panel
    juce::Label titleLabel;
    juce::Label infoLabel;
    juce::TextButton exportButton{"Export Layout"};
    juce::TextButton gridToggle{"Toggle Grid"};
    
    // Create all component sections
    void createComponentSections();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ComponentGallery)
};