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
        
        // Create main panel container
        m_mainPanel = std::make_unique<HAM::UI::Panel>(HAM::UI::Panel::Style::Flat);
        m_owner.addAndMakeVisible(m_mainPanel.get());
        
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
        
        // Stage grid in center (will expand to multiple tracks later)
        m_stageGrid->setBounds(contentArea);
        
        // Background panel
        m_mainPanel->setBounds(m_owner.getLocalBounds());
    }
    
    void paint(juce::Graphics& g)
    {
        // Pure black background (Pulse dark void aesthetic)
        g.fillAll(juce::Colour(HAM::UI::DesignTokens::Colors::BG_VOID));
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
    
    MainComponent& m_owner;
    
    // Message system
    std::unique_ptr<HAM::MessageDispatcher> m_messageDispatcher;
    
    // UI Components
    std::unique_ptr<HAM::UI::Panel> m_mainPanel;
    std::unique_ptr<HAM::UI::TransportBar> m_transportBar;
    std::unique_ptr<HAM::UI::StageGrid> m_stageGrid;
    
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