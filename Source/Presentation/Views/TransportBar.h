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
class TransportBar : public BaseComponent, 
                     private juce::Timer {
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
        
        // Create pattern buttons (4 quick access patterns)
        for (int i = 0; i < 4; ++i) {
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
        
        // Create compact swing knob
        m_swingKnob = std::make_unique<CompactSwingKnob>();
        m_swingKnob->setValue(0.5f); // 50% = no swing
        m_swingKnob->onValueChange = [this](float value) {
            if (onSwingChanged) {
                onSwingChanged(value);
            }
        };
        addAndMakeVisible(m_swingKnob.get());
        
        // Create pattern length display
        m_patternLengthLabel = std::make_unique<juce::Label>();
        m_patternLengthLabel->setText("16", juce::dontSendNotification);
        m_patternLengthLabel->setColour(juce::Label::textColourId, 
                                       juce::Colour(DesignTokens::Colors::TEXT_PRIMARY));
        m_patternLengthLabel->setFont(juce::Font(juce::FontOptions(12.0f)));
        m_patternLengthLabel->setJustificationType(juce::Justification::centred);
        addAndMakeVisible(m_patternLengthLabel.get());
        
        // Pattern length label
        m_lengthLabel = std::make_unique<juce::Label>();
        m_lengthLabel->setText("LEN", juce::dontSendNotification);
        m_lengthLabel->setColour(juce::Label::textColourId, 
                                juce::Colour(DesignTokens::Colors::TEXT_MUTED));
        m_lengthLabel->setFont(juce::Font(juce::FontOptions(10.0f)));
        m_lengthLabel->setJustificationType(juce::Justification::centred);
        addAndMakeVisible(m_lengthLabel.get());
        
        // Create CPU meter (simple text display for now)
        m_cpuLabel = std::make_unique<juce::Label>();
        m_cpuLabel->setText("CPU: 2%", juce::dontSendNotification);
        m_cpuLabel->setColour(juce::Label::textColourId, 
                             juce::Colour(DesignTokens::Colors::TEXT_MUTED));
        m_cpuLabel->setFont(juce::Font(juce::FontOptions(10.0f)));
        m_cpuLabel->setJustificationType(juce::Justification::centred);
        addAndMakeVisible(m_cpuLabel.get());
        
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
        
        // Start timer for CPU updates (10 Hz)
        startTimerHz(10);
    }
    
    ~TransportBar() override {
        stopTimer();
    }
    
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
        float dividerX = scaled(200);
        g.drawLine(dividerX, scaled(10), dividerX, bounds.getBottom() - scaled(10), scaled(0.5f));
        
        // After pattern buttons
        dividerX = scaled(530);
        g.drawLine(dividerX, scaled(10), dividerX, bounds.getBottom() - scaled(10), scaled(0.5f));
        
        // After tempo
        dividerX = scaled(690);
        g.drawLine(dividerX, scaled(10), dividerX, bounds.getBottom() - scaled(10), scaled(0.5f));
        
        // Before status section
        dividerX = getWidth() - scaled(200);
        g.drawLine(dividerX, scaled(10), dividerX, bounds.getBottom() - scaled(10), scaled(0.5f));
    }
    
    // Resize override
    void resized() override {
        auto bounds = getLocalBounds().reduced(scaled(10));
        
        // Left Section: Transport buttons (180px)
        auto transportSection = bounds.removeFromLeft(scaled(180));
        
        // Play button
        m_playButton->setBounds(transportSection.removeFromLeft(scaled(50))
                                              .withSizeKeepingCentre(scaled(45), scaled(45)));
        transportSection.removeFromLeft(scaled(5));
        
        // Stop button
        m_stopButton->setBounds(transportSection.removeFromLeft(scaled(50))
                                              .withSizeKeepingCentre(scaled(45), scaled(45)));
        transportSection.removeFromLeft(scaled(5));
        
        // Record button
        m_recordButton->setBounds(transportSection.removeFromLeft(scaled(50))
                                                .withSizeKeepingCentre(scaled(45), scaled(45)));
        
        bounds.removeFromLeft(scaled(15));
        
        // Pattern buttons section (320px)
        auto patternSection = bounds.removeFromLeft(scaled(320));
        float patternButtonWidth = scaled(75);
        float patternButtonHeight = scaled(40);
        float patternGap = scaled(5);
        
        for (size_t i = 0; i < m_patternButtons.size(); ++i) {
            auto buttonBounds = patternSection.removeFromLeft(patternButtonWidth)
                                             .withSizeKeepingCentre(patternButtonWidth, patternButtonHeight);
            m_patternButtons[i]->setBounds(buttonBounds);
            patternSection.removeFromLeft(patternGap);
        }
        
        bounds.removeFromLeft(scaled(15));
        
        // Tempo section (140px total)
        auto tempoSection = bounds.removeFromLeft(scaled(140));
        
        // Tempo display (smaller)
        auto tempoDisplayBounds = tempoSection.removeFromLeft(scaled(90))
                                              .withSizeKeepingCentre(scaled(85), scaled(45));
        m_tempoDisplay->setBounds(tempoDisplayBounds);
        
        tempoSection.removeFromLeft(scaled(5));
        
        // Tempo arrows
        auto arrowBounds = tempoSection.removeFromLeft(scaled(40))
                                      .withSizeKeepingCentre(scaled(35), scaled(40));
        m_tempoArrows->setBounds(arrowBounds);
        
        bounds.removeFromLeft(scaled(15));
        
        // Additional controls section (200px)
        auto controlsSection = bounds.removeFromLeft(scaled(200));
        
        // Swing knob
        auto swingBounds = controlsSection.removeFromLeft(scaled(45))
                                         .withSizeKeepingCentre(scaled(40), scaled(40));
        m_swingKnob->setBounds(swingBounds);
        
        controlsSection.removeFromLeft(scaled(10));
        
        // Pattern length
        auto lengthSection = controlsSection.removeFromLeft(scaled(60));
        m_lengthLabel->setBounds(lengthSection.removeFromTop(scaled(15)));
        m_patternLengthLabel->setBounds(lengthSection.withSizeKeepingCentre(scaled(50), scaled(25)));
        
        // Spacer to push status section to the right
        bounds.removeFromLeft(scaled(20));
        
        // Right Section: Status & Monitoring (remaining space)
        auto statusSection = bounds;
        
        // CPU meter
        auto cpuBounds = statusSection.removeFromLeft(scaled(60))
                                     .withSizeKeepingCentre(scaled(55), scaled(30));
        m_cpuLabel->setBounds(cpuBounds);
        
        statusSection.removeFromLeft(scaled(5));
        
        // MIDI LED
        auto ledBounds = statusSection.removeFromLeft(scaled(30))
                                     .withSizeKeepingCentre(scaled(25), scaled(25));
        m_midiActivityLED->setBounds(ledBounds);
        
        statusSection.removeFromLeft(scaled(10));
        
        // Panic button
        auto panicBounds = statusSection.removeFromLeft(scaled(60))
                                       .withSizeKeepingCentre(scaled(55), scaled(30));
        m_panicButton->setBounds(panicBounds);
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
    
    void setSwing(float swing) {
        if (m_swingKnob) {
            m_swingKnob->setValue(swing);
        }
    }
    
    void setPatternLength(int length) {
        if (m_patternLengthLabel) {
            m_patternLengthLabel->setText(juce::String(length), juce::dontSendNotification);
        }
    }
    
    // Callbacks
    std::function<void(bool playing)> onPlayStateChanged;
    std::function<void()> onStopClicked;
    std::function<void(bool recording)> onRecordStateChanged;
    std::function<void(float bpm)> onBPMChanged;
    std::function<void(float swing)> onSwingChanged;
    std::function<void(int pattern)> onPatternSelected;
    std::function<void(int pattern, bool chain)> onPatternChain;
    std::function<void()> onPanicClicked;
    
    // Callback to get current CPU usage
    std::function<float()> onRequestCPUUsage;
    
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
    // Timer callback for updating CPU and other periodic displays
    void timerCallback() override {
        // Update CPU usage
        if (onRequestCPUUsage) {
            float cpuUsage = onRequestCPUUsage();
            juce::String cpuText = juce::String::formatted("CPU: %.1f%%", cpuUsage);
            m_cpuLabel->setText(cpuText, juce::dontSendNotification);
            
            // Color code based on CPU usage
            if (cpuUsage > 80.0f) {
                m_cpuLabel->setColour(juce::Label::textColourId, 
                                     juce::Colour(DesignTokens::Colors::ACCENT_RED));
            } else if (cpuUsage > 50.0f) {
                m_cpuLabel->setColour(juce::Label::textColourId, 
                                     juce::Colour(DesignTokens::Colors::ACCENT_AMBER));
            } else {
                m_cpuLabel->setColour(juce::Label::textColourId, 
                                     juce::Colour(DesignTokens::Colors::TEXT_MUTED));
            }
        }
    }
    
    // Transport controls from HAM component library
    std::unique_ptr<PlayButton> m_playButton;
    std::unique_ptr<StopButton> m_stopButton;
    std::unique_ptr<RecordButton> m_recordButton;
    std::vector<std::unique_ptr<PatternButton>> m_patternButtons;
    std::unique_ptr<TempoDisplay> m_tempoDisplay;
    std::unique_ptr<TempoArrows> m_tempoArrows;
    std::unique_ptr<CompactSwingKnob> m_swingKnob;
    std::unique_ptr<juce::Label> m_patternLengthLabel;
    std::unique_ptr<juce::Label> m_lengthLabel;
    std::unique_ptr<juce::Label> m_cpuLabel;
    std::unique_ptr<LED> m_midiActivityLED;
    std::unique_ptr<PanicButton> m_panicButton;
    
    // State
    bool m_isPlaying = false;
    bool m_isRecording = false;
    float m_currentBPM = 120.0f;
};

} // namespace HAM::UI