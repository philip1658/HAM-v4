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
#include "Presentation/Views/ScaleSlotSelector.h"
#include "Presentation/Views/ScaleBrowser.h"
#include "Presentation/ViewModels/ScaleSlotViewModel.h"
#include "../../UI/Components/HAMComponentLibrary.h"
#include "Infrastructure/Audio/HAMAudioProcessor.h"
#include "../../Domain/Services/TrackManager.h"

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
void UICoordinator::setAudioProcessor(HAMAudioProcessor* processor)
{
    if (!processor)
        return;
    
    // Connect the stage grid to the processor for playhead tracking
    if (m_stageGrid)
    {
        m_stageGrid->setAudioProcessor(processor);
    }
    
    // Create the mixer view now that we have a processor
    if (!m_mixerView)
    {
        m_mixerView = std::make_unique<MixerView>(*processor);
        m_contentContainer->addAndMakeVisible(m_mixerView.get());
        
        // Set visibility based on current active view
        // But MixerView is created regardless so its plugin browser can be used
        m_mixerView->setVisible(m_activeView == ViewMode::Mixer);
        
        // MixerView provides the plugin browser for the entire application
        // It can be accessed even when MixerView itself is not visible
        
        // If we're currently showing the mixer view, layout it
        if (m_activeView == ViewMode::Mixer)
        {
            resized();
        }
    }
}

//==============================================================================
void UICoordinator::createUIComponents()
{
    // Create transport bar
    m_transportBar = std::make_unique<TransportBar>();
    addAndMakeVisible(m_transportBar.get());
    
    // Create view toggle buttons with symbolic icons
    m_sequencerTabButton = std::make_unique<IconButton>("Sequencer", IconButton::IconType::Sequencer);
    m_sequencerTabButton->setTooltip("Sequencer View");
    m_sequencerTabButton->setActive(true);  // Start with sequencer active
    addAndMakeVisible(m_sequencerTabButton.get());
    
    m_mixerTabButton = std::make_unique<IconButton>("Mixer", IconButton::IconType::Mixer);
    m_mixerTabButton->setTooltip("Mixer View");
    addAndMakeVisible(m_mixerTabButton.get());
    
    m_settingsTabButton = std::make_unique<IconButton>("Settings", IconButton::IconType::Settings);
    m_settingsTabButton->setTooltip("Settings");
    addAndMakeVisible(m_settingsTabButton.get());
    
    // Create track management buttons with icon style
    m_addTrackButton = std::make_unique<IconButton>("Add Track", IconButton::IconType::AddTrack);
    m_addTrackButton->setTooltip("Add Track");
    m_addTrackButton->setActiveColor(juce::Colour(0xFF00FF88));  // Green
    m_addTrackButton->onClick = [this]() {
        m_controller.addTrack();
        
        // Update the track sidebar and stage grid to show the new track
        auto& trackManager = TrackManager::getInstance();
        int newTrackCount = trackManager.getTrackCount();
        
        if (m_trackSidebar) {
            m_trackSidebar->setTrackCount(newTrackCount);
        }
        if (m_stageGrid) {
            m_stageGrid->setTrackCount(newTrackCount);
        }
        
        // Force the sequencer content to recalculate its size
        if (m_sequencerContent) {
            // Calculate new height based on track count
            const int trackHeight = 480; // Height per track
            const int trackGap = 8;
            int newHeight = newTrackCount * (trackHeight + trackGap);
            m_sequencerContent->setSize(m_sequencerContent->getWidth(), newHeight);
        }
        
        // Refresh the viewport
        if (m_sequencerViewport) {
            m_sequencerViewport->resized();
        }
        
        DBG("Track added - now have " << newTrackCount << " tracks");
    };
    addAndMakeVisible(m_addTrackButton.get());
    
    m_removeTrackButton = std::make_unique<IconButton>("Remove Track", IconButton::IconType::RemoveTrack);
    m_removeTrackButton->setTooltip("Remove Track");
    m_removeTrackButton->setActiveColor(juce::Colour(0xFFFF5555));  // Red
    m_removeTrackButton->onClick = [this]() {
        auto& trackManager = TrackManager::getInstance();
        int trackCount = trackManager.getTrackCount();
        
        if (trackCount > 1) {
            m_controller.removeTrack(trackCount - 1); // Remove last track
            
            // Get updated count after removal
            int newTrackCount = trackManager.getTrackCount();
            
            // Update the track sidebar and stage grid
            if (m_trackSidebar) {
                m_trackSidebar->setTrackCount(newTrackCount);
            }
            if (m_stageGrid) {
                m_stageGrid->setTrackCount(newTrackCount);
            }
            
            // Force the sequencer content to recalculate its size
            if (m_sequencerContent) {
                // Calculate new height based on track count
                const int trackHeight = 480; // Height per track
                const int trackGap = 8;
                int newHeight = newTrackCount * (trackHeight + trackGap);
                m_sequencerContent->setSize(m_sequencerContent->getWidth(), newHeight);
            }
            
            // Refresh the viewport
            if (m_sequencerViewport) {
                m_sequencerViewport->resized();
            }
            
            DBG("Track removed - now have " << newTrackCount << " tracks");
        } else {
            DBG("Cannot remove last track");
        }
    };
    addAndMakeVisible(m_removeTrackButton.get());
    
    // Create main content container
    m_contentContainer = std::make_unique<juce::Component>();
    addAndMakeVisible(m_contentContainer.get());
    
    // Create sequencer page components
    m_sequencerPage = std::make_unique<juce::Component>();
    m_contentContainer->addAndMakeVisible(m_sequencerPage.get());
    
    // Create a content container that holds both sidebar and grid
    m_sequencerContent = std::make_unique<juce::Component>();
    
    // Create track sidebar
    m_trackSidebar = std::make_unique<TrackSidebar>();
    // TrackSidebar automatically gets track count from TrackManager (8 tracks)
    m_sequencerContent->addAndMakeVisible(m_trackSidebar.get());
    
    // Create stage grid
    m_stageGrid = std::make_unique<StageGrid>();
    // StageGrid now defaults to 8 tracks matching TrackManager
    m_sequencerContent->addAndMakeVisible(m_stageGrid.get());
    
    // Wrap the content container in a viewport for synchronized scrolling
    m_sequencerViewport = std::make_unique<juce::Viewport>("sequencerViewport");
    m_sequencerViewport->setViewedComponent(m_sequencerContent.get(), false);
    m_sequencerViewport->setScrollBarsShown(true, true); // Both vertical and horizontal scrolling
    m_sequencerViewport->setScrollBarThickness(10);
    m_sequencerPage->addAndMakeVisible(m_sequencerViewport.get());
    
    // Create scale UI components
    m_scaleSlotViewModel = std::make_unique<ScaleSlotViewModel>();
    m_scaleSlotSelector = std::make_unique<ScaleSlotSelector>();
    m_scaleSlotSelector->setViewModel(m_scaleSlotViewModel.get());
    
    // Setup scale browser callback
    m_scaleSlotSelector->onScaleBrowserRequested = [this](int slotIndex) {
        showScaleBrowser(slotIndex);
    };
    
    // Scale slot selector is now added to the main coordinator, not the sequencer page
    addAndMakeVisible(m_scaleSlotSelector.get());
    
    // Mixer view will be created when we have processor available
    
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
    
    m_transportBar->onPatternSelected = [this](int pattern)
    {
        // TODO: Implement pattern selection in controller
        DBG("Pattern " << pattern << " selected");
    };
    
    m_transportBar->onPatternChain = [this](int pattern, bool chain)
    {
        // TODO: Implement pattern chaining
        DBG("Pattern " << pattern << " chaining: " << chain);
    };
    
    m_transportBar->onPanicClicked = [this]()
    {
        // TODO: Implement all notes off functionality in HAMAudioProcessor
        DBG("Panic button clicked - all notes off requested");
        
        if (auto* processor = m_controller.getAudioProcessor())
        {
            // For now, stop the sequencer which will have a similar effect
            processor->stop();
        }
    };
    
    // CPU usage callback
    m_transportBar->onRequestCPUUsage = [this]() -> float
    {
        if (auto* processor = m_controller.getAudioProcessor())
        {
            return processor->getCpuUsage();
        }
        return 0.0f;
    };
    
    // Track management button handlers
    m_addTrackButton->onClick = [this]()
    {
        // Add a new track through the controller
        m_controller.addTrack();
        DBG("Add track button clicked");
        
        // Refresh the UI
        if (m_trackSidebar)
        {
            m_trackSidebar->refreshTracks();
        }
        if (m_stageGrid)
        {
            m_stageGrid->setTrackCount(TrackManager::getInstance().getTrackCount());
        }
    };
    
    m_removeTrackButton->onClick = [this]()
    {
        // Remove the last track (or selected track)
        auto& trackManager = TrackManager::getInstance();
        int trackCount = trackManager.getTrackCount();
        
        if (trackCount > 1)  // Keep at least 1 track
        {
            m_controller.removeTrack(trackCount - 1);
            DBG("Remove track button clicked");
            
            // Refresh the UI
            if (m_trackSidebar)
            {
                m_trackSidebar->refreshTracks();
            }
            if (m_stageGrid)
            {
                m_stageGrid->setTrackCount(trackManager.getTrackCount());
            }
        }
        else
        {
            DBG("Cannot remove last track");
        }
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
    
    // Connect track sidebar callbacks
    if (m_trackSidebar)
    {
        m_trackSidebar->onTrackParameterChanged = [this](int trackIndex, const juce::String& param, float value)
        {
            // Handle plugin button click
            if (param == "openPlugin")
            {
                showPluginBrowser(trackIndex, false); // false = instrument plugin
            }
            else if (param == "openAccumulator")
            {
                // TODO: Show accumulator editor
                DBG("Accumulator editor requested for track " << trackIndex);
            }
            else if (param == "mute")
            {
                m_controller.setTrackMute(trackIndex, value > 0.5f);
            }
            else if (param == "solo")
            {
                m_controller.setTrackSolo(trackIndex, value > 0.5f);
            }
            else if (param == "channel")
            {
                // TODO: Add setTrackMidiChannel to AppController
                // m_controller.setTrackMidiChannel(trackIndex, static_cast<int>(value));
                DBG("MIDI channel change requested for track " << trackIndex << ": " << static_cast<int>(value));
            }
            else if (param == "voiceMode")
            {
                // TODO: Add setTrackVoiceMode to AppController
                // m_controller.setTrackVoiceMode(trackIndex, value > 0.5f ? 1 : 0); // 1 = poly, 0 = mono
                DBG("Voice mode change requested for track " << trackIndex << ": " << (value > 0.5f ? "poly" : "mono"));
            }
            else if (param == "division")
            {
                // TODO: Add setTrackDivision to AppController
                // m_controller.setTrackDivision(trackIndex, static_cast<int>(value));
                DBG("Division change requested for track " << trackIndex << ": " << static_cast<int>(value));
            }
            else if (param == "swing")
            {
                // TODO: Add setTrackSwing to AppController
                // m_controller.setTrackSwing(trackIndex, value);
                DBG("Swing change requested for track " << trackIndex << ": " << value);
            }
            else if (param == "octave")
            {
                // TODO: Add setTrackOctave to AppController
                // m_controller.setTrackOctave(trackIndex, static_cast<int>(value));
                DBG("Octave change requested for track " << trackIndex << ": " << static_cast<int>(value));
            }
            else if (param == "maxPulseLength")
            {
                // TODO: Add setTrackMaxPulseLength to AppController
                // m_controller.setTrackMaxPulseLength(trackIndex, static_cast<int>(value));
                DBG("Max pulse length change requested for track " << trackIndex << ": " << static_cast<int>(value));
            }
        };
        
        m_trackSidebar->onTrackSelected = [this](int trackIndex)
        {
            // TODO: Add selectTrack to AppController
            // m_controller.selectTrack(trackIndex);
            DBG("Track " << trackIndex << " selected");
        };
        
        m_trackSidebar->onAddTrack = [this]()
        {
            // TODO: Add addTrack to AppController
            // m_controller.addTrack();
            DBG("Add track requested");
        };
        
        m_trackSidebar->onRemoveTrack = [this](int trackIndex)
        {
            // TODO: Add removeTrack to AppController
            // m_controller.removeTrack(trackIndex);
            DBG("Remove track " << trackIndex << " requested");
        };
        
        // Connect plugin-specific callbacks to use unified browser without forcing view switch
        m_trackSidebar->onPluginBrowserRequested = [this](int trackIndex)
        {
            DBG("Plugin browser requested from sidebar for track " << trackIndex);
            
            // Check if plugin is already loaded first
            auto& trackManager = TrackManager::getInstance();
            auto* pluginState = trackManager.getPluginState(trackIndex, true);
            
            if (pluginState && pluginState->hasPlugin)
            {
                // Plugin is loaded - open its editor window directly
                DBG("Plugin already loaded, opening editor for: " << pluginState->pluginName);
                if (auto* processor = m_controller.getAudioProcessor())
                {
                    processor->showPluginEditor(trackIndex, -1);
                }
            }
            else
            {
                // No plugin loaded - we need to open the browser
                // First ensure MixerView exists (create it if needed but don't show it)
                if (!m_mixerView)
                {
                    if (auto* processor = m_controller.getAudioProcessor())
                    {
                        setAudioProcessor(processor);
                    }
                }
                
                // If MixerView exists now, use its browser (but stay on current view)
                if (m_mixerView)
                {
                    // Use MixerView's browser but don't switch views
                    m_mixerView->openPluginBrowserForTrack(trackIndex);
                }
                else
                {
                    // Fallback to the overlay plugin browser
                    DBG("Using overlay plugin browser");
                    showPluginBrowser(trackIndex, false);
                }
            }
        };
        
        m_trackSidebar->onPluginEditorRequested = [this](int trackIndex)
        {
            DBG("Plugin editor requested from sidebar for track " << trackIndex);
            
            // Check if a plugin is actually loaded
            auto& trackManager = TrackManager::getInstance();
            auto* pluginState = trackManager.getPluginState(trackIndex, true);
            
            if (pluginState && pluginState->hasPlugin)
            {
                // Open the plugin editor window directly without switching views
                if (auto* processor = m_controller.getAudioProcessor())
                {
                    processor->showPluginEditor(trackIndex, -1);
                }
            }
            // If no plugin is loaded, do nothing - the browser should be opened via onPluginBrowserRequested
            // This prevents duplicate browser windows
        };
    }
    
}

//==============================================================================
void UICoordinator::setActiveView(ViewMode mode)
{
    m_activeView = mode;
    
    // Update visibility
    if (m_sequencerPage)
        m_sequencerPage->setVisible(mode == ViewMode::Sequencer);
    
    // Create placeholder Component if MixerView doesn't exist yet
    if (mode == ViewMode::Mixer && !m_mixerView && !m_mixerPlaceholder)
    {
        // Create a placeholder component with basic UI
        m_mixerPlaceholder = std::make_unique<juce::Component>();
        m_contentContainer->addAndMakeVisible(m_mixerPlaceholder.get());
        
        // Add a label to show it's a placeholder
        auto* label = new juce::Label("placeholder", "Mixer View - Waiting for Audio Processor");
        label->setFont(juce::Font(juce::FontOptions(20.0f)));
        label->setColour(juce::Label::textColourId, juce::Colours::grey);
        label->setJustificationType(juce::Justification::centred);
        m_mixerPlaceholder->addAndMakeVisible(label);
        
        DBG("Created placeholder for MixerView");
    }
    
    if (m_mixerView)
        m_mixerView->setVisible(mode == ViewMode::Mixer);
    else if (m_mixerPlaceholder)
        m_mixerPlaceholder->setVisible(mode == ViewMode::Mixer);
    
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
    
    // Setup callback to load plugin through audio processor
    m_pluginBrowser->onPluginChosen = [this, trackIndex, forEffects](const juce::PluginDescription& desc)
    {
        // Get audio processor from controller
        if (auto* processor = m_controller.getAudioProcessor())
        {
            // Load plugin through the audio processor
            bool isInstrument = !forEffects && desc.isInstrument;
            processor->loadPlugin(trackIndex, desc, isInstrument);
            
            // Update track state
            m_controller.loadPluginForTrack(trackIndex, desc.fileOrIdentifier);
        }
        hidePluginBrowser();
    };
    
    // Setup close callback
    m_pluginBrowser->onCloseRequested = [this]()
    {
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
    
    // Single integrated toolbar - everything on one line
    auto tabBarArea = bounds.removeFromTop(TAB_BAR_HEIGHT);
    
    int iconButtonSize = 36;  // Consistent size for all icon buttons
    int buttonY = tabBarArea.getY() + (TAB_BAR_HEIGHT - iconButtonSize) / 2;  // Vertically centered
    
    // Left side: Navigation buttons
    int leftX = 10;
    
    // Track management buttons (same size as navigation)
    if (m_addTrackButton)
    {
        m_addTrackButton->setBounds(leftX, buttonY, iconButtonSize, iconButtonSize);
        leftX += iconButtonSize + 5;
    }
    if (m_removeTrackButton)
    {
        m_removeTrackButton->setBounds(leftX, buttonY, iconButtonSize, iconButtonSize);
        leftX += iconButtonSize + 10;  // Space before navigation
    }
    
    // Navigation icons
    if (m_sequencerTabButton)
    {
        m_sequencerTabButton->setBounds(leftX, buttonY, iconButtonSize, iconButtonSize);
        leftX += iconButtonSize + 5;
    }
    if (m_mixerTabButton)
    {
        m_mixerTabButton->setBounds(leftX, buttonY, iconButtonSize, iconButtonSize);
        leftX += iconButtonSize + 5;
    }
    if (m_settingsTabButton)
    {
        m_settingsTabButton->setBounds(leftX, buttonY, iconButtonSize, iconButtonSize);
        leftX += iconButtonSize + 5;
    }
    
    // Center: Scale slot selector 
    if (m_scaleSlotSelector)
    {
        // Calculate scale selector width (fixed size for centered positioning)
        int scaleSelectorWidth = 700;  // Fixed width for scale selector
        
        // Center the scale selector in the remaining space
        int availableWidth = tabBarArea.getWidth() - leftX - 10;  // 10px right margin
        int centerX = leftX + (availableWidth - scaleSelectorWidth) / 2;
        
        // Make sure it doesn't go off screen
        if (centerX < leftX + 20)
            centerX = leftX + 20;
        
        m_scaleSlotSelector->setBounds(centerX, tabBarArea.getY(), scaleSelectorWidth, TAB_BAR_HEIGHT);
    }
    
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
    
    // Scale slot selector is now in the toolbar, so we don't position it here
    
    // HAM Editor at bottom (if visible)
    if (m_hamEditorVisible)
    {
        // Reserve space for HAM editor
        bounds.removeFromBottom(HAM_EDITOR_HEIGHT);
        // TODO: Position HAMEditorPanel
    }
    
    // Viewport fills the entire remaining space (more room now!)
    if (m_sequencerViewport)
    {
        m_sequencerViewport->setBounds(bounds);
        
        // Layout the content within the viewport
        if (m_sequencerContent && m_trackSidebar && m_stageGrid)
        {
            // Get track count from TrackManager
            auto& trackManager = TrackManager::getInstance();
            const int numTracks = static_cast<int>(trackManager.getAllTracks().size());
            
            // Calculate required dimensions
            const int sidebarWidth = SIDEBAR_WIDTH;
            const int gridWidth = 8 * STAGE_CARD_WIDTH + 7 * 1; // 8 cards + minimal horizontal gaps
            const int totalWidth = sidebarWidth + gridWidth + 10; // Add some padding
            
            const int trackGap = 8; // Gap between tracks (matches both TrackSidebar and StageGrid)
            const int totalHeight = numTracks * STAGE_CARD_HEIGHT + (numTracks - 1) * trackGap;
            
            // Set the total content size
            m_sequencerContent->setSize(totalWidth, totalHeight);
            
            // Position sidebar on left
            m_trackSidebar->setBounds(0, 0, sidebarWidth, totalHeight);
            
            // Position stage grid next to sidebar
            m_stageGrid->setBounds(sidebarWidth, 0, gridWidth, totalHeight);
        }
    }
}

void UICoordinator::layoutMixerView()
{
    if (!m_contentContainer)
        return;
        
    auto bounds = m_contentContainer->getLocalBounds();
    
    if (m_mixerView)
    {
        m_mixerView->setBounds(bounds);
        m_mixerView->setVisible(true);  // Ensure it's visible
    }
    else if (m_mixerPlaceholder)
    {
        m_mixerPlaceholder->setBounds(bounds);
        m_mixerPlaceholder->setVisible(true);  // Ensure it's visible
        
        // Position the label
        if (m_mixerPlaceholder->getNumChildComponents() > 0)
        {
            m_mixerPlaceholder->getChildComponent(0)->setBounds(bounds);
        }
    }
}

void UICoordinator::updateViewButtonStates()
{
    // Update button active states based on current view
    if (m_sequencerTabButton)
        m_sequencerTabButton->setActive(m_activeView == ViewMode::Sequencer);
    
    if (m_mixerTabButton)
        m_mixerTabButton->setActive(m_activeView == ViewMode::Mixer);
    
    if (m_settingsTabButton)
        m_settingsTabButton->setActive(m_activeView == ViewMode::Settings);
}

//==============================================================================
// Scale browser methods

void UICoordinator::showScaleBrowser(int slotIndex)
{
    // Create and show scale browser dialog
    ScaleBrowser::showScaleBrowser(slotIndex,
        [this](int slot, const Scale& scale, const juce::String& name)
        {
            // Load selected scale into slot via view model
            if (m_scaleSlotViewModel)
            {
                m_scaleSlotViewModel->loadScaleIntoSlot(slot, scale, name);
            }
        });
}

void UICoordinator::hideScaleBrowser()
{
    // Scale browser is a modal dialog that handles its own closing
    // Nothing to do here
}

} // namespace UI
} // namespace HAM