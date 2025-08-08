/*
  ==============================================================================

    TrackSidebar.cpp
    Implementation of Track control sidebar for HAM sequencer
    Fixed 480px height with all controls always visible

  ==============================================================================
*/

#include "TrackSidebar.h"
#include "../../Infrastructure/Messaging/MessageDispatcher.h"
#include "../../Infrastructure/Messaging/MessageTypes.h"
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
    m_trackNameEditor->setFont(juce::Font(16.0f, juce::Font::bold));
    m_trackNameEditor->setJustification(juce::Justification::centredLeft);
    m_trackNameEditor->onReturnKey = [this] {
        m_trackName = m_trackNameEditor->getText();
        m_trackNameEditor->giveAwayKeyboardFocus();
    };
    m_trackNameEditor->onFocusLost = [this] {
        m_trackName = m_trackNameEditor->getText();
    };
    addAndMakeVisible(m_trackNameEditor.get());
    
    // Mute button
    m_muteButton = std::make_unique<ModernButton>("M", ModernButton::Style::Outline);
    m_muteButton->onClick = [this]() {
        m_isMuted = !m_isMuted;
        m_muteButton->setColor(m_isMuted ? 
            juce::Colour(DesignTokens::Colors::ACCENT_RED) : 
            m_trackColor.withAlpha(0.3f));
        if (onMuteChanged) onMuteChanged(m_trackIndex, m_isMuted);
    };
    addAndMakeVisible(m_muteButton.get());
    
    // Solo button
    m_soloButton = std::make_unique<ModernButton>("S", ModernButton::Style::Outline);
    m_soloButton->onClick = [this]() {
        m_isSoloed = !m_isSoloed;
        m_soloButton->setColor(m_isSoloed ? 
            juce::Colour(DesignTokens::Colors::ACCENT_AMBER) : 
            m_trackColor.withAlpha(0.3f));
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
    
    // Swing slider
    m_swingLabel = createLabel("Swing");
    m_swingSlider = std::make_unique<ModernSlider>(false); // Horizontal
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
    
    // Plugin button with icon - use distinct bright blue color
    m_pluginButton = std::make_unique<ModernButton>("PLUGIN", ModernButton::Style::Gradient);
    m_pluginButton->setColor(juce::Colour(0xFF00AAFF));  // Bright cyan-blue
    m_pluginButton->onClick = [this]() {
        DBG("Plugin button clicked for track " << m_trackIndex);
        if (onPluginButtonClicked) onPluginButtonClicked(m_trackIndex);
    };
    addAndMakeVisible(m_pluginButton.get());
    
    // Accumulator button with icon - use distinct bright amber/orange color
    m_accumulatorButton = std::make_unique<ModernButton>("ACCUM", ModernButton::Style::Gradient);
    m_accumulatorButton->setColor(juce::Colour(0xFFFF8800));  // Bright orange
    m_accumulatorButton->onClick = [this]() {
        DBG("Accumulator button clicked for track " << m_trackIndex);
        if (onAccumulatorButtonClicked) onAccumulatorButtonClicked(m_trackIndex);
    };
    addAndMakeVisible(m_accumulatorButton.get());
}

std::unique_ptr<juce::Label> TrackControlStrip::createLabel(const juce::String& text)
{
    auto label = std::make_unique<juce::Label>(text, text);
    label->setFont(juce::Font(11.0f));
    label->setColour(juce::Label::textColourId, juce::Colour(DesignTokens::Colors::TEXT_MUTED));
    label->setJustificationType(juce::Justification::left);
    addAndMakeVisible(label.get());
    return label;
}

void TrackControlStrip::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    
    // Background based on selection state
    if (m_isSelected)
    {
        g.setColour(juce::Colour(DesignTokens::Colors::BG_RAISED));
        g.fillRoundedRectangle(bounds.toFloat(), 4.0f);
    }
    else
    {
        g.setColour(juce::Colour(DesignTokens::Colors::BG_PANEL));
        g.fillRoundedRectangle(bounds.toFloat(), 4.0f);
    }
    
    // Left color indicator strip
    auto colorStrip = bounds.removeFromLeft(4);
    g.setColour(m_trackColor);
    g.fillRoundedRectangle(colorStrip.toFloat(), 4.0f);
    
    // Subtle border
    g.setColour(juce::Colour(DesignTokens::Colors::BORDER).withAlpha(0.3f));
    g.drawRoundedRectangle(bounds.toFloat(), 4.0f, 1.0f);
}

void TrackControlStrip::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromLeft(8); // Space for color strip
    bounds.reduce(8, 8); // Overall padding
    
    // Header section (40px instead of 48px)
    auto headerArea = bounds.removeFromTop(40);
    
    // Track name on left
    m_trackNameEditor->setBounds(headerArea.removeFromLeft(100));
    
    // Mute/Solo buttons on right
    auto buttonArea = headerArea.removeFromRight(80);
    m_muteButton->setBounds(buttonArea.removeFromLeft(35));
    buttonArea.removeFromLeft(8);
    m_soloButton->setBounds(buttonArea.removeFromLeft(35));
    
    bounds.removeFromTop(12); // Reduced spacing
    
    // More compact control layout helper
    auto layoutControl = [&bounds](juce::Component* label, juce::Component* control, int height = 28) {
        auto row = bounds.removeFromTop(height + 14); // Reduced heights
        if (label) label->setBounds(row.removeFromTop(14));
        control->setBounds(row);
        bounds.removeFromTop(6); // Reduced spacing between controls
    };
    
    // MIDI Channel
    layoutControl(m_channelLabel.get(), m_channelSelector.get());
    
    // Voice Mode (more compact)
    auto voiceRow = bounds.removeFromTop(42);
    m_voiceModeLabel->setBounds(voiceRow.removeFromTop(14));
    auto toggleRow = voiceRow.removeFromTop(28);
    auto monoLabel = toggleRow.removeFromLeft(40);
    m_voiceModeToggle->setBounds(toggleRow.removeFromLeft(60));
    auto polyLabel = toggleRow.removeFromLeft(40);
    bounds.removeFromTop(6);
    
    // Max Pulse Length
    layoutControl(m_maxPulseLengthLabel.get(), m_maxPulseLengthSlider.get());
    
    // Swing
    layoutControl(m_swingLabel.get(), m_swingSlider.get());
    
    // Division
    layoutControl(m_divisionLabel.get(), m_divisionControl.get());
    
    // Octave (more compact)
    auto octaveRow = bounds.removeFromTop(42);
    m_octaveLabel->setBounds(octaveRow.removeFromTop(14));
    m_octaveInput->setBounds(octaveRow.removeFromLeft(80));
    bounds.removeFromTop(6);
    
    // Bottom buttons - ensure both are visible
    // We have remaining space after all controls are laid out
    bounds.removeFromBottom(8); // Bottom padding
    
    // Each button gets 36px height with 8px spacing between
    auto pluginRow = bounds.removeFromBottom(36);
    bounds.removeFromBottom(8); // Spacing between buttons
    auto accumRow = bounds.removeFromBottom(36);
    
    // Full width buttons with horizontal padding
    m_pluginButton->setBounds(pluginRow.reduced(4, 0));
    m_accumulatorButton->setBounds(accumRow.reduced(4, 0));
}

void TrackControlStrip::updateFromTrack(const TrackViewModel& track)
{
    m_trackName = track.getName();
    m_trackNameEditor->setText(m_trackName);
    m_trackColor = track.getTrackColor();
    m_isMuted = track.isMuted();
    m_isSoloed = track.isSoloed();
    
    // Update control states
    m_muteButton->setColor(m_isMuted ? 
        juce::Colour(DesignTokens::Colors::ACCENT_RED) : 
        m_trackColor.withAlpha(0.3f));
    m_soloButton->setColor(m_isSoloed ? 
        juce::Colour(DesignTokens::Colors::ACCENT_AMBER) : 
        m_trackColor.withAlpha(0.3f));
    
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
    
    // Keep button colors distinct (don't update to track color)
    // Plugin button stays blue, Accumulator button stays amber
    
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
    // Create header panel
    m_headerPanel = std::make_unique<Panel>(Panel::Style::Raised);
    addAndMakeVisible(m_headerPanel.get());
    
    // Header label
    m_headerLabel = std::make_unique<juce::Label>("tracks", "TRACKS");
    m_headerLabel->setFont(juce::Font(14.0f, juce::Font::bold));
    m_headerLabel->setColour(juce::Label::textColourId, juce::Colour(DesignTokens::Colors::TEXT_PRIMARY));
    m_headerPanel->addAndMakeVisible(m_headerLabel.get());
    
    // Add track button
    m_addTrackButton = std::make_unique<ModernButton>("+", ModernButton::Style::Gradient);
    m_addTrackButton->onClick = [this]() {
        if (onAddTrack) onAddTrack();
    };
    m_headerPanel->addAndMakeVisible(m_addTrackButton.get());
    
    // Create container for track strips (no viewport needed at fixed height)
    m_trackContainer = std::make_unique<juce::Component>();
    addAndMakeVisible(m_trackContainer.get());
    
    // Initialize with default tracks
    setTrackCount(1); // Start with 1 track at 480px height
    
    // Start timer for periodic updates
    startTimerHz(10);
}

TrackSidebar::~TrackSidebar()
{
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
    
    // Header at top
    m_headerPanel->setBounds(bounds.removeFromTop(HEADER_HEIGHT));
    
    // Layout header contents
    auto headerBounds = m_headerPanel->getLocalBounds().reduced(8);
    m_headerLabel->setBounds(headerBounds.removeFromLeft(100));
    m_addTrackButton->setBounds(headerBounds.removeFromRight(30));
    
    // Track container takes remaining space
    m_trackContainer->setBounds(bounds);
    
    // Update track container layout
    updateTrackLayout();
}

void TrackSidebar::setTrackCount(int count)
{
    m_trackStrips.clear();
    
    // For now, limit to 1 track since we have fixed 480px height
    // Multiple tracks would require horizontal scrolling or tabs
    int tracksToShow = std::min(count, 1);
    
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
        
        strip->onPluginButtonClicked = [this](int index) {
            handleTrackParameter(index, "openPlugin", 1.0f);
            DBG("Plugin button clicked for track " << index);
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

void TrackSidebar::timerCallback()
{
    // Periodic UI updates can be handled here if needed
    // For now, keeping empty as updates are event-driven
}

void TrackSidebar::updateTrackLayout()
{
    // Fixed layout - one track at 480px height (matches StageCard)
    if (!m_trackStrips.empty())
    {
        m_trackStrips[0]->setBounds(0, 0, m_trackContainer->getWidth(), TRACK_HEIGHT);
    }
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

} // namespace HAM::UI