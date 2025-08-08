/*
  ==============================================================================

    Track.cpp
    Implementation of the Track model

  ==============================================================================
*/

#include "Track.h"

namespace HAM {

//==============================================================================
Track::Track()
{
    // Initialize stages with default values
    for (int i = 0; i < 8; ++i)
    {
        m_stages[i].setPitch(60 + i);  // Default to C major scale
    }
}

//==============================================================================
// Stage Management

Stage& Track::getStage(int index)
{
    jassert(index >= 0 && index < 8);
    return m_stages[juce::jlimit(0, 7, index)];
}

const Stage& Track::getStage(int index) const
{
    jassert(index >= 0 && index < 8);
    return m_stages[juce::jlimit(0, 7, index)];
}

//==============================================================================
// MIDI Configuration

void Track::setMidiChannel(int channel)
{
    m_midiChannel = juce::jlimit(1, 16, channel);
}

void Track::setMaxVoices(int voices)
{
    m_maxVoices = juce::jlimit(1, 16, voices);
    
    // If we're in mono mode, force max voices to 1
    if (m_voiceMode == VoiceMode::MONO)
    {
        m_maxVoices = 1;
    }
}

//==============================================================================
// Sequencing Parameters

void Track::setLength(int length)
{
    m_length = juce::jlimit(1, 8, length);
    
    // Reset position if it's beyond new length
    if (m_currentStageIndex >= m_length)
    {
        m_currentStageIndex = 0;
    }
}

void Track::setDivision(int division)
{
    m_division = juce::jlimit(1, 64, division);
}

void Track::setSwing(float swing)
{
    m_swing = juce::jlimit(50.0f, 75.0f, swing);
}

void Track::setOctaveOffset(int octaves)
{
    m_octaveOffset = juce::jlimit(-4, 4, octaves);
}

void Track::setRootNote(int root)
{
    m_rootNote = juce::jlimit(0, 11, root);
}

//==============================================================================
// State Management

void Track::setCurrentStageIndex(int index)
{
    m_currentStageIndex = juce::jlimit(0, m_length - 1, index);
}

void Track::reset()
{
    // Reset all stages
    for (auto& stage : m_stages)
    {
        stage.reset();
    }
    
    // Reset track parameters to defaults
    m_name = "Track";
    m_color = juce::Colour(0xFF00FF88);
    m_enabled = true;
    m_muted = false;
    m_solo = false;
    
    m_midiChannel = 1;
    m_voiceMode = VoiceMode::MONO;
    m_maxVoices = 1;
    
    m_direction = Direction::FORWARD;
    m_length = 8;
    m_division = 1;
    m_swing = 50.0f;
    m_octaveOffset = 0;
    
    m_accumulatorMode = AccumulatorMode::OFF;
    m_accumulatorOffset = 0;
    m_accumulatorReset = 0;
    m_accumulatorValue = 0;
    
    m_scaleId = "chromatic";
    m_rootNote = 0;
    
    m_currentStageIndex = 0;
    
    // Re-initialize default pitches
    for (int i = 0; i < 8; ++i)
    {
        m_stages[i].setPitch(60 + i);
    }
}

void Track::resetPosition()
{
    m_currentStageIndex = 0;
    m_accumulatorValue = 0;
}

//==============================================================================
// Serialization

juce::ValueTree Track::toValueTree() const
{
    juce::ValueTree tree("Track");
    
    // Track info
    tree.setProperty("name", m_name, nullptr);
    tree.setProperty("color", m_color.toString(), nullptr);
    tree.setProperty("enabled", m_enabled, nullptr);
    tree.setProperty("muted", m_muted, nullptr);
    tree.setProperty("solo", m_solo, nullptr);
    
    // MIDI settings
    tree.setProperty("midiChannel", m_midiChannel, nullptr);
    tree.setProperty("voiceMode", static_cast<int>(m_voiceMode), nullptr);
    tree.setProperty("maxVoices", m_maxVoices, nullptr);
    
    // Sequencing
    tree.setProperty("direction", static_cast<int>(m_direction), nullptr);
    tree.setProperty("length", m_length, nullptr);
    tree.setProperty("division", m_division, nullptr);
    tree.setProperty("swing", m_swing, nullptr);
    tree.setProperty("octaveOffset", m_octaveOffset, nullptr);
    
    // Accumulator
    tree.setProperty("accumulatorMode", static_cast<int>(m_accumulatorMode), nullptr);
    tree.setProperty("accumulatorOffset", m_accumulatorOffset, nullptr);
    tree.setProperty("accumulatorReset", m_accumulatorReset, nullptr);
    tree.setProperty("accumulatorValue", m_accumulatorValue, nullptr);
    
    // Scale
    tree.setProperty("scaleId", m_scaleId, nullptr);
    tree.setProperty("rootNote", m_rootNote, nullptr);
    
    // Playback state
    tree.setProperty("currentStageIndex", m_currentStageIndex, nullptr);
    
    // Save stages
    juce::ValueTree stages("Stages");
    for (int i = 0; i < 8; ++i)
    {
        stages.addChild(m_stages[i].toValueTree(), -1, nullptr);
    }
    tree.addChild(stages, -1, nullptr);
    
    return tree;
}

void Track::fromValueTree(const juce::ValueTree& tree)
{
    if (!tree.hasType("Track"))
        return;
    
    // Track info
    m_name = tree.getProperty("name", "Track");
    m_color = juce::Colour::fromString(tree.getProperty("color", "FF00FF88").toString());
    m_enabled = tree.getProperty("enabled", true);
    m_muted = tree.getProperty("muted", false);
    m_solo = tree.getProperty("solo", false);
    
    // MIDI settings
    m_midiChannel = tree.getProperty("midiChannel", 1);
    m_voiceMode = static_cast<VoiceMode>((int)tree.getProperty("voiceMode", 0));
    m_maxVoices = tree.getProperty("maxVoices", 1);
    
    // Sequencing
    m_direction = static_cast<Direction>((int)tree.getProperty("direction", 0));
    m_length = tree.getProperty("length", 8);
    m_division = tree.getProperty("division", 1);
    m_swing = tree.getProperty("swing", 50.0f);
    m_octaveOffset = tree.getProperty("octaveOffset", 0);
    
    // Accumulator
    m_accumulatorMode = static_cast<AccumulatorMode>((int)tree.getProperty("accumulatorMode", 0));
    m_accumulatorOffset = tree.getProperty("accumulatorOffset", 0);
    m_accumulatorReset = tree.getProperty("accumulatorReset", 0);
    m_accumulatorValue = tree.getProperty("accumulatorValue", 0);
    
    // Scale
    m_scaleId = tree.getProperty("scaleId", "chromatic");
    m_rootNote = tree.getProperty("rootNote", 0);
    
    // Playback state
    m_currentStageIndex = tree.getProperty("currentStageIndex", 0);
    
    // Load stages
    auto stages = tree.getChildWithName("Stages");
    if (stages.isValid())
    {
        for (int i = 0; i < juce::jmin(8, stages.getNumChildren()); ++i)
        {
            m_stages[i].fromValueTree(stages.getChild(i));
        }
    }
}

//==============================================================================
// Scale Management

Scale* Track::getScale() const
{
    // Note: This returns a pointer to a static Scale object from ScaleManager
    // This is for compatibility with MidiEventGenerator that expects a pointer
    static Scale s_scaleBuffer;
    s_scaleBuffer = ScaleManager::getInstance().getScale(m_scaleId);
    return &s_scaleBuffer;
}

} // namespace HAM