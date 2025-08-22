/*
  ==============================================================================

    HAMAudioProcessor.cpp
    Main audio processor implementation

  ==============================================================================
*/

#include "HAMAudioProcessor.h"
#include "../../Presentation/Views/MainEditor.h"

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

    // Register UI handlers for debug monitor toggle
    m_messageDispatcher->registerUIHandler(UIToEngineMessage::ENABLE_DEBUG_MODE, [this](const UIToEngineMessage&) {
        if (m_midiRouter) m_midiRouter->setDebugChannelEnabled(true);
    });
    m_messageDispatcher->registerUIHandler(UIToEngineMessage::DISABLE_DEBUG_MODE, [this](const UIToEngineMessage&) {
        if (m_midiRouter) m_midiRouter->setDebugChannelEnabled(false);
    });
    
    // Create default pattern
    m_currentPattern = std::make_unique<Pattern>();
    m_currentPattern->addTrack(); // Add one default track
    
    // Initialize per-track processors for default tracks
    for (int i = 0; i < 8; ++i)  // Start with 8 tracks
    {
        m_pitchEngines.push_back(std::make_unique<PitchEngine>());
        m_accumulatorEngines.push_back(std::make_unique<AccumulatorEngine>());
    }
    
    // Setup clock listener
    m_masterClock->addListener(this);
    m_masterClock->addListener(m_sequencerEngine.get());
    
    // Initialize sequencer with pattern
    m_sequencerEngine->setPattern(m_currentPattern.get());
    m_sequencerEngine->setVoiceManager(m_voiceManager.get());
}

HAMAudioProcessor::~HAMAudioProcessor()
{
    m_masterClock->removeListener(this);
    m_masterClock->removeListener(m_sequencerEngine.get());
}

//==============================================================================
void HAMAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    m_currentSampleRate = sampleRate;
    m_currentBlockSize = samplesPerBlock;
    
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
        
        // Get MIDI events from sequencer using pre-allocated buffer
        m_midiEventBuffer.clear();
        m_sequencerEngine->getAndClearMidiEvents(m_midiEventBuffer);
        
        // Render events to MIDI buffer
        for (const auto& event : m_midiEventBuffer)
        {
            if (event.sampleOffset < numSamples)
            {
                midiMessages.addEvent(event.message, event.sampleOffset);
            }
        }
        
        // Route MIDI through channel manager
        m_channelManager->processMidiBuffer(midiMessages, numSamples);
    }
    
    // Copy incoming MIDI for processing
    m_incomingMidi = midiMessages;
    
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
    // Route UI messages into dispatcher and handle key toggles
    int processed = 0;
    const int maxMessages = 32;
    UIToEngineMessage msg;
    while (processed < maxMessages && m_messageDispatcher->getNumPendingUIMessages() > 0)
    {
        // Pull from dispatcher's queue
        m_messageDispatcher->processUIMessages(1);
        processed++;
    }
}

// TODO: Implement when MessageTypes are defined
/*
void HAMAudioProcessor::handleParameterChange(const ParameterChangeMessage& msg)
{
    // Route parameter changes to appropriate engine
    // This is lock-free as all engines use atomic parameters
}  
*/

// TODO: Implement when MessageTypes are defined
/*
void HAMAudioProcessor::handleTransportCommand(const TransportMessage& msg)
{
    // Handle transport commands from UI
}
*/

//==============================================================================
void HAMAudioProcessor::play()
{
    m_transport->play();
    m_masterClock->start();
    m_sequencerEngine->start();
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
void HAMAudioProcessor::loadPattern(int patternIndex)
{
    // TODO: Implement pattern loading from storage
    // For now, just reset the current pattern
    m_currentPattern = std::make_unique<Pattern>();
    m_currentPattern->addTrack();
    m_sequencerEngine->setPattern(m_currentPattern.get());
}

void HAMAudioProcessor::savePattern(int patternIndex)
{
    // TODO: Implement pattern saving to storage
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
juce::AudioProcessorEditor* HAMAudioProcessor::createEditor()
{
    // TODO: Create and return MainEditor
    return nullptr;
}

//==============================================================================
void HAMAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // TODO: Serialize state to memory block
    // This would include current pattern, settings, etc.
}

void HAMAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // TODO: Deserialize state from memory block
}

} // namespace HAM