// SPDX-License-Identifier: MIT
#pragma once

#include <JuceHeader.h>
#include "../Core/BaseComponent.h"
#include "../Core/DesignSystem.h"
#include "../../UI/Components/HAMComponentLibrary.h"
#include <memory>
#include <functional>
#include <array>

namespace HAM::UI {

// ==========================================
// StageGrid - Container for 8 Stage Cards
// ==========================================
class StageGrid : public BaseComponent {
public:
    StageGrid() {
        // Create 8 stage cards using HAM component library
        for (int i = 0; i < 8; ++i) {
            auto card = std::make_unique<StageCard>();
            
            // Track colors are already set in the StageCard sliders
            
            addAndMakeVisible(card.get());
            m_stageCards[i] = std::move(card);
        }
        
        // Set default size
        setSize(1200, 460);
    }
    
    ~StageGrid() override = default;
    
    // Paint override
    void paint(juce::Graphics& g) override {
        // Pure black background (dark void aesthetic)
        g.fillAll(juce::Colours::black);
        
        // Optional grid lines
        if (m_showGridLines) {
            g.setColour(juce::Colours::white.withAlpha(0.05f));
            for (int i = 1; i < 8; ++i) {
                int x = i * (getWidth() / 8);
                g.drawVerticalLine(x, 0, getHeight());
            }
        }
    }
    
    // Layout override
    void resized() override {
        // Calculate card dimensions with padding
        const int padding = scaledInt(10);
        const int cardWidth = (getWidth() - (padding * 9)) / 8;
        const int cardHeight = getHeight() - (padding * 2);
        
        // Position each card
        for (int i = 0; i < 8; ++i) {
            if (m_stageCards[i]) {
                int x = padding + (i * (cardWidth + padding));
                int y = padding;
                m_stageCards[i]->setBounds(x, y, cardWidth, cardHeight);
            }
        }
    }
    
    // Public interface
    void setActiveStage(int stage) {
        // For now, just track the active stage
        m_activeStage = stage;
        repaint();
    }
    
    void setShowGridLines(bool show) {
        m_showGridLines = show;
        repaint();
    }
    
    // Get access to stage cards for external control
    StageCard* getStageCard(int index) {
        if (index >= 0 && index < 8) {
            return m_stageCards[index].get();
        }
        return nullptr;
    }
    
    // Callbacks
    std::function<void(int track, int stage, const juce::String& param, float value)> onStageParameterChanged;
    std::function<void(int stage)> onStageSelected;
    std::function<void(int stage)> onHAMEditorRequested;
    
private:
    std::array<std::unique_ptr<StageCard>, 8> m_stageCards;
    int m_activeStage = 0;
    bool m_showGridLines = false;
};

} // namespace HAM::UI