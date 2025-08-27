#pragma once

#include <JuceHeader.h>

namespace HAM::UI {

/**
 * ArrowButton - A button that displays a clear arrow symbol
 * Used for navigation in scale slots and other UI elements
 */
class ArrowButton : public juce::Button
{
public:
    enum class Direction
    {
        Left,
        Right,
        Up,
        Down
    };
    
    ArrowButton(const juce::String& name, Direction dir)
        : juce::Button(name), m_direction(dir)
    {
        setSize(30, 30);
    }
    
    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = getLocalBounds().toFloat();
        
        // Background
        auto bgColor = juce::Colour(0xFF1A1A1A);
        if (shouldDrawButtonAsDown)
            bgColor = juce::Colour(0xFF00CCFF);
        else if (shouldDrawButtonAsHighlighted)
            bgColor = juce::Colour(0xFF2A2A2A);
            
        g.setColour(bgColor);
        g.fillRoundedRectangle(bounds, 4.0f);
        
        // Border
        g.setColour(shouldDrawButtonAsHighlighted ? juce::Colour(0xFF00CCFF) : juce::Colour(0xFF3A3A3A));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 1.0f);
        
        // Draw arrow
        g.setColour(juce::Colours::white);
        drawArrow(g, bounds.reduced(bounds.getWidth() * 0.25f));
    }
    
    void setDirection(Direction dir) 
    { 
        m_direction = dir;
        repaint();
    }
    
private:
    void drawArrow(juce::Graphics& g, juce::Rectangle<float> bounds)
    {
        juce::Path arrow;
        float centerX = bounds.getCentreX();
        float centerY = bounds.getCentreY();
        float size = bounds.getWidth() * 0.4f;
        
        switch (m_direction)
        {
            case Direction::Left:
                arrow.startNewSubPath(centerX + size/2, centerY - size/2);
                arrow.lineTo(centerX - size/2, centerY);
                arrow.lineTo(centerX + size/2, centerY + size/2);
                break;
                
            case Direction::Right:
                arrow.startNewSubPath(centerX - size/2, centerY - size/2);
                arrow.lineTo(centerX + size/2, centerY);
                arrow.lineTo(centerX - size/2, centerY + size/2);
                break;
                
            case Direction::Up:
                arrow.startNewSubPath(centerX - size/2, centerY + size/2);
                arrow.lineTo(centerX, centerY - size/2);
                arrow.lineTo(centerX + size/2, centerY + size/2);
                break;
                
            case Direction::Down:
                arrow.startNewSubPath(centerX - size/2, centerY - size/2);
                arrow.lineTo(centerX, centerY + size/2);
                arrow.lineTo(centerX + size/2, centerY - size/2);
                break;
        }
        
        // Draw as thick lines
        g.strokePath(arrow, juce::PathStrokeType(2.5f, juce::PathStrokeType::JointStyle::mitered,
                                                 juce::PathStrokeType::EndCapStyle::rounded));
    }
    
    Direction m_direction;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArrowButton)
};

} // namespace HAM::UI