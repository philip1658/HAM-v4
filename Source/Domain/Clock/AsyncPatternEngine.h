/*
  ==============================================================================

    AsyncPatternEngine.h
    Asynchronous pattern switching engine for seamless scene changes

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "MasterClock.h"
#include "../Models/Pattern.h"
#include <atomic>
#include <memory>

namespace HAM {

//==============================================================================
/**
    Handles asynchronous pattern switching with quantization options.
    Ensures seamless transitions between patterns/scenes without glitches.
*/
class AsyncPatternEngine : public MasterClock::Listener
{
public:
    //==========================================================================
    // Quantization modes for pattern switching
    enum class SwitchQuantization
    {
        IMMEDIATE,      // Switch immediately (may cause glitches)
        NEXT_PULSE,     // Switch on next 24ppq pulse
        NEXT_BEAT,      // Switch on next beat (quarter note)
        NEXT_BAR,       // Switch on next bar
        NEXT_2_BARS,    // Switch after 2 bars
        NEXT_4_BARS,    // Switch after 4 bars
        NEXT_8_BARS,    // Switch after 8 bars
        NEXT_16_BARS    // Switch after 16 bars
    };
    
    //==========================================================================
    // Listener interface for pattern changes
    class Listener
    {
    public:
        virtual ~Listener() = default;
        
        /** Called when pattern switch is queued */
        virtual void onPatternQueued(int patternIndex) {}
        
        /** Called when pattern actually switches */
        virtual void onPatternSwitched(int patternIndex) {}
        
        /** Called when scene switch is queued */
        virtual void onSceneQueued(int sceneIndex) {}
        
        /** Called when scene actually switches */
        virtual void onSceneSwitched(int sceneIndex) {}
    };
    
    //==========================================================================
    // Construction
    AsyncPatternEngine(MasterClock& clock);
    ~AsyncPatternEngine() override;
    
    //==========================================================================
    // Pattern Management
    
    /** Queue a pattern for switching */
    void queuePattern(int patternIndex, SwitchQuantization quantization = SwitchQuantization::NEXT_BAR);
    
    /** Queue a scene (collection of patterns) for switching */
    void queueScene(int sceneIndex, SwitchQuantization quantization = SwitchQuantization::NEXT_BAR);
    
    /** Cancel any pending pattern/scene switch */
    void cancelPendingSwitch();
    
    /** Check if switch is pending */
    bool hasPendingSwitch() const { return m_pendingPatternIndex >= 0 || m_pendingSceneIndex >= 0; }
    
    /** Get pending pattern index (-1 if none) */
    int getPendingPatternIndex() const { return m_pendingPatternIndex.load(); }
    
    /** Get pending scene index (-1 if none) */
    int getPendingSceneIndex() const { return m_pendingSceneIndex.load(); }
    
    //==========================================================================
    // Current State
    
    /** Get current pattern index */
    int getCurrentPatternIndex() const { return m_currentPatternIndex.load(); }
    
    /** Get current scene index */
    int getCurrentSceneIndex() const { return m_currentSceneIndex.load(); }
    
    /** Get bars until switch (or -1 if no pending switch) */
    int getBarsUntilSwitch() const;
    
    /** Get beats until switch (or -1 if no pending switch) */
    int getBeatsUntilSwitch() const;
    
    //==========================================================================
    // Quantization Settings
    
    /** Set default quantization mode */
    void setDefaultQuantization(SwitchQuantization mode) { m_defaultQuantization = mode; }
    
    /** Get default quantization mode */
    SwitchQuantization getDefaultQuantization() const { return m_defaultQuantization; }
    
    //==========================================================================
    // Listener Management
    
    void addListener(Listener* listener);
    void removeListener(Listener* listener);
    
    //==========================================================================
    // MasterClock::Listener implementation
    
    void onClockPulse(int pulseNumber) override;
    void onClockStart() override;
    void onClockStop() override;
    void onClockReset() override;
    void onTempoChanged(float newBPM) override;
    
private:
    //==========================================================================
    // Internal State
    
    MasterClock& m_clock;
    
    // Current state
    std::atomic<int> m_currentPatternIndex{0};
    std::atomic<int> m_currentSceneIndex{0};
    
    // Pending switches
    std::atomic<int> m_pendingPatternIndex{-1};
    std::atomic<int> m_pendingSceneIndex{-1};
    std::atomic<int> m_switchTargetPulse{-1};
    
    SwitchQuantization m_pendingQuantization{SwitchQuantization::NEXT_BAR};
    SwitchQuantization m_defaultQuantization{SwitchQuantization::NEXT_BAR};
    
    // Listeners
    std::vector<Listener*> m_listeners;
    juce::CriticalSection m_listenerLock;
    
    //==========================================================================
    // Internal Methods
    
    /** Calculate target pulse for switch based on quantization */
    int calculateTargetPulse(SwitchQuantization quantization) const;
    
    /** Execute the pending switch */
    void executePendingSwitch();
    
    /** Notify listeners of pattern queue */
    void notifyPatternQueued(int index);
    void notifyPatternSwitched(int index);
    void notifySceneQueued(int index);
    void notifySceneSwitched(int index);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AsyncPatternEngine)
};

} // namespace HAM