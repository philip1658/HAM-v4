/*
  ==============================================================================

    MixerView.h
    Complete mixer interface with plugin management, volume/pan controls
    Integrates plugin browser and window management

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../../Infrastructure/Audio/HAMAudioProcessor.h"
#include "../../Infrastructure/Plugins/PluginWindowManager.h"
#include "PluginBrowser.h"
#include <vector>
#include <memory>

namespace HAM::UI
{

/**
 * TrackStrip - Single channel strip in the mixer
 * Contains volume, pan, mute, solo, and plugin slots
 */
class TrackStrip : public juce::Component,
                   public juce::Slider::Listener,
                   public juce::Button::Listener
{
public:
    TrackStrip(int trackIndex, HAMAudioProcessor& processor)
        : m_trackIndex(trackIndex)
        , m_processor(processor)
    {
        // Track name label
        m_trackLabel.setText("Track " + juce::String(trackIndex + 1), juce::dontSendNotification);
        m_trackLabel.setJustificationType(juce::Justification::centred);
        m_trackLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(m_trackLabel);
        
        // Volume fader
        m_volumeSlider.setSliderStyle(juce::Slider::LinearVertical);
        m_volumeSlider.setRange(0.0, 1.0, 0.01);
        m_volumeSlider.setValue(1.0);
        m_volumeSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
        m_volumeSlider.setColour(juce::Slider::backgroundColourId, juce::Colour(0xff2a2a2a));
        m_volumeSlider.setColour(juce::Slider::trackColourId, juce::Colour(0xff5a5a5a));
        m_volumeSlider.setColour(juce::Slider::thumbColourId, getTrackColor(trackIndex));
        m_volumeSlider.addListener(this);
        addAndMakeVisible(m_volumeSlider);
        
        // Pan knob
        m_panSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        m_panSlider.setRange(-1.0, 1.0, 0.01);
        m_panSlider.setValue(0.0);
        m_panSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        m_panSlider.setColour(juce::Slider::rotarySliderFillColourId, getTrackColor(trackIndex));
        m_panSlider.addListener(this);
        addAndMakeVisible(m_panSlider);
        
        // Mute button
        m_muteButton.setButtonText("M");
        m_muteButton.setToggleable(true);
        m_muteButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff3a3a3a));
        m_muteButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffff3030));
        m_muteButton.addListener(this);
        addAndMakeVisible(m_muteButton);
        
        // Solo button
        m_soloButton.setButtonText("S");
        m_soloButton.setToggleable(true);
        m_soloButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff3a3a3a));
        m_soloButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffffff30));
        m_soloButton.addListener(this);
        addAndMakeVisible(m_soloButton);
        
        // Instrument slot
        m_instrumentSlot.setButtonText("< No Instrument >");
        m_instrumentSlot.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2a2a2a));
        m_instrumentSlot.onClick = [this] { onInstrumentSlotClicked(); };
        addAndMakeVisible(m_instrumentSlot);
        
        // Effect slots (3 slots initially)
        for (int i = 0; i < 3; ++i)
        {
            auto effectSlot = std::make_unique<juce::TextButton>();
            effectSlot->setButtonText("< Empty >");
            effectSlot->setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1a1a1a));
            effectSlot->onClick = [this, i] { onEffectSlotClicked(i); };
            addAndMakeVisible(effectSlot.get());
            m_effectSlots.push_back(std::move(effectSlot));
        }
        
        // Add effect button
        m_addEffectButton.setButtonText("+");
        m_addEffectButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2a2a2a));
        m_addEffectButton.onClick = [this] { onAddEffectClicked(); };
        addAndMakeVisible(m_addEffectButton);
        
        setSize(100, 500);
    }
    
    void paint(juce::Graphics& g) override
    {
        // Background
        g.fillAll(juce::Colour(0xff1e1e1e));
        
        // Track color strip at top
        g.setColour(getTrackColor(m_trackIndex));
        g.fillRect(0, 0, getWidth(), 3);
        
        // Divider lines
        g.setColour(juce::Colour(0xff3a3a3a));
        g.drawLine(0, 40, getWidth(), 40);
        g.drawLine(0, 280, getWidth(), 280);
    }
    
    void resized() override
    {
        auto bounds = getLocalBounds();
        
        // Track label
        m_trackLabel.setBounds(bounds.removeFromTop(40).reduced(5));
        
        // Mute/Solo buttons
        auto buttonArea = bounds.removeFromTop(30);
        m_muteButton.setBounds(buttonArea.removeFromLeft(getWidth() / 2).reduced(2));
        m_soloButton.setBounds(buttonArea.reduced(2));
        
        // Pan knob
        m_panSlider.setBounds(bounds.removeFromTop(60).reduced(10));
        
        // Volume fader
        m_volumeSlider.setBounds(bounds.removeFromTop(140).reduced(10, 0));
        
        bounds.removeFromTop(10); // Spacing
        
        // Instrument slot
        m_instrumentSlot.setBounds(bounds.removeFromTop(30).reduced(5, 2));
        
        // Effect slots
        for (auto& slot : m_effectSlots)
        {
            slot->setBounds(bounds.removeFromTop(25).reduced(5, 2));
        }
        
        // Add effect button
        m_addEffectButton.setBounds(bounds.removeFromTop(25).reduced(5, 2));
    }
    
    // Slider listener
    void sliderValueChanged(juce::Slider* slider) override
    {
        if (slider == &m_volumeSlider)
        {
            // TODO: Send volume change to processor
            // m_processor.setTrackVolume(m_trackIndex, slider->getValue());
        }
        else if (slider == &m_panSlider)
        {
            // TODO: Send pan change to processor
            // m_processor.setTrackPan(m_trackIndex, slider->getValue());
        }
    }
    
    // Button listener
    void buttonClicked(juce::Button* button) override
    {
        if (button == &m_muteButton)
        {
            if (auto* track = m_processor.getTrack(m_trackIndex))
            {
                track->setMuted(button->getToggleState());
            }
        }
        else if (button == &m_soloButton)
        {
            if (auto* track = m_processor.getTrack(m_trackIndex))
            {
                track->setSolo(button->getToggleState());
            }
        }
    }
    
    void updatePluginDisplay()
    {
        // Update instrument slot display
        // TODO: Get actual plugin name from processor
        // if (auto* plugin = m_processor.getInstrumentPlugin(m_trackIndex))
        // {
        //     m_instrumentSlot.setButtonText(plugin->getName());
        // }
    }
    
private:
    void onInstrumentSlotClicked()
    {
        // Check if plugin already loaded
        auto& windowManager = PluginWindowManager::getInstance();
        
        if (windowManager.isWindowOpen(m_trackIndex, -1))
        {
            // Plugin window exists, just bring to front
            windowManager.openPluginWindow(m_trackIndex, -1, nullptr, "");
        }
        else
        {
            // Show plugin browser to select instrument
            showPluginBrowser(true);
        }
    }
    
    void onEffectSlotClicked(int slotIndex)
    {
        auto& windowManager = PluginWindowManager::getInstance();
        
        if (windowManager.isWindowOpen(m_trackIndex, slotIndex))
        {
            // Effect window exists, bring to front
            windowManager.openPluginWindow(m_trackIndex, slotIndex, nullptr, "");
        }
        else
        {
            // Show plugin browser to select effect
            m_selectedEffectSlot = slotIndex;
            showPluginBrowser(false);
        }
    }
    
    void onAddEffectClicked()
    {
        // Add new effect slot and show browser
        m_selectedEffectSlot = m_effectSlots.size();
        showPluginBrowser(false);
    }
    
    void showPluginBrowser(bool forInstrument)
    {
        // Create plugin browser window
        auto browserWindow = std::make_unique<juce::DocumentWindow>(
            forInstrument ? "Select Instrument" : "Select Effect",
            juce::Colours::darkgrey,
            juce::DocumentWindow::allButtons);
        
        auto browser = std::make_unique<PluginBrowser>();
        
        // Set callback for when plugin is chosen
        browser->onPluginChosen = [this, forInstrument](const juce::PluginDescription& desc)
        {
            // Load the plugin
            bool success = m_processor.loadPlugin(m_trackIndex, desc, forInstrument);
            
            if (success)
            {
                // Update display
                updatePluginDisplay();
                
                // Open plugin window
                if (forInstrument)
                {
                    m_processor.showPluginEditor(m_trackIndex, -1);
                }
                else
                {
                    m_processor.showPluginEditor(m_trackIndex, m_selectedEffectSlot);
                }
            }
            
            // Close browser window
            if (auto* window = dynamic_cast<juce::DocumentWindow*>(
                    juce::Component::getCurrentlyModalComponent()))
            {
                window->exitModalState(0);
            }
        };
        
        browserWindow->setContentOwned(browser.release(), true);
        browserWindow->centreWithSize(600, 400);
        browserWindow->setVisible(true);
        // Note: Modal loops are not recommended in modern JUCE
        // The window will stay open and the callback will handle selection
    }
    
    juce::Colour getTrackColor(int index) const
    {
        const std::vector<juce::Colour> trackColors = {
            juce::Colour(0xff00ffaa), // Mint
            juce::Colour(0xff00aaff), // Cyan
            juce::Colour(0xffff00aa), // Magenta
            juce::Colour(0xffffaa00), // Orange
            juce::Colour(0xffaa00ff), // Purple
            juce::Colour(0xff00ff00), // Green
            juce::Colour(0xffff0055), // Red
            juce::Colour(0xff55aaff)  // Blue
        };
        
        return trackColors[index % trackColors.size()];
    }
    
    int m_trackIndex;
    int m_selectedEffectSlot = 0;
    HAMAudioProcessor& m_processor;
    
    juce::Label m_trackLabel;
    juce::Slider m_volumeSlider;
    juce::Slider m_panSlider;
    juce::TextButton m_muteButton;
    juce::TextButton m_soloButton;
    juce::TextButton m_instrumentSlot;
    std::vector<std::unique_ptr<juce::TextButton>> m_effectSlots;
    juce::TextButton m_addEffectButton;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackStrip)
};

/**
 * MixerView - Complete mixing console with plugin management
 * Shows all tracks with volume/pan/plugins
 */
class MixerView : public juce::Component
{
public:
    MixerView(HAMAudioProcessor& processor)
        : m_processor(processor)
    {
        // Create track strips
        updateTrackCount();
        
        // Master section
        m_masterLabel.setText("MASTER", juce::dontSendNotification);
        m_masterLabel.setJustificationType(juce::Justification::centred);
        m_masterLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(m_masterLabel);
        
        m_masterVolume.setSliderStyle(juce::Slider::LinearVertical);
        m_masterVolume.setRange(0.0, 1.0, 0.01);
        m_masterVolume.setValue(1.0);
        m_masterVolume.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
        m_masterVolume.setColour(juce::Slider::backgroundColourId, juce::Colour(0xff2a2a2a));
        m_masterVolume.setColour(juce::Slider::trackColourId, juce::Colour(0xff5a5a5a));
        m_masterVolume.setColour(juce::Slider::thumbColourId, juce::Colours::white);
        addAndMakeVisible(m_masterVolume);
        
        // Scroll viewport for tracks
        m_viewport.setViewedComponent(&m_trackContainer, false);
        m_viewport.setScrollBarsShown(false, true);
        addAndMakeVisible(m_viewport);
        
        setSize(800, 500);
    }
    
    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff151515));
        
        // Draw separator before master
        g.setColour(juce::Colour(0xff3a3a3a));
        g.drawLine(getWidth() - 120, 0, getWidth() - 120, getHeight(), 2);
    }
    
    void resized() override
    {
        auto bounds = getLocalBounds();
        
        // Master section on the right
        auto masterBounds = bounds.removeFromRight(120);
        m_masterLabel.setBounds(masterBounds.removeFromTop(40).reduced(5));
        m_masterVolume.setBounds(masterBounds.removeFromTop(200).reduced(20, 10));
        
        // Tracks viewport takes remaining space
        m_viewport.setBounds(bounds);
        
        // Layout track strips
        layoutTracks();
    }
    
    void updateTrackCount()
    {
        int trackCount = m_processor.getNumTracks();
        
        // Add or remove track strips as needed
        while (m_trackStrips.size() < trackCount)
        {
            auto strip = std::make_unique<TrackStrip>(m_trackStrips.size(), m_processor);
            m_trackContainer.addAndMakeVisible(strip.get());
            m_trackStrips.push_back(std::move(strip));
        }
        
        while (m_trackStrips.size() > trackCount)
        {
            m_trackStrips.pop_back();
        }
        
        layoutTracks();
    }
    
private:
    void layoutTracks()
    {
        int stripWidth = 100;
        int totalWidth = stripWidth * m_trackStrips.size();
        
        m_trackContainer.setSize(totalWidth, getHeight());
        
        int x = 0;
        for (auto& strip : m_trackStrips)
        {
            strip->setBounds(x, 0, stripWidth, getHeight());
            x += stripWidth;
        }
    }
    
    HAMAudioProcessor& m_processor;
    
    juce::Viewport m_viewport;
    juce::Component m_trackContainer;
    std::vector<std::unique_ptr<TrackStrip>> m_trackStrips;
    
    juce::Label m_masterLabel;
    juce::Slider m_masterVolume;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerView)
};

} // namespace HAM::UI