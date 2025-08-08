// SPDX-License-Identifier: MIT
#pragma once

#include <JuceHeader.h>
#include "Components/HAMComponentLibrary.h"
#include "Components/OptimizedComponents.h"

namespace HAM::UI {

/**
 * Side-by-side comparison of current vs optimized components
 * Shows render time and visual differences
 */
class PerformanceComparison : public juce::Component, private juce::Timer {
public:
    PerformanceComparison() {
        // Create current (rich) version
        m_currentCard = std::make_unique<StageCard>();
        m_currentCard->setTrackColor(DesignTokens::Colors::getTrackColor(0));
        addAndMakeVisible(m_currentCard.get());
        
        // Create optimized version
        m_optimizedCard = std::make_unique<Optimized::OptimizedStageCard>();
        m_optimizedCard->setTrackColor(DesignTokens::Colors::getTrackColor(0));
        addAndMakeVisible(m_optimizedCard.get());
        
        // Labels
        m_currentLabel = std::make_unique<juce::Label>("current", "Current (Rich) Version");
        m_currentLabel->setFont(juce::Font(16.0f, juce::Font::bold));
        m_currentLabel->setJustificationType(juce::Justification::centred);
        addAndMakeVisible(m_currentLabel.get());
        
        m_optimizedLabel = std::make_unique<juce::Label>("optimized", "Optimized Version");
        m_optimizedLabel->setFont(juce::Font(16.0f, juce::Font::bold));
        m_optimizedLabel->setJustificationType(juce::Justification::centred);
        addAndMakeVisible(m_optimizedLabel.get());
        
        // Performance labels
        m_currentPerfLabel = std::make_unique<juce::Label>("currentPerf", "Render time: --");
        m_currentPerfLabel->setFont(juce::Font(12.0f));
        m_currentPerfLabel->setJustificationType(juce::Justification::centred);
        addAndMakeVisible(m_currentPerfLabel.get());
        
        m_optimizedPerfLabel = std::make_unique<juce::Label>("optimizedPerf", "Render time: --");
        m_optimizedPerfLabel->setFont(juce::Font(12.0f));
        m_optimizedPerfLabel->setJustificationType(juce::Justification::centred);
        addAndMakeVisible(m_optimizedPerfLabel.get());
        
        // Start performance monitoring
        startTimerHz(10); // Measure at 10Hz
        
        setSize(600, 500);
    }
    
    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(0xFF1A1A1A));
        
        // Draw dividing line
        g.setColour(juce::Colours::white.withAlpha(0.2f));
        g.drawVerticalLine(getWidth() / 2, 0, getHeight());
        
        // Measure render times
        measureRenderTimes();
    }
    
    void resized() override {
        auto bounds = getLocalBounds();
        auto halfWidth = bounds.getWidth() / 2;
        
        // Left side - current version
        auto leftBounds = bounds.removeFromLeft(halfWidth).reduced(20);
        m_currentLabel->setBounds(leftBounds.removeFromTop(30));
        m_currentPerfLabel->setBounds(leftBounds.removeFromTop(20));
        leftBounds.removeFromTop(10);
        m_currentCard->setBounds(leftBounds.withHeight(420).withWidth(140)
                                          .withCentre(leftBounds.getCentre()));
        
        // Right side - optimized version
        auto rightBounds = bounds.reduced(20);
        m_optimizedLabel->setBounds(rightBounds.removeFromTop(30));
        m_optimizedPerfLabel->setBounds(rightBounds.removeFromTop(20));
        rightBounds.removeFromTop(10);
        m_optimizedCard->setBounds(rightBounds.withHeight(420).withWidth(140)
                                             .withCentre(rightBounds.getCentre()));
    }
    
private:
    void timerCallback() override {
        // Animate sliders to show continuous repainting
        static float phase = 0.0f;
        phase += 0.05f;
        
        float value = (std::sin(phase) + 1.0f) * 0.5f;
        
        // Update both versions
        if (auto* slider = m_currentCard->getPitchSlider()) {
            slider->setValue(value);
        }
        if (auto* slider = m_optimizedCard->m_pitchSlider.get()) {
            slider->setValue(value);
        }
        
        // Toggle active state periodically
        bool active = (static_cast<int>(phase * 0.5f) % 2) == 0;
        m_currentCard->setActive(active);
        m_optimizedCard->setActive(active);
    }
    
    void measureRenderTimes() {
        // Create temporary graphics contexts to measure render time
        juce::Image testImage(juce::Image::ARGB, 140, 420, true);
        
        // Measure current version
        {
            juce::Graphics g(testImage);
            auto startTime = juce::Time::getHighResolutionTicks();
            
            // Render 10 times for more accurate measurement
            for (int i = 0; i < 10; ++i) {
                m_currentCard->paint(g);
            }
            
            auto endTime = juce::Time::getHighResolutionTicks();
            double ms = juce::Time::highResolutionTicksToSeconds(endTime - startTime) * 100.0; // ms for 10 renders
            m_currentPerfLabel->setText("Render time: " + juce::String(ms, 2) + " ms", 
                                       juce::dontSendNotification);
        }
        
        // Measure optimized version
        {
            juce::Graphics g(testImage);
            auto startTime = juce::Time::getHighResolutionTicks();
            
            // Render 10 times
            for (int i = 0; i < 10; ++i) {
                m_optimizedCard->paint(g);
            }
            
            auto endTime = juce::Time::getHighResolutionTicks();
            double ms = juce::Time::highResolutionTicksToSeconds(endTime - startTime) * 100.0;
            m_optimizedPerfLabel->setText("Render time: " + juce::String(ms, 2) + " ms", 
                                         juce::dontSendNotification);
        }
    }
    
    std::unique_ptr<StageCard> m_currentCard;
    std::unique_ptr<Optimized::OptimizedStageCard> m_optimizedCard;
    
    std::unique_ptr<juce::Label> m_currentLabel;
    std::unique_ptr<juce::Label> m_optimizedLabel;
    std::unique_ptr<juce::Label> m_currentPerfLabel;
    std::unique_ptr<juce::Label> m_optimizedPerfLabel;
    
public:
    // Make sliders accessible for animation
    friend class OptimizedStageCard;
};

} // namespace HAM::UI