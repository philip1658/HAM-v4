#pragma once

#include <JuceHeader.h>
#include <map>
#include <memory>

namespace HAM {

//==============================================================================
/**
 * Component Showcase with 24x24 Grid System
 * 
 * Grid coordinates:
 * - Vertical: A-X (24 rows)
 * - Horizontal: 1-24 (24 columns)
 * 
 * All components are fully resizable and named for easy reference
 */
class ComponentShowcase : public juce::Component,
                          private juce::Timer
{
public:
    ComponentShowcase();
    ~ComponentShowcase() override;
    
    //==============================================================================
    // Component access by name
    juce::Component* getComponentByName(const juce::String& name);
    
    // Grid helpers
    juce::Rectangle<int> getGridCell(char row, int col, int rowSpan = 1, int colSpan = 1) const;
    juce::String getGridPosition(const juce::Point<int>& point) const;
    
    //==============================================================================
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseMove(const juce::MouseEvent& event) override;
    
private:
    //==============================================================================
    // Base resizable component
    class ResizableComponent : public juce::Component
    {
    public:
        ResizableComponent(const juce::String& componentName) : name(componentName) {}
        virtual ~ResizableComponent() = default;
        
        const juce::String& getComponentName() const { return name; }
        void setScaleFactor(float scale) { scaleFactor = scale; resized(); }
        
    protected:
        juce::String name;
        float scaleFactor = 1.0f;
    };
    
    //==============================================================================
    // SLIDERS
    class VerticalSlider : public ResizableComponent
    {
    public:
        enum class Style { Small, Large };
        
        VerticalSlider(const juce::String& name, Style style);
        void paint(juce::Graphics& g) override;
        void mouseDown(const juce::MouseEvent& event) override;
        void mouseDrag(const juce::MouseEvent& event) override;
        
    private:
        Style style;
        float value = 0.5f;
        float targetValue = 0.5f;
        juce::Colour trackColor;
        float glowIntensity = 0.0f;
    };
    
    class HorizontalSlider : public ResizableComponent
    {
    public:
        enum class Style { Small, Large };
        
        HorizontalSlider(const juce::String& name, Style style);
        void paint(juce::Graphics& g) override;
        void mouseDown(const juce::MouseEvent& event) override;
        void mouseDrag(const juce::MouseEvent& event) override;
        
    private:
        Style style;
        float value = 0.5f;
        juce::Colour trackColor;
    };
    
    //==============================================================================
    // BUTTONS
    class ModernButton : public ResizableComponent
    {
    public:
        enum class Style { Small, Large };
        enum class Type { Solid, Outline, Ghost };
        
        ModernButton(const juce::String& name, Style style, Type type = Type::Solid);
        void paint(juce::Graphics& g) override;
        void mouseEnter(const juce::MouseEvent& event) override;
        void mouseExit(const juce::MouseEvent& event) override;
        void mouseDown(const juce::MouseEvent& event) override;
        void mouseUp(const juce::MouseEvent& event) override;
        
    private:
        Style style;
        Type type;
        float hoverAmount = 0.0f;
        float clickAnimation = 0.0f;
        bool isHovering = false;
        bool isPressed = false;
    };
    
    //==============================================================================
    // TOGGLES
    class ToggleSwitch : public ResizableComponent
    {
    public:
        enum class Style { Small, Large };
        
        ToggleSwitch(const juce::String& name, Style style);
        void paint(juce::Graphics& g) override;
        void mouseDown(const juce::MouseEvent& event) override;
        
    private:
        Style style;
        bool isOn = false;
        float thumbPosition = 0.0f;
        float backgroundOpacity = 0.0f;
    };
    
    //==============================================================================
    // DROPDOWNS
    class Dropdown : public ResizableComponent
    {
    public:
        enum class Style { Small, Large };
        
        Dropdown(const juce::String& name, Style style);
        void paint(juce::Graphics& g) override;
        void mouseEnter(const juce::MouseEvent& event) override;
        void mouseExit(const juce::MouseEvent& event) override;
        void mouseDown(const juce::MouseEvent& event) override;
        
    private:
        Style style;
        juce::String selectedText = "Select...";
        float hoverAmount = 0.0f;
        bool isHovering = false;
        
        void drawArrow(juce::Graphics& g, juce::Rectangle<float> bounds);
    };
    
    //==============================================================================
    // BACKGROUNDS
    class Panel : public ResizableComponent
    {
    public:
        enum class Style { Flat, Raised, Recessed, Glass };
        
        Panel(const juce::String& name, Style style);
        void paint(juce::Graphics& g) override;
        
    private:
        Style style;
    };
    
    //==============================================================================
    // Grid system
    struct GridInfo
    {
        int cellWidth = 40;
        int cellHeight = 40;
        bool showGrid = true;
        bool showLabels = true;
        juce::Point<int> hoveredCell;
    };
    
    GridInfo grid;
    
    // Component storage
    std::map<juce::String, std::unique_ptr<ResizableComponent>> components;
    
    // Current hover position display
    juce::Label positionLabel;
    
    // Timer for animations
    void timerCallback() override;
    
    // Helper to create all showcase components
    void createShowcaseComponents();
    void layoutComponents();
    
    // Color scheme
    struct Colors
    {
        static const juce::Colour BG_DARKEST;
        static const juce::Colour BG_DARK;
        static const juce::Colour BG_MID;
        static const juce::Colour BG_LIGHT;
        static const juce::Colour BG_LIGHTER;
        
        static const juce::Colour TEXT_PRIMARY;
        static const juce::Colour TEXT_SECONDARY;
        static const juce::Colour TEXT_DIMMED;
        
        static const juce::Colour TRACK_MINT;
        static const juce::Colour TRACK_CYAN;
        static const juce::Colour TRACK_MAGENTA;
        static const juce::Colour TRACK_ORANGE;
    };
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ComponentShowcase)
};

} // namespace HAM