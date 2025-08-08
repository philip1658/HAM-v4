// SPDX-License-Identifier: MIT
#pragma once

#include <JuceHeader.h>
#include "../Core/BaseComponent.h"
#include "../Core/DesignSystem.h"
#include "../ViewModels/StageViewModel.h"
#include "../Components/ModernSlider.h"
#include <memory>

namespace HAM::UI {

// ==========================================
// Stage Card - 140x420px with 2x2 Slider Grid
// ==========================================
class StageCard : public AnimatedComponent, public juce::ChangeListener {
public:
    StageCard() {
        // Create sliders (vertical, no thumb)
        m_pitchSlider = std::make_unique<ModernSlider>(true);
        m_pitchSlider->setLabel("PITCH");
        m_pitchSlider->setRange(0, 127, 1);
        addAndMakeVisible(m_pitchSlider.get());
        
        m_pulseSlider = std::make_unique<ModernSlider>(true);
        m_pulseSlider->setLabel("PULSE");
        m_pulseSlider->setRange(1, 8, 1);
        addAndMakeVisible(m_pulseSlider.get());
        
        m_velocitySlider = std::make_unique<ModernSlider>(true);
        m_velocitySlider->setLabel("VEL");
        m_velocitySlider->setRange(0, 127, 1);
        addAndMakeVisible(m_velocitySlider.get());
        
        m_gateSlider = std::make_unique<ModernSlider>(true);
        m_gateSlider->setLabel("GATE");
        m_gateSlider->setRange(0.0f, 1.0f, 0.01f);
        addAndMakeVisible(m_gateSlider.get());
        
        // HAM button
        m_hamButton = std::make_unique<juce::TextButton>("HAM");
        m_hamButton->setColour(juce::TextButton::buttonColourId, 
                               DesignSystem::Colors::getColor(DesignSystem::Colors::ACCENT_PRIMARY));
        addAndMakeVisible(m_hamButton.get());
        
        // Set default size
        setSize(DesignSystem::Dimensions::STAGE_CARD_WIDTH, 
                DesignSystem::Dimensions::STAGE_CARD_HEIGHT);
        
        // Setup callbacks
        setupCallbacks();
    }
    
    ~StageCard() override {
        if (m_viewModel) {
            m_viewModel->removeChangeListener(this);
        }
    }
    
    // Bind to ViewModel
    void bindToViewModel(StageViewModel* viewModel) {
        if (m_viewModel) {
            m_viewModel->removeChangeListener(this);
        }
        
        m_viewModel = viewModel;
        
        if (m_viewModel) {
            m_viewModel->addChangeListener(this);
            updateFromViewModel();
        }
    }
    
    // Paint override
    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();
        
        // Background animation for playing state
        if (m_viewModel && m_viewModel->isPlaying()) {
            float alpha = m_playAnimation.current * 0.3f;
            g.setColour(DesignSystem::Colors::getColor(DesignSystem::Colors::ACCENT_PRIMARY).withAlpha(alpha));
            g.fillRoundedRectangle(bounds, scaled(DesignSystem::Dimensions::CORNER_RADIUS));
        }
        
        // Card background
        bool isRaised = m_viewModel && (m_viewModel->isSelected() || m_viewModel->isActive());
        drawPanel(g, bounds, isRaised);
        
        // Active/Selected state outline
        if (m_viewModel) {
            if (m_viewModel->isActive()) {
                g.setColour(DesignSystem::Colors::getColor(DesignSystem::Colors::ACCENT_PRIMARY));
                g.drawRoundedRectangle(bounds.reduced(1), 
                                       scaled(DesignSystem::Dimensions::CORNER_RADIUS), 
                                       scaled(2));
            } else if (m_viewModel->isSelected()) {
                g.setColour(DesignSystem::Colors::getColor(DesignSystem::Colors::ACCENT_PRIMARY).withAlpha(0.5f));
                g.drawRoundedRectangle(bounds.reduced(1), 
                                       scaled(DesignSystem::Dimensions::CORNER_RADIUS), 
                                       scaled(1));
            }
        }
        
        // Stage number
        g.setColour(DesignSystem::Colors::getColor(DesignSystem::Colors::TEXT_MUTED));
        g.setFont(DesignSystem::Typography::getHeaderFont().withHeight(scaled(20)));
        
        juce::String stageText = m_viewModel ? juce::String(m_viewModel->getStageIndex() + 1) : "?";
        g.drawText(stageText, bounds.removeFromTop(scaled(40)), juce::Justification::centred);
        
        // Skip indicator
        if (m_viewModel && m_viewModel->isSkipped()) {
            g.setColour(DesignSystem::Colors::getColor(DesignSystem::Colors::TEXT_DIM));
            g.drawLine(bounds.getX() + scaled(10), bounds.getCentreY(),
                      bounds.getRight() - scaled(10), bounds.getCentreY(), scaled(2));
        }
        
        // Note name display
        if (m_viewModel) {
            auto noteNameBounds = bounds.removeFromTop(scaled(30));
            g.setColour(DesignSystem::Colors::getColor(DesignSystem::Colors::TEXT_PRIMARY));
            g.setFont(DesignSystem::Typography::getLargeFont());
            g.drawText(m_viewModel->getNoteName(), noteNameBounds, juce::Justification::centred);
        }
    }
    
    // Resize override
    void resized() override {
        auto bounds = getLocalBounds();
        
        // Stage number and note name area
        bounds.removeFromTop(scaled(70));
        
        // 2x2 Grid for sliders
        auto sliderArea = bounds.removeFromTop(scaled(300));
        int sliderWidth = sliderArea.getWidth() / 2;
        int sliderHeight = sliderArea.getHeight() / 2;
        
        // Top row: PITCH and PULSE
        m_pitchSlider->setBounds(sliderArea.removeFromLeft(sliderWidth).reduced(scaled(4)));
        m_pulseSlider->setBounds(sliderArea.removeFromLeft(sliderWidth).reduced(scaled(4)));
        
        // Reset for bottom row
        sliderArea = bounds.removeFromTop(0);
        sliderArea.setBounds(getLocalBounds().getX(), 
                            scaled(70) + sliderHeight,
                            getWidth(), sliderHeight);
        
        // Bottom row: VEL and GATE
        m_velocitySlider->setBounds(sliderArea.removeFromLeft(sliderWidth).reduced(scaled(4)));
        m_gateSlider->setBounds(sliderArea.removeFromLeft(sliderWidth).reduced(scaled(4)));
        
        // HAM button at bottom
        auto buttonArea = getLocalBounds().removeFromBottom(scaled(40));
        m_hamButton->setBounds(buttonArea.reduced(scaled(20), scaled(5)));
    }
    
    // Mouse handling
    void mouseDown(const juce::MouseEvent& e) override {
        if (m_viewModel) {
            if (e.mods.isCommandDown()) {
                // Toggle selection with cmd
                m_viewModel->setSelected(!m_viewModel->isSelected());
            } else {
                // Single selection
                m_viewModel->setSelected(true);
            }
            
            if (onStageClicked) {
                onStageClicked(m_viewModel->getStageIndex(), e.mods);
            }
        }
    }
    
    void mouseDoubleClick(const juce::MouseEvent&) override {
        if (onHamEditorRequested && m_viewModel) {
            onHamEditorRequested(m_viewModel->getStageIndex());
        }
    }
    
    // Callbacks
    std::function<void(int stageIndex, const juce::ModifierKeys& mods)> onStageClicked;
    std::function<void(int stageIndex)> onHamEditorRequested;
    std::function<void(int stageIndex, const juce::String& param, float value)> onParameterChanged;
    
protected:
    // Animation update
    void updateAnimations(float deltaTime) override {
        if (m_viewModel && m_viewModel->isPlaying()) {
            m_playAnimation.setTarget(1.0f, DesignSystem::Animation::DURATION_FAST);
        } else {
            m_playAnimation.setTarget(0.0f, DesignSystem::Animation::DURATION_SLOW);
        }
        
        m_playAnimation.update(deltaTime);
    }
    
    bool hasActiveAnimations() const override {
        return m_playAnimation.isAnimating();
    }
    
private:
    // Components
    std::unique_ptr<ModernSlider> m_pitchSlider;
    std::unique_ptr<ModernSlider> m_pulseSlider;
    std::unique_ptr<ModernSlider> m_velocitySlider;
    std::unique_ptr<ModernSlider> m_gateSlider;
    std::unique_ptr<juce::TextButton> m_hamButton;
    
    // State
    StageViewModel* m_viewModel = nullptr;
    AnimationState m_playAnimation;
    
    // ChangeListener callback
    void changeListenerCallback(juce::ChangeBroadcaster*) override {
        updateFromViewModel();
        repaint();
    }
    
    // Update UI from ViewModel
    void updateFromViewModel() {
        if (!m_viewModel) return;
        
        m_pitchSlider->setValue(m_viewModel->getPitch());
        m_pulseSlider->setValue(m_viewModel->getPulseCount());
        m_velocitySlider->setValue(m_viewModel->getVelocity());
        m_gateSlider->setValue(m_viewModel->getGate());
        
        // Update colors based on state
        if (m_viewModel->isActive()) {
            auto color = DesignSystem::Colors::getColor(DesignSystem::Colors::ACCENT_PRIMARY);
            m_pitchSlider->setTrackColor(color);
            m_pulseSlider->setTrackColor(color);
            m_velocitySlider->setTrackColor(color);
            m_gateSlider->setTrackColor(color);
        } else {
            auto color = DesignSystem::Colors::getColor(DesignSystem::Colors::ACCENT_BLUE);
            m_pitchSlider->setTrackColor(color);
            m_pulseSlider->setTrackColor(color);
            m_velocitySlider->setTrackColor(color);
            m_gateSlider->setTrackColor(color);
        }
    }
    
    // Setup slider callbacks
    void setupCallbacks() {
        m_pitchSlider->onValueChange = [this](float value) {
            if (m_viewModel) {
                m_viewModel->setPitch(static_cast<int>(value));
                if (onParameterChanged) {
                    onParameterChanged(m_viewModel->getStageIndex(), "pitch", value);
                }
            }
        };
        
        m_pulseSlider->onValueChange = [this](float value) {
            if (m_viewModel) {
                m_viewModel->setPulseCount(static_cast<int>(value));
                if (onParameterChanged) {
                    onParameterChanged(m_viewModel->getStageIndex(), "pulse", value);
                }
            }
        };
        
        m_velocitySlider->onValueChange = [this](float value) {
            if (m_viewModel) {
                m_viewModel->setVelocity(static_cast<int>(value));
                if (onParameterChanged) {
                    onParameterChanged(m_viewModel->getStageIndex(), "velocity", value);
                }
            }
        };
        
        m_gateSlider->onValueChange = [this](float value) {
            if (m_viewModel) {
                m_viewModel->setGate(value);
                if (onParameterChanged) {
                    onParameterChanged(m_viewModel->getStageIndex(), "gate", value);
                }
            }
        };
        
        m_hamButton->onClick = [this]() {
            if (onHamEditorRequested && m_viewModel) {
                onHamEditorRequested(m_viewModel->getStageIndex());
            }
        };
    }
};

} // namespace HAM::UI