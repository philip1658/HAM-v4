/*
  ==============================================================================

    TransportControl.cpp
    Transport control UI implementation

  ==============================================================================
*/

#include "TransportControl.h"
#include "../../Infrastructure/Audio/HAMAudioProcessor.h"

namespace HAM {
namespace UI {

//==============================================================================
TransportControl::TransportControl()
{
    // Configure Play button
    m_playButton.setButtonText("PLAY");
    m_playButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF00FF00).withAlpha(0.2f));
    m_playButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFF00FF00));
    m_playButton.onClick = [this] { onPlayClicked(); };
    addAndMakeVisible(m_playButton);
    
    // Configure Stop button
    m_stopButton.setButtonText("STOP");
    m_stopButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFFFF0000).withAlpha(0.2f));
    m_stopButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFFF0000));
    m_stopButton.onClick = [this] { onStopClicked(); };
    addAndMakeVisible(m_stopButton);
    
    // Configure Pause button
    m_pauseButton.setButtonText("PAUSE");
    m_pauseButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFFFFFF00).withAlpha(0.2f));
    m_pauseButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFFFFF00));
    m_pauseButton.onClick = [this] { onPauseClicked(); };
    addAndMakeVisible(m_pauseButton);
    
    // Configure BPM slider
    m_bpmSlider.setRange(20.0, 300.0, 0.1);
    m_bpmSlider.setValue(120.0);
    m_bpmSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    m_bpmSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 60, 20);
    m_bpmSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xFFFFFFFF));
    m_bpmSlider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xFF202020));
    m_bpmSlider.setColour(juce::Slider::trackColourId, juce::Colour(0xFF00FFFF));
    m_bpmSlider.onValueChange = [this] { onBpmChanged(); };
    addAndMakeVisible(m_bpmSlider);
    
    // Configure BPM label
    m_bpmLabel.setText("BPM:", juce::dontSendNotification);
    m_bpmLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFFFFFFF));
    m_bpmLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(m_bpmLabel);
    
    // Configure position label
    m_positionLabel.setText("1:1:0", juce::dontSendNotification);
    m_positionLabel.setColour(juce::Label::textColourId, juce::Colour(0xFF00FFFF));
    m_positionLabel.setJustificationType(juce::Justification::centred);
    m_positionLabel.setFont(juce::Font(20.0f, juce::Font::bold));
    addAndMakeVisible(m_positionLabel);
    
    // Start timer for position updates (10Hz)
    startTimer(100);
}

TransportControl::~TransportControl()
{
    stopTimer();
}

//==============================================================================
void TransportControl::setAudioProcessor(HAMAudioProcessor* processor)
{
    m_processor = processor;
    
    if (m_processor)
    {
        // Get initial BPM
        updateBPM(m_processor->getBPM());
    }
}

//==============================================================================
void TransportControl::paint(juce::Graphics& g)
{
    // Background
    g.fillAll(juce::Colour(0xFF1A1A1A));
    
    // Border
    g.setColour(juce::Colour(0xFF3A3A3A));
    g.drawRect(getLocalBounds(), 1);
    
    // Separator lines
    g.setColour(juce::Colour(0xFF2A2A2A));
    g.drawVerticalLine(getWidth() / 3, 10, getHeight() - 10);
    g.drawVerticalLine(2 * getWidth() / 3, 10, getHeight() - 10);
}

void TransportControl::resized()
{
    auto bounds = getLocalBounds().reduced(10);
    auto buttonArea = bounds.removeFromLeft(bounds.getWidth() / 3);
    auto bpmArea = bounds.removeFromLeft(bounds.getWidth() / 2);
    auto positionArea = bounds;
    
    // Layout buttons
    auto buttonWidth = buttonArea.getWidth() / 3 - 5;
    m_playButton.setBounds(buttonArea.removeFromLeft(buttonWidth));
    buttonArea.removeFromLeft(5);
    m_stopButton.setBounds(buttonArea.removeFromLeft(buttonWidth));
    buttonArea.removeFromLeft(5);
    m_pauseButton.setBounds(buttonArea);
    
    // Layout BPM controls
    m_bpmLabel.setBounds(bpmArea.removeFromLeft(40));
    m_bpmSlider.setBounds(bpmArea.reduced(5, 10));
    
    // Layout position display
    m_positionLabel.setBounds(positionArea);
}

//==============================================================================
void TransportControl::timerCallback()
{
    updateTransportState();
    updatePositionDisplay();
}

void TransportControl::updateTransportState()
{
    if (!m_processor)
        return;
    
    bool isPlaying = m_processor->isPlaying();
    
    if (isPlaying != m_isPlaying.load())
    {
        m_isPlaying.store(isPlaying);
        
        // Update button states
        m_playButton.setEnabled(!isPlaying);
        m_stopButton.setEnabled(isPlaying);
        m_pauseButton.setEnabled(isPlaying);
        
        // Update button colors
        if (isPlaying)
        {
            m_playButton.setColour(juce::TextButton::buttonColourId, 
                                  juce::Colour(0xFF00FF00).withAlpha(0.8f));
        }
        else
        {
            m_playButton.setColour(juce::TextButton::buttonColourId, 
                                  juce::Colour(0xFF00FF00).withAlpha(0.2f));
        }
    }
}

void TransportControl::updateBPM(float bpm)
{
    m_currentBpm.store(bpm);
    m_bpmSlider.setValue(bpm, juce::dontSendNotification);
}

void TransportControl::updatePositionDisplay()
{
    if (!m_processor)
        return;
    
    // Get position directly from processor
    int bar = m_processor->getCurrentBar();
    int beat = m_processor->getCurrentBeat();
    int pulse = m_processor->getCurrentPulse();
    
    // Format as "bar:beat:pulse"
    juce::String positionText = juce::String(bar + 1) + ":" + 
                                juce::String(beat + 1) + ":" + 
                                juce::String(pulse);
    
    m_positionLabel.setText(positionText, juce::dontSendNotification);
}

//==============================================================================
void TransportControl::onPlayClicked()
{
    if (m_processor)
    {
        DBG("TransportControl: Play button clicked");
        m_processor->play();
        m_isPlaying.store(true);
        updateTransportState();
    }
}

void TransportControl::onStopClicked()
{
    if (m_processor)
    {
        DBG("TransportControl: Stop button clicked");
        m_processor->stop();
        m_isPlaying.store(false);
        updateTransportState();
    }
}

void TransportControl::onPauseClicked()
{
    if (m_processor)
    {
        DBG("TransportControl: Pause button clicked");
        m_processor->pause();
        updateTransportState();
    }
}

void TransportControl::onBpmChanged()
{
    if (m_processor)
    {
        float newBpm = static_cast<float>(m_bpmSlider.getValue());
        m_processor->setBPM(newBpm);
        m_currentBpm.store(newBpm);
    }
}

} // namespace UI
} // namespace HAM