// SPDX-License-Identifier: MIT
#pragma once

#include <JuceHeader.h>
#include "StageViewModel.h"
#include "../Core/DesignSystem.h"
#include <memory>
#include <array>

namespace HAM::UI {

// ==========================================
// Track ViewModel - UI State for a Track
// ==========================================
class TrackViewModel : public juce::ChangeBroadcaster {
public:
    TrackViewModel(int trackIndex) 
        : m_trackIndex(trackIndex),
          m_trackColor(DesignSystem::Colors::getTrackColor(trackIndex)) {
        
        // Initialize 8 stages
        for (int i = 0; i < 8; ++i) {
            m_stages[i] = std::make_unique<StageViewModel>(i);
        }
    }
    
    // Track properties
    void setName(const juce::String& name) {
        m_name = name;
        sendChangeMessage();
    }
    
    void setMuted(bool muted) {
        if (m_isMuted != muted) {
            m_isMuted = muted;
            sendChangeMessage();
        }
    }
    
    void setSoloed(bool soloed) {
        if (m_isSoloed != soloed) {
            m_isSoloed = soloed;
            sendChangeMessage();
        }
    }
    
    void setVolume(float volume) {
        m_volume = juce::jlimit(0.0f, 1.0f, volume);
        sendChangeMessage();
    }
    
    void setPan(float pan) {
        m_pan = juce::jlimit(-1.0f, 1.0f, pan);
        sendChangeMessage();
    }
    
    // Voice mode
    enum class VoiceMode {
        Mono,
        Poly
    };
    
    void setVoiceMode(VoiceMode mode) {
        if (m_voiceMode != mode) {
            m_voiceMode = mode;
            sendChangeMessage();
        }
    }
    
    // Clock division
    enum class Division {
        Whole = 1,
        Half = 2,
        Quarter = 4,
        Eighth = 8,
        Sixteenth = 16,
        ThirtySecond = 32,
        Triplet = 6,
        Quintuplet = 5,
        Septuplet = 7
    };
    
    void setDivision(Division div) {
        if (m_division != div) {
            m_division = div;
            sendChangeMessage();
        }
    }
    
    void setSwing(float swing) {
        m_swing = juce::jlimit(0.0f, 1.0f, swing);
        sendChangeMessage();
    }
    
    // MIDI settings
    void setMidiChannel(int channel) {
        m_midiChannel = juce::jlimit(1, 16, channel);
        sendChangeMessage();
    }
    
    // MIDI Routing
    enum class MidiRoutingMode {
        PluginOnly,    // Send MIDI only to internal plugins
        ExternalOnly,  // Send MIDI only to external MIDI devices
        Both          // Send MIDI to both plugins and external devices
    };
    
    void setMidiRoutingMode(MidiRoutingMode mode) {
        if (m_midiRoutingMode != mode) {
            m_midiRoutingMode = mode;
            sendChangeMessage();
        }
    }
    
    void setOctaveOffset(int offset) {
        m_octaveOffset = juce::jlimit(-4, 4, offset);
        sendChangeMessage();
    }
    
    // Pattern settings
    void setPatternLength(int length) {
        m_patternLength = juce::jlimit(1, 8, length);
        sendChangeMessage();
    }
    
    void setCurrentStageIndex(int index) {
        if (index >= 0 && index < 8 && m_currentStageIndex != index) {
            // Update active state on stages
            if (m_currentStageIndex >= 0) {
                m_stages[m_currentStageIndex]->setActive(false);
            }
            m_currentStageIndex = index;
            m_stages[m_currentStageIndex]->setActive(true);
            sendChangeMessage();
        }
    }
    
    // Plugin state
    void setHasPlugin(bool hasPlugin) {
        m_hasPlugin = hasPlugin;
        sendChangeMessage();
    }
    
    void setPluginName(const juce::String& name) {
        m_pluginName = name;
        sendChangeMessage();
    }
    
    // Selection state
    void setSelected(bool selected) {
        if (m_isSelected != selected) {
            m_isSelected = selected;
            sendChangeMessage();
        }
    }
    
    void setExpanded(bool expanded) {
        if (m_isExpanded != expanded) {
            m_isExpanded = expanded;
            sendChangeMessage();
        }
    }
    
    // Getters
    juce::String getName() const { return m_name; }
    bool isMuted() const { return m_isMuted; }
    bool isSoloed() const { return m_isSoloed; }
    float getVolume() const { return m_volume.load(); }
    float getPan() const { return m_pan.load(); }
    VoiceMode getVoiceMode() const { return m_voiceMode; }
    Division getDivision() const { return m_division; }
    float getSwing() const { return m_swing.load(); }
    int getMidiChannel() const { return m_midiChannel; }
    MidiRoutingMode getMidiRoutingMode() const { return m_midiRoutingMode; }
    int getOctaveOffset() const { return m_octaveOffset; }
    int getPatternLength() const { return m_patternLength; }
    int getCurrentStageIndex() const { return m_currentStageIndex; }
    bool hasPlugin() const { return m_hasPlugin; }
    juce::String getPluginName() const { return m_pluginName; }
    bool isSelected() const { return m_isSelected; }
    bool isExpanded() const { return m_isExpanded; }
    juce::Colour getTrackColor() const { return m_trackColor; }
    int getTrackIndex() const { return m_trackIndex; }
    
    // Stage access
    StageViewModel* getStage(int index) {
        if (index >= 0 && index < 8) {
            return m_stages[index].get();
        }
        return nullptr;
    }
    
    const StageViewModel* getStage(int index) const {
        if (index >= 0 && index < 8) {
            return m_stages[index].get();
        }
        return nullptr;
    }
    
    // Activity monitoring
    void setActivity(float level) {
        m_activityLevel = juce::jlimit(0.0f, 1.0f, level);
        m_lastActivityTime = juce::Time::currentTimeMillis();
    }
    
    float getActivity() const {
        // Decay activity over time
        int64_t timeSinceActivity = juce::Time::currentTimeMillis() - m_lastActivityTime;
        float decay = juce::jmax(0.0f, 1.0f - (timeSinceActivity / 1000.0f));
        return m_activityLevel * decay;
    }
    
    // Copy settings from another track
    void copyFrom(const TrackViewModel& other) {
        m_name = other.m_name;
        m_voiceMode = other.m_voiceMode;
        m_division = other.m_division;
        m_swing = other.m_swing.load();
        m_octaveOffset = other.m_octaveOffset;
        m_patternLength = other.m_patternLength;
        
        // Copy stage data
        for (int i = 0; i < 8; ++i) {
            m_stages[i]->copyFrom(*other.m_stages[i]);
        }
        
        sendChangeMessage();
    }
    
    // Reset to defaults
    void reset() {
        m_name = "Track " + juce::String(m_trackIndex + 1);
        m_isMuted = false;
        m_isSoloed = false;
        m_volume = 0.75f;
        m_pan = 0.0f;
        m_voiceMode = VoiceMode::Mono;
        m_division = Division::Quarter;
        m_swing = 0.0f;
        m_midiChannel = m_trackIndex + 1;
        m_midiRoutingMode = MidiRoutingMode::PluginOnly;
        m_octaveOffset = 0;
        m_patternLength = 8;
        m_currentStageIndex = 0;
        
        for (auto& stage : m_stages) {
            stage->reset();
        }
        
        sendChangeMessage();
    }
    
private:
    const int m_trackIndex;
    const juce::Colour m_trackColor;
    
    // Track properties
    juce::String m_name{"Track"};
    bool m_isMuted = false;
    bool m_isSoloed = false;
    std::atomic<float> m_volume{0.75f};
    std::atomic<float> m_pan{0.0f};
    
    // Sequencer settings
    VoiceMode m_voiceMode = VoiceMode::Mono;
    Division m_division = Division::Quarter;
    std::atomic<float> m_swing{0.0f};
    int m_midiChannel = 1;
    MidiRoutingMode m_midiRoutingMode = MidiRoutingMode::PluginOnly;
    int m_octaveOffset = 0;
    int m_patternLength = 8;
    int m_currentStageIndex = -1;
    
    // Plugin state
    bool m_hasPlugin = false;
    juce::String m_pluginName;
    
    // UI state
    bool m_isSelected = false;
    bool m_isExpanded = false;
    
    // Activity monitoring
    std::atomic<float> m_activityLevel{0.0f};
    int64_t m_lastActivityTime = 0;
    
    // Stages
    std::array<std::unique_ptr<StageViewModel>, 8> m_stages;
};

} // namespace HAM::UI