// SPDX-License-Identifier: MIT
#pragma once

#include <JuceHeader.h>
#include <array>
#include <functional>

namespace HAM::UI {

// ==========================================
// Design Tokens - Pulse Dark Void Aesthetic
// ==========================================
struct DesignTokens {
    // Grid System
    static constexpr int GRID_UNIT = 8;
    
    // Colors - Dark Void Theme
    struct Colors {
        // Background - Dark Void
        static constexpr uint32_t BG_VOID      = 0xFF000000;  // Pure black
        static constexpr uint32_t BG_DARK      = 0xFF0A0A0A;  // Near black
        static constexpr uint32_t BG_PANEL     = 0xFF1A1A1A;  // Dark panel
        static constexpr uint32_t BG_RAISED    = 0xFF2A2A2A;  // Raised surface
        static constexpr uint32_t BG_RECESSED  = 0xFF151515;  // Recessed surface
        
        // Borders & Lines
        static constexpr uint32_t BORDER       = 0xFF3A3A3A;  // Subtle border
        static constexpr uint32_t HAIRLINE     = 0x20FFFFFF;  // Very subtle
        static constexpr uint32_t GRID_LINE    = 0x10FFFFFF;  // Grid lines
        
        // Text
        static constexpr uint32_t TEXT_PRIMARY = 0xFFE0E0E0;  // Light grey
        static constexpr uint32_t TEXT_MUTED   = 0xFF808080;  // Mid grey
        static constexpr uint32_t TEXT_DIM     = 0xFF505050;  // Dark grey
        
        // Accents - Subtle neon colors
        static constexpr uint32_t ACCENT_BLUE  = 0xFF4080FF;  // Primary accent
        static constexpr uint32_t ACCENT_CYAN  = 0xFF00D4E4;  // Secondary
        static constexpr uint32_t ACCENT_GREEN = 0xFF00E676;  // Success
        static constexpr uint32_t ACCENT_AMBER = 0xFFFFAB00;  // Warning
        static constexpr uint32_t ACCENT_RED   = 0xFFFF1744;  // Error
        
        // Track Colors - Neon palette
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
        
        // Helpers
        static juce::Colour getTrackColor(int index) {
            return juce::Colour(TRACK_COLORS[index % 8]);
        }
    };
    
    // Dimensions
    struct Dimensions {
        static constexpr float CORNER_RADIUS = 3.0f;  // Rectangular look
        static constexpr float SLIDER_TRACK_WIDTH = 22.0f;  // Vertical slider width
        static constexpr float BORDER_WIDTH = 1.0f;
        static constexpr float SHADOW_RADIUS = 8.0f;
    };
};

// ==========================================
// Base Resizable Component
// ==========================================
class ResizableComponent : public juce::Component {
public:
    ResizableComponent() = default;
    virtual ~ResizableComponent() = default;
    
    // Scale factor for responsive design
    void setScaleFactor(float scale) {
        m_scaleFactor = juce::jmax(0.5f, scale);
        repaint();
    }
    
    float getScaleFactor() const { return m_scaleFactor; }
    
    // Helper to scale dimensions
    float scaled(float value) const {
        return value * m_scaleFactor;
    }
    
    int scaledInt(float value) const {
        return juce::roundToInt(scaled(value));
    }
    
    // Grid-aligned scaling
    int gridScaled(int gridUnits) const {
        return scaledInt(gridUnits * DesignTokens::GRID_UNIT);
    }
    
protected:
    float m_scaleFactor = 1.0f;
    
    // Optimized single-layer shadow (67% fewer draw calls)
    void drawMultiLayerShadow(juce::Graphics& g, juce::Rectangle<float> bounds) {
        // Single semi-transparent stroke instead of multiple layers
        g.setColour(juce::Colour(0x60000000));
        g.drawRoundedRectangle(bounds.expanded(scaled(1)), 
                              scaled(DesignTokens::Dimensions::CORNER_RADIUS), 
                              scaled(2.0f)); // Thicker single stroke
    }
};

// ==========================================
// Modern Vertical Slider (No Thumb)
// ==========================================
class ModernSlider : public ResizableComponent {
public:
    ModernSlider(bool vertical = true) : m_vertical(vertical) {
        setInterceptsMouseClicks(true, false);
        // Enable buffering for static background
        setBufferedToImage(true);
    }
    
    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();
        
        // Reserve space for label if vertical
        if (m_vertical && m_label.isNotEmpty()) {
            bounds.removeFromTop(scaled(14)); // Space for label above slider
        }
        
        // Track background
        auto trackBounds = m_vertical 
            ? bounds.withWidth(scaled(DesignTokens::Dimensions::SLIDER_TRACK_WIDTH))
                    .withX((bounds.getWidth() - scaled(DesignTokens::Dimensions::SLIDER_TRACK_WIDTH)) * 0.5f)
            : bounds.withHeight(scaled(DesignTokens::Dimensions::SLIDER_TRACK_WIDTH))
                    .withY((bounds.getHeight() - scaled(DesignTokens::Dimensions::SLIDER_TRACK_WIDTH)) * 0.5f);
        
        // Multi-layer shadow
        drawMultiLayerShadow(g, trackBounds);
        
        // Optimized: Solid color instead of gradient for track background
        g.setColour(juce::Colour(DesignTokens::Colors::BG_RECESSED).withAlpha(0.8f));
        g.fillRoundedRectangle(trackBounds, scaled(DesignTokens::Dimensions::CORNER_RADIUS));
        
        // Track border
        g.setColour(juce::Colour(DesignTokens::Colors::BORDER));
        g.drawRoundedRectangle(trackBounds, scaled(DesignTokens::Dimensions::CORNER_RADIUS), scaled(1.0f));
        
        // Optimized: Solid color with alpha for fill (value indicator)
        float fillProportion = m_value;
        if (fillProportion > 0.01f) {
            auto fillBounds = m_vertical
                ? trackBounds.withTrimmedTop(trackBounds.getHeight() * (1.0f - fillProportion))
                : trackBounds.withTrimmedRight(trackBounds.getWidth() * (1.0f - fillProportion));
            
            // Solid color with transparency instead of gradient
            g.setColour(m_trackColor.withAlpha(0.6f));
            g.fillRoundedRectangle(fillBounds, scaled(DesignTokens::Dimensions::CORNER_RADIUS));
        }
        
        // Line indicator (instead of thumb)
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
        
        // Draw label at the top for vertical sliders
        if (m_label.isNotEmpty()) {
            g.setColour(juce::Colour(DesignTokens::Colors::TEXT_MUTED));
            g.setFont(juce::Font(juce::FontOptions(scaled(9))));
            if (m_vertical) {
                // Draw label at the very top of the component
                auto labelBounds = getLocalBounds().toFloat().withHeight(scaled(14));
                g.drawText(m_label, labelBounds, juce::Justification::centred);
            } else {
                // Keep horizontal slider labels to the left
                g.drawText(m_label, bounds.reduced(scaled(2)), juce::Justification::centredLeft);
            }
        }
    }
    
    void mouseDown(const juce::MouseEvent& e) override {
        updateValue(e.position);
    }
    
    void mouseDrag(const juce::MouseEvent& e) override {
        updateValue(e.position);
    }
    
    // Value and properties
    void setValue(float newValue) {
        m_value = juce::jlimit(0.0f, 1.0f, newValue);
        repaint();
        if (onValueChange) onValueChange(m_value);
    }
    
    float getValue() const { return m_value; }
    
    void setLabel(const juce::String& label) {
        m_label = label;
        repaint();
    }
    
    void setTrackColor(const juce::Colour& color) {
        m_trackColor = color;
        repaint();
    }
    
    // Callback
    std::function<void(float)> onValueChange;
    
private:
    bool m_vertical;
    float m_value = 0.5f;
    juce::String m_label;
    juce::Colour m_trackColor = juce::Colour(DesignTokens::Colors::ACCENT_BLUE);
    
    void updateValue(juce::Point<float> pos) {
        auto bounds = getLocalBounds().toFloat();
        float newValue = m_vertical
            ? 1.0f - (pos.y / bounds.getHeight())
            : pos.x / bounds.getWidth();
        setValue(newValue);
    }
};

// ==========================================
// Panel Container
// ==========================================
class Panel : public ResizableComponent {
public:
    enum class Style {
        Flat,
        Raised,
        Recessed,
        Glass
    };
    
    Panel(Style style = Style::Raised) : m_style(style) {}
    
    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat().reduced(scaled(2));
        
        // Multi-layer shadow for depth
        if (m_style == Style::Raised) {
            drawMultiLayerShadow(g, bounds);
        }
        
        // Background
        juce::Colour bgColor;
        switch (m_style) {
            case Style::Flat:     bgColor = juce::Colour(DesignTokens::Colors::BG_PANEL); break;
            case Style::Raised:   bgColor = juce::Colour(DesignTokens::Colors::BG_RAISED); break;
            case Style::Recessed: bgColor = juce::Colour(DesignTokens::Colors::BG_RECESSED); break;
            case Style::Glass:    bgColor = juce::Colour(DesignTokens::Colors::BG_DARK).withAlpha(0.8f); break;
        }
        
        if (m_style == Style::Glass) {
            // Glass effect with gradient
            juce::ColourGradient gradient(
                bgColor.withAlpha(0.9f), bounds.getTopLeft(),
                bgColor.withAlpha(0.6f), bounds.getBottomRight(),
                false
            );
            g.setGradientFill(gradient);
        } else {
            g.setColour(bgColor);
        }
        
        g.fillRoundedRectangle(bounds, scaled(DesignTokens::Dimensions::CORNER_RADIUS));
        
        // Border
        g.setColour(juce::Colour(DesignTokens::Colors::BORDER));
        g.drawRoundedRectangle(bounds, scaled(DesignTokens::Dimensions::CORNER_RADIUS), scaled(1));
    }
    
    void setStyle(Style style) {
        m_style = style;
        repaint();
    }
    
private:
    Style m_style;
};

// ==========================================
// Modern Button
// ==========================================
class ModernButton : public ResizableComponent {
public:
    enum class Style {
        Solid,
        Outline,
        Ghost,
        Gradient
    };
    
    ModernButton(const juce::String& text, Style style = Style::Solid) 
        : m_text(text), m_style(style) {
        setInterceptsMouseClicks(true, false);
    }
    
    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat().reduced(scaled(2));
        
        // Button state colors
        auto bgColor = m_isDown ? m_color.darker(0.2f) : (m_isHovered ? m_color.brighter(0.1f) : m_color);
        
        // Only draw outline - no fill for any style
        // Border for all styles
        g.setColour(bgColor);
        g.drawRoundedRectangle(bounds, scaled(DesignTokens::Dimensions::CORNER_RADIUS), scaled(1.5f));
        
        // Text uses the button color
        g.setColour(bgColor);
        g.setFont(juce::Font(juce::FontOptions(scaled(11))).withStyle(juce::Font::bold));
        g.drawText(m_text, bounds, juce::Justification::centred);
    }
    
    void mouseEnter(const juce::MouseEvent&) override {
        m_isHovered = true;
        repaint();
    }
    
    void mouseExit(const juce::MouseEvent&) override {
        m_isHovered = false;
        repaint();
    }
    
    void mouseDown(const juce::MouseEvent&) override {
        m_isDown = true;
        repaint();
    }
    
    void mouseUp(const juce::MouseEvent&) override {
        if (m_isDown && onClick) onClick();
        m_isDown = false;
        repaint();
    }
    
    void setText(const juce::String& text) {
        m_text = text;
        repaint();
    }
    
    void setColor(const juce::Colour& color) {
        m_color = color;
        repaint();
    }
    
    void setButtonStyle(Style style) {
        m_style = style;
        repaint();
    }
    
    // Callback
    std::function<void()> onClick;
    
private:
    juce::String m_text;
    Style m_style;
    juce::Colour m_color = juce::Colour(DesignTokens::Colors::ACCENT_BLUE);
    bool m_isHovered = false;
    bool m_isDown = false;
};

// ==========================================
// Modern Toggle
// ==========================================
class ModernToggle : public ResizableComponent {
public:
    ModernToggle() {
        setInterceptsMouseClicks(true, false);
    }
    
    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();
        auto toggleBounds = bounds.withWidth(scaled(44)).withHeight(scaled(24))
                                  .withCentre(bounds.getCentre());
        
        // Track
        auto trackColor = m_checked 
            ? juce::Colour(DesignTokens::Colors::ACCENT_GREEN).withAlpha(0.3f)
            : juce::Colour(DesignTokens::Colors::BG_RECESSED);
        g.setColour(trackColor);
        g.fillRoundedRectangle(toggleBounds, toggleBounds.getHeight() * 0.5f);
        
        // Border
        g.setColour(juce::Colour(DesignTokens::Colors::BORDER));
        g.drawRoundedRectangle(toggleBounds, toggleBounds.getHeight() * 0.5f, scaled(1));
        
        // Thumb with glow when checked
        float thumbX = m_checked 
            ? toggleBounds.getRight() - toggleBounds.getHeight() * 0.7f
            : toggleBounds.getX() + toggleBounds.getHeight() * 0.3f;
        
        auto thumbBounds = juce::Rectangle<float>(scaled(18), scaled(18))
                                .withCentre({thumbX, toggleBounds.getCentreY()});
        
        if (m_checked) {
            // Glow effect
            g.setColour(juce::Colour(DesignTokens::Colors::ACCENT_GREEN).withAlpha(0.3f));
            g.fillEllipse(thumbBounds.expanded(scaled(4)));
        }
        
        g.setColour(m_checked 
                   ? juce::Colour(DesignTokens::Colors::ACCENT_GREEN)
                   : juce::Colour(DesignTokens::Colors::TEXT_MUTED));
        g.fillEllipse(thumbBounds);
    }
    
    void mouseUp(const juce::MouseEvent&) override {
        m_checked = !m_checked;
        repaint();
        if (onToggle) onToggle(m_checked);
    }
    
    void setChecked(bool checked) {
        m_checked = checked;
        repaint();
    }
    
    bool isChecked() const { return m_checked; }
    
    // Callback
    std::function<void(bool)> onToggle;
    
private:
    bool m_checked = false;
};

// ==========================================
// Stage Card (2x2 Grid)
// ==========================================
class StageCard : public Panel {
public:
    StageCard() : Panel(Style::Raised) {
        // Create 2x2 grid of sliders
        m_pitchSlider = std::make_unique<ModernSlider>(true);
        m_pulseSlider = std::make_unique<ModernSlider>(true);
        m_velocitySlider = std::make_unique<ModernSlider>(true);
        m_gateSlider = std::make_unique<ModernSlider>(true);
        
        m_pitchSlider->setLabel("PITCH");
        m_pulseSlider->setLabel("PULSE");
        m_velocitySlider->setLabel("VEL");
        m_gateSlider->setLabel("GATE");
        
        // Set different colors for each slider (as originally intended)
        m_pitchSlider->setTrackColor(juce::Colour(DesignTokens::Colors::TRACK_COLORS[0]));
        m_pulseSlider->setTrackColor(juce::Colour(DesignTokens::Colors::TRACK_COLORS[1]));
        m_velocitySlider->setTrackColor(juce::Colour(DesignTokens::Colors::TRACK_COLORS[2]));
        m_gateSlider->setTrackColor(juce::Colour(DesignTokens::Colors::TRACK_COLORS[3]));
        
        // Create Stage Editor button - don't set color yet
        m_stageEditorButton = std::make_unique<ModernButton>("EDIT", ModernButton::Style::Solid);
        m_stageEditorButton->onClick = [this]() {
            DBG("Stage Editor button clicked for stage " << m_stageNumber);
            if (onStageEditorClicked) onStageEditorClicked(m_stageNumber);
        };
        
        addAndMakeVisible(m_pitchSlider.get());
        addAndMakeVisible(m_pulseSlider.get());
        addAndMakeVisible(m_velocitySlider.get());
        addAndMakeVisible(m_gateSlider.get());
        addAndMakeVisible(m_stageEditorButton.get());
        
        // Don't set a fixed size - let parent control our bounds
        DBG("StageCard created");
    }
    
    void resized() override {
        const int padding = 8;
        auto bounds = getLocalBounds().reduced(padding);
        
        // Reserve small space for active indicator (15px)
        bounds.removeFromTop(15);
        
        // Reserve space for Stage Editor button at bottom (30px)
        auto buttonArea = bounds.removeFromBottom(30);
        
        // Calculate 2x2 grid for sliders with equal sizes
        // The remaining area after header and button
        auto sliderGridArea = bounds.reduced(4); // Small inner padding
        
        const int gridSpacing = 6; // Space between sliders in grid
        int sliderWidth = (sliderGridArea.getWidth() - gridSpacing) / 2;
        int sliderHeight = (sliderGridArea.getHeight() - gridSpacing) / 2;
        
        // Ensure sliders are truly equal sized
        // Top row: PITCH | PULSE
        m_pitchSlider->setBounds(
            sliderGridArea.getX(), 
            sliderGridArea.getY(), 
            sliderWidth, 
            sliderHeight
        );
        
        m_pulseSlider->setBounds(
            sliderGridArea.getX() + sliderWidth + gridSpacing,
            sliderGridArea.getY(),
            sliderWidth,
            sliderHeight
        );
        
        // Bottom row: VEL | GATE  
        m_velocitySlider->setBounds(
            sliderGridArea.getX(),
            sliderGridArea.getY() + sliderHeight + gridSpacing,
            sliderWidth,
            sliderHeight
        );
        
        m_gateSlider->setBounds(
            sliderGridArea.getX() + sliderWidth + gridSpacing,
            sliderGridArea.getY() + sliderHeight + gridSpacing,
            sliderWidth,
            sliderHeight
        );
        
        // Stage Editor button centered at bottom
        int buttonWidth = juce::jmin(100, buttonArea.getWidth() - 20);
        m_stageEditorButton->setBounds(
            buttonArea.getCentreX() - buttonWidth/2,
            buttonArea.getY() + 2,
            buttonWidth,
            buttonArea.getHeight() - 4
        );
    }
    
    void paint(juce::Graphics& g) override {
        Panel::paint(g);
        
        // Active indicator LED at top center (no stage number)
        if (m_isActive) {
            const int padding = 10;
            auto bounds = getLocalBounds().reduced(padding);
            auto headerArea = bounds.removeFromTop(20);
            
            auto ledBounds = headerArea.withWidth(12).withHeight(12)
                                       .withCentre(headerArea.getCentre()).toFloat();
            // Glow effect
            g.setColour(juce::Colour(DesignTokens::Colors::ACCENT_GREEN).withAlpha(0.3f));
            g.fillEllipse(ledBounds.expanded(3));
            // LED itself
            g.setColour(juce::Colour(DesignTokens::Colors::ACCENT_GREEN));
            g.fillEllipse(ledBounds);
        }
    }
    
    void setStageNumber(int number) {
        m_stageNumber = number;
        repaint();
    }
    
    void setActive(bool active) {
        m_isActive = active;
        repaint();
    }
    
    void setTrackColor(const juce::Colour& color) {
        m_trackColor = color;
        
        // Update EDIT button to use track color
        if (m_stageEditorButton) {
            m_stageEditorButton->setColor(m_trackColor);
        }
        repaint();
    }
    
    // Get slider references for external control
    ModernSlider* getPitchSlider() { return m_pitchSlider.get(); }
    ModernSlider* getPulseSlider() { return m_pulseSlider.get(); }
    ModernSlider* getVelocitySlider() { return m_velocitySlider.get(); }
    ModernSlider* getGateSlider() { return m_gateSlider.get(); }
    
    // Callback for Stage Editor button
    std::function<void(int)> onStageEditorClicked;
    
private:
    std::unique_ptr<ModernSlider> m_pitchSlider;
    std::unique_ptr<ModernSlider> m_pulseSlider;
    std::unique_ptr<ModernSlider> m_velocitySlider;
    std::unique_ptr<ModernSlider> m_gateSlider;
    std::unique_ptr<ModernButton> m_stageEditorButton;
    int m_stageNumber = 1;
    bool m_isActive = false;
    juce::Colour m_trackColor = DesignTokens::Colors::getTrackColor(0);  // Default to track 0 color
};

// ==========================================
// Grid Container
// ==========================================
class GridContainer : public ResizableComponent {
public:
    GridContainer(int columns = 2, int rows = 2) 
        : m_columns(columns), m_rows(rows) {}
    
    void paint(juce::Graphics& g) override {
        // Optional grid lines for debugging
        if (m_showGrid) {
            g.setColour(juce::Colour(DesignTokens::Colors::GRID_LINE));
            
            auto bounds = getLocalBounds().toFloat();
            float cellWidth = bounds.getWidth() / m_columns;
            float cellHeight = bounds.getHeight() / m_rows;
            
            // Vertical lines
            for (int i = 1; i < m_columns; ++i) {
                float x = i * cellWidth;
                g.drawLine(x, 0, x, bounds.getHeight(), scaled(0.5f));
            }
            
            // Horizontal lines
            for (int i = 1; i < m_rows; ++i) {
                float y = i * cellHeight;
                g.drawLine(0, y, bounds.getWidth(), y, scaled(0.5f));
            }
        }
    }
    
    void resized() override {
        auto bounds = getLocalBounds();
        float cellWidth = bounds.getWidth() / (float)m_columns;
        float cellHeight = bounds.getHeight() / (float)m_rows;
        
        for (auto& item : m_items) {
            if (item.component) {
                int x = item.column * cellWidth + gridScaled(item.padding);
                int y = item.row * cellHeight + gridScaled(item.padding);
                int w = item.colSpan * cellWidth - gridScaled(item.padding * 2);
                int h = item.rowSpan * cellHeight - gridScaled(item.padding * 2);
                
                item.component->setBounds(x, y, w, h);
            }
        }
    }
    
    void addItem(Component* component, int col, int row, int colSpan = 1, int rowSpan = 1, int padding = 1) {
        if (component) {
            addAndMakeVisible(component);
            m_items.push_back({component, col, row, colSpan, rowSpan, padding});
            resized();
        }
    }
    
    void setShowGrid(bool show) {
        m_showGrid = show;
        repaint();
    }
    
private:
    struct GridItem {
        Component* component;
        int column, row;
        int colSpan, rowSpan;
        int padding;
    };
    
    int m_columns, m_rows;
    std::vector<GridItem> m_items;
    bool m_showGrid = false;
};

} // namespace HAM::UI