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
    
    void setActive(bool active) 
    { 
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
        // Draw horizontal lines representing timeline/sequence
        float lineHeight = 2.0f;
        float spacing = bounds.getHeight() / 4.0f;
        
        for (int i = 0; i < 3; i++)
        {
            float y = bounds.getY() + spacing * (i + 1) - lineHeight / 2;
            g.fillRoundedRectangle(bounds.getX(), y, bounds.getWidth(), lineHeight, 1.0f);
        }
    }
    
    void drawMixerIcon(juce::Graphics& g, juce::Rectangle<float> bounds)
    {
        // Draw vertical sliders
        float sliderWidth = bounds.getWidth() / 5.0f;
        float spacing = bounds.getWidth() / 3.0f;
        
        for (int i = 0; i < 3; i++)
        {
            float x = bounds.getX() + spacing * i + sliderWidth / 2;
            
            // Slider track
            g.setColour(juce::Colour(0xFF555555));
            g.fillRoundedRectangle(x, bounds.getY(), sliderWidth, bounds.getHeight(), 1.0f);
            
            // Slider handle
            g.setColour(m_isActive ? juce::Colours::white : juce::Colour(0xFFCCCCCC));
            float handleY = bounds.getY() + bounds.getHeight() * (0.3f + i * 0.2f);
            g.fillEllipse(x - sliderWidth * 0.3f, handleY - sliderWidth * 0.3f,
                         sliderWidth * 1.6f, sliderWidth * 1.6f);
        }
    }
    
    void drawSettingsIcon(juce::Graphics& g, juce::Rectangle<float> bounds)
    {
        // Draw cogwheel
        float centerX = bounds.getCentreX();
        float centerY = bounds.getCentreY();
        float outerRadius = bounds.getWidth() * 0.4f;
        float innerRadius = outerRadius * 0.6f;
        float holeRadius = innerRadius * 0.5f;
        int numTeeth = 8;
        
        juce::Path cogwheel;
        
        for (int i = 0; i < numTeeth; i++)
        {
            float angle = (i * 2.0f * juce::MathConstants<float>::pi) / numTeeth;
            float toothAngle = juce::MathConstants<float>::pi / numTeeth * 0.3f;
            
            float angle1 = angle - toothAngle;
            float angle2 = angle + toothAngle;
            float angle3 = angle + juce::MathConstants<float>::pi / numTeeth - toothAngle;
            float angle4 = angle + juce::MathConstants<float>::pi / numTeeth + toothAngle;
            
            if (i == 0)
            {
                cogwheel.startNewSubPath(centerX + outerRadius * std::cos(angle1),
                                        centerY + outerRadius * std::sin(angle1));
            }
            
            cogwheel.lineTo(centerX + outerRadius * std::cos(angle2),
                           centerY + outerRadius * std::sin(angle2));
            cogwheel.lineTo(centerX + innerRadius * std::cos(angle3),
                           centerY + innerRadius * std::sin(angle3));
            cogwheel.lineTo(centerX + innerRadius * std::cos(angle4),
                           centerY + innerRadius * std::sin(angle4));
        }
        
        cogwheel.closeSubPath();
        
        // Add center hole as a new sub-path (will be subtracted)
        cogwheel.addEllipse(centerX - holeRadius, centerY - holeRadius,
                           holeRadius * 2, holeRadius * 2);
        
        g.fillPath(cogwheel);
    }
    
    void drawAddIcon(juce::Graphics& g, juce::Rectangle<float> bounds)
    {
        // Draw plus sign
        float thickness = 2.5f;
        float length = bounds.getWidth() * 0.5f;
        float centerX = bounds.getCentreX();
        float centerY = bounds.getCentreY();
        
        // Horizontal line
        g.fillRoundedRectangle(centerX - length/2, centerY - thickness/2, 
                               length, thickness, 1.0f);
        // Vertical line
        g.fillRoundedRectangle(centerX - thickness/2, centerY - length/2,
                               thickness, length, 1.0f);
    }
    
    void drawRemoveIcon(juce::Graphics& g, juce::Rectangle<float> bounds)
    {
        // Draw minus sign
        float thickness = 2.5f;
        float length = bounds.getWidth() * 0.5f;
        float centerX = bounds.getCentreX();
        float centerY = bounds.getCentreY();
        
        // Horizontal line only
        g.fillRoundedRectangle(centerX - length/2, centerY - thickness/2,
                               length, thickness, 1.0f);
    }
    
    IconType m_iconType;
    bool m_isActive = false;
    juce::Colour m_baseColor{0xFF1A1A1A};
    juce::Colour m_activeColor{0xFF00CCFF};
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(IconButton)
};

} // namespace HAM::UI