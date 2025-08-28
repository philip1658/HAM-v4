/*
  ==============================================================================

    PulseScaleSwitcher.h
    Pulse-style Scale Slot Manager with 8 slots, global root, and bar-quantized switching
    Based on Metropolix/Pulse hardware sequencer design

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../../Domain/Models/Scale.h"
#include "../../Domain/Models/ScaleSlotManager.h"

namespace HAM {
namespace UI {

//==============================================================================
/**
 * Pulse-style Scale Switcher with 8 slots, global root control, and bar-quantized switching
 * 
 * Features:
 * - 8 global scale slots (shared across all tracks)
 * - Global root note selector
 * - Bar-quantized scale changes (armed until next bar boundary)
 * - Visual feedback for pending changes
 * - Auto-progression mode with configurable intervals
 * - Compact horizontal layout for toolbar integration
 */
class PulseScaleSwitcher : public juce::Component,
                           private juce::Timer
{
public:
    //==========================================================================
    PulseScaleSwitcher();
    ~PulseScaleSwitcher() override;
    
    //==========================================================================
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    //==========================================================================
    // Public interface
    
    /** Connect to ScaleSlotManager for data binding */
    void setScaleSlotManager(ScaleSlotManager* manager);
    
    /** Set the current bar position (0.0 - 1.0) for progress display */
    void setBarProgress(float progress);
    
    /** Callbacks */
    std::function<void(int slotIndex)> onSlotSelected;
    std::function<void(int slotIndex)> onSlotEditRequested;
    std::function<void(int rootNote)> onRootNoteChanged;
    std::function<void(bool enabled, int bars)> onAutoModeChanged;
    
private:
    //==========================================================================
    // Internal components
    
    /** Individual scale slot button (compact design) */
    class SlotButton : public juce::Component
    {
    public:
        SlotButton(int index);
        
        void paint(juce::Graphics& g) override;
        void mouseDown(const juce::MouseEvent& event) override;
        void mouseEnter(const juce::MouseEvent& event) override;
        void mouseExit(const juce::MouseEvent& event) override;
        
        void setScale(const Scale* scale);
        void setActive(bool active);
        void setPending(bool pending);
        void setModified(bool modified);
        
        std::function<void()> onClick;
        std::function<void()> onRightClick;
        
    private:
        int m_index;
        const Scale* m_scale = nullptr;
        juce::String m_displayName;
        bool m_isActive = false;
        bool m_isPending = false;
        bool m_isModified = false;
        bool m_isHovered = false;
        
        // Colors based on slot index (like Metropolix)
        juce::Colour getSlotColor() const;
    };
    
    /** Root note selector button */
    class RootButton : public juce::Component
    {
    public:
        RootButton();
        
        void paint(juce::Graphics& g) override;
        void mouseDown(const juce::MouseEvent& event) override;
        
        void setRootNote(int note);
        int getRootNote() const { return m_rootNote; }
        
        std::function<void(int)> onRootChanged;
        
    private:
        int m_rootNote = 0; // C
        
        void showRootMenu();
        juce::String getRootName() const;
    };
    
    /** Auto-progression control */
    class AutoModeButton : public juce::Component
    {
    public:
        AutoModeButton();
        
        void paint(juce::Graphics& g) override;
        void mouseDown(const juce::MouseEvent& event) override;
        
        void setEnabled(bool enabled);
        void setInterval(int bars);
        
        std::function<void(bool, int)> onModeChanged;
        
    private:
        bool m_enabled = false;
        int m_intervalBars = 4;
        
        void showIntervalMenu();
    };
    
    //==========================================================================
    // Timer callback for animations
    void timerCallback() override;
    
    //==========================================================================
    // Layout helpers
    void layoutComponents();
    void updateSlotDisplays();
    void handleSlotSelection(int slotIndex);
    void handleSlotEdit(int slotIndex);
    
    //==========================================================================
    // Data
    ScaleSlotManager* m_scaleManager = nullptr;
    
    // UI Components
    std::array<std::unique_ptr<SlotButton>, 8> m_slotButtons;
    std::unique_ptr<RootButton> m_rootButton;
    std::unique_ptr<AutoModeButton> m_autoButton;
    std::unique_ptr<juce::Label> m_statusLabel;
    
    // State
    int m_activeSlot = 0;
    int m_pendingSlot = -1;
    float m_barProgress = 0.0f;
    bool m_hasArmedChange = false;
    
    // Animation
    float m_pendingFlashPhase = 0.0f;
    float m_progressAnimation = 0.0f;
    
    // Layout constants
    static constexpr int SLOT_WIDTH = 70;
    static constexpr int SLOT_HEIGHT = 32;
    static constexpr int ROOT_WIDTH = 60;
    static constexpr int AUTO_WIDTH = 80;
    static constexpr int SPACING = 4;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PulseScaleSwitcher)
};

} // namespace UI
} // namespace HAM