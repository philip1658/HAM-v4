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
#include <cmath>  // For std::round, std::abs

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
    
    // Get current stage with bounds checking
    size_t stageIndex = static_cast<size_t>(track.getCurrentStageIndex());
    size_t maxStages = static_cast<size_t>(track.getLength());
    
    // Validate stage index bounds
    if (maxStages == 0)
        return;  // No stages to process
    
    // Ensure stage index is within valid range
    if (stageIndex >= maxStages)
    {
        stageIndex = stageIndex % maxStages;  // Safe modulo operation
        track.setCurrentStageIndex(static_cast<int>(stageIndex));
    }
    
    Stage& stage = track.getStage(static_cast<int>(stageIndex));
    
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
            generateStageEvents(track, stage, trackIndex, static_cast<int>(stageIndex));
            
            // Start ratchet processing for this stage
            processRatchets(stage, track, trackIndex, static_cast<int>(stageIndex));
            
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
            size_t newStageIndex = static_cast<size_t>(track.getCurrentStageIndex());
            // Validate after advancement
            if (newStageIndex >= maxStages)
            {
                newStageIndex = 0;  // Wrap to beginning
                track.setCurrentStageIndex(0);
            }
            const Stage& newStage = track.getStage(static_cast<int>(newStageIndex));
            
            // Start the new stage
            if (m_voiceManager)
            {
                m_voiceManager->allNotesOff(track.getMidiChannel());
            }
            generateStageEvents(track, newStage, trackIndex, static_cast<int>(newStageIndex));
            processRatchets(newStage, track, trackIndex, static_cast<int>(newStageIndex));
            
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
                generateStageEvents(track, stage, trackIndex, static_cast<int>(stageIndex));
            }
            
            // Process ratchets for current pulse
            if (pulseCounter < stage.getPulseCount())
            {
                processRatchets(stage, track, trackIndex, static_cast<int>(stageIndex));
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
    
    // Bounds check the next index
    size_t maxStages = static_cast<size_t>(track.getLength());
    if (maxStages > 0 && static_cast<size_t>(nextIndex) >= maxStages)
    {
        nextIndex = static_cast<int>(static_cast<size_t>(nextIndex) % maxStages);
    }
    
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
    constexpr float SWING_EPSILON = 0.01f;
    if (swing > (50.0f + SWING_EPSILON) && (pulseNumber % 2) == 1)  // Apply swing to off-beats
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
    
    // Get velocity with proper rounding
    float velocityFloat = stage.getVelocity();
    int velocity = static_cast<int>(std::round(velocityFloat * 127.0f));
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
    
    // Calculate ratchet timing with double precision
    double samplesPerPulse = 0.0;  // Need to get from MasterClock
    if (m_masterClock)
    {
        // Use double precision for all timing calculations
        double bpm = static_cast<double>(m_masterClock->getBPM());
        double sampleRate = 44100.0;  // Need to get actual sample rate
        
        // Validate BPM range
        constexpr double MIN_BPM = 20.0;
        constexpr double MAX_BPM = 999.0;
        bpm = std::clamp(bpm, MIN_BPM, MAX_BPM);
        
        // Calculate with double precision and proper PPQN
        constexpr double PPQN = 24.0;  // Pulses per quarter note
        samplesPerPulse = (60.0 / bpm) * sampleRate / PPQN;
    }
    
    double samplesPerRatchet = samplesPerPulse / static_cast<double>(ratchetCount);
    
    // Generate ratchet events
    for (int r = 1; r < ratchetCount; ++r)  // Start at 1, first ratchet already played
    {
        // Use std::round for proper float-to-int conversion
        int sampleOffset = static_cast<int>(std::round(static_cast<double>(r) * samplesPerRatchet));
        
        // Apply ratchet probability with epsilon comparison
        float probability = stage.getRatchetProbability();
        constexpr float EPSILON = 1e-6f;
        if (probability < (1.0f - EPSILON))  // Not effectively 1.0
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
            int velocity = static_cast<int>(std::round(stage.getVelocity() * 127.0f));
            
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
    
    // Avoid division by zero
    if (totalPulses <= 0)
        return 0.0f;
    
    // Use double precision for intermediate calculation
    double position = static_cast<double>(currentPulse) / static_cast<double>(totalPulses);
    return static_cast<float>(position);
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
    
    // Calculate sample position within buffer with proper rounding
    float pulsePhase = m_masterClock->getPulsePhase();
    
    // Clamp phase to valid range
    pulsePhase = juce::jlimit(0.0f, 1.0f, pulsePhase);
    
    // Use double precision and round properly
    double offset = static_cast<double>(pulsePhase) * static_cast<double>(numSamples);
    return static_cast<int>(std::round(offset));
}

int SequencerEngine::getNextStageIndex(const Track& track, int currentIndex) const
{
    int length = track.getLength();
    Direction direction = track.getDirection();
    
    // Validate length
    if (length <= 0)
        return 0;  // Invalid length, return first stage
    
    // Ensure currentIndex is valid
    if (currentIndex < 0)
        currentIndex = 0;
    if (currentIndex >= length)
        currentIndex = currentIndex % length;
    
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
            if (length <= 1)
                return 0;  // Only one stage or invalid
            
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
    // Check skip probability with epsilon comparison
    float skipProbability = stage.getSkipProbability();
    constexpr float EPSILON = 1e-6f;
    if (skipProbability > EPSILON)  // Not effectively zero
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