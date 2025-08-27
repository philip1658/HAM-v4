/*
  ==============================================================================

    TrackManager.cpp
    Implementation of centralized track management service

  ==============================================================================
*/

#include "TrackManager.h"

namespace HAM
{

// Define the track color palette
const std::array<juce::Colour, 8> TrackManager::s_trackColors = {
    juce::Colour(0xff00ffaa),  // Mint
    juce::Colour(0xff00aaff),  // Cyan  
    juce::Colour(0xffff00aa),  // Magenta
    juce::Colour(0xffffaa00),  // Orange
    juce::Colour(0xffaa00ff),  // Purple
    juce::Colour(0xff00ff00),  // Green
    juce::Colour(0xffff0055),  // Red
    juce::Colour(0xff55aaff)   // Light Blue
};

//==============================================================================
TrackManager::TrackManager()
{
    // Initialize with 1 default track
    for (int i = 0; i < 1; ++i)
    {
        TrackState track;
        track.index = i;
        track.name = generateTrackName(i);
        track.color = getTrackColor(i);
        track.midiChannel = (i % 16) + 1;  // Channels 1-16
        track.isPolyMode = true;
        track.volume = 1.0f;
        track.pan = 0.0f;
        
        m_tracks.push_back(track);
    }
}

//==============================================================================
int TrackManager::addTrack(const juce::String& name)
{
    int newIndex = static_cast<int>(m_tracks.size());
    
    TrackState track;
    track.index = newIndex;
    track.name = name.isEmpty() ? generateTrackName(newIndex) : name;
    track.color = getTrackColor(newIndex);
    track.midiChannel = ((newIndex % 16) + 1);
    track.isPolyMode = true;
    track.volume = 1.0f;
    track.pan = 0.0f;
    
    m_tracks.push_back(track);
    
    notifyTrackAdded(newIndex);
    
    return newIndex;
}

void TrackManager::removeTrack(int trackIndex)
{
    if (trackIndex < 0 || trackIndex >= static_cast<int>(m_tracks.size()))
        return;
    
    // Don't allow removing the last track
    if (m_tracks.size() <= 1)
        return;
    
    m_tracks.erase(m_tracks.begin() + trackIndex);
    
    // Update indices for remaining tracks
    for (size_t i = trackIndex; i < m_tracks.size(); ++i)
    {
        m_tracks[i].index = static_cast<int>(i);
    }
    
    notifyTrackRemoved(trackIndex);
}

TrackManager::TrackState* TrackManager::getTrack(int trackIndex)
{
    if (trackIndex >= 0 && trackIndex < static_cast<int>(m_tracks.size()))
        return &m_tracks[trackIndex];
    
    return nullptr;
}

//==============================================================================
void TrackManager::setPluginState(int trackIndex, const PluginState& state, bool isInstrument)
{
    auto* track = getTrack(trackIndex);
    if (!track)
        return;
    
    if (isInstrument)
    {
        track->instrumentPlugin = state;
    }
    else
    {
        // For effects, add to the list or update existing
        // For simplicity, we'll just replace the first effect slot for now
        if (track->effectPlugins.empty())
            track->effectPlugins.push_back(state);
        else
            track->effectPlugins[0] = state;
    }
    
    notifyTrackPluginChanged(trackIndex);
}

TrackManager::PluginState* TrackManager::getPluginState(int trackIndex, bool isInstrument)
{
    auto* track = getTrack(trackIndex);
    if (!track)
        return nullptr;
    
    if (isInstrument)
    {
        return &track->instrumentPlugin;
    }
    else if (!track->effectPlugins.empty())
    {
        return &track->effectPlugins[0];
    }
    
    return nullptr;
}

bool TrackManager::hasPlugin(int trackIndex) const
{
    if (trackIndex >= 0 && trackIndex < static_cast<int>(m_tracks.size()))
    {
        return m_tracks[trackIndex].instrumentPlugin.hasPlugin;
    }
    return false;
}

void TrackManager::clearPlugin(int trackIndex, bool isInstrument)
{
    auto* track = getTrack(trackIndex);
    if (!track)
        return;
    
    if (isInstrument)
    {
        track->instrumentPlugin = PluginState();
    }
    else
    {
        track->effectPlugins.clear();
    }
    
    notifyTrackPluginChanged(trackIndex);
}

//==============================================================================
void TrackManager::setMuted(int trackIndex, bool muted)
{
    auto* track = getTrack(trackIndex);
    if (!track)
        return;
    
    track->isMuted = muted;
    notifyTrackParametersChanged(trackIndex);
}

void TrackManager::setSoloed(int trackIndex, bool soloed)
{
    auto* track = getTrack(trackIndex);
    if (!track)
        return;
    
    track->isSoloed = soloed;
    notifyTrackParametersChanged(trackIndex);
}

void TrackManager::setMidiChannel(int trackIndex, int channel)
{
    auto* track = getTrack(trackIndex);
    if (!track)
        return;
    
    track->midiChannel = juce::jlimit(1, 16, channel);
    notifyTrackParametersChanged(trackIndex);
}

void TrackManager::setPolyMode(int trackIndex, bool isPoly)
{
    auto* track = getTrack(trackIndex);
    if (!track)
        return;
    
    track->isPolyMode = isPoly;
    notifyTrackParametersChanged(trackIndex);
}

void TrackManager::setVolume(int trackIndex, float volume)
{
    auto* track = getTrack(trackIndex);
    if (!track)
        return;
    
    track->volume = juce::jlimit(0.0f, 1.0f, volume);
    notifyTrackParametersChanged(trackIndex);
}

void TrackManager::setPan(int trackIndex, float pan)
{
    auto* track = getTrack(trackIndex);
    if (!track)
        return;
    
    track->pan = juce::jlimit(-1.0f, 1.0f, pan);
    notifyTrackParametersChanged(trackIndex);
}

void TrackManager::setTrackName(int trackIndex, const juce::String& name)
{
    auto* track = getTrack(trackIndex);
    if (!track)
        return;
    
    track->name = name;
    notifyTrackParametersChanged(trackIndex);
}

//==============================================================================
void TrackManager::addListener(Listener* listener)
{
    if (listener && std::find(m_listeners.begin(), m_listeners.end(), listener) == m_listeners.end())
    {
        m_listeners.push_back(listener);
    }
}

void TrackManager::removeListener(Listener* listener)
{
    auto it = std::find(m_listeners.begin(), m_listeners.end(), listener);
    if (it != m_listeners.end())
    {
        m_listeners.erase(it);
    }
}

//==============================================================================
juce::Colour TrackManager::getTrackColor(int trackIndex)
{
    return s_trackColors[trackIndex % s_trackColors.size()];
}

juce::String TrackManager::generateTrackName(int trackIndex)
{
    return "Track " + juce::String(trackIndex + 1);
}

//==============================================================================
void TrackManager::notifyTrackAdded(int trackIndex)
{
    for (auto* listener : m_listeners)
    {
        listener->trackAdded(trackIndex);
    }
}

void TrackManager::notifyTrackRemoved(int trackIndex)
{
    for (auto* listener : m_listeners)
    {
        listener->trackRemoved(trackIndex);
    }
}

void TrackManager::notifyTrackParametersChanged(int trackIndex)
{
    for (auto* listener : m_listeners)
    {
        listener->trackParametersChanged(trackIndex);
    }
}

void TrackManager::notifyTrackPluginChanged(int trackIndex)
{
    for (auto* listener : m_listeners)
    {
        listener->trackPluginChanged(trackIndex);
    }
}

} // namespace HAM