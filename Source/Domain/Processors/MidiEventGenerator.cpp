/*
  ==============================================================================

    MidiEventGenerator.cpp
    MIDI event generation implementation

  ==============================================================================
*/

#include "MidiEventGenerator.h"
#include <algorithm>
#include <cmath>

namespace HAM
{

//==============================================================================
MidiEventGenerator::MidiEventGenerator()
    : m_gateEngine(std::make_unique<GateEngine>())
    , m_pitchEngine(std::make_unique<PitchEngine>())
    , m_randomGenerator(std::random_device{}())
{
}

//==============================================================================
std::vector<MidiEventGenerator::MidiEvent> MidiEventGenerator::generateStageEvents(
    const Stage& stage,
    int stageIndex,
    const Track* track,
    int pulseIndex,
    double sampleRate,
    int samplesPerPulse,
    int bufferSize)
{
    std::vector<MidiEvent> events;
    
    if (!track) return events;
    
    // Get MIDI channel for this track
    int channel = track->getMidiChannel();
    if (channel < 1 || channel > 16) return events;
    
    // Set track swing on the gate engine
    // Convert from track's 50-75 range to -0.25 to +0.25 range for the engine
    float trackSwing = (track->getSwing() - 50.0f) / 100.0f;  // Convert to 0-0.25 range
    m_gateEngine->setGlobalSwing(trackSwing);
    
    // Process gate events
    auto gateEvents = m_gateEngine->processStageGate(stage, pulseIndex, sampleRate, samplesPerPulse);
    
    // Get pitch from stage (with scale quantization if needed)
    int basePitch = stage.getPitch();
    Scale* scale = track->getScale();
    if (scale)
    {
        // Set scale on engine and quantize
        m_pitchEngine->setScale(*scale);
        basePitch = m_pitchEngine->quantizeToScale(basePitch);
    }
    
    // Apply accumulator if enabled
    if (track->hasAccumulator())
    {
        basePitch += track->getAccumulatorValue();
        basePitch = std::clamp(basePitch, 0, 127);
    }
    
    // Convert gate events to MIDI events
    for (const auto& gateEvent : gateEvents)
    {
        MidiEvent midiEvent;
        midiEvent.channel = channel;
        midiEvent.trackIndex = track->getIndex();
        midiEvent.stageIndex = stageIndex;
        midiEvent.ratchetIndex = gateEvent.ratchetIndex;
        midiEvent.sampleOffset = std::min(gateEvent.sampleOffset, bufferSize - 1);
        
        if (gateEvent.isNoteOn)
        {
            // Apply velocity curve to stage velocity
            float randomValue = m_random.nextFloat(); // Random value for curve processing
            int baseVelocity = stage.getProcessedVelocity(randomValue);
            
            // Apply global velocity scaling
            int velocity = static_cast<int>(baseVelocity * m_globalVelocity.load());
            
            // Apply additional randomization if enabled
            velocity = applyVelocityRandomization(velocity);
            velocity = std::clamp(velocity, 1, 127);
            
            midiEvent.message = juce::MidiMessage::noteOn(channel, basePitch, (uint8)velocity);
            midiEvent.velocity = velocity / 127.0f;
        }
        else
        {
            midiEvent.message = juce::MidiMessage::noteOff(channel, basePitch);
            midiEvent.velocity = 0.0f;
        }
        
        // Apply timing randomization if enabled
        float timingRandom = m_timingRandom.load();
        if (timingRandom > 0.0f)
        {
            midiEvent.sampleOffset = applyTimingRandomization(midiEvent.sampleOffset, 
                                                             static_cast<int>(samplesPerPulse * 0.1f));
        }
        
        events.push_back(midiEvent);
    }
    
    // Generate CC events if enabled
    if (m_ccEnabled.load() && stage.hasModulation())
    {
        auto ccEvents = generateCCEvents(stage, channel, 0);
        events.insert(events.end(), ccEvents.begin(), ccEvents.end());
    }
    
    // Generate pitch bend if needed
    if (stage.hasPitchBend())
    {
        auto pbEvent = generatePitchBendEvent(stage, channel, 0);
        if (pbEvent.has_value())
        {
            events.push_back(pbEvent.value());
        }
    }
    
    return events;
}

//==============================================================================
std::vector<MidiEventGenerator::MidiEvent> MidiEventGenerator::generateRatchetedEvents(
    int baseNote,
    int velocity,
    int ratchetCount,
    int samplesPerPulse,
    int channel)
{
    std::vector<MidiEvent> events;
    
    if (ratchetCount <= 0) return events;
    
    int samplesPerRatchet = samplesPerPulse / ratchetCount;
    int gateLength = static_cast<int>(samplesPerRatchet * 0.9f); // 90% gate length
    
    for (int i = 0; i < ratchetCount; ++i)
    {
        int offset = i * samplesPerRatchet;
        
        // Note on
        events.push_back(createNoteOnEvent(baseNote, velocity, channel, offset));
        
        // Note off
        events.push_back(createNoteOffEvent(baseNote, channel, offset + gateLength));
    }
    
    return events;
}


//==============================================================================
void MidiEventGenerator::applyHumanization(std::vector<MidiEvent>& events,
                                          float timingAmount,
                                          float velocityAmount)
{
    for (auto& event : events)
    {
        // Apply timing humanization
        if (timingAmount > 0.0f)
        {
            float variation = m_normalDistribution(m_randomGenerator) * timingAmount;
            int timingOffset = static_cast<int>(variation * 10.0f); // +/- 10 samples max
            event.sampleOffset = std::max(0, event.sampleOffset + timingOffset);
        }
        
        // Apply velocity humanization to note-on events
        if (velocityAmount > 0.0f && event.message.isNoteOn())
        {
            float variation = m_normalDistribution(m_randomGenerator) * velocityAmount;
            int velocity = event.message.getVelocity();
            velocity += static_cast<int>(variation * 10.0f); // +/- 10 velocity max
            velocity = std::clamp(velocity, 1, 127);
            
            event.message = juce::MidiMessage::noteOn(
                event.message.getChannel(),
                event.message.getNoteNumber(),
                (uint8)velocity
            );
            
            event.velocity = velocity / 127.0f;
        }
    }
}

//==============================================================================
std::vector<MidiEventGenerator::MidiEvent> MidiEventGenerator::generateCCEvents(
    const Stage& stage,
    int channel,
    int sampleOffset)
{
    std::vector<MidiEvent> events;
    
    // Get CC mappings from stage
    auto ccMappings = stage.getCCMappingsAsMap();
    
    for (const auto& [ccNumber, value] : ccMappings)
    {
        if (ccNumber >= 0 && ccNumber <= 127)
        {
            events.push_back(createCCEvent(ccNumber, value, channel, sampleOffset));
        }
    }
    
    return events;
}

//==============================================================================
std::optional<MidiEventGenerator::MidiEvent> MidiEventGenerator::generatePitchBendEvent(
    const Stage& stage,
    int channel,
    int sampleOffset)
{
    if (!stage.hasPitchBend())
        return std::nullopt;
    
    float pitchBend = stage.getPitchBend(); // -1.0 to 1.0
    int bendValue = static_cast<int>((pitchBend + 1.0f) * 8192.0f);
    bendValue = std::clamp(bendValue, 0, 16383);
    
    return createPitchBendEvent(bendValue, channel, sampleOffset);
}

//==============================================================================
// Private helper methods
//==============================================================================

int MidiEventGenerator::applyVelocityRandomization(int baseVelocity)
{
    float randomAmount = m_velocityRandom.load();
    if (randomAmount <= 0.0f) return baseVelocity;
    
    float variation = (m_distribution(m_randomGenerator) - 0.5f) * 2.0f * randomAmount;
    int velocityOffset = static_cast<int>(variation * 20.0f); // +/- 20 velocity max
    
    return std::clamp(baseVelocity + velocityOffset, 1, 127);
}

int MidiEventGenerator::applyTimingRandomization(int sampleOffset, int maxOffset)
{
    float randomAmount = m_timingRandom.load();
    if (randomAmount <= 0.0f) return sampleOffset;
    
    float variation = (m_distribution(m_randomGenerator) - 0.5f) * 2.0f * randomAmount;
    int timingOffset = static_cast<int>(variation * maxOffset);
    
    return std::max(0, sampleOffset + timingOffset);
}

MidiEventGenerator::MidiEvent MidiEventGenerator::createNoteOnEvent(
    int note, int velocity, int channel, int sampleOffset)
{
    MidiEvent event;
    event.message = juce::MidiMessage::noteOn(channel, note, (uint8)velocity);
    event.sampleOffset = sampleOffset;
    event.channel = channel;
    event.velocity = velocity / 127.0f;
    event.trackIndex = 0;    // Initialize to default
    event.stageIndex = 0;    // Initialize to default
    event.ratchetIndex = 0;  // Initialize to default
    return event;
}

MidiEventGenerator::MidiEvent MidiEventGenerator::createNoteOffEvent(
    int note, int channel, int sampleOffset)
{
    MidiEvent event;
    event.message = juce::MidiMessage::noteOff(channel, note);
    event.sampleOffset = sampleOffset;
    event.channel = channel;
    event.velocity = 0.0f;
    event.trackIndex = 0;    // Initialize to default
    event.stageIndex = 0;    // Initialize to default
    event.ratchetIndex = 0;  // Initialize to default
    return event;
}

MidiEventGenerator::MidiEvent MidiEventGenerator::createCCEvent(
    int ccNumber, int value, int channel, int sampleOffset)
{
    MidiEvent event;
    event.message = juce::MidiMessage::controllerEvent(channel, ccNumber, value);
    event.sampleOffset = sampleOffset;
    event.channel = channel;
    event.velocity = 0.0f;
    event.trackIndex = 0;    // Initialize to default
    event.stageIndex = 0;    // Initialize to default
    event.ratchetIndex = 0;  // Initialize to default
    return event;
}

MidiEventGenerator::MidiEvent MidiEventGenerator::createPitchBendEvent(
    int value, int channel, int sampleOffset)
{
    MidiEvent event;
    event.message = juce::MidiMessage::pitchWheel(channel, value);
    event.sampleOffset = sampleOffset;
    event.channel = channel;
    event.velocity = 0.0f;
    event.trackIndex = 0;    // Initialize to default
    event.stageIndex = 0;    // Initialize to default
    event.ratchetIndex = 0;  // Initialize to default
    return event;
}


} // namespace HAM