/*
  ==============================================================================

    GateEngine.cpp
    Gate processing implementation

  ==============================================================================
*/

#include "GateEngine.h"
#include "../Models/Track.h"
#include <algorithm>
#include <cmath>

namespace HAM
{

//==============================================================================
GateEngine::GateEngine()
    : m_randomGenerator(std::random_device{}())
{
}

//==============================================================================
std::vector<GateEngine::GateEvent> GateEngine::processStageGate(
    const Stage& stage, 
    int pulseIndex,
    double sampleRate,
    int samplesPerPulse)
{
    std::vector<GateEvent> events;
    
    // Get gate type and check for REST
    GateType gateType = getGateTypeFromStage(stage);
    if (gateType == GateType::REST)
    {
        return events; // No gates for REST
    }
    
    // Get ratchet count for this pulse
    int ratchetCount = stage.getRatchetCount(pulseIndex);
    if (ratchetCount <= 0) ratchetCount = 1;
    if (ratchetCount > 8) ratchetCount = 8;
    
    // Calculate gate length
    float gateLength = stage.getGateLength() * m_globalGateLength.load();
    int gateLengthSamples = calculateGateLength(gateLength, samplesPerPulse, gateType);
    
    // Generate ratchet pattern
    auto ratchetOffsets = generateRatchetPattern(ratchetCount, samplesPerPulse);
    
    // Apply swing
    float swing = m_globalSwing.load() + stage.getSwing();
    swing = std::clamp(swing, -0.5f, 0.5f);
    
    // Process based on gate type
    switch (gateType)
    {
        case GateType::MULTIPLE:
        {
            // Generate gate for each ratchet
            for (int i = 0; i < ratchetCount; ++i)
            {
                // First check stage probability
                float stageProbability = stage.getProbability() / 100.0f;  // Convert from percentage
                if (!shouldTrigger(stageProbability))
                    continue;
                
                // Then check ratchet probability (only for ratchets after the first)
                if (i > 0)  // First ratchet always plays if stage probability passes
                {
                    float ratchetProbability = stage.getRatchetProbability();
                    if (!shouldTrigger(ratchetProbability))
                        continue;
                }
                
                int offset = ratchetOffsets[i];
                bool isEvenBeat = (i % 2) == 0;
                offset = applySwing(offset, swing, isEvenBeat);
                
                // Note on
                GateEvent noteOn;
                noteOn.isNoteOn = true;
                noteOn.sampleOffset = offset;
                noteOn.velocity = getEffectiveVelocity(stage, i);
                noteOn.ratchetIndex = i;
                events.push_back(noteOn);
                
                // Note off
                GateEvent noteOff;
                noteOff.isNoteOn = false;
                noteOff.sampleOffset = offset + gateLengthSamples;
                noteOff.velocity = 0.0f;
                noteOff.ratchetIndex = i;
                events.push_back(noteOff);
            }
            break;
        }
        
        case GateType::HOLD:
        {
            // Single gate spanning all ratchets
            float probability = stage.getProbability() / 100.0f;  // Convert from percentage
            if (shouldTrigger(probability))
            {
                // Note on at first ratchet
                GateEvent noteOn;
                noteOn.isNoteOn = true;
                noteOn.sampleOffset = ratchetOffsets[0];
                noteOn.velocity = getEffectiveVelocity(stage, 0);
                noteOn.ratchetIndex = 0;
                events.push_back(noteOn);
                
                // Note off after full pulse duration
                GateEvent noteOff;
                noteOff.isNoteOn = false;
                noteOff.sampleOffset = samplesPerPulse - 1;
                noteOff.velocity = 0.0f;
                noteOff.ratchetIndex = ratchetCount - 1;
                events.push_back(noteOff);
            }
            break;
        }
        
        case GateType::SINGLE:
        {
            // Gate only on first ratchet
            float probability = stage.getProbability() / 100.0f;  // Convert from percentage
            if (shouldTrigger(probability))
            {
                int offset = ratchetOffsets[0];
                bool isEvenBeat = (pulseIndex % 2) == 0;
                offset = applySwing(offset, swing, isEvenBeat);
                
                // Note on
                GateEvent noteOn;
                noteOn.isNoteOn = true;
                noteOn.sampleOffset = offset;
                noteOn.velocity = getEffectiveVelocity(stage, 0);
                noteOn.ratchetIndex = 0;
                events.push_back(noteOn);
                
                // Note off
                GateEvent noteOff;
                noteOff.isNoteOn = false;
                noteOff.sampleOffset = offset + gateLengthSamples;
                noteOff.velocity = 0.0f;
                noteOff.ratchetIndex = 0;
                events.push_back(noteOff);
            }
            break;
        }
        
        default:
            break;
    }
    
    return events;
}

//==============================================================================
int GateEngine::calculateGateLength(float gateLength, 
                                   int samplesPerPulse,
                                   GateType gateType)
{
    // Clamp gate length
    gateLength = std::clamp(gateLength, 0.01f, 1.0f);
    
    // Calculate base length
    int samples = static_cast<int>(gateLength * samplesPerPulse);
    
    // Apply minimum gate length
    float minMs = m_minGateLengthMs.load();
    int minSamples = static_cast<int>((minMs / 1000.0f) * 48000.0f); // Assuming 48kHz
    samples = std::max(samples, minSamples);
    
    // Apply stretching if enabled
    if (m_gateStretchingEnabled.load() && gateType == GateType::HOLD)
    {
        samples = samplesPerPulse - 1; // Full pulse minus 1 sample
    }
    
    // Ensure we don't exceed pulse length
    samples = std::min(samples, samplesPerPulse - 1);
    
    return samples;
}

//==============================================================================
int GateEngine::applySwing(int sampleOffset, float swingAmount, bool isEvenBeat)
{
    if (std::abs(swingAmount) < 0.01f)
        return sampleOffset;
    
    // Swing only affects off-beats (odd beats)
    if (isEvenBeat)
        return sampleOffset;
    
    // Calculate swing offset (positive = late, negative = early)
    float swingFactor = swingAmount * 0.25f; // Max 25% of beat
    int swingOffset = static_cast<int>(sampleOffset * swingFactor);
    
    return sampleOffset + swingOffset;
}

//==============================================================================
std::vector<int> GateEngine::generateRatchetPattern(int ratchetCount, int pulseLength)
{
    std::vector<int> offsets;
    
    if (ratchetCount <= 1)
    {
        offsets.push_back(0);
        return offsets;
    }
    
    // Divide pulse evenly
    float stepSize = static_cast<float>(pulseLength) / ratchetCount;
    
    for (int i = 0; i < ratchetCount; ++i)
    {
        int offset = static_cast<int>(i * stepSize);
        offsets.push_back(offset);
    }
    
    return offsets;
}

//==============================================================================
bool GateEngine::shouldTrigger(float probability)
{
    if (probability >= 0.99f)
        return true;
    if (probability <= 0.01f)
        return false;
    
    return m_distribution(m_randomGenerator) < probability;
}

//==============================================================================
GateEngine::RatchetPattern GateEngine::morphGatePatterns(
    const RatchetPattern& from,
    const RatchetPattern& to,
    float amount)
{
    amount = std::clamp(amount, 0.0f, 1.0f);
    
    RatchetPattern result;
    
    // Interpolate pulse count
    result.pulseCount = static_cast<int>(
        from.pulseCount * (1.0f - amount) + to.pulseCount * amount + 0.5f
    );
    
    // Interpolate subdivisions, velocities, and probabilities
    for (int i = 0; i < 8; ++i)
    {
        result.subdivisions[i] = static_cast<int>(
            from.subdivisions[i] * (1.0f - amount) + to.subdivisions[i] * amount + 0.5f
        );
        
        result.velocities[i] = from.velocities[i] * (1.0f - amount) + to.velocities[i] * amount;
        result.probabilities[i] = from.probabilities[i] * (1.0f - amount) + to.probabilities[i] * amount;
    }
    
    return result;
}

//==============================================================================
GateEngine::GateType GateEngine::getGateTypeFromStage(const Stage& stage) const
{
    // Direct mapping from Stage's GateType enum
    int gateTypeInt = stage.getGateTypeAsInt();
    
    switch (gateTypeInt)
    {
        case 0: return GateType::MULTIPLE;
        case 1: return GateType::HOLD;
        case 2: return GateType::SINGLE;
        case 3: return GateType::REST;
        default: return GateType::MULTIPLE;
    }
}

//==============================================================================
float GateEngine::getEffectiveVelocity(const Stage& stage, int ratchetIndex) const
{
    float baseVelocity = stage.getVelocity() / 127.0f;
    
    // Could apply per-ratchet velocity scaling here
    // For now, just return base velocity
    return baseVelocity;
}

//==============================================================================
// TrackGateProcessor implementation
//==============================================================================

TrackGateProcessor::TrackGateProcessor()
{
}

std::vector<GateEngine::GateEvent> TrackGateProcessor::processTrackGates(
    const Track* track,
    int currentStage,
    int pulseInStage,
    double sampleRate,
    int samplesPerPulse)
{
    std::vector<GateEngine::GateEvent> events;
    
    if (!track || currentStage < 0 || currentStage >= 8)
        return events;
    
    // Check if we've already processed this stage/pulse
    if (currentStage == m_lastProcessedStage && pulseInStage == m_lastProcessedPulse)
        return events;
    
    // Get the current stage
    const Stage& stage = track->getStage(currentStage);
    
    // Process gates for this stage
    events = m_gateEngine.processStageGate(stage, pulseInStage, sampleRate, samplesPerPulse);
    
    // Update tracking
    m_lastProcessedStage = currentStage;
    m_lastProcessedPulse = pulseInStage;
    
    return events;
}

void TrackGateProcessor::reset()
{
    m_lastProcessedStage = -1;
    m_lastProcessedPulse = -1;
}

} // namespace HAM