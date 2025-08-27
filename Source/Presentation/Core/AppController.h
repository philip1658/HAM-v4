/*
  ==============================================================================

    AppController.h
    Business logic coordination and engine management for HAM sequencer

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <memory>
#include <atomic>

namespace HAM {

// Forward declarations
class HAMAudioProcessor;
class MessageDispatcher;

namespace UI {

//==============================================================================
/**
 * Manages the business logic, coordinates between engines and UI
 * Handles all non-UI logic that was previously in MainComponent
 */
class AppController : public juce::Timer
{
public:
    //==============================================================================
    AppController();
    ~AppController() override;

    //==============================================================================
    // Audio system management
    void initializeAudio();
    void shutdownAudio();
    bool isAudioInitialized() const { return m_audioInitialized; }
    
    // Plugin system management
    void initializePlugins();
    
    // Transport control
    void play();
    void stop();
    void pause();
    bool isPlaying() const { return m_isPlaying.load(); }
    void setBPM(float bpm);
    float getBPM() const { return m_currentBPM.load(); }
    
    // Pattern management
    void loadPattern(int patternIndex);
    void savePattern(int patternIndex);
    void clearPattern(int patternIndex);
    int getCurrentPatternIndex() const { return m_currentPatternIndex; }
    
    // Track management
    void addTrack();
    void removeTrack(int trackIndex);
    void setTrackMute(int trackIndex, bool muted);
    void setTrackSolo(int trackIndex, bool solo);
    void setTrackVolume(int trackIndex, float volume);
    void setTrackPan(int trackIndex, float pan);
    
    // Stage parameter updates
    void updateStageParameter(int track, int stage, const juce::String& param, float value);
    
    // Plugin management
    void loadPluginForTrack(int trackIndex, const juce::String& pluginPath);
    void removePluginFromTrack(int trackIndex);
    void showPluginEditorForTrack(int trackIndex);
    
    // Project management
    void newProject();
    void loadProject(const juce::File& file);
    void saveProject(const juce::File& file);
    bool hasUnsavedChanges() const { return m_hasUnsavedChanges; }
    
    // MIDI monitoring
    void setMidiMonitorEnabled(bool enabled);
    bool isMidiMonitorEnabled() const { return m_midiMonitorEnabled; }
    
    // Performance stats
    struct PerformanceStats
    {
        float cpuUsage = 0.0f;
        int activeVoices = 0;
        int eventsProcessed = 0;
        float audioLatency = 0.0f;
    };
    PerformanceStats getPerformanceStats() const;
    
    // Message dispatcher access
    MessageDispatcher* getMessageDispatcher() { return m_messageDispatcher; }
    
    // Audio processor access
    HAMAudioProcessor* getAudioProcessor() { return m_processor.get(); }
    
    //==============================================================================
    // Timer callback for performance monitoring
    void timerCallback() override;
    
private:
    //==============================================================================
    void setupMessageHandlers();
    void markProjectDirty();
    void updatePerformanceStats();
    
    // Audio system
    juce::AudioDeviceManager m_deviceManager;
    juce::AudioProcessorPlayer m_audioPlayer;
    std::unique_ptr<HAMAudioProcessor> m_processor;
    MessageDispatcher* m_messageDispatcher = nullptr;
    bool m_audioInitialized = false;
    
    // Transport state
    std::atomic<bool> m_isPlaying{false};
    std::atomic<float> m_currentBPM{120.0f};
    
    // Pattern state
    int m_currentPatternIndex = 0;
    bool m_hasUnsavedChanges = false;
    
    // Track states (for 8 tracks initially)
    struct TrackState
    {
        bool muted = false;
        bool solo = false;
        float volume = 0.8f;
        float pan = 0.5f;
        juce::String pluginPath;
    };
    std::array<TrackState, 8> m_trackStates;
    
    // MIDI monitoring
    bool m_midiMonitorEnabled = false;
    
    // Performance monitoring
    PerformanceStats m_performanceStats;
    std::atomic<int> m_totalEventsProcessed{0};
    
    // Project file
    juce::File m_currentProjectFile;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AppController)
};

} // namespace UI
} // namespace HAM