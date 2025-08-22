/*
  ==============================================================================

    Stage.cpp
    Implementation of the Stage model

  ==============================================================================
*/

#include "Stage.h"

namespace HAM {

//==============================================================================
Stage::Stage()
{
}

//==============================================================================
// Core Parameters

void Stage::setPitch(int pitch)
{
    m_pitch = juce::jlimit(0, 127, pitch);
}

void Stage::setGate(float gate)
{
    m_gate = juce::jlimit(0.0f, 1.0f, gate);
}

void Stage::setVelocity(int velocity)
{
    m_velocity = juce::jlimit(0, 127, velocity);
}

void Stage::setPulseCount(int count)
{
    m_pulseCount = juce::jlimit(1, 8, count);
}

//==============================================================================
// Ratcheting

void Stage::setRatchetCount(int pulseIndex, int ratchetCount)
{
    if (pulseIndex >= 0 && pulseIndex < 8)
    {
        m_ratchets[pulseIndex] = juce::jlimit(1, 8, ratchetCount);
    }
}

int Stage::getRatchetCount(int pulseIndex) const
{
    if (pulseIndex >= 0 && pulseIndex < 8)
        return m_ratchets[pulseIndex];
    return 1;
}

//==============================================================================
// Probability & Conditions

void Stage::setProbability(float probability)
{
    // Probability expressed in percent (0..100)
    m_probability = juce::jlimit(0.0f, 100.0f, probability);
}

//==============================================================================
// HAM Editor Features

void Stage::addCCMapping(const CCMapping& mapping)
{
    m_ccMappings.push_back(mapping);
}

void Stage::removeCCMapping(int index)
{
    if (index >= 0 && index < static_cast<int>(m_ccMappings.size()))
    {
        m_ccMappings.erase(m_ccMappings.begin() + index);
    }
}

//==============================================================================
// Slide & Glide

void Stage::setSlideTime(float time)
{
    m_slideTime = juce::jlimit(0.0f, 1.0f, time);
}

//==============================================================================
// Serialization

juce::ValueTree Stage::toValueTree() const
{
    juce::ValueTree tree("Stage");
    
    // Core parameters
    tree.setProperty("pitch", m_pitch, nullptr);
    tree.setProperty("gate", m_gate, nullptr);
    tree.setProperty("velocity", m_velocity, nullptr);
    tree.setProperty("pulseCount", m_pulseCount, nullptr);
    
    // Ratchets
    juce::ValueTree ratchets("Ratchets");
    for (int i = 0; i < 8; ++i)
    {
        ratchets.setProperty("r" + juce::String(i), m_ratchets[i], nullptr);
    }
    tree.addChild(ratchets, -1, nullptr);
    
    // Gate control
    tree.setProperty("gateType", static_cast<int>(m_gateType), nullptr);
    tree.setProperty("gateStretching", m_gateStretching, nullptr);
    
    // Probability
    tree.setProperty("probability", m_probability, nullptr);
    tree.setProperty("skipOnFirstLoop", m_skipOnFirstLoop, nullptr);
    
    // Slide
    tree.setProperty("slide", m_slide, nullptr);
    tree.setProperty("slideTime", m_slideTime, nullptr);
    
    // Modulation
    juce::ValueTree mod("Modulation");
    mod.setProperty("pitchBend", m_modulation.pitchBend, nullptr);
    mod.setProperty("modWheel", m_modulation.modWheel, nullptr);
    mod.setProperty("aftertouch", m_modulation.aftertouch, nullptr);
    mod.setProperty("enabled", m_modulation.enabled, nullptr);
    tree.addChild(mod, -1, nullptr);
    
    // CC Mappings
    if (!m_ccMappings.empty())
    {
        juce::ValueTree ccMappings("CCMappings");
        for (const auto& cc : m_ccMappings)
        {
            juce::ValueTree mapping("CCMapping");
            mapping.setProperty("ccNumber", cc.ccNumber, nullptr);
            mapping.setProperty("minValue", cc.minValue, nullptr);
            mapping.setProperty("maxValue", cc.maxValue, nullptr);
            mapping.setProperty("targetChannel", cc.targetChannel, nullptr);
            mapping.setProperty("enabled", cc.enabled, nullptr);
            ccMappings.addChild(mapping, -1, nullptr);
        }
        tree.addChild(ccMappings, -1, nullptr);
    }
    
    return tree;
}

void Stage::fromValueTree(const juce::ValueTree& tree)
{
    if (!tree.hasType("Stage"))
        return;
    
    // Core parameters
    m_pitch = tree.getProperty("pitch", 60);
    m_gate = tree.getProperty("gate", 0.5f);
    m_velocity = tree.getProperty("velocity", 100);
    m_pulseCount = tree.getProperty("pulseCount", 1);
    
    // Ratchets
    auto ratchets = tree.getChildWithName("Ratchets");
    if (ratchets.isValid())
    {
        for (int i = 0; i < 8; ++i)
        {
            m_ratchets[i] = ratchets.getProperty("r" + juce::String(i), 1);
        }
    }
    
    // Gate control
    m_gateType = static_cast<GateType>((int)tree.getProperty("gateType", 0));
    m_gateStretching = tree.getProperty("gateStretching", false);
    
    // Probability
    m_probability = tree.getProperty("probability", 100.0f);
    m_skipOnFirstLoop = tree.getProperty("skipOnFirstLoop", false);
    
    // Slide
    m_slide = tree.getProperty("slide", false);
    m_slideTime = tree.getProperty("slideTime", 0.1f);
    
    // Modulation
    auto mod = tree.getChildWithName("Modulation");
    if (mod.isValid())
    {
        m_modulation.pitchBend = mod.getProperty("pitchBend", 0.0f);
        m_modulation.modWheel = mod.getProperty("modWheel", 0.0f);
        m_modulation.aftertouch = mod.getProperty("aftertouch", 0.0f);
        m_modulation.enabled = mod.getProperty("enabled", false);
    }
    
    // CC Mappings
    m_ccMappings.clear();
    auto ccMappings = tree.getChildWithName("CCMappings");
    if (ccMappings.isValid())
    {
        for (int i = 0; i < ccMappings.getNumChildren(); ++i)
        {
            auto mapping = ccMappings.getChild(i);
            CCMapping cc;
            cc.ccNumber = mapping.getProperty("ccNumber", 1);
            cc.minValue = mapping.getProperty("minValue", 0.0f);
            cc.maxValue = mapping.getProperty("maxValue", 1.0f);
            cc.targetChannel = mapping.getProperty("targetChannel", 1);
            cc.enabled = mapping.getProperty("enabled", false);
            m_ccMappings.push_back(cc);
        }
    }
}

//==============================================================================
// State Query

bool Stage::isDefault() const
{
    return m_pitch == 60 &&
           m_gate == 0.5f &&
           m_velocity == 100 &&
           m_pulseCount == 1 &&
           m_ratchets[0] == 1 &&
           m_gateType == GateType::MULTIPLE &&
           !m_gateStretching &&
           m_probability == 100.0f &&
           !m_skipOnFirstLoop &&
           !m_slide &&
           m_slideTime == 0.1f &&
           !m_modulation.enabled &&
           m_ccMappings.empty();
}

void Stage::reset()
{
    m_pitch = 60;
    m_gate = 0.5f;
    m_velocity = 100;
    m_pulseCount = 1;
    
    for (auto& r : m_ratchets)
        r = 1;
    
    m_gateType = GateType::MULTIPLE;
    m_gateStretching = false;
    
    m_probability = 100.0f;
    m_skipOnFirstLoop = false;
    
    m_slide = false;
    m_slideTime = 0.1f;
    
    m_modulation = ModulationSettings();
    m_ccMappings.clear();
}

//==============================================================================
// Helper Methods for MidiEventGenerator

std::map<int, int> Stage::getCCMappingsAsMap() const
{
    std::map<int, int> result;
    for (const auto& mapping : m_ccMappings)
    {
        if (mapping.enabled)
        {
            // Convert float value (0-1) to MIDI CC value (0-127) based on min/max
            float normalizedValue = (mapping.maxValue - mapping.minValue) * 0.5f + mapping.minValue;
            int midiValue = static_cast<int>(normalizedValue * 127.0f);
            result[mapping.ccNumber] = juce::jlimit(0, 127, midiValue);
        }
    }
    return result;
}

} // namespace HAM