/*
  ==============================================================================

    MainComponent.cpp
    Main component implementation for HAM sequencer

  ==============================================================================
*/

#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
{
    // Set up title label
    m_titleLabel.setText("HAM Sequencer", juce::dontSendNotification);
    m_titleLabel.setFont(juce::Font(36.0f, juce::Font::bold));
    m_titleLabel.setColour(juce::Label::textColourId, juce::Colour(Colors::ACCENT_PRIMARY));
    m_titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(m_titleLabel);
    
    // Set up status label
    m_statusLabel.setText("Phase 1.1 - Project Setup", juce::dontSendNotification);
    m_statusLabel.setFont(juce::Font(16.0f));
    m_statusLabel.setColour(juce::Label::textColourId, juce::Colour(Colors::TEXT_SECONDARY));
    m_statusLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(m_statusLabel);
    
    // Set up test button
    m_testButton.setColour(juce::TextButton::buttonColourId, juce::Colour(Colors::BACKGROUND_MID));
    m_testButton.setColour(juce::TextButton::textColourOffId, juce::Colour(Colors::TEXT_PRIMARY));
    m_testButton.onClick = [this]() {
        m_statusLabel.setText("Audio System: OK | JUCE " + juce::String(JUCE_MAJOR_VERSION) + "." + 
                             juce::String(JUCE_MINOR_VERSION) + "." + 
                             juce::String(JUCE_BUILDNUMBER), 
                             juce::dontSendNotification);
        
        // Trigger animation
        m_animationPhase = 1.0f;
    };
    addAndMakeVisible(m_testButton);
    
    // Set component size
    setSize(1400, 900);
    
    // Start timer for animations (30 FPS)
    startTimerHz(30);
}

MainComponent::~MainComponent()
{
    stopTimer();
}

//==============================================================================
void MainComponent::paint(juce::Graphics& g)
{
    // Dark background
    g.fillAll(juce::Colour(Colors::BACKGROUND_DARK));
    
    // Draw animated accent circle (visual test)
    if (m_animationPhase > 0.01f)
    {
        auto bounds = getLocalBounds();
        auto center = bounds.getCentre().toFloat();
        
        float radius = 100.0f * m_animationPhase;
        float alpha = m_animationPhase * 0.3f;
        
        g.setColour(juce::Colour(Colors::ACCENT_PRIMARY).withAlpha(alpha));
        g.fillEllipse(center.x - radius, center.y - radius, radius * 2, radius * 2);
    }
    
    // Draw grid pattern (visual test for rendering)
    g.setColour(juce::Colour(Colors::BACKGROUND_MID).withAlpha(0.3f));
    
    const int gridSize = 50;
    for (int x = 0; x < getWidth(); x += gridSize)
    {
        g.drawVerticalLine(x, 0, getHeight());
    }
    for (int y = 0; y < getHeight(); y += gridSize)
    {
        g.drawHorizontalLine(y, 0, getWidth());
    }
    
    // Version info
    g.setColour(juce::Colour(Colors::TEXT_SECONDARY).withAlpha(0.5f));
    g.setFont(12.0f);
    g.drawText("HAM v0.1.0 - Build Test", getLocalBounds().removeFromBottom(20), 
               juce::Justification::centredRight);
}

void MainComponent::resized()
{
    auto bounds = getLocalBounds();
    
    // Layout components
    m_titleLabel.setBounds(bounds.removeFromTop(100).reduced(20));
    m_statusLabel.setBounds(bounds.removeFromTop(50).reduced(20));
    
    // Center test button
    auto buttonBounds = bounds.removeFromTop(100).reduced(20);
    m_testButton.setBounds(buttonBounds.withSizeKeepingCentre(200, 40));
}

void MainComponent::timerCallback()
{
    // Animate the visual test
    if (m_animationPhase > 0.01f)
    {
        m_animationPhase *= 0.95f; // Decay animation
        repaint();
    }
}