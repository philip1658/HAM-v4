// SPDX-License-Identifier: MIT
#pragma once

#include <JuceHeader.h>
#include <array>

namespace HAM::UI {

// ==========================================
// Design System - Pulse Dark Void Aesthetic
// ==========================================
struct DesignSystem {
    
    // Grid System - 8px base unit
    static constexpr int GRID_UNIT = 8;
    
    // Component Dimensions
    struct Dimensions {
        // Stage Card
        static constexpr int STAGE_CARD_WIDTH = 140;
        static constexpr int STAGE_CARD_HEIGHT = 420;
        
        // Sliders
        static constexpr float SLIDER_TRACK_WIDTH = 22.0f;
        static constexpr float SLIDER_INDICATOR_HEIGHT = 2.0f;
        
        // Corners & Borders
        static constexpr float CORNER_RADIUS = 3.0f;
        static constexpr float BORDER_WIDTH = 1.0f;
        
        // Transport Bar
        static constexpr int TRANSPORT_HEIGHT = 60;
        
        // Track Sidebar
        static constexpr int TRACK_SIDEBAR_WIDTH = 250;
        
        // HAM Editor Panel
        static constexpr int HAM_EDITOR_HEIGHT = 200;
        
        // Shadows
        static constexpr float SHADOW_RADIUS = 8.0f;
        static constexpr float SHADOW_OPACITY = 0.4f;
    };
    
    // Colors - Dark Void Theme
    struct Colors {
        // Backgrounds - Dark to Light
        static constexpr uint32_t BG_VOID      = 0xFF000000;  // Pure black
        static constexpr uint32_t BG_DARK      = 0xFF0A0A0A;  // Near black
        static constexpr uint32_t BG_PANEL     = 0xFF1A1A1A;  // Panel bg
        static constexpr uint32_t BG_RAISED    = 0xFF2A2A2A;  // Raised
        static constexpr uint32_t BG_RECESSED  = 0xFF151515;  // Recessed
        static constexpr uint32_t BG_HOVER     = 0xFF3A3A3A;  // Hover state
        
        // Borders & Lines
        static constexpr uint32_t BORDER       = 0xFF3A3A3A;  // Subtle border
        static constexpr uint32_t BORDER_FOCUS = 0xFF00FF88;  // Focus state
        static constexpr uint32_t HAIRLINE     = 0x20FFFFFF;  // Very subtle
        static constexpr uint32_t GRID_LINE    = 0x10FFFFFF;  // Grid lines
        
        // Text
        static constexpr uint32_t TEXT_PRIMARY = 0xFFE0E0E0;  // Light grey
        static constexpr uint32_t TEXT_MUTED   = 0xFF808080;  // Mid grey
        static constexpr uint32_t TEXT_DIM     = 0xFF505050;  // Dark grey
        static constexpr uint32_t TEXT_ACCENT  = 0xFF00FF88;  // Mint accent
        
        // Primary Accent - Mint
        static constexpr uint32_t ACCENT_PRIMARY = 0xFF00FF88;
        static constexpr uint32_t ACCENT_PRIMARY_DIM = 0xFF00AA55;
        static constexpr uint32_t ACCENT_PRIMARY_BRIGHT = 0xFF00FFAA;
        
        // System Colors
        static constexpr uint32_t ACCENT_BLUE  = 0xFF4080FF;  // Info
        static constexpr uint32_t ACCENT_GREEN = 0xFF00E676;  // Success
        static constexpr uint32_t ACCENT_AMBER = 0xFFFFAB00;  // Warning
        static constexpr uint32_t ACCENT_RED   = 0xFFFF1744;  // Error
        
        // Track Colors - Neon palette (8 tracks)
        static constexpr std::array<uint32_t, 8> TRACK_COLORS = {
            0xFF00FFD4,  // Mint
            0xFF00D4FF,  // Cyan
            0xFFFF00FF,  // Magenta
            0xFFFF8800,  // Orange
            0xFF00FF88,  // Green
            0xFF8800FF,  // Purple
            0xFFFFFF00,  // Yellow
            0xFFFF0088   // Pink
        };
        
        // Helper functions
        static juce::Colour getColor(uint32_t color) {
            return juce::Colour(color);
        }
        
        static juce::Colour getTrackColor(int trackIndex) {
            return juce::Colour(TRACK_COLORS[trackIndex % 8]);
        }
        
        static juce::Colour withAlpha(uint32_t color, float alpha) {
            return juce::Colour(color).withAlpha(alpha);
        }
    };
    
    // Typography
    struct Typography {
        static constexpr float SIZE_TINY = 10.0f;
        static constexpr float SIZE_SMALL = 12.0f;
        static constexpr float SIZE_NORMAL = 14.0f;
        static constexpr float SIZE_LARGE = 16.0f;
        static constexpr float SIZE_TITLE = 20.0f;
        static constexpr float SIZE_HEADER = 24.0f;
        
        static juce::Font getTinyFont() { return juce::Font(juce::FontOptions(SIZE_TINY)); }
        static juce::Font getSmallFont() { return juce::Font(juce::FontOptions(SIZE_SMALL)); }
        static juce::Font getNormalFont() { return juce::Font(juce::FontOptions(SIZE_NORMAL)); }
        static juce::Font getLargeFont() { return juce::Font(juce::FontOptions(SIZE_LARGE)); }
        static juce::Font getTitleFont() { return juce::Font(juce::FontOptions(SIZE_TITLE)).withStyle(juce::Font::bold); }
        static juce::Font getHeaderFont() { return juce::Font(juce::FontOptions(SIZE_HEADER)).withStyle(juce::Font::bold); }
    };
    
    // Animation
    struct Animation {
        static constexpr float DURATION_INSTANT = 0.0f;
        static constexpr float DURATION_FAST = 0.15f;
        static constexpr float DURATION_NORMAL = 0.3f;
        static constexpr float DURATION_SLOW = 0.6f;
        
        static constexpr int FPS_UI = 30;  // UI animation frame rate
        static constexpr int FPS_ACTIVITY = 60;  // Activity indicators
        
        // Easing functions
        static float easeOut(float t) {
            return 1.0f - std::pow(1.0f - t, 3.0f);
        }
        
        static float easeInOut(float t) {
            return t < 0.5f 
                ? 4.0f * t * t * t 
                : 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) / 2.0f;
        }
        
        static float springDamped(float t, float frequency = 2.0f, float damping = 0.3f) {
            return 1.0f - std::exp(-damping * t) * std::cos(frequency * juce::MathConstants<float>::twoPi * t);
        }
    };
    
    // Layout
    struct Layout {
        static constexpr int MIN_WINDOW_WIDTH = 1024;
        static constexpr int MIN_WINDOW_HEIGHT = 768;
        static constexpr int DEFAULT_WINDOW_WIDTH = 1440;
        static constexpr int DEFAULT_WINDOW_HEIGHT = 900;
        
        // Spacing based on grid units
        static constexpr int SPACING_TINY = GRID_UNIT / 2;    // 4px
        static constexpr int SPACING_SMALL = GRID_UNIT;       // 8px
        static constexpr int SPACING_MEDIUM = GRID_UNIT * 2;  // 16px
        static constexpr int SPACING_LARGE = GRID_UNIT * 3;   // 24px
        static constexpr int SPACING_HUGE = GRID_UNIT * 4;    // 32px
    };
    
    // Shadow Utilities
    static void drawShadow(juce::Graphics& g, juce::Rectangle<float> bounds, 
                          float radius = Dimensions::SHADOW_RADIUS,
                          float opacity = Dimensions::SHADOW_OPACITY) {
        // Multi-layer shadow for depth
        for (int i = 3; i > 0; --i) {
            float layerOpacity = opacity * (0.3f / i);
            float expansion = radius * i * 0.3f;
            g.setColour(juce::Colours::black.withAlpha(layerOpacity));
            g.fillRoundedRectangle(bounds.expanded(expansion), Dimensions::CORNER_RADIUS + expansion);
        }
    }
    
    // Gradient Utilities
    static juce::ColourGradient createVerticalGradient(juce::Rectangle<float> bounds,
                                                       juce::Colour topColor,
                                                       juce::Colour bottomColor) {
        return juce::ColourGradient(topColor, bounds.getTopLeft(),
                                   bottomColor, bounds.getBottomLeft(), false);
    }
    
    static juce::ColourGradient createRadialGradient(juce::Point<float> center,
                                                     float radius,
                                                     juce::Colour innerColor,
                                                     juce::Colour outerColor) {
        return juce::ColourGradient(innerColor, center,
                                   outerColor, center.translated(radius, 0), true);
    }
};

} // namespace HAM::UI