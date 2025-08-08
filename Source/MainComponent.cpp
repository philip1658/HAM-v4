/*
  ==============================================================================

    MainComponent.cpp
    Main component for HAM sequencer with Pulse UI components

  ==============================================================================
*/

#include "MainComponent.h"
#include "Infrastructure/Messaging/MessageDispatcher.h"
#include "Infrastructure/Messaging/MessageTypes.h"
#include "UI/Components/HAMComponentLibrary.h"
#include "UI/Components/HAMComponentLibraryExtended.h"
#include "Presentation/Core/DesignSystem.h"
#include "Presentation/Views/StageGrid.h"
#include "Presentation/Views/TransportBar.h"
#include "Presentation/Views/TrackSidebar.h"

//==============================================================================
class MainComponent::Impl : public juce::Timer
{
public:
    Impl(MainComponent& owner) : m_owner(owner)
    {
        // Initialize message dispatcher
        m_messageDispatcher = std::make_unique<HAM::MessageDispatcher>();
        setupMessageHandlers();
        
        // Create transport bar with Pulse components
        m_transportBar = std::make_unique<HAM::UI::TransportBar>();
        m_transportBar->onPlayStateChanged = [this](bool playing) {
            handlePlayStateChange(playing);
        };
        m_transportBar->onBPMChanged = [this](float bpm) {
            handleBPMChange(bpm);
        };
        m_owner.addAndMakeVisible(m_transportBar.get());
        
        // Create stage grid
        m_stageGrid = std::make_unique<HAM::UI::StageGrid>();
        m_stageGrid->onStageParameterChanged = [this](int track, int stage, const juce::String& param, float value) {
            handleStageParameterChange(track, stage, param, value);
        };
        m_owner.addAndMakeVisible(m_stageGrid.get());
        
        // Create track sidebar
        m_trackSidebar = std::make_unique<HAM::UI::TrackSidebar>();
        m_trackSidebar->onTrackSelected = [this](int trackIndex) {
            handleTrackSelection(trackIndex);
        };
        m_trackSidebar->onTrackParameterChanged = [this](int trackIndex, const juce::String& param, float value) {
            handleTrackParameterChange(trackIndex, param, value);
        };
        m_owner.addAndMakeVisible(m_trackSidebar.get());
        
        // Create Add Track button for pattern bar
        m_addTrackButton = std::make_unique<HAM::UI::ModernButton>("+ ADD TRACK", HAM::UI::ModernButton::Style::Gradient);
        m_addTrackButton->setColor(juce::Colour(0xFF00CCFF)); // Bright cyan
        m_addTrackButton->onClick = [this]() {
            handleAddTrack();
        };
        m_owner.addAndMakeVisible(m_addTrackButton.get());
        
        // Note: Removed main panel as it was covering other components
        
        // Start timer for UI updates from engine
        startTimer(30); // ~33Hz UI update rate
    }
    
    ~Impl()
    {
        stopTimer();
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
        
        // Position Add Track button on the left side of pattern bar
        if (m_addTrackButton)
        {
            auto buttonBounds = patternBar.reduced(5);
            buttonBounds.setWidth(120);
            m_addTrackButton->setBounds(buttonBounds);
        }
        
        // Calculate exact remaining height for perfect alignment
        auto contentArea = bounds;  // This is now exactly 480px high (600 - 80 - 40)
        
        // Track sidebar on left - extended to line L (264px = 11 * 24px grid)
        static constexpr int SIDEBAR_WIDTH = 264;  // Line L
        auto sidebarBounds = contentArea.removeFromLeft(SIDEBAR_WIDTH);
        m_trackSidebar->setBounds(sidebarBounds);
        
        // Stage grid takes ALL remaining space - no gaps
        // This ensures both components have the exact same height
        m_stageGrid->setBounds(contentArea);
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
        // Draw coordinate grid overlay on top of all children
        drawCoordinateGrid(g);
    }
    
private:
    void timerCallback() override
    {
        // Process messages from engine to UI
        m_messageDispatcher->processEngineMessages(50);
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
        // For now just track it
        m_currentBar = static_cast<int>(bars);
        m_currentBeat = static_cast<int>(beats);
        m_currentPulse = static_cast<int>(pulses);
    }
    
    void updateCurrentStage(int track, int stage)
    {
        // Update stage grid to show active stage
        // For now, we only support single track, so ignore track parameter
        if (m_stageGrid) {
            m_stageGrid->setActiveStage(stage);
        }
    }
    
    void handleTrackSelection(int trackIndex)
    {
        // Send track selection to engine
        auto msg = HAM::UIToEngineMessage();
        msg.type = HAM::UIToEngineMessage::UPDATE_TRACK;
        msg.data.trackParam = {trackIndex, 0}; // 0 as placeholder value
        m_messageDispatcher->sendToEngine(msg);
        
        DBG("Track " << trackIndex << " selected");
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
        // For now, just log - actual implementation would add a new track to the model
        DBG("Add track requested");
        // In future: send message to engine to add track
    }
    
    MainComponent& m_owner;
    
    // Message system
    std::unique_ptr<HAM::MessageDispatcher> m_messageDispatcher;
    
    // UI Components
    std::unique_ptr<HAM::UI::TransportBar> m_transportBar;
    std::unique_ptr<HAM::UI::StageGrid> m_stageGrid;
    std::unique_ptr<HAM::UI::TrackSidebar> m_trackSidebar;
    std::unique_ptr<HAM::UI::ModernButton> m_addTrackButton;
    
    // Current state
    int m_currentBar = 0;
    int m_currentBeat = 0;
    int m_currentPulse = 0;
    
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
};

//==============================================================================
MainComponent::MainComponent()
{
    m_impl = std::make_unique<Impl>(*this);
    
    // Set default size
    setSize(1200, 600);
    
    // Apply Pulse look and feel
    setLookAndFeel(&m_pulseLookAndFeel);
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