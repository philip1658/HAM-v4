#pragma once

#include <JuceHeader.h>
#include <functional>

namespace HAM::UI {

/**
 * IconButton - A button that displays an icon instead of text
 * Used for navigation tabs with symbolic representations
 */
class IconButton : public juce::Button
{
public:
    enum class IconType
    {
        Sequencer,  // Horizontal lines (timeline)
        Mixer,      // Vertical sliders
        Settings,   // Cogwheel
        AddTrack,   // Plus sign
        RemoveTrack,// Minus sign
        Custom      // User-defined drawing
    };
    
    IconButton(const juce::String& name, IconType type = IconType::Custom)
        : juce::Button(name), m_iconType(type)
    {
        setSize(36, 36);
    }
    
    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = getLocalBounds().toFloat();
        
        // Background
        auto baseColor = m_isActive ? m_activeColor : m_baseColor;
        if (shouldDrawButtonAsDown)
            baseColor = baseColor.darker(0.2f);
        else if (shouldDrawButtonAsHighlighted)
            baseColor = baseColor.brighter(0.1f);
            
        g.setColour(baseColor);
        g.fillRoundedRectangle(bounds, 4.0f);
        
        // Border
        g.setColour(m_isActive ? m_activeColor.brighter(0.3f) : juce::Colour(0xFF3A3A3A));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 1.0f);
        
        // Draw icon
        g.setColour(m_isActive ? juce::Colours::white : juce::Colour(0xFFCCCCCC));
        
        auto iconBounds = bounds.reduced(bounds.getWidth() * 0.2f);
        
        switch (m_iconType)
        {
            case IconType::Sequencer:
                drawSequencerIcon(g, iconBounds);
                break;
                
            case IconType::Mixer:
                drawMixerIcon(g, iconBounds);
                break;
                
            case IconType::Settings:
                drawSettingsIcon(g, iconBounds);
                break;
                
            case IconType::AddTrack:
                drawAddIcon(g, iconBounds);
                break;
                
            case IconType::RemoveTrack:
                drawRemoveIcon(g, iconBounds);
                break;
                
            case IconType::Custom:
                if (onDrawIcon)
                    onDrawIcon(g, iconBounds);
                break;
        }
    }
    
    void clicked() override
    {
        juce::Logger::writeToLog("IconButton::clicked() - " + getName() + " button was clicked!");
        
        // Log if onClick is set
        if (onClick)
        {
            juce::Logger::writeToLog("IconButton::clicked() - onClick handler exists, calling it");
        }
        else
        {
            juce::Logger::writeToLog("IconButton::clicked() - WARNING: No onClick handler set!");
        }
        
        Button::clicked();  // Call parent class to trigger onClick callback
    }
    
    void mouseDown(const juce::MouseEvent& event) override
    {
        juce::Logger::writeToLog("IconButton::mouseDown() - " + getName() + " at position (" + 
                                juce::String(event.x) + ", " + juce::String(event.y) + ")");
        Button::mouseDown(event);
    }
    
    void mouseEnter(const juce::MouseEvent& event) override
    {
        juce::Logger::writeToLog("IconButton::mouseEnter() - " + getName());
        Button::mouseEnter(event);
    }
    
    void mouseExit(const juce::MouseEvent& event) override
    {
        juce::Logger::writeToLog("IconButton::mouseExit() - " + getName());
        Button::mouseExit(event);
    }
    
    void setActive(bool active) 
    { 
        juce::Logger::writeToLog("IconButton::setActive() - " + getName() + " active=" + 
                                juce::String(active ? "true" : "false"));
        m_isActive = active;
        repaint();
    }
    
    void setBaseColor(juce::Colour color) { m_baseColor = color; }
    void setActiveColor(juce::Colour color) { m_activeColor = color; }
    
    // Custom drawing callback for IconType::Custom
    std::function<void(juce::Graphics&, juce::Rectangle<float>)> onDrawIcon;
    
private:
    void drawSequencerIcon(juce::Graphics& g, juce::Rectangle<float> bounds)
    {
        // Modern minimalist sequencer icon - horizontal step pattern
        float stepWidth = bounds.getWidth() / 8.0f;
        float stepHeight = 3.0f;
        float dotSize = 2.0f;
        float centerY = bounds.getCentreY();
        
        // Draw 8 steps in a pattern suggesting rhythm
        float pattern[] = { 1.0f, 0.3f, 0.6f, 0.3f, 0.9f, 0.3f, 0.5f, 0.7f };
        
        for (int i = 0; i < 8; i++)
        {
            float x = bounds.getX() + i * stepWidth + stepWidth * 0.5f - dotSize * 0.5f;
            float height = stepHeight + (bounds.getHeight() * 0.4f * pattern[i]);
            float y = centerY - height * 0.5f;
            
            // Draw vertical lines of varying heights (like a step sequencer)
            g.fillRoundedRectangle(x - dotSize * 0.5f, y, dotSize, height, dotSize * 0.5f);
            
            // Add a small dot at the base for visual anchor
            if (pattern[i] > 0.5f)
            {
                g.fillEllipse(x - dotSize * 0.5f, centerY + bounds.getHeight() * 0.3f - dotSize,
                             dotSize, dotSize);
            }
        }
    }
    
    void drawMixerIcon(juce::Graphics& g, juce::Rectangle<float> bounds)
    {
        // Ultra-clean mixer icon with line-based faders
        float faderSpacing = bounds.getWidth() / 4.0f;
        float faderHeight = bounds.getHeight() * 0.75f;
        float lineWidth = 2.0f;
        float knobSize = 6.0f;
        float startY = bounds.getY() + (bounds.getHeight() - faderHeight) * 0.5f;
        
        // Fader positions for visual variety
        float positions[] = { 0.65f, 0.35f, 0.8f };
        
        for (int i = 0; i < 3; i++)
        {
            float x = bounds.getX() + faderSpacing * (i + 1);
            
            // Draw fader track (thin vertical line)
            g.setColour(juce::Colour(0xFF4A4A4A));
            g.fillRoundedRectangle(x - lineWidth * 0.5f, startY, lineWidth, faderHeight, lineWidth * 0.5f);
            
            // Draw fader knob (circle)
            float knobY = startY + faderHeight * (1.0f - positions[i]);
            g.setColour(m_isActive ? juce::Colours::white : juce::Colour(0xFFAAAAAA));
            g.fillEllipse(x - knobSize * 0.5f, knobY - knobSize * 0.5f, knobSize, knobSize);
            
            // Add subtle tick marks
            g.setColour(juce::Colour(0xFF5A5A5A));
            for (int tick = 0; tick <= 4; tick++)
            {
                float tickY = startY + (faderHeight / 4.0f) * tick;
                g.fillRect(x - 4.0f, tickY - 0.5f, 8.0f, 1.0f);
            }
        }
    }
    
    void drawSettingsIcon(juce::Graphics& g, juce::Rectangle<float> bounds)
    {
        // Modern settings icon - three horizontal sliders
        float lineHeight = 2.0f;
        float lineLength = bounds.getWidth() * 0.7f;
        float knobSize = 5.0f;
        float spacing = bounds.getHeight() / 4.0f;
        float startX = bounds.getX() + (bounds.getWidth() - lineLength) * 0.5f;
        
        // Knob positions for each slider (0.0 to 1.0)
        float positions[] = { 0.3f, 0.7f, 0.45f };
        
        for (int i = 0; i < 3; i++)
        {
            float y = bounds.getY() + spacing * (i + 1);
            
            // Draw slider track
            g.setColour(juce::Colour(0xFF4A4A4A));
            g.fillRoundedRectangle(startX, y - lineHeight * 0.5f, lineLength, lineHeight, lineHeight * 0.5f);
            
            // Draw slider knob
            float knobX = startX + lineLength * positions[i];
            g.setColour(m_isActive ? juce::Colours::white : juce::Colour(0xFFBBBBBB));
            g.fillEllipse(knobX - knobSize * 0.5f, y - knobSize * 0.5f, knobSize, knobSize);
            
            // Add subtle end markers
            g.setColour(juce::Colour(0xFF6A6A6A));
            g.fillRect(startX - 1.0f, y - 3.0f, 2.0f, 6.0f);
            g.fillRect(startX + lineLength - 1.0f, y - 3.0f, 2.0f, 6.0f);
        }
    }
    
    void drawAddIcon(juce::Graphics& g, juce::Rectangle<float> bounds)
    {
        // Clean, perfectly centered plus icon
        float thickness = 2.0f;
        float length = bounds.getWidth() * 0.4f;
        float centerX = bounds.getCentreX();
        float centerY = bounds.getCentreY();
        
        // Draw with rounded ends for softer appearance
        // Horizontal line
        g.fillRoundedRectangle(centerX - length/2, centerY - thickness/2, 
                               length, thickness, thickness * 0.5f);
        // Vertical line
        g.fillRoundedRectangle(centerX - thickness/2, centerY - length/2,
                               thickness, length, thickness * 0.5f);
    }
    
    void drawRemoveIcon(juce::Graphics& g, juce::Rectangle<float> bounds)
    {
        // Clean minus icon matching the plus icon style
        float thickness = 2.0f;
        float length = bounds.getWidth() * 0.4f;
        float centerX = bounds.getCentreX();
        float centerY = bounds.getCentreY();
        
        // Horizontal line with rounded ends
        g.fillRoundedRectangle(centerX - length/2, centerY - thickness/2,
                               length, thickness, thickness * 0.5f);
    }
    
    IconType m_iconType;
    bool m_isActive = false;
    juce::Colour m_baseColor{0xFF1A1A1A};
    juce::Colour m_activeColor{0xFF00CCFF};
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(IconButton)
};

} // namespace HAM::UI