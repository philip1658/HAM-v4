/*
  ==============================================================================

    TrackSidebar.h
    Track control sidebar for HAM sequencer
    
    Features:
    - Fixed 440px height per track (matches reduced StageCard height)
    - All controls always visible (no expand/collapse)
    - Track color indicator and name
    - Mute/Solo buttons
    - MIDI channel selector (1-16)
    - Mono/Poly voice mode toggle
    - Max Pulse Length control (1-8)
    - Division, Swing, and Octave controls
    - Plugin and Accumulator buttons

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../ViewModels/TrackViewModel.h"
#include "../../UI/Components/HAMComponentLibrary.h"
#include "../../UI/Components/HAMComponentLibraryExtended.h"
#include "../../Domain/Services/TrackManager.h"
#include "../../Domain/Models/Track.h"
#include "../../Domain/Types/MidiRoutingTypes.h"
#include <vector>
#include <functional>

namespace HAM::UI {

//==============================================================================
/**
 * Individual track control strip in the sidebar
 * Fixed height of 440px to match reduced StageCard
 */
class TrackControlStrip : public ResizableComponent
{
public:
    TrackControlStrip(int trackIndex);
    ~TrackControlStrip() override = default;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Update display from track data
    void updateFromTrack(const TrackViewModel& track);
    
    // Selection state
    void setSelected(bool selected);
    bool isSelected() const { return m_isSelected; }
    
    // Callbacks
    std::function<void(int trackIndex)> onTrackSelected;
    std::function<void(int trackIndex, bool muted)> onMuteChanged;
    std::function<void(int trackIndex, bool soloed)> onSoloChanged;
    std::function<void(int trackIndex, int channel)> onChannelChanged;
    std::function<void(int trackIndex, HAM::MidiRoutingMode mode)> onMidiRoutingChanged;
    std::function<void(int trackIndex, bool polyMode)> onVoiceModeChanged;
    std::function<void(int trackIndex, int maxPulseLength)> onMaxPulseLengthChanged;
    std::function<void(int trackIndex, int division)> onDivisionChanged;
    std::function<void(int trackIndex, float swing)> onSwingChanged;
    std::function<void(int trackIndex, int octave)> onOctaveChanged;
    std::function<void(int trackIndex)> onPluginBrowserRequested;  // For opening browser
    std::function<void(int trackIndex)> onPluginEditorRequested;  // For opening editor
    std::function<void(int trackIndex)> onAccumulatorButtonClicked;
    
private:
    int m_trackIndex;
    bool m_isSelected = false;
    
    // Track info
    juce::String m_trackName;
    juce::Colour m_trackColor;
    bool m_isMuted = false;
    bool m_isSoloed = false;
    
    // Header controls
    std::unique_ptr<juce::TextEditor> m_trackNameEditor;
    std::unique_ptr<ModernButton> m_muteButton;
    std::unique_ptr<ModernButton> m_soloButton;
    
    // Main controls (all always visible)
    std::unique_ptr<juce::Label> m_channelLabel;
    std::unique_ptr<juce::ComboBox> m_channelSelector;
    
    std::unique_ptr<juce::Label> m_midiRoutingLabel;
    std::unique_ptr<juce::ComboBox> m_midiRoutingSelector;
    
    std::unique_ptr<juce::Label> m_voiceModeLabel;
    std::unique_ptr<ModernToggle> m_voiceModeToggle;
    
    std::unique_ptr<juce::Label> m_maxPulseLengthLabel;
    std::unique_ptr<ModernSlider> m_maxPulseLengthSlider;
    
    std::unique_ptr<juce::Label> m_swingLabel;
    std::unique_ptr<ModernSlider> m_swingSlider;
    
    std::unique_ptr<juce::Label> m_divisionLabel;
    std::unique_ptr<SegmentedControl> m_divisionControl;
    
    std::unique_ptr<juce::Label> m_octaveLabel;
    std::unique_ptr<NumericInput> m_octaveInput;
    
    // Bottom buttons
    std::unique_ptr<ModernButton> m_pluginButton;
    std::unique_ptr<ModernButton> m_accumulatorButton;
    
    void setupControls();
    std::unique_ptr<juce::Label> createLabel(const juce::String& text);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackControlStrip)
};

//==============================================================================
/**
 * Track sidebar containing list of track controls
 */
class TrackSidebar : public ResizableComponent,
                     public TrackManager::Listener,
                     private juce::Timer
{
public:
    TrackSidebar();
    ~TrackSidebar() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // TrackManager::Listener overrides
    void trackAdded(int trackIndex) override;
    void trackRemoved(int trackIndex) override;
    void trackParametersChanged(int trackIndex) override;
    void trackPluginChanged(int trackIndex) override;
    
    // Track management
    void setTrackCount(int count);
    void updateTrack(int index, const TrackViewModel& track);
    void selectTrack(int index);
    int getSelectedTrackIndex() const { return m_selectedTrackIndex; }
    void refreshTracks();
    
    // Callbacks for track changes
    std::function<void(int trackIndex)> onTrackSelected;
    std::function<void(int trackIndex, const juce::String& param, float value)> onTrackParameterChanged;
    std::function<void()> onAddTrack;
    std::function<void(int trackIndex)> onRemoveTrack;
    std::function<void(int trackIndex)> onPluginBrowserRequested;  // For opening plugin browser
    std::function<void(int trackIndex)> onPluginEditorRequested;  // For opening plugin editor
    
private:
    void timerCallback() override;
    
    // Track controls
    std::vector<std::unique_ptr<TrackControlStrip>> m_trackStrips;
    int m_selectedTrackIndex = 0;
    
    // Track container (no scrolling needed at 440px)
    std::unique_ptr<juce::Component> m_trackContainer;
    
    // Layout helpers
    void updateTrackLayout();
    void handleTrackSelection(int index);
    void handleTrackParameter(int trackIndex, const juce::String& param, float value);
    
    static constexpr int MIN_WIDTH = 240;
    static constexpr int IDEAL_WIDTH = 250;  // Wider for better button layout
    static constexpr int TRACK_HEIGHT = 480;  // Full height now without header
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackSidebar)
};

} // namespace HAM::UI