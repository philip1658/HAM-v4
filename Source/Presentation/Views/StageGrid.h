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
            card->setStageNumber(i + 1);
            
            // Set track color (using track 0 color for now - will be updated when multi-track is implemented)
            card->setTrackColor(DesignTokens::Colors::getTrackColor(0));
            
            // Connect Stage Editor button callback
            card->onStageEditorClicked = [this, i](int stageNum) {
                DBG("Stage Editor requested for stage " << stageNum);
                if (onHAMEditorRequested) onHAMEditorRequested(stageNum);
            };
            
            addAndMakeVisible(card.get());
            m_stageCards[i] = std::move(card);
        }
        
        // Don't set a fixed size - parent will control our bounds
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
        // Get the actual available space
        auto bounds = getLocalBounds();
        
        // Calculate optimized card dimensions
        const int numCards = 8;
        const int horizontalPadding = 1;  // 1px spacing between cards for clean separation
        
        // Don't reduce bounds - we want cards to use full width with only 1px gaps
        // bounds = bounds.reduced(horizontalPadding/2, 0);  // Removed to maximize card width
        
        // Line 5 absolute position = 120px from window top (5 * 24px)
        // Content area starts at 120px (80px transport + 40px pattern bar)
        // So relative to content area: 120 - 120 = 0px from content top
        // No offset needed - cards start at top of content area
        
        // Calculate card width to fill available space
        int totalHorizontalPadding = horizontalPadding * (numCards - 1);
        int availableWidth = bounds.getWidth() - totalHorizontalPadding;
        int cardWidth = availableWidth / numCards;
        
        // Height from line 6 to line 26 = (26-6) * 24px = 480px
        int cardHeight = 480; // From line 6 to line 26
        
        // Position cards at the calculated offset
        int yOffset = bounds.getY();
        
        // Position each card
        for (int i = 0; i < numCards; ++i) {
            if (m_stageCards[i]) {
                int x = bounds.getX() + i * (cardWidth + horizontalPadding);
                m_stageCards[i]->setBounds(x, yOffset, cardWidth, cardHeight);
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