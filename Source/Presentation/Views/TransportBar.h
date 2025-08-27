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
        
        // Create panic button
        m_panicButton = std::make_unique<PanicButton>();
        m_panicButton->onPanic = [this]() {
            if (onPanicClicked) {
                onPanicClicked();
            }
        };
        addAndMakeVisible(m_panicButton.get());
        
        // Set default size
        setSize(1200, 80);
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
        auto bounds = getLocalBounds().reduced(scaled(10));
        
        // Left Section: Transport buttons (160px) - More reasonable sizes
        auto transportSection = bounds.removeFromLeft(scaled(160));
        
        // Play button (45px - still prominent but not oversized)
        m_playButton->setBounds(transportSection.removeFromLeft(scaled(50))
                                              .withSizeKeepingCentre(scaled(45), scaled(45)));
        transportSection.removeFromLeft(scaled(5));
        
        // Stop button (45px)
        m_stopButton->setBounds(transportSection.removeFromLeft(scaled(50))
                                              .withSizeKeepingCentre(scaled(45), scaled(45)));
        transportSection.removeFromLeft(scaled(5));
        
        // Record button (40px - slightly smaller)
        m_recordButton->setBounds(transportSection.removeFromLeft(scaled(45))
                                                .withSizeKeepingCentre(scaled(40), scaled(40)));
        
        bounds.removeFromLeft(scaled(15)); // Visual separator
        
        // Tempo Section FIRST (150px) - More logical placement before patterns
        auto tempoSection = bounds.removeFromLeft(scaled(150));
        
        // Tempo display (medium size)
        auto tempoDisplayBounds = tempoSection.removeFromLeft(scaled(100))
                                              .withSizeKeepingCentre(scaled(95), scaled(45));
        m_tempoDisplay->setBounds(tempoDisplayBounds);
        
        tempoSection.removeFromLeft(scaled(5));
        
        // Tempo arrows (smaller)
        auto arrowBounds = tempoSection.removeFromLeft(scaled(40))
                                      .withSizeKeepingCentre(scaled(35), scaled(35));
        m_tempoArrows->setBounds(arrowBounds);
        
        bounds.removeFromLeft(scaled(20)); // Visual separator
        
        // Pattern Section (700px) - More space for 8 pattern buttons
        auto patternSection = bounds.removeFromLeft(scaled(700));
        
        // Pattern selection buttons - now 8 buttons arranged in two rows
        // No pattern name label - cleaner interface
        auto patternTopRow = patternSection.removeFromTop(scaled(32));
        auto patternBottomRow = patternSection.removeFromTop(scaled(32));
        
        float patternButtonWidth = scaled(65);
        float patternGap = scaled(5);
        
        // First row - patterns 1-4
        for (size_t i = 0; i < 4 && i < m_patternButtons.size(); ++i) {
            auto buttonBounds = patternTopRow.removeFromLeft(patternButtonWidth)
                                            .withSizeKeepingCentre(patternButtonWidth, scaled(28));
            m_patternButtons[i]->setBounds(buttonBounds);
            patternTopRow.removeFromLeft(patternGap);
        }
        
        // Second row - patterns 5-8
        for (size_t i = 4; i < 8 && i < m_patternButtons.size(); ++i) {
            auto buttonBounds = patternBottomRow.removeFromLeft(patternButtonWidth)
                                               .withSizeKeepingCentre(patternButtonWidth, scaled(28));
            m_patternButtons[i]->setBounds(buttonBounds);
            patternBottomRow.removeFromLeft(patternGap);
        }
        
        // Gap before save/load buttons
        patternTopRow.removeFromLeft(scaled(20));
        
        // Save/Load buttons aligned with pattern buttons
        auto saveLoadWidth = scaled(55);
        m_saveButton->setBounds(patternTopRow.removeFromLeft(saveLoadWidth)
                                            .withSizeKeepingCentre(saveLoadWidth, scaled(28)));
        patternTopRow.removeFromLeft(scaled(5));
        
        m_loadButton->setBounds(patternTopRow.removeFromLeft(saveLoadWidth)
                                            .withSizeKeepingCentre(saveLoadWidth, scaled(28)));
        
        // Right Section: Status & Monitoring (remaining space)
        auto statusSection = bounds.removeFromRight(scaled(100));
        
        // Panic button (rightmost)
        auto panicBounds = statusSection.removeFromRight(scaled(55))
                                       .withSizeKeepingCentre(scaled(50), scaled(25));
        m_panicButton->setBounds(panicBounds);
        
        statusSection.removeFromRight(scaled(10));
        
        // MIDI LED
        auto ledBounds = statusSection.removeFromRight(scaled(25))
                                     .withSizeKeepingCentre(scaled(20), scaled(20));
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
    std::function<void()> onPanicClicked;
    
    bool isPlaying() const { return m_isPlaying; }
    
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
    std::unique_ptr<PanicButton> m_panicButton;
    
    // State
    bool m_isPlaying = false;
    bool m_isRecording = false;
    float m_currentBPM = 120.0f;
};

} // namespace HAM::UI