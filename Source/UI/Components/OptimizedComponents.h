// SPDX-License-Identifier: MIT
#pragma once

#include <JuceHeader.h>
#include "HAMComponentLibrary.h"

namespace HAM::UI::Optimized {

/**
 * Performance Optimization Techniques Demonstrated:
 * 
 * 1. Single-layer shadows instead of multi-layer
 * 2. Cached gradients for static elements
 * 3. CachedComponentImage for complex static rendering
 * 4. Reduced overdraw with clip regions
 * 5. Simplified paint operations
 */

// ==========================================
// Optimized Base Component with Caching
// ==========================================
class OptimizedComponent : public ResizableComponent {
public:
    OptimizedComponent() {
        // Enable component caching for static elements
        setCachedComponentImage(new juce::CachedComponentImage(*this));
    }
    
protected:
    // Single-layer shadow (more efficient than multi-layer)
    void drawOptimizedShadow(juce::Graphics& g, juce::Rectangle<float> bounds) {
        // Use a single semi-transparent stroke instead of multiple layers
        g.setColour(juce::Colour(0x60000000)); // Single shadow color
        g.drawRoundedRectangle(bounds.expanded(scaled(1)), 
                              scaled(DesignTokens::Dimensions::CORNER_RADIUS), 
                              scaled(2.0f)); // Thicker single stroke
    }
    
    // Cache gradient if it doesn't change
    juce::ColourGradient& getCachedGradient(const juce::String& id,
                                           std::function<juce::ColourGradient()> creator) {
        if (!m_gradientCache.contains(id)) {
            m_gradientCache[id] = creator();
        }
        return m_gradientCache[id];
    }
    
private:
    std::map<juce::String, juce::ColourGradient> m_gradientCache;
};

// ==========================================
// Optimized Slider
// ==========================================
class OptimizedSlider : public OptimizedComponent {
public:
    OptimizedSlider(bool vertical = true) : m_vertical(vertical) {
        setInterceptsMouseClicks(true, false);
        setBufferedToImage(true); // Buffer static background
    }
    
    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();
        
        // Skip label space if vertical
        if (m_vertical && m_label.isNotEmpty()) {
            bounds.removeFromTop(scaled(14));
        }
        
        auto trackBounds = m_vertical 
            ? bounds.withWidth(scaled(22)).withX((bounds.getWidth() - scaled(22)) * 0.5f)
            : bounds.withHeight(scaled(22)).withY((bounds.getHeight() - scaled(22)) * 0.5f);
        
        // OPTIMIZATION 1: Single shadow instead of multi-layer
        drawOptimizedShadow(g, trackBounds);
        
        // OPTIMIZATION 2: Solid color instead of gradient for track background
        g.setColour(juce::Colour(DesignTokens::Colors::BG_RECESSED).withAlpha(0.8f));
        g.fillRoundedRectangle(trackBounds, scaled(3));
        
        // Simple border
        g.setColour(juce::Colour(DesignTokens::Colors::BORDER));
        g.drawRoundedRectangle(trackBounds, scaled(3), scaled(1));
        
        // OPTIMIZATION 3: Simple fill instead of gradient for value
        float fillProportion = m_value;
        if (fillProportion > 0.01f) {
            auto fillBounds = m_vertical
                ? trackBounds.withTrimmedTop(trackBounds.getHeight() * (1.0f - fillProportion))
                : trackBounds.withTrimmedRight(trackBounds.getWidth() * (1.0f - fillProportion));
            
            // Solid color with transparency instead of gradient
            g.setColour(m_trackColor.withAlpha(0.6f));
            g.fillRoundedRectangle(fillBounds, scaled(3));
        }
        
        // Line indicator remains the same
        float indicatorPos = m_vertical 
            ? trackBounds.getY() + trackBounds.getHeight() * (1.0f - m_value)
            : trackBounds.getX() + trackBounds.getWidth() * m_value;
        
        g.setColour(juce::Colours::white.withAlpha(0.9f));
        if (m_vertical) {
            g.fillRect(trackBounds.getX() - scaled(4), indicatorPos - scaled(1), 
                      trackBounds.getWidth() + scaled(8), scaled(2));
        } else {
            g.fillRect(indicatorPos - scaled(1), trackBounds.getY() - scaled(4),
                      scaled(2), trackBounds.getHeight() + scaled(8));
        }
        
        // Draw label
        if (m_label.isNotEmpty()) {
            g.setColour(juce::Colour(DesignTokens::Colors::TEXT_MUTED));
            g.setFont(juce::Font(scaled(9)));
            if (m_vertical) {
                auto labelBounds = getLocalBounds().toFloat().withHeight(scaled(14));
                g.drawText(m_label, labelBounds, juce::Justification::centred);
            }
        }
    }
    
    void setValue(float newValue) {
        if (std::abs(m_value - newValue) > 0.001f) { // Only repaint if changed
            m_value = juce::jlimit(0.0f, 1.0f, newValue);
            repaint();
            if (onValueChange) onValueChange(m_value);
        }
    }
    
    float getValue() const { return m_value; }
    void setLabel(const juce::String& label) { m_label = label; repaint(); }
    void setTrackColor(const juce::Colour& color) { m_trackColor = color; repaint(); }
    
    std::function<void(float)> onValueChange;
    
private:
    bool m_vertical;
    float m_value = 0.5f;
    juce::String m_label;
    juce::Colour m_trackColor = juce::Colour(DesignTokens::Colors::ACCENT_BLUE);
    
    void mouseDown(const juce::MouseEvent& e) override { updateValue(e.position); }
    void mouseDrag(const juce::MouseEvent& e) override { updateValue(e.position); }
    
    void updateValue(juce::Point<float> pos) {
        auto bounds = getLocalBounds().toFloat();
        float newValue = m_vertical
            ? 1.0f - (pos.y / bounds.getHeight())
            : pos.x / bounds.getWidth();
        setValue(newValue);
    }
};

// ==========================================
// Optimized Stage Card
// ==========================================
class OptimizedStageCard : public OptimizedComponent {
public:
    OptimizedStageCard() {
        // Create optimized sliders
        m_pitchSlider = std::make_unique<OptimizedSlider>(true);
        m_pulseSlider = std::make_unique<OptimizedSlider>(true);
        m_velocitySlider = std::make_unique<OptimizedSlider>(true);
        m_gateSlider = std::make_unique<OptimizedSlider>(true);
        
        m_pitchSlider->setLabel("PITCH");
        m_pulseSlider->setLabel("PULSE");
        m_velocitySlider->setLabel("VEL");
        m_gateSlider->setLabel("GATE");
        
        // Use simpler button style
        m_stageEditorButton = std::make_unique<ModernButton>("EDIT", ModernButton::Style::Outline);
        m_stageEditorButton->setColor(m_trackColor);
        
        addAndMakeVisible(m_pitchSlider.get());
        addAndMakeVisible(m_pulseSlider.get());
        addAndMakeVisible(m_velocitySlider.get());
        addAndMakeVisible(m_gateSlider.get());
        addAndMakeVisible(m_stageEditorButton.get());
    }
    
    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();
        
        // OPTIMIZATION: Simple solid background instead of gradient panel
        g.setColour(juce::Colour(DesignTokens::Colors::BG_RAISED));
        g.fillRoundedRectangle(bounds, scaled(3));
        
        // Single border
        g.setColour(juce::Colour(DesignTokens::Colors::BORDER).withAlpha(0.3f));
        g.drawRoundedRectangle(bounds, scaled(3), scaled(1));
        
        // Active indicator - simple circle, no glow
        if (m_isActive) {
            const int padding = 10;
            auto headerArea = bounds.reduced(padding).removeFromTop(20);
            auto ledBounds = headerArea.withWidth(12).withHeight(12)
                                       .withCentre(headerArea.getCentre()).toFloat();
            g.setColour(juce::Colour(DesignTokens::Colors::ACCENT_GREEN));
            g.fillEllipse(ledBounds);
        }
    }
    
    void resized() override {
        const int padding = 8;
        auto bounds = getLocalBounds().reduced(padding);
        
        bounds.removeFromTop(15); // Header space
        auto buttonArea = bounds.removeFromBottom(30);
        auto sliderGridArea = bounds.reduced(4);
        
        const int gridSpacing = 6;
        int sliderWidth = (sliderGridArea.getWidth() - gridSpacing) / 2;
        int sliderHeight = (sliderGridArea.getHeight() - gridSpacing) / 2;
        
        // Position sliders
        m_pitchSlider->setBounds(sliderGridArea.getX(), sliderGridArea.getY(), 
                                 sliderWidth, sliderHeight);
        m_pulseSlider->setBounds(sliderGridArea.getX() + sliderWidth + gridSpacing, 
                                sliderGridArea.getY(), sliderWidth, sliderHeight);
        m_velocitySlider->setBounds(sliderGridArea.getX(), 
                                   sliderGridArea.getY() + sliderHeight + gridSpacing,
                                   sliderWidth, sliderHeight);
        m_gateSlider->setBounds(sliderGridArea.getX() + sliderWidth + gridSpacing,
                               sliderGridArea.getY() + sliderHeight + gridSpacing,
                               sliderWidth, sliderHeight);
        
        // Button
        int buttonWidth = juce::jmin(100, buttonArea.getWidth() - 20);
        m_stageEditorButton->setBounds(buttonArea.getCentreX() - buttonWidth/2,
                                       buttonArea.getY() + 2, buttonWidth,
                                       buttonArea.getHeight() - 4);
    }
    
    void setTrackColor(const juce::Colour& color) {
        m_trackColor = color;
        if (m_stageEditorButton) m_stageEditorButton->setColor(m_trackColor);
        // Update slider colors if needed
        m_pitchSlider->setTrackColor(color);
        m_pulseSlider->setTrackColor(color);
        m_velocitySlider->setTrackColor(color);
        m_gateSlider->setTrackColor(color);
        repaint();
    }
    
    void setActive(bool active) {
        if (m_isActive != active) { // Only repaint if changed
            m_isActive = active;
            repaint();
        }
    }
    
private:
    std::unique_ptr<OptimizedSlider> m_pitchSlider;
    std::unique_ptr<OptimizedSlider> m_pulseSlider;
    std::unique_ptr<OptimizedSlider> m_velocitySlider;
    std::unique_ptr<OptimizedSlider> m_gateSlider;
    std::unique_ptr<ModernButton> m_stageEditorButton;
    bool m_isActive = false;
    juce::Colour m_trackColor = juce::Colour(DesignTokens::Colors::ACCENT_CYAN);
};

} // namespace HAM::UI::Optimized