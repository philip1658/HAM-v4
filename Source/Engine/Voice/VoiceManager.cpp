/*
  ==============================================================================

    VoiceManager.cpp
    Implementation of the voice allocation and management system

  ==============================================================================
*/

#include "VoiceManager.h"
#include <algorithm>
#include <limits>

namespace HAM {

//==============================================================================
VoiceManager::VoiceManager()
{
    // Initialize voices with unique IDs
    for (size_t i = 0; i < MAX_VOICES; ++i)
    {
        m_voices[i].voiceId = static_cast<int>(i);
        m_voices[i].reset();
    }
}

VoiceManager::~VoiceManager()
{
    // Ensure all voices are stopped
    panic();
}

//==============================================================================
// Voice Mode Control

void VoiceManager::setVoiceMode(VoiceMode mode)
{
    if (m_voiceMode.exchange(mode) != mode)
    {
        // Mode changed - stop all notes for clean transition
        allNotesOff(0);
        
        // Reset mono state
        m_lastNoteNumber.store(-1);
        m_lastVoiceIndex.store(-1);
    }
}

void VoiceManager::setMaxVoices(int maxVoices)
{
    maxVoices = juce::jlimit(1, MAX_VOICES, maxVoices);
    
    int oldMax = m_maxVoices.exchange(maxVoices);
    
    // If reducing voices, stop any voices beyond the new limit
    if (maxVoices < oldMax)
    {
        for (int i = maxVoices; i < oldMax; ++i)
        {
            voiceAt(i).stopNote();
        }
    }
}

//==============================================================================
// Note Management

int VoiceManager::noteOn(int noteNumber, int velocity, int channel)
{
    if (velocity == 0)
    {
        // Velocity 0 is treated as note off
        noteOff(noteNumber, channel);
        return -1;
    }
    
    // Update statistics
    m_statistics.totalNotesPlayed.fetch_add(1);
    
    // Route based on voice mode
    VoiceMode mode = m_voiceMode.load();
    
    switch (mode)
    {
        case VoiceMode::MONO:
        case VoiceMode::MONO_LEGATO:
        case VoiceMode::MONO_RETRIG:
            return handleMonoNoteOn(noteNumber, velocity, channel);
            
        case VoiceMode::POLY:
            return handlePolyNoteOn(noteNumber, velocity, channel);
            
        case VoiceMode::UNISON:
            // TODO: Implement unison mode (all voices play same note)
            return handlePolyNoteOn(noteNumber, velocity, channel);
            
        default:
            return handlePolyNoteOn(noteNumber, velocity, channel);
    }
}

void VoiceManager::noteOff(int noteNumber, int channel)
{
    // In mono mode, only stop if it's the current note
    VoiceMode mode = m_voiceMode.load();
    
    if (mode == VoiceMode::MONO || 
        mode == VoiceMode::MONO_LEGATO || 
        mode == VoiceMode::MONO_RETRIG)
    {
        if (m_lastNoteNumber.load() == noteNumber)
        {
            int voiceIndex = m_lastVoiceIndex.load();
            if (voiceIndex >= 0 && voiceIndex < MAX_VOICES)
            {
                voiceAt(voiceIndex).stopNote();
                m_lastNoteNumber.store(-1);
                m_lastVoiceIndex.store(-1);
            }
        }
        return;
    }
    
    // Poly mode - find and stop all matching notes
    for (int i = 0; i < m_maxVoices.load(); ++i)
    {
        if (voiceAt(i).active.load() &&
            voiceAt(i).noteNumber.load() == noteNumber &&
            (channel == 0 || voiceAt(i).channel.load() == channel))
        {
            voiceAt(i).stopNote();
        }
    }
    
    updateStatistics();
}

void VoiceManager::allNotesOff(int channel)
{
    for (int i = 0; i < MAX_VOICES; ++i)
    {
        if (voiceAt(i).active.load() &&
            (channel == 0 || voiceAt(i).channel.load() == channel))
        {
            voiceAt(i).stopNote();
        }
    }
    
    // Reset mono state
    if (channel == 0)
    {
        m_lastNoteNumber.store(-1);
        m_lastVoiceIndex.store(-1);
    }
    
    updateStatistics();
}

void VoiceManager::panic()
{
    // Immediately reset all voices
    for (int i = 0; i < MAX_VOICES; ++i)
    {
        voiceAt(i).reset();
    }
    
    m_lastNoteNumber.store(-1);
    m_lastVoiceIndex.store(-1);
    
    updateStatistics();
}

//==============================================================================
// Voice Query

VoiceManager::Voice* VoiceManager::getVoice(int index)
{
    if (index >= 0 && index < MAX_VOICES)
        return &voiceAt(index);
    return nullptr;
}

const VoiceManager::Voice* VoiceManager::getVoice(int index) const
{
    if (index >= 0 && index < MAX_VOICES)
        return &voiceAt(index);
    return nullptr;
}

VoiceManager::Voice* VoiceManager::findVoiceForNote(int noteNumber, int channel)
{
    for (int i = 0; i < m_maxVoices.load(); ++i)
    {
        if (voiceAt(i).active.load() &&
            voiceAt(i).noteNumber.load() == noteNumber &&
            (channel == 0 || voiceAt(i).channel.load() == channel))
        {
            return &voiceAt(i);
        }
    }
    return nullptr;
}

int VoiceManager::getActiveVoiceCount() const
{
    int count = 0;
    for (int i = 0; i < m_maxVoices.load(); ++i)
    {
        if (voiceAt(i).active.load())
            count++;
    }
    return count;
}

std::vector<VoiceManager::Voice*> VoiceManager::getActiveVoices()
{
    std::vector<Voice*> activeVoices;
    activeVoices.reserve(static_cast<size_t>(m_maxVoices.load()));
    
    for (int i = 0; i < m_maxVoices.load(); ++i)
    {
        if (voiceAt(i).active.load())
            activeVoices.push_back(&voiceAt(i));
    }
    
    return activeVoices;
}

bool VoiceManager::isNotePlaying(int noteNumber, int channel) const
{
    for (int i = 0; i < m_maxVoices.load(); ++i)
    {
        if (voiceAt(i).active.load() &&
            voiceAt(i).noteNumber.load() == noteNumber &&
            (channel == 0 || voiceAt(i).channel.load() == channel))
        {
            return true;
        }
    }
    return false;
}

//==============================================================================
// MPE Support

void VoiceManager::setPitchBend(int voiceId, float bend)
{
    if (voiceId >= 0 && voiceId < MAX_VOICES)
    {
        voiceAt(voiceId).pitchBend.store(bend);
    }
}

void VoiceManager::setPressure(int voiceId, float pressure)
{
    if (voiceId >= 0 && voiceId < MAX_VOICES)
    {
        voiceAt(voiceId).pressure.store(pressure);
    }
}

void VoiceManager::setSlide(int voiceId, float slide)
{
    if (voiceId >= 0 && voiceId < MAX_VOICES)
    {
        voiceAt(voiceId).slide.store(slide);
    }
}

//==============================================================================
// Real-time Safe Operations

void VoiceManager::processVoices()
{
    // Update statistics
    updateStatistics();
    
    // Could do voice-specific processing here
    // For now, just update stats
}

//==============================================================================
// Internal Methods

int VoiceManager::findFreeVoice()
{
    int maxVoices = m_maxVoices.load();
    
    for (int i = 0; i < maxVoices; ++i)
    {
        if (!voiceAt(i).active.load())
            return i;
    }
    
    return -1;  // No free voice
}

int VoiceManager::stealVoice()
{
    StealingMode mode = m_stealingMode.load();
    
    switch (mode)
    {
        case StealingMode::OLDEST:
            return findOldestVoice();
            
        case StealingMode::LOWEST:
            return findLowestVoice();
            
        case StealingMode::HIGHEST:
            return findHighestVoice();
            
        case StealingMode::QUIETEST:
            return findQuietestVoice();
            
        case StealingMode::NONE:
        default:
            return -1;  // Don't steal
    }
}

int VoiceManager::findOldestVoice()
{
    int oldestIndex = -1;
    int64_t oldestTime = std::numeric_limits<int64_t>::max();
    int maxVoices = m_maxVoices.load();
    
    for (int i = 0; i < maxVoices; ++i)
    {
        if (voiceAt(i).active.load())
        {
            int64_t startTime = voiceAt(i).startTime.load();
            if (startTime < oldestTime)
            {
                oldestTime = startTime;
                oldestIndex = i;
            }
        }
    }
    
    return oldestIndex;
}

int VoiceManager::findLowestVoice()
{
    int lowestIndex = -1;
    int lowestNote = 128;
    int maxVoices = m_maxVoices.load();
    
    for (int i = 0; i < maxVoices; ++i)
    {
        if (voiceAt(i).active.load())
        {
            int note = voiceAt(i).noteNumber.load();
            if (note < lowestNote)
            {
                lowestNote = note;
                lowestIndex = i;
            }
        }
    }
    
    return lowestIndex;
}

int VoiceManager::findHighestVoice()
{
    int highestIndex = -1;
    int highestNote = -1;
    int maxVoices = m_maxVoices.load();
    
    for (int i = 0; i < maxVoices; ++i)
    {
        if (voiceAt(i).active.load())
        {
            int note = voiceAt(i).noteNumber.load();
            if (note > highestNote)
            {
                highestNote = note;
                highestIndex = i;
            }
        }
    }
    
    return highestIndex;
}

int VoiceManager::findQuietestVoice()
{
    int quietestIndex = -1;
    int quietestVelocity = 128;
    int maxVoices = m_maxVoices.load();
    
    for (int i = 0; i < maxVoices; ++i)
    {
        if (voiceAt(i).active.load())
        {
            int velocity = voiceAt(i).velocity.load();
            if (velocity < quietestVelocity)
            {
                quietestVelocity = velocity;
                quietestIndex = i;
            }
        }
    }
    
    return quietestIndex;
}

void VoiceManager::updateStatistics()
{
    int activeCount = getActiveVoiceCount();
    m_statistics.activeVoices.store(activeCount);
    
    // Update peak count
    int peak = m_statistics.peakVoiceCount.load();
    if (activeCount > peak)
    {
        m_statistics.peakVoiceCount.store(activeCount);
    }
}

int VoiceManager::handleMonoNoteOn(int noteNumber, int velocity, int channel)
{
    VoiceMode mode = m_voiceMode.load();
    
    // Check if we need to retrigger
    bool shouldRetrigger = (mode == VoiceMode::MONO || mode == VoiceMode::MONO_RETRIG);
    bool isLegato = (mode == VoiceMode::MONO_LEGATO) && (m_lastNoteNumber.load() >= 0);
    
    // Get voice index
    int voiceIndex = 0;  // Always use first voice for mono
    
    if (isLegato && !shouldRetrigger)
    {
        // Legato mode - just change the pitch, don't retrigger
        voiceAt(voiceIndex).noteNumber.store(noteNumber);
        voiceAt(voiceIndex).velocity.store(velocity);
        // Don't reset start time for legato
    }
    else
    {
        // Stop previous note if playing
        if (voiceAt(voiceIndex).active.load())
        {
            voiceAt(voiceIndex).stopNote();
        }
        
        // Start new note
        voiceAt(voiceIndex).startNote(noteNumber, velocity, channel);
    }
    
    // Update mono state
    m_lastNoteNumber.store(noteNumber);
    m_lastVoiceIndex.store(voiceIndex);
    
    updateStatistics();
    return voiceIndex;
}

int VoiceManager::handlePolyNoteOn(int noteNumber, int velocity, int channel)
{
    // Find a free voice
    int voiceIndex = findFreeVoice();
    
    if (voiceIndex < 0)
    {
        // No free voice - try to steal one
        voiceIndex = stealVoice();
        
        if (voiceIndex < 0)
        {
            // Can't steal - ignore note
            return -1;
        }
        
        // Stop the stolen voice
        voiceAt(voiceIndex).stopNote();
        m_statistics.notesStolen.fetch_add(1);
    }
    
    // Start the new note
    voiceAt(voiceIndex).startNote(noteNumber, velocity, channel);
    
    updateStatistics();
    return voiceIndex;
}

} // namespace HAM