/*
  ==============================================================================

    TrackSidebar.cpp
    Implementation of Track control sidebar for HAM sequencer
    Fixed 510px height with all controls always visible (increased for Accumulator button)

  ==============================================================================
*/

#include "TrackSidebar.h"
#include "../../Infrastructure/Messaging/MessageDispatcher.h"
#include "../../Infrastructure/Messaging/MessageTypes.h"
#include "../../Infrastructure/Plugins/PluginWindowManager.h"
#include "../../Domain/Services/TrackManager.h"
#include "../ViewModels/TrackViewModel.h"

namespace HAM::UI {

//==============================================================================
// TrackControlStrip Implementation
//==============================================================================

TrackControlStrip::TrackControlStrip(int trackIndex)
    : m_trackIndex(trackIndex)
{
    // Get track color from design tokens
    m_trackColor = DesignTokens::Colors::getTrackColor(trackIndex);
    m_trackName = "Track " + juce::String(trackIndex + 1);
    
    setupControls();
}

void TrackControlStrip::setupControls()
{
    // Track name editor
    m_trackNameEditor = std::make_unique<juce::TextEditor>("trackName");
    m_trackNameEditor->setText(m_trackName);
    m_trackNameEditor->setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
    m_trackNameEditor->setColour(juce::TextEditor::textColourId, juce::Colour(DesignTokens::Colors::TEXT_PRIMARY));
    m_trackNameEditor->setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    m_trackNameEditor->setColour(juce::TextEditor::focusedOutlineColourId, m_trackColor.withAlpha(0.5f));
    m_trackNameEditor->setFont(juce::Font(juce::FontOptions(16.0f)).withStyle(juce::Font::bold));
    m_trackNameEditor->setJustification(juce::Justification::centredLeft);
    m_trackNameEditor->onReturnKey = [this] {
        m_trackName = m_trackNameEditor->getText();
        m_trackNameEditor->giveAwayKeyboardFocus();
    };
    m_trackNameEditor->onFocusLost = [this] {
        m_trackName = m_trackNameEditor->getText();
    };
    addAndMakeVisible(m_trackNameEditor.get());
    
    // Mute button - use track color
    m_muteButton = std::make_unique<ModernButton>("M", ModernButton::Style::Outline);
    m_muteButton->setColor(m_trackColor);  // Use track color by default
    m_muteButton->onClick = [this]() {
        m_isMuted = !m_isMuted;
        m_muteButton->setColor(m_isMuted ? 
            juce::Colour(DesignTokens::Colors::ACCENT_RED) : 
            m_trackColor);
        if (onMuteChanged) onMuteChanged(m_trackIndex, m_isMuted);
    };
    addAndMakeVisible(m_muteButton.get());
    
    // Solo button - use track color
    m_soloButton = std::make_unique<ModernButton>("S", ModernButton::Style::Outline);
    m_soloButton->setColor(m_trackColor);  // Use track color by default
    m_soloButton->onClick = [this]() {
        m_isSoloed = !m_isSoloed;
        m_soloButton->setColor(m_isSoloed ? 
            juce::Colour(DesignTokens::Colors::ACCENT_AMBER) : 
            m_trackColor);
        if (onSoloChanged) onSoloChanged(m_trackIndex, m_isSoloed);
    };
    addAndMakeVisible(m_soloButton.get());
    
    // MIDI Channel selector (1-16)
    m_channelLabel = createLabel("MIDI Channel");
    m_channelSelector = std::make_unique<juce::ComboBox>("Channel");
    for (int i = 1; i <= 16; ++i)
    {
        m_channelSelector->addItem("Ch " + juce::String(i), i);
    }
    m_channelSelector->onChange = [this]() {
        if (onChannelChanged) 
            onChannelChanged(m_trackIndex, m_channelSelector->getSelectedId());
    };
    m_channelSelector->setSelectedId(m_trackIndex + 1); // Default to track number
    m_channelSelector->setColour(juce::ComboBox::backgroundColourId, juce::Colour(DesignTokens::Colors::BG_RAISED));
    m_channelSelector->setColour(juce::ComboBox::textColourId, juce::Colour(DesignTokens::Colors::TEXT_PRIMARY));
    m_channelSelector->setColour(juce::ComboBox::outlineColourId, m_trackColor.withAlpha(0.3f));
    addAndMakeVisible(m_channelSelector.get());
    
    // MIDI Routing selector
    m_midiRoutingLabel = createLabel("MIDI Routing");
    m_midiRoutingSelector = std::make_unique<juce::ComboBox>("MidiRouting");
    m_midiRoutingSelector->addItem("Plugin Only", 1);
    m_midiRoutingSelector->addItem("External Only", 2);
    m_midiRoutingSelector->addItem("Both", 3);
    m_midiRoutingSelector->onChange = [this]() {
        if (onMidiRoutingChanged) {
            HAM::MidiRoutingMode mode;
            switch (m_midiRoutingSelector->getSelectedId()) {
                case 1: mode = HAM::MidiRoutingMode::PLUGIN_ONLY; break;
                case 2: mode = HAM::MidiRoutingMode::EXTERNAL_ONLY; break;
                case 3: mode = HAM::MidiRoutingMode::BOTH; break;
                default: mode = HAM::MidiRoutingMode::PLUGIN_ONLY; break;
            }
            onMidiRoutingChanged(m_trackIndex, mode);
        }
    };
    m_midiRoutingSelector->setSelectedId(1); // Default to Plugin Only
    m_midiRoutingSelector->setColour(juce::ComboBox::backgroundColourId, juce::Colour(DesignTokens::Colors::BG_RAISED));
    m_midiRoutingSelector->setColour(juce::ComboBox::textColourId, juce::Colour(DesignTokens::Colors::TEXT_PRIMARY));
    m_midiRoutingSelector->setColour(juce::ComboBox::outlineColourId, m_trackColor.withAlpha(0.3f));
    addAndMakeVisible(m_midiRoutingSelector.get());
    
    // Voice mode toggle (Mono/Poly)
    m_voiceModeLabel = createLabel("Voice Mode");
    m_voiceModeToggle = std::make_unique<ModernToggle>();
    m_voiceModeToggle->onToggle = [this](bool isPoly) {
        if (onVoiceModeChanged) onVoiceModeChanged(m_trackIndex, isPoly);
    };
    m_voiceModeToggle->setChecked(true); // Default to Poly
    addAndMakeVisible(m_voiceModeToggle.get());
    
    // Max Pulse Length slider (NEW)
    m_maxPulseLengthLabel = createLabel("Max Pulse Length");
    m_maxPulseLengthSlider = std::make_unique<ModernSlider>(false); // Horizontal
    m_maxPulseLengthSlider->setValue(0.5f); // Default 4 pulses (mapped from 1-8)
    m_maxPulseLengthSlider->setTrackColor(m_trackColor);
    m_maxPulseLengthSlider->onValueChange = [this](float value) {
        int maxPulseLength = static_cast<int>(value * 7.0f + 1.0f); // Map 0-1 to 1-8
        if (onMaxPulseLengthChanged) onMaxPulseLengthChanged(m_trackIndex, maxPulseLength);
    };
    addAndMakeVisible(m_maxPulseLengthSlider.get());
    
    // Swing slider - consistent with other sliders in track sidebar
    m_swingLabel = createLabel("Swing");
    m_swingSlider = std::make_unique<ModernSlider>(false); // Horizontal slider
    m_swingSlider->setValue(0.5f); // Default 50% (no swing)
    m_swingSlider->setTrackColor(m_trackColor);
    m_swingSlider->onValueChange = [this](float value) {
        if (onSwingChanged) 
            onSwingChanged(m_trackIndex, value * 100.0f); // Convert to 0-100 range
    };
    addAndMakeVisible(m_swingSlider.get());
    
    // Division control (segmented)
    m_divisionLabel = createLabel("Division");
    juce::StringArray divisions;
    divisions.add("1/4");
    divisions.add("1/8");
    divisions.add("1/16");
    divisions.add("1/32");
    m_divisionControl = std::make_unique<SegmentedControl>(divisions);
    m_divisionControl->onSelectionChanged = [this](int index) {
        int division = 4 * (1 << index); // 4, 8, 16, 32
        if (onDivisionChanged) onDivisionChanged(m_trackIndex, division);
    };
    addAndMakeVisible(m_divisionControl.get());
    
    // Octave input (-3 to +3)
    m_octaveLabel = createLabel("Octave");
    m_octaveInput = std::make_unique<NumericInput>(-3, 3, 1);
    m_octaveInput->setValue(0); // Default octave
    m_octaveInput->onValueChanged = [this](float value) {
        if (onOctaveChanged) onOctaveChanged(m_trackIndex, static_cast<int>(value));
    };
    addAndMakeVisible(m_octaveInput.get());
    
    // Plugin button - use track color
    m_pluginButton = std::make_unique<ModernButton>("PLUGIN", ModernButton::Style::Gradient);
    m_pluginButton->setColor(m_trackColor);  // Use track color
    m_pluginButton->onClick = [this]() {
        DBG("Plugin button clicked for track " << m_trackIndex);
        
        // Check if a plugin is already loaded
        auto& trackManager = TrackManager::getInstance();
        auto* pluginState = trackManager.getPluginState(m_trackIndex, true);
        
        if (pluginState && pluginState->hasPlugin)
        {
            // Plugin is loaded - open/focus the editor window
            DBG("Opening/focusing plugin editor for: " << pluginState->pluginName);
            if (onPluginEditorRequested)
                onPluginEditorRequested(m_trackIndex);
        }
        else
        {
            // No plugin loaded - open the browser
            // Only call onPluginBrowserRequested, not onPluginButtonClicked
            // This prevents duplicate browser windows
            DBG("No plugin loaded, opening browser");
            if (onPluginBrowserRequested) 
                onPluginBrowserRequested(m_trackIndex);
        }
    };
    addAndMakeVisible(m_pluginButton.get());
    
    // Accumulator button - use track color
    m_accumulatorButton = std::make_unique<ModernButton>("ACCUM", ModernButton::Style::Gradient);
    m_accumulatorButton->setColor(m_trackColor);  // Use track color
    m_accumulatorButton->onClick = [this]() {
        DBG("Accumulator button clicked for track " << m_trackIndex);
        if (onAccumulatorButtonClicked) onAccumulatorButtonClicked(m_trackIndex);
    };
    addAndMakeVisible(m_accumulatorButton.get());
}

std::unique_ptr<juce::Label> TrackControlStrip::createLabel(const juce::String& text)
{
    auto label = std::make_unique<juce::Label>(text, text);
    label->setFont(juce::Font(juce::FontOptions(11.0f)));
    label->setColour(juce::Label::textColourId, juce::Colour(DesignTokens::Colors::TEXT_MUTED));
    label->setJustificationType(juce::Justification::left);
    addAndMakeVisible(label.get());
    return label;
}

void TrackControlStrip::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    
    // Simple solid color background with track color tint
    if (m_isSelected)
    {
        // Selected state - subtle track color overlay
        g.setColour(m_trackColor.withAlpha(0.08f));
        g.fillRoundedRectangle(bounds.toFloat(), 6.0f);
    }
    else
    {
        // Unselected state - very subtle track color
        g.setColour(m_trackColor.withAlpha(0.05f));
        g.fillRoundedRectangle(bounds.toFloat(), 6.0f);
    }
    
    // Border with track color accent
    if (m_isSelected) {
        g.setColour(m_trackColor.withAlpha(0.3f));
        g.drawRoundedRectangle(bounds.toFloat(), 6.0f, 1.5f);
    } else {
        g.setColour(m_trackColor.withAlpha(0.15f));
        g.drawRoundedRectangle(bounds.toFloat(), 6.0f, 1.0f);
    }
}

void TrackControlStrip::resized()
{
    auto bounds = getLocalBounds();
    
    // Use full height allocated (510px to accommodate Accumulator button)
    // No need to limit height anymore since we have the space
    
    // Match stage card positioning - start content at top of content area (line 5)
    // No top offset needed since cards now start at content area top
    bounds.reduce(12, 0); // Only horizontal padding, no vertical padding to align with control bar
    
    // Header section (36px)
    auto headerArea = bounds.removeFromTop(36);
    
    // Track name takes more space
    m_trackNameEditor->setBounds(headerArea.removeFromLeft(headerArea.getWidth() - 84));
    
    // Mute/Solo buttons on right, more compact
    headerArea.removeFromLeft(4);
    m_muteButton->setBounds(headerArea.removeFromLeft(38));
    headerArea.removeFromLeft(4);
    m_soloButton->setBounds(headerArea.removeFromLeft(38));
    
    bounds.removeFromTop(10); // Spacing after header
    
    // Optimized control layout helper
    auto layoutControl = [&bounds](juce::Component* label, juce::Component* control, int height = 26) {
        auto row = bounds.removeFromTop(height + 16);
        if (label) label->setBounds(row.removeFromTop(14));
        control->setBounds(row.removeFromTop(height));
        bounds.removeFromTop(8); // Consistent spacing
    };
    
    // MIDI Channel
    layoutControl(m_channelLabel.get(), m_channelSelector.get(), 30);
    
    // MIDI Routing  
    layoutControl(m_midiRoutingLabel.get(), m_midiRoutingSelector.get(), 30);
    
    // Voice Mode - centered toggle
    auto voiceRow = bounds.removeFromTop(46);
    m_voiceModeLabel->setBounds(voiceRow.removeFromTop(14));
    auto toggleArea = voiceRow.removeFromTop(28);
    // Center the toggle
    int toggleWidth = 60;
    m_voiceModeToggle->setBounds(
        toggleArea.getCentreX() - toggleWidth/2,
        toggleArea.getY(),
        toggleWidth,
        toggleArea.getHeight()
    );
    bounds.removeFromTop(8);
    
    // Max Pulse Length
    layoutControl(m_maxPulseLengthLabel.get(), m_maxPulseLengthSlider.get(), 26);
    
    // Swing - now using slider same size as pulse length
    layoutControl(m_swingLabel.get(), m_swingSlider.get(), 26);
    
    // Division - make more compact
    layoutControl(m_divisionLabel.get(), m_divisionControl.get(), 32);
    
    // Octave - centered input
    auto octaveRow = bounds.removeFromTop(46);
    m_octaveLabel->setBounds(octaveRow.removeFromTop(14));
    auto octaveInputArea = octaveRow.removeFromTop(28);
    int inputWidth = 80;
    m_octaveInput->setBounds(
        octaveInputArea.getCentreX() - inputWidth/2,
        octaveInputArea.getY(),
        inputWidth,
        octaveInputArea.getHeight()
    );
    
    // Bottom buttons with proper spacing now that we have full height
    // Calculate remaining space for buttons
    bounds.removeFromTop(8); // Small spacing before buttons
    
    // Plugin button - consistent height
    auto pluginButtonBounds = bounds.removeFromTop(36);
    m_pluginButton->setBounds(pluginButtonBounds.reduced(4, 2));
    
    bounds.removeFromTop(6); // Spacing between buttons
    
    // Accumulator button - same height as plugin button
    auto accumButtonBounds = bounds.removeFromTop(36);
    m_accumulatorButton->setBounds(accumButtonBounds.reduced(4, 2));
}

void TrackControlStrip::updateFromTrack(const TrackViewModel& track)
{
    m_trackName = track.getName();
    m_trackNameEditor->setText(m_trackName);
    m_trackColor = track.getTrackColor();
    m_isMuted = track.isMuted();
    m_isSoloed = track.isSoloed();
    
    // Update MIDI routing dropdown
    switch (track.getMidiRoutingMode()) {
        case TrackViewModel::MidiRoutingMode::PluginOnly:
            m_midiRoutingSelector->setSelectedId(1, juce::dontSendNotification);
            break;
        case TrackViewModel::MidiRoutingMode::ExternalOnly:
            m_midiRoutingSelector->setSelectedId(2, juce::dontSendNotification);
            break;
        case TrackViewModel::MidiRoutingMode::Both:
            m_midiRoutingSelector->setSelectedId(3, juce::dontSendNotification);
            break;
    }
    
    // Update control states - use track color when not active
    m_muteButton->setColor(m_isMuted ? 
        juce::Colour(DesignTokens::Colors::ACCENT_RED) : 
        m_trackColor);
    m_soloButton->setColor(m_isSoloed ? 
        juce::Colour(DesignTokens::Colors::ACCENT_AMBER) : 
        m_trackColor);
    
    // Update all slider colors to match track
    m_maxPulseLengthSlider->setTrackColor(m_trackColor);
    m_swingSlider->setTrackColor(m_trackColor);
    
    if (m_channelSelector) m_channelSelector->setSelectedId(track.getMidiChannel());
    if (m_voiceModeToggle) {
        bool isPoly = (track.getVoiceMode() == TrackViewModel::VoiceMode::Poly);
        m_voiceModeToggle->setChecked(isPoly);
    }
    if (m_swingSlider) m_swingSlider->setValue(track.getSwing()); // Already 0-1 range
    if (m_octaveInput) m_octaveInput->setValue(static_cast<float>(track.getOctaveOffset()));
    
    // Update button colors to match track
    if (m_pluginButton) m_pluginButton->setColor(m_trackColor);
    if (m_accumulatorButton) m_accumulatorButton->setColor(m_trackColor);
    
    repaint();
}

void TrackControlStrip::setSelected(bool selected)
{
    m_isSelected = selected;
    repaint();
}

//==============================================================================
// TrackSidebar Implementation
//==============================================================================

TrackSidebar::TrackSidebar()
{
    // Create container for track strips
    m_trackContainer = std::make_unique<juce::Component>();
    addAndMakeVisible(m_trackContainer.get());
    
    // Register as TrackManager listener
    auto& trackManager = TrackManager::getInstance();
    trackManager.addListener(this);
    
    // Initialize with tracks from TrackManager
    const auto& tracks = trackManager.getAllTracks();
    setTrackCount(static_cast<int>(tracks.size()));
    
    // Start timer for periodic updates
    startTimerHz(10);
}

TrackSidebar::~TrackSidebar()
{
    // Unregister from TrackManager
    auto& trackManager = TrackManager::getInstance();
    trackManager.removeListener(this);
    
    stopTimer();
}

void TrackSidebar::paint(juce::Graphics& g)
{
    // Dark background
    g.fillAll(juce::Colour(DesignTokens::Colors::BG_DARK));
    
    // Right border
    g.setColour(juce::Colour(DesignTokens::Colors::BORDER).withAlpha(0.3f));
    g.drawVerticalLine(getWidth() - 1, 0, getHeight());
}

void TrackSidebar::resized()
{
    auto bounds = getLocalBounds();
    
    // Track container fills available bounds (scrolling handled by parent viewport)
    m_trackContainer->setBounds(bounds);
    
    // Update track container layout
    updateTrackLayout();
}

void TrackSidebar::setTrackCount(int count)
{
    m_trackStrips.clear();
    
    int tracksToShow = std::max(1, count);
    
    for (int i = 0; i < tracksToShow; ++i)
    {
        auto strip = std::make_unique<TrackControlStrip>(i);
        
        // Connect callbacks
        strip->onTrackSelected = [this](int index) {
            handleTrackSelection(index);
        };
        
        strip->onMuteChanged = [this](int index, bool muted) {
            handleTrackParameter(index, "mute", muted ? 1.0f : 0.0f);
        };
        
        strip->onSoloChanged = [this](int index, bool soloed) {
            handleTrackParameter(index, "solo", soloed ? 1.0f : 0.0f);
        };
        
        strip->onChannelChanged = [this](int index, int channel) {
            handleTrackParameter(index, "channel", static_cast<float>(channel));
        };
        
        strip->onVoiceModeChanged = [this](int index, bool polyMode) {
            handleTrackParameter(index, "voiceMode", polyMode ? 1.0f : 0.0f);
        };
        
        strip->onMaxPulseLengthChanged = [this](int index, int maxPulseLength) {
            handleTrackParameter(index, "maxPulseLength", static_cast<float>(maxPulseLength));
        };
        
        strip->onDivisionChanged = [this](int index, int division) {
            handleTrackParameter(index, "division", static_cast<float>(division));
        };
        
        strip->onSwingChanged = [this](int index, float swing) {
            handleTrackParameter(index, "swing", swing);
        };
        
        strip->onOctaveChanged = [this](int index, int octave) {
            handleTrackParameter(index, "octave", static_cast<float>(octave));
        };
        
        strip->onPluginBrowserRequested = [this](int index) {
            DBG("Plugin browser requested for track " << index);
            // Forward the browser request
            if (onPluginBrowserRequested)
                onPluginBrowserRequested(index);
        };
        
        strip->onPluginEditorRequested = [this](int index) {
            DBG("Plugin editor requested for track " << index);
            // Forward the editor request
            if (onPluginEditorRequested)
                onPluginEditorRequested(index);
        };
        
        strip->onAccumulatorButtonClicked = [this](int index) {
            handleTrackParameter(index, "openAccumulator", 1.0f);
            DBG("Accumulator button clicked for track " << index);
        };
        
        m_trackContainer->addAndMakeVisible(strip.get());
        m_trackStrips.push_back(std::move(strip));
    }
    
    // Select first track by default
    if (!m_trackStrips.empty())
    {
        selectTrack(0);
    }
    
    updateTrackLayout();
}

void TrackSidebar::updateTrack(int index, const TrackViewModel& track)
{
    if (index >= 0 && index < static_cast<int>(m_trackStrips.size()))
    {
        m_trackStrips[index]->updateFromTrack(track);
    }
}

void TrackSidebar::selectTrack(int index)
{
    if (index >= 0 && index < static_cast<int>(m_trackStrips.size()))
    {
        // Deselect previous
        if (m_selectedTrackIndex >= 0 && m_selectedTrackIndex < static_cast<int>(m_trackStrips.size()))
        {
            m_trackStrips[m_selectedTrackIndex]->setSelected(false);
        }
        
        // Select new
        m_selectedTrackIndex = index;
        m_trackStrips[index]->setSelected(true);
        
        if (onTrackSelected)
        {
            onTrackSelected(index);
        }
    }
}

void TrackSidebar::refreshTracks()
{
    // Rebuild track strips based on current track count
    auto& trackManager = TrackManager::getInstance();
    setTrackCount(trackManager.getTrackCount());
}

void TrackSidebar::timerCallback()
{
    // Periodic UI updates can be handled here if needed
    // For now, keeping empty as updates are event-driven
}

void TrackSidebar::updateTrackLayout()
{
    // Vertical list layout, allow container to grow for viewport scrolling
    // Start directly at top with no offset
    int y = 0;
    const int gap = 8;
    for (auto& strip : m_trackStrips)
    {
        if (strip)
        {
            // Position track strip with consistent height matching stage cards
            strip->setBounds(0, y, m_trackContainer->getWidth(), 512);  // Adjusted for button height
            y += 512 + gap;  // Adjusted for button height
        }
    }
    // Set container size to accommodate all tracks
    m_trackContainer->setSize(getWidth(), std::max(getHeight(), y));
}

void TrackSidebar::handleTrackSelection(int index)
{
    selectTrack(index);
}

void TrackSidebar::handleTrackParameter(int trackIndex, const juce::String& param, float value)
{
    if (onTrackParameterChanged)
    {
        onTrackParameterChanged(trackIndex, param, value);
    }
    
    DBG("Track " << trackIndex << " " << param << " changed to: " << value);
}

//==============================================================================
// TrackManager::Listener Implementation
//==============================================================================

void TrackSidebar::trackAdded(int trackIndex)
{
    // Refresh all tracks
    auto& trackManager = TrackManager::getInstance();
    const auto& tracks = trackManager.getAllTracks();
    setTrackCount(static_cast<int>(tracks.size()));
}

void TrackSidebar::trackRemoved(int trackIndex)
{
    // Refresh all tracks
    auto& trackManager = TrackManager::getInstance();
    const auto& tracks = trackManager.getAllTracks();
    setTrackCount(static_cast<int>(tracks.size()));
}

void TrackSidebar::trackParametersChanged(int trackIndex)
{
    // Update specific track display if needed
    if (trackIndex >= 0 && trackIndex < static_cast<int>(m_trackStrips.size()))
    {
        m_trackStrips[trackIndex]->repaint();
    }
}

void TrackSidebar::trackPluginChanged(int trackIndex)
{
    // Update plugin button state
    if (trackIndex >= 0 && trackIndex < static_cast<int>(m_trackStrips.size()))
    {
        auto& trackManager = TrackManager::getInstance();
        auto* pluginState = trackManager.getPluginState(trackIndex, true);
        
        if (pluginState && pluginState->hasPlugin)
        {
            // Update button to show plugin is loaded
            // Note: We'll add updatePluginState method to TrackControlStrip
            m_trackStrips[trackIndex]->repaint();
        }
        else
        {
            m_trackStrips[trackIndex]->repaint();
        }
    }
}

} // namespace HAM::UI