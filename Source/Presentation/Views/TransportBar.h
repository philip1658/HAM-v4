// SPDX-License-Identifier: MIT
#pragma once

#include <JuceHeader.h>
#include "../Core/BaseComponent.h"
#include "../Core/DesignSystem.h"
#include "../../UI/Components/HAMComponentLibrary.h"
#include "../../UI/Components/HAMComponentLibraryExtended.h"
#include <memory>
#include <functional>

namespace HAM::UI {

// ==========================================
// Transport Bar - Main playback controls
// ==========================================
class TransportBar : public BaseComponent {
public:
    TransportBar() {
        // Create large transport buttons for visual hierarchy
        m_playButton = std::make_unique<LargeTransportButton>(LargeTransportButton::ButtonType::Play);
        m_playButton->onPlayStateChanged = [this](bool playing) {
            m_isPlaying = playing;
            if (onPlayStateChanged) {
                onPlayStateChanged(playing);
            }
        };
        addAndMakeVisible(m_playButton.get());
        
        // Create stop button
        m_stopButton = std::make_unique<LargeTransportButton>(LargeTransportButton::ButtonType::Stop);
        m_stopButton->onStop = [this]() {
            m_playButton->setPlaying(false);
            m_isPlaying = false;
            if (onStopClicked) {
                onStopClicked();
            }
            if (onPlayStateChanged) {
                onPlayStateChanged(false);
            }
        };
        addAndMakeVisible(m_stopButton.get());
        
        // Create record button  
        m_recordButton = std::make_unique<LargeTransportButton>(LargeTransportButton::ButtonType::Record);
        m_recordButton->onRecordStateChanged = [this](bool recording) {
            m_isRecording = recording;
            if (onRecordStateChanged) {
                onRecordStateChanged(recording);
            }
        };
        addAndMakeVisible(m_recordButton.get());
        
        // Create pattern buttons (8 quick access patterns - more slots!)
        for (int i = 0; i < 8; ++i) {
            auto patternButton = std::make_unique<PatternButton>(i + 1);
            patternButton->onPatternSelected = [this, i](int pattern) {
                selectPattern(i);
                if (onPatternSelected) {
                    onPatternSelected(pattern);
                }
            };
            patternButton->onPatternChain = [this](int pattern, bool chain) {
                if (onPatternChain) {
                    onPatternChain(pattern, chain);
                }
            };
            addAndMakeVisible(patternButton.get());
            m_patternButtons.push_back(std::move(patternButton));
        }
        
        // Set first pattern as active by default
        if (!m_patternButtons.empty()) {
            m_patternButtons[0]->setActive(true);
        }
        
        // Create tempo display (smaller)
        m_tempoDisplay = std::make_unique<TempoDisplay>();
        m_tempoDisplay->setBPM(120.0f);
        m_tempoDisplay->onBPMChanged = [this](float bpm) {
            m_currentBPM = bpm;
            if (onBPMChanged) {
                onBPMChanged(bpm);
            }
        };
        addAndMakeVisible(m_tempoDisplay.get());
        
        // Create tempo arrows
        m_tempoArrows = std::make_unique<TempoArrows>();
        m_tempoArrows->onTempoChange = [this](float increment) {
            m_currentBPM = juce::jlimit(20.0f, 999.0f, m_currentBPM + increment);
            m_tempoDisplay->setBPM(m_currentBPM);
            if (onBPMChanged) {
                onBPMChanged(m_currentBPM);
            }
        };
        addAndMakeVisible(m_tempoArrows.get());
        
        // Create only save and load buttons for pattern management
        m_saveButton = std::make_unique<PatternManagementButton>(PatternManagementButton::ButtonType::Save);
        m_saveButton->onClick = [this]() {
            if (onPatternSave) onPatternSave();
        };
        addAndMakeVisible(m_saveButton.get());
        
        m_loadButton = std::make_unique<PatternManagementButton>(PatternManagementButton::ButtonType::Load);
        m_loadButton->onClick = [this]() {
            if (onPatternLoad) onPatternLoad();
        };
        addAndMakeVisible(m_loadButton.get());
        
        // Create MIDI activity LED
        m_midiActivityLED = std::make_unique<LED>(juce::Colour(DesignTokens::Colors::ACCENT_GREEN));
        addAndMakeVisible(m_midiActivityLED.get());
        
        // Panic button removed - no longer needed
        
        // Set default size - unified height
        setSize(1200, 52);  // Matches UNIFIED_BAR_HEIGHT in UICoordinator
    }
    
    ~TransportBar() override = default;
    
    // Paint override
    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();
        
        // Background panel - dark void aesthetic
        g.setColour(juce::Colour(DesignTokens::Colors::BG_PANEL));
        g.fillRoundedRectangle(bounds, scaled(DesignTokens::Dimensions::CORNER_RADIUS));
        
        // Bottom border
        g.setColour(juce::Colour(DesignTokens::Colors::BORDER));
        g.drawLine(0, bounds.getBottom() - 1, bounds.getRight(), bounds.getBottom() - 1, scaled(1));
        
        // Section dividers
        g.setColour(juce::Colour(DesignTokens::Colors::HAIRLINE));
        
        // After transport buttons (smaller now at 160px)
        float dividerX = scaled(160);
        g.drawLine(dividerX, scaled(10), dividerX, bounds.getBottom() - scaled(10), scaled(0.5f));
        
        // After tempo (now comes second at 330px)
        dividerX = scaled(330);
        g.drawLine(dividerX, scaled(10), dividerX, bounds.getBottom() - scaled(10), scaled(0.5f));
        
        // After pattern section (now at 1050px with more buttons)
        dividerX = scaled(1050);
        g.drawLine(dividerX, scaled(10), dividerX, bounds.getBottom() - scaled(10), scaled(0.5f));
        
        // Before status section (smaller now without CPU)
        dividerX = getWidth() - scaled(100);
        g.drawLine(dividerX, scaled(10), dividerX, bounds.getBottom() - scaled(10), scaled(0.5f));
    }
    
    // Resize override
    void resized() override {
        // UNIFIED GRID LAYOUT: Centered button groups for visual balance
        const int UNIFIED_BUTTON_HEIGHT = scaled(36);
        const int UNIFIED_BUTTON_WIDTH = scaled(43);  // Standard width for 8-button groups
        const int TIGHT_SPACING = scaled(4);          // Between buttons in 8-button groups
        const int UNIFIED_SPACING = scaled(8);        // Between different button groups
        const int UNIFIED_MARGIN = scaled(8);
        
        auto bounds = getLocalBounds().reduced(UNIFIED_MARGIN);
        int buttonY = (getHeight() - UNIFIED_BUTTON_HEIGHT) / 2;  // Center vertically
        
        // Calculate total width of pattern button group
        const int CONTROL_BUTTON_WIDTH = scaled(45);
        int patternGroupWidth = (8 * UNIFIED_BUTTON_WIDTH) +  // 8 pattern buttons
                               (7 * TIGHT_SPACING) +           // Spacing between patterns
                               UNIFIED_SPACING +                // Gap before save/load
                               (2 * CONTROL_BUTTON_WIDTH) +    // Save and Load buttons
                               TIGHT_SPACING;                   // Space between save/load
        
        // Center the pattern group horizontally
        int patternGroupX = (getWidth() - patternGroupWidth) / 2;
        int currentX = patternGroupX;
        
        // Store the pattern start position for alignment with scale slots
        m_patternStartX = currentX;  // We'll need to add this member variable
        
        // Pattern Section: 8 buttons with consistent width and spacing
        for (size_t i = 0; i < 8 && i < m_patternButtons.size(); ++i) {
            m_patternButtons[i]->setBounds(currentX, buttonY, UNIFIED_BUTTON_WIDTH, UNIFIED_BUTTON_HEIGHT);
            currentX += UNIFIED_BUTTON_WIDTH + TIGHT_SPACING;
        }
        
        currentX += UNIFIED_SPACING;  // Gap before save/load
        
        // Save/Load buttons
        m_saveButton->setBounds(currentX, buttonY, CONTROL_BUTTON_WIDTH, UNIFIED_BUTTON_HEIGHT);
        currentX += CONTROL_BUTTON_WIDTH + TIGHT_SPACING;
        
        m_loadButton->setBounds(currentX, buttonY, CONTROL_BUTTON_WIDTH, UNIFIED_BUTTON_HEIGHT);
        
        // Left Section: Transport buttons (positioned to the left of center)
        int transportX = UNIFIED_MARGIN * 2;
        m_playButton->setBounds(transportX, buttonY, scaled(40), UNIFIED_BUTTON_HEIGHT);
        transportX += scaled(40) + TIGHT_SPACING;
        
        m_stopButton->setBounds(transportX, buttonY, scaled(40), UNIFIED_BUTTON_HEIGHT);
        transportX += scaled(40) + TIGHT_SPACING;
        
        m_recordButton->setBounds(transportX, buttonY, scaled(40), UNIFIED_BUTTON_HEIGHT);
        
        // Tempo Section (positioned after transport)
        int tempoX = transportX + scaled(40) + UNIFIED_SPACING * 2;
        m_tempoDisplay->setBounds(tempoX, buttonY, scaled(75), UNIFIED_BUTTON_HEIGHT);
        tempoX += scaled(75) + TIGHT_SPACING;
        
        m_tempoArrows->setBounds(tempoX, buttonY, scaled(30), UNIFIED_BUTTON_HEIGHT);
        
        // Right Section: MIDI LED (far right)
        auto ledBounds = bounds.removeFromRight(scaled(20))
                              .withSizeKeepingCentre(scaled(16), scaled(16));
        m_midiActivityLED->setBounds(ledBounds);
    }
    
    // Public methods
    void setPlayState(bool playing) {
        m_isPlaying = playing;
        if (m_playButton) {
            m_playButton->setPlaying(playing);
        }
    }
    
    void setRecordState(bool recording) {
        m_isRecording = recording;
        // RecordButton doesn't have setState method
        // It manages its own visual state
    }
    
    void setBPM(float bpm) {
        m_currentBPM = bpm;
        if (m_tempoDisplay) {
            m_tempoDisplay->setBPM(bpm);
        }
    }
    
    
    // Callbacks
    std::function<void(bool playing)> onPlayStateChanged;
    std::function<void()> onStopClicked;
    std::function<void(bool recording)> onRecordStateChanged;
    std::function<void(float bpm)> onBPMChanged;
    std::function<void(int pattern)> onPatternSelected;
    std::function<void(int pattern, bool chain)> onPatternChain;
    std::function<void()> onPatternSave;
    std::function<void()> onPatternLoad;
    // Panic button removed - std::function<void()> onPanicClicked;
    
    bool isPlaying() const { return m_isPlaying; }
    
    // Get the X position where pattern buttons start (for alignment)
    int getPatternStartX() const { return m_patternStartX; }
    
    // Public methods for external control
    void setMidiActivity(bool active) {
        if (m_midiActivityLED) {
            m_midiActivityLED->setOn(active);
        }
    }
    
    void selectPattern(int index) {
        // Deactivate all patterns
        for (auto& btn : m_patternButtons) {
            btn->setActive(false);
        }
        // Activate selected pattern
        if (index >= 0 && index < static_cast<int>(m_patternButtons.size())) {
            m_patternButtons[index]->setActive(true);
        }
    }
    
private:
    // Transport controls - VISUAL HIERARCHY PRIMARY
    std::unique_ptr<LargeTransportButton> m_playButton;
    std::unique_ptr<LargeTransportButton> m_stopButton;
    std::unique_ptr<LargeTransportButton> m_recordButton;
    
    // Pattern controls - VISUAL HIERARCHY SECONDARY  
    std::vector<std::unique_ptr<PatternButton>> m_patternButtons;
    std::unique_ptr<PatternManagementButton> m_saveButton;
    std::unique_ptr<PatternManagementButton> m_loadButton;
    
    // Tempo controls - VISUAL HIERARCHY TERTIARY
    std::unique_ptr<TempoDisplay> m_tempoDisplay;
    std::unique_ptr<TempoArrows> m_tempoArrows;
    
    // Status displays - VISUAL HIERARCHY MINIMAL
    std::unique_ptr<LED> m_midiActivityLED;
    // Panic button removed
    
    // State
    bool m_isPlaying = false;
    bool m_isRecording = false;
    float m_currentBPM = 120.0f;
    int m_patternStartX = 0;  // Store pattern button start position for alignment
};

} // namespace HAM::UI