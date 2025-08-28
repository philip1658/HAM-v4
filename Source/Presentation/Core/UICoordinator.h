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
#include "Presentation/Components/IconButton.h"

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
class ScaleSlotSelector;
class ScaleSlotViewModel;
class ScaleBrowser;

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
    ScaleSlotSelector* getScaleSlotSelector() { return m_scaleSlotSelector.get(); }
    
    // Plugin browser
    void showPluginBrowser(int trackIndex, bool forEffects = false);
    void hidePluginBrowser();
    
    // Scale browser
    void showScaleBrowser(int slotIndex);
    void hideScaleBrowser();
    
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
    
    // View switching buttons (using icons)
    std::unique_ptr<IconButton> m_sequencerTabButton;
    std::unique_ptr<IconButton> m_mixerTabButton;
    // Settings button removed - std::unique_ptr<IconButton> m_settingsTabButton;
    
    // Track management buttons (using icons)
    std::unique_ptr<IconButton> m_addTrackButton;
    std::unique_ptr<IconButton> m_removeTrackButton;
    
    // Sequencer view components
    std::unique_ptr<juce::Component> m_sequencerPage;
    std::unique_ptr<juce::Component> m_sequencerContent;  // Container for both sidebar and grid
    std::unique_ptr<TrackSidebar> m_trackSidebar;
    std::unique_ptr<juce::Viewport> m_sequencerViewport;  // Single viewport for synchronized scrolling
    std::unique_ptr<StageGrid> m_stageGrid;
    std::unique_ptr<ScaleSlotSelector> m_scaleSlotSelector;
    std::unique_ptr<ScaleSlotViewModel> m_scaleSlotViewModel;
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
    
    // Layout constants - UNIFIED HEIGHTS
    static constexpr int TRANSPORT_HEIGHT = 52;  // Unified bar height
    static constexpr int TAB_BAR_HEIGHT = 52;    // Same height for both bars
    static constexpr int SIDEBAR_WIDTH = 250;  // Wider for better layout
    static constexpr int HAM_EDITOR_HEIGHT = 300;
    static constexpr int STAGE_CARD_WIDTH = 140;  // Default width (now used as minimum)
    static constexpr int STAGE_CARD_HEIGHT = 510;  // Increased by 30px for Accumulator button
    
    // Responsive sizing constants
    static constexpr int MIN_STAGE_CARD_WIDTH = 120;  // Minimum card width to remain usable
    static constexpr int MAX_STAGE_CARD_WIDTH = 200;  // Maximum card width to prevent over-stretching
    static constexpr float STAGE_CARD_ASPECT_RATIO = 140.0f / 510.0f;  // Width to height ratio
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UICoordinator)
};

} // namespace UI
} // namespace HAM