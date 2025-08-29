/*
  ==============================================================================

    ScaleSlotSelector.h
    Visual component for scale slot selection and management

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../../UI/BasicComponents.h"
#include "../../UI/AdvancedComponents.h"
#include "../ViewModels/ScaleSlotViewModel.h"
#include "../Components/ArrowButton.h"

namespace HAM {
namespace UI {

//==============================================================================
/**
 * Visual scale slot selector component - displays 8 scale slots with
 * navigation arrows and auto-mode controls. Inspired by Metropolix.
 */
class ScaleSlotSelector : public PulseComponent,
                          public juce::ChangeListener,
                          private juce::Timer
{
public:
    //==========================================================================
    ScaleSlotSelector();
    ~ScaleSlotSelector() override;
    
    //==========================================================================
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseMove(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;
    bool keyPressed(const juce::KeyPress& key) override;
    
    //==========================================================================
    // ChangeListener for ViewModel updates
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;
    
    //==========================================================================
    // Public interface
    
    /** Connect to view model for data binding */
    void setViewModel(ScaleSlotViewModel* viewModel);
    
    /** Set callback for when scale browser should open */
    std::function<void(int slotIndex)> onScaleBrowserRequested;
    
    /** Set callback for when root note should change */
    std::function<void(int rootNote)> onRootNoteChanged;
    
private:
    //==========================================================================
    // Internal components
    
    /** Individual scale slot button */
    class ScaleSlotButton : public PulseComponent
    {
    public:
        ScaleSlotButton(int slotIndex);
        ~ScaleSlotButton() override = default;
        
        void paint(juce::Graphics& g) override;
        void mouseDown(const juce::MouseEvent& event) override;
        void mouseEnter(const juce::MouseEvent& event) override;
        void mouseExit(const juce::MouseEvent& event) override;
        
        // Update slot display info
        void setSlotInfo(const ScaleSlotViewModel::SlotInfo& info);
        void setActive(bool active);
        void setPending(bool pending);
        void setHovered(bool hovered);
        
        std::function<void()> onClick;
        std::function<void()> onBrowseClick;
        
    private:
        int m_slotIndex;
        ScaleSlotViewModel::SlotInfo m_slotInfo;
        bool m_isActive = false;
        bool m_isPending = false;
        bool m_isHovered = false;
        
        // Animation values
        float m_hoverAnimation = 0.0f;
        float m_activeAnimation = 0.0f;
        
        // Browse button hit area
        juce::Rectangle<int> m_browseButtonArea;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScaleSlotButton)
    };
    
    //==========================================================================
    // Timer callback for animations
    void timerCallback() override;
    
    // UI Layout helpers
    void layoutSlots();
    void updateSlotStates();
    
    // Event handling
    void handleSlotClick(int slotIndex);
    void handleLeftArrowClick();
    void handleRightArrowClick();
    void handleAutoModeToggle();
    void handleAutoModeMenu();
    void handleRootNoteMenu();
    
    // Helper functions
    int getSlotUnderMouse(const juce::Point<int>& position);
    juce::Colour getSlotColor(int slotIndex) const;
    juce::String getRootNoteString(int rootNote) const;
    juce::String getAutoModeString(ScaleSlotViewModel::AutoMode mode) const;
    
    //==========================================================================
    // Internal state
    
    // View model connection
    ScaleSlotViewModel* m_viewModel = nullptr;
    
    // UI Components
    std::array<std::unique_ptr<ScaleSlotButton>, 8> m_slotButtons;
    std::unique_ptr<ArrowButton> m_leftArrowButton;
    std::unique_ptr<ArrowButton> m_rightArrowButton;
    std::unique_ptr<PulseButton> m_autoModeButton;  // Changed to button
    std::unique_ptr<PulseButton> m_autoModeMenuButton;
    std::unique_ptr<PulseButton> m_rootNoteButton;
    
    // Layout regions
    juce::Rectangle<int> m_slotsArea;
    juce::Rectangle<int> m_controlsArea;
    
    // Animation state
    float m_autoProgressAnimation = 0.0f;
    int m_hoveredSlotIndex = -1;
    
    // Colors for scale slots (8 distinct colors)
    const std::array<juce::Colour, 8> m_slotColors = {
        juce::Colour(0xFF00FF88),  // Mint (Pulse signature)
        juce::Colour(0xFF00DDFF),  // Cyan
        juce::Colour(0xFFFF00DD),  // Magenta
        juce::Colour(0xFFFFAA00),  // Orange
        juce::Colour(0xFF88FF00),  // Lime
        juce::Colour(0xFF0088FF),  // Blue
        juce::Colour(0xFFFF0088),  // Pink
        juce::Colour(0xFF8800FF)   // Purple
    };
    
    // Note names for root display
    const std::array<juce::String, 12> m_noteNames = {
        "C", "C#", "D", "D#", "E", "F", 
        "F#", "G", "G#", "A", "A#", "B"
    };
    
    // Position tracking for alignment
    int m_scaleSlotsStartX = 0;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScaleSlotSelector)
};

} // namespace UI
} // namespace HAM