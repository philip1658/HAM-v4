// SPDX-License-Identifier: MIT
#pragma once

#include <JuceHeader.h>
#include "../Core/BaseComponent.h"
#include "../Core/DesignSystem.h"
#include "../../UI/Components/HAMComponentLibrary.h"
#include "StageCard.h"
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
            
            // Set unique color for each stage from track colors
            auto& sliders = getStageSliders(card.get());
            auto trackColor = DesignTokens::getTrackColor(i);
            sliders.pitch->setTrackColor(trackColor.withAlpha(0.8f));
            sliders.pulse->setTrackColor(trackColor.withAlpha(0.6f));
            sliders.velocity->setTrackColor(trackColor.withAlpha(0.7f));
            sliders.gate->setTrackColor(trackColor.withAlpha(0.5f));
            
            // Setup callbacks for this stage
            setupStageCallbacks(card.get(), i);
            
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
        g.fillAll(juce::Colour(DesignTokens::Colors::BG_VOID));
        
        // Optional grid background pattern
        if (m_showGridLines) {
            g.setColour(juce::Colour(DesignTokens::Colors::GRID_LINE));
            
            // Vertical lines between stages
            const int cardWidth = 140;
            const int spacing = 16;
            int x = cardWidth + spacing / 2;
            
            for (int i = 0; i < 7; ++i) {
                g.drawLine(x, 0, x, getHeight(), scaled(0.5f));
                x += cardWidth + spacing;
            }
        }
    }
    
    // Resize override
    void resized() override {
        const int cardWidth = 140;
        const int cardHeight = 420;
        const int spacing = 16;
        
        // Calculate starting position to center the grid
        const int totalWidth = (cardWidth * 8) + (spacing * 7);
        int startX = (getWidth() - totalWidth) / 2;
        const int y = (getHeight() - cardHeight) / 2;
        
        // Position each stage card
        for (int i = 0; i < 8; ++i) {
            if (m_stageCards[i]) {
                m_stageCards[i]->setBounds(startX, y, cardWidth, cardHeight);
                startX += cardWidth + spacing;
            }
        }
    }
    
    // Public methods
    void setActiveStage(int track, int stage) {
        // For now, single track support - highlight active stage
        for (int i = 0; i < 8; ++i) {
            if (m_stageCards[i]) {
                m_stageCards[i]->setActive(i == stage);
            }
        }
        m_activeStage = stage;
    }
    
    void setStageParameter(int stage, const juce::String& param, float value) {
        if (stage >= 0 && stage < 8 && m_stageCards[stage]) {
            auto& sliders = getStageSliders(m_stageCards[stage].get());
            
            if (param == "pitch" && sliders.pitch) {
                sliders.pitch->setValue(value / 127.0f); // Normalize to 0-1
            } else if (param == "pulse" && sliders.pulse) {
                sliders.pulse->setValue(value / 8.0f); // Normalize to 0-1
            } else if (param == "velocity" && sliders.velocity) {
                sliders.velocity->setValue(value / 127.0f); // Normalize to 0-1
            } else if (param == "gate" && sliders.gate) {
                sliders.gate->setValue(value); // Already 0-1
            }
        }
    }
    
    void setShowGridLines(bool show) {
        m_showGridLines = show;
        repaint();
    }
    
    // Callbacks
    std::function<void(int track, int stage, const juce::String& param, float value)> onStageParameterChanged;
    std::function<void(int stage)> onStageSelected;
    std::function<void(int stage)> onHAMEditorRequested;
    
private:
    // Helper to get slider references from stage card
    struct StageSliders {
        ModernSlider* pitch;
        ModernSlider* pulse;
        ModernSlider* velocity;
        ModernSlider* gate;
    };
    
    StageSliders getStageSliders(StageCard* card) {
        return {
            card->getPitchSlider(),
            card->getPulseSlider(),
            card->getVelocitySlider(),
            card->getGateSlider()
        };
    }
    
    void setupStageCallbacks(StageCard* card, int stageIndex) {
        auto& sliders = getStageSliders(card);
        
        // Pitch slider callback
        if (sliders.pitch) {
            sliders.pitch->onValueChange = [this, stageIndex](float value) {
                float pitchValue = value * 127.0f; // Convert to MIDI range
                if (onStageParameterChanged) {
                    onStageParameterChanged(0, stageIndex, "pitch", pitchValue);
                }
            };
        }
        
        // Pulse slider callback
        if (sliders.pulse) {
            sliders.pulse->onValueChange = [this, stageIndex](float value) {
                float pulseValue = value * 8.0f; // Convert to 1-8 range
                if (onStageParameterChanged) {
                    onStageParameterChanged(0, stageIndex, "pulse", pulseValue);
                }
            };
        }
        
        // Velocity slider callback
        if (sliders.velocity) {
            sliders.velocity->onValueChange = [this, stageIndex](float value) {
                float velocityValue = value * 127.0f; // Convert to MIDI range
                if (onStageParameterChanged) {
                    onStageParameterChanged(0, stageIndex, "velocity", velocityValue);
                }
            };
        }
        
        // Gate slider callback
        if (sliders.gate) {
            sliders.gate->onValueChange = [this, stageIndex](float value) {
                if (onStageParameterChanged) {
                    onStageParameterChanged(0, stageIndex, "gate", value);
                }
            };
        }
    }
    
    // Stage cards
    std::array<std::unique_ptr<StageCard>, 8> m_stageCards;
    
    // State
    int m_activeStage = -1;
    bool m_showGridLines = false;
};

} // namespace HAM::UI