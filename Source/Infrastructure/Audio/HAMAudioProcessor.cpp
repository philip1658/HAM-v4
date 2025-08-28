/*
  ==============================================================================

    HAMAudioProcessor.cpp
    Main audio processor implementation

  ==============================================================================
*/

#include "HAMAudioProcessor.h"
#include "../../Presentation/Views/MainEditor.h"
#include "../../Domain/Models/Stage.h"
#include "../../Domain/Models/Track.h"
#include "../Plugins/PluginWindowManager.h"

namespace HAM
{

//==============================================================================
HAMAudioProcessor::HAMAudioProcessor()
    : AudioProcessor(BusesProperties()
                    .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    // Initialize core components
    m_masterClock = std::make_unique<MasterClock>();
    m_transport = std::make_unique<Transport>(*m_masterClock);
    m_voiceManager = std::make_unique<VoiceManager>();
    m_sequencerEngine = std::make_unique<SequencerEngine>();
    m_midiRouter = std::make_unique<MidiRouter>();
    m_channelManager = std::make_unique<ChannelManager>();
    
    // Initialize message system
    m_messageQueue = std::make_unique<LockFreeMessageQueue<UIToEngineMessage, 2048>>();
    m_messageDispatcher = std::make_unique<MessageDispatcher>();

    // Register ALL UI message handlers
    // Transport messages
    m_messageDispatcher->registerUIHandler(UIToEngineMessage::TRANSPORT_PLAY, [this](const UIToEngineMessage& msg) {
        juce::Logger::writeToLog("HAMAudioProcessor: Handler called for TRANSPORT_PLAY");
        processUIMessage(msg);
    });
    m_messageDispatcher->registerUIHandler(UIToEngineMessage::TRANSPORT_STOP, [this](const UIToEngineMessage& msg) {
        processUIMessage(msg);
    });
    m_messageDispatcher->registerUIHandler(UIToEngineMessage::TRANSPORT_PAUSE, [this](const UIToEngineMessage& msg) {
        processUIMessage(msg);
    });
    m_messageDispatcher->registerUIHandler(UIToEngineMessage::TRANSPORT_PANIC, [this](const UIToEngineMessage& msg) {
        processUIMessage(msg);
    });
    
    // Parameter messages
    m_messageDispatcher->registerUIHandler(UIToEngineMessage::SET_BPM, [this](const UIToEngineMessage& msg) {
        processUIMessage(msg);
    });
    
    // Debug mode handlers
    m_messageDispatcher->registerUIHandler(UIToEngineMessage::ENABLE_DEBUG_MODE, [this](const UIToEngineMessage&) {
        if (m_midiRouter) m_midiRouter->setDebugChannelEnabled(true);
    });
    m_messageDispatcher->registerUIHandler(UIToEngineMessage::DISABLE_DEBUG_MODE, [this](const UIToEngineMessage&) {
        if (m_midiRouter) m_midiRouter->setDebugChannelEnabled(false);
    });
    
    // Create default pattern with proper configuration
    m_currentPattern = std::make_shared<Pattern>();
    m_currentPattern->addTrack(); // Add one default track
    
    // Configure the default track properly for immediate playback
    if (auto* track = m_currentPattern->getTrack(0))
    {
        track->setLength(8);  // Use all 8 stages
        track->setDivision(4); // Quarter note division
        track->setVoiceMode(VoiceMode::MONO); // Start with mono mode
        track->setMidiChannel(1); // Use MIDI channel 1
        
        // Create a simple C major ascending pattern
        for (int i = 0; i < 8; ++i)
        {
            Stage& stage = track->getStage(i);
            stage.setPitch(60 + (i * 2));  // C major scale: C, D, E, F, G, A, B, C
            stage.setVelocity(100);  // Good velocity for hearing notes
            stage.setGate(0.9f);  // 90% gate length for clear notes
            stage.setPulseCount(1);  // One pulse per stage for now
            stage.setGateType(GateType::MULTIPLE);  // Standard gate type
        }
        
        DBG("Default pattern created with " << track->getLength() << " stages");
    }
    
    // Initialize per-track processors for default tracks
    for (int i = 0; i < 1; ++i)  // Start with 1 track
    {
        m_pitchEngines.push_back(std::make_unique<PitchEngine>());
        m_accumulatorEngines.push_back(std::make_unique<AccumulatorEngine>());
    }
    
    // Setup clock listener
    m_masterClock->addListener(this);
    m_masterClock->addListener(m_sequencerEngine.get());
    
    // Initialize external MIDI output (IAC Driver Bus 1 for monitoring)
    auto midiOutputDevices = juce::MidiOutput::getAvailableDevices();
    for (const auto& device : midiOutputDevices)
    {
        if (device.name.contains("IAC") && device.name.contains("Bus 1"))
        {
            m_externalMidiOutput = juce::MidiOutput::openDevice(device.identifier);
            if (m_externalMidiOutput && m_midiRouter)
            {
                m_midiRouter->setExternalMidiOutput(m_externalMidiOutput.get());
                DBG("External MIDI output initialized: " << device.name);
            }
            break;
        }
    }
    
    // Initialize sequencer with pattern
    m_sequencerEngine->setPattern(m_currentPattern);
    m_sequencerEngine->setVoiceManager(m_voiceManager.get());
    
    // Initialize plugin graph
    m_pluginGraph = std::make_unique<juce::AudioProcessorGraph>();
    
    // Create I/O nodes - addNode returns Node::Ptr directly
    m_audioInputNode = m_pluginGraph->addNode(
        std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
            juce::AudioProcessorGraph::AudioGraphIOProcessor::audioInputNode));
    
    m_audioOutputNode = m_pluginGraph->addNode(
        std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
            juce::AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode));
    
    m_midiInputNode = m_pluginGraph->addNode(
        std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
            juce::AudioProcessorGraph::AudioGraphIOProcessor::midiInputNode));
    
    m_midiOutputNode = m_pluginGraph->addNode(
        std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
            juce::AudioProcessorGraph::AudioGraphIOProcessor::midiOutputNode));
    
    // Initialize track plugin chains (start with 1 track)
    for (int i = 0; i < 1; ++i)
    {
        m_trackPluginChains.push_back(std::make_unique<TrackPluginChain>(i));
    }
    
    // Initialize debug timing analyzer
    #ifdef DEBUG
    m_timingAnalyzer = std::make_unique<MidiTimingAnalyzer>(48000.0, 120.0);
    #endif
}

HAMAudioProcessor::~HAMAudioProcessor()
{
    // Close all plugin windows before destroying the processor
    // This prevents crashes during static destruction at app exit
    PluginWindowManager::getInstance().closeAllWindows();
    
    m_masterClock->removeListener(this);
    m_masterClock->removeListener(m_sequencerEngine.get());
}

//==============================================================================
void HAMAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    m_currentSampleRate = sampleRate;
    m_currentBlockSize = samplesPerBlock;
    
    // Prepare plugin graph FIRST (before engines)
    if (m_pluginGraph)
    {
        m_pluginGraph->setPlayConfigDetails(getTotalNumInputChannels(),
                                           getTotalNumOutputChannels(),
                                           sampleRate, samplesPerBlock);
        m_pluginGraph->prepareToPlay(sampleRate, samplesPerBlock);
    }
    
    // Prepare all engines
    m_masterClock->setSampleRate(sampleRate);
    
    // Reset engines
    m_voiceManager->panic();  // Clear all voices
    m_sequencerEngine->reset();
    
    // Clear MIDI buffers
    m_incomingMidi.clear();
    m_outgoingMidi.clear();
    
    // Pre-allocate MIDI event buffer (1024 events should be plenty)
    m_midiEventBuffer.clear();
    m_midiEventBuffer.reserve(1024);
    
    // Reset all per-track processors
    for (auto& engine : m_pitchEngines)
    {
        engine->reset();
    }
    
    for (auto& engine : m_accumulatorEngines)
        engine->reset();
}

void HAMAudioProcessor::releaseResources()
{
    // Stop playback
    m_transport->stop();
    m_masterClock->stop();
    
    // Release plugin graph resources
    if (m_pluginGraph)
    {
        m_pluginGraph->releaseResources();
    }
    
    // Clear buffers
    m_incomingMidi.clear();
    m_outgoingMidi.clear();
}

//==============================================================================
bool HAMAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // We only support stereo output
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    
    return true;
}

//==============================================================================
void HAMAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, 
                                    juce::MidiBuffer& midiMessages)
{
    // Check if already processing (atomic flag for re-entrancy protection)
    if (m_isProcessing.exchange(true))
    {
        // Already processing, clear buffers and exit early
        buffer.clear();
        midiMessages.clear();
        return;
    }
    
    // Debug: Log every 1000th call to verify processing is happening (reduced frequency)
    static int processCounter = 0;
    if (++processCounter % 1000 == 0)
    {
        DBG("HAMAudioProcessor::processBlock called " << processCounter << " times, playing: " 
            << (m_transport->isPlaying() ? "YES" : "NO") 
            << ", BPM: " << m_masterClock->getBPM()
            << ", Position: " << m_masterClock->getCurrentBar() << ":" 
            << m_masterClock->getCurrentBeat() << ":" 
            << m_masterClock->getCurrentPulse());
    }
    
    // Clear output buffer (we don't generate audio, only MIDI)
    buffer.clear();
    
    // Process UI messages (lock-free)
    processUIMessages();
    
    // Get number of samples in this block
    const int numSamples = buffer.getNumSamples();
    
    // Process transport and clock
    if (m_transport->isPlaying())
    {
        // Process clock for this block
        m_masterClock->processBlock(m_currentSampleRate, numSamples);
        
        // Process sequencer events
        m_sequencerEngine->processBlock(m_currentSampleRate, numSamples);
        
        // ============================================================================
        // PER-TRACK MIDI ROUTING ARCHITECTURE
        // 
        // 1. Each track has its own FIFO queue in SequencerEngine
        // 2. Events are separated by track index (0-7), NOT by MIDI channel
        // 3. Each track's events go ONLY to its corresponding plugin
        // 4. All events are converted to Channel 1 for the plugin (plugins expect Ch 1)
        // 
        // This achieves true track isolation while maintaining plugin compatibility
        // ============================================================================
        
        // Clear the main MIDI buffer (we won't use it for plugin routing anymore)
        midiMessages.clear();
        
        // Process each track's MIDI separately
        for (size_t trackIndex = 0; trackIndex < m_trackPluginChains.size(); ++trackIndex)
        {
            // Get MIDI events for THIS track only
            std::vector<SequencerEngine::MidiEvent> trackMidiEvents;
            m_sequencerEngine->getAndClearTrackMidiEvents(static_cast<int>(trackIndex), trackMidiEvents);
            
            if (!trackMidiEvents.empty() && m_trackPluginChains[trackIndex])
            {
                // Create a MIDI buffer for this track
                juce::MidiBuffer trackMidiBuffer;
                
                for (const auto& event : trackMidiEvents)
                {
                    if (event.sampleOffset < numSamples)
                    {
                        // Force Channel 1 for plugins (most plugins only listen to Ch 1)
                        juce::MidiMessage msg = event.message;
                        if (msg.isNoteOn())
                        {
                            msg = juce::MidiMessage::noteOn(1, msg.getNoteNumber(), msg.getVelocity());
                        }
                        else if (msg.isNoteOff())
                        {
                            msg = juce::MidiMessage::noteOff(1, msg.getNoteNumber(), msg.getVelocity());
                        }
                        else if (msg.isController())
                        {
                            msg = juce::MidiMessage::controllerEvent(1, msg.getControllerNumber(), msg.getControllerValue());
                        }
                        else if (msg.isPitchWheel())
                        {
                            msg = juce::MidiMessage::pitchWheel(1, msg.getPitchWheelValue());
                        }
                        else if (msg.isChannelPressure())
                        {
                            msg = juce::MidiMessage::channelPressureChange(1, msg.getChannelPressureValue());
                        }
                        trackMidiBuffer.addEvent(msg, event.sampleOffset);
                        
                        // Debug: Capture MIDI events for timing analysis
                        #ifdef DEBUG
                        if (m_timingAnalyzer)
                        {
                            m_timingAnalyzer->addEvent(event.message, event.sampleOffset, 
                                                      event.trackIndex, event.stageIndex, 0);
                        }
                        #endif
                    }
                }
                
                // Process THIS track's plugin with THIS track's MIDI
                auto& chain = m_trackPluginChains[trackIndex];
                if (chain->instrumentNode && chain->instrumentNode->getProcessor())
                {
                    // Process the plugin directly with its track's MIDI
                    chain->instrumentNode->getProcessor()->processBlock(buffer, trackMidiBuffer);
                }
            }
        }
        
        // Get any remaining global MIDI events (for backward compatibility)
        m_midiEventBuffer.clear();
        m_sequencerEngine->getAndClearMidiEvents(m_midiEventBuffer);
        
        // Add global events to main buffer (if any)
        for (const auto& event : m_midiEventBuffer)
        {
            if (event.sampleOffset < numSamples)
            {
                midiMessages.addEvent(event.message, event.sampleOffset);
            }
        }
        
        // Debug: Advance timing analyzer
        #ifdef DEBUG
        if (m_timingAnalyzer)
        {
            m_timingAnalyzer->advanceTime(numSamples);
            
            m_timingAnalysisCounter += numSamples;
            if (m_timingAnalysisCounter >= m_currentSampleRate * 4)
            {
                int division = 1;
                if (m_currentPattern && m_currentPattern->getTrack(0))
                {
                    division = m_currentPattern->getTrack(0)->getDivision();
                }
                
                m_timingAnalyzer->printDetailedReport(division);
                m_timingAnalyzer->reset();
                m_timingAnalysisCounter = 0;
            }
        }
        #endif
    }
    
    // Copy incoming MIDI for processing
    m_incomingMidi = midiMessages;
    
    // Process the audio graph for effects and mixing (but NOT for MIDI routing)
    if (m_pluginGraph)
    {
        // Process effects chains and audio mixing only
        // MIDI has already been processed per-track above
        juce::MidiBuffer emptyMidi;
        m_pluginGraph->processBlock(buffer, emptyMidi);
    }
    
    // Performance monitoring
    static juce::PerformanceCounter perfCounter("HAM processBlock", 100);
    perfCounter.start();
    
    // Calculate actual CPU usage
    auto startTime = juce::Time::getHighResolutionTicks();
    
    // Main processing happens above...
    
    auto endTime = juce::Time::getHighResolutionTicks();
    auto elapsedSeconds = juce::Time::highResolutionTicksToSeconds(endTime - startTime);
    auto bufferDuration = numSamples / m_currentSampleRate;
    
    // Update CPU usage (smoothed)
    float instantCpu = static_cast<float>(elapsedSeconds / bufferDuration) * 100.0f;
    float currentCpu = m_cpuUsage.load();
    float smoothedCpu = currentCpu * 0.9f + instantCpu * 0.1f;  // Simple smoothing
    m_cpuUsage.store(smoothedCpu);
    
    perfCounter.stop();
    
    // Clear processing flag
    m_isProcessing.store(false);
}

//==============================================================================
void HAMAudioProcessor::processUIMessages()
{
    // Process messages from MessageDispatcher's queue
    const int maxMessages = 32;
    
    // Use the MessageDispatcher's built-in processing
    if (m_messageDispatcher)
    {
        m_messageDispatcher->processUIMessages(maxMessages);
    }
}

void HAMAudioProcessor::processUIMessage(const UIToEngineMessage& msg)
{
    switch (msg.type)
    {
        // Transport Control
        case UIToEngineMessage::TRANSPORT_PLAY:
            juce::Logger::writeToLog("HAMAudioProcessor: Received TRANSPORT_PLAY message");
            play();
            juce::Logger::writeToLog("HAMAudioProcessor: After play() - isPlaying: " + juce::String(m_transport->isPlaying() ? "true" : "false"));
            break;
            
        case UIToEngineMessage::TRANSPORT_STOP:
            stop();
            break;
            
        case UIToEngineMessage::TRANSPORT_PAUSE:
            pause();
            break;
            
        case UIToEngineMessage::TRANSPORT_PANIC:
            m_voiceManager->panic();
            break;
            
        // Parameter Changes
        case UIToEngineMessage::SET_BPM:
            setBPM(msg.data.floatParam.value);
            break;
            
        case UIToEngineMessage::SET_SWING:
            if (m_masterClock)
                // TODO: Swing should be applied per-track, not globally
                // For now, store swing value but don't apply it
                DBG("Swing set to: " << msg.data.floatParam.value);
            break;
            
        case UIToEngineMessage::SET_MASTER_VOLUME:
            // Store master volume for use in processBlock
            m_masterVolume.store(msg.data.floatParam.value);
            break;
            
        case UIToEngineMessage::SET_PATTERN_LENGTH:
            if (m_currentPattern)
            {
                // TODO: Pattern length should be managed differently
                // For now, log the requested length change
                DBG("Pattern length set to: " << msg.data.intParam.value);
                // Notify sequencer of pattern change
                if (m_sequencerEngine)
                    m_sequencerEngine->setPattern(m_currentPattern);
            }
            break;
            
        // Pattern Changes
        case UIToEngineMessage::LOAD_PATTERN:
            loadPattern(msg.data.patternParam.patternId);
            break;
            
        case UIToEngineMessage::CLEAR_PATTERN:
            // Reset the current pattern instead of clearing (Pattern has no clear method)
            if (m_currentPattern)
            {
                m_currentPattern = std::make_shared<Pattern>();
                m_currentPattern->addTrack();
                m_sequencerEngine->setPattern(m_currentPattern);
            }
            break;
            
        // Track Control
        case UIToEngineMessage::SET_TRACK_MUTE:
            if (auto* track = getTrack(msg.data.trackParam.trackIndex))
                track->setMuted(msg.data.trackParam.value != 0);
            break;
            
        case UIToEngineMessage::SET_TRACK_SOLO:
            if (auto* track = getTrack(msg.data.trackParam.trackIndex))
                track->setSolo(msg.data.trackParam.value != 0);
            break;
            
        case UIToEngineMessage::SET_TRACK_VOICE_MODE:
            if (auto* track = getTrack(msg.data.trackParam.trackIndex))
                track->setVoiceMode(static_cast<VoiceMode>(msg.data.trackParam.value));
            break;
            
        case UIToEngineMessage::SET_TRACK_DIVISION:
            if (auto* track = getTrack(msg.data.trackParam.trackIndex))
                track->setDivision(msg.data.trackParam.value);
            break;
            
        case UIToEngineMessage::SET_TRACK_CHANNEL:
            if (auto* track = getTrack(msg.data.trackParam.trackIndex))
                track->setMidiChannel(msg.data.trackParam.value);
            break;
            
        case UIToEngineMessage::ADD_TRACK:
            addTrack();
            break;
            
        case UIToEngineMessage::REMOVE_TRACK:
            removeTrack(msg.data.intParam.value);
            break;
            
        // Stage Parameters
        case UIToEngineMessage::SET_STAGE_PITCH:
            if (auto* track = getTrack(msg.data.stageParam.trackIndex))
            {
                if (msg.data.stageParam.stageIndex >= 0 && msg.data.stageParam.stageIndex < 8)
                {
                    Stage& stage = track->getStage(msg.data.stageParam.stageIndex);
                    stage.setPitch(static_cast<int>(msg.data.stageParam.value));
                }
            }
            break;
            
        case UIToEngineMessage::SET_STAGE_VELOCITY:
            if (auto* track = getTrack(msg.data.stageParam.trackIndex))
            {
                if (msg.data.stageParam.stageIndex >= 0 && msg.data.stageParam.stageIndex < 8)
                {
                    Stage& stage = track->getStage(msg.data.stageParam.stageIndex);
                    stage.setVelocity(static_cast<int>(msg.data.stageParam.value));
                }
            }
            break;
            
        case UIToEngineMessage::SET_STAGE_GATE:
            if (auto* track = getTrack(msg.data.stageParam.trackIndex))
            {
                if (msg.data.stageParam.stageIndex >= 0 && msg.data.stageParam.stageIndex < 8)
                {
                    Stage& stage = track->getStage(msg.data.stageParam.stageIndex);
                    stage.setGate(msg.data.stageParam.value);
                }
            }
            break;
            
        case UIToEngineMessage::SET_STAGE_PULSE_COUNT:
            if (auto* track = getTrack(msg.data.stageParam.trackIndex))
            {
                if (msg.data.stageParam.stageIndex >= 0 && msg.data.stageParam.stageIndex < 8)
                {
                    Stage& stage = track->getStage(msg.data.stageParam.stageIndex);
                    stage.setPulseCount(static_cast<int>(msg.data.stageParam.value));
                }
            }
            break;
            
        case UIToEngineMessage::SET_STAGE_RATCHETS:
            if (auto* track = getTrack(msg.data.stageParam.trackIndex))
            {
                if (msg.data.stageParam.stageIndex >= 0 && msg.data.stageParam.stageIndex < 8)
                {
                    Stage& stage = track->getStage(msg.data.stageParam.stageIndex);
                    // Copy ratchet pattern from extraData
                    for (size_t i = 0; i < 8 && i < msg.extraData.size(); ++i)
                        stage.setRatchetCount(static_cast<int>(i), static_cast<int>(msg.extraData[i]));
                }
            }
            break;
            
        // Engine Configuration
        case UIToEngineMessage::SET_SCALE:
            // Set scale for the pitch engine of the specified track
            if (msg.data.trackParam.trackIndex >= 0 && 
                msg.data.trackParam.trackIndex < static_cast<int>(m_pitchEngines.size()))
            {
                if (auto& pitchEngine = m_pitchEngines[msg.data.trackParam.trackIndex])
                {
                    // TODO: Scale setting should use Scale object, not int value
                    DBG("Scale set for track " << msg.data.trackParam.trackIndex << " to value: " << msg.data.trackParam.value);
                }
            }
            break;
            
        case UIToEngineMessage::SET_ACCUMULATOR_MODE:
            // Set accumulator mode for the specified track
            if (msg.data.trackParam.trackIndex >= 0 && 
                msg.data.trackParam.trackIndex < static_cast<int>(m_accumulatorEngines.size()))
            {
                if (auto& accEngine = m_accumulatorEngines[msg.data.trackParam.trackIndex])
                {
                    accEngine->setMode(static_cast<AccumulatorEngine::AccumulatorMode>(msg.data.trackParam.value));
                }
            }
            break;
            
        case UIToEngineMessage::SET_GATE_TYPE:
            // Set gate type for the specified track
            // This will be used by the gate processor when implemented
            if (auto* track = getTrack(msg.data.trackParam.trackIndex))
            {
                // TODO: GateType should be set per-stage, not per-track
                // For now, set gate type on all stages of this track
                for (int i = 0; i < 8; ++i) {
                    track->getStage(i).setGateType(static_cast<GateType>(msg.data.trackParam.value));
                }
            }
            break;
            
        case UIToEngineMessage::SET_VOICE_STEALING_MODE:
            if (m_voiceManager)
                m_voiceManager->setStealingMode(static_cast<VoiceManager::StealingMode>(msg.data.intParam.value));
            break;
            
        // Morphing Control
        case UIToEngineMessage::START_MORPH:
            // Start morphing between two pattern snapshots
            // Future implementation will use SnapshotManager
            DBG("Morph started between slots " << msg.data.morphParam.sourceSlot 
                << " and " << msg.data.morphParam.targetSlot);
            break;
            
        case UIToEngineMessage::SET_MORPH_POSITION:
            // Set morph position (0.0 = source, 1.0 = target)
            // Future implementation will interpolate pattern parameters
            DBG("Morph position set to " << msg.data.floatParam.value);
            break;
            
        case UIToEngineMessage::SAVE_SNAPSHOT:
            // Save current pattern state to snapshot slot
            // Future implementation will use SnapshotManager
            DBG("Pattern snapshot saved to slot " << msg.data.snapshotParam.snapshotSlot);
            break;
            
        case UIToEngineMessage::LOAD_SNAPSHOT:
            // Load pattern state from snapshot slot
            // Future implementation will use SnapshotManager
            DBG("Pattern snapshot loaded from slot " << msg.data.snapshotParam.snapshotSlot);
            break;
            
        // System Control
        case UIToEngineMessage::REQUEST_STATE_DUMP:
            // Send comprehensive state information back to UI
            {
                EngineToUIMessage stateMsg;
                stateMsg.type = EngineToUIMessage::TRANSPORT_STATUS;
                stateMsg.data.transport.playing = m_transport->isPlaying();
                stateMsg.data.transport.recording = false;
                stateMsg.data.transport.bpm = m_masterClock->getBPM();
                processEngineMessage(stateMsg);
                
                // Send voice count
                if (m_voiceManager)
                {
                    EngineToUIMessage voiceMsg;
                    voiceMsg.type = EngineToUIMessage::ACTIVE_VOICE_COUNT;
                    voiceMsg.data.voices.count = m_voiceManager->getActiveVoiceCount();
                    voiceMsg.data.voices.stolen = 0; // Will be updated when stats are available
                    voiceMsg.data.voices.peak = m_voiceManager->getActiveVoiceCount();
                    processEngineMessage(voiceMsg);
                }
            }
            break;
            
        case UIToEngineMessage::RESET_STATISTICS:
            // Reset performance statistics
            m_cpuUsage.store(0.0f);
            m_droppedMessages.store(0);
            if (m_voiceManager)
                m_voiceManager->resetStatistics();
            DBG("Statistics reset");
            break;
            
        case UIToEngineMessage::ENABLE_DEBUG_MODE:
            if (m_transport)
            {
                m_transport->setDebugMode(true);
                DBG("Debug mode enabled");
                juce::Logger::writeToLog("Transport debug mode enabled - detailed timing logs will be generated");
            }
            break;
            
        case UIToEngineMessage::DISABLE_DEBUG_MODE:
            if (m_transport)
            {
                m_transport->setDebugMode(false);
                DBG("Debug mode disabled");
                juce::Logger::writeToLog("Transport debug mode disabled");
            }
            break;
            
        default:
            // Unknown message type
            break;
    }
}

void HAMAudioProcessor::processEngineMessage(const EngineToUIMessage& msg)
{
    // Forward engine messages to the UI through the message dispatcher
    // These are typically status updates from the audio thread
    if (m_messageDispatcher)
    {
        // Send to UI with appropriate priority based on message type
        switch (msg.type)
        {
            case EngineToUIMessage::TRANSPORT_STATUS:
            case EngineToUIMessage::ERROR_CPU_OVERLOAD:
            case EngineToUIMessage::BUFFER_UNDERRUN:
                // Critical messages get sent immediately
                m_messageDispatcher->sendStatusToUI(msg);
                break;
                
            case EngineToUIMessage::PLAYHEAD_POSITION:
            case EngineToUIMessage::CURRENT_STAGE:
            case EngineToUIMessage::ACTIVE_VOICE_COUNT:
            case EngineToUIMessage::MIDI_NOTE_ON:
            case EngineToUIMessage::MIDI_NOTE_OFF:
            case EngineToUIMessage::CPU_USAGE:
            case EngineToUIMessage::TIMING_DRIFT:
                // Regular updates use normal priority
                m_messageDispatcher->sendToUI(msg);
                break;
                
            case EngineToUIMessage::DEBUG_TIMING_INFO:
            case EngineToUIMessage::DEBUG_QUEUE_STATS:
                // Debug messages only sent when debug mode is active
                if (m_transport && m_transport->isDebugMode())
                {
                    m_messageDispatcher->sendToUI(msg);
                }
                break;
                
            default:
                // Forward any other messages with normal priority
                m_messageDispatcher->sendToUI(msg);
                break;
        }
    }
}

//==============================================================================
void HAMAudioProcessor::play()
{
    // Use Logger for Console output
    juce::Logger::writeToLog("=== HAMAudioProcessor::play() - Starting playback ===");
    juce::Logger::writeToLog("Initial State:");
    juce::Logger::writeToLog(juce::String("  - Transport playing: ") + (m_transport->isPlaying() ? "YES" : "NO"));
    juce::Logger::writeToLog(juce::String("  - Clock running: ") + (m_masterClock->isRunning() ? "YES" : "NO"));
    juce::Logger::writeToLog(juce::String("  - Clock BPM: ") + juce::String(m_masterClock->getBPM()));
    juce::Logger::writeToLog(juce::String("  - Clock Position: ") + juce::String(m_masterClock->getCurrentBar()) + ":" 
        + juce::String(m_masterClock->getCurrentBeat()) + ":" + juce::String(m_masterClock->getCurrentPulse()));
    
    DBG("=== HAMAudioProcessor::play() - Starting playback ===");
    DBG("Initial State:");
    DBG(juce::String("  - Transport playing: ") + (m_transport->isPlaying() ? "YES" : "NO"));
    DBG(juce::String("  - Clock running: ") + (m_masterClock->isRunning() ? "YES" : "NO"));
    DBG(juce::String("  - Clock BPM: ") + juce::String(m_masterClock->getBPM()));
    DBG(juce::String("  - Clock Position: ") + juce::String(m_masterClock->getCurrentBar()) + ":" 
        + juce::String(m_masterClock->getCurrentBeat()) + ":" + juce::String(m_masterClock->getCurrentPulse()));
    
    // Don't force stop first - just call play which handles state transitions properly
    m_transport->play();
    
    // Only start sequencer if transport is actually playing
    if (m_transport->isPlaying())
    {
        m_sequencerEngine->start();
        juce::Logger::writeToLog("HAMAudioProcessor::play() - All components started successfully");
        DBG("HAMAudioProcessor::play() - All components started successfully");
    }
    else
    {
        juce::Logger::writeToLog("HAMAudioProcessor::play() - WARNING: Transport failed to start!");
        DBG("HAMAudioProcessor::play() - WARNING: Transport failed to start!");
        // Try to recover by stopping then playing
        m_transport->stop(false);  // Stop without return to zero
        
        // Add a small delay to ensure clean state transition
        juce::Thread::sleep(10);
        
        m_transport->play();        // Try again
        
        if (m_transport->isPlaying())
        {
            m_sequencerEngine->start();
            juce::Logger::writeToLog("HAMAudioProcessor::play() - Started on second attempt");
            DBG("HAMAudioProcessor::play() - Started on second attempt");
        }
        else
        {
            juce::Logger::writeToLog("HAMAudioProcessor::play() - ERROR: Failed to start transport!");
            DBG("HAMAudioProcessor::play() - ERROR: Failed to start transport!");
            // One more attempt - completely reset everything
            juce::Logger::writeToLog("HAMAudioProcessor::play() - Attempting full reset...");
            DBG("HAMAudioProcessor::play() - Attempting full reset...");
            m_masterClock->stop();
            m_masterClock->reset();
            m_transport->stop(true);  // Return to zero
            juce::Thread::sleep(10);
            
            m_transport->play();
            if (m_transport->isPlaying())
            {
                m_sequencerEngine->start();
                juce::Logger::writeToLog("HAMAudioProcessor::play() - Started after full reset");
                DBG("HAMAudioProcessor::play() - Started after full reset");
            }
        }
    }
    
    juce::Logger::writeToLog("Final State:");
    juce::Logger::writeToLog(juce::String("  - Transport playing: ") + (m_transport->isPlaying() ? "YES" : "NO"));
    juce::Logger::writeToLog(juce::String("  - Clock running: ") + (m_masterClock->isRunning() ? "YES" : "NO"));
    juce::Logger::writeToLog(juce::String("  - Sequencer state: ") + juce::String(static_cast<int>(m_sequencerEngine->getState())));
    juce::Logger::writeToLog("=================================");
    
    DBG("Final State:");
    DBG(juce::String("  - Transport playing: ") + (m_transport->isPlaying() ? "YES" : "NO"));
    DBG(juce::String("  - Clock running: ") + (m_masterClock->isRunning() ? "YES" : "NO"));
    DBG(juce::String("  - Sequencer state: ") + juce::String(static_cast<int>(m_sequencerEngine->getState())));
    DBG("=================================");
}

void HAMAudioProcessor::stop()
{
    m_transport->stop();
    m_masterClock->stop();
    m_sequencerEngine->stop();
    m_voiceManager->allNotesOff();
}

void HAMAudioProcessor::pause()
{
    m_transport->pause();
    m_masterClock->stop();
}

void HAMAudioProcessor::setBPM(float bpm)
{
    m_masterClock->setBPM(bpm);
}

//==============================================================================
void HAMAudioProcessor::addProcessorsForTrack(int trackIndex)
{
    // Add pitch engine for the new track
    if (trackIndex >= m_pitchEngines.size())
    {
        m_pitchEngines.push_back(std::make_unique<PitchEngine>());
    }
    
    // Add accumulator engine for the new track
    if (trackIndex >= m_accumulatorEngines.size())
    {
        m_accumulatorEngines.push_back(std::make_unique<AccumulatorEngine>());
    }
    
    // Add plugin chain for the new track
    if (trackIndex >= m_trackPluginChains.size())
    {
        m_trackPluginChains.push_back(std::make_unique<TrackPluginChain>(trackIndex));
    }
    
    DBG("Added processors for track " << trackIndex);
}

void HAMAudioProcessor::removeProcessorsForTrack(int trackIndex)
{
    // Remove pitch engine
    if (trackIndex < m_pitchEngines.size())
    {
        m_pitchEngines.erase(m_pitchEngines.begin() + trackIndex);
    }
    
    // Remove accumulator engine
    if (trackIndex < m_accumulatorEngines.size())
    {
        m_accumulatorEngines.erase(m_accumulatorEngines.begin() + trackIndex);
    }
    
    // Remove plugin chain
    if (trackIndex < m_trackPluginChains.size())
    {
        m_trackPluginChains.erase(m_trackPluginChains.begin() + trackIndex);
    }
    
    DBG("Removed processors for track " << trackIndex);
}

//==============================================================================
void HAMAudioProcessor::loadPattern(int patternIndex)
{
    // Load pattern from internal storage (will be enhanced with file I/O later)
    // For now, create a demo pattern
    m_currentPattern = std::make_shared<Pattern>();
    
    // Add a default track with some interesting stages
    m_currentPattern->addTrack();
    if (auto* track = m_currentPattern->getTrack(0))
    {
        // Set up a simple 8-stage pattern
        for (int i = 0; i < 8; ++i)
        {
            Stage& stage = track->getStage(i);
            stage.setPitch(60 + (i * 2));  // C major scale
            stage.setVelocity(80 + (i * 5));  // Varying velocity
            stage.setGate(0.8f);  // 80% gate length
            stage.setPulseCount(1);
        }
    }
    
    m_sequencerEngine->setPattern(m_currentPattern);
}

void HAMAudioProcessor::savePattern(int patternIndex)
{
    // Save pattern to internal storage (will be enhanced with file I/O later)
    // For now, we just validate that we have a pattern to save
    if (!m_currentPattern)
        return;
    
    // Pattern saving will be implemented with the preset system
    // This will involve serializing to ValueTree and then to JSON/binary
}

//==============================================================================
Track* HAMAudioProcessor::getTrack(int index)
{
    if (!m_currentPattern)
        return nullptr;
        
    if (index < 0 || index >= m_currentPattern->getTrackCount())
        return nullptr;
        
    return m_currentPattern->getTrack(index);
}

int HAMAudioProcessor::getNumTracks() const
{
    return m_currentPattern ? m_currentPattern->getTrackCount() : 0;
}

void HAMAudioProcessor::addTrack()
{
    if (!m_currentPattern)
        return;
        
    m_currentPattern->addTrack();
    
    // Add processors for new track
    // TODO: Re-enable when TrackGateProcessor is implemented
    // m_gateProcessors.push_back(std::make_unique<TrackGateProcessor>());
    m_pitchEngines.push_back(std::make_unique<PitchEngine>());
    m_accumulatorEngines.push_back(std::make_unique<AccumulatorEngine>());
    
    // Add plugin chain for new track
    m_trackPluginChains.push_back(std::make_unique<TrackPluginChain>(
        static_cast<int>(m_trackPluginChains.size())));
}

//==============================================================================
// Plugin Management

bool HAMAudioProcessor::loadPlugin(int trackIndex, const juce::PluginDescription& desc, bool isInstrument)
{
    if (trackIndex < 0 || trackIndex >= static_cast<int>(m_trackPluginChains.size()))
        return false;
    
    auto& chain = m_trackPluginChains[trackIndex];
    
    // Create plugin instance
    juce::String errorMessage;
    juce::AudioPluginFormatManager formatManager;
    formatManager.addDefaultFormats();
    
    auto pluginInstance = formatManager.createPluginInstance(
        desc, 
        m_currentSampleRate, 
        m_currentBlockSize, 
        errorMessage);
    
    if (!pluginInstance)
    {
        DBG("Failed to load plugin: " << errorMessage);
        return false;
    }
    
    // Add to graph - addNode returns Node::Ptr directly
    auto node = m_pluginGraph->addNode(std::move(pluginInstance));
    
    if (!node)
        return false;
    
    if (isInstrument)
    {
        // Replace existing instrument
        if (chain->instrumentNode)
        {
            m_pluginGraph->removeNode(chain->instrumentNode.get());
        }
        chain->instrumentNode = node;
        
        // DON'T connect MIDI input - we'll inject MIDI directly in processBlock
        // This is the key fix: no shared MIDI routing!
        
        // Only connect instrument audio output
        for (int ch = 0; ch < 2; ++ch)
        {
            m_pluginGraph->addConnection(
                {{node->nodeID, ch},
                 {m_audioOutputNode->nodeID, ch}});
        }
    }
    else
    {
        // Add as effect
        chain->effectNodes.push_back(node);
        
        // Rebuild the entire effect chain connections
        rebuildEffectChain(trackIndex);
    }
    
    return true;
}

bool HAMAudioProcessor::removePlugin(int trackIndex, int pluginIndex)
{
    if (trackIndex < 0 || trackIndex >= static_cast<int>(m_trackPluginChains.size()))
        return false;
    
    auto& chain = m_trackPluginChains[trackIndex];
    
    if (pluginIndex == -1)  // Remove instrument
    {
        if (chain->instrumentNode)
        {
            m_pluginGraph->removeNode(chain->instrumentNode.get());
            chain->instrumentNode = nullptr;
            return true;
        }
    }
    else if (pluginIndex >= 0 && pluginIndex < static_cast<int>(chain->effectNodes.size()))
    {
        m_pluginGraph->removeNode(chain->effectNodes[pluginIndex].get());
        chain->effectNodes.erase(chain->effectNodes.begin() + pluginIndex);
        
        // Rebuild the effect chain after removing the effect
        rebuildEffectChain(trackIndex);
        return true;
    }
    
    return false;
}

void HAMAudioProcessor::showPluginEditor(int trackIndex, int pluginIndex)
{
    DBG("HAMAudioProcessor::showPluginEditor called for track " << trackIndex << ", plugin " << pluginIndex);
    
    if (trackIndex < 0 || trackIndex >= static_cast<int>(m_trackPluginChains.size()))
    {
        DBG("Invalid track index: " << trackIndex);
        return;
    }
    
    auto& chain = m_trackPluginChains[trackIndex];
    if (!chain)
    {
        DBG("No plugin chain for track " << trackIndex);
        return;
    }
    
    juce::AudioProcessorGraph::Node::Ptr node;
    juce::AudioPluginInstance* pluginInstance = nullptr;
    
    if (pluginIndex == -1 && chain->instrumentNode)
    {
        DBG("Attempting to show instrument plugin editor");
        node = chain->instrumentNode;
    }
    else if (pluginIndex >= 0 && pluginIndex < static_cast<int>(chain->effectNodes.size()))
    {
        DBG("Attempting to show effect plugin " << pluginIndex << " editor");
        node = chain->effectNodes[pluginIndex];
    }
    else
    {
        DBG("No plugin found at index " << pluginIndex << " for track " << trackIndex);
        return;
    }
    
    if (node && node->getProcessor())
    {
        DBG("Found node with processor");
        // Cast to AudioPluginInstance to access plugin-specific features
        pluginInstance = dynamic_cast<juce::AudioPluginInstance*>(node->getProcessor());
        
        if (pluginInstance)
        {
            // Use PluginWindowManager for proper window lifecycle management
            juce::String pluginName = pluginInstance->getName();
            DBG("Opening plugin window for: " << pluginName);
            DBG("Plugin has editor: " << (pluginInstance->hasEditor() ? "YES" : "NO"));
            
            bool success = PluginWindowManager::getInstance().openPluginWindow(trackIndex, pluginIndex, 
                                                                pluginInstance, pluginName);
            DBG("Plugin window open result: " << (success ? "SUCCESS" : "FAILED"));
        }
        else
        {
            DBG("Failed to cast processor to AudioPluginInstance");
        }
    }
    else
    {
        DBG("No node or processor found");
    }
}

void HAMAudioProcessor::rebuildEffectChain(int trackIndex)
{
    if (trackIndex < 0 || trackIndex >= static_cast<int>(m_trackPluginChains.size()))
        return;
    
    auto& chain = m_trackPluginChains[trackIndex];
    
    // First, disconnect all existing connections for this track's plugins
    // This is a simplified version - in production you'd track connections more carefully
    
    // Determine the starting point of the chain
    juce::AudioProcessorGraph::NodeID sourceNode;
    bool hasInstrument = chain->instrumentNode != nullptr;
    
    if (hasInstrument)
    {
        sourceNode = chain->instrumentNode->nodeID;
    }
    else
    {
        // No instrument, start from audio input
        sourceNode = m_audioInputNode->nodeID;
    }
    
    // Connect through all effects in sequence
    juce::AudioProcessorGraph::NodeID lastNode = sourceNode;
    
    for (auto& effectNode : chain->effectNodes)
    {
        // Connect previous node to this effect
        for (int ch = 0; ch < 2; ++ch)
        {
            // Remove any existing connection to this effect
            m_pluginGraph->removeConnection(
                {{lastNode, ch},
                 {effectNode->nodeID, ch}});
            
            // Add new connection
            m_pluginGraph->addConnection(
                {{lastNode, ch},
                 {effectNode->nodeID, ch}});
        }
        
        // No MIDI routing for effects - they'll get MIDI from the track if needed
        
        lastNode = effectNode->nodeID;
    }
    
    // Finally connect to audio output
    for (int ch = 0; ch < 2; ++ch)
    {
        // Remove any existing connection from last node to output
        m_pluginGraph->removeConnection(
            {{lastNode, ch},
             {m_audioOutputNode->nodeID, ch}});
        
        // Add new connection to output
        m_pluginGraph->addConnection(
            {{lastNode, ch},
             {m_audioOutputNode->nodeID, ch}});
    }
}

void HAMAudioProcessor::removeTrack(int index)
{
    if (!m_currentPattern)
        return;
        
    if (index < 0 || index >= m_currentPattern->getTrackCount())
        return;
        
    m_currentPattern->removeTrack(index);
    
    // Remove processors for track
    // TODO: Re-enable when TrackGateProcessor is implemented
    // if (index < m_gateProcessors.size())
    //     m_gateProcessors.erase(m_gateProcessors.begin() + index);
        
    if (index < m_pitchEngines.size())
        m_pitchEngines.erase(m_pitchEngines.begin() + index);
        
    if (index < m_accumulatorEngines.size())
        m_accumulatorEngines.erase(m_accumulatorEngines.begin() + index);
}

//==============================================================================
void HAMAudioProcessor::onClockPulse(int pulseNumber)
{
    // Clock pulse received - handled by SequencerEngine
}

void HAMAudioProcessor::onClockStart()
{
    // Clock started - can be used for UI updates
}

void HAMAudioProcessor::onClockStop()
{
    // Clock stopped - can be used for UI updates
}

void HAMAudioProcessor::onClockReset()
{
    // Clock reset - can be used for UI updates
}

void HAMAudioProcessor::onTempoChanged(float newBPM)
{
    // Tempo changed - update any tempo-dependent parameters
}

//==============================================================================
// Performance monitoring

size_t HAMAudioProcessor::getMemoryUsage() const
{
    size_t totalMemory = 0;
    
    // Estimate memory usage of major components
    // Pattern memory
    if (m_currentPattern)
    {
        // Approximate: 8 tracks * 8 stages * sizeof(Stage) + overhead
        totalMemory += 8 * 8 * 256; // Rough estimate per stage
    }
    
    // MIDI buffer memory
    totalMemory += m_midiEventBuffer.capacity() * sizeof(SequencerEngine::MidiEvent);
    
    // Engine memory estimates (these would need actual implementation in each engine)
    totalMemory += 64 * sizeof(float) * 8; // Voice manager estimate (64 voices)
    totalMemory += 1024 * 4; // Accumulator history estimate
    totalMemory += 512 * 4; // Gate processor estimate
    
    // Audio buffer memory (if we were using internal buffers)
    totalMemory += m_currentBlockSize * sizeof(float) * 2; // Stereo buffer estimate
    
    return totalMemory;
}

//==============================================================================
juce::AudioProcessorEditor* HAMAudioProcessor::createEditor()
{
    // Create the main editor UI for the processor
    // MainEditor handles all UI components and communication with the processor
    return new MainEditor(*this);
}

//==============================================================================
void HAMAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // Create a ValueTree to store the state
    juce::ValueTree state("HAMState");
    
    // Store version for backwards compatibility
    state.setProperty("version", "1.0.0", nullptr);
    
    // Store transport settings
    state.setProperty("bpm", m_masterClock->getBPM(), nullptr);
    state.setProperty("isPlaying", m_transport->isPlaying(), nullptr);
    
    // Store pattern data
    if (m_currentPattern)
    {
        juce::ValueTree patternTree("Pattern");
        patternTree.setProperty("trackCount", m_currentPattern->getTrackCount(), nullptr);
        
        // Store each track
        for (int t = 0; t < m_currentPattern->getTrackCount(); ++t)
        {
            if (auto* track = m_currentPattern->getTrack(t))
            {
                juce::ValueTree trackTree("Track");
                trackTree.setProperty("index", t, nullptr);
                trackTree.setProperty("midiChannel", track->getMidiChannel(), nullptr);
                trackTree.setProperty("voiceMode", static_cast<int>(track->getVoiceMode()), nullptr);
                trackTree.setProperty("muted", track->isMuted(), nullptr);
                trackTree.setProperty("solo", track->isSolo(), nullptr);
                // TODO: Add volume and pan when Track supports them
                // trackTree.setProperty("volume", track->getVolume(), nullptr);
                // trackTree.setProperty("pan", track->getPan(), nullptr);
                trackTree.setProperty("division", track->getDivision(), nullptr);
                trackTree.setProperty("length", track->getLength(), nullptr);
                
                // Store stages
                for (int s = 0; s < 8; ++s)
                {
                    const Stage& stage = track->getStage(s);
                    juce::ValueTree stageTree("Stage");
                    stageTree.setProperty("index", s, nullptr);
                    stageTree.setProperty("pitch", stage.getPitch(), nullptr);
                    stageTree.setProperty("velocity", stage.getVelocity(), nullptr);
                    stageTree.setProperty("gate", stage.getGate(), nullptr);
                    stageTree.setProperty("pulseCount", stage.getPulseCount(), nullptr);
                    stageTree.setProperty("gateType", stage.getGateTypeAsInt(), nullptr);
                    
                    // Store ratchets
                    const auto& ratchets = stage.getRatchets();
                    for (int r = 0; r < 8; ++r)
                    {
                        stageTree.setProperty("ratchet" + juce::String(r), ratchets[r], nullptr);
                    }
                    
                    trackTree.addChild(stageTree, -1, nullptr);
                }
                
                patternTree.addChild(trackTree, -1, nullptr);
            }
        }
        
        state.addChild(patternTree, -1, nullptr);
    }
    
    // Store plugin states
    juce::ValueTree pluginsTree("Plugins");
    
    // Store plugin graph state (includes all plugin connections and states)
    if (m_pluginGraph)
    {
        juce::MemoryBlock graphData;
        m_pluginGraph->getStateInformation(graphData);
        
        // Convert to base64 for storage in ValueTree
        pluginsTree.setProperty("graphState", graphData.toBase64Encoding(), nullptr);
    }
    
    // Store per-track plugin information
    for (size_t t = 0; t < m_trackPluginChains.size(); ++t)
    {
        juce::ValueTree trackPluginsTree("TrackPlugins");
        trackPluginsTree.setProperty("trackIndex", static_cast<int>(t), nullptr);
        
        const auto& chain = m_trackPluginChains[t];
        
        // Store instrument plugin info if present
        if (chain->instrumentNode && chain->instrumentNode->getProcessor())
        {
            if (auto* plugin = dynamic_cast<juce::AudioPluginInstance*>(chain->instrumentNode->getProcessor()))
            {
                juce::ValueTree instrumentTree("Instrument");
                
                // Store plugin description
                const auto& desc = plugin->getPluginDescription();
                instrumentTree.setProperty("name", desc.name, nullptr);
                instrumentTree.setProperty("manufacturer", desc.manufacturerName, nullptr);
                instrumentTree.setProperty("fileOrId", desc.fileOrIdentifier, nullptr);
                instrumentTree.setProperty("uniqueId", desc.uniqueId, nullptr);
                
                // Store plugin state
                juce::MemoryBlock pluginData;
                plugin->getStateInformation(pluginData);
                instrumentTree.setProperty("state", pluginData.toBase64Encoding(), nullptr);
                
                trackPluginsTree.addChild(instrumentTree, -1, nullptr);
            }
        }
        
        // Store effect plugins
        juce::ValueTree effectsTree("Effects");
        for (size_t e = 0; e < chain->effectNodes.size(); ++e)
        {
            if (chain->effectNodes[e] && chain->effectNodes[e]->getProcessor())
            {
                if (auto* plugin = dynamic_cast<juce::AudioPluginInstance*>(chain->effectNodes[e]->getProcessor()))
                {
                    juce::ValueTree effectTree("Effect");
                    effectTree.setProperty("index", static_cast<int>(e), nullptr);
                    
                    // Store plugin description
                    const auto& desc = plugin->getPluginDescription();
                    effectTree.setProperty("name", desc.name, nullptr);
                    effectTree.setProperty("manufacturer", desc.manufacturerName, nullptr);
                    effectTree.setProperty("fileOrId", desc.fileOrIdentifier, nullptr);
                    effectTree.setProperty("uniqueId", desc.uniqueId, nullptr);
                    
                    // Store plugin state
                    juce::MemoryBlock pluginData;
                    plugin->getStateInformation(pluginData);
                    effectTree.setProperty("state", pluginData.toBase64Encoding(), nullptr);
                    
                    effectsTree.addChild(effectTree, -1, nullptr);
                }
            }
        }
        
        if (effectsTree.getNumChildren() > 0)
            trackPluginsTree.addChild(effectsTree, -1, nullptr);
        
        pluginsTree.addChild(trackPluginsTree, -1, nullptr);
    }
    
    state.addChild(pluginsTree, -1, nullptr);
    
    // Convert ValueTree to binary
    juce::MemoryOutputStream stream(destData, true);
    state.writeToStream(stream);
}

void HAMAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // Parse the saved state
    juce::ValueTree state = juce::ValueTree::readFromData(data, static_cast<size_t>(sizeInBytes));
    
    if (!state.isValid() || state.getType() != juce::Identifier("HAMState"))
        return;
    
    // Check version for compatibility
    juce::String version = state.getProperty("version", "1.0.0");
    
    // Restore transport settings
    float bpm = state.getProperty("bpm", 120.0f);
    m_masterClock->setBPM(bpm);
    
    // Restore pattern data
    juce::ValueTree patternTree = state.getChildWithName("Pattern");
    if (patternTree.isValid())
    {
        m_currentPattern = std::make_shared<Pattern>();
        int trackCount = patternTree.getProperty("trackCount", 1);
        
        // Restore each track
        for (int t = 0; t < trackCount; ++t)
        {
            m_currentPattern->addTrack();
            juce::ValueTree trackTree = patternTree.getChild(t);
            
            if (trackTree.isValid() && trackTree.getType() == juce::Identifier("Track"))
            {
                if (auto* track = m_currentPattern->getTrack(t))
                {
                    track->setMidiChannel(trackTree.getProperty("midiChannel", 1));
                    track->setVoiceMode(static_cast<VoiceMode>(int(trackTree.getProperty("voiceMode", 0))));
                    track->setMuted(trackTree.getProperty("muted", false));
                    track->setSolo(trackTree.getProperty("solo", false));
                    // TODO: Restore volume and pan when Track supports them
                    // track->setVolume(trackTree.getProperty("volume", 1.0f));
                    // track->setPan(trackTree.getProperty("pan", 0.0f));
                    track->setDivision(trackTree.getProperty("division", 4));
                    track->setLength(trackTree.getProperty("length", 8));
                    
                    // Restore stages
                    for (int s = 0; s < trackTree.getNumChildren(); ++s)
                    {
                        juce::ValueTree stageTree = trackTree.getChild(s);
                        if (stageTree.isValid() && stageTree.getType() == juce::Identifier("Stage"))
                        {
                            int stageIndex = stageTree.getProperty("index", s);
                            if (stageIndex >= 0 && stageIndex < 8)
                            {
                                Stage& stage = track->getStage(stageIndex);
                                stage.setPitch(stageTree.getProperty("pitch", 60));
                                stage.setVelocity(stageTree.getProperty("velocity", 100));
                                stage.setGate(stageTree.getProperty("gate", 0.8f));
                                stage.setPulseCount(stageTree.getProperty("pulseCount", 1));
                                stage.setGateType(int(stageTree.getProperty("gateType", 0)));
                                
                                // Restore ratchets
                                for (int r = 0; r < 8; ++r)
                                {
                                    int ratchetCount = stageTree.getProperty("ratchet" + juce::String(r), 1);
                                    stage.setRatchetCount(r, ratchetCount);
                                }
                            }
                        }
                    }
                }
            }
        }
        
        // Update sequencer with loaded pattern
        m_sequencerEngine->setPattern(m_currentPattern);
    }
    
    // Restore plugin states
    juce::ValueTree pluginsTree = state.getChildWithName("Plugins");
    if (pluginsTree.isValid())
    {
        // Restore plugin graph state first
        juce::String graphStateBase64 = pluginsTree.getProperty("graphState", "");
        if (graphStateBase64.isNotEmpty() && m_pluginGraph)
        {
            juce::MemoryBlock graphData;
            graphData.fromBase64Encoding(graphStateBase64);
            m_pluginGraph->setStateInformation(graphData.getData(), static_cast<int>(graphData.getSize()));
        }
        
        // Note: Individual plugin loading would require re-instantiating plugins
        // This is complex as it requires:
        // 1. Plugin format manager
        // 2. Re-creating plugin instances from descriptions
        // 3. Restoring their states
        // For now, the graph state should handle most of the restoration
        
        // TODO: Implement full plugin restoration if graph state is insufficient
        // This would involve:
        // - Loading plugin descriptions from the saved data
        // - Using loadPlugin() to recreate each plugin
        // - Restoring each plugin's individual state
    }
    
    // Restore playing state if it was playing
    bool wasPlaying = state.getProperty("isPlaying", false);
    if (wasPlaying)
    {
        play();
    }
}

} // namespace HAM