/*
  ==============================================================================

    ScaleSlotViewModel.h
    ViewModel for scale slot management - bridges UI and ScaleSlotManager

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Domain/Models/ScaleSlotManager.h"
#include <array>
#include <atomic>

namespace HAM {
namespace UI {

//==============================================================================
/**
 * ViewModel that bridges the ScaleSlotSelector UI with the ScaleSlotManager
 * Implements the listener pattern for real-time updates from the audio thread
 */
class ScaleSlotViewModel : public juce::ChangeBroadcaster,
                          public ScaleSlotManager::Listener,
                          private juce::Timer
{
public:
    //==========================================================================
    // Auto-progression modes
    enum class AutoMode
    {
        OFF,
        QUARTER_BAR,    // Change every 1/4 bar
        ONE_BAR,        // Change every bar
        TWO_BARS,       // Change every 2 bars
        FOUR_BARS,      // Change every 4 bars
        EIGHT_BARS,     // Change every 8 bars
        SIXTEEN_BARS    // Change every 16 bars
    };
    
    //==========================================================================
    // Slot display info for UI
    struct SlotInfo
    {
        bool isEmpty = true;
        bool isActive = false;
        bool isPending = false;
        juce::String displayName;
        juce::String scaleType;
        int rootNote = 0;  // 0-11, C=0
    };
    
    //==========================================================================
    ScaleSlotViewModel();
    ~ScaleSlotViewModel() override;
    
    //==========================================================================
    // UI → Backend: User actions
    
    /** Select a scale slot (will queue for bar-quantized change) */
    void selectSlot(int slotIndex);
    
    /** Load a scale into a slot */
    void loadScaleIntoSlot(int slotIndex, const Scale& scale, const juce::String& name);
    
    /** Clear a slot */
    void clearSlot(int slotIndex);
    
    /** Set global root note */
    void setGlobalRoot(int rootNote);
    
    /** Manual slot navigation */
    void selectNextSlot();
    void selectPreviousSlot();
    
    /** Auto-progression control */
    void setAutoMode(AutoMode mode);
    AutoMode getAutoMode() const { return m_autoMode; }
    void startAutoProgression();
    void stopAutoProgression();
    bool isAutoProgressionActive() const { return m_autoProgressionActive; }
    
    //==========================================================================
    // Backend → UI: State queries
    
    /** Get display info for all 8 slots */
    std::array<SlotInfo, 8> getAllSlotInfo() const;
    
    /** Get info for a specific slot */
    SlotInfo getSlotInfo(int slotIndex) const;
    
    /** Get currently active slot index */
    int getActiveSlotIndex() const;
    
    /** Get pending slot index (-1 if none) */
    int getPendingSlotIndex() const;
    
    /** Get global root note */
    int getGlobalRoot() const;
    
    /** Get progress in current auto-progression cycle (0.0-1.0) */
    float getAutoProgressionProgress() const { return m_autoProgress; }
    
    //==========================================================================
    // ScaleSlotManager::Listener implementation
    
    void scaleSlotSelected(int slotIndex) override;
    void scaleChanged(int slotIndex) override;
    void globalRootChanged(int rootNote) override;
    
private:
    //==========================================================================
    // Timer callback for auto-progression
    void timerCallback() override;
    
    /** Update cached slot info from ScaleSlotManager */
    void updateSlotInfo(int slotIndex);
    void updateAllSlotInfo();
    
    /** Find next non-empty slot for auto-progression */
    int findNextNonEmptySlot(int currentSlot) const;
    
    /** Calculate timer interval based on auto mode and BPM */
    int calculateTimerInterval() const;
    
    //==========================================================================
    // Internal state
    
    // Cached slot information for UI display
    mutable std::array<SlotInfo, 8> m_slotInfoCache;
    mutable std::atomic<bool> m_cacheValid{false};
    
    // Auto-progression state
    AutoMode m_autoMode{AutoMode::OFF};
    bool m_autoProgressionActive{false};
    float m_autoProgress{0.0f};
    int m_autoBarCounter{0};
    int m_barsPerSlot{1};
    
    // Current state
    std::atomic<int> m_activeSlotIndex{0};
    std::atomic<int> m_pendingSlotIndex{-1};
    std::atomic<int> m_globalRoot{0};
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScaleSlotViewModel)
};

} // namespace UI
} // namespace HAM