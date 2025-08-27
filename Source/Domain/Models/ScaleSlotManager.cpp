/*
  ==============================================================================

    ScaleSlotManager.cpp
    Implementation of global scale slot management

  ==============================================================================
*/

#include "ScaleSlotManager.h"
#include <cmath>

namespace HAM {

//==============================================================================
// Singleton implementation

ScaleSlotManager& ScaleSlotManager::getInstance()
{
    static ScaleSlotManager instance;
    return instance;
}

ScaleSlotManager::ScaleSlotManager()
{
    initializeDefaults();
}

//==============================================================================
// Initialization

void ScaleSlotManager::initializeDefaults()
{
    // Initialize with common scales (like Metropolix defaults)
    m_slots[0] = ScaleSlot(Scale::Chromatic, "Chromatic");
    m_slots[1] = ScaleSlot(Scale::Major, "Major");
    m_slots[2] = ScaleSlot(Scale::Minor, "Minor");
    m_slots[3] = ScaleSlot(Scale::Dorian, "Dorian");
    m_slots[4] = ScaleSlot(Scale::Mixolydian, "Mixolydian");
    m_slots[5] = ScaleSlot(Scale::PentatonicMajor, "Pent Major");
    m_slots[6] = ScaleSlot(Scale::PentatonicMinor, "Pent Minor");
    m_slots[7] = ScaleSlot(Scale::Blues, "Blues");
    
    // Default to Major scale
    m_activeSlot = 1;
    m_globalRoot = 0;  // C
}

//==============================================================================
// Scale slot management

void ScaleSlotManager::setSlot(int slotIndex, const Scale& scale, const juce::String& name)
{
    if (slotIndex < 0 || slotIndex >= 8)
        return;
    
    {
        juce::ScopedWriteLock lock(m_slotLock);
        m_slots[slotIndex].scale = scale;
        m_slots[slotIndex].displayName = name;
        m_slots[slotIndex].rootNote = -1;  // Use global root
        m_slots[slotIndex].isUserScale = false;
    }
}

void ScaleSlotManager::setUserSlot(int slotIndex, const Scale& scale, int rootNote, const juce::String& name)
{
    if (slotIndex < 0 || slotIndex >= 8)
        return;
    
    {
        juce::ScopedWriteLock lock(m_slotLock);
        m_slots[slotIndex].scale = scale;
        m_slots[slotIndex].displayName = name;
        m_slots[slotIndex].rootNote = juce::jlimit(0, 11, rootNote);
        m_slots[slotIndex].isUserScale = true;
    }
}

const ScaleSlot& ScaleSlotManager::getSlot(int slotIndex) const
{
    juce::ScopedReadLock lock(m_slotLock);
    slotIndex = juce::jlimit(0, 7, slotIndex);
    return m_slots[slotIndex];
}

const Scale& ScaleSlotManager::getActiveScale() const
{
    return getSlot(m_activeSlot.load()).scale;
}

int ScaleSlotManager::getEffectiveRoot(int trackRootOffset) const
{
    int activeIdx = m_activeSlot.load();
    const ScaleSlot& slot = getSlot(activeIdx);
    
    // If slot has specific root (user scale), use it; otherwise use global
    int baseRoot = (slot.rootNote >= 0) ? slot.rootNote : m_globalRoot.load();
    
    // Apply track offset and wrap to 0-11
    int effectiveRoot = (baseRoot + trackRootOffset + 144) % 12;  // +144 ensures positive
    return effectiveRoot;
}

//==============================================================================
// Scale switching

void ScaleSlotManager::selectSlot(int slotIndex)
{
    if (slotIndex < 0 || slotIndex >= 8)
        return;
    
    // Mark pending change for bar-quantized switching
    m_pendingSlot = slotIndex;
    m_pendingChange = true;
    
    // Notify listeners immediately for UI update
    notifySlotSelected(slotIndex);
}

void ScaleSlotManager::executePendingChange()
{
    if (!m_pendingChange.load())
        return;
    
    int newSlot = m_pendingSlot.load();
    m_activeSlot = newSlot;
    m_pendingChange = false;
    
    // Notify listeners of actual change
    notifyScaleChanged(newSlot);
}

void ScaleSlotManager::cancelPendingChange()
{
    m_pendingChange = false;
    m_pendingSlot = m_activeSlot.load();
}

//==============================================================================
// Global root control

void ScaleSlotManager::setGlobalRoot(int rootNote)
{
    rootNote = juce::jlimit(0, 11, rootNote);
    m_globalRoot = rootNote;
    notifyGlobalRootChanged(rootNote);
}

//==============================================================================
// Serialization

juce::ValueTree ScaleSlotManager::toValueTree() const
{
    juce::ValueTree tree("ScaleSlotManager");
    
    tree.setProperty("activeSlot", m_activeSlot.load(), nullptr);
    tree.setProperty("globalRoot", m_globalRoot.load(), nullptr);
    
    // Save all slots
    juce::ValueTree slotsTree("Slots");
    for (int i = 0; i < 8; ++i)
    {
        juce::ValueTree slotTree("Slot");
        slotTree.setProperty("index", i, nullptr);
        slotTree.setProperty("displayName", m_slots[i].displayName, nullptr);
        slotTree.setProperty("rootNote", m_slots[i].rootNote, nullptr);
        slotTree.setProperty("isUserScale", m_slots[i].isUserScale, nullptr);
        
        // Add scale data
        slotTree.addChild(m_slots[i].scale.toValueTree(), -1, nullptr);
        slotsTree.addChild(slotTree, -1, nullptr);
    }
    tree.addChild(slotsTree, -1, nullptr);
    
    return tree;
}

void ScaleSlotManager::fromValueTree(const juce::ValueTree& tree)
{
    if (!tree.hasType("ScaleSlotManager"))
        return;
    
    m_activeSlot = tree.getProperty("activeSlot", 1);
    m_globalRoot = tree.getProperty("globalRoot", 0);
    
    auto slotsTree = tree.getChildWithName("Slots");
    if (slotsTree.isValid())
    {
        for (int i = 0; i < slotsTree.getNumChildren() && i < 8; ++i)
        {
            auto slotTree = slotsTree.getChild(i);
            int index = slotTree.getProperty("index", i);
            
            if (index >= 0 && index < 8)
            {
                m_slots[index].displayName = slotTree.getProperty("displayName", "");
                m_slots[index].rootNote = slotTree.getProperty("rootNote", -1);
                m_slots[index].isUserScale = slotTree.getProperty("isUserScale", false);
                
                // Load scale data
                auto scaleTree = slotTree.getChildWithName("Scale");
                if (scaleTree.isValid())
                {
                    m_slots[index].scale.fromValueTree(scaleTree);
                }
            }
        }
    }
}

//==============================================================================
// Listeners

void ScaleSlotManager::addListener(Listener* listener)
{
    m_listeners.add(listener);
}

void ScaleSlotManager::removeListener(Listener* listener)
{
    m_listeners.remove(listener);
}

void ScaleSlotManager::notifySlotSelected(int slotIndex)
{
    m_listeners.call(&Listener::scaleSlotSelected, slotIndex);
}

void ScaleSlotManager::notifyScaleChanged(int slotIndex)
{
    m_listeners.call(&Listener::scaleChanged, slotIndex);
}

void ScaleSlotManager::notifyGlobalRootChanged(int rootNote)
{
    m_listeners.call(&Listener::globalRootChanged, rootNote);
}

//==============================================================================
// ScaleDegreeMapper implementation

int ScaleDegreeMapper::degreeToMidiNote(int scaleDegree, const Scale& scale, 
                                        int effectiveRoot, int baseOctave)
{
    int scaleSize = scale.getSize();
    if (scaleSize == 0)
        return 60;  // Default to C4 if scale is empty
    
    // Calculate octave offset and degree within octave
    int octaveOffset = getDegreeOctave(scaleDegree, scaleSize);
    int degreeInOctave = getDegreeinOctave(scaleDegree, scaleSize);
    
    // Get intervals from scale
    const auto& intervals = scale.getIntervals();
    if (degreeInOctave >= intervals.size())
        return 60;  // Safety check
    
    // Calculate MIDI note
    int semitones = intervals[degreeInOctave] + (12 * octaveOffset);
    int midiNote = (baseOctave * 12) + effectiveRoot + semitones;
    
    // Clamp to valid MIDI range
    return juce::jlimit(0, 127, midiNote);
}

int ScaleDegreeMapper::midiNoteToDegree(int midiNote, const Scale& scale, 
                                        int effectiveRoot, int baseOctave)
{
    // Calculate chromatic distance from base
    int baseNote = (baseOctave * 12) + effectiveRoot;
    int chromaticDistance = midiNote - baseNote;
    
    // Find octave and semitone within octave
    int octave = chromaticDistance / 12;
    int semitoneInOctave = ((chromaticDistance % 12) + 12) % 12;
    
    // Find closest scale degree
    const auto& intervals = scale.getIntervals();
    int closestDegree = 0;
    int minDistance = 12;
    
    for (int i = 0; i < intervals.size(); ++i)
    {
        int distance = std::abs(intervals[i] - semitoneInOctave);
        if (distance < minDistance)
        {
            minDistance = distance;
            closestDegree = i;
        }
    }
    
    // Return total degree (octave * scaleSize + degree)
    return (octave * scale.getSize()) + closestDegree;
}

int ScaleDegreeMapper::getDegreeOctave(int scaleDegree, int scaleSize)
{
    if (scaleSize <= 0)
        return 0;
    
    if (scaleDegree >= 0)
        return scaleDegree / scaleSize;
    else
        return -(((-scaleDegree - 1) / scaleSize) + 1);
}

int ScaleDegreeMapper::getDegreeinOctave(int scaleDegree, int scaleSize)
{
    if (scaleSize <= 0)
        return 0;
    
    int degree = scaleDegree % scaleSize;
    if (degree < 0)
        degree += scaleSize;
    
    return degree;
}

} // namespace HAM