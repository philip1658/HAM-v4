/*
  ==============================================================================

    TrackManager.h
    Centralized track management service
    Handles track creation, deletion, and synchronization between UI views

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <vector>
#include <functional>
#include <atomic>

namespace HAM
{

/**
 * TrackManager - Central service for managing tracks across the application
 * 
 * Responsibilities:
 * - Track creation and deletion
 * - Plugin state tracking per track
 * - Synchronization between mixer and sequencer views
 * - Track parameter management
 */
class TrackManager
{
public:
    //==============================================================================
    // Track Plugin State
    struct PluginState
    {
        bool hasPlugin = false;
        juce::String pluginName;
        juce::PluginDescription description;
        bool isInstrument = true;
        bool editorOpen = false;
    };
    
    struct TrackState
    {
        int index = 0;
        juce::String name;
        juce::Colour color;
        bool isMuted = false;
        bool isSoloed = false;
        int midiChannel = 1;
        bool isPolyMode = true;
        float volume = 1.0f;
        float pan = 0.0f;
        
        // Plugin states
        PluginState instrumentPlugin;
        std::vector<PluginState> effectPlugins;
    };
    
    //==============================================================================
    // Singleton Access
    static TrackManager& getInstance()
    {
        static TrackManager instance;
        return instance;
    }
    
    // Delete copy/move constructors
    TrackManager(const TrackManager&) = delete;
    TrackManager& operator=(const TrackManager&) = delete;
    
    //==============================================================================
    // Track Management
    
    /** Add a new track */
    int addTrack(const juce::String& name = "");
    
    /** Remove a track */
    void removeTrack(int trackIndex);
    
    /** Get track state */
    TrackState* getTrack(int trackIndex);
    
    /** Get all tracks */
    const std::vector<TrackState>& getAllTracks() const { return m_tracks; }
    
    /** Get track count */
    int getTrackCount() const { return static_cast<int>(m_tracks.size()); }
    
    //==============================================================================
    // Plugin Management
    
    /** Set plugin state for a track */
    void setPluginState(int trackIndex, const PluginState& state, bool isInstrument = true);
    
    /** Get plugin state for a track */
    PluginState* getPluginState(int trackIndex, bool isInstrument = true);
    
    /** Check if track has a plugin loaded */
    bool hasPlugin(int trackIndex) const;
    
    /** Clear plugin from track */
    void clearPlugin(int trackIndex, bool isInstrument = true);
    
    //==============================================================================
    // Track Parameters
    
    /** Set track mute state */
    void setMuted(int trackIndex, bool muted);
    
    /** Set track solo state */
    void setSoloed(int trackIndex, bool soloed);
    
    /** Set track MIDI channel */
    void setMidiChannel(int trackIndex, int channel);
    
    /** Set track voice mode */
    void setPolyMode(int trackIndex, bool isPoly);
    
    /** Set track volume */
    void setVolume(int trackIndex, float volume);
    
    /** Set track pan */
    void setPan(int trackIndex, float pan);
    
    /** Set track name */
    void setTrackName(int trackIndex, const juce::String& name);
    
    //==============================================================================
    // Listeners
    
    class Listener
    {
    public:
        virtual ~Listener() = default;
        
        /** Called when a track is added */
        virtual void trackAdded(int trackIndex) {}
        
        /** Called when a track is removed */
        virtual void trackRemoved(int trackIndex) {}
        
        /** Called when track parameters change */
        virtual void trackParametersChanged(int trackIndex) {}
        
        /** Called when plugin state changes */
        virtual void trackPluginChanged(int trackIndex) {}
    };
    
    /** Add a listener */
    void addListener(Listener* listener);
    
    /** Remove a listener */
    void removeListener(Listener* listener);
    
    //==============================================================================
    // Utility
    
    /** Get track color based on index */
    static juce::Colour getTrackColor(int trackIndex);
    
    /** Generate default track name */
    static juce::String generateTrackName(int trackIndex);
    
private:
    //==============================================================================
    // Private Constructor/Destructor
    TrackManager();
    ~TrackManager() = default;
    
    //==============================================================================
    // Internal Methods
    
    void notifyTrackAdded(int trackIndex);
    void notifyTrackRemoved(int trackIndex);
    void notifyTrackParametersChanged(int trackIndex);
    void notifyTrackPluginChanged(int trackIndex);
    
    //==============================================================================
    // Internal Data
    
    std::vector<TrackState> m_tracks;
    std::vector<Listener*> m_listeners;
    std::atomic<int> m_nextTrackId{1};
    
    // Track colors palette
    static const std::array<juce::Colour, 8> s_trackColors;
};

} // namespace HAM