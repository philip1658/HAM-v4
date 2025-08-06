/*
  ==============================================================================

    MainComponent.h
    Main component for HAM sequencer

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
class MainComponent : public juce::Component,
                      public juce::Timer
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    //==============================================================================
    void paint(juce::Graphics&) override;
    void resized() override;
    
    // Timer callback for UI updates (30 FPS)
    void timerCallback() override;

private:
    //==============================================================================
    // Visual test elements for Phase 1.1
    juce::Label m_titleLabel;
    juce::Label m_statusLabel;
    juce::TextButton m_testButton{"Test Audio System"};
    
    // Test animation for visual feedback
    float m_animationPhase = 0.0f;
    
    // Color scheme
    struct Colors {
        static constexpr juce::uint32 BACKGROUND_DARK = 0xFF0A0A0A;
        static constexpr juce::uint32 BACKGROUND_MID = 0xFF1A1A1A;
        static constexpr juce::uint32 ACCENT_PRIMARY = 0xFF00FF88;
        static constexpr juce::uint32 TEXT_PRIMARY = 0xFFFFFFFF;
        static constexpr juce::uint32 TEXT_SECONDARY = 0xFFB0B0B0;
    };
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};