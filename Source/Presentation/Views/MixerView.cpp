/*
  ==============================================================================

    MixerView.cpp
    Direct plugin management implementation based on PulseProject
    
  ==============================================================================
*/

#include "MixerView.h"
#include <iostream>

namespace HAM::UI
{

//==============================================================================
// SimpleMixerView Implementation
//==============================================================================

MixerView::MixerView(HAMAudioProcessor& processor)
    : m_processor(processor)
{
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
    stopTimer();
}

void MixerView::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff0a0a0a));
    
    // Master section divider
    g.setColour(juce::Colour(0xff2a2a2a));
    g.drawLine(getWidth() - 120, 0, getWidth() - 120, getHeight(), 2);
}

void MixerView::resized()
{
    auto bounds = getLocalBounds();
    
    // Master section on the right
    auto masterBounds = bounds.removeFromRight(120);
    m_masterLabel.setBounds(masterBounds.removeFromTop(40).reduced(5));
    m_masterVolume.setBounds(masterBounds.removeFromTop(200).reduced(20, 10));
    
    // Tracks viewport
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
    int numTracks = 8; // Default to 8 tracks
    
    for (int i = 0; i < numTracks; ++i)
    {
        auto strip = std::make_unique<ChannelStrip>(i, m_processor, this);
        m_channelContainer.addAndMakeVisible(strip.get());
        m_channelStrips.push_back(std::move(strip));
    }
}

void MixerView::layoutChannels()
{
    const int stripWidth = 100;
    const int totalWidth = stripWidth * static_cast<int>(m_channelStrips.size());
    
    m_channelContainer.setSize(totalWidth, getHeight());
    
    int x = 0;
    for (auto& strip : m_channelStrips)
    {
        strip->setBounds(x, 0, stripWidth, getHeight());
        x += stripWidth;
    }
}

void MixerView::onPluginSelected(int channelIndex, const juce::PluginDescription& desc)
{
    DBG("SimpleMixerView: Plugin selected: " << desc.name << " for channel " << channelIndex);
    
    // Load plugin directly - bypass complex architecture
    bool success = loadPluginDirect(channelIndex, desc);
    
    if (success)
    {
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
    DBG("SimpleMixerView: Loading plugin directly - " << desc.name);
    
    // Get plugin manager and create instance
    auto& pluginManager = PluginManager::instance();
    
    // Get sample rate and block size from processor
    double sampleRate = m_processor.getSampleRate();
    int blockSize = m_processor.getBlockSize();
    
    // Create plugin instance synchronously (like Pulse does)
    auto instance = pluginManager.createPluginInstance(desc, sampleRate, blockSize);
    
    if (!instance)
    {
        DBG("Failed to create plugin instance");
        return false;
    }
    
    // Add to plugin manager
    bool success = pluginManager.addInstrumentPlugin(channelIndex, std::move(instance), desc);
    
    if (success)
    {
        DBG("Plugin loaded successfully!");
        
        // TODO: Connect plugin to audio processing chain
        // This would involve adding the plugin to the processor graph
    }
    
    return success;
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
    // Background
    g.fillAll(juce::Colour(0xff151515));
    
    // Track color strip at top
    g.setColour(getTrackColor());
    g.fillRect(0, 0, getWidth(), 3);
}

void MixerView::ChannelStrip::resized()
{
    auto bounds = getLocalBounds();
    
    // Track label
    m_channelLabel.setBounds(bounds.removeFromTop(30).reduced(5));
    
    // Plugin slot - THE KEY UI ELEMENT
    m_pluginSlot.setBounds(bounds.removeFromTop(30).reduced(5, 2));
    
    // Plugin controls (when loaded)
    if (m_hasPlugin)
    {
        auto controlArea = bounds.removeFromTop(25);
        int buttonWidth = controlArea.getWidth() / 3;
        m_deleteButton.setBounds(controlArea.removeFromLeft(buttonWidth).reduced(1));
        m_bypassButton.setBounds(controlArea.removeFromLeft(buttonWidth).reduced(1));
        m_editButton.setBounds(controlArea.reduced(1));
    }
    
    // Mute/Solo
    auto muteSoloArea = bounds.removeFromTop(30);
    m_muteButton.setBounds(muteSoloArea.removeFromLeft(getWidth() / 2).reduced(2));
    m_soloButton.setBounds(muteSoloArea.reduced(2));
    
    // Pan knob
    m_panSlider.setBounds(bounds.removeFromTop(50).reduced(10));
    
    // Volume fader
    m_volumeSlider.setBounds(bounds.removeFromTop(120).reduced(10, 0));
}

void MixerView::ChannelStrip::buttonClicked(juce::Button* button)
{
    if (button == &m_pluginSlot)
    {
        DBG("ChannelStrip: Plugin slot clicked for channel " << m_channelIndex);
        
        if (m_hasPlugin)
        {
            // Show plugin editor if already loaded
            showPluginEditor();
        }
        else
        {
            // Show plugin browser - CRITICAL PART
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