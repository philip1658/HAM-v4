#pragma once

#include <JuceHeader.h>

namespace HAM {

//==============================================================================
/**
 * Base classes and shared definitions for HAM UI Components
 * Contains color palette, base component class, and shared utilities
 */

//==============================================================================
// Pulse Color Palette (EXACT from Pulse)
struct PulseColors
{
    // Background hierarchy
    static const juce::Colour BG_VOID;        // #000000 - Deepest black
    static const juce::Colour BG_DARKEST;     // #0A0A0A
    static const juce::Colour BG_DARK;        // #1A1A1A  
    static const juce::Colour BG_MID;         // #2A2A2A
    static const juce::Colour BG_LIGHT;       // #3A3A3A
    static const juce::Colour BG_RAISED;      // #4A4A4A
    static const juce::Colour BG_HIGHLIGHT;   // #5A5A5A
    
    // Text colors
    static const juce::Colour TEXT_PRIMARY;   // #FFFFFF
    static const juce::Colour TEXT_SECONDARY; // #CCCCCC
    static const juce::Colour TEXT_DIMMED;    // #888888
    static const juce::Colour TEXT_DISABLED;  // #555555
    
    // Track colors (Pulse accent colors)
    static const juce::Colour TRACK_MINT;     // #00FF88 - Primary accent
    static const juce::Colour TRACK_CYAN;     // #00D9FF
    static const juce::Colour TRACK_PINK;     // #FF0088
    static const juce::Colour TRACK_AMBER;    // #FFAA00
    static const juce::Colour TRACK_PURPLE;   // #FF00FF
    static const juce::Colour TRACK_BLUE;     // #0088FF
    static const juce::Colour TRACK_RED;      // #FF0044
    static const juce::Colour TRACK_YELLOW;   // #FFFF00
    
    // Special effects
    static const juce::Colour GLOW_CYAN;      // #00FFFF with alpha
    static const juce::Colour GLOW_GREEN;     // #00FF00 with alpha
    static const juce::Colour ERROR_RED;      // #FF0000
    static const juce::Colour WARNING_AMBER;  // #FFAA00
    
    // Get track color by index
    static juce::Colour getTrackColor(int index)
    {
        const juce::Colour trackColors[] = {
            TRACK_MINT, TRACK_CYAN, TRACK_PINK, TRACK_AMBER,
            TRACK_PURPLE, TRACK_BLUE, TRACK_RED, TRACK_YELLOW
        };
        return trackColors[index % 8];
    }
};

//==============================================================================
// Base resizable component with animations
class PulseComponent : public juce::Component
{
public:
    explicit PulseComponent(const juce::String& name) : componentName(name) {}
    virtual ~PulseComponent() = default;
    
    const juce::String& getName() const { return componentName; }
    void setScaleFactor(float scale) { scaleFactor = scale; resized(); }
    
protected:
    juce::String componentName;
    float scaleFactor = 1.0f;
    
    // Animation helpers
    float hoverAmount = 0.0f;
    float clickAnimation = 0.0f;
    float glowIntensity = 0.0f;
    bool isHovering = false;
    bool isPressed = false;
    
    // Shadow drawing helper
    void drawMultiLayerShadow(juce::Graphics& g, juce::Rectangle<float> bounds, 
                              int layers = 3, float spread = 4.0f)
    {
        for (int i = layers - 1; i >= 0; --i)
        {
            float offset = (i + 1) * spread;
            float alpha = 0.15f / (i + 1);
            
            g.setColour(juce::Colours::black.withAlpha(alpha));
            g.fillRoundedRectangle(bounds.translated(0, offset).expanded(offset * 0.5f), 3.0f);
        }
    }
    
    // Gradient helper
    void fillWithGradient(juce::Graphics& g, juce::Rectangle<float> bounds,
                         juce::Colour topColor, juce::Colour bottomColor)
    {
        juce::ColourGradient gradient(topColor, bounds.getX(), bounds.getY(),
                                     bottomColor, bounds.getX(), bounds.getBottom(), false);
        g.setGradientFill(gradient);
        g.fillRoundedRectangle(bounds, 3.0f);
    }
    
    // Animation value smoother
    float smoothValue(float current, float target, float speed = 0.15f)
    {
        return current + (target - current) * speed;
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PulseComponent)
};

//==============================================================================
// Shared utility functions
class UIUtils
{
public:
    // Calculate relative size based on component bounds
    static float getRelativeSize(const juce::Rectangle<int>& bounds, float percentage)
    {
        return juce::jmin(bounds.getWidth(), bounds.getHeight()) * percentage;
    }
    
    // Draw glow effect
    static void drawGlow(juce::Graphics& g, juce::Rectangle<float> bounds, 
                        juce::Colour glowColor, float intensity)
    {
        for (int i = 5; i > 0; --i)
        {
            float expansion = i * 3.0f * intensity;
            float alpha = (0.2f / i) * intensity;
            
            g.setColour(glowColor.withAlpha(alpha));
            g.drawRoundedRectangle(bounds.expanded(expansion), 3.0f, 1.0f);
        }
    }
    
    // Format value for display
    static juce::String formatValue(float value, const juce::String& suffix = "")
    {
        if (suffix.isEmpty())
            return juce::String(value, 2);
        return juce::String(value, 2) + " " + suffix;
    }
    
    // Spring animation calculator
    static float calculateSpring(float current, float target, float& velocity,
                                float stiffness = 0.3f, float damping = 0.7f)
    {
        float force = (target - current) * stiffness;
        velocity = (velocity + force) * damping;
        return current + velocity;
    }
};

} // namespace HAM