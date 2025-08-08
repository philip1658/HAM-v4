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
        m_trackSidebar->onAddTrack = [this]() {
            handleAddTrack();
        };
        m_owner.addAndMakeVisible(m_trackSidebar.get());
        
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
        m_transportBar->setBounds(bounds.removeFromTop(80));
        
        // Main content area
        auto contentArea = bounds.reduced(8);
        
        // Track sidebar on left (250px for fixed 480px height design)
        static constexpr int SIDEBAR_WIDTH = 250;
        m_trackSidebar->setBounds(contentArea.removeFromLeft(SIDEBAR_WIDTH));
        
        // Add some spacing between sidebar and main content
        contentArea.removeFromLeft(8);
        
        // Stage grid takes remaining space (will expand to multiple tracks later)
        m_stageGrid->setBounds(contentArea);
    }
    
    void paint(juce::Graphics& g)
    {
        // Dark background with subtle gradient to verify rendering
        g.fillAll(juce::Colour(0xFF1A1A1A)); // Very dark gray instead of pure black
        
        // Add subtle grid lines to see if rendering works
        g.setColour(juce::Colours::white.withAlpha(0.05f));
        for (int i = 0; i < m_owner.getWidth(); i += 50)
        {
            g.drawVerticalLine(i, 0, m_owner.getHeight());
        }
        for (int i = 0; i < m_owner.getHeight(); i += 50)
        {
            g.drawHorizontalLine(i, 0, m_owner.getWidth());
        }
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
    
    // Current state
    int m_currentBar = 0;
    int m_currentBeat = 0;
    int m_currentPulse = 0;
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

void MainComponent::resized()
{
    m_impl->resized();
}