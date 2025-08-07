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
    // TODO: Initialize HAM sequencer components here
    // For now, just set a default size
    setSize(1400, 900);
}

MainComponent::~MainComponent()
{
}

//==============================================================================
void MainComponent::paint(juce::Graphics& g)
{
    // Dark background matching HAM design
    g.fillAll(juce::Colour(0xFF000000));
    
    // Temporary placeholder text
    g.setColour(juce::Colour(0xFF00FF88));
    g.setFont(juce::Font(32.0f));
    g.drawText("HAM Sequencer", getLocalBounds().reduced(20, 20), 
               juce::Justification::centredTop);
    
    g.setColour(juce::Colour(0xFF888888));
    g.setFont(juce::Font(16.0f));
    g.drawText("Ready for UI implementation", 
               getLocalBounds().reduced(20, 80),
               juce::Justification::centredTop);
}

void MainComponent::resized()
{
    // TODO: Layout HAM sequencer components here
}