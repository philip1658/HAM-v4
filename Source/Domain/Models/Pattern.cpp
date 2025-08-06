/*
  ==============================================================================

    Pattern.cpp
    Implementation of the Pattern model

  ==============================================================================
*/

#include "Pattern.h"

namespace HAM {

//==============================================================================
Pattern::Pattern()
{
    // Initialize with one default track
    addTrack();
    
    // Initialize scene names
    for (int i = 0; i < 64; ++i)
    {
        m_sceneNames[i] = "Scene " + juce::String(i + 1);
    }
}

//==============================================================================
// Pattern Info

void Pattern::setBPM(float bpm)
{
    m_bpm = juce::jlimit(20.0f, 999.0f, bpm);
    m_modified = true;
}

void Pattern::setTimeSignature(int numerator, int denominator)
{
    m_timeSignatureNum = juce::jlimit(1, 16, numerator);
    m_timeSignatureDenom = juce::jlimit(1, 16, denominator);
    m_modified = true;
}

//==============================================================================
// Track Management

int Pattern::addTrack()
{
    if (m_tracks.size() >= MAX_TRACKS)
        return -1;
    
    auto track = std::make_unique<Track>();
    track->setName("Track " + juce::String(m_tracks.size() + 1));
    
    // Assign different colors to tracks
    const juce::Colour trackColors[] = {
        juce::Colour(0xFF00FF88),  // Mint
        juce::Colour(0xFF00D9FF),  // Sky Blue
        juce::Colour(0xFFFF0088),  // Hot Pink
        juce::Colour(0xFFFFAA00),  // Amber
        juce::Colour(0xFF8800FF),  // Purple
        juce::Colour(0xFFFF5500),  // Orange
    };
    
    int colorIndex = m_tracks.size() % 6;
    track->setColor(trackColors[colorIndex]);
    
    // Set MIDI channel based on track index
    track->setMidiChannel((m_tracks.size() % 16) + 1);
    
    m_tracks.push_back(std::move(track));
    m_modified = true;
    
    return static_cast<int>(m_tracks.size() - 1);
}

void Pattern::removeTrack(int index)
{
    if (index >= 0 && index < static_cast<int>(m_tracks.size()))
    {
        m_tracks.erase(m_tracks.begin() + index);
        m_modified = true;
    }
}

Track* Pattern::getTrack(int index)
{
    if (index >= 0 && index < static_cast<int>(m_tracks.size()))
        return m_tracks[index].get();
    return nullptr;
}

const Track* Pattern::getTrack(int index) const
{
    if (index >= 0 && index < static_cast<int>(m_tracks.size()))
        return m_tracks[index].get();
    return nullptr;
}

void Pattern::clearTracks()
{
    m_tracks.clear();
    m_modified = true;
}

//==============================================================================
// Scene Management

void Pattern::setSceneIndex(int index)
{
    m_sceneIndex = juce::jlimit(0, 63, index);
    m_modified = true;
}

void Pattern::setSceneName(int index, const juce::String& name)
{
    if (index >= 0 && index < 64)
    {
        m_sceneNames[index] = name;
        m_modified = true;
    }
}

juce::String Pattern::getSceneName(int index) const
{
    if (index >= 0 && index < 64)
        return m_sceneNames[index];
    return "";
}

void Pattern::setSceneUsed(int index, bool used)
{
    if (index >= 0 && index < 64)
    {
        m_scenesUsed[index] = used;
        m_modified = true;
    }
}

bool Pattern::isSceneUsed(int index) const
{
    if (index >= 0 && index < 64)
        return m_scenesUsed[index];
    return false;
}

//==============================================================================
// Snapshot Management

int Pattern::captureSnapshot(const juce::String& name)
{
    if (m_snapshots.size() >= MAX_SNAPSHOTS)
        return -1;
    
    auto snapshot = std::make_unique<Snapshot>();
    snapshot->name = name.isEmpty() ? 
        "Snapshot " + juce::String(m_snapshots.size() + 1) : name;
    snapshot->data = toValueTree();
    snapshot->timestamp = juce::Time::currentTimeMillis();
    
    m_snapshots.push_back(std::move(snapshot));
    m_modified = true;
    
    return static_cast<int>(m_snapshots.size() - 1);
}

void Pattern::recallSnapshot(int index)
{
    if (index >= 0 && index < static_cast<int>(m_snapshots.size()))
    {
        fromValueTree(m_snapshots[index]->data);
        m_modified = true;
    }
}

Snapshot* Pattern::getSnapshot(int index)
{
    if (index >= 0 && index < static_cast<int>(m_snapshots.size()))
        return m_snapshots[index].get();
    return nullptr;
}

const Snapshot* Pattern::getSnapshot(int index) const
{
    if (index >= 0 && index < static_cast<int>(m_snapshots.size()))
        return m_snapshots[index].get();
    return nullptr;
}

void Pattern::removeSnapshot(int index)
{
    if (index >= 0 && index < static_cast<int>(m_snapshots.size()))
    {
        m_snapshots.erase(m_snapshots.begin() + index);
        m_modified = true;
    }
}

void Pattern::clearSnapshots()
{
    m_snapshots.clear();
    m_modified = true;
}

//==============================================================================
// Pattern Morphing

void Pattern::morphSnapshots(int indexA, int indexB, float position)
{
    auto snapA = getSnapshot(indexA);
    auto snapB = getSnapshot(indexB);
    
    if (!snapA || !snapB)
        return;
    
    // Create temporary patterns to load snapshots into
    Pattern tempPatternA;
    Pattern tempPatternB;
    
    // Load snapshots into temporary patterns
    tempPatternA.fromValueTree(snapA->data);
    tempPatternB.fromValueTree(snapB->data);
    
    // Clamp position
    position = juce::jlimit(0.0f, 1.0f, position);
    
    // Apply quantization if enabled
    if (m_morphQuantization > 0)
    {
        float step = 1.0f / m_morphQuantization;
        position = std::round(position / step) * step;
    }
    
    // Morph global parameters
    if (m_morphInterpolation)
    {
        interpolateValue(m_bpm, tempPatternA.m_bpm, tempPatternB.m_bpm, position);
        interpolateValue(m_globalSwing, tempPatternA.m_globalSwing, tempPatternB.m_globalSwing, position);
        interpolateValue(m_globalGateLength, tempPatternA.m_globalGateLength, tempPatternB.m_globalGateLength, position);
    }
    
    // Morph tracks
    int trackCount = juce::jmin(tempPatternA.getTrackCount(), tempPatternB.getTrackCount());
    for (int t = 0; t < trackCount && t < getTrackCount(); ++t)
    {
        auto trackA = tempPatternA.getTrack(t);
        auto trackB = tempPatternB.getTrack(t);
        auto currentTrack = getTrack(t);
        
        if (!trackA || !trackB || !currentTrack)
            continue;
        
        // Morph track parameters
        if (m_morphInterpolation)
        {
            float swingA = trackA->getSwing();
            float swingB = trackB->getSwing();
            float morphedSwing = swingA + (swingB - swingA) * position;
            currentTrack->setSwing(morphedSwing);
            
            int octaveA = trackA->getOctaveOffset();
            int octaveB = trackB->getOctaveOffset();
            int morphedOctave = static_cast<int>(std::round(octaveA + (octaveB - octaveA) * position));
            currentTrack->setOctaveOffset(morphedOctave);
        }
        
        // Morph stages
        for (int s = 0; s < 8; ++s)
        {
            interpolateStage(currentTrack->getStage(s), 
                           trackA->getStage(s), 
                           trackB->getStage(s), 
                           position);
        }
    }
    
    m_modified = true;
}

void Pattern::interpolateValue(float& target, float valueA, float valueB, float position)
{
    target = valueA + (valueB - valueA) * position;
}

void Pattern::interpolateStage(Stage& target, const Stage& stageA, const Stage& stageB, float position)
{
    if (!m_morphInterpolation)
    {
        // Binary switch at 50%
        if (position < 0.5f)
            target = stageA;
        else
            target = stageB;
        return;
    }
    
    // Interpolate continuous values
    int pitchA = stageA.getPitch();
    int pitchB = stageB.getPitch();
    target.setPitch(static_cast<int>(std::round(pitchA + (pitchB - pitchA) * position)));
    
    float gateA = stageA.getGate();
    float gateB = stageB.getGate();
    target.setGate(gateA + (gateB - gateA) * position);
    
    int velocityA = stageA.getVelocity();
    int velocityB = stageB.getVelocity();
    target.setVelocity(static_cast<int>(std::round(velocityA + (velocityB - velocityA) * position)));
    
    float probA = stageA.getProbability();
    float probB = stageB.getProbability();
    target.setProbability(probA + (probB - probA) * position);
    
    // Binary properties switch at 50%
    if (position < 0.5f)
    {
        target.setGateType(stageA.getGateType());
        target.setSlide(stageA.hasSlide());
        target.setSkipOnFirstLoop(stageA.shouldSkipOnFirstLoop());
    }
    else
    {
        target.setGateType(stageB.getGateType());
        target.setSlide(stageB.hasSlide());
        target.setSkipOnFirstLoop(stageB.shouldSkipOnFirstLoop());
    }
}

//==============================================================================
// Global Pattern Settings

void Pattern::setGlobalSwing(float swing)
{
    m_globalSwing = juce::jlimit(50.0f, 75.0f, swing);
    m_modified = true;
}

void Pattern::setGlobalGateLength(float multiplier)
{
    m_globalGateLength = juce::jlimit(0.1f, 2.0f, multiplier);
    m_modified = true;
}

void Pattern::setLoopLength(int bars)
{
    m_loopLength = juce::jlimit(1, 64, bars);
    m_modified = true;
}

//==============================================================================
// State Management

void Pattern::reset()
{
    m_name = "New Pattern";
    m_author.clear();
    m_description.clear();
    
    m_bpm = 120.0f;
    m_timeSignatureNum = 4;
    m_timeSignatureDenom = 4;
    m_loopLength = 4;
    
    clearTracks();
    addTrack();  // Add one default track
    
    m_sceneIndex = 0;
    for (int i = 0; i < 64; ++i)
    {
        m_sceneNames[i] = "Scene " + juce::String(i + 1);
        m_scenesUsed[i] = false;
    }
    
    clearSnapshots();
    
    m_morphQuantization = 0;
    m_morphInterpolation = true;
    
    m_globalSwing = 50.0f;
    m_globalGateLength = 1.0f;
    
    m_modified = false;
}

void Pattern::resetPositions()
{
    for (auto& track : m_tracks)
    {
        track->resetPosition();
    }
}

//==============================================================================
// Serialization

juce::ValueTree Pattern::toValueTree() const
{
    juce::ValueTree tree("Pattern");
    
    // Pattern info
    tree.setProperty("name", m_name, nullptr);
    tree.setProperty("author", m_author, nullptr);
    tree.setProperty("description", m_description, nullptr);
    
    // Timing
    tree.setProperty("bpm", m_bpm, nullptr);
    tree.setProperty("timeSignatureNum", m_timeSignatureNum, nullptr);
    tree.setProperty("timeSignatureDenom", m_timeSignatureDenom, nullptr);
    tree.setProperty("loopLength", m_loopLength, nullptr);
    
    // Tracks
    juce::ValueTree tracks("Tracks");
    for (const auto& track : m_tracks)
    {
        tracks.addChild(track->toValueTree(), -1, nullptr);
    }
    tree.addChild(tracks, -1, nullptr);
    
    // Scenes
    tree.setProperty("sceneIndex", m_sceneIndex, nullptr);
    juce::ValueTree scenes("Scenes");
    for (int i = 0; i < 64; ++i)
    {
        if (m_scenesUsed[i])
        {
            juce::ValueTree scene("Scene");
            scene.setProperty("index", i, nullptr);
            scene.setProperty("name", m_sceneNames[i], nullptr);
            scene.setProperty("used", true, nullptr);
            scenes.addChild(scene, -1, nullptr);
        }
    }
    tree.addChild(scenes, -1, nullptr);
    
    // Morphing settings
    tree.setProperty("morphQuantization", m_morphQuantization, nullptr);
    tree.setProperty("morphInterpolation", m_morphInterpolation, nullptr);
    
    // Global modifiers
    tree.setProperty("globalSwing", m_globalSwing, nullptr);
    tree.setProperty("globalGateLength", m_globalGateLength, nullptr);
    
    return tree;
}

void Pattern::fromValueTree(const juce::ValueTree& tree)
{
    if (!tree.hasType("Pattern"))
        return;
    
    // Pattern info
    m_name = tree.getProperty("name", "New Pattern");
    m_author = tree.getProperty("author", "");
    m_description = tree.getProperty("description", "");
    
    // Timing
    m_bpm = tree.getProperty("bpm", 120.0f);
    m_timeSignatureNum = tree.getProperty("timeSignatureNum", 4);
    m_timeSignatureDenom = tree.getProperty("timeSignatureDenom", 4);
    m_loopLength = tree.getProperty("loopLength", 4);
    
    // Tracks
    m_tracks.clear();
    auto tracks = tree.getChildWithName("Tracks");
    if (tracks.isValid())
    {
        for (int i = 0; i < tracks.getNumChildren(); ++i)
        {
            auto track = std::make_unique<Track>();
            track->fromValueTree(tracks.getChild(i));
            m_tracks.push_back(std::move(track));
        }
    }
    
    // Scenes
    m_sceneIndex = tree.getProperty("sceneIndex", 0);
    
    // Reset all scenes first
    for (int i = 0; i < 64; ++i)
    {
        m_sceneNames[i] = "Scene " + juce::String(i + 1);
        m_scenesUsed[i] = false;
    }
    
    auto scenes = tree.getChildWithName("Scenes");
    if (scenes.isValid())
    {
        for (int i = 0; i < scenes.getNumChildren(); ++i)
        {
            auto scene = scenes.getChild(i);
            int index = scene.getProperty("index", -1);
            if (index >= 0 && index < 64)
            {
                m_sceneNames[index] = scene.getProperty("name", "");
                m_scenesUsed[index] = scene.getProperty("used", false);
            }
        }
    }
    
    // Morphing settings
    m_morphQuantization = tree.getProperty("morphQuantization", 0);
    m_morphInterpolation = tree.getProperty("morphInterpolation", true);
    
    // Global modifiers
    m_globalSwing = tree.getProperty("globalSwing", 50.0f);
    m_globalGateLength = tree.getProperty("globalGateLength", 1.0f);
    
    m_modified = false;
}

juce::String Pattern::toJSON() const
{
    auto tree = toValueTree();
    return juce::JSON::toString(tree.toXmlString());
}

bool Pattern::fromJSON(const juce::String& json)
{
    auto parsed = juce::JSON::parse(json);
    if (parsed.isString())
    {
        auto xml = juce::parseXML(parsed.toString());
        if (xml)
        {
            auto tree = juce::ValueTree::fromXml(*xml);
            fromValueTree(tree);
            return true;
        }
    }
    return false;
}

} // namespace HAM