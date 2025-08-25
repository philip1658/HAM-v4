/*
  ==============================================================================

    TransportControl.h
    Transport control UI with Play/Stop/Pause buttons and BPM display

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <atomic>
#include <functional>

namespace HAM {

// Forward declarations
class HAMAudioProcessor;

namespace UI {

class TransportControl : public juce::Component,
                        public juce::Timer
{
public:
    //==============================================================================
    TransportControl();
    ~TransportControl() override;
    
    //==============================================================================
    // Set the audio processor to control
    void setAudioProcessor(HAMAudioProcessor* processor);
    
    //==============================================================================
    // UI refresh
    void updateTransportState();
    void updateBPM(float bpm);
    
    //==============================================================================
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;
    
private:
    //==============================================================================
    // UI Components
    juce::TextButton m_playButton;
    juce::TextButton m_stopButton;
    juce::TextButton m_pauseButton;
    juce::Label m_bpmLabel;
    juce::Slider m_bpmSlider;
    juce::Label m_positionLabel;
    
    // State
    HAMAudioProcessor* m_processor = nullptr;
    std::atomic<bool> m_isPlaying{false};
    std::atomic<float> m_currentBpm{120.0f};
    
    // Button handlers
    void onPlayClicked();
    void onStopClicked();
    void onPauseClicked();
    void onBpmChanged();
    
    // Update position display
    void updatePositionDisplay();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransportControl)
};

} // namespace UI
} // namespace HAM