/*
  ==============================================================================

    ScaleBrowser.h
    Scale browser dialog for selecting and loading scales into slots

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../../Domain/Models/Scale.h"
#include "../../UI/BasicComponents.h"
#include "../../UI/AdvancedComponents.h"
#include "../../Infrastructure/Messaging/MessageTypes.h"

namespace HAM {
namespace UI {

//==============================================================================
/**
 * Scale browser dialog - provides categorized access to 1000+ scales
 * with search, preview, and loading functionality
 */
class ScaleBrowser : public juce::DialogWindow,
                     public juce::TextEditor::Listener
{
public:
    //==========================================================================
    // Scale categories for organization
    enum class ScaleCategory
    {
        Common,      // Major, Minor, Pentatonic, etc.
        Modal,       // Dorian, Phrygian, Lydian, etc.
        Jazz,        // Bebop, Altered, Diminished, etc.
        World,       // Arabic, Japanese, Indian, etc.
        Synthetic,   // Whole tone, Augmented, etc.
        Custom,      // User-created scales
        All          // Show all scales
    };
    
    //==========================================================================
    ScaleBrowser(int targetSlotIndex);
    ~ScaleBrowser() override;
    
    //==========================================================================
    // DialogWindow overrides
    void closeButtonPressed() override;
    bool keyPressed(const juce::KeyPress& key) override;
    
    //==========================================================================
    // Callbacks
    std::function<void(int slotIndex, const Scale& scale, const juce::String& name)> onScaleSelected;
    
    //==========================================================================
    // Static method to show the browser
    static void showScaleBrowser(int targetSlotIndex,
                                 std::function<void(int, const Scale&, const juce::String&)> callback);
    
private:
    //==========================================================================
    // Internal content component
    class ScaleBrowserContent : public PulseComponent,
                                public juce::ListBoxModel,
                                public juce::TextEditor::Listener
    {
    public:
        ScaleBrowserContent(int targetSlot);
        ~ScaleBrowserContent() override;
        
        void paint(juce::Graphics& g) override;
        void resized() override;
        
        // ListBoxModel implementation
        int getNumRows() override;
        void paintListBoxItem(int rowNumber, juce::Graphics& g,
                             int width, int height,
                             bool rowIsSelected) override;
        void listBoxItemClicked(int row, const juce::MouseEvent&) override;
        void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override;
        
        // TextEditor::Listener overrides
        void textEditorTextChanged(juce::TextEditor&) override;
        void textEditorEscapeKeyPressed(juce::TextEditor&) override;
        
        // Scale selection
        void selectScale(int index);
        void loadSelectedScale();
        void previewScale();
        
        // Get selected scale info
        const Scale& getSelectedScale() const;
        juce::String getSelectedScaleName() const;
        
        std::function<void(const Scale&, const juce::String&)> onScaleChosen;
        
    private:
        // UI Components
        std::unique_ptr<juce::TextEditor> m_searchBox;
        std::unique_ptr<juce::ListBox> m_scaleList;
        std::unique_ptr<PulseButton> m_loadButton;
        std::unique_ptr<PulseButton> m_previewButton;
        std::unique_ptr<PulseButton> m_cancelButton;
        
        // Category buttons
        std::vector<std::unique_ptr<PulseButton>> m_categoryButtons;
        
        // Scale keyboard for preview
        std::unique_ptr<juce::Component> m_scaleKeyboard;
        
        // Data
        struct ScaleEntry
        {
            Scale scale;
            juce::String name;
            juce::String category;
            juce::String description;
            std::vector<int> intervals;
        };
        
        std::vector<ScaleEntry> m_allScales;
        std::vector<ScaleEntry> m_filteredScales;
        
        int m_targetSlotIndex;
        int m_selectedIndex = -1;
        ScaleCategory m_currentCategory = ScaleCategory::All;
        juce::String m_searchText;
        int m_lastPreviewNote = -1;  // For MIDI preview tracking
        
        // Initialize scale database
        void initializeScales();
        void addCommonScales();
        void addModalScales();
        void addJazzScales();
        void addWorldScales();
        void addSyntheticScales();
        void addBluesScales();
        void addMinorScales();
        void addExoticScales();
        void addHistoricalScales();
        void addMicrotonal();
        void addContemporary();
        void addMathematical();
        
        // Filtering
        void updateFilteredScales();
        bool matchesSearch(const ScaleEntry& entry) const;
        bool matchesCategory(const ScaleEntry& entry) const;
        
        // Helpers
        juce::String getCategoryName(ScaleCategory cat) const;
        juce::String getIntervalString(const std::vector<int>& intervals) const;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScaleBrowserContent)
    };
    
    //==========================================================================
    // Member variables
    std::unique_ptr<ScaleBrowserContent> m_content;
    int m_targetSlotIndex;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScaleBrowser)
};

} // namespace UI
} // namespace HAM