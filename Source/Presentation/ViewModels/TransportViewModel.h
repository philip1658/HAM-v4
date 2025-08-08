// SPDX-License-Identifier: MIT
#pragma once

#include <JuceHeader.h>
#include <atomic>

namespace HAM::UI {

// ==========================================
// Transport ViewModel - UI State for Transport
// ==========================================
class TransportViewModel : public juce::ChangeBroadcaster {
public:
    TransportViewModel() = default;
    
    // Transport state
    enum class PlayState {
        Stopped,
        Playing,
        Recording,
        Paused
    };
    
    void setPlayState(PlayState state) {
        if (m_playState != state) {
            m_playState = state;
            if (state == PlayState::Playing || state == PlayState::Recording) {
                m_playStartTime = juce::Time::currentTimeMillis();
            }
            sendChangeMessage();
        }
    }
    
    PlayState getPlayState() const { return m_playState; }
    bool isPlaying() const { return m_playState == PlayState::Playing || m_playState == PlayState::Recording; }
    bool isRecording() const { return m_playState == PlayState::Recording; }
    
    // Control methods for TransportBar
    void play() { setPlayState(PlayState::Playing); }
    void stop() { setPlayState(PlayState::Stopped); }
    float getBpm() const { return getBPM(); }  // Alias for TransportBar
    
    // Tempo
    void setBPM(float bpm) {
        m_bpm = juce::jlimit(20.0f, 999.0f, bpm);
        sendChangeMessage();
    }
    
    float getBPM() const { return m_bpm.load(); }
    
    // Swing
    void setSwing(float swing) {
        m_swing = juce::jlimit(0.0f, 1.0f, swing);
        sendChangeMessage();
    }
    
    float getSwing() const { return m_swing.load(); }
    
    // Time display
    void setCurrentBar(int bar) {
        m_currentBar = bar;
        sendChangeMessage();
    }
    
    void setCurrentBeat(int beat) {
        m_currentBeat = beat;
        sendChangeMessage();
    }
    
    void setCurrentTick(int tick) {
        m_currentTick = tick;
        // Don't send change message for tick updates (too frequent)
    }
    
    int getCurrentBar() const { return m_currentBar; }
    int getCurrentBeat() const { return m_currentBeat; }
    int getCurrentTick() const { return m_currentTick; }
    
    juce::String getTimeString() const {
        return juce::String::formatted("%03d:%02d:%03d", 
                                      m_currentBar.load() + 1,
                                      m_currentBeat.load() + 1,
                                      m_currentTick.load());
    }
    
    // Pattern info
    void setCurrentPattern(int pattern) {
        if (m_currentPattern != pattern) {
            m_currentPattern = pattern;
            sendChangeMessage();
        }
    }
    
    void setPatternLength(int length) {
        m_patternLength = juce::jlimit(1, 128, length);
        sendChangeMessage();
    }
    
    void setLoopEnabled(bool enabled) {
        if (m_loopEnabled != enabled) {
            m_loopEnabled = enabled;
            sendChangeMessage();
        }
    }
    
    int getCurrentPattern() const { return m_currentPattern; }
    int getPatternLength() const { return m_patternLength; }
    bool isLoopEnabled() const { return m_loopEnabled; }
    
    // Scene management
    void setCurrentScene(int scene) {
        if (m_currentScene != scene) {
            m_currentScene = scene;
            sendChangeMessage();
        }
    }
    
    void setNextScene(int scene) {
        m_nextScene = scene;
        sendChangeMessage();
    }
    
    void setSceneTransitionActive(bool active) {
        m_sceneTransitionActive = active;
        sendChangeMessage();
    }
    
    int getCurrentScene() const { return m_currentScene; }
    int getNextScene() const { return m_nextScene; }
    bool isSceneTransitionActive() const { return m_sceneTransitionActive; }
    
    // Sync settings
    enum class SyncMode {
        Internal,
        MidiClock,
        Ableton,
        External
    };
    
    void setSyncMode(SyncMode mode) {
        if (m_syncMode != mode) {
            m_syncMode = mode;
            sendChangeMessage();
        }
    }
    
    SyncMode getSyncMode() const { return m_syncMode; }
    
    // Metronome
    void setMetronomeEnabled(bool enabled) {
        if (m_metronomeEnabled != enabled) {
            m_metronomeEnabled = enabled;
            sendChangeMessage();
        }
    }
    
    void setMetronomeVolume(float volume) {
        m_metronomeVolume = juce::jlimit(0.0f, 1.0f, volume);
        sendChangeMessage();
    }
    
    bool isMetronomeEnabled() const { return m_metronomeEnabled; }
    float getMetronomeVolume() const { return m_metronomeVolume; }
    
    // Count-in
    void setCountInEnabled(bool enabled) {
        if (m_countInEnabled != enabled) {
            m_countInEnabled = enabled;
            sendChangeMessage();
        }
    }
    
    void setCountInBars(int bars) {
        m_countInBars = juce::jlimit(0, 4, bars);
        sendChangeMessage();
    }
    
    bool isCountInEnabled() const { return m_countInEnabled; }
    int getCountInBars() const { return m_countInBars; }
    
    // Tap tempo
    void processTapTempo() {
        int64_t now = juce::Time::currentTimeMillis();
        
        // Reset if more than 2 seconds since last tap
        if (now - m_lastTapTime > 2000) {
            m_tapCount = 0;
        }
        
        if (m_tapCount > 0) {
            // Calculate average tempo from taps
            float interval = (now - m_firstTapTime) / (float)m_tapCount;
            float bpm = 60000.0f / interval;
            setBPM(bpm);
        } else {
            m_firstTapTime = now;
        }
        
        m_lastTapTime = now;
        m_tapCount++;
        
        // Max 8 taps for averaging
        if (m_tapCount > 8) {
            m_tapCount = 1;
            m_firstTapTime = now;
        }
    }
    
    // CPU usage
    void setCpuUsage(float usage) {
        m_cpuUsage = juce::jlimit(0.0f, 100.0f, usage);
        // Don't send change message for CPU updates (too frequent)
    }
    
    float getCpuUsage() const { return m_cpuUsage.load(); }
    
    // Playback time
    int64_t getPlaybackTimeMs() const {
        if (isPlaying()) {
            return juce::Time::currentTimeMillis() - m_playStartTime;
        }
        return 0;
    }
    
    juce::String getPlaybackTimeString() const {
        int64_t ms = getPlaybackTimeMs();
        int seconds = (ms / 1000) % 60;
        int minutes = (ms / 60000) % 60;
        int hours = ms / 3600000;
        
        if (hours > 0) {
            return juce::String::formatted("%d:%02d:%02d", hours, minutes, seconds);
        }
        return juce::String::formatted("%d:%02d", minutes, seconds);
    }
    
    // Reset transport
    void reset() {
        m_currentBar = 0;
        m_currentBeat = 0;
        m_currentTick = 0;
        sendChangeMessage();
    }
    
private:
    // Transport state
    PlayState m_playState = PlayState::Stopped;
    int64_t m_playStartTime = 0;
    
    // Tempo
    std::atomic<float> m_bpm{120.0f};
    std::atomic<float> m_swing{0.0f};
    
    // Time position
    std::atomic<int> m_currentBar{0};
    std::atomic<int> m_currentBeat{0};
    std::atomic<int> m_currentTick{0};
    
    // Pattern
    int m_currentPattern = 0;
    int m_patternLength = 4;  // bars
    bool m_loopEnabled = true;
    
    // Scene
    int m_currentScene = 0;
    int m_nextScene = -1;
    bool m_sceneTransitionActive = false;
    
    // Sync
    SyncMode m_syncMode = SyncMode::Internal;
    
    // Metronome
    bool m_metronomeEnabled = false;
    float m_metronomeVolume = 0.5f;
    
    // Count-in
    bool m_countInEnabled = false;
    int m_countInBars = 1;
    
    // Tap tempo
    int64_t m_firstTapTime = 0;
    int64_t m_lastTapTime = 0;
    int m_tapCount = 0;
    
    // Performance
    std::atomic<float> m_cpuUsage{0.0f};
};

} // namespace HAM::UI