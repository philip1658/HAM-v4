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
    {
        juce::Logger::writeToLog("UICoordinator::setAudioProcessor - processor is null!");
        return;
    }
    
    juce::Logger::writeToLog("UICoordinator::setAudioProcessor - Setting up audio processor");
    
    // Connect the stage grid to the processor for playhead tracking
    if (m_stageGrid)
    {
        m_stageGrid->setAudioProcessor(processor);
        juce::Logger::writeToLog("UICoordinator: Connected processor to StageGrid");
    }
    
    // Create the mixer view now that we have a processor
    if (!m_mixerView)
    {
        juce::Logger::writeToLog("UICoordinator: Creating MixerView with processor");
        m_mixerView = std::make_unique<MixerView>(*processor);
        m_contentContainer->addAndMakeVisible(m_mixerView.get());
        
        // Set visibility based on current active view
        // But MixerView is created regardless so its plugin browser can be used
        m_mixerView->setVisible(m_activeView == ViewMode::Mixer);
        
        juce::Logger::writeToLog("UICoordinator: MixerView created, visible=" + 
                                juce::String(m_activeView == ViewMode::Mixer ? "true" : "false"));
        
        // MixerView provides the plugin browser for the entire application
        // It can be accessed even when MixerView itself is not visible
        
        // If we're currently showing the mixer view, layout it
        if (m_activeView == ViewMode::Mixer)
        {
            resized();
        }
    }
    else
    {
        juce::Logger::writeToLog("UICoordinator: MixerView already exists");
    }
}

//==============================================================================
void UICoordinator::createUIComponents()
{
    // Create transport bar
    m_transportBar = std::make_unique<TransportBar>();
    addAndMakeVisible(m_transportBar.get());
    
    // Create main content container FIRST (at bottom of z-order)
    m_contentContainer = std::make_unique<juce::Component>();
    addAndMakeVisible(m_contentContainer.get());
    
    // Create sequencer page components (also at bottom)
    m_sequencerPage = std::make_unique<juce::Component>();
    m_contentContainer->addAndMakeVisible(m_sequencerPage.get());
    
    // Create scale UI components BEFORE buttons (so they're below in z-order)
    m_scaleSlotViewModel = std::make_unique<ScaleSlotViewModel>();
    m_scaleSlotSelector = std::make_unique<ScaleSlotSelector>();
    m_scaleSlotSelector->setViewModel(m_scaleSlotViewModel.get());
    addAndMakeVisible(m_scaleSlotSelector.get());
    
    // Create view toggle buttons AFTER scale selector (on top in z-order)
    m_sequencerTabButton = std::make_unique<IconButton>("Sequencer", IconButton::IconType::Sequencer);
    m_sequencerTabButton->setTooltip("Sequencer View");
    m_sequencerTabButton->setActive(true);  // Start with sequencer active
    addAndMakeVisible(m_sequencerTabButton.get());
    juce::Logger::writeToLog("UICoordinator: Created Sequencer tab button");
    
    m_mixerTabButton = std::make_unique<IconButton>("Mixer", IconButton::IconType::Mixer);
    m_mixerTabButton->setTooltip("Mixer View");
    addAndMakeVisible(m_mixerTabButton.get());
    juce::Logger::writeToLog("UICoordinator: Created Mixer tab button");
    
    // Settings button removed - no longer needed
    
    // Create track management buttons with icon style
    m_addTrackButton = std::make_unique<IconButton>("Add Track", IconButton::IconType::AddTrack);
    m_addTrackButton->setTooltip("Add Track");
    m_addTrackButton->setActiveColor(juce::Colour(0xFF00FF88));  // Green
    // onClick will be set in setupEventHandlers()
    addAndMakeVisible(m_addTrackButton.get());
    
    m_removeTrackButton = std::make_unique<IconButton>("Remove Track", IconButton::IconType::RemoveTrack);
    m_removeTrackButton->setTooltip("Remove Track");
    m_removeTrackButton->setActiveColor(juce::Colour(0xFFFF5555));  // Red
    // onClick will be set in setupEventHandlers()
    addAndMakeVisible(m_removeTrackButton.get());
    
    // Content container and sequencer page already created above
    
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
    
    // Scale UI components already created above
    // Setup scale browser callback
    m_scaleSlotSelector->onScaleBrowserRequested = [this](int slotIndex) {
        showScaleBrowser(slotIndex);
    };
    
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
    
    // Panic button removed - no callback needed
    
    // Track management button handlers
    m_addTrackButton->onClick = [this]()
    {
        // Add a new track through the controller
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
        
        // Re-layout the entire sequencer view to recalculate responsive widths
        layoutSequencerView();
        
        DBG("Track added - now have " << newTrackCount << " tracks");
    };
    
    m_removeTrackButton->onClick = [this]()
    {
        // Remove the last track (or selected track)
        auto& trackManager = TrackManager::getInstance();
        int trackCount = trackManager.getTrackCount();
        
        if (trackCount > 1)  // Keep at least 1 track
        {
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
            
            // Re-layout the entire sequencer view to recalculate responsive widths
            layoutSequencerView();
            
            DBG("Track removed - now have " << newTrackCount << " tracks");
        }
        else
        {
            DBG("Cannot remove last track");
        }
    };
    
    // View switching
    juce::Logger::writeToLog("UICoordinator: Setting up view switching handlers");
    
    if (m_sequencerTabButton)
    {
        m_sequencerTabButton->onClick = [this]()
        {
            juce::Logger::writeToLog("UICoordinator: Sequencer tab button clicked!");
            setActiveView(ViewMode::Sequencer);
        };
        juce::Logger::writeToLog("UICoordinator: Sequencer button onClick handler set");
    }
    else
    {
        juce::Logger::writeToLog("UICoordinator: ERROR - m_sequencerTabButton is null!");
    }
    
    if (m_mixerTabButton)
    {
        m_mixerTabButton->onClick = [this]()
        {
            juce::Logger::writeToLog("UICoordinator: Mixer tab button onClick lambda called!");
            setActiveView(ViewMode::Mixer);
        };
        juce::Logger::writeToLog("UICoordinator: Mixer button onClick handler set");
        juce::Logger::writeToLog("UICoordinator: Mixer button visible=" + 
                                juce::String(m_mixerTabButton->isVisible() ? "true" : "false"));
        juce::Logger::writeToLog("UICoordinator: Mixer button enabled=" + 
                                juce::String(m_mixerTabButton->isEnabled() ? "true" : "false"));
    }
    else
    {
        juce::Logger::writeToLog("UICoordinator: ERROR - m_mixerTabButton is null!");
    }
    
    // Settings button removed - no event handler needed
    
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
    juce::Logger::writeToLog("UICoordinator::setActiveView - Switching to " + 
                            juce::String(mode == ViewMode::Sequencer ? "Sequencer" :
                                       mode == ViewMode::Mixer ? "Mixer" : "Settings"));
    
    m_activeView = mode;
    
    // Update visibility
    if (m_sequencerPage)
        m_sequencerPage->setVisible(mode == ViewMode::Sequencer);
    
    // Create placeholder Component if MixerView doesn't exist yet
    if (mode == ViewMode::Mixer && !m_mixerView && !m_mixerPlaceholder)
    {
        juce::Logger::writeToLog("UICoordinator: Mixer requested but no MixerView - creating placeholder");
        
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
    {
        m_mixerView->setVisible(mode == ViewMode::Mixer);
        juce::Logger::writeToLog("UICoordinator: MixerView visibility set to " + 
                                juce::String(mode == ViewMode::Mixer ? "true" : "false"));
    }
    else if (m_mixerPlaceholder)
    {
        m_mixerPlaceholder->setVisible(mode == ViewMode::Mixer);
        juce::Logger::writeToLog("UICoordinator: MixerPlaceholder visibility set to " + 
                                juce::String(mode == ViewMode::Mixer ? "true" : "false"));
    }
    
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
    
    // Draw frame around the entire topbar area for visual separation
    if (m_transportBar)
    {
        // Calculate topbar area (transport + tab bar)
        auto topbarBounds = getLocalBounds();
        topbarBounds = topbarBounds.removeFromTop(TRANSPORT_HEIGHT + TAB_BAR_HEIGHT);
        
        // Draw subtle background for topbar area
        g.setColour(juce::Colour(0xFF141414));
        g.fillRect(topbarBounds);
        
        // Draw bottom border line for clean separation
        g.setColour(juce::Colour(0xFF2A2A2A));
        g.fillRect(topbarBounds.getX(), topbarBounds.getBottom() - 1, 
                   topbarBounds.getWidth(), 1);
        
        // Draw top border line (under window title bar)
        g.setColour(juce::Colour(0xFF2A2A2A));
        g.fillRect(topbarBounds.getX(), topbarBounds.getY(), 
                   topbarBounds.getWidth(), 1);
    }
    
    // Draw visual separator between track management and view tabs
    if (m_removeTrackButton && m_sequencerTabButton)
    {
        // Get positions from the buttons
        int separatorX = m_removeTrackButton->getRight() + 20;  // Centered in the gap
        int separatorY = m_removeTrackButton->getY() + 4;
        int separatorHeight = m_removeTrackButton->getHeight() - 8;
        
        // Draw vertical divider line
        g.setColour(juce::Colour(0xFF2A2A2A)); // Subtle divider color
        g.fillRect(separatorX, separatorY, 1, separatorHeight);
    }
}

void UICoordinator::resized()
{
    auto bounds = getLocalBounds();
    
    // UNIFIED LAYOUT: Both top bars use same height and spacing
    const int UNIFIED_BAR_HEIGHT = 52;  // Same height for both bars
    const int UNIFIED_BUTTON_HEIGHT = 36; // Same button height everywhere
    const int UNIFIED_SPACING = 8;  // Consistent spacing
    const int UNIFIED_MARGIN = 8;   // Uniform margins
    
    // Transport bar at top (now uses unified height)
    if (m_transportBar)
    {
        m_transportBar->setBounds(bounds.removeFromTop(UNIFIED_BAR_HEIGHT));
    }
    
    // Second bar area for tabs and scale switcher
    auto secondBarArea = bounds.removeFromTop(UNIFIED_BAR_HEIGHT);
    
    // Calculate vertical center for all buttons
    int buttonY = secondBarArea.getY() + (UNIFIED_BAR_HEIGHT - UNIFIED_BUTTON_HEIGHT) / 2;
    
    // Left side: Track management buttons
    int leftX = UNIFIED_MARGIN;
    
    // Track management buttons (square icons, unified height)
    if (m_addTrackButton)
    {
        m_addTrackButton->setBounds(leftX, buttonY, UNIFIED_BUTTON_HEIGHT, UNIFIED_BUTTON_HEIGHT);
        leftX += UNIFIED_BUTTON_HEIGHT + UNIFIED_SPACING;
    }
    if (m_removeTrackButton)
    {
        m_removeTrackButton->setBounds(leftX, buttonY, UNIFIED_BUTTON_HEIGHT, UNIFIED_BUTTON_HEIGHT);
        leftX += UNIFIED_BUTTON_HEIGHT + (UNIFIED_SPACING * 3);  // Larger gap for visual grouping
    }
    
    // Navigation tabs (only Sequencer and Mixer - Settings removed)
    const int TAB_WIDTH = 80;  // Wider tabs for better visibility
    if (m_sequencerTabButton)
    {
        m_sequencerTabButton->setBounds(leftX, buttonY, TAB_WIDTH, UNIFIED_BUTTON_HEIGHT);
        m_sequencerTabButton->toFront(false);  // Ensure button is on top
        leftX += TAB_WIDTH + UNIFIED_SPACING;
    }
    if (m_mixerTabButton)
    {
        m_mixerTabButton->setBounds(leftX, buttonY, TAB_WIDTH, UNIFIED_BUTTON_HEIGHT);
        m_mixerTabButton->toFront(false);  // Ensure button is on top
        juce::Logger::writeToLog("UICoordinator: Mixer button bounds set to: " + 
                                juce::String(leftX) + ", " + juce::String(buttonY) + ", " +
                                juce::String(TAB_WIDTH) + ", " + juce::String(UNIFIED_BUTTON_HEIGHT));
        leftX += TAB_WIDTH + UNIFIED_SPACING;
    }
    // Settings tab removed - no longer needed
    
    // Scale slot selector - position it after the buttons so it doesn't block them
    if (m_scaleSlotSelector)
    {
        // Calculate the width needed for scale selector (approximately)
        const int SCALE_SELECTOR_WIDTH = 600;  // Approximate width for scale selector content
        
        // Position it after the navigation buttons, or center if there's enough space
        int scaleSelectorX = leftX + UNIFIED_SPACING * 2;  // After mixer button with gap
        
        // If there's enough space, center it in the remaining area
        int remainingWidth = secondBarArea.getWidth() - scaleSelectorX;
        if (remainingWidth > SCALE_SELECTOR_WIDTH)
        {
            scaleSelectorX += (remainingWidth - SCALE_SELECTOR_WIDTH) / 2;
        }
        
        // Set bounds so it doesn't overlap the navigation buttons
        m_scaleSlotSelector->setBounds(scaleSelectorX, buttonY, 
                                       juce::jmin(SCALE_SELECTOR_WIDTH, remainingWidth), 
                                       UNIFIED_BUTTON_HEIGHT);
        
        juce::Logger::writeToLog("UICoordinator: Scale selector bounds: " + 
                                juce::String(scaleSelectorX) + ", " + juce::String(buttonY) + ", " +
                                juce::String(juce::jmin(SCALE_SELECTOR_WIDTH, remainingWidth)) + ", " + 
                                juce::String(UNIFIED_BUTTON_HEIGHT));
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
            
            // Get the visible viewport dimensions (this is what we see on screen)
            auto viewportBounds = m_sequencerViewport->getLocalBounds();
            const int viewportWidth = viewportBounds.getWidth();
            const int viewportHeight = viewportBounds.getHeight();
            
            // Fixed sidebar width
            const int sidebarWidth = SIDEBAR_WIDTH;
            
            // Calculate available width for the grid (viewport width minus sidebar)
            const int availableGridWidth = viewportWidth - sidebarWidth;
            
            // Calculate optimal stage card width to fill available space
            // The StageGrid component will handle the internal card sizing
            // We just need to give it the right amount of space
            
            // Calculate height based on tracks
            const int trackGap = 8; // Gap between tracks (matches both TrackSidebar and StageGrid)
            const int totalHeight = numTracks * STAGE_CARD_HEIGHT + (numTracks - 1) * trackGap;
            
            // Determine if we need horizontal scrolling
            // We want at least MIN_STAGE_CARD_WIDTH per card
            const int minRequiredWidth = 8 * MIN_STAGE_CARD_WIDTH + 7 * 1;  // 8 cards with minimal gaps
            
            // If available space is less than minimum, enable horizontal scrolling
            // Otherwise, use the full viewport width (no horizontal scroll)
            int gridWidth;
            bool needsHorizontalScroll = availableGridWidth < minRequiredWidth;
            
            if (needsHorizontalScroll) {
                // Use minimum width and allow horizontal scrolling
                gridWidth = minRequiredWidth;
            } else {
                // Use all available width - no horizontal scroll needed
                gridWidth = availableGridWidth;
            }
            
            // Set the total content size
            const int totalWidth = sidebarWidth + gridWidth;
            m_sequencerContent->setSize(totalWidth, totalHeight);
            
            // Position sidebar on left
            m_trackSidebar->setBounds(0, 0, sidebarWidth, totalHeight);
            
            // Position stage grid next to sidebar with all available/required width
            m_stageGrid->setBounds(sidebarWidth, 0, gridWidth, totalHeight);
            
            // Update scrollbar visibility
            // Show horizontal scrollbar only when needed
            m_sequencerViewport->setScrollBarsShown(true, needsHorizontalScroll);
        }
    }
}

void UICoordinator::layoutMixerView()
{
    if (!m_contentContainer)
        return;
        
    auto bounds = m_contentContainer->getLocalBounds();
    juce::Logger::writeToLog("UICoordinator::layoutMixerView - bounds: " + 
                            bounds.toString());
    
    if (m_mixerView)
    {
        m_mixerView->setBounds(bounds);
        m_mixerView->setVisible(true);  // Ensure it's visible
        juce::Logger::writeToLog("UICoordinator: MixerView bounds set and made visible");
    }
    else if (m_mixerPlaceholder)
    {
        m_mixerPlaceholder->setBounds(bounds);
        m_mixerPlaceholder->setVisible(true);  // Ensure it's visible
        juce::Logger::writeToLog("UICoordinator: MixerPlaceholder bounds set and made visible");
        
        // Position the label
        if (m_mixerPlaceholder->getNumChildComponents() > 0)
        {
            m_mixerPlaceholder->getChildComponent(0)->setBounds(bounds);
        }
    }
    else
    {
        juce::Logger::writeToLog("UICoordinator: WARNING - No MixerView or placeholder to layout!");
    }
}

void UICoordinator::updateViewButtonStates()
{
    // Update button active states based on current view
    if (m_sequencerTabButton)
        m_sequencerTabButton->setActive(m_activeView == ViewMode::Sequencer);
    
    if (m_mixerTabButton)
        m_mixerTabButton->setActive(m_activeView == ViewMode::Mixer);
    
    // Settings button removed - no state update needed
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