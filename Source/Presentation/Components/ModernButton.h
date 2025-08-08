/*
  ==============================================================================

    ModernButton.h
    Modern button component with multiple styles

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../Core/BaseComponent.h"
#include "../Core/DesignSystem.h"

namespace HAM {

/**
 * Modern button with multiple visual styles matching Pulse aesthetic
 */
class ModernButton : public juce::TextButton
{
public:
    enum class Style
    {
        Solid,
        Outline,
        Ghost,
        Gradient
    };
    
    explicit ModernButton(const juce::String& buttonText)
        : juce::TextButton(buttonText)
        , m_style(Style::Solid)
    {
        setClickingTogglesState(false);
        updateLookAndFeel();
    }
    
    ~ModernButton() override = default;
    
    void setButtonStyle(Style style)
    {
        m_style = style;
        updateLookAndFeel();
        repaint();
    }
    
    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, 
                    bool shouldDrawButtonAsDown) override
    {
        auto bounds = getLocalBounds().toFloat();
        const float cornerRadius = 3.0f;
        
        // Button state colors
        auto baseColor = DesignSystem::Colors::Primary::medium;
        if (getToggleState())
            baseColor = DesignSystem::Colors::Primary::bright;
        else if (shouldDrawButtonAsDown)
            baseColor = DesignSystem::Colors::Primary::dark;
        else if (shouldDrawButtonAsHighlighted)
            baseColor = DesignSystem::Colors::Primary::bright.withAlpha(0.8f);
        
        // Draw based on style
        switch (m_style)
        {
            case Style::Solid:
                g.setColour(baseColor);
                g.fillRoundedRectangle(bounds, cornerRadius);
                break;
                
            case Style::Outline:
                g.setColour(baseColor);
                g.drawRoundedRectangle(bounds.reduced(0.5f), cornerRadius, 1.5f);
                if (shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown)
                {
                    g.setColour(baseColor.withAlpha(0.1f));
                    g.fillRoundedRectangle(bounds, cornerRadius);
                }
                break;
                
            case Style::Ghost:
                if (shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown)
                {
                    g.setColour(baseColor.withAlpha(0.2f));
                    g.fillRoundedRectangle(bounds, cornerRadius);
                }
                break;
                
            case Style::Gradient:
                {
                    juce::ColourGradient gradient(
                        baseColor, bounds.getX(), bounds.getY(),
                        baseColor.darker(0.3f), bounds.getX(), bounds.getBottom(),
                        false
                    );
                    g.setGradientFill(gradient);
                    g.fillRoundedRectangle(bounds, cornerRadius);
                    
                    // Add glow effect when active
                    if (getToggleState())
                    {
                        g.setColour(baseColor.withAlpha(0.3f));
                        g.drawRoundedRectangle(bounds.expanded(2), cornerRadius + 2, 4.0f);
                    }
                }
                break;
        }
        
        // Draw text
        g.setColour(m_style == Style::Solid || m_style == Style::Gradient ? 
                   juce::Colours::black : baseColor);
        g.setFont(DesignSystem::Typography::body.bold);
        g.drawText(getButtonText(), bounds, juce::Justification::centred);
    }
    
private:
    Style m_style;
    
    void updateLookAndFeel()
    {
        // Update colors based on style
        switch (m_style)
        {
            case Style::Solid:
            case Style::Gradient:
                setColour(juce::TextButton::buttonColourId, 
                         DesignSystem::Colors::Primary::medium);
                setColour(juce::TextButton::textColourOffId, 
                         juce::Colours::black);
                break;
                
            case Style::Outline:
            case Style::Ghost:
                setColour(juce::TextButton::buttonColourId, 
                         juce::Colours::transparentWhite);
                setColour(juce::TextButton::textColourOffId, 
                         DesignSystem::Colors::Primary::medium);
                break;
        }
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModernButton)
};

} // namespace HAM