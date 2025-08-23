/*
  ==============================================================================

    AppController.cpp
    Business logic coordination and engine management for HAM sequencer

  ==============================================================================
*/

#include "AppController.h"
#include "Infrastructure/Messaging/MessageDispatcher.h"
#include "Infrastructure/Messaging/MessageTypes.h"
#include "Infrastructure/Audio/HAMAudioProcessor.h"

namespace HAM {
namespace UI {

//==============================================================================
AppController::AppController()
{
    initializeAudio();
    
    // Start performance monitoring timer (10Hz)
    startTimer(100);
}

AppController::~AppController()
{
    stopTimer();
    shutdownAudio();
}

//==============================================================================
void AppController::initializeAudio()
{
    if (m_audioInitialized)
        return;
    
    // Initialize audio engine and messaging bridge
    m_processor = std::make_unique<HAMAudioProcessor>();
    m_messageDispatcher = &m_processor->getMessageDispatcher();
    setupMessageHandlers();
    
    // Setup audio I/O and attach processor
    m_deviceManager.initialiseWithDefaultDevices(0, 2);
    m_audioPlayer.setProcessor(m_processor.get());
    m_deviceManager.addAudioCallback(&m_audioPlayer);
    
    m_audioInitialized = true;
}

void AppController::shutdownAudio()
{
    if (!m_audioInitialized)
        return;
    
    stop();
    m_deviceManager.removeAudioCallback(&m_audioPlayer);
    m_audioPlayer.setProcessor(nullptr);
    m_processor.reset();
    m_messageDispatcher = nullptr;
    
    m_audioInitialized = false;
}

//==============================================================================
void AppController::play()
{
    if (!m_audioInitialized)
        return;
    
    m_isPlaying = true;
    
    if (m_messageDispatcher)
    {
        UIToEngineMessage msg;
        msg.type = UIToEngineMessage::TRANSPORT_PLAY;
        msg.data.floatParam.value = m_currentBPM.load();
        m_messageDispatcher->sendToEngine(msg);
    }
}

void AppController::stop()
{
    if (!m_audioInitialized)
        return;
    
    m_isPlaying = false;
    
    if (m_messageDispatcher)
    {
        UIToEngineMessage msg;
        msg.type = UIToEngineMessage::TRANSPORT_STOP;
        m_messageDispatcher->sendToEngine(msg);
    }
}

void AppController::pause()
{
    // For now, pause is the same as stop
    // TODO: Implement proper pause that maintains position
    stop();
}

void AppController::setBPM(float bpm)
{
    m_currentBPM = juce::jlimit(20.0f, 300.0f, bpm);
    
    if (m_messageDispatcher)
    {
        UIToEngineMessage msg;
        msg.type = UIToEngineMessage::SET_BPM;
        msg.data.floatParam.value = m_currentBPM.load();
        m_messageDispatcher->sendToEngine(msg);
    }
}

//==============================================================================
void AppController::loadPattern(int patternIndex)
{
    if (patternIndex < 0 || patternIndex >= 128)
        return;
    
    m_currentPatternIndex = patternIndex;
    
    if (m_messageDispatcher)
    {
        UIToEngineMessage msg;
        msg.type = UIToEngineMessage::LOAD_PATTERN;
        msg.data.patternParam.patternId = patternIndex;
        m_messageDispatcher->sendToEngine(msg);
    }
}

void AppController::savePattern(int patternIndex)
{
    if (patternIndex < 0 || patternIndex >= 128)
        return;
    
    // TODO: Implement pattern saving
    markProjectDirty();
}

void AppController::clearPattern(int patternIndex)
{
    if (patternIndex < 0 || patternIndex >= 128)
        return;
    
    if (m_messageDispatcher)
    {
        UIToEngineMessage msg;
        msg.type = UIToEngineMessage::CLEAR_PATTERN;
        msg.data.patternParam.patternId = patternIndex;
        m_messageDispatcher->sendToEngine(msg);
    }
    
    markProjectDirty();
}

//==============================================================================
void AppController::setTrackMute(int trackIndex, bool muted)
{
    if (trackIndex < 0 || trackIndex >= m_trackStates.size())
        return;
    
    m_trackStates[trackIndex].muted = muted;
    
    if (m_messageDispatcher)
    {
        UIToEngineMessage msg;
        msg.type = UIToEngineMessage::TRACK_MUTE;
        msg.data.trackParam.trackIndex = trackIndex;
        msg.data.trackParam.value = muted ? 1 : 0;
        m_messageDispatcher->sendToEngine(msg);
    }
}

void AppController::setTrackSolo(int trackIndex, bool solo)
{
    if (trackIndex < 0 || trackIndex >= m_trackStates.size())
        return;
    
    m_trackStates[trackIndex].solo = solo;
    
    if (m_messageDispatcher)
    {
        UIToEngineMessage msg;
        msg.type = UIToEngineMessage::TRACK_SOLO;
        msg.data.trackParam.trackIndex = trackIndex;
        msg.data.trackParam.value = solo ? 1 : 0;
        m_messageDispatcher->sendToEngine(msg);
    }
}

void AppController::setTrackVolume(int trackIndex, float volume)
{
    if (trackIndex < 0 || trackIndex >= m_trackStates.size())
        return;
    
    m_trackStates[trackIndex].volume = juce::jlimit(0.0f, 1.0f, volume);
    
    if (m_messageDispatcher)
    {
        UIToEngineMessage msg;
        msg.type = UIToEngineMessage::TRACK_VOLUME;
        msg.data.trackParam.trackIndex = trackIndex;
        msg.data.trackParam.value = static_cast<int>(m_trackStates[trackIndex].volume * 127);
        m_messageDispatcher->sendToEngine(msg);
    }
}

void AppController::setTrackPan(int trackIndex, float pan)
{
    if (trackIndex < 0 || trackIndex >= m_trackStates.size())
        return;
    
    m_trackStates[trackIndex].pan = juce::jlimit(0.0f, 1.0f, pan);
    
    if (m_messageDispatcher)
    {
        UIToEngineMessage msg;
        msg.type = UIToEngineMessage::TRACK_PAN;
        msg.data.trackParam.trackIndex = trackIndex;
        msg.data.trackParam.value = static_cast<int>(m_trackStates[trackIndex].pan * 127);
        m_messageDispatcher->sendToEngine(msg);
    }
}

//==============================================================================
void AppController::updateStageParameter(int track, int stage, const juce::String& param, float value)
{
    if (!m_messageDispatcher)
        return;
    
    UIToEngineMessage msg;
    msg.data.stageParam.trackIndex = track;
    msg.data.stageParam.stageIndex = stage;
    msg.data.stageParam.value = value;
    
    // Map parameter name to message type
    if (param == "PITCH")
        msg.type = UIToEngineMessage::SET_STAGE_PITCH;
    else if (param == "VELOCITY")
        msg.type = UIToEngineMessage::SET_STAGE_VELOCITY;
    else if (param == "GATE")
        msg.type = UIToEngineMessage::SET_STAGE_GATE;
    else if (param == "PULSES")
        msg.type = UIToEngineMessage::SET_STAGE_PULSE_COUNT;
    else if (param == "RATCHETS")
        msg.type = UIToEngineMessage::SET_STAGE_RATCHETS;
    else
        return; // Unknown parameter
    
    m_messageDispatcher->sendToEngine(msg);
    markProjectDirty();
}

//==============================================================================
void AppController::loadPluginForTrack(int trackIndex, const juce::String& pluginPath)
{
    if (trackIndex < 0 || trackIndex >= m_trackStates.size())
        return;
    
    m_trackStates[trackIndex].pluginPath = pluginPath;
    
    // TODO: Actually load the plugin via PluginManager
    markProjectDirty();
}

void AppController::removePluginFromTrack(int trackIndex)
{
    if (trackIndex < 0 || trackIndex >= m_trackStates.size())
        return;
    
    m_trackStates[trackIndex].pluginPath.clear();
    
    // TODO: Actually remove the plugin via PluginManager
    markProjectDirty();
}

void AppController::showPluginEditorForTrack(int trackIndex)
{
    if (trackIndex < 0 || trackIndex >= m_trackStates.size())
        return;
    
    // TODO: Show plugin editor window
}

//==============================================================================
void AppController::newProject()
{
    // Clear all patterns
    for (int i = 0; i < 128; ++i)
    {
        clearPattern(i);
    }
    
    // Reset track states
    for (auto& track : m_trackStates)
    {
        track.muted = false;
        track.solo = false;
        track.volume = 0.8f;
        track.pan = 0.5f;
        track.pluginPath.clear();
    }
    
    // Reset transport
    stop();
    setBPM(120.0f);
    m_currentPatternIndex = 0;
    
    m_currentProjectFile = juce::File();
    m_hasUnsavedChanges = false;
}

void AppController::loadProject(const juce::File& file)
{
    if (!file.existsAsFile())
        return;
    
    // TODO: Implement project loading
    m_currentProjectFile = file;
    m_hasUnsavedChanges = false;
}

void AppController::saveProject(const juce::File& file)
{
    // TODO: Implement project saving
    m_currentProjectFile = file;
    m_hasUnsavedChanges = false;
}

//==============================================================================
void AppController::setMidiMonitorEnabled(bool enabled)
{
    m_midiMonitorEnabled = enabled;
    
    if (m_messageDispatcher)
    {
        UIToEngineMessage msg;
        msg.type = enabled ? UIToEngineMessage::ENABLE_DEBUG_MODE : UIToEngineMessage::DISABLE_DEBUG_MODE;
        m_messageDispatcher->sendToEngine(msg);
    }
}

//==============================================================================
AppController::PerformanceStats AppController::getPerformanceStats() const
{
    return m_performanceStats;
}

void AppController::timerCallback()
{
    updatePerformanceStats();
}

//==============================================================================
void AppController::setupMessageHandlers()
{
    if (!m_messageDispatcher)
        return;
    
    // Process messages from engine to UI
    // This will be called periodically to update UI state
}

void AppController::markProjectDirty()
{
    m_hasUnsavedChanges = true;
}

void AppController::updatePerformanceStats()
{
    // Update local performance stats
    m_performanceStats.eventsProcessed = m_totalEventsProcessed.load();
    
    // Process any messages from the engine
    if (m_messageDispatcher)
    {
        EngineToUIMessage msg;
        while (m_messageDispatcher->processEngineMessage(msg))
        {
            if (msg.type == EngineToUIMessage::CPU_USAGE)
            {
                m_performanceStats.cpuUsage = msg.data.performance.cpu;
            }
            else if (msg.type == EngineToUIMessage::ACTIVE_VOICE_COUNT)
            {
                m_performanceStats.activeVoices = msg.data.voices.count;
            }
            else if (msg.type == EngineToUIMessage::TIMING_DRIFT)
            {
                m_performanceStats.audioLatency = msg.data.timing.latency;
            }
        }
    }
}

} // namespace UI
} // namespace HAM