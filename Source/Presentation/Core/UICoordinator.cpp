/*
  ==============================================================================

    UICoordinator.cpp
    UI component orchestration and layout management for HAM sequencer

  ==============================================================================
*/

#include "UICoordinator.h"
#include "AppController.h"
#include "Presentation/Views/TransportBar.h"
#include "Presentation/Views/StageGrid.h"
#include "Presentation/Views/TrackSidebar.h"
#include "Presentation/Views/MixerView.h"
#include "Presentation/Views/PluginBrowser.h"
#include "../../UI/Components/HAMComponentLibrary.h"

namespace HAM {
namespace UI {

// Use the component library types (already in HAM::UI namespace)

//==============================================================================
UICoordinator::UICoordinator(AppController& controller)
    : m_controller(controller)
{
    createUIComponents();
    setupEventHandlers();
}

UICoordinator::~UICoordinator() = default;

//==============================================================================
void UICoordinator::createUIComponents()
{
    // Create transport bar
    m_transportBar = std::make_unique<TransportBar>();
    addAndMakeVisible(m_transportBar.get());
    
    // Create view toggle buttons
    m_sequencerTabButton = std::make_unique<ModernButton>(
        "SEQUENCER", ModernButton::Style::Solid);
    m_sequencerTabButton->setColor(juce::Colour(0xFF00CCFF));
    addAndMakeVisible(m_sequencerTabButton.get());
    
    m_mixerTabButton = std::make_unique<ModernButton>(
        "MIXER", ModernButton::Style::Outline);
    m_mixerTabButton->setColor(juce::Colour(0xFF00CCFF));
    addAndMakeVisible(m_mixerTabButton.get());
    
    m_settingsTabButton = std::make_unique<ModernButton>(
        "SETTINGS", ModernButton::Style::Outline);
    m_settingsTabButton->setColor(juce::Colour(0xFF00CCFF));
    addAndMakeVisible(m_settingsTabButton.get());
    
    // Create main content container
    m_contentContainer = std::make_unique<juce::Component>();
    addAndMakeVisible(m_contentContainer.get());
    
    // Create sequencer page components
    m_sequencerPage = std::make_unique<juce::Component>();
    m_contentContainer->addAndMakeVisible(m_sequencerPage.get());
    
    // Create track sidebar
    m_trackSidebar = std::make_unique<TrackSidebar>();
    m_trackSidebar->setTrackCount(1); // Start with 1 track
    m_sequencerPage->addAndMakeVisible(m_trackSidebar.get());
    
    // Create stage grid
    m_stageGrid = std::make_unique<StageGrid>();
    m_stageGrid->setTrackCount(1); // Start with 1 track (creates 8 stage cards)
    
    // Wrap StageGrid in a viewport for scrolling
    m_stageViewport = std::make_unique<juce::Viewport>("stageViewport");
    m_stageViewport->setViewedComponent(m_stageGrid.get(), false);
    m_stageViewport->setScrollBarsShown(true, true); // Both vertical and horizontal scrolling
    m_stageViewport->setScrollBarThickness(10);
    m_sequencerPage->addAndMakeVisible(m_stageViewport.get());
    
    // Create mixer view
    m_mixerView = std::make_unique<MixerView>();
    m_contentContainer->addAndMakeVisible(m_mixerView.get());
    m_mixerView->setVisible(false); // Start with sequencer view
    
    // Create plugin browser (initially hidden - don't add to component yet)
    m_pluginBrowser = std::make_unique<PluginBrowser>();
    m_pluginBrowser->setVisible(false);
    // Don't add it as child - we'll add it only when needed
    
    // Initialize with sequencer view
    setActiveView(ViewMode::Sequencer);
}

void UICoordinator::setupEventHandlers()
{
    // Transport bar handlers
    m_transportBar->onPlayStateChanged = [this](bool playing)
    {
        if (playing)
            m_controller.play();
        else
            m_controller.stop();
    };
    
    m_transportBar->onBPMChanged = [this](float bpm)
    {
        m_controller.setBPM(bpm);
    };
    
    m_transportBar->onMidiMonitorToggled = [this](bool enabled)
    {
        m_controller.setMidiMonitorEnabled(enabled);
    };
    
    // View switching
    m_sequencerTabButton->onClick = [this]()
    {
        setActiveView(ViewMode::Sequencer);
    };
    
    m_mixerTabButton->onClick = [this]()
    {
        setActiveView(ViewMode::Mixer);
    };
    
    m_settingsTabButton->onClick = [this]()
    {
        setActiveView(ViewMode::Settings);
    };
    
    // Stage grid handlers
    m_stageGrid->onStageParameterChanged = [this](int track, int stage, 
                                                   const juce::String& param, 
                                                   float value)
    {
        m_controller.updateStageParameter(track, stage, param, value);
    };
    
    m_stageGrid->onHAMEditorRequested = [this](int stage)
    {
        showHAMEditor(stage);
    };
    
    // TODO: Connect track sidebar callbacks when UI components are fully implemented
    // m_trackSidebar->onTrackMuted = [this](int trackIndex, bool muted)
    // {
    //     m_controller.setTrackMute(trackIndex, muted);
    // };
    
    // m_trackSidebar->onTrackSoloed = [this](int trackIndex, bool solo)
    // {
    //     m_controller.setTrackSolo(trackIndex, solo);
    // };
    
    // m_trackSidebar->onTrackVolumeChanged = [this](int trackIndex, float volume)
    // {
    //     m_controller.setTrackVolume(trackIndex, volume);
    // };
    
    // m_trackSidebar->onTrackPanChanged = [this](int trackIndex, float pan)
    // {
    //     m_controller.setTrackPan(trackIndex, pan);
    // };
    
    // Mixer view handlers
    m_mixerView->onAliasInstrumentPlugin = [this](int trackIndex)
    {
        showPluginBrowser(trackIndex, false);
    };
    
    m_mixerView->onAddFxPlugin = [this](int trackIndex)
    {
        showPluginBrowser(trackIndex, true);
    };
}

//==============================================================================
void UICoordinator::setActiveView(ViewMode mode)
{
    m_activeView = mode;
    
    // Update visibility
    m_sequencerPage->setVisible(mode == ViewMode::Sequencer);
    m_mixerView->setVisible(mode == ViewMode::Mixer);
    // Settings view will be implemented later
    
    updateViewButtonStates();
    resized();
}

void UICoordinator::showPluginBrowser(int trackIndex, bool forEffects)
{
    if (!m_pluginBrowser)
        return;
    
    // Add to component hierarchy if not already added
    if (!m_pluginBrowser->getParentComponent())
    {
        addAndMakeVisible(m_pluginBrowser.get());
    }
    
    // TODO: Configure plugin browser for the specific track
    m_pluginBrowser->setVisible(true);
    m_pluginBrowser->toFront(true);
    
    // Setup callback
    m_pluginBrowser->onPluginChosen = [this, trackIndex, forEffects](const juce::PluginDescription& desc)
    {
        if (forEffects)
        {
            // TODO: Add effect to track
        }
        else
        {
            // TODO: Set instrument for track
        }
        hidePluginBrowser();
    };
    
    resized();
}

void UICoordinator::hidePluginBrowser()
{
    if (m_pluginBrowser)
    {
        m_pluginBrowser->setVisible(false);
        // Optionally remove from component hierarchy
        if (m_pluginBrowser->getParentComponent())
        {
            removeChildComponent(m_pluginBrowser.get());
        }
        resized();
    }
}

void UICoordinator::showHAMEditor(int stageIndex)
{
    m_hamEditorVisible = true;
    m_hamEditorStageIndex = stageIndex;
    
    // TODO: Create and show HAMEditorPanel
    // if (m_hamEditor)
    // {
    //     m_hamEditor->setExpanded(true);
    //     m_hamEditor->setCurrentStage(0, stageIndex);
    // }
    
    resized();
}

void UICoordinator::hideHAMEditor()
{
    m_hamEditorVisible = false;
    m_hamEditorStageIndex = -1;
    
    // TODO: Hide HAMEditorPanel
    // if (m_hamEditor)
    //     m_hamEditor->setExpanded(false);
    
    resized();
}

void UICoordinator::showSettings()
{
    setActiveView(ViewMode::Settings);
}

void UICoordinator::hideSettings()
{
    setActiveView(ViewMode::Sequencer);
}

//==============================================================================
void UICoordinator::paint(juce::Graphics& g)
{
    // Dark background matching Pulse aesthetic
    g.fillAll(juce::Colour(0xFF0A0A0A));
}

void UICoordinator::resized()
{
    auto bounds = getLocalBounds();
    
    // Transport bar at top
    if (m_transportBar)
    {
        m_transportBar->setBounds(bounds.removeFromTop(TRANSPORT_HEIGHT));
    }
    
    // Tab bar
    auto tabBarArea = bounds.removeFromTop(TAB_BAR_HEIGHT);
    int tabWidth = 120;
    int tabX = tabBarArea.getCentreX() - (tabWidth * 3) / 2;
    
    if (m_sequencerTabButton)
        m_sequencerTabButton->setBounds(tabX, tabBarArea.getY(), tabWidth, TAB_BAR_HEIGHT);
    if (m_mixerTabButton)
        m_mixerTabButton->setBounds(tabX + tabWidth, tabBarArea.getY(), tabWidth, TAB_BAR_HEIGHT);
    if (m_settingsTabButton)
        m_settingsTabButton->setBounds(tabX + tabWidth * 2, tabBarArea.getY(), tabWidth, TAB_BAR_HEIGHT);
    
    // Content area
    if (m_contentContainer)
    {
        m_contentContainer->setBounds(bounds);
        
        // Also set the sequencer page bounds
        if (m_sequencerPage)
        {
            m_sequencerPage->setBounds(m_contentContainer->getLocalBounds());
        }
        
        // Layout based on active view
        if (m_activeView == ViewMode::Sequencer)
        {
            layoutSequencerView();
        }
        else if (m_activeView == ViewMode::Mixer)
        {
            layoutMixerView();
        }
    }
    
    // Plugin browser overlay (if visible)
    if (m_pluginBrowser && m_pluginBrowser->isVisible())
    {
        auto browserBounds = getLocalBounds();
        browserBounds.reduce(50, 50); // Inset from edges
        m_pluginBrowser->setBounds(browserBounds);
    }
}

void UICoordinator::layoutSequencerView()
{
    if (!m_sequencerPage)
        return;
    
    auto bounds = m_sequencerPage->getLocalBounds();
    
    // Track sidebar on left
    if (m_trackSidebar)
    {
        m_trackSidebar->setBounds(bounds.removeFromLeft(SIDEBAR_WIDTH));
    }
    
    // HAM Editor at bottom (if visible)
    if (m_hamEditorVisible)
    {
        // Reserve space for HAM editor
        bounds.removeFromBottom(HAM_EDITOR_HEIGHT);
        // TODO: Position HAMEditorPanel
    }
    
    // Stage grid in remaining space
    if (m_stageViewport)
    {
        m_stageViewport->setBounds(bounds);
        
        // Set stage grid size (8 stages)
        if (m_stageGrid)
        {
            const int gridWidth = 8 * STAGE_CARD_WIDTH + 7 * 10; // 8 cards + gaps
            // Calculate total height needed based on track count
            const int numTracks = 1; // TODO: Get actual track count from model
            const int verticalPadding = 1; // Matches StageGrid::resized()
            const int gridHeight = numTracks * STAGE_CARD_HEIGHT + (numTracks - 1) * verticalPadding;
            m_stageGrid->setSize(gridWidth, gridHeight);
        }
    }
}

void UICoordinator::layoutMixerView()
{
    if (!m_mixerView)
        return;
    
    m_mixerView->setBounds(m_contentContainer->getLocalBounds());
}

void UICoordinator::updateViewButtonStates()
{
    // Update button styles based on active view
    // TODO: Update button styles when ModernButton::setStyle is implemented
    // if (m_sequencerTabButton)
    // {
    //     m_sequencerTabButton->setStyle(
    //         m_activeView == ViewMode::Sequencer ? 
    //         ModernButton::Style::Solid : 
    //         ModernButton::Style::Outline
    //     );
    // }
    
    // if (m_mixerTabButton)
    // {
    //     m_mixerTabButton->setStyle(
    //         m_activeView == ViewMode::Mixer ? 
    //         ModernButton::Style::Solid : 
    //         ModernButton::Style::Outline
    //     );
    // }
    
    // if (m_settingsTabButton)
    // {
    //     m_settingsTabButton->setStyle(
    //         m_activeView == ViewMode::Settings ? 
    //         ModernButton::Style::Solid : 
    //         ModernButton::Style::Outline
    //     );
    // }
}

} // namespace UI
} // namespace HAM