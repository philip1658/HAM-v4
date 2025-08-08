// SPDX-License-Identifier: MIT
#pragma once

#include <JuceHeader.h>
#include "DesignSystem.h"

namespace HAM::UI {

// ==========================================
// Base Component - Foundation for all UI
// ==========================================
class BaseComponent : public juce::Component {
public:
    BaseComponent() = default;
    virtual ~BaseComponent() = default;
    
    // Scale factor for responsive design
    void setScaleFactor(float scale) {
        m_scaleFactor = juce::jmax(0.5f, juce::jmin(2.0f, scale));
        updateLayout();
        repaint();
    }
    
    float getScaleFactor() const { return m_scaleFactor; }
    
    // Focus management
    void setFocused(bool focused) {
        if (m_isFocused != focused) {
            m_isFocused = focused;
            repaint();
            if (onFocusChanged) onFocusChanged(focused);
        }
    }
    
    bool isFocused() const { return m_isFocused; }
    
    // Enable/disable state
    void setEnabled(bool enabled) {
        juce::Component::setEnabled(enabled);
        repaint();
    }
    
    // Hover state
    void mouseEnter(const juce::MouseEvent&) override {
        m_isHovered = true;
        repaint();
    }
    
    void mouseExit(const juce::MouseEvent&) override {
        m_isHovered = false;
        repaint();
    }
    
    // Callbacks
    std::function<void(bool)> onFocusChanged;
    
protected:
    // Scaling helpers
    float scaled(float value) const {
        return value * m_scaleFactor;
    }
    
    int scaledInt(float value) const {
        return juce::roundToInt(scaled(value));
    }
    
    juce::Rectangle<float> scaledBounds(juce::Rectangle<float> bounds) const {
        return bounds.withSize(scaled(bounds.getWidth()), scaled(bounds.getHeight()));
    }
    
    // Grid-aligned scaling
    int gridScaled(int gridUnits) const {
        return scaledInt(gridUnits * DesignSystem::GRID_UNIT);
    }
    
    // Virtual layout update for subclasses
    virtual void updateLayout() {}
    
    // Common drawing utilities
    void drawBackground(juce::Graphics& g, juce::Colour color) {
        g.fillAll(color);
    }
    
    void drawPanel(juce::Graphics& g, juce::Rectangle<float> bounds, bool raised = false) {
        // Shadow for raised panels
        if (raised) {
            DesignSystem::drawShadow(g, bounds);
        }
        
        // Background
        auto bgColor = raised 
            ? DesignSystem::Colors::getColor(DesignSystem::Colors::BG_RAISED)
            : DesignSystem::Colors::getColor(DesignSystem::Colors::BG_PANEL);
            
        g.setColour(bgColor);
        g.fillRoundedRectangle(bounds, scaled(DesignSystem::Dimensions::CORNER_RADIUS));
        
        // Border
        g.setColour(DesignSystem::Colors::getColor(DesignSystem::Colors::BORDER));
        g.drawRoundedRectangle(bounds, scaled(DesignSystem::Dimensions::CORNER_RADIUS), 
                               scaled(DesignSystem::Dimensions::BORDER_WIDTH));
    }
    
    void drawFocusOutline(juce::Graphics& g, juce::Rectangle<float> bounds) {
        if (m_isFocused) {
            g.setColour(DesignSystem::Colors::getColor(DesignSystem::Colors::BORDER_FOCUS));
            g.drawRoundedRectangle(bounds.expanded(scaled(2)), 
                                  scaled(DesignSystem::Dimensions::CORNER_RADIUS), 
                                  scaled(2));
        }
    }
    
    // Get appropriate text color based on state
    juce::Colour getTextColor() const {
        if (!isEnabled()) {
            return DesignSystem::Colors::getColor(DesignSystem::Colors::TEXT_DIM);
        }
        if (m_isHovered) {
            return DesignSystem::Colors::getColor(DesignSystem::Colors::TEXT_PRIMARY);
        }
        return DesignSystem::Colors::getColor(DesignSystem::Colors::TEXT_MUTED);
    }
    
    // State flags
    float m_scaleFactor = 1.0f;
    bool m_isFocused = false;
    bool m_isHovered = false;
};

// ==========================================
// Animated Component - Base with animation
// ==========================================
class AnimatedComponent : public BaseComponent, private juce::Timer {
public:
    AnimatedComponent() {
        startTimerHz(DesignSystem::Animation::FPS_UI);
    }
    
    ~AnimatedComponent() override {
        stopTimer();
    }
    
protected:
    // Animation state
    struct AnimationState {
        float current = 0.0f;
        float target = 0.0f;
        float velocity = 0.0f;
        float duration = DesignSystem::Animation::DURATION_NORMAL;
        
        bool isAnimating() const {
            return std::abs(current - target) > 0.001f;
        }
        
        void update(float deltaTime) {
            if (!isAnimating()) return;
            
            float t = juce::jmin(1.0f, deltaTime / duration);
            current += (target - current) * DesignSystem::Animation::easeOut(t);
        }
        
        void setTarget(float newTarget, float newDuration = DesignSystem::Animation::DURATION_NORMAL) {
            target = newTarget;
            duration = newDuration;
        }
        
        void snapTo(float value) {
            current = target = value;
            velocity = 0.0f;
        }
    };
    
    // Override to update animations
    virtual void updateAnimations(float deltaTime) = 0;
    
    // Timer callback
    void timerCallback() override {
        float deltaTime = 1.0f / DesignSystem::Animation::FPS_UI;
        updateAnimations(deltaTime);
        
        // Only repaint if something is animating
        if (hasActiveAnimations()) {
            repaint();
        }
    }
    
    // Override to check if animations are active
    virtual bool hasActiveAnimations() const = 0;
    
    // Spring animation helper
    void animateWithSpring(AnimationState& state, float stiffness = 100.0f, float damping = 10.0f) {
        float deltaTime = 1.0f / DesignSystem::Animation::FPS_UI;
        
        // Spring physics
        float force = (state.target - state.current) * stiffness;
        float dampingForce = -state.velocity * damping;
        float acceleration = force + dampingForce;
        
        state.velocity += acceleration * deltaTime;
        state.current += state.velocity * deltaTime;
        
        // Stop when close enough
        if (std::abs(state.current - state.target) < 0.001f && std::abs(state.velocity) < 0.001f) {
            state.snapTo(state.target);
        }
    }
};

} // namespace HAM::UI