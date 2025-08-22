/*
  ==============================================================================

    MainComponent.cpp
    Main component for HAM sequencer with Pulse UI components

  ==============================================================================
*/

#include "MainComponent.h"
#include "Infrastructure/Messaging/MessageDispatcher.h"
#include "Infrastructure/Messaging/MessageTypes.h"
#include "Infrastructure/Audio/HAMAudioProcessor.h"
#include "Infrastructure/Plugins/PluginManager.h"
// UI Components will be added when implemented
#include "Presentation/Core/DesignSystem.h"
#include "Presentation/Views/StageGrid.h"
#include "Presentation/Views/TransportBar.h"
#include "Presentation/Views/TrackSidebar.h"
// #include "Presentation/Views/HAMEditorPanel.h" // TODO: Implement HAMEditorPanel
#include "Presentation/Views/PluginBrowser.h"
#include <unordered_map>

//==============================================================================
class MainComponent::Impl : public juce::Timer
{
public:
    Impl(MainComponent& owner) : m_owner(owner)
    {
        // Initialize audio engine and messaging bridge
        m_processor = std::make_unique<HAM::HAMAudioProcessor>();
        m_messageDispatcher = &m_processor->getMessageDispatcher();
        setupMessageHandlers();

        // Setup audio I/O and attach processor
        m_deviceManager.initialiseWithDefaultDevices(0, 2);
        m_audioPlayer.setProcessor(m_processor.get());
        m_deviceManager.addAudioCallback(&m_audioPlayer);
        
        // Create transport bar with Pulse components
        m_transportBar = std::make_unique<HAM::UI::TransportBar>();
        m_transportBar->onPlayStateChanged = [this](bool playing) {
            handlePlayStateChange(playing);
        };
        m_transportBar->onBPMChanged = [this](float bpm) {
            handleBPMChange(bpm);
        };
        m_transportBar->onMidiMonitorToggled = [this](bool enabled) {
            handleMidiMonitorToggle(enabled);
        };
        m_owner.addAndMakeVisible(m_transportBar.get());
        
        // Create view toggle buttons for pattern bar
        m_sequencerTabButton = std::make_unique<HAM::UI::ModernButton>("SEQUENCER", HAM::UI::ModernButton::Style::Solid);
        m_sequencerTabButton->setColor(juce::Colour(0xFF00CCFF));
        m_sequencerTabButton->onClick = [this]() { setActiveView(0); };
        m_owner.addAndMakeVisible(m_sequencerTabButton.get());
        
        m_mixerTabButton = std::make_unique<HAM::UI::ModernButton>("MIXER", HAM::UI::ModernButton::Style::Outline);
        m_mixerTabButton->setColor(juce::Colour(0xFF00CCFF));
        m_mixerTabButton->onClick = [this]() { setActiveView(1); };
        m_owner.addAndMakeVisible(m_mixerTabButton.get());

        // Create main content container (replaces tabs)
        m_contentContainer = std::make_unique<juce::Component>();
        m_owner.addAndMakeVisible(m_contentContainer.get());
        
        // Sequencer page root container
        m_sequencerPage = std::make_unique<juce::Component>();
        m_contentContainer->addAndMakeVisible(m_sequencerPage.get());

        // Mixer page
        m_mixerView = std::make_unique<HAM::UI::MixerView>();
        m_mixerView->onAliasInstrumentPlugin = [this](int trackIndex) { openPluginBrowser(trackIndex); };
        m_mixerView->onAddFxPlugin = [this](int trackIndex) { openFxPluginBrowser(trackIndex); };
        m_contentContainer->addAndMakeVisible(m_mixerView.get());
        m_mixerView->setVisible(false); // Start with sequencer view
        
        // Create stage grid
        m_stageGrid = std::make_unique<HAM::UI::StageGrid>();
        m_stageGrid->onStageParameterChanged = [this](int track, int stage, const juce::String& param, float value) {
            handleStageParameterChange(track, stage, param, value);
        };
        m_stageGrid->onHAMEditorRequested = [this](int stage) {
            // Expand editor and select stage
            // TODO: Implement HAMEditorPanel
            // if (m_hamEditor)
            // {
            //     m_hamEditor->setExpanded(true);
            //     m_hamEditor->setCurrentStage(0, stage);
            //     m_owner.resized(); // Relayout to show editor
            // }
        };
        // Wrap StageGrid in a viewport for scrolling
        m_stageViewport = std::make_unique<juce::Viewport>("stageViewport");
        m_stageViewport->setViewedComponent(m_stageGrid.get(), false);
        m_stageViewport->setScrollBarsShown(false, true); // Hide vertical scrollbar for testing
        m_stageViewport->setScrollOnDragMode(juce::Viewport::ScrollOnDragMode::all);
        m_stageViewport->setViewPosition(0, 0); // Ensure content starts at top
        m_stageViewport->setScrollBarThickness(0); // Remove scrollbar thickness
        m_owner.addAndMakeVisible(m_stageViewport.get());
        
        // Create track sidebar
        m_trackSidebar = std::make_unique<HAM::UI::TrackSidebar>();
        m_trackSidebar->onTrackSelected = [this](int trackIndex) {
            handleTrackSelection(trackIndex);
        };
        m_trackSidebar->onTrackParameterChanged = [this](int trackIndex, const juce::String& param, float value) {
            handleTrackParameterChange(trackIndex, param, value);
        };
        m_trackSidebar->onAddTrack = [this]() { handleAddTrack(); };
        // Öffne Plugin Browser beim Plugin Button
        // (TrackSidebar gibt "openPlugin" als Param via onTrackParameterChanged – wir fangen es zusätzlich hier UI-seitig ab)
        // Alternative: separater Callback onPluginButtonClicked im Sidebar; hier nutzen wir Param-Schiene unten.
        // Wrap TrackSidebar in a viewport for vertical scrolling
        m_trackViewport = std::make_unique<juce::Viewport>("trackViewport");
        m_trackViewport->setViewedComponent(m_trackSidebar.get(), false);
        m_trackViewport->setScrollBarsShown(false, false); // Hide both scrollbars for testing
        m_trackViewport->setScrollOnDragMode(juce::Viewport::ScrollOnDragMode::all);
        m_trackViewport->setViewPosition(0, 0); // Ensure content starts at top
        m_trackViewport->setScrollBarThickness(0); // Remove scrollbar thickness
        m_owner.addAndMakeVisible(m_trackViewport.get());
        
        // Create Add Track button for pattern bar
        m_addTrackButton = std::make_unique<HAM::UI::ModernButton>("+ ADD TRACK", HAM::UI::ModernButton::Style::Gradient);
        m_addTrackButton->setColor(juce::Colour(0xFF00CCFF)); // Bright cyan
        m_addTrackButton->onClick = [this]() {
            handleAddTrack();
        };
        m_owner.addAndMakeVisible(m_addTrackButton.get());
        
        // Create Remove Track button for pattern bar
        m_removeTrackButton = std::make_unique<HAM::UI::ModernButton>("- REMOVE", HAM::UI::ModernButton::Style::Outline);
        m_removeTrackButton->setColor(juce::Colour(0xFFFF4444)); // Red color for remove
        m_removeTrackButton->onClick = [this]() {
            handleRemoveTrack();
        };
        m_owner.addAndMakeVisible(m_removeTrackButton.get());

        // Create pattern buttons A–H in pattern bar
        for (int i = 0; i < 8; ++i)
        {
            auto btn = std::make_unique<HAM::UI::ModernButton>(juce::String::charToString('A' + i), HAM::UI::ModernButton::Style::Outline);
            btn->onClick = [this, i]() { selectPattern(i); };
            m_owner.addAndMakeVisible(btn.get());
            m_patternButtons.push_back(std::move(btn));
        }
        updatePatternButtonStyles();
        
        // Note: Removed main panel as it was covering other components
        
        // HAM Editor panel (collapsed by default)
        // TODO: Implement HAMEditorPanel
        // m_hamEditor = std::make_unique<HAM::UI::HAMEditorPanel>();
        // m_hamEditor->setExpanded(false);
        // m_hamEditor->onStageParamChanged = [this](int track, int stage, const juce::String& param, float value) {
        //     handleStageParameterChange(track, stage, param, value);
        // };
        // m_sequencerPage->addAndMakeVisible(m_hamEditor.get());
        
        // Start timer for UI updates from engine
        startTimer(30); // ~33Hz UI update rate
    }
    
    ~Impl()
    {
        stopTimer();
        m_deviceManager.removeAudioCallback(&m_audioPlayer);
        m_audioPlayer.setProcessor(nullptr);
    }
    
    void resized()
    {
        auto bounds = m_owner.getLocalBounds();
        
        // Transport bar at top (80px)
        auto transportHeight = 80;
        m_transportBar->setBounds(bounds.removeFromTop(transportHeight));
        
        // Pattern bar below transport (40px) - full width for pattern buttons
        auto patternBarHeight = 40;
        auto patternBar = bounds.removeFromTop(patternBarHeight);
        m_patternBarBounds = patternBar;
        
        // Store the remaining area after transport and pattern bars
        auto contentArea = bounds;
        
        // Position track control buttons on the left side of pattern bar
        auto leftArea = patternBar.reduced(5);
        
        // Add Track button
        if (m_addTrackButton)
        {
            auto addBounds = leftArea.removeFromLeft(100);
            m_addTrackButton->setBounds(addBounds);
        }
        
        // Remove Track button  
        if (m_removeTrackButton)
        {
            leftArea.removeFromLeft(5); // Small gap
            auto removeBounds = leftArea.removeFromLeft(80);
            m_removeTrackButton->setBounds(removeBounds);
        }
        
        // View toggle buttons in center of pattern bar
        auto centerX = patternBar.getCentreX();
        int tabButtonWidth = 90;
        int tabGap = 5;
        
        if (m_sequencerTabButton)
        {
            m_sequencerTabButton->setBounds(centerX - tabButtonWidth - tabGap/2, 
                                           patternBar.getY() + 5,
                                           tabButtonWidth, 
                                           patternBar.getHeight() - 10);
        }
        
        if (m_mixerTabButton)
        {
            m_mixerTabButton->setBounds(centerX + tabGap/2,
                                       patternBar.getY() + 5,
                                       tabButtonWidth,
                                       patternBar.getHeight() - 10);
        }

        // Pattern buttons on the right side of pattern bar
        {
            auto buttonsArea = patternBar;
            int buttonWidth = 34;
            int gap = 4;
            int totalWidth = 8 * buttonWidth + 7 * gap;
            buttonsArea.setX(buttonsArea.getRight() - totalWidth - 8);
            buttonsArea.setWidth(totalWidth);
            for (int i = 0; i < static_cast<int>(m_patternButtons.size()); ++i)
            {
                auto x = buttonsArea.getX() + i * (buttonWidth + gap);
                m_patternButtons[i]->setBounds(x, buttonsArea.getY() + 4, buttonWidth, buttonsArea.getHeight() - 8);
            }
        }
        
        DBG("Content Area Bounds: x=" << contentArea.getX() 
            << " y=" << contentArea.getY() 
            << " w=" << contentArea.getWidth() 
            << " h=" << contentArea.getHeight());
        
        // For sequencer view, we need special layout
        if (m_activeView == 0)
        {
            // Track sidebar on left - starts at same height as stage cards
            static constexpr int SIDEBAR_WIDTH = 264;  // Line L
            
            // Both track sidebar and stage grid start at the same Y position (after pattern bar)
            auto sidebarBounds = contentArea;
            sidebarBounds.setWidth(SIDEBAR_WIDTH);
            
            // Debug output to check actual positioning
            DBG("Track Viewport Bounds: x=" << sidebarBounds.getX() 
                << " y=" << sidebarBounds.getY() 
                << " w=" << sidebarBounds.getWidth() 
                << " h=" << sidebarBounds.getHeight());
            
            if (m_trackViewport)
            {
                m_trackViewport->setBounds(sidebarBounds);
                m_trackViewport->setViewPosition(0, 0); // Force content to start at top
                if (auto* viewed = m_trackViewport->getViewedComponent())
                    viewed->setSize(sidebarBounds.getWidth(), std::max(sidebarBounds.getHeight(), viewed->getHeight()));
            }
            
            // Stage grid area - starts at same height as track sidebar, but offset horizontally
            auto stageArea = contentArea;
            stageArea.removeFromLeft(SIDEBAR_WIDTH + 1); // Remove sidebar width + 1px gap
            
            // Debug output to check actual positioning
            DBG("Stage Viewport Bounds: x=" << stageArea.getX() 
                << " y=" << stageArea.getY() 
                << " w=" << stageArea.getWidth() 
                << " h=" << stageArea.getHeight());
            
            if (m_stageViewport)
            {
                m_stageViewport->setBounds(stageArea);
                m_stageViewport->setViewPosition(0, 0); // Force content to start at top
                if (auto* viewed = m_stageViewport->getViewedComponent())
                    viewed->setSize(std::max(stageArea.getWidth(), viewed->getWidth()), 
                                   std::max(stageArea.getHeight(), viewed->getHeight()));
            }
            
            // Make sure track and stage viewports are visible
            if (m_trackViewport)
                m_trackViewport->setVisible(true);
            if (m_stageViewport)
                m_stageViewport->setVisible(true);
            
            // Hide mixer view
            if (m_mixerView)
                m_mixerView->setVisible(false);
        }
        else if (m_activeView == 1)
        {
            // Mixer view uses the normal content area
            if (m_mixerView)
            {
                m_mixerView->setBounds(contentArea);
                m_mixerView->setVisible(true);
            }
            
            // Hide sequencer components
            if (m_trackViewport)
                m_trackViewport->setVisible(false);
            if (m_stageViewport)
                m_stageViewport->setVisible(false);
        }
        
        // Keep content container for mixer view
        if (m_contentContainer)
            m_contentContainer->setBounds(contentArea);

    }
    
    void paint(juce::Graphics& g)
    {
        // Dark background with subtle gradient to verify rendering
        g.fillAll(juce::Colour(0xFF1A1A1A)); // Very dark gray instead of pure black
        
        // Draw pattern bar area
        if (!m_patternBarBounds.isEmpty())
        {
            // Dark raised panel for pattern bar
            g.setColour(juce::Colour(HAM::UI::DesignTokens::Colors::BG_RAISED));
            g.fillRect(m_patternBarBounds);
            
            // Bottom border
            g.setColour(juce::Colour(HAM::UI::DesignTokens::Colors::BORDER).withAlpha(0.3f));
            g.drawHorizontalLine(m_patternBarBounds.getBottom() - 1, 
                                m_patternBarBounds.getX(), 
                                m_patternBarBounds.getRight());
        }
    }
    
    void paintOverChildren(juce::Graphics& g)
    {
        // Grid overlay disabled - uncomment to show coordinate grid for UI positioning
        // drawCoordinateGrid(g);
    }
    
private:
    // Helpers exposed to MainComponent for key handling
public:
    void togglePlay()
    {
        if (!m_transportBar) return;
        bool nowPlaying = !m_transportBar->isPlaying();
        m_transportBar->setPlayState(nowPlaying);
        handlePlayStateChange(nowPlaying);
    }

    void stepStageLeft()
    {
        int next = juce::jlimit(0, 7, m_currentStageIndex - 1);
        if (next != m_currentStageIndex)
        {
            updateCurrentStage(m_currentTrackIndex, next);
        }
    }

    void stepStageRight()
    {
        int next = juce::jlimit(0, 7, m_currentStageIndex + 1);
        if (next != m_currentStageIndex)
        {
            updateCurrentStage(m_currentTrackIndex, next);
        }
    }
    void timerCallback() override
    {
        // Process messages from engine to UI
        if (m_messageDispatcher) m_messageDispatcher->processEngineMessages(50);
    }
    
    void setupMessageHandlers()
    {
        using namespace HAM;
        
        // Register handlers for engine messages
        m_messageDispatcher->registerEngineHandler(
            EngineToUIMessage::TRANSPORT_STATUS,
            [this](const EngineToUIMessage& msg) {
                updateTransportStatus(msg.data.transport.playing, 
                                    msg.data.transport.recording,
                                    msg.data.transport.bpm);
            }
        );
        
        m_messageDispatcher->registerEngineHandler(
            EngineToUIMessage::PLAYHEAD_POSITION,
            [this](const EngineToUIMessage& msg) {
                updatePlayheadPosition(msg.data.playhead.bars,
                                     msg.data.playhead.beats,
                                     msg.data.playhead.pulses);
            }
        );
        
        m_messageDispatcher->registerEngineHandler(
            EngineToUIMessage::CURRENT_STAGE,
            [this](const EngineToUIMessage& msg) {
                updateCurrentStage(msg.data.position.track,
                                 msg.data.position.stage);
            }
        );
    }
    
    void handlePlayStateChange(bool playing)
    {
        // Send play/stop message to engine
        auto msg = playing ? 
            HAM::MessageFactory::makePlayMessage() :
            HAM::MessageFactory::makeStopMessage();
            
        m_messageDispatcher->sendToEngine(msg);
        
        DBG("Transport: " << (playing ? "Playing" : "Stopped"));
    }
    
    void handleBPMChange(float bpm)
    {
        // Send BPM change to engine
        auto msg = HAM::MessageFactory::setBPM(bpm);
        m_messageDispatcher->sendToEngine(msg);
        
        DBG("BPM changed to: " << bpm);
    }

    void handleMidiMonitorToggle(bool enabled)
    {
        HAM::UIToEngineMessage msg;
        msg.type = enabled ? HAM::UIToEngineMessage::ENABLE_DEBUG_MODE
                            : HAM::UIToEngineMessage::DISABLE_DEBUG_MODE;
        msg.data.boolParam.value = enabled;
        m_messageDispatcher->sendToEngine(msg);
        DBG("MIDI Monitor " << (enabled ? "ENABLED" : "DISABLED"));
    }
    
    void handleStageParameterChange(int track, int stage, const juce::String& param, float value)
    {
        using namespace HAM;
        
        UIToEngineMessage msg;
        
        if (param == "pitch") {
            msg = MessageFactory::setStagePitch(track, stage, value);
        } else if (param == "velocity") {
            msg.type = UIToEngineMessage::SET_STAGE_VELOCITY;
            msg.data.stageParam = {track, stage, value};
        } else if (param == "gate") {
            msg.type = UIToEngineMessage::SET_STAGE_GATE;
            msg.data.stageParam = {track, stage, value};
        } else if (param == "pulse") {
            msg.type = UIToEngineMessage::SET_STAGE_PULSE_COUNT;
            msg.data.stageParam = {track, stage, value};
        } else {
            return; // Unknown parameter
        }
        
        m_messageDispatcher->sendToEngine(msg);
        
        DBG("Stage " << stage << " " << param << " changed to: " << value);
    }
    
    void updateTransportStatus(bool playing, bool recording, float bpm)
    {
        // Update transport UI
        if (m_transportBar) {
            m_transportBar->setPlayState(playing);
            m_transportBar->setBPM(bpm);
        }
    }
    
    void updatePlayheadPosition(float bars, float beats, float pulses)
    {
        // Update position display (future feature)
        m_currentBar = static_cast<int>(bars);
        m_currentBeat = static_cast<int>(beats);
        m_currentPulse = static_cast<int>(pulses);
        if (m_transportBar)
        {
            m_transportBar->setPosition(m_currentBar, m_currentBeat, m_currentPulse);
        }
    }
    
    void updateCurrentStage(int track, int stage)
    {
        // Update stage grid to show active stage
        // For now, we only support single track, so ignore track parameter
        if (m_stageGrid) {
            m_stageGrid->setActiveStage(stage);
        }

        // TODO: Implement HAMEditorPanel
        // if (m_hamEditor)
        // {
        //     m_hamEditor->setCurrentStage(track, stage);
        // }

        m_currentStageIndex = stage;
    }
    
    void handleTrackSelection(int trackIndex)
    {
        // Send track selection to engine
        auto msg = HAM::UIToEngineMessage();
        msg.type = HAM::UIToEngineMessage::UPDATE_TRACK;
        msg.data.trackParam = {trackIndex, 0}; // 0 as placeholder value
        m_messageDispatcher->sendToEngine(msg);
        
        DBG("Track " << trackIndex << " selected");

        m_currentTrackIndex = trackIndex;
    }
    
    void handleTrackParameterChange(int trackIndex, const juce::String& param, float value)
    {
        using namespace HAM;
        UIToEngineMessage msg;
        
        if (param == "mute") {
            msg.type = UIToEngineMessage::SET_TRACK_MUTE;
            msg.data.trackParam = {trackIndex, static_cast<int>(value)};
        } else if (param == "solo") {
            msg.type = UIToEngineMessage::SET_TRACK_SOLO;
            msg.data.trackParam = {trackIndex, static_cast<int>(value)};
        } else if (param == "channel") {
            msg.type = UIToEngineMessage::SET_TRACK_CHANNEL;
            msg.data.trackParam = {trackIndex, static_cast<int>(value)};
        } else if (param == "voiceMode") {
            msg.type = UIToEngineMessage::SET_TRACK_VOICE_MODE;
            msg.data.trackParam = {trackIndex, static_cast<int>(value)};
        } else if (param == "division") {
            msg.type = UIToEngineMessage::SET_TRACK_DIVISION;
            msg.data.trackParam = {trackIndex, static_cast<int>(value)};
        } else if (param == "swing") {
            msg.type = UIToEngineMessage::SET_SWING;
            msg.data.floatParam = {value / 100.0f}; // Convert from 0-100 to 0-1
        } else if (param == "openPlugin") {
            // UI: Plugin Browser öffnen (nicht in Engine senden)
            openPluginBrowser(trackIndex);
            return;
        } else if (param == "octave") {
            // Note: No specific message type for octave, would need to be added
            // For now, we can log it
            DBG("Track " << trackIndex << " octave: " << value);
            return;
        } else {
            return; // Unknown parameter
        }
        
        m_messageDispatcher->sendToEngine(msg);
        DBG("Track " << trackIndex << " " << param << " changed to: " << value);
    }
    
    void handleAddTrack()
    {
        // Append neuer Track direkt nach Track 1 (Index-basiert am Ende)
        m_numTracks = std::max(1, m_numTracks + 1);
        int newTrackIndex = m_numTracks - 1;
        // UI aktualisieren - WICHTIG: StageGrid muss auch aktualisiert werden!
        if (m_trackSidebar) m_trackSidebar->setTrackCount(m_numTracks);
        if (m_stageGrid) m_stageGrid->setTrackCount(m_numTracks);
        if (m_mixerView) m_mixerView->setTrackCount(m_numTracks);
        // Engine informieren
        auto msg = HAM::MessageFactory::addTrack(newTrackIndex);
        if (m_messageDispatcher) m_messageDispatcher->sendToEngine(msg);
        DBG("Add track requested -> index=" << newTrackIndex << ", total=" << m_numTracks);
    }
    
    void handleRemoveTrack()
    {
        // Don't remove the last track
        if (m_numTracks <= 1) 
        {
            DBG("Cannot remove last track");
            return;
        }
        
        // Remove the last track
        m_numTracks = std::max(1, m_numTracks - 1);
        int removedTrackIndex = m_numTracks; // The track that was removed
        
        // If we were viewing the removed track, switch to the last remaining track
        if (m_currentTrackIndex >= m_numTracks)
        {
            m_currentTrackIndex = m_numTracks - 1;
            handleTrackSelection(m_currentTrackIndex);
        }
        
        // Update UI - WICHTIG: StageGrid muss auch aktualisiert werden!
        if (m_trackSidebar) m_trackSidebar->setTrackCount(m_numTracks);
        if (m_stageGrid) m_stageGrid->setTrackCount(m_numTracks);
        if (m_mixerView) m_mixerView->setTrackCount(m_numTracks);
        
        // Inform engine about track removal
        auto msg = HAM::UIToEngineMessage();
        msg.type = HAM::UIToEngineMessage::REMOVE_TRACK;
        msg.data.trackParam = {removedTrackIndex, 0};
        if (m_messageDispatcher) m_messageDispatcher->sendToEngine(msg);
        
        DBG("Remove track -> removed index=" << removedTrackIndex << ", remaining=" << m_numTracks);
    }
    
    void setActiveView(int viewIndex)
    {
        m_activeView = viewIndex;
        
        // Update button styles
        if (viewIndex == 0) // Sequencer
        {
            if (m_sequencerTabButton) 
            {
                m_sequencerTabButton->setButtonStyle(HAM::UI::ModernButton::Style::Solid);
                m_sequencerTabButton->repaint();
            }
            if (m_mixerTabButton)
            {
                m_mixerTabButton->setButtonStyle(HAM::UI::ModernButton::Style::Outline);
                m_mixerTabButton->repaint();
            }
            // Visibility is now handled in resized()
        }
        else if (viewIndex == 1) // Mixer
        {
            if (m_sequencerTabButton)
            {
                m_sequencerTabButton->setButtonStyle(HAM::UI::ModernButton::Style::Outline);
                m_sequencerTabButton->repaint();
            }
            if (m_mixerTabButton)
            {
                m_mixerTabButton->setButtonStyle(HAM::UI::ModernButton::Style::Solid);
                m_mixerTabButton->repaint();
            }
            // Visibility is now handled in resized()
        }
        
        m_owner.resized(); // Re-layout
        DBG("Switched to view: " << (viewIndex == 0 ? "Sequencer" : "Mixer"));
    }

    void openPluginBrowser(int trackIndex)
    {
        if (!m_pluginBrowser)
        {
            m_pluginBrowser = std::make_unique<HAM::UI::PluginBrowser>();
            m_pluginBrowser->onPluginChosen = [this, trackIndex](const juce::PluginDescription& desc){
                instantiatePluginForTrack(desc, trackIndex);
            };
        }
        if (!m_overlay)
        {
            m_overlay = std::make_unique<juce::Component>();
            m_overlay->setInterceptsMouseClicks(true, true);
            m_overlay->setAlwaysOnTop(true);
            if (m_contentContainer)
                m_contentContainer->addAndMakeVisible(m_overlay.get());
            else
                m_owner.addAndMakeVisible(m_overlay.get());
        }
        // Layout: zentriertes Panel innerhalb des Content Container
        auto hostBounds = (m_contentContainer ? m_contentContainer->getBounds() : m_owner.getLocalBounds());
        auto b = hostBounds.reduced(120, 80);
        m_overlay->setBounds(hostBounds);
        m_pluginBrowser->setBounds(b);
        m_overlay->addAndMakeVisible(m_pluginBrowser.get());
        m_overlay->toFront(true);
    }

    void openFxPluginBrowser(int trackIndex)
    {
        // Re-use PluginBrowser for FX insert selection; upon choose, add to FX list
        if (!m_pluginBrowser)
        {
            m_pluginBrowser = std::make_unique<HAM::UI::PluginBrowser>();
            m_pluginBrowser->onPluginChosen = [this, trackIndex](const juce::PluginDescription& desc){
                instantiateFxForTrack(desc, trackIndex);
            };
        }
        if (!m_overlay)
        {
            m_overlay = std::make_unique<juce::Component>();
            m_overlay->setInterceptsMouseClicks(true, true);
            m_overlay->setAlwaysOnTop(true);
            if (m_contentContainer)
                m_contentContainer->addAndMakeVisible(m_overlay.get());
            else
                m_owner.addAndMakeVisible(m_overlay.get());
        }
        auto hostBounds2 = (m_contentContainer ? m_contentContainer->getBounds() : m_owner.getLocalBounds());
        auto b2 = hostBounds2.reduced(120, 80);
        m_overlay->setBounds(hostBounds2);
        m_pluginBrowser->setBounds(b2);
        m_overlay->addAndMakeVisible(m_pluginBrowser.get());
        m_overlay->toFront(true);
    }

    void instantiatePluginForTrack(const juce::PluginDescription& desc, int trackIndex)
    {
        // Zuerst Sandbox-Probe in separatem Prozess (PluginProbeWorker)
        auto exe = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
        // MacOS -> Contents -> *.app -> Release -> HAM_artefacts -> build (6 Ebenen)
        auto dir = exe; for (int i = 0; i < 6; ++i) dir = dir.getParentDirectory();
        auto worker1 = dir.getChildFile("bin").getChildFile("PluginProbeWorker");
        auto worker2 = dir.getChildFile("PluginProbeWorker_artefacts").getChildFile("Release").getChildFile("PluginProbeWorker");
        juce::File worker = worker1.existsAsFile() ? worker1 : worker2;

        struct ProbeResult { bool success{false}; bool usedRosetta{false}; };
        auto runProbe = [](const juce::File& workerFile, const juce::String& fmtName, const juce::String& ident) -> ProbeResult
        {
            auto escapeArg = [](const juce::String& s){ return juce::String("\"") + s.replace("\"", "\\\"") + "\""; };
            auto baseCmd = workerFile.getFullPathName() + " " + escapeArg(fmtName) + " " + escapeArg(ident);

            auto runAndWait = [](const juce::String& cmd) -> std::optional<int>
            {
                juce::ChildProcess p;
                if (!p.start(cmd)) return std::nullopt;
                for (int i = 0; i < 120; ++i) { if (!p.isRunning()) break; juce::Thread::sleep(50); }
                return p.getExitCode();
            };

            if (auto ec = runAndWait(baseCmd))
            {
                if (*ec == 0) return { true, false };
            }

            // Fallback: Rosetta (x86_64) starten, falls Plugin nur Intel ist
            juce::String rosettaCmd = "/usr/bin/arch -x86_64 " + baseCmd;
            if (auto ec2 = runAndWait(rosettaCmd))
            {
                if (*ec2 == 0) return { true, true };
            }
            return { false, false };
        };

        if (worker.existsAsFile())
        {
            juce::String fmtName = desc.pluginFormatName;
            juce::String ident = desc.fileOrIdentifier.isNotEmpty() ? desc.fileOrIdentifier : desc.name;
            ProbeResult probe = runProbe(worker, fmtName, ident);
            // Selbst wenn der Probe-Test fehlschlug, versuche die Bridge (manche Formate/IDs schlagen im Probe-Worker fehl)
            bool useRosetta = probe.usedRosetta;
            launchPluginEditorBridge(fmtName, ident, useRosetta);
            // Browser schließen
            m_pluginBrowser.reset();
            return;
        }
        else
        {
            // Falls kein Probe-Worker vorhanden ist, direkt Bridge versuchen
            juce::String fmtName = desc.pluginFormatName;
            juce::String ident = desc.fileOrIdentifier.isNotEmpty() ? desc.fileOrIdentifier : desc.name;
            launchPluginEditorBridge(fmtName, ident, false);
            m_pluginBrowser.reset();
            return;
        }

        // In-Process Instanziierung wird durch Bridge ersetzt
        juce::ignoreUnused(desc, trackIndex);
        return;
    }

    void instantiateFxForTrack(const juce::PluginDescription& desc, int trackIndex)
    {
        // Sandbox-Probe identisch wie bei Instrument, inkl. Rosetta-Fallback
        auto exe = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
        auto dir = exe; for (int i = 0; i < 6; ++i) dir = dir.getParentDirectory();
        auto worker1 = dir.getChildFile("bin").getChildFile("PluginProbeWorker");
        auto worker2 = dir.getChildFile("PluginProbeWorker_artefacts").getChildFile("Release").getChildFile("PluginProbeWorker");
        juce::File worker = worker1.existsAsFile() ? worker1 : worker2;
        if (!worker.existsAsFile())
        {
            // Direkt Bridge nutzen, wenn kein Probe-Worker existiert
            juce::String fmtName = desc.pluginFormatName;
            juce::String ident = desc.fileOrIdentifier.isNotEmpty() ? desc.fileOrIdentifier : desc.name;
            launchPluginEditorBridge(fmtName, ident, false);
            m_pluginBrowser.reset();
            return;
        }

        struct ProbeResult { bool success{false}; bool usedRosetta{false}; };
        auto runProbe = [](const juce::File& workerFile, const juce::String& fmtName, const juce::String& ident) -> ProbeResult
        {
            auto escapeArg = [](const juce::String& s){ return juce::String("\"") + s.replace("\"", "\\\"") + "\""; };
            auto baseCmd = workerFile.getFullPathName() + " " + escapeArg(fmtName) + " " + escapeArg(ident);

            auto runAndWait = [](const juce::String& cmd) -> std::optional<int>
            {
                juce::ChildProcess p;
                if (!p.start(cmd)) return std::nullopt;
                for (int i = 0; i < 120; ++i) { if (!p.isRunning()) break; juce::Thread::sleep(50); }
                return p.getExitCode();
            };

            if (auto ec = runAndWait(baseCmd))
            {
                if (*ec == 0) return { true, false };
            }

            juce::String rosettaCmd = "/usr/bin/arch -x86_64 " + baseCmd;
            if (auto ec2 = runAndWait(rosettaCmd))
            {
                if (*ec2 == 0) return { true, true };
            }
            return { false, false };
        };

        juce::String fmtName = desc.pluginFormatName;
        juce::String ident = desc.fileOrIdentifier.isNotEmpty() ? desc.fileOrIdentifier : desc.name;
        ProbeResult probe = runProbe(worker, fmtName, ident);
        // Auch bei fehlgeschlagenem Probe-Test: Bridge probieren
        launchPluginEditorBridge(fmtName, ident, probe.usedRosetta);
        // Schließe Browser
        m_pluginBrowser.reset();
        return;
    }

    // Pattern selection
    void selectPattern(int index)
    {
        if (index < 0 || index >= 8) return;
        m_activePatternIndex = index;
        updatePatternButtonStyles();
        // TODO: send message to engine when pattern scheduler is wired
        DBG("Pattern selected: " << static_cast<char>('A' + index));
    }

    void updatePatternButtonStyles()
    {
        auto activeColor = juce::Colour(HAM::UI::DesignTokens::Colors::ACCENT_BLUE);
        auto inactiveColor = juce::Colour(HAM::UI::DesignTokens::Colors::TEXT_DIM);
        for (int i = 0; i < static_cast<int>(m_patternButtons.size()); ++i)
        {
            if (m_patternButtons[i])
                m_patternButtons[i]->setColor(i == m_activePatternIndex ? activeColor : inactiveColor);
        }
    }
    
    MainComponent& m_owner;
    
    // Message system (from audio processor)
    HAM::MessageDispatcher* m_messageDispatcher { nullptr };
    
    // UI Components
    std::unique_ptr<HAM::UI::TransportBar> m_transportBar;
    // View switching
    std::unique_ptr<juce::Component> m_contentContainer;
    std::unique_ptr<HAM::UI::ModernButton> m_sequencerTabButton;
    std::unique_ptr<HAM::UI::ModernButton> m_mixerTabButton;
    int m_activeView = 0; // 0 = Sequencer, 1 = Mixer
    // Pages
    std::unique_ptr<juce::Component> m_sequencerPage;
    std::unique_ptr<HAM::UI::MixerView> m_mixerView;
    std::unique_ptr<HAM::UI::StageGrid> m_stageGrid;
    std::unique_ptr<juce::Viewport> m_stageViewport;
    std::unique_ptr<HAM::UI::TrackSidebar> m_trackSidebar;
    std::unique_ptr<juce::Viewport> m_trackViewport;
    // std::unique_ptr<HAM::UI::HAMEditorPanel> m_hamEditor; // TODO: Implement HAMEditorPanel
    std::unique_ptr<HAM::UI::ModernButton> m_addTrackButton;
    std::unique_ptr<HAM::UI::ModernButton> m_removeTrackButton;
    std::vector<std::unique_ptr<HAM::UI::ModernButton>> m_patternButtons;
    std::unique_ptr<juce::Component> m_overlay;
    std::unique_ptr<HAM::UI::PluginBrowser> m_pluginBrowser;
    std::unordered_map<int, std::unique_ptr<juce::AudioPluginInstance>> m_trackPlugins;
    std::unordered_map<int, std::vector<std::unique_ptr<juce::AudioPluginInstance>>> m_trackFxPlugins;
    std::unique_ptr<juce::AudioProcessorEditor> m_pluginEditor;
    std::unique_ptr<juce::ChildProcess> m_bridgeProcess;

    // Audio hosting
    juce::AudioDeviceManager m_deviceManager;
    juce::AudioProcessorPlayer m_audioPlayer;
    std::unique_ptr<HAM::HAMAudioProcessor> m_processor;
    
    // Current state
    int m_currentBar = 0;
    int m_currentBeat = 0;
    int m_currentPulse = 0;
    int m_currentStageIndex = 0;
    int m_currentTrackIndex = 0;
    int m_numTracks = 1;
    int m_activePatternIndex = 0;
    
    // Pattern bar bounds for future use
    juce::Rectangle<int> m_patternBarBounds;
    
    // Helper function to draw coordinate grid
    void drawCoordinateGrid(juce::Graphics& g)
    {
        auto bounds = m_owner.getLocalBounds();
        const int gridSize = 24; // 24px grid for better visibility
        
        // Set grid line color - white with low opacity
        g.setColour(juce::Colours::white.withAlpha(0.15f));
        g.setFont(10.0f);
        
        // Draw vertical lines and labels (A-Z, then AA-AX)
        int colIndex = 0;
        for (int x = 0; x < bounds.getWidth(); x += gridSize)
        {
            g.drawVerticalLine(x, 0, bounds.getHeight());
            
            // Generate column label (A-Z, then AA-AX)
            juce::String label;
            if (colIndex < 26) {
                label = juce::String::charToString('A' + colIndex);
            } else {
                int firstChar = (colIndex - 26) / 26;
                int secondChar = (colIndex - 26) % 26;
                label = juce::String::charToString('A' + firstChar) + juce::String::charToString('A' + secondChar);
            }
            
            // Draw label at top
            g.setColour(juce::Colours::white.withAlpha(0.5f));
            g.drawText(label, x + 2, 2, gridSize - 4, 12, juce::Justification::left, false);
            g.setColour(juce::Colours::white.withAlpha(0.15f));
            
            colIndex++;
        }
        
        // Draw horizontal lines and labels (1-50)
        int rowIndex = 1;
        for (int y = 0; y < bounds.getHeight(); y += gridSize)
        {
            g.drawHorizontalLine(y, 0, bounds.getWidth());
            
            // Draw row number on the left
            g.setColour(juce::Colours::white.withAlpha(0.5f));
            g.drawText(juce::String(rowIndex), 2, y + 2, 20, gridSize - 4, juce::Justification::left, false);
            g.setColour(juce::Colours::white.withAlpha(0.15f));
            
            rowIndex++;
        }
        
        // Draw more prominent lines every 5 cells
        g.setColour(juce::Colours::white.withAlpha(0.25f));
        for (int x = 0; x < bounds.getWidth(); x += gridSize * 5)
        {
            g.drawVerticalLine(x, 0, bounds.getHeight());
        }
        for (int y = 0; y < bounds.getHeight(); y += gridSize * 5)
        {
            g.drawHorizontalLine(y, 0, bounds.getWidth());
        }
    }

    // Bridge launcher helpers
    static juce::File findPluginHostBridge()
    {
        auto exe = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
        auto dir = exe; for (int i = 0; i < 6; ++i) dir = dir.getParentDirectory();
        auto candidateApp = dir.getChildFile("PluginHostBridge_artefacts").getChildFile("Release")
                               .getChildFile("PluginHostBridge.app").getChildFile("Contents")
                               .getChildFile("MacOS").getChildFile("PluginHostBridge");
        if (candidateApp.existsAsFile()) return candidateApp;
        auto candidateBin = dir.getChildFile("bin").getChildFile("PluginHostBridge");
        if (candidateBin.existsAsFile()) return candidateBin;
        return {};
    }

    void launchPluginEditorBridge(const juce::String& formatName, const juce::String& identifier, bool useRosetta)
    {
        auto bridge = findPluginHostBridge();
        if (!bridge.existsAsFile())
        {
            juce::AlertWindow::showMessageBoxAsync(
                juce::MessageBoxIconType::WarningIcon,
                "Bridge fehlt",
                "PluginHostBridge wurde nicht gefunden.");
            return;
        }

        // Choose a localhost port for window control (SHOW/HIDE)
        const int ipcPort = 53621; // fixed for MVP; could be randomized if multiple instances are needed
        auto escapeArg = [](const juce::String& s){ return juce::String("\"") + s.replace("\"", "\\\"") + "\""; };
        juce::String cmd = bridge.getFullPathName()
                            + " --format=" + escapeArg(formatName)
                            + " --identifier=" + escapeArg(identifier)
                            + " --port=" + juce::String(ipcPort);
       #if JUCE_MAC
        if (useRosetta)
            cmd = "/usr/bin/arch -x86_64 " + cmd;
       #endif
        m_bridgeProcess = std::make_unique<juce::ChildProcess>();
        if (!m_bridgeProcess->start(cmd))
        {
            m_bridgeProcess.reset();
            juce::AlertWindow::showMessageBoxAsync(
                juce::MessageBoxIconType::WarningIcon,
                "Bridge Start fehlgeschlagen",
                "Der PluginHostBridge-Prozess konnte nicht gestartet werden.");
            return;
        }

        // Fire-and-forget: bring window to front after short delay
        juce::Timer::callAfterDelay(300, [ipcPort]() {
            juce::StreamingSocket s;
            if (s.connect("127.0.0.1", ipcPort, 1000))
            {
                const char* msg = "SHOW";
                s.write(msg, (int) std::strlen(msg));
            }
        });
    }
};

//==============================================================================
MainComponent::MainComponent()
{
    m_impl = std::make_unique<Impl>(*this);
    
    // Set default size
    setSize(1200, 600);
    
    // Apply Pulse look and feel
    setLookAndFeel(&m_pulseLookAndFeel);

    setWantsKeyboardFocus(true);
    grabKeyboardFocus();
}

MainComponent::~MainComponent()
{
    setLookAndFeel(nullptr);
}

//==============================================================================
void MainComponent::paint(juce::Graphics& g)
{
    m_impl->paint(g);
}

void MainComponent::paintOverChildren(juce::Graphics& g)
{
    m_impl->paintOverChildren(g);
}

void MainComponent::resized()
{
    m_impl->resized();
}

bool MainComponent::keyPressed(const juce::KeyPress& key)
{
    // Space: toggle play/pause
    if (key == juce::KeyPress(juce::KeyPress::spaceKey))
    {
        if (m_impl) m_impl->togglePlay();
        return true;
    }
    // Left/Right: change active stage within 0..7
    if (key == juce::KeyPress(juce::KeyPress::leftKey) || key == juce::KeyPress(juce::KeyPress::rightKey))
    {
        if (!m_impl) return false;
        if (key == juce::KeyPress(juce::KeyPress::leftKey)) m_impl->stepStageLeft();
        if (key == juce::KeyPress(juce::KeyPress::rightKey)) m_impl->stepStageRight();
        return true;
    }
    return false;
}