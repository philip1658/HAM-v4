/*
  ==============================================================================

    SequencerEngine.cpp
    Core sequencer implementation with proper MONO/POLY mode handling:
    - POLY: Advances after 1 pulse, allows overlapping stages
    - MONO: Plays all pulses before advancing, cuts previous notes

  ==============================================================================
*/

#include "SequencerEngine.h"
#include "../Models/Scale.h"
#include <algorithm>
#include <random>

namespace HAM {

//==============================================================================
SequencerEngine::SequencerEngine()
{
    m_midiEventBuffer.resize(1024);
}

SequencerEngine::~SequencerEngine()
{
    if (m_masterClock)
    {
        m_masterClock->removeListener(this);
    }
}

//==============================================================================
// Pattern Management

void SequencerEngine::setActivePattern(std::shared_ptr<Pattern> pattern)
{
    m_activePattern = pattern;
    if (pattern)
    {
        initializeTrackStates();
    }
}

void SequencerEngine::queuePatternChange(std::shared_ptr<Pattern> pattern)
{
    m_queuedPattern = pattern;
}

//==============================================================================
// Transport Control

void SequencerEngine::start()
{
    m_state.store(State::PLAYING);
    if (m_masterClock)
    {
        m_masterClock->start();
    }
}

void SequencerEngine::stop()
{
    m_state.store(State::STOPPED);
    
    // Send note-offs for all active voices
    if (m_voiceManager)
    {
        for (int i = 0; i < VoiceManager::MAX_VOICES; ++i)
        {
            auto* voice = m_voiceManager->getVoice(i);
            if (voice && voice->active.load())
            {
                MidiEvent event;
                event.message = juce::MidiMessage::noteOff(
                    voice->channel.load(),
                    voice->noteNumber.load()
                );
                event.sampleOffset = 0;
                queueMidiEvent(event);
            }
        }
    }
    
    if (m_masterClock)
    {
        m_masterClock->stop();
    }
}

void SequencerEngine::reset()
{
    m_currentPatternBar.store(0);
    m_lastProcessedPulse.store(-1);
    
    if (m_activePattern)
    {
        for (auto& trackPtr : m_activePattern->getTracks())
        {
            trackPtr->resetPosition();
        }
    }
    
    initializeTrackStates();
    
    if (m_masterClock)
    {
        m_masterClock->reset();
    }
}

//==============================================================================
// Clock Integration

void SequencerEngine::setMasterClock(MasterClock* clock)
{
    if (m_masterClock)
    {
        m_masterClock->removeListener(this);
    }
    
    m_masterClock = clock;
    
    if (m_masterClock)
    {
        m_masterClock->addListener(this);
    }
}

void SequencerEngine::onClockPulse(int pulseNumber)
{
    if (m_state.load() != State::PLAYING || !m_activePattern)
        return;
    
    // Prevent double processing
    int lastPulse = m_lastProcessedPulse.load();
    if (pulseNumber <= lastPulse)
        return;
    
    m_lastProcessedPulse.store(pulseNumber);
    
    // Process each track
    int trackIndex = 0;
    for (auto& trackPtr : m_activePattern->getTracks())
    {
        if (trackPtr->isEnabled() && !trackPtr->isMuted())
        {
            // Check for solo mode
            if (!hasSoloedTracks() || trackPtr->isSolo())
            {
                processTrack(*trackPtr, trackIndex, pulseNumber);
            }
        }
        trackIndex++;
    }
    
    // Handle pattern switching at loop points
    if (isAtLoopPoint())
    {
        handlePatternSwitch();
    }
    
    // Update stats
    m_stats.tracksProcessed = trackIndex;
}

void SequencerEngine::onClockStart()
{
    // Clock started
}

void SequencerEngine::onClockStop()
{
    // Clock stopped
}

void SequencerEngine::onClockReset()
{
    reset();
}

void SequencerEngine::onTempoChanged(float newBPM)
{
    // Tempo changed - might need to recalculate some timing
}

//==============================================================================
// Track Processing

void SequencerEngine::processTrack(Track& track, int trackIndex, int pulseNumber)
{
    // Check if track should trigger based on division
    if (!shouldTrackTrigger(track, pulseNumber))
        return;
    
    // Get current stage
    int stageIndex = track.getCurrentStageIndex();
    Stage& stage = track.getStage(stageIndex);
    
    
    // Get track pulse counter (ensure within bounds)
    if (trackIndex >= 128)
        return;  // Too many tracks
    
    int pulseCounter = m_trackPulseCounters[trackIndex].load();
    int lastTrigger = m_trackLastTriggerPulse[trackIndex].load();
    
    
    // CRITICAL: Voice mode determines advancement behavior
    VoiceMode voiceMode = track.getVoiceMode();
    
    if (voiceMode == VoiceMode::POLY)
    {
        // POLY MODE: Play one pulse then advance on next pulse
        
        // Process current stage on first trigger
        if (pulseCounter == 0)
        {
            generateStageEvents(track, stage, trackIndex, stageIndex);
            
            // Start ratchet processing for this stage
            processRatchets(stage, track, trackIndex, stageIndex);
            
            // Set counter to 1 to indicate stage has been triggered
            m_trackPulseCounters[trackIndex].store(1);
        }
        else if (pulseCounter == 1)
        {
            // Already played this stage, advance on next pulse
            advanceTrackStage(track, pulseNumber);
            m_trackPulseCounters[trackIndex].store(0);
        }
    }
    else  // MONO mode
    {
        // MONO MODE: Play all pulses before advancing
        
        // Check if stage is complete from previous pulse
        if (pulseCounter >= stage.getPulseCount())
        {
            // Advance to next stage
            advanceTrackStage(track, pulseNumber);
            
            // Get the new stage after advancement
            stageIndex = track.getCurrentStageIndex();
            const Stage& newStage = track.getStage(stageIndex);
            
            // Start the new stage
            if (m_voiceManager)
            {
                m_voiceManager->allNotesOff(track.getMidiChannel());
            }
            generateStageEvents(track, newStage, trackIndex, stageIndex);
            processRatchets(newStage, track, trackIndex, stageIndex);
            
            // Reset counter for new stage (already playing first pulse)
            m_trackPulseCounters[trackIndex].store(1);
        }
        else
        {
            // Continue current stage
            if (pulseCounter == 0)
            {
                // Starting a stage for the first time
                if (m_voiceManager)
                {
                    m_voiceManager->allNotesOff(track.getMidiChannel());
                }
                generateStageEvents(track, stage, trackIndex, stageIndex);
            }
            
            // Process ratchets for current pulse
            if (pulseCounter < stage.getPulseCount())
            {
                processRatchets(stage, track, trackIndex, stageIndex);
            }
            
            // Increment pulse counter for next trigger
            m_trackPulseCounters[trackIndex].store(pulseCounter + 1);
        }
    }
    
    m_trackLastTriggerPulse[trackIndex].store(pulseNumber);
    m_stats.stagesProcessed++;
}

void SequencerEngine::advanceTrackStage(Track& track, int pulseNumber)
{
    int currentIndex = track.getCurrentStageIndex();
    int nextIndex = getNextStageIndex(track, currentIndex);
    
    // Apply accumulator if needed
    if (track.getAccumulatorMode() == AccumulatorMode::STAGE)
    {
        int currentValue = track.getAccumulatorValue();
        int offset = track.getAccumulatorOffset();
        int resetValue = track.getAccumulatorReset();
        
        currentValue += offset;
        
        // Handle reset
        if (resetValue > 0 && std::abs(currentValue) >= resetValue)
        {
            currentValue = 0;
        }
        
        track.setAccumulatorValue(currentValue);
    }
    
    track.setCurrentStageIndex(nextIndex);
}

bool SequencerEngine::shouldTrackTrigger(const Track& track, int pulseNumber) const
{
    // Check clock division (ensure it's at least 1)
    int division = juce::jmax(1, track.getDivision());
    
    // Calculate if we're on a division boundary
    // Division 1 = every pulse, Division 2 = every 2 pulses, etc.
    int pulsesPerDivision = division;  // Direct division value
    
    // Apply swing timing if available
    float swing = track.getSwing();
    if (swing > 50.0f && (pulseNumber % 2) == 1)  // Apply swing to off-beats
    {
        // Swing calculation would modify the timing here
        // For now, just check the boundary
    }
    
    return (pulseNumber % pulsesPerDivision) == 0;
}

void SequencerEngine::generateStageEvents(Track& track, const Stage& stage, 
                                         int trackIndex, int stageIndex)
{
    // Skip if stage has REST gate
    if (stage.getGateType() == GateType::REST)
        return;
    
    // Check skip conditions
    if (shouldSkipStage(stage))
        return;
    
    // Calculate final pitch
    int pitch = calculatePitch(track, stage);
    
    // Ensure pitch is in valid MIDI range
    pitch = juce::jlimit(0, 127, pitch);
    
    // Get velocity
    int velocity = static_cast<int>(stage.getVelocity() * 127.0f);
    velocity = juce::jlimit(1, 127, velocity);
    
    // Create MIDI note-on
    MidiEvent event;
    event.message = juce::MidiMessage::noteOn(
        track.getMidiChannel(),
        pitch,
        static_cast<juce::uint8>(velocity)
    );
    event.trackIndex = trackIndex;
    event.stageIndex = stageIndex;
    event.sampleOffset = 0;  // Will be calculated in processBlock
    
    // Handle voice allocation
    if (m_voiceManager)
    {
        if (track.getVoiceMode() == VoiceMode::MONO)
        {
            // In MONO mode, stop previous note
            m_voiceManager->allNotesOff(track.getMidiChannel());
        }
        
        // Allocate voice for new note
        m_voiceManager->noteOn(pitch, velocity, track.getMidiChannel());
    }
    
    queueMidiEvent(event);
    m_stats.eventsGenerated++;
    
    // Handle gate length (schedule note-off)
    float gateLength = stage.getGateLength();
    if (gateLength < 1.0f && stage.getGateType() != GateType::HOLD)
    {
        // Schedule note-off based on gate length
        // This will be handled in processRatchets for proper timing
    }
}

//==============================================================================
// Stage Processing

void SequencerEngine::processRatchets(const Stage& stage, Track& track,
                                     int trackIndex, int stageIndex)
{
    // Get ratchet settings for current pulse
    int pulseIndex = track.getCurrentStageIndex() % stage.getPulseCount();
    
    // Check if we have ratchets for this pulse
    const auto& ratchets = stage.getRatchets();
    if (pulseIndex >= ratchets.size())
        return;
    
    int ratchetCount = ratchets[pulseIndex];
    if (ratchetCount <= 1)
        return;  // No ratcheting
    
    // Calculate ratchet timing
    double samplesPerPulse = 0.0;  // Need to get from MasterClock
    if (m_masterClock)
    {
        // Calculate based on BPM and sample rate
        float bpm = m_masterClock->getBPM();
        double sampleRate = 44100.0;  // Need to get actual sample rate
        samplesPerPulse = (60.0 / bpm) * sampleRate / 24.0;  // 24 PPQN
    }
    
    double samplesPerRatchet = samplesPerPulse / ratchetCount;
    
    // Generate ratchet events
    for (int r = 1; r < ratchetCount; ++r)  // Start at 1, first ratchet already played
    {
        int sampleOffset = static_cast<int>(r * samplesPerRatchet);
        
        // Apply ratchet probability
        float probability = stage.getRatchetProbability();
        if (probability < 1.0f)
        {
            static std::random_device rd;
            static std::mt19937 gen(rd());
            static std::uniform_real_distribution<> dis(0.0, 1.0);
            
            if (dis(gen) > probability)
                continue;  // Skip this ratchet
        }
        
        // Generate ratchet note based on gate type
        GateType gateType = stage.getGateType();
        
        if (gateType == GateType::MULTIPLE || gateType == GateType::SINGLE)
        {
            // Create note event for ratchet
            int pitch = calculatePitch(track, stage);
            int velocity = static_cast<int>(stage.getVelocity() * 127.0f);
            
            MidiEvent event;
            event.message = juce::MidiMessage::noteOn(
                track.getMidiChannel(),
                pitch,
                static_cast<juce::uint8>(velocity)
            );
            event.trackIndex = trackIndex;
            event.stageIndex = stageIndex;
            event.sampleOffset = sampleOffset;
            
            queueMidiEvent(event);
            
            // Apply accumulator for per-ratchet mode
            if (track.getAccumulatorMode() == AccumulatorMode::RATCHET)
            {
                int currentValue = track.getAccumulatorValue();
                currentValue += track.getAccumulatorOffset();
                
                // Handle reset
                int resetValue = track.getAccumulatorReset();
                if (resetValue > 0 && std::abs(currentValue) >= resetValue)
                {
                    currentValue = 0;
                }
                
                track.setAccumulatorValue(currentValue);
            }
        }
    }
}

int SequencerEngine::calculatePitch(const Track& track, const Stage& stage) const
{
    // Start with base pitch
    int pitch = stage.getPitch();
    
    // Apply octave offset
    pitch += track.getOctaveOffset() * 12;
    
    // Apply accumulator
    pitch = applyAccumulator(track, pitch);
    
    // TODO: Apply scale quantization when Scale class is integrated
    // const juce::String& scaleId = track.getScaleId();
    // if (scaleId != "chromatic")
    // {
    //     int rootNote = track.getRootNote();
    //     // Quantize pitch to scale
    // }
    
    return pitch;
}

int SequencerEngine::applyAccumulator(const Track& track, int basePitch) const
{
    int accumulatorValue = track.getAccumulatorValue();
    return basePitch + accumulatorValue;
}

//==============================================================================
// Pattern Management

void SequencerEngine::handlePatternSwitch()
{
    if (m_queuedPattern)
    {
        setActivePattern(m_queuedPattern);
        m_queuedPattern.reset();
        reset();
    }
}

bool SequencerEngine::isAtLoopPoint() const
{
    if (!m_activePattern || !m_masterClock)
        return false;
    
    // Check if we're at the end of the pattern
    int currentBar = m_masterClock->getCurrentBar();
    int patternBars = getTotalPatternBars();
    
    return (currentBar > 0 && (currentBar % patternBars) == 0);
}

//==============================================================================
// State Query

float SequencerEngine::getPatternPosition() const
{
    if (!m_activePattern || !m_masterClock)
        return 0.0f;
    
    int currentPulse = m_masterClock->getCurrentPulse();
    int totalPulses = getTotalPatternBars() * 96;  // 96 pulses per bar
    
    return static_cast<float>(currentPulse) / static_cast<float>(totalPulses);
}

int SequencerEngine::getTotalPatternBars() const
{
    if (!m_activePattern)
        return 4;  // Default 4 bars
    
    // Calculate based on longest track
    int maxBars = 4;
    for (const auto& trackPtr : m_activePattern->getTracks())
    {
        int trackBars = trackPtr->getLength();  // Stages determine length
        maxBars = std::max(maxBars, trackBars);
    }
    
    return maxBars;
}

bool SequencerEngine::hasSoloedTracks() const
{
    if (!m_activePattern)
        return false;
    
    for (const auto& trackPtr : m_activePattern->getTracks())
    {
        if (trackPtr->isSolo())
            return true;
    }
    
    return false;
}

//==============================================================================
// MIDI Output

void SequencerEngine::processBlock(double sampleRate, int numSamples)
{
    // Process timing for this block
    // The actual MIDI event generation happens in onClockPulse
    // Events are then retrieved via getAndClearMidiEvents
    
    // Update sample rate if needed
    // This method is mainly here for block-based processing coordination
}

std::vector<SequencerEngine::MidiEvent> SequencerEngine::getPendingMidiEvents()
{
    std::vector<MidiEvent> events;
    
    // Read from lock-free FIFO
    int start1, size1, start2, size2;
    m_midiEventFifo.prepareToRead(m_midiEventFifo.getNumReady(), start1, size1, start2, size2);
    
    // Copy events
    for (int i = 0; i < size1; ++i)
    {
        events.push_back(m_midiEventBuffer[start1 + i]);
    }
    for (int i = 0; i < size2; ++i)
    {
        events.push_back(m_midiEventBuffer[start2 + i]);
    }
    
    m_midiEventFifo.finishedRead(size1 + size2);
    
    return events;
}

void SequencerEngine::getAndClearMidiEvents(std::vector<MidiEvent>& events)
{
    events = getPendingMidiEvents();  // Just get all pending events
}

//==============================================================================
// Internal Methods

void SequencerEngine::initializeTrackStates()
{
    if (!m_activePattern)
        return;
    
    size_t numTracks = m_activePattern->getTracks().size();
    
    // Initialize counters for active tracks
    for (size_t i = 0; i < std::min(numTracks, size_t(128)); ++i)
    {
        m_trackPulseCounters[i].store(0);
        m_trackLastTriggerPulse[i].store(-1);
    }
}

void SequencerEngine::queueMidiEvent(const MidiEvent& event)
{
    // Write to lock-free FIFO
    int start1, size1, start2, size2;
    m_midiEventFifo.prepareToWrite(1, start1, size1, start2, size2);
    
    if (size1 > 0)
    {
        m_midiEventBuffer[start1] = event;
        m_midiEventFifo.finishedWrite(1);
    }
    else if (size2 > 0)
    {
        m_midiEventBuffer[start2] = event;
        m_midiEventFifo.finishedWrite(1);
    }
    // If both are 0, the FIFO is full - event is dropped
}

int SequencerEngine::calculateSampleOffset(int pulseNumber, int numSamples) const
{
    if (!m_masterClock)
        return 0;
    
    // Calculate sample position within buffer
    float pulsePhase = m_masterClock->getPulsePhase();
    return static_cast<int>(pulsePhase * numSamples);
}

int SequencerEngine::getNextStageIndex(const Track& track, int currentIndex) const
{
    int length = track.getLength();
    Direction direction = track.getDirection();
    
    switch (direction)
    {
        case Direction::FORWARD:
            return (currentIndex + 1) % length;
            
        case Direction::BACKWARD:
            return (currentIndex - 1 + length) % length;
            
        case Direction::PENDULUM:
        {
            // Pendulum mode: forward then backward
            // Track needs to store pendulum direction state
            static std::unordered_map<const Track*, bool> pendulumDirections;
            bool& goingForward = pendulumDirections[&track];
            
            int nextIndex;
            if (goingForward)
            {
                nextIndex = currentIndex + 1;
                if (nextIndex >= length - 1)
                {
                    goingForward = false;  // Hit end, reverse
                    nextIndex = length - 1;
                }
            }
            else
            {
                nextIndex = currentIndex - 1;
                if (nextIndex <= 0)
                {
                    goingForward = true;  // Hit start, reverse
                    nextIndex = 0;
                }
            }
            return nextIndex;
        }
            
        case Direction::RANDOM:
        {
            static std::random_device rd;
            static std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, length - 1);
            return dis(gen);
        }
            
        default:
            return (currentIndex + 1) % length;
    }
}

bool SequencerEngine::shouldSkipStage(const Stage& stage) const
{
    // Check skip probability
    float skipProbability = stage.getSkipProbability();
    if (skipProbability > 0.0f)
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_real_distribution<> dis(0.0, 1.0);
        
        if (dis(gen) < skipProbability)
            return true;
    }
    
    // Check skip conditions
    SkipCondition condition = stage.getSkipCondition();
    if (condition != SkipCondition::NEVER)
    {
        // TODO: Implement skip condition logic
        // This would check things like:
        // - EVERY_2: Skip every other time
        // - EVERY_3: Skip every third time
        // - FILL: Only play during fills
        // etc.
    }
    
    return false;
}

} // namespace HAM