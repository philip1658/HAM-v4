/*
  ==============================================================================

    HAMAudioProcessor.cpp - FIXED VERSION WITH WORKING PLUGIN PROCESSING
    Main audio processor implementation with proper plugin graph processing
    
    CRITICAL FIX: Added m_pluginGraph->processBlock() to actually process plugins!

  ==============================================================================
*/

#include "HAMAudioProcessor.h"
#include "../../Presentation/Views/MainEditor.h"
#include "../Plugins/PluginWindowManager.h"
#include "../Messaging/MessageTypes.h"
#include <chrono>

namespace HAM
{

//==============================================================================
HAMAudioProcessor::HAMAudioProcessor()
    : AudioProcessor(BusesProperties()
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , m_currentSampleRate(44100.0)
    , m_currentBlockSize(512)
    , m_cpuUsage(0.0f)
    , m_droppedMessages(0)
    , m_isProcessing(false)
{
    // Initialize core components
    m_masterClock = std::make_unique<MasterClock>();
    m_transport = std::make_unique<Transport>(*m_masterClock);
    m_voiceManager = std::make_unique<VoiceManager>();
    m_sequencerEngine = std::make_unique<SequencerEngine>();
    m_midiRouter = std::make_unique<MidiRouter>();
    m_channelManager = std::make_unique<ChannelManager>();
    
    // Initialize message dispatcher for UI communication
    m_messageDispatcher = std::make_unique<MessageDispatcher>();
    m_messageQueue = std::make_unique<LockFreeMessageQueue<UIToEngineMessage, 2048>>();
    
    // Register message handlers for transport control
    setupMessageHandlers();
    
    // Initialize default pattern
    m_currentPattern = std::make_unique<Pattern>();
    m_currentPattern->setTrackCount(8);
    
    // Initialize per-track engines (8 tracks by default)
    for (int i = 0; i < 8; ++i)
    {
        // Gate engines are commented out until TrackGateProcessor is implemented
        // m_gateProcessors.push_back(std::make_unique<TrackGateProcessor>());
        m_pitchEngines.push_back(std::make_unique<PitchEngine>());
        m_accumulatorEngines.push_back(std::make_unique<AccumulatorEngine>());
        
        // Initialize track plugin chains
        m_trackPluginChains.push_back(std::make_unique<TrackPluginChain>(i));
    }
    
    // Set pattern in sequencer
    m_sequencerEngine->setPattern(m_currentPattern);
    
    // Pre-allocate MIDI event buffer
    m_midiEventBuffer.reserve(1024);
    
    // Register as clock listener
    m_masterClock->addListener(this);
    
    // Connect engines
    m_sequencerEngine->setMasterClock(m_masterClock.get());
    m_sequencerEngine->setVoiceManager(m_voiceManager.get());
    
    // Initialize plugin format manager
    m_formatManager.addDefaultFormats();
    
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
    
    // Connect default routing (pass-through for now)
    // Audio: Input -> Output
    for (int ch = 0; ch < 2; ++ch)
    {
        m_pluginGraph->addConnection(
            {{m_audioInputNode->nodeID, ch},
             {m_audioOutputNode->nodeID, ch}});
    }
    
    // MIDI: Input -> Output
    m_pluginGraph->addConnection(
        {{m_midiInputNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex},
         {m_midiOutputNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex}});
}

HAMAudioProcessor::~HAMAudioProcessor()
{
    // Close all plugin windows first to avoid crashes during shutdown
    PluginWindowManager::getInstance().closeAllWindows();
    
    // Unregister from clock
    if (m_masterClock)
        m_masterClock->removeListener(this);
    
    // Stop processing
    if (m_transport)
        m_transport->stop();
    
    // Clean up plugin resources
    if (m_pluginGraph)
    {
        m_pluginGraph->clear();
    }
}

//==============================================================================
void HAMAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    m_currentSampleRate = sampleRate;
    m_currentBlockSize = samplesPerBlock;
    
    // Prepare plugin graph
    if (m_pluginGraph)
    {
        m_pluginGraph->setPlayConfigDetails(getTotalNumInputChannels(),
                                           getTotalNumOutputChannels(),
                                           sampleRate, samplesPerBlock);
        m_pluginGraph->prepareToPlay(sampleRate, samplesPerBlock);
    }
    
    // Prepare engines
    m_masterClock->setSampleRate(sampleRate);
    // SequencerEngine doesn't have prepareToPlay, it uses processBlock
    // m_sequencerEngine->prepareToPlay(sampleRate, samplesPerBlock);
    
    // VoiceManager doesn't have setSampleRate method
    // m_voiceManager->setSampleRate(sampleRate);
    
    // Clear message buffers
    m_incomingMidi.clear();
    m_outgoingMidi.clear();
    
    // Reset engines
    // Gate engines are not implemented yet
    // for (auto& engine : m_gateProcessors)
    // {
    //     engine->reset();
    // }
    
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
    // Debug: Log first few calls to verify processBlock is being called
    static int processCallCount = 0;
    if (processCallCount < 10)
    {
        DBG("HAMAudioProcessor::processBlock() - Call #" + juce::String(++processCallCount));
        DBG("  - Transport is playing: " + juce::String(m_transport ? (m_transport->isPlaying() ? "YES" : "NO") : "NO TRANSPORT"));
        DBG("  - Sample rate: " + juce::String(m_currentSampleRate));
        DBG("  - Block size: " + juce::String(buffer.getNumSamples()));
    }
    
    // Check if already processing (atomic flag for re-entrancy protection)
    if (m_isProcessing.exchange(true))
    {
        // Already processing, clear buffers and exit early
        buffer.clear();
        midiMessages.clear();
        return;
    }
    
    // Performance monitoring
    juce::PerformanceCounter perfCounter("processBlock", 1000);
    perfCounter.start();
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Process UI messages (lock-free)
    processUIMessages();
    
    // Get number of samples in this block
    const int numSamples = buffer.getNumSamples();
    
    // Clear the MIDI buffer to prepare for new events
    midiMessages.clear();
    
    // Process transport and clock
    if (m_transport->isPlaying())
    {
        // Process clock for this block
        m_masterClock->processBlock(m_currentSampleRate, numSamples);
        
        // Process sequencer events
        m_sequencerEngine->processBlock(m_currentSampleRate, numSamples);
        
        // Get MIDI events from sequencer using pre-allocated buffer
        m_midiEventBuffer.clear();
        m_sequencerEngine->getAndClearMidiEvents(m_midiEventBuffer);
        
        // Convert to JUCE MidiBuffer with sample-accurate timing
        for (const auto& event : m_midiEventBuffer)
        {
            // The event contains a juce::MidiMessage and sample offset
            int samplePosition = event.sampleOffset;
            
            // Ensure sample position is within block bounds
            if (samplePosition >= 0 && samplePosition < numSamples)
            {
                // Add the MIDI message directly
                midiMessages.addEvent(event.message, samplePosition);
            }
        }
        
        // Route MIDI through MidiRouter for filtering/processing
        if (m_midiRouter)
        {
            m_midiRouter->processBlock(midiMessages, numSamples);
        }
    }
    
    // ============================================================================
    // CRITICAL FIX: Process plugins with the plugin graph!
    // The midiMessages buffer contains MIDI from the sequencer that must be
    // passed to the plugin graph for routing to instruments
    // ============================================================================
    if (m_pluginGraph)
    {
        // The plugin graph will:
        // 1. Route MIDI to loaded instrument plugins via MIDI input node
        // 2. Process audio through the plugin chain
        // 3. Mix plugin outputs to the main audio buffer
        //
        // IMPORTANT: The midiMessages buffer is automatically injected into
        // the MIDI input node when we call processBlock()
        m_pluginGraph->processBlock(buffer, midiMessages);
    }
    else
    {
        // Fallback: If no plugin graph, just clear audio
        buffer.clear();
    }
    
    // Apply master volume/pan if needed (future feature)
    // applyMasterEffects(buffer);
    
    // Send feedback to UI about current state
    // sendEngineStatus(); // TODO: Implement when UI messaging is ready
    
    // Performance monitoring
    auto endTime = std::chrono::high_resolution_clock::now();
    auto elapsedMicros = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
    double elapsedSeconds = elapsedMicros / 1000000.0;
    double bufferDuration = numSamples / m_currentSampleRate;
    
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
// Plugin Management
bool HAMAudioProcessor::loadPlugin(int trackIndex, const juce::PluginDescription& desc, bool isInstrument)
{
    if (trackIndex < 0 || trackIndex >= static_cast<int>(m_trackPluginChains.size()))
        return false;
    
    auto& chain = m_trackPluginChains[trackIndex];
    if (!chain)
        return false;
    
    // Create plugin instance
    juce::String errorMessage;
    auto pluginInstance = m_formatManager.createPluginInstance(desc, m_currentSampleRate, 
                                                               m_currentBlockSize, errorMessage);
    
    if (!pluginInstance)
    {
        DBG("Failed to create plugin instance: " << errorMessage);
        return false;
    }
    
    // Prepare the plugin
    pluginInstance->prepareToPlay(m_currentSampleRate, m_currentBlockSize);
    
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
        
        // Reconnect routing for this track
        reconnectTrackRouting(trackIndex);
    }
    else
    {
        // Add as effect
        chain->effectNodes.push_back(node);
        
        // Reconnect routing to include new effect
        reconnectTrackRouting(trackIndex);
    }
    
    return true;
}

bool HAMAudioProcessor::removePlugin(int trackIndex, int pluginIndex)
{
    if (trackIndex < 0 || trackIndex >= static_cast<int>(m_trackPluginChains.size()))
        return false;
    
    auto& chain = m_trackPluginChains[trackIndex];
    if (!chain)
        return false;
    
    if (pluginIndex == -1)
    {
        // Remove instrument
        if (chain->instrumentNode)
        {
            m_pluginGraph->removeNode(chain->instrumentNode.get());
            chain->instrumentNode = nullptr;
            reconnectTrackRouting(trackIndex);
            return true;
        }
    }
    else if (pluginIndex >= 0 && pluginIndex < static_cast<int>(chain->effectNodes.size()))
    {
        // Remove effect
        m_pluginGraph->removeNode(chain->effectNodes[pluginIndex].get());
        chain->effectNodes.erase(chain->effectNodes.begin() + pluginIndex);
        reconnectTrackRouting(trackIndex);
        return true;
    }
    
    return false;
}

void HAMAudioProcessor::showPluginEditor(int trackIndex, int pluginIndex)
{
    if (trackIndex < 0 || trackIndex >= static_cast<int>(m_trackPluginChains.size()))
        return;
    
    auto& chain = m_trackPluginChains[trackIndex];
    if (!chain)
        return;
    
    juce::AudioProcessorGraph::Node::Ptr nodeToEdit;
    juce::String pluginName;
    
    if (pluginIndex == -1 && chain->instrumentNode)
    {
        nodeToEdit = chain->instrumentNode;
    }
    else if (pluginIndex >= 0 && pluginIndex < static_cast<int>(chain->effectNodes.size()))
    {
        nodeToEdit = chain->effectNodes[pluginIndex];
    }
    
    if (nodeToEdit && nodeToEdit->getProcessor())
    {
        // Get the plugin instance
        auto* plugin = dynamic_cast<juce::AudioPluginInstance*>(nodeToEdit->getProcessor());
        if (plugin)
        {
            pluginName = plugin->getName();
            
            // Use PluginWindowManager singleton to manage the window
            auto& windowManager = PluginWindowManager::getInstance();
            
            // Open or focus the plugin editor window
            windowManager.openPluginWindow(trackIndex, pluginIndex, plugin, pluginName);
        }
    }
}

void HAMAudioProcessor::sendEngineStatus()
{
    // TODO: Implement status updates to UI
    // This will send transport state, CPU usage, etc.
    // For now, this is a stub to allow compilation
}

void HAMAudioProcessor::reconnectTrackRouting(int trackIndex)
{
    if (trackIndex < 0 || trackIndex >= static_cast<int>(m_trackPluginChains.size()))
        return;
    
    auto& chain = m_trackPluginChains[trackIndex];
    if (!chain)
        return;
    
    // Disconnect all existing connections for this track
    // (In production, track which connections belong to which track)
    
    // Build new routing chain:
    // MIDI Input -> Instrument -> Effects -> Audio Output
    
    if (chain->instrumentNode)
    {
        // FIXED: Connect MIDI input to instrument
        // When processBlock() is called with a midiMessages buffer,
        // that buffer is automatically injected into the MIDI input node,
        // so this connection will receive the sequencer's MIDI output
        m_pluginGraph->addConnection(
            {{m_midiInputNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex},
             {chain->instrumentNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex}});
        
        // Route audio through effects chain
        juce::AudioProcessorGraph::NodeID lastNode = chain->instrumentNode->nodeID;
        
        for (auto& effectNode : chain->effectNodes)
        {
            // Connect stereo audio from last node to this effect
            for (int ch = 0; ch < 2; ++ch)
            {
                m_pluginGraph->addConnection({{lastNode, ch}, {effectNode->nodeID, ch}});
            }
            lastNode = effectNode->nodeID;
        }
        
        // Connect final node to output
        for (int ch = 0; ch < 2; ++ch)
        {
            m_pluginGraph->addConnection({{lastNode, ch}, {m_audioOutputNode->nodeID, ch}});
        }
    }
}

//==============================================================================
// Message Handler Setup
void HAMAudioProcessor::setupMessageHandlers()
{
    if (!m_messageDispatcher)
        return;
    
    // Register handlers for transport control messages
    m_messageDispatcher->registerUIHandler(UIToEngineMessage::TRANSPORT_PLAY,
        [this](const UIToEngineMessage& msg)
        {
            DBG("HAMAudioProcessor: Handling TRANSPORT_PLAY message");
            play();
        });
    
    m_messageDispatcher->registerUIHandler(UIToEngineMessage::TRANSPORT_STOP,
        [this](const UIToEngineMessage& msg)
        {
            DBG("HAMAudioProcessor: Handling TRANSPORT_STOP message");
            stop();
        });
    
    m_messageDispatcher->registerUIHandler(UIToEngineMessage::TRANSPORT_PAUSE,
        [this](const UIToEngineMessage& msg)
        {
            DBG("HAMAudioProcessor: Handling TRANSPORT_PAUSE message");
            if (m_transport)
                m_transport->pause();
        });
    
    m_messageDispatcher->registerUIHandler(UIToEngineMessage::SET_BPM,
        [this](const UIToEngineMessage& msg)
        {
            DBG("HAMAudioProcessor: Handling SET_BPM message: " + juce::String(msg.data.floatParam.value));
            if (m_masterClock)
                m_masterClock->setBPM(msg.data.floatParam.value);
        });
}

//==============================================================================
// UI Message Processing
void HAMAudioProcessor::processUIMessages()
{
    if (m_messageDispatcher)
    {
        // Process messages - handlers are already registered
        m_messageDispatcher->processUIMessages(10);
    }
}

//==============================================================================
// State Management
void HAMAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // TODO: Serialize state to binary format
    // For now, just clear the data
    destData.reset();
}

void HAMAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // TODO: Deserialize state from binary format
    // For now, this is a stub
    juce::ignoreUnused(data, sizeInBytes);
}

//==============================================================================
// Editor Creation
juce::AudioProcessorEditor* HAMAudioProcessor::createEditor()
{
    // Return nullptr for now - will be implemented when UI is ready
    return nullptr;
}

//==============================================================================
// Transport Control
void HAMAudioProcessor::play()
{
    DBG("HAMAudioProcessor::play() called");
    if (m_transport)
    {
        DBG("HAMAudioProcessor: Calling transport->play()");
        m_transport->play();
        
        // Verify transport actually started
        if (m_transport->isPlaying())
        {
            DBG("HAMAudioProcessor: Transport confirmed playing");
        }
        else
        {
            DBG("HAMAudioProcessor: WARNING - Transport failed to start!");
        }
    }
    else
    {
        DBG("HAMAudioProcessor: ERROR - No transport available!");
    }
}

void HAMAudioProcessor::stop()
{
    if (m_transport)
    {
        m_transport->stop();
    }
}

//==============================================================================
// Track Processor Management
void HAMAudioProcessor::addProcessorsForTrack(int trackIndex)
{
    // Ensure we have enough track chains
    while (trackIndex >= static_cast<int>(m_trackPluginChains.size()))
    {
        int newIndex = static_cast<int>(m_trackPluginChains.size());
        m_trackPluginChains.push_back(std::make_unique<TrackPluginChain>(newIndex));
        m_pitchEngines.push_back(std::make_unique<PitchEngine>());
        m_accumulatorEngines.push_back(std::make_unique<AccumulatorEngine>());
    }
}

void HAMAudioProcessor::removeProcessorsForTrack(int trackIndex)
{
    if (trackIndex >= 0 && trackIndex < static_cast<int>(m_trackPluginChains.size()))
    {
        // Remove any loaded plugins for this track
        auto& chain = m_trackPluginChains[trackIndex];
        if (chain)
        {
            if (chain->instrumentNode)
            {
                m_pluginGraph->removeNode(chain->instrumentNode.get());
            }
            for (auto& effectNode : chain->effectNodes)
            {
                m_pluginGraph->removeNode(effectNode.get());
            }
        }
    }
}

//==============================================================================
// MasterClock::Listener overrides
void HAMAudioProcessor::onClockPulse(int pulseNumber)
{
    // Handle clock pulse - typically used for timing synchronization
    juce::ignoreUnused(pulseNumber);
}

void HAMAudioProcessor::onClockStart()
{
    // Handle clock start event
}

void HAMAudioProcessor::onClockStop()
{
    // Handle clock stop event
}

void HAMAudioProcessor::onClockReset()
{
    // Handle clock reset event
}

void HAMAudioProcessor::onTempoChanged(float newBPM)
{
    // Handle tempo change event
    juce::ignoreUnused(newBPM);
}

} // namespace HAM