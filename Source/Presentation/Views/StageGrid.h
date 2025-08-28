// SPDX-License-Identifier: MIT
#pragma once

#include <JuceHeader.h>
#include "../Core/BaseComponent.h"
#include "../Core/DesignSystem.h"
#include "../../UI/Components/HAMComponentLibrary.h"
#include <memory>
#include <functional>
#include <vector>

namespace HAM {
    class HAMAudioProcessor;  // Forward declaration
}

namespace HAM::UI {

// ==========================================
// StageGrid - Container for 8 Stage Cards
// ==========================================
class StageGrid : public BaseComponent, public juce::Timer {
public:
    StageGrid() {
        // Start with 8 tracks to match TrackManager initialization
        setTrackCount(8);
        
        // Start timer for playhead updates (30 FPS)
        startTimer(33);
    }
    
    ~StageGrid() override {
        stopTimer();
    }
    
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
        
        // Draw playhead if playing
        if (m_isPlaying && m_currentStageIndex >= 0 && m_currentStageIndex < 8) {
            int cardWidth = getWidth() / 8;
            int x = m_currentStageIndex * cardWidth;
            
            // Draw vertical playhead line
            g.setColour(juce::Colour(0xFF00FFFF).withAlpha(0.8f)); // Cyan playhead
            g.fillRect(x, 0, 2, getHeight()); // 2px wide playhead
            
            // Add glow effect
            g.setColour(juce::Colour(0xFF00FFFF).withAlpha(0.3f));
            g.fillRect(x - 2, 0, 6, getHeight()); // Glow around playhead
        }
    }
    
    // Layout override
    void resized() override {
        // Get the actual available space
        auto bounds = getLocalBounds();
        
        // Calculate optimized card dimensions
        const int numCardsPerRow = 8;
        const int horizontalPadding = 1;  // 1px spacing between cards for clean separation
        const int verticalPadding = 8;    // 8px spacing between tracks to match TrackSidebar
        
        // Calculate card width to fill available space
        int totalHorizontalPadding = horizontalPadding * (numCardsPerRow - 1);
        int availableWidth = bounds.getWidth() - totalHorizontalPadding;
        int cardWidth = availableWidth / numCardsPerRow;
        
        // Cards should align directly at the top of the content area
        // Match TrackSidebar height for proper alignment
        int cardHeight = 510; // Match TrackSidebar::TRACK_HEIGHT (increased by 30px)
        
        // Position cards for each track
        for (int track = 0; track < m_trackCount; ++track) {
            int yOffset = track * (cardHeight + verticalPadding);
            
            for (int stage = 0; stage < numCardsPerRow; ++stage) {
                int cardIndex = track * numCardsPerRow + stage;
                if (cardIndex < static_cast<int>(m_stageCards.size())) {
                    int x = stage * (cardWidth + horizontalPadding);
                    m_stageCards[cardIndex]->setBounds(x, yOffset, cardWidth, cardHeight);
                }
            }
        }
    }
    
    // Public interface
    void setTrackCount(int count) {
        m_trackCount = count;
        
        // Clear existing cards
        m_stageCards.clear();
        removeAllChildren();
        
        // Create stage cards for all tracks
        for (int track = 0; track < count; ++track) {
            for (int stage = 0; stage < 8; ++stage) {
                auto card = std::make_unique<StageCard>();
                card->setStageNumber(stage + 1);
                card->setTrackColor(DesignTokens::Colors::getTrackColor(track % 8));
                
                // Connect Stage Editor button callback
                card->onStageEditorClicked = [this, track, stage](int stageNum) {
                    DBG("Stage Editor requested for track " << track << ", stage " << stageNum);
                    if (onHAMEditorRequested) onHAMEditorRequested(stageNum);
                };
                
                addAndMakeVisible(card.get());
                m_stageCards.push_back(std::move(card));
            }
        }
        
        resized();
    }
    
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
    StageCard* getStageCard(int track, int stage) {
        int index = track * 8 + stage;
        if (index >= 0 && index < static_cast<int>(m_stageCards.size())) {
            return m_stageCards[index].get();
        }
        return nullptr;
    }
    
    // Timer callback for playhead updates
    void timerCallback() override;
    
    // Set the audio processor for position tracking
    void setAudioProcessor(HAM::HAMAudioProcessor* proc) {
        m_processor = proc;
    }
    
    // Callbacks
    std::function<void(int track, int stage, const juce::String& param, float value)> onStageParameterChanged;
    std::function<void(int stage)> onStageSelected;
    std::function<void(int stage)> onHAMEditorRequested;
    
private:
    std::vector<std::unique_ptr<StageCard>> m_stageCards;
    int m_trackCount = 1;
    int m_activeStage = 0;
    bool m_showGridLines = false;
    
    // Playhead visualization
    HAM::HAMAudioProcessor* m_processor = nullptr;
    bool m_isPlaying = false;
    int m_currentStageIndex = -1;
};

} // namespace HAM::UI