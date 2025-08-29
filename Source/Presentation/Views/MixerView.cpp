/*
  ==============================================================================

    MixerView.cpp
    Direct plugin management implementation based on PulseProject
    
  ==============================================================================
*/

#include "MixerView.h"
#include "../../Infrastructure/Plugins/PluginWindowManager.h"
#include "../../Domain/Services/TrackManager.h"
#include <iostream>

namespace HAM::UI
{

//==============================================================================
// SimpleMixerView Implementation
//==============================================================================

MixerView::MixerView(HAMAudioProcessor& processor)
    : m_processor(processor)
{
    // Register as TrackManager listener
    auto& trackManager = TrackManager::getInstance();
    trackManager.addListener(this);
    
    // Create channel strips
    createChannelStrips();
    
    // Setup master section
    m_masterLabel.setJustificationType(juce::Justification::centred);
    m_masterLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(m_masterLabel);
    
    m_masterVolume.setSliderStyle(juce::Slider::LinearVertical);
    m_masterVolume.setRange(0.0, 1.0, 0.01);
    m_masterVolume.setValue(1.0);
    m_masterVolume.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    addAndMakeVisible(m_masterVolume);
    
    // Setup viewport for channels
    m_viewport.setViewedComponent(&m_channelContainer, false);
    m_viewport.setScrollBarsShown(false, true);
    addAndMakeVisible(m_viewport);
    
    // Start timer for UI updates
    startTimer(100); // 10Hz updates
    
    setSize(800, 500);
}

MixerView::~MixerView()
{
    // Unregister from TrackManager
    auto& trackManager = TrackManager::getInstance();
    trackManager.removeListener(this);
    
    stopTimer();
}

void MixerView::paint(juce::Graphics& g)
{
    // Dark gradient background for depth
    juce::ColourGradient bgGradient(
        juce::Colour(0xff0a0a0a), 0, 0,
        juce::Colour(0xff050505), 0, static_cast<float>(getHeight()),
        false
    );
    g.setGradientFill(bgGradient);
    g.fillAll();
    
    // Master section divider with glow effect
    const int masterDividerX = getWidth() - 150;  // Wider master section
    
    // Glow effect
    g.setColour(juce::Colour(0xff2a2a2a).withAlpha(0.3f));
    g.fillRect(masterDividerX - 2, 0, 4, getHeight());
    
    // Main divider line
    g.setColour(juce::Colour(0xff3a3a3a));
    g.fillRect(masterDividerX, 0, 1, getHeight());
}

void MixerView::resized()
{
    auto bounds = getLocalBounds();
    
    // Master section on the right (wider for better control)
    auto masterBounds = bounds.removeFromRight(150);
    masterBounds.reduce(10, 10);  // Add padding
    
    // Master label with better spacing
    auto masterLabelBounds = masterBounds.removeFromTop(50);
    m_masterLabel.setBounds(masterLabelBounds);
    m_masterLabel.setFont(juce::Font(juce::FontOptions(18.0f)).withStyle(juce::Font::bold));
    
    // Master volume fader (taller for precision)
    masterBounds.removeFromTop(20);  // Spacing
    auto masterFaderBounds = masterBounds.removeFromTop(300);
    m_masterVolume.setBounds(masterFaderBounds.reduced(25, 10));
    
    // Style the master fader
    m_masterVolume.setColour(juce::Slider::backgroundColourId, juce::Colour(0xff1a1a1a));
    m_masterVolume.setColour(juce::Slider::trackColourId, juce::Colour(0xff00ff88));
    m_masterVolume.setColour(juce::Slider::thumbColourId, juce::Colour(0xffffffff));
    
    // Tracks viewport with small margin
    bounds.reduce(5, 5);
    m_viewport.setBounds(bounds);
    
    layoutChannels();
}

void MixerView::timerCallback()
{
    // Update UI state if needed
    // This is where we'd update meters, etc.
}

void MixerView::createChannelStrips()
{
    // Get tracks from TrackManager
    auto& trackManager = TrackManager::getInstance();
    const auto& tracks = trackManager.getAllTracks();
    
    for (size_t i = 0; i < tracks.size(); ++i)
    {
        auto strip = std::make_unique<ChannelStrip>(static_cast<int>(i), m_processor, this);
        m_channelContainer.addAndMakeVisible(strip.get());
        m_channelStrips.push_back(std::move(strip));
    }
}

void MixerView::layoutChannels()
{
    // Calculate optimal channel strip width
    const int minStripWidth = 120;  // Minimum for usability
    const int maxStripWidth = 180;  // Maximum to prevent over-stretching
    const int preferredStripWidth = 140;  // Ideal width
    const int stripSpacing = 2;  // Small gap between strips
    
    int viewportWidth = m_viewport.getWidth();
    int numStrips = static_cast<int>(m_channelStrips.size());
    
    if (numStrips == 0) return;
    
    // Calculate strip width based on available space
    int stripWidth = preferredStripWidth;
    int totalPreferredWidth = (preferredStripWidth + stripSpacing) * numStrips;
    
    // If strips fit in viewport at preferred width, center them or stretch slightly
    if (totalPreferredWidth <= viewportWidth)
    {
        // Distribute available space evenly if there's room
        stripWidth = std::min(maxStripWidth, 
                             (viewportWidth - stripSpacing * (numStrips - 1)) / numStrips);
    }
    else
    {
        // Use preferred width and allow horizontal scrolling
        stripWidth = preferredStripWidth;
    }
    
    // Ensure minimum width
    stripWidth = std::max(minStripWidth, stripWidth);
    
    // Calculate total width needed
    const int totalWidth = (stripWidth + stripSpacing) * numStrips - stripSpacing;
    m_channelContainer.setSize(totalWidth, m_viewport.getHeight());
    
    // Position each strip with spacing
    int x = 0;
    for (auto& strip : m_channelStrips)
    {
        strip->setBounds(x, 0, stripWidth, m_viewport.getHeight());
        x += stripWidth + stripSpacing;
    }
    
    // Update scrollbar visibility
    m_viewport.setScrollBarsShown(false, totalWidth > viewportWidth);
}

void MixerView::onPluginSelected(int channelIndex, const juce::PluginDescription& desc)
{
    DBG("SimpleMixerView: Plugin selected: " << desc.name << " for channel " << channelIndex);
    
    // Load plugin directly - bypass complex architecture
    bool success = loadPluginDirect(channelIndex, desc);
    
    if (success)
    {
        // Update TrackManager state
        auto& trackManager = TrackManager::getInstance();
        TrackManager::PluginState pluginState;
        pluginState.hasPlugin = true;
        pluginState.pluginName = desc.name;
        pluginState.description = desc;
        pluginState.isInstrument = desc.isInstrument;
        pluginState.editorOpen = false;
        
        trackManager.setPluginState(channelIndex, pluginState, true);
        
        // Update channel strip display
        if (channelIndex >= 0 && channelIndex < m_channelStrips.size())
        {
            m_channelStrips[channelIndex]->updatePluginDisplay(desc.name);
        }
        
        // Close browser
        m_browserManager.closeBrowser();
        
        // Show plugin editor automatically
        m_processor.showPluginEditor(channelIndex, -1);
    }
    else
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Plugin Load Failed",
            "Failed to load " + desc.name);
    }
}

bool MixerView::loadPluginDirect(int channelIndex, const juce::PluginDescription& desc)
{
    DBG("MixerView: Loading plugin directly - " << desc.name);
    
    // Load the plugin through the audio processor instead of plugin manager
    // This ensures it's properly connected to the audio graph
    bool isInstrument = desc.isInstrument;
    bool success = m_processor.loadPlugin(channelIndex, desc, isInstrument);
    
    if (success)
    {
        DBG("Plugin loaded successfully through audio processor!");
    }
    else
    {
        DBG("Failed to load plugin through audio processor");
    }
    
    return success;
}

//==============================================================================
// TrackManager Listener Implementation
//==============================================================================

void MixerView::trackAdded(int trackIndex)
{
    // Add a new channel strip
    auto strip = std::make_unique<ChannelStrip>(trackIndex, m_processor, this);
    m_channelContainer.addAndMakeVisible(strip.get());
    m_channelStrips.insert(m_channelStrips.begin() + trackIndex, std::move(strip));
    
    // Re-layout
    layoutChannels();
}

void MixerView::trackRemoved(int trackIndex)
{
    // Remove the channel strip
    if (trackIndex >= 0 && trackIndex < static_cast<int>(m_channelStrips.size()))
    {
        m_channelStrips.erase(m_channelStrips.begin() + trackIndex);
        
        // Update indices for remaining strips
        for (size_t i = trackIndex; i < m_channelStrips.size(); ++i)
        {
            // Note: ChannelStrip might need a setTrackIndex method for this
        }
        
        layoutChannels();
    }
}

void MixerView::trackParametersChanged(int trackIndex)
{
    // Update the channel strip display if needed
    if (trackIndex >= 0 && trackIndex < static_cast<int>(m_channelStrips.size()))
    {
        // Channel strip will update itself based on track state
        m_channelStrips[trackIndex]->repaint();
    }
}

void MixerView::trackPluginChanged(int trackIndex)
{
    // Update the channel strip plugin display
    if (trackIndex >= 0 && trackIndex < static_cast<int>(m_channelStrips.size()))
    {
        auto& trackManager = TrackManager::getInstance();
        auto* pluginState = trackManager.getPluginState(trackIndex, true);
        
        if (pluginState && pluginState->hasPlugin)
        {
            m_channelStrips[trackIndex]->updatePluginDisplay(pluginState->pluginName);
        }
        else
        {
            // Clear plugin display
            m_channelStrips[trackIndex]->updatePluginDisplay("");
        }
    }
}

void MixerView::openPluginBrowserForTrack(int trackIndex)
{
    DBG("MixerView::openPluginBrowserForTrack called for track " << trackIndex);
    
    // Make sure we have a valid track index
    if (trackIndex < 0 || trackIndex >= static_cast<int>(m_channelStrips.size()))
    {
        DBG("Invalid track index: " << trackIndex);
        return;
    }
    
    // Check if plugin is already loaded
    auto& trackManager = TrackManager::getInstance();
    auto* pluginState = trackManager.getPluginState(trackIndex, true);
    
    if (pluginState && pluginState->hasPlugin)
    {
        // Plugin is already loaded - open its editor instead
        DBG("Plugin already loaded, opening editor for: " << pluginState->pluginName);
        m_channelStrips[trackIndex]->showPluginEditor();
    }
    else
    {
        // No plugin loaded - open browser
        DBG("Opening plugin browser for track " << trackIndex);
        
        // Create or show the browser window
        auto* browserWindow = m_browserManager.createBrowserWindow();
        if (!browserWindow)
        {
            DBG("Failed to create browser window!");
            return;
        }
        
        // Get the browser component
        if (auto* browser = m_browserManager.getBrowser())
        {
            // Set up callback to handle plugin selection
            browser->onPluginSelected = [this, trackIndex](const juce::PluginDescription& desc)
            {
                DBG("Plugin selected from browser: " << desc.name);
                onPluginSelected(trackIndex, desc);
                m_browserManager.closeBrowser();
            };
        }
        
        browserWindow->setVisible(true);
        browserWindow->toFront(true);
    }
}

//==============================================================================
// ChannelStrip Implementation
//==============================================================================

MixerView::ChannelStrip::ChannelStrip(int channelIndex, HAMAudioProcessor& processor, MixerView* parent)
    : m_channelIndex(channelIndex)
    , m_processor(processor)
    , m_parent(parent)
{
    // Channel label
    m_channelLabel.setText("Track " + juce::String(channelIndex + 1), juce::dontSendNotification);
    m_channelLabel.setJustificationType(juce::Justification::centred);
    m_channelLabel.setColour(juce::Label::textColourId, getTrackColor());
    addAndMakeVisible(m_channelLabel);
    
    // Plugin slot button - THE CRITICAL PART
    m_pluginSlot.setButtonText("< No Plugin >");
    m_pluginSlot.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1a1a1a));
    m_pluginSlot.setColour(juce::TextButton::textColourOffId, juce::Colours::grey);
    m_pluginSlot.addListener(this);
    addAndMakeVisible(m_pluginSlot);
    
    // Control buttons (initially hidden)
    m_deleteButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffaa0000));
    m_deleteButton.addListener(this);
    m_deleteButton.setVisible(false);
    addAndMakeVisible(m_deleteButton);
    
    m_bypassButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff444444));
    m_bypassButton.addListener(this);
    m_bypassButton.setVisible(false);
    addAndMakeVisible(m_bypassButton);
    
    m_editButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff004466));
    m_editButton.addListener(this);
    m_editButton.setVisible(false);
    addAndMakeVisible(m_editButton);
    
    // Volume slider
    m_volumeSlider.setSliderStyle(juce::Slider::LinearVertical);
    m_volumeSlider.setRange(0.0, 1.0, 0.01);
    m_volumeSlider.setValue(0.8);
    m_volumeSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    m_volumeSlider.addListener(this);
    addAndMakeVisible(m_volumeSlider);
    
    // Pan knob
    m_panSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    m_panSlider.setRange(-1.0, 1.0, 0.01);
    m_panSlider.setValue(0.0);
    m_panSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    m_panSlider.addListener(this);
    addAndMakeVisible(m_panSlider);
    
    // Mute/Solo buttons
    m_muteButton.setToggleable(true);
    m_muteButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2a2a2a));
    m_muteButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffff3030));
    m_muteButton.addListener(this);
    addAndMakeVisible(m_muteButton);
    
    m_soloButton.setToggleable(true);
    m_soloButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2a2a2a));
    m_soloButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffffff30));
    m_soloButton.addListener(this);
    addAndMakeVisible(m_soloButton);
}

void MixerView::ChannelStrip::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    
    // Gradient background for depth
    juce::ColourGradient bgGradient(
        juce::Colour(0xff1a1a1a), bounds.getCentreX(), 0,
        juce::Colour(0xff0f0f0f), bounds.getCentreX(), bounds.getHeight(),
        false
    );
    g.setGradientFill(bgGradient);
    g.fillRoundedRectangle(bounds.toFloat(), 4.0f);
    
    // Subtle border
    g.setColour(juce::Colour(0xff2a2a2a));
    g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f), 4.0f, 1.0f);
    
    // Track color accent at top (thicker and gradient)
    auto trackColor = getTrackColor();
    auto colorStrip = bounds.removeFromTop(4);
    
    juce::ColourGradient colorGradient(
        trackColor.withAlpha(0.9f), colorStrip.getCentreX(), colorStrip.getY(),
        trackColor.withAlpha(0.4f), colorStrip.getCentreX(), colorStrip.getBottom(),
        false
    );
    g.setGradientFill(colorGradient);
    g.fillRect(colorStrip);
    
    // Add glow effect under color strip
    g.setColour(trackColor.withAlpha(0.2f));
    g.fillRect(bounds.removeFromTop(10));
    
    // Level meter background (placeholder for future implementation)
    auto meterArea = getLocalBounds().removeFromBottom(100).removeFromTop(80).reduced(15, 0);
    g.setColour(juce::Colour(0xff0a0a0a));
    g.fillRoundedRectangle(meterArea.toFloat(), 2.0f);
    
    // Fake level meter bars for visual appeal
    g.setColour(juce::Colour(0xff00ff88).withAlpha(0.7f));
    auto leftMeter = meterArea.removeFromLeft(meterArea.getWidth() / 2 - 2);
    auto rightMeter = meterArea.removeFromRight(meterArea.getWidth() - 2);
    
    // Simulate some level
    float level = 0.6f + (0.2f * std::sin(juce::Time::currentTimeMillis() * 0.001f + m_channelIndex));
    int meterHeight = static_cast<int>(leftMeter.getHeight() * level);
    
    g.fillRect(leftMeter.removeFromBottom(meterHeight));
    g.fillRect(rightMeter.removeFromBottom(meterHeight));
}

void MixerView::ChannelStrip::resized()
{
    auto bounds = getLocalBounds();
    bounds.reduce(8, 8);  // Overall padding
    
    // Skip track color area
    bounds.removeFromTop(14);  // Space for color strip and glow
    
    // Track label - larger and centered
    auto labelBounds = bounds.removeFromTop(35);
    m_channelLabel.setBounds(labelBounds);
    m_channelLabel.setFont(juce::Font(juce::FontOptions(14.0f)).withStyle(juce::Font::bold));
    m_channelLabel.setJustificationType(juce::Justification::centred);
    
    bounds.removeFromTop(5);  // Spacing
    
    // Plugin slot - prominent button
    auto pluginBounds = bounds.removeFromTop(36);
    m_pluginSlot.setBounds(pluginBounds.reduced(4, 2));
    
    // Style the plugin button
    m_pluginSlot.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2a2a2a));
    m_pluginSlot.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff3a3a3a));
    m_pluginSlot.setColour(juce::TextButton::textColourOffId, 
                           m_hasPlugin ? juce::Colours::white : juce::Colours::grey);
    
    bounds.removeFromTop(4);  // Spacing
    
    // Plugin controls (when loaded) - better layout
    if (m_hasPlugin)
    {
        auto controlArea = bounds.removeFromTop(28);
        int buttonWidth = (controlArea.getWidth() - 8) / 3;  // Account for gaps
        
        m_editButton.setBounds(controlArea.removeFromLeft(buttonWidth));
        controlArea.removeFromLeft(4);  // Gap
        m_bypassButton.setBounds(controlArea.removeFromLeft(buttonWidth));
        controlArea.removeFromLeft(4);  // Gap
        m_deleteButton.setBounds(controlArea);
        
        // Style the control buttons
        m_editButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff004466));
        m_bypassButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff666644));
        m_deleteButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff662222));
        
        bounds.removeFromTop(4);
    }
    
    bounds.removeFromTop(8);  // More spacing
    
    // Pan knob - larger and centered
    auto panArea = bounds.removeFromTop(60);
    int knobSize = 50;
    m_panSlider.setBounds(panArea.withSizeKeepingCentre(knobSize, knobSize));
    
    // Style the pan knob
    m_panSlider.setColour(juce::Slider::rotarySliderFillColourId, getTrackColor().withAlpha(0.7f));
    m_panSlider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff2a2a2a));
    m_panSlider.setColour(juce::Slider::thumbColourId, juce::Colours::white);
    
    bounds.removeFromTop(8);  // Spacing
    
    // Mute/Solo - side by side with better styling
    auto muteSoloArea = bounds.removeFromTop(32);
    int buttonSpacing = 4;
    int halfWidth = (muteSoloArea.getWidth() - buttonSpacing) / 2;
    
    m_muteButton.setBounds(muteSoloArea.removeFromLeft(halfWidth));
    muteSoloArea.removeFromLeft(buttonSpacing);
    m_soloButton.setBounds(muteSoloArea);
    
    bounds.removeFromTop(12);  // Spacing before fader
    
    // Volume fader - taller and centered
    auto faderArea = bounds.removeFromTop(200);
    int faderWidth = 40;
    m_volumeSlider.setBounds(faderArea.withSizeKeepingCentre(faderWidth, faderArea.getHeight()));
    
    // Style the volume fader
    m_volumeSlider.setColour(juce::Slider::backgroundColourId, juce::Colour(0xff0a0a0a));
    m_volumeSlider.setColour(juce::Slider::trackColourId, getTrackColor().withAlpha(0.6f));
    m_volumeSlider.setColour(juce::Slider::thumbColourId, juce::Colours::white);
    
    // Leave space at bottom for level meters (painted in paint())
}

void MixerView::ChannelStrip::buttonClicked(juce::Button* button)
{
    if (button == &m_pluginSlot)
    {
        DBG("ChannelStrip: Plugin slot clicked for channel " << m_channelIndex);
        
        // Check current plugin state from TrackManager
        auto& trackManager = TrackManager::getInstance();
        auto* pluginState = trackManager.getPluginState(m_channelIndex, true);
        
        if (pluginState && pluginState->hasPlugin)
        {
            // Plugin is loaded - check if window is already open
            auto& windowManager = PluginWindowManager::getInstance();
            
            if (windowManager.isWindowOpen(m_channelIndex, -1))
            {
                // Window is open - just bring it to front
                DBG("Plugin window already open, bringing to front");
                // The PluginWindowManager handles focus automatically
                showPluginEditor();
            }
            else
            {
                // Window is not open - open the plugin editor
                DBG("Opening plugin editor for: " << pluginState->pluginName);
                showPluginEditor();
            }
        }
        else
        {
            // No plugin loaded - show browser
            DBG("No plugin loaded, opening browser");
            auto* browserWindow = m_parent->m_browserManager.createBrowserWindow();
            
            if (auto* browser = m_parent->m_browserManager.getBrowser())
            {
                // Set callback - DIRECT like Pulse
                browser->onPluginSelected = [this](const juce::PluginDescription& desc) {
                    m_parent->onPluginSelected(m_channelIndex, desc);
                };
            }
        }
    }
    else if (button == &m_deleteButton)
    {
        // Remove plugin
        auto& pluginManager = PluginManager::instance();
        pluginManager.removeInstrumentPlugin(m_channelIndex);
        
        // Clear from TrackManager
        auto& trackManager = TrackManager::getInstance();
        trackManager.clearPlugin(m_channelIndex, true);
        
        // Close any open plugin window
        auto& windowManager = PluginWindowManager::getInstance();
        windowManager.closePluginWindow(m_channelIndex, -1);
        
        m_hasPlugin = false;
        m_pluginSlot.setButtonText("< No Plugin >");
        m_deleteButton.setVisible(false);
        m_bypassButton.setVisible(false);
        m_editButton.setVisible(false);
        resized();
    }
    else if (button == &m_editButton)
    {
        showPluginEditor();
    }
    else if (button == &m_muteButton)
    {
        // Handle mute
    }
    else if (button == &m_soloButton)
    {
        // Handle solo
    }
}

void MixerView::ChannelStrip::sliderValueChanged(juce::Slider* slider)
{
    if (slider == &m_volumeSlider)
    {
        // Set track volume
    }
    else if (slider == &m_panSlider)
    {
        // Set track pan
    }
}

void MixerView::ChannelStrip::updatePluginDisplay(const juce::String& pluginName)
{
    m_hasPlugin = true;
    m_loadedPluginName = pluginName;
    m_pluginSlot.setButtonText(pluginName);
    m_pluginSlot.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    
    // Show control buttons
    m_deleteButton.setVisible(true);
    m_bypassButton.setVisible(true);
    m_editButton.setVisible(true);
    
    resized();
}

void MixerView::ChannelStrip::showPluginEditor()
{
    m_processor.showPluginEditor(m_channelIndex, -1);
}

juce::Colour MixerView::ChannelStrip::getTrackColor() const
{
    const std::vector<juce::Colour> colors = {
        juce::Colour(0xff00ffaa), // Mint
        juce::Colour(0xff00aaff), // Cyan
        juce::Colour(0xffff00aa), // Magenta
        juce::Colour(0xffffaa00), // Orange
        juce::Colour(0xffaa00ff), // Purple
        juce::Colour(0xff00ff00), // Green
        juce::Colour(0xffff0055), // Red
        juce::Colour(0xff55aaff)  // Blue
    };
    
    return colors[m_channelIndex % colors.size()];
}

//==============================================================================
// BrowserWindowManager Implementation
//==============================================================================

MixerView::BrowserWindowManager::~BrowserWindowManager()
{
    closeBrowser();
}

juce::DocumentWindow* MixerView::BrowserWindowManager::createBrowserWindow()
{
    if (m_browserWindow && m_browserWindow->isVisible())
    {
        m_browserWindow->toFront(true);
        return m_browserWindow.get();
    }
    
    // Create new browser window
    m_browserWindow = std::make_unique<juce::DocumentWindow>(
        "Plugin Browser",
        juce::Colour(0xff1e1e1e),
        juce::DocumentWindow::allButtons);
    
    auto browser = std::make_unique<LightweightPluginBrowser>();
    
    m_browserWindow->setContentOwned(browser.release(), true);
    m_browserWindow->setUsingNativeTitleBar(true);
    m_browserWindow->centreWithSize(700, 500);
    m_browserWindow->setVisible(true);
    m_browserWindow->setResizable(true, true);
    
    return m_browserWindow.get();
}

LightweightPluginBrowser* MixerView::BrowserWindowManager::getBrowser() const
{
    if (m_browserWindow)
    {
        return dynamic_cast<LightweightPluginBrowser*>(m_browserWindow->getContentComponent());
    }
    return nullptr;
}

void MixerView::BrowserWindowManager::closeBrowser()
{
    if (m_browserWindow)
    {
        m_browserWindow->setVisible(false);
        m_browserWindow.reset();
    }
}

//==============================================================================
// LightweightPluginBrowser Implementation
//==============================================================================

LightweightPluginBrowser::LightweightPluginBrowser()
{
    // Search box
    m_searchBox.setTextToShowWhenEmpty("Search plugins...", juce::Colours::grey);
    m_searchBox.onTextChange = [this] { filterPlugins(); };
    addAndMakeVisible(m_searchBox);
    
    // Type filter
    m_typeFilter.addItem("All Types", 1);
    m_typeFilter.addItem("Instruments", 2);
    m_typeFilter.addItem("Effects", 3);
    m_typeFilter.setSelectedId(1);
    m_typeFilter.onChange = [this] { filterPlugins(); };
    addAndMakeVisible(m_typeFilter);
    
    // Plugin list
    m_pluginList.setModel(this);
    m_pluginList.setRowHeight(30);
    m_pluginList.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff0a0a0a));
    addAndMakeVisible(m_pluginList);
    
    // Load button
    m_loadButton.onClick = [this] { loadSelectedPlugin(); };
    m_loadButton.setEnabled(false);
    addAndMakeVisible(m_loadButton);
    
    // Scan button
    m_scanButton.onClick = [this] { startPluginScan(); };
    addAndMakeVisible(m_scanButton);
    
    // Status label
    m_statusLabel.setJustificationType(juce::Justification::centred);
    m_statusLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
    addAndMakeVisible(m_statusLabel);
    
    // Load cached plugins IMMEDIATELY - KEY DIFFERENCE
    loadCachedPlugins();
    
    setSize(700, 500);
}

LightweightPluginBrowser::~LightweightPluginBrowser()
{
    stopTimer();
}

void LightweightPluginBrowser::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff0a0a0a));
}

void LightweightPluginBrowser::resized()
{
    auto bounds = getLocalBounds().reduced(10);
    
    // Search and filter
    auto topRow = bounds.removeFromTop(30);
    m_searchBox.setBounds(topRow.removeFromLeft(300));
    topRow.removeFromLeft(10);
    m_typeFilter.setBounds(topRow.removeFromLeft(150));
    topRow.removeFromLeft(10);
    m_scanButton.setBounds(topRow.removeFromLeft(100));
    
    bounds.removeFromTop(10);
    
    // Plugin list
    m_pluginList.setBounds(bounds.removeFromTop(bounds.getHeight() - 80));
    
    bounds.removeFromTop(10);
    
    // Bottom buttons
    auto bottomRow = bounds.removeFromTop(30);
    m_loadButton.setBounds(bottomRow.removeFromRight(100));
    
    // Status
    m_statusLabel.setBounds(bounds);
}

int LightweightPluginBrowser::getNumRows()
{
    return m_filteredPlugins.size();
}

void LightweightPluginBrowser::paintListBoxItem(int rowNumber, juce::Graphics& g, 
                                                int width, int height, bool rowIsSelected)
{
    if (rowNumber >= 0 && rowNumber < m_filteredPlugins.size())
    {
        const auto& plugin = m_filteredPlugins[rowNumber];
        
        if (rowIsSelected)
        {
            g.fillAll(juce::Colour(0xff2a2a2a));
        }
        
        g.setColour(juce::Colours::white);
        g.setFont(14.0f);
        
        juce::String text = plugin.name;
        if (plugin.manufacturerName.isNotEmpty())
        {
            text += " - " + plugin.manufacturerName;
        }
        text += " (" + plugin.pluginFormatName + ")";
        
        g.drawText(text, 5, 0, width - 10, height, juce::Justification::centredLeft);
    }
}

void LightweightPluginBrowser::listBoxItemDoubleClicked(int row, const juce::MouseEvent&)
{
    if (row >= 0 && row < m_filteredPlugins.size())
    {
        if (onPluginSelected)
        {
            onPluginSelected(m_filteredPlugins[row]);
        }
    }
}

void LightweightPluginBrowser::loadCachedPlugins()
{
    DBG("Loading cached plugins...");
    
    // Get plugin manager and load list
    auto& pluginManager = PluginManager::instance();
    const auto& knownPlugins = pluginManager.getKnownPluginList();
    
    // Convert to array
    m_allPlugins = knownPlugins.getTypes();
    
    DBG("Loaded " << m_allPlugins.size() << " plugins from cache");
    
    // Update display
    filterPlugins();
    
    // Update status
    m_statusLabel.setText(juce::String(m_allPlugins.size()) + " plugins available", 
                         juce::dontSendNotification);
}

void LightweightPluginBrowser::filterPlugins()
{
    m_filteredPlugins.clear();
    
    juce::String searchText = m_searchBox.getText().toLowerCase();
    int typeId = m_typeFilter.getSelectedId();
    
    for (const auto& plugin : m_allPlugins)
    {
        // Type filter
        if (typeId == 2 && !plugin.isInstrument) continue;
        if (typeId == 3 && plugin.isInstrument) continue;
        
        // Search filter
        if (searchText.isNotEmpty())
        {
            if (!plugin.name.toLowerCase().contains(searchText) &&
                !plugin.manufacturerName.toLowerCase().contains(searchText))
            {
                continue;
            }
        }
        
        m_filteredPlugins.add(plugin);
    }
    
    m_pluginList.updateContent();
    
    // Enable load button if something is selected
    m_loadButton.setEnabled(m_pluginList.getSelectedRow() >= 0);
}

void LightweightPluginBrowser::loadSelectedPlugin()
{
    int selectedRow = m_pluginList.getSelectedRow();
    
    if (selectedRow >= 0 && selectedRow < m_filteredPlugins.size())
    {
        if (onPluginSelected)
        {
            onPluginSelected(m_filteredPlugins[selectedRow]);
        }
    }
}

void LightweightPluginBrowser::startPluginScan()
{
    DBG("Starting plugin scan...");
    
    auto& pluginManager = PluginManager::instance();
    pluginManager.startSandboxedScan(true);
    
    // Start timer to check progress
    startTimer(100);
}

void LightweightPluginBrowser::timerCallback()
{
    auto& pluginManager = PluginManager::instance();
    
    if (!pluginManager.isScanning())
    {
        stopTimer();
        loadCachedPlugins(); // Reload after scan
    }
}

} // namespace HAM::UI