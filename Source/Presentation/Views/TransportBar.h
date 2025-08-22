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
        // Create play button using HAM library
        m_playButton = std::make_unique<PlayButton>();
        m_playButton->onPlayStateChanged = [this](bool playing) {
            m_isPlaying = playing;
            if (onPlayStateChanged) {
                onPlayStateChanged(playing);
            }
        };
        addAndMakeVisible(m_playButton.get());
        
        // Create stop button
        m_stopButton = std::make_unique<StopButton>();
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
        m_recordButton = std::make_unique<RecordButton>();
        m_recordButton->onRecordStateChanged = [this](bool recording) {
            m_isRecording = recording;
            if (onRecordStateChanged) {
                onRecordStateChanged(recording);
            }
        };
        addAndMakeVisible(m_recordButton.get());
        
        // Create tempo display
        m_tempoDisplay = std::make_unique<TempoDisplay>();
        m_tempoDisplay->setBPM(120.0f);
        m_tempoDisplay->onBPMChanged = [this](float bpm) {
            m_currentBPM = bpm;
            if (onBPMChanged) {
                onBPMChanged(bpm);
            }
        };
        addAndMakeVisible(m_tempoDisplay.get());
        
        // Create position display label
        m_positionLabel = std::make_unique<juce::Label>();
        m_positionLabel->setText("001:01:00", juce::dontSendNotification);
        m_positionLabel->setColour(juce::Label::textColourId, 
                                  juce::Colour(DesignTokens::Colors::TEXT_PRIMARY));
        m_positionLabel->setFont(juce::Font("Menlo", 14.0f, juce::Font::plain));
        m_positionLabel->setJustificationType(juce::Justification::centred);
        addAndMakeVisible(m_positionLabel.get());
        
        // Create swing control
        m_swingSlider = std::make_unique<ModernSlider>(false); // Horizontal
        m_swingSlider->setLabel("SWING");
        m_swingSlider->setValue(0.5f); // 50% = no swing
        m_swingSlider->setTrackColor(juce::Colour(DesignTokens::Colors::ACCENT_AMBER));
        m_swingSlider->onValueChange = [this](float value) {
            if (onSwingChanged) {
                onSwingChanged(value);
            }
        };
        addAndMakeVisible(m_swingSlider.get());

        // MIDI Monitor toggle (Channel 16 debug)
        m_midiMonitorToggle = std::make_unique<ModernToggle>();
        m_midiMonitorToggle->setChecked(false);
        m_midiMonitorToggle->onToggle = [this](bool enabled) {
            if (onMidiMonitorToggled) {
                onMidiMonitorToggled(enabled);
            }
        };
        addAndMakeVisible(m_midiMonitorToggle.get());

        m_midiMonitorLabel = std::make_unique<juce::Label>("", "MON");
        m_midiMonitorLabel->setJustificationType(juce::Justification::centred);
        m_midiMonitorLabel->setColour(juce::Label::textColourId, juce::Colour(DesignTokens::Colors::TEXT_MUTED));
        addAndMakeVisible(m_midiMonitorLabel.get());
        
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
        
        // After transport buttons
        float dividerX = scaled(260);
        g.drawLine(dividerX, scaled(10), dividerX, bounds.getBottom() - scaled(10), scaled(0.5f));
        
        // After tempo
        dividerX = scaled(500);
        g.drawLine(dividerX, scaled(10), dividerX, bounds.getBottom() - scaled(10), scaled(0.5f));
    }
    
    // Resize override
    void resized() override {
        auto bounds = getLocalBounds().reduced(scaled(10));
        
        // Transport buttons section (left)
        auto transportSection = bounds.removeFromLeft(scaled(240));
        
        // Play button
        m_playButton->setBounds(transportSection.removeFromLeft(scaled(60))
                                              .withSizeKeepingCentre(scaled(50), scaled(50)));
        transportSection.removeFromLeft(scaled(10));
        
        // Stop button
        m_stopButton->setBounds(transportSection.removeFromLeft(scaled(60))
                                              .withSizeKeepingCentre(scaled(50), scaled(50)));
        transportSection.removeFromLeft(scaled(10));
        
        // Record button
        m_recordButton->setBounds(transportSection.removeFromLeft(scaled(60))
                                                .withSizeKeepingCentre(scaled(50), scaled(50)));
        
        bounds.removeFromLeft(scaled(20));
        
        // Tempo section
        auto tempoSection = bounds.removeFromLeft(scaled(220));
        m_tempoDisplay->setBounds(tempoSection.withSizeKeepingCentre(scaled(200), scaled(50)));
        
        bounds.removeFromLeft(scaled(20));
        
        // Position display
        auto positionSection = bounds.removeFromLeft(scaled(120));
        m_positionLabel->setBounds(positionSection.withSizeKeepingCentre(scaled(100), scaled(30)));
        
        bounds.removeFromLeft(scaled(20));
        
        // Swing control
        auto swingSection = bounds.removeFromLeft(scaled(150));
        m_swingSlider->setBounds(swingSection.withSizeKeepingCentre(scaled(140), scaled(40)));

        // MIDI Monitor toggle section
        bounds.removeFromLeft(scaled(10));
        auto monitorSection = bounds.removeFromLeft(scaled(80));
        if (m_midiMonitorToggle) {
            auto toggleArea = monitorSection.removeFromLeft(monitorSection.getWidth() / 2);
            m_midiMonitorToggle->setBounds(toggleArea.withSizeKeepingCentre(scaled(44), scaled(24)));
        }
        if (m_midiMonitorLabel) {
            m_midiMonitorLabel->setBounds(monitorSection);
        }
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
    
    void setPosition(int bar, int beat, int pulse) {
        juce::String posText = juce::String::formatted("%03d:%02d:%02d", bar, beat, pulse);
        if (m_positionLabel) {
            m_positionLabel->setText(posText, juce::dontSendNotification);
        }
    }
    
    void setSwing(float swing) {
        if (m_swingSlider) {
            m_swingSlider->setValue(swing);
        }
    }
    
    // Callbacks
    std::function<void(bool playing)> onPlayStateChanged;
    std::function<void()> onStopClicked;
    std::function<void(bool recording)> onRecordStateChanged;
    std::function<void(float bpm)> onBPMChanged;
    std::function<void(float swing)> onSwingChanged;
    std::function<void(bool enabled)> onMidiMonitorToggled;
    
    bool isPlaying() const { return m_isPlaying; }
    
private:
    // Transport controls from HAM component library
    std::unique_ptr<PlayButton> m_playButton;
    std::unique_ptr<StopButton> m_stopButton;
    std::unique_ptr<RecordButton> m_recordButton;
    std::unique_ptr<TempoDisplay> m_tempoDisplay;
    std::unique_ptr<juce::Label> m_positionLabel;
    std::unique_ptr<ModernSlider> m_swingSlider;
    std::unique_ptr<ModernToggle> m_midiMonitorToggle;
    std::unique_ptr<juce::Label> m_midiMonitorLabel;
    
    // State
    bool m_isPlaying = false;
    bool m_isRecording = false;
    float m_currentBPM = 120.0f;
};

} // namespace HAM::UI