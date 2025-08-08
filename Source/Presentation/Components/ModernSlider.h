// SPDX-License-Identifier: MIT
#pragma once

#include <JuceHeader.h>
#include "../Core/BaseComponent.h"
#include "../Core/DesignSystem.h"

namespace HAM::UI {

// ==========================================
// Modern Slider - Vertical with no thumb
// ==========================================
class ModernSlider : public BaseComponent {
public:
    ModernSlider(bool vertical = true) 
        : m_vertical(vertical) {
        setInterceptsMouseClicks(true, false);
    }
    
    // Value management
    void setValue(float value) {
        float normalized = (value - m_min) / (m_max - m_min);
        m_normalizedValue = juce::jlimit(0.0f, 1.0f, normalized);
        repaint();
        
        if (onValueChange) {
            onValueChange(getValue());
        }
    }
    
    float getValue() const {
        return m_min + (m_normalizedValue * (m_max - m_min));
    }
    
    void setRange(float min, float max, float step = 0.0f) {
        m_min = min;
        m_max = max;
        m_step = step;
        
        // Re-clamp current value
        setValue(getValue());
    }
    
    void setLabel(const juce::String& label) {
        m_label = label;
        repaint();
    }
    
    void setTrackColor(const juce::Colour& color) {
        m_trackColor = color;
        repaint();
    }
    
    void setShowValue(bool show) {
        m_showValue = show;
        repaint();
    }
    
    // Paint
    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();
        
        // Calculate track bounds (22px wide for vertical)
        auto trackBounds = m_vertical 
            ? bounds.withWidth(scaled(DesignSystem::Dimensions::SLIDER_TRACK_WIDTH))
                    .withX((bounds.getWidth() - scaled(DesignSystem::Dimensions::SLIDER_TRACK_WIDTH)) * 0.5f)
            : bounds.withHeight(scaled(DesignSystem::Dimensions::SLIDER_TRACK_WIDTH))
                    .withY((bounds.getHeight() - scaled(DesignSystem::Dimensions::SLIDER_TRACK_WIDTH)) * 0.5f);
        
        // Draw shadow for depth
        g.setColour(juce::Colour(0x40000000));
        g.fillRoundedRectangle(trackBounds.translated(0, scaled(1)), 
                               scaled(DesignSystem::Dimensions::CORNER_RADIUS));
        
        // Track background (recessed)
        juce::ColourGradient trackGradient = DesignSystem::createVerticalGradient(
            trackBounds,
            DesignSystem::Colors::getColor(DesignSystem::Colors::BG_RECESSED).withAlpha(0.9f),
            DesignSystem::Colors::getColor(DesignSystem::Colors::BG_RECESSED).withAlpha(0.7f)
        );
        g.setGradientFill(trackGradient);
        g.fillRoundedRectangle(trackBounds, scaled(DesignSystem::Dimensions::CORNER_RADIUS));
        
        // Track border
        g.setColour(DesignSystem::Colors::getColor(DesignSystem::Colors::BORDER));
        g.drawRoundedRectangle(trackBounds, scaled(DesignSystem::Dimensions::CORNER_RADIUS), 
                               scaled(DesignSystem::Dimensions::BORDER_WIDTH));
        
        // Fill (value indicator)
        if (m_normalizedValue > 0.01f) {
            auto fillBounds = m_vertical
                ? trackBounds.withTrimmedTop(trackBounds.getHeight() * (1.0f - m_normalizedValue))
                : trackBounds.withTrimmedRight(trackBounds.getWidth() * (1.0f - m_normalizedValue));
            
            // Gradient fill
            juce::ColourGradient fillGradient = DesignSystem::createVerticalGradient(
                fillBounds,
                m_trackColor.withAlpha(0.8f),
                m_trackColor.withAlpha(0.4f)
            );
            g.setGradientFill(fillGradient);
            g.fillRoundedRectangle(fillBounds, scaled(DesignSystem::Dimensions::CORNER_RADIUS));
            
            // Glow effect on fill
            g.setColour(m_trackColor.withAlpha(0.2f));
            g.fillRoundedRectangle(fillBounds.expanded(scaled(1)), 
                                  scaled(DesignSystem::Dimensions::CORNER_RADIUS));
        }
        
        // Line indicator (instead of thumb)
        float indicatorPos = m_vertical 
            ? trackBounds.getY() + trackBounds.getHeight() * (1.0f - m_normalizedValue)
            : trackBounds.getX() + trackBounds.getWidth() * m_normalizedValue;
        
        // White line indicator
        g.setColour(juce::Colours::white.withAlpha(0.9f));
        if (m_vertical) {
            // Horizontal line across and beyond track
            g.fillRect(trackBounds.getX() - scaled(4), 
                      indicatorPos - scaled(1), 
                      trackBounds.getWidth() + scaled(8), 
                      scaled(DesignSystem::Dimensions::SLIDER_INDICATOR_HEIGHT));
        } else {
            // Vertical line
            g.fillRect(indicatorPos - scaled(1), 
                      trackBounds.getY() - scaled(4),
                      scaled(DesignSystem::Dimensions::SLIDER_INDICATOR_HEIGHT), 
                      trackBounds.getHeight() + scaled(8));
        }
        
        // Label
        if (m_label.isNotEmpty()) {
            g.setColour(DesignSystem::Colors::getColor(DesignSystem::Colors::TEXT_MUTED));
            g.setFont(DesignSystem::Typography::getSmallFont().withHeight(scaled(10)));
            
            auto labelBounds = m_vertical 
                ? bounds.removeFromBottom(scaled(20))
                : bounds.removeFromRight(scaled(40));
                
            g.drawText(m_label, labelBounds, juce::Justification::centred);
        }
        
        // Value display (on hover or if enabled)
        if ((m_showValue || m_isHovered) && !m_isDragging) {
            auto valueBounds = trackBounds.withHeight(scaled(20))
                                         .withCentre(trackBounds.getCentre());
            
            // Background for value
            g.setColour(DesignSystem::Colors::getColor(DesignSystem::Colors::BG_DARK).withAlpha(0.9f));
            g.fillRoundedRectangle(valueBounds.expanded(scaled(4), scaled(2)), scaled(2));
            
            // Value text
            g.setColour(DesignSystem::Colors::getColor(DesignSystem::Colors::TEXT_PRIMARY));
            g.setFont(DesignSystem::Typography::getSmallFont());
            
            juce::String valueText;
            if (m_step > 0 && m_step >= 1.0f) {
                valueText = juce::String(static_cast<int>(getValue()));
            } else {
                valueText = juce::String(getValue(), 2);
            }
            
            g.drawText(valueText, valueBounds, juce::Justification::centred);
        }
    }
    
    // Mouse handling
    void mouseDown(const juce::MouseEvent& e) override {
        m_isDragging = true;
        m_dragStartValue = m_normalizedValue;
        updateValueFromMouse(e.position);
    }
    
    void mouseDrag(const juce::MouseEvent& e) override {
        if (e.mods.isShiftDown()) {
            // Fine control with shift
            float delta = m_vertical 
                ? -(e.position.y - e.mouseDownPosition.y) / (getHeight() * 10.0f)
                : (e.position.x - e.mouseDownPosition.x) / (getWidth() * 10.0f);
            
            m_normalizedValue = juce::jlimit(0.0f, 1.0f, m_dragStartValue + delta);
        } else {
            updateValueFromMouse(e.position);
        }
        
        repaint();
        if (onValueChange) {
            onValueChange(getValue());
        }
    }
    
    void mouseUp(const juce::MouseEvent&) override {
        m_isDragging = false;
        repaint();
    }
    
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel) override {
        float delta = wheel.deltaY * 0.05f;
        if (wheel.isReversed) delta = -delta;
        if (!m_vertical) delta = -delta;
        
        m_normalizedValue = juce::jlimit(0.0f, 1.0f, m_normalizedValue + delta);
        
        repaint();
        if (onValueChange) {
            onValueChange(getValue());
        }
    }
    
    // Callback
    std::function<void(float)> onValueChange;
    
private:
    bool m_vertical;
    float m_normalizedValue = 0.5f;
    float m_min = 0.0f;
    float m_max = 1.0f;
    float m_step = 0.0f;
    juce::String m_label;
    juce::Colour m_trackColor = DesignSystem::Colors::getColor(DesignSystem::Colors::ACCENT_BLUE);
    bool m_showValue = false;
    bool m_isDragging = false;
    float m_dragStartValue = 0.0f;
    
    void updateValueFromMouse(juce::Point<float> pos) {
        auto bounds = getLocalBounds().toFloat();
        
        float newValue = m_vertical
            ? 1.0f - juce::jlimit(0.0f, 1.0f, pos.y / bounds.getHeight())
            : juce::jlimit(0.0f, 1.0f, pos.x / bounds.getWidth());
        
        // Apply stepping if needed
        if (m_step > 0) {
            float actualValue = m_min + (newValue * (m_max - m_min));
            actualValue = m_min + m_step * std::round((actualValue - m_min) / m_step);
            newValue = (actualValue - m_min) / (m_max - m_min);
        }
        
        m_normalizedValue = newValue;
    }
};

} // namespace HAM::UI