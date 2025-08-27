/*
  ==============================================================================

    UICoordinator.h
    UI component orchestration and layout management for HAM sequencer

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <memory>
#include "Presentation/Core/ComponentBase.h"

namespace HAM {

// Forward declare HAMAudioProcessor
class HAMAudioProcessor;

namespace UI {

// Forward declarations
class AppController;
class TransportBar;
class StageGrid;
class TrackSidebar;
class MixerView;
class PluginBrowser;
class ModernButton;

//==============================================================================
/**
 * Manages UI component creation, layout, and event routing
 * Coordinates between UI components and the AppController
 */
class UICoordinator : public juce::Component
{
public:
    //==============================================================================
    UICoordinator(AppController& controller);
    ~UICoordinator() override;
    
    // Set the audio processor (needed for MixerView)
    void setAudioProcessor(HAMAudioProcessor* processor);

    //==============================================================================
    // View switching
    enum class ViewMode
    {
        Sequencer,
        Mixer,
        Settings
    };
    
    void setActiveView(ViewMode mode);
    ViewMode getActiveView() const { return m_activeView; }
    
    // UI component access
    TransportBar* getTransportBar() { return m_transportBar.get(); }
    StageGrid* getStageGrid() { return m_stageGrid.get(); }
    TrackSidebar* getTrackSidebar() { return m_trackSidebar.get(); }
    MixerView* getMixerView() { return m_mixerView.get(); }
    
    // Plugin browser
    void showPluginBrowser(int trackIndex, bool forEffects = false);
    void hidePluginBrowser();
    
    // HAM Editor panel
    void showHAMEditor(int stageIndex);
    void hideHAMEditor();
    bool isHAMEditorVisible() const { return m_hamEditorVisible; }
    
    // Settings panel
    void showSettings();
    void hideSettings();
    
    //==============================================================================
    void paint(juce::Graphics&) override;
    void resized() override;
    
private:
    //==============================================================================
    void createUIComponents();
    void setupEventHandlers();
    void layoutSequencerView();
    void layoutMixerView();
    void updateViewButtonStates();
    
    // Reference to app controller
    AppController& m_controller;
    
    // Main UI sections
    std::unique_ptr<TransportBar> m_transportBar;
    std::unique_ptr<juce::Component> m_contentContainer;
    
    // View switching buttons
    std::unique_ptr<ModernButton> m_sequencerTabButton;
    std::unique_ptr<ModernButton> m_mixerTabButton;
    std::unique_ptr<ModernButton> m_settingsTabButton;
    
    // Sequencer view components
    std::unique_ptr<juce::Component> m_sequencerPage;
    std::unique_ptr<juce::Component> m_sequencerContent;  // Container for both sidebar and grid
    std::unique_ptr<TrackSidebar> m_trackSidebar;
    std::unique_ptr<juce::Viewport> m_sequencerViewport;  // Single viewport for synchronized scrolling
    std::unique_ptr<StageGrid> m_stageGrid;
    // std::unique_ptr<HAMEditorPanel> m_hamEditor; // TODO: Implement
    
    // Mixer view components
    std::unique_ptr<MixerView> m_mixerView;
    std::unique_ptr<juce::Component> m_mixerPlaceholder; // Placeholder when no AudioProcessor
    
    // Plugin browser (overlay)
    std::unique_ptr<PluginBrowser> m_pluginBrowser;
    
    // Current view state
    ViewMode m_activeView = ViewMode::Sequencer;
    bool m_hamEditorVisible = false;
    int m_hamEditorStageIndex = -1;
    
    // Layout constants
    static constexpr int TRANSPORT_HEIGHT = 80;
    static constexpr int TAB_BAR_HEIGHT = 40;
    static constexpr int SIDEBAR_WIDTH = 250;  // Wider for better layout
    static constexpr int HAM_EDITOR_HEIGHT = 300;
    static constexpr int STAGE_CARD_WIDTH = 140;
    static constexpr int STAGE_CARD_HEIGHT = 480;  // Match TrackSidebar height
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UICoordinator)
};

} // namespace UI
} // namespace HAM