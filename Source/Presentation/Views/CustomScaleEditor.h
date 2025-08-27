/*
  ==============================================================================

    CustomScaleEditor.h
    Interactive editor for creating custom musical scales

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../../Domain/Models/Scale.h"
#include "../../UI/BasicComponents.h"
#include "../../UI/AdvancedComponents.h"
#include <vector>
#include <array>

namespace HAM {
namespace UI {

//==============================================================================
/**
 * Custom scale editor - allows users to create and edit their own scales
 * Features:
 * - Interactive piano keyboard for note selection
 * - Microtonal adjustment with cent offsets
 * - Scale preview and audition
 * - Save/load custom scales
 */
class CustomScaleEditor : public juce::DialogWindow
{
public:
    //==========================================================================
    CustomScaleEditor(const juce::String& initialScaleName = "Custom Scale");
    ~CustomScaleEditor() override;
    
    //==========================================================================
    // DialogWindow overrides
    void closeButtonPressed() override;
    bool keyPressed(const juce::KeyPress& key) override;
    
    //==========================================================================
    // Callbacks
    std::function<void(const Scale& scale, const juce::String& name)> onScaleCreated;
    
    //==========================================================================
    // Static method to show the editor
    static void showScaleEditor(std::function<void(const Scale&, const juce::String&)> callback);
    
private:
    //==========================================================================
    // Internal content component
    class ScaleEditorContent : public PulseComponent
    {
    public:
        ScaleEditorContent();
        ~ScaleEditorContent() override;
        
        void paint(juce::Graphics& g) override;
        void resized() override;
        
        // Get the created scale
        Scale getScale() const;
        juce::String getScaleName() const { return m_nameEditor->getText(); }
        
        // Set initial scale for editing
        void setScale(const Scale& scale);
        
        std::function<void()> onSaveClicked;
        std::function<void()> onCancelClicked;
        
    private:
        //======================================================================
        // Piano keyboard for note selection
        class InteractivePianoKeyboard : public juce::Component
        {
        public:
            InteractivePianoKeyboard();
            
            void paint(juce::Graphics& g) override;
            void resized() override;
            void mouseDown(const juce::MouseEvent& event) override;
            void mouseDrag(const juce::MouseEvent& event) override;
            void mouseUp(const juce::MouseEvent& event) override;
            
            // Note selection
            void setNoteSelected(int noteNumber, bool selected);
            bool isNoteSelected(int noteNumber) const;
            std::vector<int> getSelectedNotes() const;
            void clearSelection();
            
            // Microtonal adjustments
            void setCentOffset(int noteNumber, float cents);
            float getCentOffset(int noteNumber) const;
            
            // Callbacks
            std::function<void(int noteNumber)> onNoteToggled;
            std::function<void(int noteNumber)> onNotePreview;
            
        private:
            struct KeyInfo
            {
                juce::Rectangle<float> bounds;
                bool isBlackKey;
                bool isSelected = false;
                float centOffset = 0.0f;
                bool isHovered = false;
            };
            
            std::array<KeyInfo, 12> m_keys;  // One octave
            int m_rootNote = 0;  // C
            
            int getKeyAtPosition(juce::Point<int> pos) const;
            void drawWhiteKey(juce::Graphics& g, const KeyInfo& key, int noteNum);
            void drawBlackKey(juce::Graphics& g, const KeyInfo& key, int noteNum);
            
            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InteractivePianoKeyboard)
        };
        
        //======================================================================
        // Interval display showing scale structure
        class IntervalDisplay : public juce::Component
        {
        public:
            IntervalDisplay();
            
            void paint(juce::Graphics& g) override;
            void setIntervals(const std::vector<int>& intervals);
            
        private:
            std::vector<int> m_intervals;
            
            juce::String getIntervalName(int semitones) const;
            juce::Colour getIntervalColor(int semitones) const;
            
            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(IntervalDisplay)
        };
        
        //======================================================================
        // Member components
        std::unique_ptr<juce::TextEditor> m_nameEditor;
        std::unique_ptr<InteractivePianoKeyboard> m_keyboard;
        std::unique_ptr<IntervalDisplay> m_intervalDisplay;
        
        // Control buttons
        std::unique_ptr<PulseButton> m_saveButton;
        std::unique_ptr<PulseButton> m_cancelButton;
        std::unique_ptr<PulseButton> m_previewButton;
        std::unique_ptr<PulseButton> m_clearButton;
        
        // Preset templates
        std::unique_ptr<PulseDropdown> m_templateDropdown;
        
        // Microtonal controls
        std::unique_ptr<PulseHorizontalSlider> m_centSlider;
        std::unique_ptr<juce::Label> m_centLabel;
        int m_selectedNoteForCents = -1;
        
        // Scale data
        std::vector<int> m_selectedNotes;
        std::array<float, 12> m_centOffsets;
        
        // Helper methods
        void updateIntervalDisplay();
        void loadTemplate(const juce::String& templateName);
        void previewScale();
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScaleEditorContent)
    };
    
    //==========================================================================
    // Member variables
    std::unique_ptr<ScaleEditorContent> m_content;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CustomScaleEditor)
};

} // namespace UI
} // namespace HAM