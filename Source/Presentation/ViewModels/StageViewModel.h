// SPDX-License-Identifier: MIT
#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <functional>
#include <array>

namespace HAM::UI {

// ==========================================
// Stage ViewModel - UI State for a Stage
// ==========================================
class StageViewModel : public juce::ChangeBroadcaster {
public:
    StageViewModel(int stageIndex) : m_stageIndex(stageIndex) {}
    
    // Core parameters (thread-safe atomics)
    void setPitch(int pitch) {
        m_pitch = juce::jlimit(0, 127, pitch);
        sendChangeMessage();
    }
    
    void setVelocity(int velocity) {
        m_velocity = juce::jlimit(0, 127, velocity);
        sendChangeMessage();
    }
    
    void setGate(float gate) {
        m_gate = juce::jlimit(0.0f, 1.0f, gate);
        sendChangeMessage();
    }
    
    void setPulseCount(int pulseCount) {
        m_pulseCount = juce::jlimit(1, 8, pulseCount);
        sendChangeMessage();
    }
    
    void setGatePattern(const std::array<bool, 8>& pattern) {
        m_gatePattern = pattern;
        sendChangeMessage();
    }
    
    void setRatchetPattern(const std::array<int, 8>& pattern) {
        for (int i = 0; i < 8; ++i) {
            m_ratchetPattern[i] = juce::jlimit(1, 8, pattern[i]);
        }
        sendChangeMessage();
    }
    
    // Getters (thread-safe)
    int getPitch() const { return m_pitch.load(); }
    int getVelocity() const { return m_velocity.load(); }
    float getGate() const { return m_gate.load(); }
    int getPulseCount() const { return m_pulseCount.load(); }
    std::array<bool, 8> getGatePattern() const { return m_gatePattern; }
    std::array<int, 8> getRatchetPattern() const { return m_ratchetPattern; }
    int getStageIndex() const { return m_stageIndex; }
    
    // UI-specific state
    void setSelected(bool selected) {
        if (m_isSelected != selected) {
            m_isSelected = selected;
            sendChangeMessage();
        }
    }
    
    void setPlaying(bool playing) {
        if (m_isPlaying != playing) {
            m_isPlaying = playing;
            m_playingStartTime = playing ? juce::Time::currentTimeMillis() : 0;
            sendChangeMessage();
        }
    }
    
    void setActive(bool active) {
        if (m_isActive != active) {
            m_isActive = active;
            sendChangeMessage();
        }
    }
    
    void setSkipped(bool skipped) {
        if (m_isSkipped != skipped) {
            m_isSkipped = skipped;
            sendChangeMessage();
        }
    }
    
    bool isSelected() const { return m_isSelected; }
    bool isPlaying() const { return m_isPlaying; }
    bool isActive() const { return m_isActive; }
    bool isSkipped() const { return m_isSkipped; }
    int64_t getPlayingStartTime() const { return m_playingStartTime; }
    
    // Direction and mode
    enum class Direction {
        Forward,
        Backward,
        Pendulum,
        Random
    };
    
    void setDirection(Direction dir) {
        m_direction = dir;
        sendChangeMessage();
    }
    
    Direction getDirection() const { return m_direction; }
    
    // Note name helper
    juce::String getNoteName() const {
        const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
        int note = m_pitch % 12;
        int octave = (m_pitch / 12) - 2;
        return juce::String(noteNames[note]) + juce::String(octave);
    }
    
    // Accumulator value for display
    void setAccumulatorValue(int value) {
        m_accumulatorValue = value;
        sendChangeMessage();
    }
    
    int getAccumulatorValue() const { return m_accumulatorValue; }
    
    // Copy state from another stage
    void copyFrom(const StageViewModel& other) {
        m_pitch = other.m_pitch.load();
        m_velocity = other.m_velocity.load();
        m_gate = other.m_gate.load();
        m_pulseCount = other.m_pulseCount.load();
        m_gatePattern = other.m_gatePattern;
        m_ratchetPattern = other.m_ratchetPattern;
        m_direction = other.m_direction;
        sendChangeMessage();
    }
    
    // Reset to defaults
    void reset() {
        m_pitch = 60;  // Middle C
        m_velocity = 100;
        m_gate = 0.5f;
        m_pulseCount = 4;
        m_gatePattern.fill(true);
        m_ratchetPattern.fill(1);
        m_direction = Direction::Forward;
        m_accumulatorValue = 0;
        sendChangeMessage();
    }
    
private:
    const int m_stageIndex;
    
    // Core parameters (atomic for thread safety)
    std::atomic<int> m_pitch{60};
    std::atomic<int> m_velocity{100};
    std::atomic<float> m_gate{0.5f};
    std::atomic<int> m_pulseCount{4};
    
    // Patterns
    std::array<bool, 8> m_gatePattern{true, true, true, true, true, true, true, true};
    std::array<int, 8> m_ratchetPattern{1, 1, 1, 1, 1, 1, 1, 1};
    
    // UI state
    bool m_isSelected = false;
    bool m_isPlaying = false;
    bool m_isActive = false;
    bool m_isSkipped = false;
    int64_t m_playingStartTime = 0;
    
    // Additional state
    Direction m_direction = Direction::Forward;
    std::atomic<int> m_accumulatorValue{0};
};

} // namespace HAM::UI