/*
  ==============================================================================

    ScaleBrowser.cpp
    Implementation of scale browser dialog

  ==============================================================================
*/

#include "ScaleBrowser.h"

namespace HAM {
namespace UI {

//==============================================================================
// ScaleBrowser implementation

ScaleBrowser::ScaleBrowser(int targetSlotIndex)
    : DialogWindow("Scale Browser - Slot " + juce::String(targetSlotIndex + 1),
                   juce::Colour(0xFF1A1A1A), true),
      m_targetSlotIndex(targetSlotIndex)
{
    m_content = std::make_unique<ScaleBrowserContent>(targetSlotIndex);
    
    m_content->onScaleChosen = [this](const Scale& scale, const juce::String& name)
    {
        if (onScaleSelected)
            onScaleSelected(m_targetSlotIndex, scale, name);
        closeButtonPressed();
    };
    
    setContentOwned(m_content.release(), false);
    
    // Set dialog size and position
    centreWithSize(800, 600);
    setResizable(false, false);
    setUsingNativeTitleBar(false);
}

ScaleBrowser::~ScaleBrowser()
{
}

void ScaleBrowser::closeButtonPressed()
{
    setVisible(false);
}

bool ScaleBrowser::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::escapeKey)
    {
        closeButtonPressed();
        return true;
    }
    return false;
}

void ScaleBrowser::showScaleBrowser(int targetSlotIndex,
                                    std::function<void(int, const Scale&, const juce::String&)> callback)
{
    auto* browser = new ScaleBrowser(targetSlotIndex);
    browser->onScaleSelected = callback;
    browser->setVisible(true);
}

//==============================================================================
// ScaleBrowserContent implementation

ScaleBrowser::ScaleBrowserContent::ScaleBrowserContent(int targetSlot)
    : PulseComponent("ScaleBrowserContent"),
      m_targetSlotIndex(targetSlot)
{
    
    // Initialize scale database
    initializeScales();
    
    // Create search box
    m_searchBox = std::make_unique<juce::TextEditor>();
    m_searchBox->setTextToShowWhenEmpty("Search scales...", juce::Colours::grey);
    m_searchBox->setFont(juce::FontOptions(14.0f));
    m_searchBox->addListener(this);
    addAndMakeVisible(m_searchBox.get());
    
    // Create scale list
    m_scaleList = std::make_unique<juce::ListBox>("ScaleList", this);
    m_scaleList->setRowHeight(40);
    m_scaleList->setColour(juce::ListBox::backgroundColourId, juce::Colour(0xFF0A0A0A));
    m_scaleList->setOutlineThickness(1);
    addAndMakeVisible(m_scaleList.get());
    
    // Create category buttons
    const std::vector<ScaleCategory> categories = {
        ScaleCategory::All,
        ScaleCategory::Common,
        ScaleCategory::Modal,
        ScaleCategory::Jazz,
        ScaleCategory::World,
        ScaleCategory::Synthetic
    };
    
    for (size_t i = 0; i < categories.size(); ++i)
    {
        auto category = categories[i];
        auto button = std::make_unique<PulseButton>(getCategoryName(category), 
                                                    PulseButton::Outline);
        button->onClick = [this, category]() {
            m_currentCategory = category;
            updateFilteredScales();
        };
        addAndMakeVisible(button.get());
        m_categoryButtons.push_back(std::move(button));
    }
    
    // Create action buttons
    m_loadButton = std::make_unique<PulseButton>("Load Scale", PulseButton::Solid);
    m_loadButton->onClick = [this]() {
        loadSelectedScale();
    };
    addAndMakeVisible(m_loadButton.get());
    
    m_previewButton = std::make_unique<PulseButton>("Preview", PulseButton::Outline);
    m_previewButton->onClick = [this]() {
        previewScale();
    };
    addAndMakeVisible(m_previewButton.get());
    
    m_cancelButton = std::make_unique<PulseButton>("Cancel", PulseButton::Ghost);
    m_cancelButton->onClick = [this]() {
        if (auto* dialog = findParentComponentOfClass<juce::DialogWindow>())
            dialog->closeButtonPressed();
    };
    addAndMakeVisible(m_cancelButton.get());
    
    // Initialize with all scales
    m_filteredScales = m_allScales;
    m_scaleList->updateContent();
}

ScaleBrowser::ScaleBrowserContent::~ScaleBrowserContent()
{
}

void ScaleBrowser::ScaleBrowserContent::paint(juce::Graphics& g)
{
    // Background
    g.fillAll(juce::Colour(0xFF0A0A0A));
    
    // Title area
    g.setColour(juce::Colour(0xFF1A1A1A));
    g.fillRect(0, 0, getWidth(), 50);
    
    g.setFont(18.0f);
    g.setColour(juce::Colours::white);
    g.drawText("Select Scale for Slot " + juce::String(m_targetSlotIndex + 1),
               10, 10, getWidth() - 20, 30, juce::Justification::centred);
    
    // Selected scale info
    if (m_selectedIndex >= 0 && m_selectedIndex < m_filteredScales.size())
    {
        auto& entry = m_filteredScales[m_selectedIndex];
        
        auto infoBounds = juce::Rectangle<int>(getWidth() - 250, 120, 240, 300);
        
        g.setColour(juce::Colour(0xFF1A1A1A));
        g.fillRoundedRectangle(infoBounds.toFloat(), 5.0f);
        
        g.setColour(juce::Colour(0xFF3A3A3A));
        g.drawRoundedRectangle(infoBounds.toFloat(), 5.0f, 1.0f);
        
        g.setColour(juce::Colours::white);
        g.setFont(16.0f);
        g.drawText(entry.name, infoBounds.removeFromTop(40), 
                  juce::Justification::centred);
        
        g.setFont(12.0f);
        g.setColour(juce::Colours::white.withAlpha(0.7f));
        g.drawText("Category: " + entry.category,
                  infoBounds.removeFromTop(25), juce::Justification::centred);
        
        g.drawText("Intervals: " + getIntervalString(entry.intervals),
                  infoBounds.removeFromTop(25), juce::Justification::centred);
        
        if (entry.description.isNotEmpty())
        {
            g.drawMultiLineText(entry.description, 
                               infoBounds.getX() + 10, 
                               infoBounds.getY() + 20,
                               infoBounds.getWidth() - 20);
        }
    }
}

void ScaleBrowser::ScaleBrowserContent::resized()
{
    auto bounds = getLocalBounds();
    
    // Title area
    bounds.removeFromTop(50);
    
    // Search box
    m_searchBox->setBounds(bounds.removeFromTop(40).reduced(10, 5));
    
    // Category buttons
    auto categoryArea = bounds.removeFromTop(40);
    int buttonWidth = categoryArea.getWidth() / m_categoryButtons.size();
    for (auto& button : m_categoryButtons)
    {
        button->setBounds(categoryArea.removeFromLeft(buttonWidth).reduced(5));
    }
    
    // Bottom buttons
    auto buttonArea = bounds.removeFromBottom(50);
    m_cancelButton->setBounds(buttonArea.removeFromLeft(100).reduced(10));
    m_loadButton->setBounds(buttonArea.removeFromRight(120).reduced(10));
    m_previewButton->setBounds(buttonArea.removeFromRight(100).reduced(10));
    
    // Scale list (left side, leaving room for info panel)
    m_scaleList->setBounds(bounds.reduced(10).withWidth(bounds.getWidth() - 270));
}

int ScaleBrowser::ScaleBrowserContent::getNumRows()
{
    return m_filteredScales.size();
}

void ScaleBrowser::ScaleBrowserContent::paintListBoxItem(int rowNumber, juce::Graphics& g,
                                                         int width, int height,
                                                         bool rowIsSelected)
{
    if (rowNumber >= m_filteredScales.size())
        return;
    
    auto& entry = m_filteredScales[rowNumber];
    
    // Background
    if (rowIsSelected)
    {
        g.setColour(juce::Colour(0xFF00FF88).withAlpha(0.2f));
        g.fillRect(0, 0, width, height);
    }
    else if (rowNumber % 2 == 0)
    {
        g.setColour(juce::Colour(0xFF1A1A1A).withAlpha(0.3f));
        g.fillRect(0, 0, width, height);
    }
    
    // Scale name
    g.setFont(14.0f);
    g.setColour(rowIsSelected ? juce::Colour(0xFF00FF88) : juce::Colours::white);
    g.drawText(entry.name, 10, 5, width - 20, 20, juce::Justification::left);
    
    // Category and interval count
    g.setFont(11.0f);
    g.setColour(juce::Colours::white.withAlpha(0.6f));
    g.drawText(entry.category + " - " + juce::String(entry.intervals.size()) + " notes",
               10, 22, width - 20, 15, juce::Justification::left);
    
    // Border
    g.setColour(juce::Colour(0xFF3A3A3A).withAlpha(0.3f));
    g.drawLine(0, height - 1, width, height - 1, 0.5f);
}

void ScaleBrowser::ScaleBrowserContent::listBoxItemClicked(int row, const juce::MouseEvent&)
{
    selectScale(row);
}

void ScaleBrowser::ScaleBrowserContent::listBoxItemDoubleClicked(int row, const juce::MouseEvent&)
{
    selectScale(row);
    loadSelectedScale();
}

// Button click handlers are now done via onClick callbacks in constructor

void ScaleBrowser::ScaleBrowserContent::textEditorTextChanged(juce::TextEditor&)
{
    m_searchText = m_searchBox->getText();
    updateFilteredScales();
}

void ScaleBrowser::ScaleBrowserContent::textEditorEscapeKeyPressed(juce::TextEditor&)
{
    m_searchBox->setText("");
    m_searchText.clear();
    updateFilteredScales();
}

void ScaleBrowser::ScaleBrowserContent::selectScale(int index)
{
    if (index >= 0 && index < m_filteredScales.size())
    {
        m_selectedIndex = index;
        repaint();
    }
}

void ScaleBrowser::ScaleBrowserContent::loadSelectedScale()
{
    if (m_selectedIndex >= 0 && m_selectedIndex < m_filteredScales.size())
    {
        auto& entry = m_filteredScales[m_selectedIndex];
        if (onScaleChosen)
            onScaleChosen(entry.scale, entry.name);
    }
}

void ScaleBrowser::ScaleBrowserContent::previewScale()
{
    // Play ascending scale pattern for preview
    if (m_selectedIndex >= 0 && m_selectedIndex < m_filteredScales.size())
    {
        auto& entry = m_filteredScales[m_selectedIndex];
        DBG("Preview scale: " << entry.name);
        
        // Get the app controller to send MIDI preview messages
        if (auto* appController = findParentComponentOfClass<ScaleBrowser>())
        {
            // Send note-off for any previous preview note
            if (m_lastPreviewNote >= 0)
            {
                UIToEngineMessage noteOffMsg;
                noteOffMsg.type = UIToEngineMessage::PREVIEW_NOTE_OFF;
                noteOffMsg.data.previewParam.note = m_lastPreviewNote;
                noteOffMsg.data.previewParam.velocity = 0.0f;
                noteOffMsg.data.previewParam.channel = 1;
                
                // Send via message dispatcher (we'll need to get this reference)
                // For now, just prepare the structure
            }
            
            // Play ascending scale pattern
            const int rootNote = 60; // Middle C
            const float velocity = 100.0f;
            
            // Schedule notes to play in sequence
            for (size_t i = 0; i < entry.intervals.size(); ++i)
            {
                int note = rootNote + entry.intervals[i];
                
                // Create note-on message
                UIToEngineMessage noteOnMsg;
                noteOnMsg.type = UIToEngineMessage::PREVIEW_NOTE_ON;
                noteOnMsg.data.previewParam.note = note;
                noteOnMsg.data.previewParam.velocity = velocity;
                noteOnMsg.data.previewParam.channel = 1;
                
                // Store for later note-off
                m_lastPreviewNote = note;
                
                // In a real implementation, we'd schedule these with timing
                // For now, this shows the structure
            }
        }
    }
}

const Scale& ScaleBrowser::ScaleBrowserContent::getSelectedScale() const
{
    static Scale emptyScale;
    if (m_selectedIndex >= 0 && m_selectedIndex < m_filteredScales.size())
        return m_filteredScales[m_selectedIndex].scale;
    return emptyScale;
}

juce::String ScaleBrowser::ScaleBrowserContent::getSelectedScaleName() const
{
    if (m_selectedIndex >= 0 && m_selectedIndex < m_filteredScales.size())
        return m_filteredScales[m_selectedIndex].name;
    return "";
}

void ScaleBrowser::ScaleBrowserContent::initializeScales()
{
    addCommonScales();
    addModalScales();
    addJazzScales();
    addWorldScales();
    addSyntheticScales();
    addBluesScales();
    addMinorScales();
    addExoticScales();
    addHistoricalScales();
    addMicrotonal();
    addContemporary();
    addMathematical();
}

void ScaleBrowser::ScaleBrowserContent::addCommonScales()
{
    // Major/Minor scales and variations
    m_allScales.push_back({Scale("Major", {0,2,4,5,7,9,11}), 
                          "Major", "Common", 
                          "The major scale - happy and bright",
                          {0,2,4,5,7,9,11}});
    
    m_allScales.push_back({Scale("Natural Minor", {0,2,3,5,7,8,10}),
                          "Natural Minor", "Common",
                          "The natural minor scale - dark and melancholic", 
                          {0,2,3,5,7,8,10}});
    
    m_allScales.push_back({Scale("Harmonic Minor", {0,2,3,5,7,8,11}),
                          "Harmonic Minor", "Common",
                          "Minor scale with raised 7th degree",
                          {0,2,3,5,7,8,11}});
    
    m_allScales.push_back({Scale("Melodic Minor", {0,2,3,5,7,9,11}),
                          "Melodic Minor", "Common",
                          "Jazz minor scale",
                          {0,2,3,5,7,9,11}});

    m_allScales.push_back({Scale("Ascending Melodic Minor", {0,2,3,5,7,9,11}),
                          "Ascending Melodic Minor", "Common",
                          "Traditional ascending melodic minor",
                          {0,2,3,5,7,9,11}});

    m_allScales.push_back({Scale("Descending Melodic Minor", {0,2,3,5,7,8,10}),
                          "Descending Melodic Minor", "Common",
                          "Traditional descending melodic minor (natural minor)",
                          {0,2,3,5,7,8,10}});
    
    // Pentatonic scales
    m_allScales.push_back({Scale("Major Pentatonic", {0,2,4,7,9}),
                          "Major Pentatonic", "Common",
                          "Five-note major scale",
                          {0,2,4,7,9}});
    
    m_allScales.push_back({Scale("Minor Pentatonic", {0,3,5,7,10}),
                          "Minor Pentatonic", "Common",
                          "Five-note minor scale - blues/rock",
                          {0,3,5,7,10}});

    m_allScales.push_back({Scale("Egyptian Pentatonic", {0,2,5,7,10}),
                          "Egyptian Pentatonic", "Common",
                          "Ancient Egyptian scale",
                          {0,2,5,7,10}});

    m_allScales.push_back({Scale("Suspended Pentatonic", {0,2,5,7,10}),
                          "Suspended Pentatonic", "Common",
                          "Pentatonic with suspended feel",
                          {0,2,5,7,10}});

    m_allScales.push_back({Scale("Man Gong", {0,3,5,8,10}),
                          "Man Gong", "Common",
                          "Chinese pentatonic variation",
                          {0,3,5,8,10}});

    m_allScales.push_back({Scale("Ritusen", {0,2,5,7,9}),
                          "Ritusen", "Common",
                          "Japanese pentatonic scale",
                          {0,2,5,7,9}});

    m_allScales.push_back({Scale("Yo", {0,2,5,7,9}),
                          "Yo", "Common",
                          "Japanese pentatonic scale",
                          {0,2,5,7,9}});
    
    // Hexatonic scales (6-note)
    m_allScales.push_back({Scale("Major Hexatonic", {0,2,4,5,7,9}),
                          "Major Hexatonic", "Common",
                          "Major scale without 7th",
                          {0,2,4,5,7,9}});

    m_allScales.push_back({Scale("Minor Hexatonic", {0,2,3,5,7,8}),
                          "Minor Hexatonic", "Common",
                          "Natural minor without 7th",
                          {0,2,3,5,7,8}});

    m_allScales.push_back({Scale("Blues Hexatonic", {0,3,5,6,7,10}),
                          "Blues Hexatonic", "Common",
                          "Standard 6-note blues scale",
                          {0,3,5,6,7,10}});

    m_allScales.push_back({Scale("Whole Tone", {0,2,4,6,8,10}),
                          "Whole Tone", "Common",
                          "All whole steps - impressionistic",
                          {0,2,4,6,8,10}});
    
    // Chromatic and related
    m_allScales.push_back({Scale("Chromatic", {0,1,2,3,4,5,6,7,8,9,10,11}),
                          "Chromatic", "Common",
                          "All 12 notes",
                          {0,1,2,3,4,5,6,7,8,9,10,11}});

    m_allScales.push_back({Scale("Semitone", {0,1}),
                          "Semitone", "Common",
                          "Two adjacent semitones",
                          {0,1}});

    m_allScales.push_back({Scale("Tritone", {0,6}),
                          "Tritone", "Common",
                          "The devil's interval",
                          {0,6}});

    m_allScales.push_back({Scale("Perfect Fourth", {0,5}),
                          "Perfect Fourth", "Common",
                          "Two notes a perfect fourth apart",
                          {0,5}});

    m_allScales.push_back({Scale("Perfect Fifth", {0,7}),
                          "Perfect Fifth", "Common",
                          "Two notes a perfect fifth apart",
                          {0,7}});

    m_allScales.push_back({Scale("Octave", {0}),
                          "Octave", "Common",
                          "Single note (octave doubling)",
                          {0}});
}

void ScaleBrowser::ScaleBrowserContent::addModalScales()
{
    // Greek modes (Church modes)
    m_allScales.push_back({Scale("Ionian", {0,2,4,5,7,9,11}),
                          "Ionian", "Modal",
                          "Major scale - 1st mode",
                          {0,2,4,5,7,9,11}});

    m_allScales.push_back({Scale("Dorian", {0,2,3,5,7,9,10}),
                          "Dorian", "Modal",
                          "Minor scale with raised 6th - 2nd mode",
                          {0,2,3,5,7,9,10}});
    
    m_allScales.push_back({Scale("Phrygian", {0,1,3,5,7,8,10}),
                          "Phrygian", "Modal",
                          "Minor scale with lowered 2nd - 3rd mode",
                          {0,1,3,5,7,8,10}});
    
    m_allScales.push_back({Scale("Lydian", {0,2,4,6,7,9,11}),
                          "Lydian", "Modal",
                          "Major scale with raised 4th - 4th mode",
                          {0,2,4,6,7,9,11}});
    
    m_allScales.push_back({Scale("Mixolydian", {0,2,4,5,7,9,10}),
                          "Mixolydian", "Modal",
                          "Major scale with lowered 7th - 5th mode",
                          {0,2,4,5,7,9,10}});
    
    m_allScales.push_back({Scale("Aeolian", {0,2,3,5,7,8,10}),
                          "Aeolian", "Modal",
                          "Natural minor scale - 6th mode",
                          {0,2,3,5,7,8,10}});
    
    m_allScales.push_back({Scale("Locrian", {0,1,3,5,6,8,10}),
                          "Locrian", "Modal",
                          "Diminished scale - 7th mode",
                          {0,1,3,5,6,8,10}});

    // Harmonic Minor modes
    m_allScales.push_back({Scale("Harmonic Minor Mode 2", {0,1,3,4,6,7,9}),
                          "Locrian #6", "Modal",
                          "2nd mode of harmonic minor",
                          {0,1,3,4,6,7,9}});

    m_allScales.push_back({Scale("Harmonic Minor Mode 3", {0,2,3,5,6,8,9}),
                          "Ionian #5", "Modal",
                          "3rd mode of harmonic minor",
                          {0,2,3,5,6,8,9}});

    m_allScales.push_back({Scale("Harmonic Minor Mode 4", {0,1,3,4,6,7,10}),
                          "Ukrainian Dorian", "Modal",
                          "4th mode of harmonic minor",
                          {0,1,3,4,6,7,10}});

    m_allScales.push_back({Scale("Harmonic Minor Mode 5", {0,2,3,5,6,9,10}),
                          "Phrygian Dominant", "Modal",
                          "5th mode of harmonic minor - Spanish scale",
                          {0,2,3,5,6,9,10}});

    m_allScales.push_back({Scale("Harmonic Minor Mode 6", {0,1,3,4,7,8,11}),
                          "Lydian #2", "Modal",
                          "6th mode of harmonic minor",
                          {0,1,3,4,7,8,11}});

    m_allScales.push_back({Scale("Harmonic Minor Mode 7", {0,2,3,6,7,10,11}),
                          "Superlocrian bb7", "Modal",
                          "7th mode of harmonic minor",
                          {0,2,3,6,7,10,11}});

    // Melodic Minor modes (Jazz Minor modes)
    m_allScales.push_back({Scale("Melodic Minor Mode 2", {0,1,3,5,7,9,10}),
                          "Dorian b2", "Modal",
                          "2nd mode of melodic minor",
                          {0,1,3,5,7,9,10}});

    m_allScales.push_back({Scale("Melodic Minor Mode 3", {0,2,4,6,8,9,11}),
                          "Lydian Augmented", "Modal",
                          "3rd mode of melodic minor",
                          {0,2,4,6,8,9,11}});

    m_allScales.push_back({Scale("Melodic Minor Mode 4", {0,2,4,6,7,9,10}),
                          "Lydian Dominant", "Modal",
                          "4th mode of melodic minor - Bartok scale",
                          {0,2,4,6,7,9,10}});

    m_allScales.push_back({Scale("Melodic Minor Mode 5", {0,2,4,5,7,8,10}),
                          "Mixolydian b6", "Modal",
                          "5th mode of melodic minor - Hindu scale",
                          {0,2,4,5,7,8,10}});

    m_allScales.push_back({Scale("Melodic Minor Mode 6", {0,2,3,5,6,8,10}),
                          "Half Diminished", "Modal",
                          "6th mode of melodic minor - Locrian #2",
                          {0,2,3,5,6,8,10}});

    m_allScales.push_back({Scale("Melodic Minor Mode 7", {0,1,3,4,6,8,10}),
                          "Altered Scale", "Modal",
                          "7th mode of melodic minor - Super Locrian",
                          {0,1,3,4,6,8,10}});

    // Double Harmonic modes
    m_allScales.push_back({Scale("Double Harmonic Major", {0,1,4,5,7,8,11}),
                          "Double Harmonic Major", "Modal",
                          "Byzantine/Arabic major scale",
                          {0,1,4,5,7,8,11}});

    m_allScales.push_back({Scale("Lydian #2 #6", {0,3,4,6,7,10,11}),
                          "Lydian #2 #6", "Modal",
                          "2nd mode of double harmonic major",
                          {0,3,4,6,7,10,11}});

    m_allScales.push_back({Scale("Phrygian bb7 bb4", {0,1,4,5,6,9,10}),
                          "Phrygian bb7 bb4", "Modal",
                          "3rd mode of double harmonic major",
                          {0,1,4,5,6,9,10}});

    m_allScales.push_back({Scale("Hungarian Minor", {0,2,3,6,7,8,11}),
                          "Hungarian Minor", "Modal",
                          "4th mode of double harmonic major",
                          {0,2,3,6,7,8,11}});

    m_allScales.push_back({Scale("Oriental", {0,1,4,5,6,9,10}),
                          "Oriental", "Modal",
                          "5th mode of double harmonic major",
                          {0,1,4,5,6,9,10}});

    m_allScales.push_back({Scale("Ionian #2 #5", {0,3,4,5,8,9,11}),
                          "Ionian #2 #5", "Modal",
                          "6th mode of double harmonic major",
                          {0,3,4,5,8,9,11}});

    m_allScales.push_back({Scale("Locrian bb3 bb7", {0,1,3,5,6,8,9}),
                          "Locrian bb3 bb7", "Modal",
                          "7th mode of double harmonic major",
                          {0,1,3,5,6,8,9}});

    // Additional modal variations
    m_allScales.push_back({Scale("Dorian #4", {0,2,3,6,7,9,10}),
                          "Dorian #4", "Modal",
                          "Dorian with raised 4th - Romanian Minor",
                          {0,2,3,6,7,9,10}});

    m_allScales.push_back({Scale("Phrygian Major", {0,1,4,5,7,8,10}),
                          "Phrygian Major", "Modal",
                          "Phrygian with major 3rd - Spanish Gypsy",
                          {0,1,4,5,7,8,10}});

    m_allScales.push_back({Scale("Lydian b7", {0,2,4,6,7,9,10}),
                          "Lydian b7", "Modal",
                          "Lydian with minor 7th - Acoustic scale",
                          {0,2,4,6,7,9,10}});

    m_allScales.push_back({Scale("Mixolydian b6", {0,2,4,5,7,8,10}),
                          "Mixolydian b6", "Modal",
                          "Mixolydian with minor 6th",
                          {0,2,4,5,7,8,10}});

    m_allScales.push_back({Scale("Locrian #2", {0,2,3,5,6,8,10}),
                          "Locrian #2", "Modal",
                          "Locrian with major 2nd",
                          {0,2,3,5,6,8,10}});

    m_allScales.push_back({Scale("Locrian #6", {0,1,3,5,6,9,10}),
                          "Locrian #6", "Modal",
                          "Locrian with major 6th",
                          {0,1,3,5,6,9,10}});
}

void ScaleBrowser::ScaleBrowserContent::addJazzScales()
{
    // Bebop scales
    m_allScales.push_back({Scale("Bebop Major", {0,2,4,5,7,8,9,11}),
                          "Bebop Major", "Jazz",
                          "Major scale with added chromatic passing tone",
                          {0,2,4,5,7,8,9,11}});
    
    m_allScales.push_back({Scale("Bebop Dominant", {0,2,4,5,7,9,10,11}),
                          "Bebop Dominant", "Jazz",
                          "Mixolydian with added major 7th",
                          {0,2,4,5,7,9,10,11}});
    
    m_allScales.push_back({Scale("Bebop Minor", {0,2,3,4,5,7,9,10}),
                          "Bebop Minor", "Jazz",
                          "Dorian with added chromatic passing tone",
                          {0,2,3,4,5,7,9,10}});
    
    m_allScales.push_back({Scale("Bebop Dorian", {0,2,3,4,5,7,9,10}),
                          "Bebop Dorian", "Jazz",
                          "Dorian with added chromatic passing tone",
                          {0,2,3,4,5,7,9,10}});

    m_allScales.push_back({Scale("Bebop Harmonic Minor", {0,2,3,5,7,8,10,11}),
                          "Bebop Harmonic Minor", "Jazz",
                          "Harmonic minor with added natural 7th",
                          {0,2,3,5,7,8,10,11}});

    m_allScales.push_back({Scale("Bebop Melodic Minor", {0,2,3,5,7,8,9,11}),
                          "Bebop Melodic Minor", "Jazz",
                          "Melodic minor with added b6",
                          {0,2,3,5,7,8,9,11}});

    m_allScales.push_back({Scale("Bebop Locrian", {0,1,3,5,6,7,8,10}),
                          "Bebop Locrian", "Jazz",
                          "Locrian with added natural 7th",
                          {0,1,3,5,6,7,8,10}});
    
    // Diminished scales
    m_allScales.push_back({Scale("Diminished (W-H)", {0,2,3,5,6,8,9,11}),
                          "Whole-Half Diminished", "Jazz",
                          "Whole-half diminished scale",
                          {0,2,3,5,6,8,9,11}});
    
    m_allScales.push_back({Scale("Diminished (H-W)", {0,1,3,4,6,7,9,10}),
                          "Half-Whole Diminished", "Jazz",
                          "Half-whole diminished scale - dominant function",
                          {0,1,3,4,6,7,9,10}});

    m_allScales.push_back({Scale("Dominant Diminished", {0,1,3,4,6,7,9,10}),
                          "Dominant Diminished", "Jazz",
                          "Diminished scale for dominant chords",
                          {0,1,3,4,6,7,9,10}});

    m_allScales.push_back({Scale("Auxiliary Diminished", {0,2,3,5,6,8,9,11}),
                          "Auxiliary Diminished", "Jazz",
                          "Diminished scale for diminished chords",
                          {0,2,3,5,6,8,9,11}});
    
    // Altered scales and variations
    m_allScales.push_back({Scale("Altered", {0,1,3,4,6,8,10}),
                          "Altered", "Jazz",
                          "Super Locrian - 7th mode of melodic minor",
                          {0,1,3,4,6,8,10}});

    m_allScales.push_back({Scale("Super Locrian", {0,1,3,4,6,8,10}),
                          "Super Locrian", "Jazz",
                          "Altered dominant scale",
                          {0,1,3,4,6,8,10}});

    m_allScales.push_back({Scale("Locrian #2", {0,2,3,5,6,8,10}),
                          "Locrian #2", "Jazz",
                          "Half-diminished scale",
                          {0,2,3,5,6,8,10}});

    m_allScales.push_back({Scale("Phrygidorian", {0,1,3,5,7,8,10}),
                          "Phrygidorian", "Jazz",
                          "Phrygian with natural 6th",
                          {0,1,3,5,7,8,10}});

    // Jazz melodic minor modes (modern jazz)
    m_allScales.push_back({Scale("Jazz Minor", {0,2,3,5,7,9,11}),
                          "Jazz Minor", "Jazz",
                          "Ascending melodic minor - modern jazz standard",
                          {0,2,3,5,7,9,11}});

    m_allScales.push_back({Scale("Lydian Augmented", {0,2,4,6,8,9,11}),
                          "Lydian Augmented", "Jazz",
                          "3rd mode of jazz minor - #5 Lydian",
                          {0,2,4,6,8,9,11}});

    m_allScales.push_back({Scale("Lydian Dominant", {0,2,4,6,7,9,10}),
                          "Lydian Dominant", "Jazz",
                          "4th mode of jazz minor - overtone scale",
                          {0,2,4,6,7,9,10}});

    m_allScales.push_back({Scale("Mixolydian b6", {0,2,4,5,7,8,10}),
                          "Mixolydian b6", "Jazz",
                          "5th mode of jazz minor - Hindu scale",
                          {0,2,4,5,7,8,10}});

    // Contemporary jazz scales
    m_allScales.push_back({Scale("Lydian b7", {0,2,4,6,7,9,10}),
                          "Lydian b7", "Jazz",
                          "Acoustic scale - natural harmonics",
                          {0,2,4,6,7,9,10}});

    m_allScales.push_back({Scale("Mixolydian #4", {0,2,4,6,7,9,10}),
                          "Mixolydian #4", "Jazz",
                          "Lydian dominant - same as Lydian b7",
                          {0,2,4,6,7,9,10}});

    m_allScales.push_back({Scale("Dorian b2", {0,1,3,5,7,9,10}),
                          "Dorian b2", "Jazz",
                          "2nd mode of melodic minor - Phrygian #6",
                          {0,1,3,5,7,9,10}});

    m_allScales.push_back({Scale("Phrygian #6", {0,1,3,5,7,9,10}),
                          "Phrygian #6", "Jazz",
                          "Phrygian with major 6th",
                          {0,1,3,5,7,9,10}});

    // Hexatonic scales in jazz
    m_allScales.push_back({Scale("Major b6 Pentatonic", {0,2,4,7,8}),
                          "Major b6 Pentatonic", "Jazz",
                          "Major pentatonic with b6",
                          {0,2,4,7,8}});

    m_allScales.push_back({Scale("Dominant Pentatonic", {0,2,4,7,10}),
                          "Dominant Pentatonic", "Jazz",
                          "Pentatonic for dominant chords",
                          {0,2,4,7,10}});

    m_allScales.push_back({Scale("Kumoi", {0,2,3,7,9}),
                          "Kumoi", "Jazz",
                          "Japanese pentatonic used in jazz",
                          {0,2,3,7,9}});

    m_allScales.push_back({Scale("Ritusen", {0,2,5,7,9}),
                          "Ritusen", "Jazz",
                          "Japanese scale popular in jazz",
                          {0,2,5,7,9}});

    // Symmetrical scales
    m_allScales.push_back({Scale("Augmented", {0,3,4,7,8,11}),
                          "Augmented", "Jazz",
                          "Symmetrical scale - m3 + m2 pattern",
                          {0,3,4,7,8,11}});

    m_allScales.push_back({Scale("Six-Tone Symmetrical", {0,1,4,5,8,9}),
                          "Six-Tone Symmetrical", "Jazz",
                          "Hexatonic symmetrical scale",
                          {0,1,4,5,8,9}});

    // Scale extensions for modern jazz
    m_allScales.push_back({Scale("Chromatic Dorian", {0,1,2,3,5,7,9,10}),
                          "Chromatic Dorian", "Jazz",
                          "Dorian with added chromatic tones",
                          {0,1,2,3,5,7,9,10}});

    m_allScales.push_back({Scale("Chromatic Mixolydian", {0,2,3,4,5,7,9,10}),
                          "Chromatic Mixolydian", "Jazz",
                          "Mixolydian with added chromatic tones",
                          {0,2,3,4,5,7,9,10}});
}

void ScaleBrowser::ScaleBrowserContent::addBluesScales()
{
    // Traditional Blues
    m_allScales.push_back({Scale("Blues", {0,3,5,6,7,10}),
                          "Blues", "Blues",
                          "Minor pentatonic with added blue note",
                          {0,3,5,6,7,10}});

    m_allScales.push_back({Scale("Major Blues", {0,2,3,4,7,9}),
                          "Major Blues", "Blues",
                          "Major pentatonic with added blue note",
                          {0,2,3,4,7,9}});

    // Extended Blues Scales
    m_allScales.push_back({Scale("Blues Heptatonic", {0,2,3,4,5,7,9,10}),
                          "Blues Heptatonic", "Blues",
                          "7-note blues scale combining major and minor",
                          {0,2,3,4,5,7,9,10}});

    m_allScales.push_back({Scale("Blues Nonatonic", {0,1,2,3,4,5,6,7,8,9,10,11}),
                          "Blues Nonatonic", "Blues",
                          "9-note comprehensive blues scale",
                          {0,2,3,4,5,6,7,9,10}});

    m_allScales.push_back({Scale("Hexatonic Blues", {0,3,4,5,7,10}),
                          "Hexatonic Blues", "Blues",
                          "6-note blues with both blue notes",
                          {0,3,4,5,7,10}});

    m_allScales.push_back({Scale("Blues Bebop", {0,2,3,4,5,6,7,9,10}),
                          "Blues Bebop", "Blues",
                          "Bebop approach to blues",
                          {0,2,3,4,5,6,7,9,10}});

    // Regional Blues Variations
    m_allScales.push_back({Scale("Country Blues", {0,2,3,5,6,7,9,10}),
                          "Country Blues", "Blues",
                          "Country and folk blues tonality",
                          {0,2,3,5,6,7,9,10}});

    m_allScales.push_back({Scale("Chicago Blues", {0,3,5,6,7,8,10}),
                          "Chicago Blues", "Blues",
                          "Electric Chicago blues sound",
                          {0,3,5,6,7,8,10}});

    m_allScales.push_back({Scale("Delta Blues", {0,3,5,6,7,10}),
                          "Delta Blues", "Blues",
                          "Traditional Mississippi Delta blues",
                          {0,3,5,6,7,10}});

    m_allScales.push_back({Scale("Texas Blues", {0,3,4,5,7,10}),
                          "Texas Blues", "Blues",
                          "Texas shuffle and swing blues",
                          {0,3,4,5,7,10}});

    m_allScales.push_back({Scale("Piedmont Blues", {0,2,4,5,7,9,10}),
                          "Piedmont Blues", "Blues",
                          "East Coast fingerstyle blues",
                          {0,2,4,5,7,9,10}});

    // Jazz-Blues Hybrids
    m_allScales.push_back({Scale("Jazz Blues", {0,2,3,4,5,7,9,10,11}),
                          "Jazz Blues", "Blues",
                          "Jazz chord changes over blues",
                          {0,2,3,4,5,7,9,10,11}});

    m_allScales.push_back({Scale("Diminished Blues", {0,1,3,4,6,7,9,10}),
                          "Diminished Blues", "Blues",
                          "Blues with diminished harmony",
                          {0,1,3,4,6,7,9,10}});

    m_allScales.push_back({Scale("Augmented Blues", {0,3,4,7,8,10}),
                          "Augmented Blues", "Blues",
                          "Blues with augmented harmony",
                          {0,3,4,7,8,10}});

    // Rock and Modern Blues
    m_allScales.push_back({Scale("Rock Blues", {0,3,5,6,7,10}),
                          "Rock Blues", "Blues",
                          "Standard rock guitar blues scale",
                          {0,3,5,6,7,10}});

    m_allScales.push_back({Scale("Minor Blues", {0,3,5,6,7,10}),
                          "Minor Blues", "Blues",
                          "Natural minor with blue notes",
                          {0,3,5,6,7,10}});

    m_allScales.push_back({Scale("Dorian Blues", {0,2,3,5,6,7,9,10}),
                          "Dorian Blues", "Blues",
                          "Dorian mode with blues inflections",
                          {0,2,3,5,6,7,9,10}});

    m_allScales.push_back({Scale("Mixolydian Blues", {0,2,3,4,5,7,9,10}),
                          "Mixolydian Blues", "Blues",
                          "Mixolydian with blues notes",
                          {0,2,3,4,5,7,9,10}});

    // International Blues
    m_allScales.push_back({Scale("British Blues", {0,2,3,5,7,8,10}),
                          "British Blues", "Blues",
                          "British blues rock interpretation",
                          {0,2,3,5,7,8,10}});

    m_allScales.push_back({Scale("Celtic Blues", {0,2,3,5,7,9,10}),
                          "Celtic Blues", "Blues",
                          "Celtic modal approach to blues",
                          {0,2,3,5,7,9,10}});

    // Experimental Blues
    m_allScales.push_back({Scale("Chromatic Blues", {0,1,2,3,4,5,6,7,8,9,10,11}),
                          "Chromatic Blues", "Blues",
                          "Chromatic approach to blues harmony",
                          {0,1,2,3,4,5,6,7,8,9,10,11}});

    m_allScales.push_back({Scale("Quartal Blues", {0,3,5,8,10}),
                          "Quartal Blues", "Blues",
                          "Blues based on fourth intervals",
                          {0,3,5,8,10}});

    m_allScales.push_back({Scale("Pentatonic Blues", {0,2,3,7,10}),
                          "Pentatonic Blues", "Blues",
                          "Simplified pentatonic blues approach",
                          {0,2,3,7,10}});
}

void ScaleBrowser::ScaleBrowserContent::addMinorScales()
{
    // All minor scale variations
    m_allScales.push_back({Scale("Natural Minor", {0,2,3,5,7,8,10}),
                          "Natural Minor", "Minor",
                          "Aeolian mode - pure minor",
                          {0,2,3,5,7,8,10}});

    m_allScales.push_back({Scale("Harmonic Minor", {0,2,3,5,7,8,11}),
                          "Harmonic Minor", "Minor",
                          "Minor with raised 7th degree",
                          {0,2,3,5,7,8,11}});

    m_allScales.push_back({Scale("Melodic Minor", {0,2,3,5,7,9,11}),
                          "Melodic Minor", "Minor",
                          "Minor with raised 6th and 7th degrees",
                          {0,2,3,5,7,9,11}});

    m_allScales.push_back({Scale("Double Harmonic Minor", {0,1,3,5,7,8,11}),
                          "Double Harmonic Minor", "Minor",
                          "Harmonic minor with lowered 2nd",
                          {0,1,3,5,7,8,11}});

    m_allScales.push_back({Scale("Neapolitan Minor", {0,1,3,5,7,8,11}),
                          "Neapolitan Minor", "Minor",
                          "Minor with lowered 2nd degree",
                          {0,1,3,5,7,8,11}});

    m_allScales.push_back({Scale("Hungarian Minor", {0,2,3,6,7,8,11}),
                          "Hungarian Minor", "Minor",
                          "Harmonic minor with raised 4th",
                          {0,2,3,6,7,8,11}});

    m_allScales.push_back({Scale("Romanian Minor", {0,2,3,6,7,9,10}),
                          "Romanian Minor", "Minor",
                          "Dorian with raised 4th",
                          {0,2,3,6,7,9,10}});

    m_allScales.push_back({Scale("Ukrainian Minor", {0,2,3,6,7,9,10}),
                          "Ukrainian Minor", "Minor",
                          "Dorian #4 - Ukrainian Dorian",
                          {0,2,3,6,7,9,10}});

    m_allScales.push_back({Scale("Gypsy Minor", {0,2,3,6,7,8,10}),
                          "Gypsy Minor", "Minor",
                          "Hungarian Gypsy scale",
                          {0,2,3,6,7,8,10}});

    m_allScales.push_back({Scale("Jewish Minor", {0,1,4,5,7,8,10}),
                          "Jewish Minor", "Minor",
                          "Ahava Rabbah - Jewish liturgical scale",
                          {0,1,4,5,7,8,10}});

    // Pentatonic minor variations
    m_allScales.push_back({Scale("Minor Pentatonic", {0,3,5,7,10}),
                          "Minor Pentatonic", "Minor",
                          "Five-note minor scale",
                          {0,3,5,7,10}});

    m_allScales.push_back({Scale("Egyptian Minor Pentatonic", {0,2,5,7,10}),
                          "Egyptian Minor Pentatonic", "Minor",
                          "Ancient Egyptian minor scale",
                          {0,2,5,7,10}});

    m_allScales.push_back({Scale("Balinese Minor", {0,1,3,7,8}),
                          "Balinese Minor", "Minor",
                          "Indonesian gamelan minor scale",
                          {0,1,3,7,8}});

    m_allScales.push_back({Scale("Japanese Minor", {0,1,5,7,8}),
                          "Japanese Minor", "Minor",
                          "Traditional Japanese minor scale",
                          {0,1,5,7,8}});

    // Modal minor scales
    m_allScales.push_back({Scale("Dorian Minor", {0,2,3,5,7,9,10}),
                          "Dorian Minor", "Minor",
                          "Natural minor with raised 6th",
                          {0,2,3,5,7,9,10}});

    m_allScales.push_back({Scale("Phrygian Minor", {0,1,3,5,7,8,10}),
                          "Phrygian Minor", "Minor",
                          "Natural minor with lowered 2nd",
                          {0,1,3,5,7,8,10}});

    m_allScales.push_back({Scale("Locrian Minor", {0,1,3,5,6,8,10}),
                          "Locrian Minor", "Minor",
                          "Diminished minor scale",
                          {0,1,3,5,6,8,10}});

    // Contemporary minor scales
    m_allScales.push_back({Scale("Altered Natural Minor", {0,2,3,5,7,8,10}),
                          "Altered Natural Minor", "Minor",
                          "Natural minor with chromatic alterations",
                          {0,2,3,5,7,8,10}});

    m_allScales.push_back({Scale("Bebop Natural Minor", {0,2,3,4,5,7,8,10}),
                          "Bebop Natural Minor", "Minor",
                          "Natural minor with passing tone",
                          {0,2,3,4,5,7,8,10}});

    m_allScales.push_back({Scale("Jazz Natural Minor", {0,2,3,5,7,8,10,11}),
                          "Jazz Natural Minor", "Minor",
                          "Natural minor with added major 7th",
                          {0,2,3,5,7,8,10,11}});

    m_allScales.push_back({Scale("Rock Minor", {0,2,3,5,7,8,10}),
                          "Rock Minor", "Minor",
                          "Standard rock minor tonality",
                          {0,2,3,5,7,8,10}});
}

void ScaleBrowser::ScaleBrowserContent::addWorldScales()
{
    // Arabic/Middle Eastern Maqams
    m_allScales.push_back({Scale("Maqam Hijaz", {0,1,4,5,7,8,10}),
                          "Maqam Hijaz", "World",
                          "Most famous Arabic maqam - Spanish Phrygian",
                          {0,1,4,5,7,8,10}});
    
    m_allScales.push_back({Scale("Maqam Bayati", {0,2,3,5,7,9,10}),
                          "Maqam Bayati", "World",
                          "Popular Arabic maqam - 12-TET approximation",
                          {0,2,3,5,7,9,10}});

    m_allScales.push_back({Scale("Maqam Rast", {0,2,4,5,7,9,11}),
                          "Maqam Rast", "World",
                          "Fundamental Arabic maqam - 12-TET approximation",
                          {0,2,4,5,7,9,11}});

    m_allScales.push_back({Scale("Maqam Saba", {0,2,3,4,6,8,10}),
                          "Maqam Saba", "World",
                          "Emotional Arabic maqam - 12-TET approximation",
                          {0,2,3,4,6,8,10}});

    m_allScales.push_back({Scale("Maqam Nahawand", {0,2,3,5,7,8,10}),
                          "Maqam Nahawand", "World",
                          "Arabic natural minor",
                          {0,2,3,5,7,8,10}});

    m_allScales.push_back({Scale("Maqam Kurd", {0,1,3,5,7,8,10}),
                          "Maqam Kurd", "World",
                          "Kurdish Arabic maqam",
                          {0,1,3,5,7,8,10}});

    m_allScales.push_back({Scale("Maqam Ajam", {0,2,4,5,7,9,11}),
                          "Maqam Ajam", "World",
                          "Arabic major scale",
                          {0,2,4,5,7,9,11}});

    m_allScales.push_back({Scale("Maqam Sikah", {0,2,4,5,7,9,11}),
                          "Maqam Sikah", "World",
                          "Complex Arabic maqam - 12-TET approximation",
                          {0,2,4,5,7,9,11}});

    m_allScales.push_back({Scale("Arabic", {0,1,4,5,7,8,10}),
                          "Arabic", "World",
                          "Generic Arabic/Spanish Phrygian scale",
                          {0,1,4,5,7,8,10}});
    
    m_allScales.push_back({Scale("Double Harmonic", {0,1,4,5,7,8,11}),
                          "Double Harmonic", "World",
                          "Byzantine/Arabic scale",
                          {0,1,4,5,7,8,11}});

    m_allScales.push_back({Scale("Hijazkar", {0,1,4,5,7,8,11}),
                          "Hijazkar", "World",
                          "Arabic scale - Hijaz with major 7th",
                          {0,1,4,5,7,8,11}});
    
    // Persian/Iranian Scales
    m_allScales.push_back({Scale("Persian", {0,1,4,5,6,8,11}),
                          "Persian", "World",
                          "Traditional Persian scale",
                          {0,1,4,5,6,8,11}});

    m_allScales.push_back({Scale("Dastgah Shur", {0,2,4,5,7,9,10}),
                          "Dastgah Shur", "World",
                          "Persian modal system - 12-TET approximation",
                          {0,2,4,5,7,9,10}});

    m_allScales.push_back({Scale("Chahargah", {0,2,4,5,7,8,11}),
                          "Chahargah", "World",
                          "Persian dastgah",
                          {0,2,4,5,7,8,11}});
    
    // Japanese Scales (Traditional)
    m_allScales.push_back({Scale("Hirajoshi", {0,2,3,7,8}),
                          "Hirajoshi", "World",
                          "Japanese pentatonic scale - joyful",
                          {0,2,3,7,8}});
    
    m_allScales.push_back({Scale("In-Sen", {0,1,5,7,10}),
                          "In-Sen", "World",
                          "Japanese scale - contemplative",
                          {0,1,5,7,10}});
    
    m_allScales.push_back({Scale("Iwato", {0,1,5,6,10}),
                          "Iwato", "World",
                          "Japanese scale - ritualistic",
                          {0,1,5,6,10}});

    m_allScales.push_back({Scale("Yo", {0,2,5,7,9}),
                          "Yo", "World",
                          "Japanese pentatonic - bright",
                          {0,2,5,7,9}});

    m_allScales.push_back({Scale("Insen", {0,1,5,7,10}),
                          "Insen", "World",
                          "Japanese scale - same as In-Sen",
                          {0,1,5,7,10}});

    m_allScales.push_back({Scale("Kumoi", {0,2,3,7,9}),
                          "Kumoi", "World",
                          "Japanese pentatonic scale",
                          {0,2,3,7,9}});

    m_allScales.push_back({Scale("Kokin-Joshi", {0,1,5,7,8}),
                          "Kokin-Joshi", "World",
                          "Japanese ancient scale",
                          {0,1,5,7,8}});

    m_allScales.push_back({Scale("Hon-Kumoi-Joshi", {0,2,3,7,9}),
                          "Hon-Kumoi-Joshi", "World",
                          "Traditional Japanese scale",
                          {0,2,3,7,9}});

    m_allScales.push_back({Scale("Sakura", {0,1,5,7,8}),
                          "Sakura", "World",
                          "Cherry blossom scale - famous Japanese melody",
                          {0,1,5,7,8}});

    m_allScales.push_back({Scale("Akebono", {0,2,3,7,9}),
                          "Akebono", "World",
                          "Japanese dawn scale",
                          {0,2,3,7,9}});
    
    // Indian Ragas (Major ones)
    m_allScales.push_back({Scale("Raga Bhairav", {0,1,4,5,7,8,11}),
                          "Raga Bhairav", "World",
                          "Indian morning raga - devotional",
                          {0,1,4,5,7,8,11}});
    
    m_allScales.push_back({Scale("Raga Marwa", {0,1,4,6,7,9,11}),
                          "Raga Marwa", "World",
                          "Indian evening raga - romantic",
                          {0,1,4,6,7,9,11}});
    
    m_allScales.push_back({Scale("Raga Todi", {0,1,3,6,7,8,11}),
                          "Raga Todi", "World",
                          "Indian classical raga - intense",
                          {0,1,3,6,7,8,11}});

    m_allScales.push_back({Scale("Raga Yaman", {0,2,4,6,7,9,11}),
                          "Raga Yaman", "World",
                          "Indian evening raga - peaceful",
                          {0,2,4,6,7,9,11}});

    m_allScales.push_back({Scale("Raga Bhupali", {0,2,4,7,9}),
                          "Raga Bhupali", "World",
                          "Indian pentatonic raga - serene",
                          {0,2,4,7,9}});

    m_allScales.push_back({Scale("Raga Malkauns", {0,3,5,8,10}),
                          "Raga Malkauns", "World",
                          "Indian pentatonic raga - deep",
                          {0,3,5,8,10}});

    m_allScales.push_back({Scale("Raga Bageshri", {0,2,3,5,7,8,11}),
                          "Raga Bageshri", "World",
                          "Indian night raga - romantic",
                          {0,2,3,5,7,8,11}});

    m_allScales.push_back({Scale("Raga Kafi", {0,2,3,5,7,9,10}),
                          "Raga Kafi", "World",
                          "Indian raga - natural minor based",
                          {0,2,3,5,7,9,10}});

    m_allScales.push_back({Scale("Raga Bilawal", {0,2,4,5,7,9,11}),
                          "Raga Bilawal", "World",
                          "Indian raga - major scale based",
                          {0,2,4,5,7,9,11}});

    m_allScales.push_back({Scale("Raga Asavari", {0,2,3,5,7,8,10}),
                          "Raga Asavari", "World",
                          "Indian morning raga - natural minor",
                          {0,2,3,5,7,8,10}});

    m_allScales.push_back({Scale("Raga Bhimpalasi", {0,2,3,5,7,9,10}),
                          "Raga Bhimpalasi", "World",
                          "Indian afternoon raga - Dorian based",
                          {0,2,3,5,7,9,10}});

    m_allScales.push_back({Scale("Raga Darbari", {0,2,3,5,6,7,8,10}),
                          "Raga Darbari", "World",
                          "Indian classical raga - regal",
                          {0,2,3,5,6,7,8,10}});

    m_allScales.push_back({Scale("Raga Jog", {0,2,4,6,7,8,11}),
                          "Raga Jog", "World",
                          "Indian evening raga",
                          {0,2,4,6,7,8,11}});

    m_allScales.push_back({Scale("Raga Puriya Dhanashri", {0,1,4,6,7,8,11}),
                          "Raga Puriya Dhanashri", "World",
                          "Indian evening raga",
                          {0,1,4,6,7,8,11}});

    // Chinese Scales
    m_allScales.push_back({Scale("Chinese", {0,2,4,7,9}),
                          "Chinese", "World",
                          "Traditional Chinese pentatonic - Gong mode",
                          {0,2,4,7,9}});

    m_allScales.push_back({Scale("Chinese Shang", {0,2,5,7,10}),
                          "Chinese Shang", "World",
                          "Chinese pentatonic - Shang mode",
                          {0,2,5,7,10}});

    m_allScales.push_back({Scale("Chinese Jue", {0,3,5,8,10}),
                          "Chinese Jue", "World",
                          "Chinese pentatonic - Jue mode",
                          {0,3,5,8,10}});

    m_allScales.push_back({Scale("Chinese Zhi", {0,2,4,7,9}),
                          "Chinese Zhi", "World",
                          "Chinese pentatonic - Zhi mode",
                          {0,2,4,7,9}});

    m_allScales.push_back({Scale("Chinese Yu", {0,3,5,7,10}),
                          "Chinese Yu", "World",
                          "Chinese pentatonic - Yu mode",
                          {0,3,5,7,10}});
    
    m_allScales.push_back({Scale("Mongolian", {0,2,4,7,9}),
                          "Mongolian", "World",
                          "Traditional Mongolian pentatonic",
                          {0,2,4,7,9}});

    m_allScales.push_back({Scale("Mongolian Long Song", {0,2,5,7,9}),
                          "Mongolian Long Song", "World",
                          "Mongolian Urtiin Duu scale",
                          {0,2,5,7,9}});

    // Korean Scales
    m_allScales.push_back({Scale("Korean Minyo", {0,3,5,7,10}),
                          "Korean Minyo", "World",
                          "Korean folk song scale",
                          {0,3,5,7,10}});

    m_allScales.push_back({Scale("Korean Pansori", {0,1,3,5,7,8,10}),
                          "Korean Pansori", "World",
                          "Korean traditional opera scale",
                          {0,1,3,5,7,8,10}});

    // Indonesian/Southeast Asian
    m_allScales.push_back({Scale("Pelog", {0,1,3,7,8}),
                          "Pelog", "World",
                          "Indonesian gamelan scale",
                          {0,1,3,7,8}});

    m_allScales.push_back({Scale("Slendro", {0,2,5,7,9}),
                          "Slendro", "World",
                          "Indonesian gamelan pentatonic",
                          {0,2,5,7,9}});

    m_allScales.push_back({Scale("Balinese", {0,1,3,7,8}),
                          "Balinese", "World",
                          "Balinese gamelan scale",
                          {0,1,3,7,8}});

    m_allScales.push_back({Scale("Javanese", {0,1,3,5,7,8,10}),
                          "Javanese", "World",
                          "Traditional Javanese scale",
                          {0,1,3,5,7,8,10}});

    m_allScales.push_back({Scale("Thai", {0,2,4,5,7,9,11}),
                          "Thai", "World",
                          "Traditional Thai scale",
                          {0,2,4,5,7,9,11}});

    m_allScales.push_back({Scale("Vietnamese", {0,2,3,6,7,8,11}),
                          "Vietnamese", "World",
                          "Traditional Vietnamese scale",
                          {0,2,3,6,7,8,11}});

    // African Scales
    m_allScales.push_back({Scale("African Pentatonic", {0,2,5,7,10}),
                          "African Pentatonic", "World",
                          "Common African pentatonic scale",
                          {0,2,5,7,10}});

    m_allScales.push_back({Scale("Ethiopian Geez", {0,2,4,5,7,9,11}),
                          "Ethiopian Geez", "World",
                          "Ethiopian liturgical scale",
                          {0,2,4,5,7,9,11}});

    m_allScales.push_back({Scale("Ethiopian Ezel", {0,1,3,5,7,8,10}),
                          "Ethiopian Ezel", "World",
                          "Ethiopian mode",
                          {0,1,3,5,7,8,10}});

    m_allScales.push_back({Scale("Ethiopian Bati", {0,2,3,5,7,9,10}),
                          "Ethiopian Bati", "World",
                          "Ethiopian mode",
                          {0,2,3,5,7,9,10}});

    m_allScales.push_back({Scale("West African", {0,2,3,5,7,8,10}),
                          "West African", "World",
                          "Common West African scale",
                          {0,2,3,5,7,8,10}});

    m_allScales.push_back({Scale("Yoruba", {0,2,4,5,7,8,10}),
                          "Yoruba", "World",
                          "West African Yoruba scale",
                          {0,2,4,5,7,8,10}});

    m_allScales.push_back({Scale("Mbira", {0,2,4,7,9,11}),
                          "Mbira", "World",
                          "African thumb piano scale",
                          {0,2,4,7,9,11}});
    
    // Spanish/Flamenco Scales
    m_allScales.push_back({Scale("Spanish 8-Tone", {0,1,3,4,5,6,8,10}),
                          "Spanish 8-Tone", "World",
                          "Spanish/Flamenco octatonic scale",
                          {0,1,3,4,5,6,8,10}});
    
    m_allScales.push_back({Scale("Phrygian Dominant", {0,1,4,5,7,8,10}),
                          "Phrygian Dominant", "World",
                          "Spanish/Jewish/Arabic scale",
                          {0,1,4,5,7,8,10}});

    m_allScales.push_back({Scale("Flamenco", {0,1,4,5,7,8,10}),
                          "Flamenco", "World",
                          "Traditional flamenco scale",
                          {0,1,4,5,7,8,10}});

    m_allScales.push_back({Scale("Andalusian", {0,1,4,5,7,8,11}),
                          "Andalusian", "World",
                          "Southern Spanish scale",
                          {0,1,4,5,7,8,11}});
    
    // Hungarian/Eastern European
    m_allScales.push_back({Scale("Hungarian Minor", {0,2,3,6,7,8,11}),
                          "Hungarian Minor", "World",
                          "Harmonic minor with raised 4th",
                          {0,2,3,6,7,8,11}});
    
    m_allScales.push_back({Scale("Hungarian Major", {0,3,4,6,7,9,10}),
                          "Hungarian Major", "World",
                          "Double harmonic major",
                          {0,3,4,6,7,9,10}});

    m_allScales.push_back({Scale("Hungarian Gypsy", {0,2,3,6,7,8,10}),
                          "Hungarian Gypsy", "World",
                          "Roma/Gypsy scale - Hungary",
                          {0,2,3,6,7,8,10}});
    
    // Romanian/Balkan
    m_allScales.push_back({Scale("Romanian Minor", {0,2,3,6,7,9,10}),
                          "Romanian Minor", "World",
                          "Dorian with raised 4th",
                          {0,2,3,6,7,9,10}});

    m_allScales.push_back({Scale("Romanian Major", {0,1,4,5,7,8,11}),
                          "Romanian Major", "World",
                          "Harmonic major Romanian style",
                          {0,1,4,5,7,8,11}});

    m_allScales.push_back({Scale("Balkan", {0,1,4,5,7,8,10}),
                          "Balkan", "World",
                          "Generic Balkan folk scale",
                          {0,1,4,5,7,8,10}});

    m_allScales.push_back({Scale("Serbian", {0,2,3,6,7,8,11}),
                          "Serbian", "World",
                          "Traditional Serbian scale",
                          {0,2,3,6,7,8,11}});

    m_allScales.push_back({Scale("Bulgarian", {0,1,4,5,7,8,10}),
                          "Bulgarian", "World",
                          "Traditional Bulgarian scale",
                          {0,1,4,5,7,8,10}});
    
    // Gypsy/Roma Scales
    m_allScales.push_back({Scale("Gypsy", {0,2,3,6,7,8,10}),
                          "Gypsy", "World",
                          "Hungarian Gypsy scale",
                          {0,2,3,6,7,8,10}});

    m_allScales.push_back({Scale("Gypsy Major", {0,1,4,5,7,8,11}),
                          "Gypsy Major", "World",
                          "Roma major scale",
                          {0,1,4,5,7,8,11}});

    m_allScales.push_back({Scale("Romani", {0,1,3,6,7,8,10}),
                          "Romani", "World",
                          "Roma people scale",
                          {0,1,3,6,7,8,10}});

    // Celtic/Irish/Scottish
    m_allScales.push_back({Scale("Celtic", {0,2,4,5,7,9,10}),
                          "Celtic", "World",
                          "Traditional Celtic scale - Mixolydian",
                          {0,2,4,5,7,9,10}});

    m_allScales.push_back({Scale("Irish", {0,2,4,5,7,9,10}),
                          "Irish", "World",
                          "Traditional Irish scale",
                          {0,2,4,5,7,9,10}});

    m_allScales.push_back({Scale("Scottish", {0,2,3,5,7,9,10}),
                          "Scottish", "World",
                          "Scottish Dorian scale",
                          {0,2,3,5,7,9,10}});

    m_allScales.push_back({Scale("Highland", {0,1,4,5,7,8,10}),
                          "Highland", "World",
                          "Scottish Highland bagpipe scale",
                          {0,1,4,5,7,8,10}});

    m_allScales.push_back({Scale("Celtic Minor", {0,2,3,5,7,8,10}),
                          "Celtic Minor", "World",
                          "Celtic natural minor",
                          {0,2,3,5,7,8,10}});

    // Nordic/Scandinavian
    m_allScales.push_back({Scale("Norwegian", {0,2,4,5,7,9,10}),
                          "Norwegian", "World",
                          "Traditional Norwegian folk scale",
                          {0,2,4,5,7,9,10}});

    m_allScales.push_back({Scale("Swedish", {0,2,3,5,7,9,10}),
                          "Swedish", "World",
                          "Swedish folk scale",
                          {0,2,3,5,7,9,10}});

    m_allScales.push_back({Scale("Icelandic", {0,2,4,5,7,8,11}),
                          "Icelandic", "World",
                          "Traditional Icelandic scale",
                          {0,2,4,5,7,8,11}});

    // Eastern European
    m_allScales.push_back({Scale("Russian", {0,2,3,5,7,8,10}),
                          "Russian", "World",
                          "Traditional Russian minor scale",
                          {0,2,3,5,7,8,10}});

    m_allScales.push_back({Scale("Ukrainian", {0,2,3,6,7,9,10}),
                          "Ukrainian", "World",
                          "Ukrainian Dorian - Romanian Minor",
                          {0,2,3,6,7,9,10}});

    m_allScales.push_back({Scale("Polish", {0,2,4,6,7,9,11}),
                          "Polish", "World",
                          "Polish Lydian folk scale",
                          {0,2,4,6,7,9,11}});

    m_allScales.push_back({Scale("Czech", {0,2,4,5,7,8,10}),
                          "Czech", "World",
                          "Traditional Czech scale",
                          {0,2,4,5,7,8,10}});
    
    // Jewish Scales
    m_allScales.push_back({Scale("Jewish", {0,1,4,5,7,8,10}),
                          "Jewish", "World",
                          "Ahava Rabbah - Jewish prayer scale",
                          {0,1,4,5,7,8,10}});

    m_allScales.push_back({Scale("Ahava Rabbah", {0,1,4,5,7,8,10}),
                          "Ahava Rabbah", "World",
                          "Jewish liturgical scale - great love",
                          {0,1,4,5,7,8,10}});

    m_allScales.push_back({Scale("Freygish", {0,1,4,5,7,8,10}),
                          "Freygish", "World",
                          "Klezmer/Ashkenazi Jewish scale",
                          {0,1,4,5,7,8,10}});

    m_allScales.push_back({Scale("Misheberach", {0,1,3,5,7,8,10}),
                          "Misheberach", "World",
                          "Jewish prayer mode",
                          {0,1,3,5,7,8,10}});
    
    // Turkish/Ottoman
    m_allScales.push_back({Scale("Turkish", {0,1,4,5,7,8,10}),
                          "Turkish", "World",
                          "Turkish makam - generic",
                          {0,1,4,5,7,8,10}});

    m_allScales.push_back({Scale("Makam Hicaz", {0,1,4,5,7,8,10}),
                          "Makam Hicaz", "World",
                          "Turkish makam - similar to Arabic Hijaz",
                          {0,1,4,5,7,8,10}});

    m_allScales.push_back({Scale("Makam Ussak", {0,2,4,5,7,9,11}),
                          "Makam Ussak", "World",
                          "Turkish makam - 12-TET approximation",
                          {0,2,4,5,7,9,11}});
    
    // Neapolitan
    m_allScales.push_back({Scale("Neapolitan Major", {0,1,3,5,7,9,11}),
                          "Neapolitan Major", "World",
                          "Italian major with lowered 2nd",
                          {0,1,3,5,7,9,11}});
    
    m_allScales.push_back({Scale("Neapolitan Minor", {0,1,3,5,7,8,11}),
                          "Neapolitan Minor", "World",
                          "Italian minor with lowered 2nd",
                          {0,1,3,5,7,8,11}});

    // Latin American
    m_allScales.push_back({Scale("Brazilian", {0,2,4,6,7,9,10}),
                          "Brazilian", "World",
                          "Brazilian popular music scale",
                          {0,2,4,6,7,9,10}});

    m_allScales.push_back({Scale("Samba", {0,2,4,5,7,9,10}),
                          "Samba", "World",
                          "Brazilian samba scale",
                          {0,2,4,5,7,9,10}});

    m_allScales.push_back({Scale("Bossa Nova", {0,2,4,6,7,9,11}),
                          "Bossa Nova", "World",
                          "Brazilian bossa nova scale",
                          {0,2,4,6,7,9,11}});

    m_allScales.push_back({Scale("Argentinian", {0,1,4,5,7,8,11}),
                          "Argentinian", "World",
                          "Argentine tango scale",
                          {0,1,4,5,7,8,11}});

    m_allScales.push_back({Scale("Mexican", {0,1,4,5,7,8,10}),
                          "Mexican", "World",
                          "Traditional Mexican scale",
                          {0,1,4,5,7,8,10}});

    // Native American
    m_allScales.push_back({Scale("Native American", {0,2,5,7,9}),
                          "Native American", "World",
                          "Traditional Native American pentatonic",
                          {0,2,5,7,9}});

    m_allScales.push_back({Scale("Native American Flute", {0,2,3,5,7,8,10}),
                          "Native American Flute", "World",
                          "Traditional flute scale",
                          {0,2,3,5,7,8,10}});

    m_allScales.push_back({Scale("Pentatonic Minor 7th", {0,3,5,7,10}),
                          "Pentatonic Minor 7th", "World",
                          "Native American influenced",
                          {0,3,5,7,10}});
}

void ScaleBrowser::ScaleBrowserContent::addSyntheticScales()
{
    // Whole tone
    m_allScales.push_back({Scale("Whole Tone", {0,2,4,6,8,10}),
                          "Whole Tone", "Synthetic",
                          "All whole steps",
                          {0,2,4,6,8,10}});
    
    // Augmented
    m_allScales.push_back({Scale("Augmented", {0,3,4,7,8,11}),
                          "Augmented", "Synthetic",
                          "Alternating minor 3rd and minor 2nd",
                          {0,3,4,7,8,11}});
    
    // Prometheus
    m_allScales.push_back({Scale("Prometheus", {0,2,4,6,9,10}),
                          "Prometheus", "Synthetic",
                          "Mystic chord scale",
                          {0,2,4,6,9,10}});
    
    // Tritone
    m_allScales.push_back({Scale("Tritone", {0,1,4,6,7,10}),
                          "Tritone", "Synthetic",
                          "Two tritones a semitone apart",
                          {0,1,4,6,7,10}});
    
    // Enigmatic
    m_allScales.push_back({Scale("Enigmatic", {0,1,4,6,8,10,11}),
                          "Enigmatic", "Synthetic",
                          "Verdi's enigmatic scale",
                          {0,1,4,6,8,10,11}});
    
    // Messiaen Modes
    m_allScales.push_back({Scale("Messiaen Mode 1", {0,2,4,6,8,10}),
                          "Messiaen Mode 1", "Synthetic",
                          "Whole tone scale",
                          {0,2,4,6,8,10}});
    
    m_allScales.push_back({Scale("Messiaen Mode 2", {0,1,3,4,6,7,9,10}),
                          "Messiaen Mode 2", "Synthetic",
                          "Octatonic - half/whole diminished",
                          {0,1,3,4,6,7,9,10}});
    
    m_allScales.push_back({Scale("Messiaen Mode 3", {0,2,3,4,6,7,8,10,11}),
                          "Messiaen Mode 3", "Synthetic",
                          "Nine-note symmetric scale",
                          {0,2,3,4,6,7,8,10,11}});
    
    m_allScales.push_back({Scale("Messiaen Mode 4", {0,1,2,5,6,7,8,11}),
                          "Messiaen Mode 4", "Synthetic",
                          "Eight-note symmetric scale",
                          {0,1,2,5,6,7,8,11}});
    
    m_allScales.push_back({Scale("Messiaen Mode 5", {0,1,5,6,7,11}),
                          "Messiaen Mode 5", "Synthetic",
                          "Six-note symmetric scale",
                          {0,1,5,6,7,11}});
    
    m_allScales.push_back({Scale("Messiaen Mode 6", {0,2,4,5,6,8,10,11}),
                          "Messiaen Mode 6", "Synthetic",
                          "Eight-note symmetric scale",
                          {0,2,4,5,6,8,10,11}});
    
    m_allScales.push_back({Scale("Messiaen Mode 7", {0,1,2,3,5,6,7,8,9,11}),
                          "Messiaen Mode 7", "Synthetic",
                          "Ten-note symmetric scale",
                          {0,1,2,3,5,6,7,8,9,11}});
    
    // Other synthetic scales
    m_allScales.push_back({Scale("Leading Whole Tone", {0,2,4,6,8,10,11}),
                          "Leading Whole Tone", "Synthetic",
                          "Whole tone with leading tone",
                          {0,2,4,6,8,10,11}});
    
    m_allScales.push_back({Scale("Six Tone Symmetrical", {0,1,4,5,8,9}),
                          "Six Tone Symmetrical", "Synthetic",
                          "Symmetric hexatonic scale",
                          {0,1,4,5,8,9}});
    
    m_allScales.push_back({Scale("Ultralocrian", {0,1,3,4,6,8,9}),
                          "Ultralocrian", "Synthetic",
                          "Super diminished scale",
                          {0,1,3,4,6,8,9}});
    
    m_allScales.push_back({Scale("Superlocrian", {0,1,3,4,6,8,10}),
                          "Superlocrian", "Synthetic",
                          "Altered scale",
                          {0,1,3,4,6,8,10}});
    
    m_allScales.push_back({Scale("Composite Blues", {0,2,3,4,5,6,7,9,10,11}),
                          "Composite Blues", "Synthetic",
                          "Combined major and minor blues",
                          {0,2,3,4,5,6,7,9,10,11}});
    
    // Exotic/Mathematical
    m_allScales.push_back({Scale("Fibonacci", {0,1,2,3,5,8}),
                          "Fibonacci", "Synthetic",
                          "Based on Fibonacci sequence",
                          {0,1,2,3,5,8}});
    
    m_allScales.push_back({Scale("Prime", {0,2,3,5,7,11}),
                          "Prime", "Synthetic",
                          "Prime number intervals",
                          {0,2,3,5,7,11}});
    
    // Modern/Contemporary
    m_allScales.push_back({Scale("Bartok", {0,2,4,6,7,9,10}),
                          "Bartok", "Synthetic",
                          "Lydian dominant - acoustic scale",
                          {0,2,4,6,7,9,10}});
    
    m_allScales.push_back({Scale("Scriabin", {0,2,4,6,9,10}),
                          "Scriabin", "Synthetic",
                          "Prometheus/Mystic chord scale",
                          {0,2,4,6,9,10}});

    // Additional Symmetrical Scales
    m_allScales.push_back({Scale("Octatonic 1", {0,1,3,4,6,7,9,10}),
                          "Octatonic 1", "Synthetic",
                          "Symmetrical half-whole scale",
                          {0,1,3,4,6,7,9,10}});

    m_allScales.push_back({Scale("Octatonic 2", {0,2,3,5,6,8,9,11}),
                          "Octatonic 2", "Synthetic",
                          "Symmetrical whole-half scale",
                          {0,2,3,5,6,8,9,11}});

    // Xenharmonic Approximations
    m_allScales.push_back({Scale("19-TET Approximation", {0,1,3,4,6,7,9,10,12}),
                          "19-TET Approximation", "Synthetic",
                          "19-tone equal temperament approximation",
                          {0,1,3,4,6,7,9,10}});

    m_allScales.push_back({Scale("31-TET Approximation", {0,2,4,5,7,9,10}),
                          "31-TET Approximation", "Synthetic",
                          "31-tone equal temperament approximation",
                          {0,2,4,5,7,9,10}});

    // Spectral Scales
    m_allScales.push_back({Scale("Harmonic Series", {0,12,19,24,28,31,34,36}),
                          "Harmonic Series", "Synthetic",
                          "Natural harmonic series approximation",
                          {0,2,4,5,7,8,9,10}});

    m_allScales.push_back({Scale("Subharmonic Series", {0,2,3,4,5,7,8,10}),
                          "Subharmonic Series", "Synthetic",
                          "Subharmonic series approximation",
                          {0,2,3,4,5,7,8,10}});

    // Quartal/Quintal Harmony
    m_allScales.push_back({Scale("Quartal", {0,5,10,3,8,1,6,11}),
                          "Quartal", "Synthetic",
                          "Based on perfect fourths",
                          {0,3,5,8,10}});

    m_allScales.push_back({Scale("Quintal", {0,7,2,9,4,11,6,1}),
                          "Quintal", "Synthetic",
                          "Based on perfect fifths",
                          {0,2,4,7,9,11}});
}

void ScaleBrowser::ScaleBrowserContent::addExoticScales()
{
    // Microtonal and Just Intonation Scales
    m_allScales.push_back({Scale("Just Major", {0,2,4,5,7,9,11}),
                          "Just Major", "Exotic",
                          "Major scale in just intonation",
                          {0,2,4,5,7,9,11}});

    m_allScales.push_back({Scale("Pythagorean Major", {0,2,4,5,7,9,11}),
                          "Pythagorean Major", "Exotic",
                          "Major scale in Pythagorean tuning",
                          {0,2,4,5,7,9,11}});

    m_allScales.push_back({Scale("Bohlen-Pierce", {0,1,3,5,7,8,10}),
                          "Bohlen-Pierce", "Exotic",
                          "13-tone equal temperament subset",
                          {0,1,3,5,7,8,10}});

    m_allScales.push_back({Scale("Quarter Tone", {0,1,2,3,4,5,6,7,8,9,10,11}),
                          "Quarter Tone", "Exotic",
                          "24-tone equal temperament approximation",
                          {0,1,2,3,4,5,6,7,8,9,10,11}});

    m_allScales.push_back({Scale("Alpha", {0,2,3,4,6,7,8,10,11}),
                          "Alpha", "Exotic",
                          "Wendy Carlos Alpha scale",
                          {0,2,3,4,6,7,8,10,11}});

    m_allScales.push_back({Scale("Beta", {0,1,2,4,5,6,7,9,10,11}),
                          "Beta", "Exotic",
                          "Wendy Carlos Beta scale",
                          {0,1,2,4,5,6,7,9,10,11}});

    m_allScales.push_back({Scale("Gamma", {0,1,3,4,5,7,8,9,11}),
                          "Gamma", "Exotic",
                          "Wendy Carlos Gamma scale",
                          {0,1,3,4,5,7,8,9,11}});

    // Theoretical and Mathematical Scales
    m_allScales.push_back({Scale("Golden Ratio", {0,2,3,6,8,9}),
                          "Golden Ratio", "Exotic",
                          "Based on golden ratio proportions",
                          {0,2,3,6,8,9}});

    m_allScales.push_back({Scale("Fibonacci Sequence", {0,1,2,3,5,8}),
                          "Fibonacci Sequence", "Exotic",
                          "Intervals from Fibonacci numbers",
                          {0,1,2,3,5,8}});

    m_allScales.push_back({Scale("Prime Numbers", {0,2,3,5,7,11}),
                          "Prime Numbers", "Exotic",
                          "Based on prime number intervals",
                          {0,2,3,5,7,11}});

    m_allScales.push_back({Scale("Lucas Numbers", {0,2,3,4,7,11}),
                          "Lucas Numbers", "Exotic",
                          "Based on Lucas sequence",
                          {0,2,3,4,7,11}});

    // Xenharmonic Scales
    m_allScales.push_back({Scale("17-TET", {0,1,2,4,5,6,8,9,10,12,13,14,16}),
                          "17-TET", "Exotic",
                          "17-tone equal temperament subset",
                          {0,1,2,4,5,6,8,9,10}});

    m_allScales.push_back({Scale("22-TET", {0,2,4,5,7,9,11}),
                          "22-TET", "Exotic",
                          "22-tone equal temperament approximation",
                          {0,2,4,5,7,9,11}});

    m_allScales.push_back({Scale("53-TET", {0,5,9,13,18,22,26,31}),
                          "53-TET", "Exotic",
                          "53-tone equal temperament approximation",
                          {0,2,4,5,7,9,11}});

    // Inharmonic and Stretched Scales
    m_allScales.push_back({Scale("Stretched Octave", {0,2,4,5,7,9,12}),
                          "Stretched Octave", "Exotic",
                          "Octave stretched beyond 1200 cents",
                          {0,2,4,5,7,9,11}});

    m_allScales.push_back({Scale("Compressed Octave", {0,2,4,5,7,9,11}),
                          "Compressed Octave", "Exotic",
                          "Octave compressed below 1200 cents",
                          {0,2,4,5,7,9,10}});

    // Spectral/Timbral Scales
    m_allScales.push_back({Scale("Spectral 1", {0,2,4,6,7,9,10}),
                          "Spectral 1", "Exotic",
                          "Based on spectral analysis",
                          {0,2,4,6,7,9,10}});

    m_allScales.push_back({Scale("Spectral 2", {0,1,3,5,6,8,9,11}),
                          "Spectral 2", "Exotic",
                          "Based on formant frequencies",
                          {0,1,3,5,6,8,9,11}});

    // Psychoacoustic Scales
    m_allScales.push_back({Scale("Mel Scale", {0,2,4,6,8,9,11}),
                          "Mel Scale", "Exotic",
                          "Based on mel frequency scale",
                          {0,2,4,6,8,9,11}});

    m_allScales.push_back({Scale("Bark Scale", {0,2,3,5,7,8,10}),
                          "Bark Scale", "Exotic",
                          "Based on critical band theory",
                          {0,2,3,5,7,8,10}});

    // Atonal and Serial Scales
    m_allScales.push_back({Scale("Twelve Tone Row", {0,1,2,3,4,5,6,7,8,9,10,11}),
                          "Twelve Tone Row", "Exotic",
                          "All 12 chromatic tones - serialist",
                          {0,1,2,3,4,5,6,7,8,9,10,11}});

    m_allScales.push_back({Scale("Schoenberg Op. 25", {0,1,3,9,2,11,4,10,7,8,5,6}),
                          "Schoenberg Op. 25", "Exotic",
                          "Schoenberg twelve-tone row",
                          {0,1,3,9,2,11}});

    m_allScales.push_back({Scale("Berg Violin Concerto", {0,2,4,5,7,9,11,1,3,6,8,10}),
                          "Berg Violin Concerto", "Exotic",
                          "Alban Berg tone row",
                          {0,2,4,5,7,9,11}});

    // Electronic and Synthesizer Scales
    m_allScales.push_back({Scale("Theremin", {0,1,2,3,4,5,6,7,8,9,10,11}),
                          "Theremin", "Exotic",
                          "Continuous pitch electronic scale",
                          {0,1,2,3,4,5,6,7,8,9,10,11}});

    m_allScales.push_back({Scale("Vocoder", {0,2,4,7,9,11}),
                          "Vocoder", "Exotic",
                          "Voice synthesis formant scale",
                          {0,2,4,7,9,11}});

    m_allScales.push_back({Scale("FM Synthesis", {0,3,5,8,10}),
                          "FM Synthesis", "Exotic",
                          "Frequency modulation ratios",
                          {0,3,5,8,10}});
}

void ScaleBrowser::ScaleBrowserContent::addHistoricalScales()
{
    // Ancient Greek Modes
    m_allScales.push_back({Scale("Dorian (Greek)", {0,1,3,5,7,8,10}),
                          "Dorian (Greek)", "Historical",
                          "Ancient Greek Dorian mode",
                          {0,1,3,5,7,8,10}});

    m_allScales.push_back({Scale("Phrygian (Greek)", {0,2,4,5,7,9,11}),
                          "Phrygian (Greek)", "Historical",
                          "Ancient Greek Phrygian mode",
                          {0,2,4,5,7,9,11}});

    m_allScales.push_back({Scale("Lydian (Greek)", {0,1,3,5,7,8,10}),
                          "Lydian (Greek)", "Historical",
                          "Ancient Greek Lydian mode",
                          {0,1,3,5,7,8,10}});

    m_allScales.push_back({Scale("Mixolydian (Greek)", {0,2,3,5,7,9,10}),
                          "Mixolydian (Greek)", "Historical",
                          "Ancient Greek Mixolydian mode",
                          {0,2,3,5,7,9,10}});

    // Medieval Church Modes
    m_allScales.push_back({Scale("Protus Authentic", {0,2,4,5,7,9,11}),
                          "Protus Authentic", "Historical",
                          "Medieval Mode 1 - Dorian final",
                          {0,2,4,5,7,9,11}});

    m_allScales.push_back({Scale("Protus Plagal", {0,2,3,5,7,9,10}),
                          "Protus Plagal", "Historical",
                          "Medieval Mode 2 - Hypodorian",
                          {0,2,3,5,7,9,10}});

    m_allScales.push_back({Scale("Deuterus Authentic", {0,1,3,5,7,8,10}),
                          "Deuterus Authentic", "Historical",
                          "Medieval Mode 3 - Phrygian",
                          {0,1,3,5,7,8,10}});

    m_allScales.push_back({Scale("Deuterus Plagal", {0,2,4,5,7,8,10}),
                          "Deuterus Plagal", "Historical",
                          "Medieval Mode 4 - Hypophrygian",
                          {0,2,4,5,7,8,10}});

    m_allScales.push_back({Scale("Tritus Authentic", {0,2,4,6,7,9,11}),
                          "Tritus Authentic", "Historical",
                          "Medieval Mode 5 - Lydian",
                          {0,2,4,6,7,9,11}});

    m_allScales.push_back({Scale("Tritus Plagal", {0,2,4,5,7,9,10}),
                          "Tritus Plagal", "Historical",
                          "Medieval Mode 6 - Hypolydian",
                          {0,2,4,5,7,9,10}});

    m_allScales.push_back({Scale("Tetrardus Authentic", {0,2,4,5,7,9,10}),
                          "Tetrardus Authentic", "Historical",
                          "Medieval Mode 7 - Mixolydian",
                          {0,2,4,5,7,9,10}});

    m_allScales.push_back({Scale("Tetrardus Plagal", {0,2,3,5,7,8,10}),
                          "Tetrardus Plagal", "Historical",
                          "Medieval Mode 8 - Hypomixolydian",
                          {0,2,3,5,7,8,10}});

    // Renaissance Modes
    m_allScales.push_back({Scale("Aeolian (Renaissance)", {0,2,3,5,7,8,10}),
                          "Aeolian (Renaissance)", "Historical",
                          "Renaissance Mode 9 - Aeolian",
                          {0,2,3,5,7,8,10}});

    m_allScales.push_back({Scale("Hypoaeolian", {0,2,4,5,7,8,10}),
                          "Hypoaeolian", "Historical",
                          "Renaissance Mode 10",
                          {0,2,4,5,7,8,10}});

    m_allScales.push_back({Scale("Ionian (Renaissance)", {0,2,4,5,7,9,11}),
                          "Ionian (Renaissance)", "Historical",
                          "Renaissance Mode 11 - Ionian",
                          {0,2,4,5,7,9,11}});

    m_allScales.push_back({Scale("Hypoionian", {0,2,4,5,7,9,10}),
                          "Hypoionian", "Historical",
                          "Renaissance Mode 12",
                          {0,2,4,5,7,9,10}});

    // Temperaments and Tuning Systems
    m_allScales.push_back({Scale("Well-Tempered", {0,2,4,5,7,9,11}),
                          "Well-Tempered", "Historical",
                          "Bach's well-tempered major",
                          {0,2,4,5,7,9,11}});

    m_allScales.push_back({Scale("Mean-Tone", {0,2,4,5,7,9,11}),
                          "Mean-Tone", "Historical",
                          "Renaissance mean-tone temperament",
                          {0,2,4,5,7,9,11}});

    m_allScales.push_back({Scale("Kirnberger", {0,2,4,5,7,9,11}),
                          "Kirnberger", "Historical",
                          "18th century Kirnberger temperament",
                          {0,2,4,5,7,9,11}});

    m_allScales.push_back({Scale("Werckmeister", {0,2,4,5,7,9,11}),
                          "Werckmeister", "Historical",
                          "Baroque Werckmeister temperament",
                          {0,2,4,5,7,9,11}});

    m_allScales.push_back({Scale("Vallotti", {0,2,4,5,7,9,11}),
                          "Vallotti", "Historical",
                          "18th century Italian temperament",
                          {0,2,4,5,7,9,11}});

    // Ancient and Prehistoric Scales
    m_allScales.push_back({Scale("Pentatonic Ancient", {0,2,5,7,10}),
                          "Pentatonic Ancient", "Historical",
                          "Oldest known scale system",
                          {0,2,5,7,10}});

    m_allScales.push_back({Scale("Sumerian", {0,2,4,7,9}),
                          "Sumerian", "Historical",
                          "Ancient Mesopotamian scale",
                          {0,2,4,7,9}});

    m_allScales.push_back({Scale("Egyptian Ancient", {0,2,5,7,10}),
                          "Egyptian Ancient", "Historical",
                          "Ancient Egyptian heptatonic",
                          {0,2,5,7,10}});

    m_allScales.push_back({Scale("Babylonian", {0,2,4,5,7,9,11}),
                          "Babylonian", "Historical",
                          "Ancient Babylonian scale",
                          {0,2,4,5,7,9,11}});

    // Baroque and Classical Period
    m_allScales.push_back({Scale("Baroque Major", {0,2,4,5,7,9,11}),
                          "Baroque Major", "Historical",
                          "17th-18th century major scale",
                          {0,2,4,5,7,9,11}});

    m_allScales.push_back({Scale("Baroque Minor", {0,2,3,5,7,8,11}),
                          "Baroque Minor", "Historical",
                          "Baroque harmonic minor preferred",
                          {0,2,3,5,7,8,11}});

    m_allScales.push_back({Scale("Galant Style", {0,2,4,5,7,9,11}),
                          "Galant Style", "Historical",
                          "18th century classical style",
                          {0,2,4,5,7,9,11}});
}

void ScaleBrowser::ScaleBrowserContent::addMicrotonal()
{
    // Quarter-tone Scales
    m_allScales.push_back({Scale("24-TET Chromatic", {0,1,2,3,4,5,6,7,8,9,10,11}),
                          "24-TET Chromatic", "Microtonal",
                          "24-tone equal temperament approximation",
                          {0,1,2,3,4,5,6,7,8,9,10,11}});

    m_allScales.push_back({Scale("Quarter-tone Major", {0,2,4,5,7,9,11}),
                          "Quarter-tone Major", "Microtonal",
                          "Major scale - quarter-tone approximation",
                          {0,2,4,5,7,9,11}});

    m_allScales.push_back({Scale("Quarter-tone Minor", {0,2,3,5,7,8,10}),
                          "Quarter-tone Minor", "Microtonal",
                          "Minor scale - quarter-tone approximation",
                          {0,2,3,5,7,8,10}});

    // Just Intonation Scales
    m_allScales.push_back({Scale("5-Limit Just", {0,2,4,5,7,9,11}),
                          "5-Limit Just", "Microtonal",
                          "Just intonation with 5-limit ratios",
                          {0,2,4,5,7,9,11}});

    m_allScales.push_back({Scale("7-Limit Just", {0,2,4,5,6,7,9,10,11}),
                          "7-Limit Just", "Microtonal",
                          "Just intonation with 7-limit ratios",
                          {0,2,4,5,6,7,9,10,11}});

    m_allScales.push_back({Scale("11-Limit Just", {0,2,3,4,5,6,7,8,9,10,11}),
                          "11-Limit Just", "Microtonal",
                          "Just intonation with 11-limit ratios",
                          {0,2,3,4,5,6,7,8,9,10,11}});

    // Various Equal Temperaments
    m_allScales.push_back({Scale("19-TET", {0,1,3,4,6,7,9,11,12,14,15,17,18}),
                          "19-TET", "Microtonal",
                          "19-tone equal temperament",
                          {0,1,3,4,6,7,9,11}});

    m_allScales.push_back({Scale("31-TET", {0,2,5,7,10,12,15,17,20,22,25,27,29}),
                          "31-TET", "Microtonal",
                          "31-tone equal temperament",
                          {0,2,5,7,10}});

    m_allScales.push_back({Scale("43-TET", {0,3,7,10,14,18,21,25,29,32,36,39}),
                          "43-TET", "Microtonal",
                          "43-tone equal temperament",
                          {0,3,7,10}});

    m_allScales.push_back({Scale("53-TET", {0,5,9,13,18,22,26,31,35,40,44,48}),
                          "53-TET", "Microtonal",
                          "53-tone equal temperament",
                          {0,5,9,13}});

    // Xenharmonic Scales
    m_allScales.push_back({Scale("Bohlen-Pierce Lambda", {0,1,3,5,7,8,10,12}),
                          "Bohlen-Pierce Lambda", "Microtonal",
                          "13-tone equal temperament subset",
                          {0,1,3,5,7,8,10}});

    m_allScales.push_back({Scale("Lucy Tuning", {0,2,4,5,7,9,11}),
                          "Lucy Tuning", "Microtonal",
                          "Pi-based tuning system",
                          {0,2,4,5,7,9,11}});

    // Non-Octave Scales
    m_allScales.push_back({Scale("Golden Ratio Scale", {0,2,3,6,8,9,12}),
                          "Golden Ratio Scale", "Microtonal",
                          "Non-octave scale based on golden ratio",
                          {0,2,3,6,8,9}});

    m_allScales.push_back({Scale("Tritave Scale", {0,4,8,12,16,20}),
                          "Tritave Scale", "Microtonal",
                          "12:1 ratio instead of 2:1 octave",
                          {0,4,8}});

    // Adaptive Tuning Systems
    m_allScales.push_back({Scale("Adaptive JI Major", {0,2,4,5,7,9,11}),
                          "Adaptive JI Major", "Microtonal",
                          "Dynamically tuned just intonation",
                          {0,2,4,5,7,9,11}});

    m_allScales.push_back({Scale("Adaptive JI Minor", {0,2,3,5,7,8,10}),
                          "Adaptive JI Minor", "Microtonal",
                          "Dynamically tuned minor scale",
                          {0,2,3,5,7,8,10}});
}

void ScaleBrowser::ScaleBrowserContent::addContemporary()
{
    // Contemporary Classical
    m_allScales.push_back({Scale("Serialism", {0,1,2,3,4,5,6,7,8,9,10,11}),
                          "Serialism", "Contemporary",
                          "12-tone serial composition",
                          {0,1,2,3,4,5,6,7,8,9,10,11}});

    m_allScales.push_back({Scale("Total Serialism", {0,1,2,3,4,5,6,7,8,9,10,11}),
                          "Total Serialism", "Contemporary",
                          "All parameters serialized",
                          {0,1,2,3,4,5,6,7,8,9,10,11}});

    m_allScales.push_back({Scale("Pointillism", {0,3,6,9}),
                          "Pointillism", "Contemporary",
                          "Webern-style sparse texture",
                          {0,3,6,9}});

    m_allScales.push_back({Scale("Klangfarbenmelodie", {0,1,2,3,4,5,6,7,8,9,10,11}),
                          "Klangfarbenmelodie", "Contemporary",
                          "Tone-color melody technique",
                          {0,1,2,3,4,5,6,7,8,9,10,11}});

    // Minimalism
    m_allScales.push_back({Scale("Minimalist Diatonic", {0,2,4,5,7,9,11}),
                          "Minimalist Diatonic", "Contemporary",
                          "Reich/Glass style diatonic",
                          {0,2,4,5,7,9,11}});

    m_allScales.push_back({Scale("Process Music", {0,2,4,7,9}),
                          "Process Music", "Contemporary",
                          "Steve Reich process-based",
                          {0,2,4,7,9}});

    m_allScales.push_back({Scale("Phase Music", {0,3,7,10}),
                          "Phase Music", "Contemporary",
                          "Phasing technique scales",
                          {0,3,7,10}});

    // Spectral Music
    m_allScales.push_back({Scale("Spectral Fundamental", {0,2,4,6,7,9,10}),
                          "Spectral Fundamental", "Contemporary",
                          "Based on harmonic spectrum analysis",
                          {0,2,4,6,7,9,10}});

    m_allScales.push_back({Scale("Grisey Spectrum", {0,1,3,5,6,8,9,11}),
                          "Grisey Spectrum", "Contemporary",
                          "Gerard Grisey spectral techniques",
                          {0,1,3,5,6,8,9,11}});

    m_allScales.push_back({Scale("Murail Formants", {0,2,3,5,7,8,10}),
                          "Murail Formants", "Contemporary",
                          "Tristan Murail formant-based",
                          {0,2,3,5,7,8,10}});

    // New Complexity
    m_allScales.push_back({Scale("Ferneyhough", {0,1,2,3,4,5,6,7,8,9,10,11}),
                          "Ferneyhough", "Contemporary",
                          "Brian Ferneyhough complexity",
                          {0,1,2,3,4,5,6,7,8,9,10,11}});

    m_allScales.push_back({Scale("Barrett Polyphony", {0,1,3,4,6,7,9,10}),
                          "Barrett Polyphony", "Contemporary",
                          "Richard Barrett polyphonic writing",
                          {0,1,3,4,6,7,9,10}});

    // Extended Techniques
    m_allScales.push_back({Scale("Multiphonic", {0,2,5,7,9,11}),
                          "Multiphonic", "Contemporary",
                          "Wind instrument multiphonic scales",
                          {0,2,5,7,9,11}});

    m_allScales.push_back({Scale("String Harmonic", {0,12,19,24,28,31,34}),
                          "String Harmonic", "Contemporary",
                          "Natural string harmonics",
                          {0,2,4,5,7,8,9}});

    m_allScales.push_back({Scale("Prepared Piano", {0,1,3,5,6,8,10}),
                          "Prepared Piano", "Contemporary",
                          "John Cage prepared piano scales",
                          {0,1,3,5,6,8,10}});

    // Electronic/Computer Music
    m_allScales.push_back({Scale("Granular Synthesis", {0,1,2,3,4,5,6,7,8,9,10,11}),
                          "Granular Synthesis", "Contemporary",
                          "Computer music granular scales",
                          {0,1,2,3,4,5,6,7,8,9,10,11}});

    m_allScales.push_back({Scale("Algorithmic", {0,1,3,5,8,13}),
                          "Algorithmic", "Contemporary",
                          "Computer-generated scales",
                          {0,1,3,5,8,13}});

    m_allScales.push_back({Scale("AI Generated", {0,2,3,6,8,9,11}),
                          "AI Generated", "Contemporary",
                          "Artificial intelligence composed",
                          {0,2,3,6,8,9,11}});

    // Post-Genre Fusion
    m_allScales.push_back({Scale("World Fusion", {0,1,4,5,7,8,10}),
                          "World Fusion", "Contemporary",
                          "Contemporary world music fusion",
                          {0,1,4,5,7,8,10}});

    m_allScales.push_back({Scale("Jazz-Classical", {0,2,4,6,7,9,10,11}),
                          "Jazz-Classical", "Contemporary",
                          "Third stream movement",
                          {0,2,4,6,7,9,10,11}});

    m_allScales.push_back({Scale("Pop-Classical", {0,2,4,5,7,9,11}),
                          "Pop-Classical", "Contemporary",
                          "Crossover classical-popular",
                          {0,2,4,5,7,9,11}});
}

void ScaleBrowser::ScaleBrowserContent::addMathematical()
{
    // Number Theory Based
    m_allScales.push_back({Scale("Fibonacci", {0,1,2,3,5,8}),
                          "Fibonacci", "Mathematical",
                          "Based on Fibonacci sequence intervals",
                          {0,1,2,3,5,8}});

    m_allScales.push_back({Scale("Prime Intervals", {0,2,3,5,7,11}),
                          "Prime Intervals", "Mathematical",
                          "Intervals based on prime numbers",
                          {0,2,3,5,7,11}});

    m_allScales.push_back({Scale("Lucas Numbers", {0,2,3,4,7,11}),
                          "Lucas Numbers", "Mathematical",
                          "Based on Lucas sequence",
                          {0,2,3,4,7,11}});

    m_allScales.push_back({Scale("Catalan Numbers", {0,1,2,5,14}),
                          "Catalan Numbers", "Mathematical",
                          "Catalan sequence intervals (mod 12)",
                          {0,1,2,5}});

    m_allScales.push_back({Scale("Pascal Triangle", {0,1,3,6,10}),
                          "Pascal Triangle", "Mathematical",
                          "Triangular numbers mod 12",
                          {0,1,3,6,10}});

    // Geometric Progressions
    m_allScales.push_back({Scale("Golden Ratio", {0,2,3,6,8,9}),
                          "Golden Ratio", "Mathematical",
                          "Intervals based on φ (1.618...)",
                          {0,2,3,6,8,9}});

    m_allScales.push_back({Scale("Silver Ratio", {0,2,4,6,8,10}),
                          "Silver Ratio", "Mathematical",
                          "Based on silver ratio (1+√2)",
                          {0,2,4,6,8,10}});

    m_allScales.push_back({Scale("Bronze Ratio", {0,3,6,9}),
                          "Bronze Ratio", "Mathematical",
                          "Based on bronze ratio",
                          {0,3,6,9}});

    m_allScales.push_back({Scale("Plastic Number", {0,1,4,5,8,9}),
                          "Plastic Number", "Mathematical",
                          "Based on plastic number ratio",
                          {0,1,4,5,8,9}});

    // Fractal and Chaos Theory
    m_allScales.push_back({Scale("Mandelbrot Set", {0,1,4,9,16}),
                          "Mandelbrot Set", "Mathematical",
                          "Based on Mandelbrot iteration (mod 12)",
                          {0,1,4,9}});

    m_allScales.push_back({Scale("Julia Set", {0,2,8,14,20}),
                          "Julia Set", "Mathematical",
                          "Julia set iteration values (mod 12)",
                          {0,2,8}});

    m_allScales.push_back({Scale("Lorenz Attractor", {0,3,7,10}),
                          "Lorenz Attractor", "Mathematical",
                          "Chaotic system quantized",
                          {0,3,7,10}});

    m_allScales.push_back({Scale("Sierpinski Triangle", {0,3,6,9}),
                          "Sierpinski Triangle", "Mathematical",
                          "Fractal triangle pattern",
                          {0,3,6,9}});

    // Group Theory and Algebra
    m_allScales.push_back({Scale("Cyclic Group Z12", {0,1,2,3,4,5,6,7,8,9,10,11}),
                          "Cyclic Group Z12", "Mathematical",
                          "Complete cyclic group of order 12",
                          {0,1,2,3,4,5,6,7,8,9,10,11}});

    m_allScales.push_back({Scale("Dihedral Group", {0,2,4,6,8,10}),
                          "Dihedral Group", "Mathematical",
                          "Symmetry group of hexagon",
                          {0,2,4,6,8,10}});

    m_allScales.push_back({Scale("Klein Four-Group", {0,3,6,9}),
                          "Klein Four-Group", "Mathematical",
                          "Non-cyclic group of order 4",
                          {0,3,6,9}});

    m_allScales.push_back({Scale("Symmetric Group", {0,1,3,6,10}),
                          "Symmetric Group", "Mathematical",
                          "Permutation group elements",
                          {0,1,3,6,10}});

    // Modular Arithmetic
    m_allScales.push_back({Scale("Mod 3 Residues", {0,3,6,9}),
                          "Mod 3 Residues", "Mathematical",
                          "Congruence classes modulo 3",
                          {0,3,6,9}});

    m_allScales.push_back({Scale("Mod 4 Residues", {0,3,6,9}),
                          "Mod 4 Residues", "Mathematical",
                          "Congruence classes modulo 4",
                          {0,3,6,9}});

    m_allScales.push_back({Scale("Mod 5 Residues", {0,2,5,7,10}),
                          "Mod 5 Residues", "Mathematical",
                          "Congruence classes modulo 5",
                          {0,2,5,7,10}});

    // Topology and Geometry
    m_allScales.push_back({Scale("Möbius Strip", {0,6}),
                          "Möbius Strip", "Mathematical",
                          "One-sided surface representation",
                          {0,6}});

    m_allScales.push_back({Scale("Klein Bottle", {0,3,6,9}),
                          "Klein Bottle", "Mathematical",
                          "Non-orientable surface",
                          {0,3,6,9}});

    m_allScales.push_back({Scale("Hypercube", {0,1,2,4,7,8,11}),
                          "Hypercube", "Mathematical",
                          "4D cube projection to 12-TET",
                          {0,1,2,4,7,8,11}});

    // Information Theory
    m_allScales.push_back({Scale("Maximum Entropy", {0,1,2,3,4,5,6,7,8,9,10,11}),
                          "Maximum Entropy", "Mathematical",
                          "Uniform distribution - maximum entropy",
                          {0,1,2,3,4,5,6,7,8,9,10,11}});

    m_allScales.push_back({Scale("Shannon Coding", {0,2,3,5,8,13}),
                          "Shannon Coding", "Mathematical",
                          "Optimal coding theory intervals (mod 12)",
                          {0,2,3,5,8}});

    m_allScales.push_back({Scale("Huffman Tree", {0,1,3,7,15}),
                          "Huffman Tree", "Mathematical",
                          "Huffman coding structure (mod 12)",
                          {0,1,3,7}});

    // Cellular Automata
    m_allScales.push_back({Scale("Rule 30", {0,1,3,4,6,7,9,10}),
                          "Rule 30", "Mathematical",
                          "Wolfram cellular automaton Rule 30",
                          {0,1,3,4,6,7,9,10}});

    m_allScales.push_back({Scale("Rule 110", {0,2,3,5,6,8,9,11}),
                          "Rule 110", "Mathematical",
                          "Turing-complete cellular automaton",
                          {0,2,3,5,6,8,9,11}});

    m_allScales.push_back({Scale("Conway Life", {0,1,4,5,8,9}),
                          "Conway Life", "Mathematical",
                          "Game of Life stable patterns",
                          {0,1,4,5,8,9}});
}

void ScaleBrowser::ScaleBrowserContent::updateFilteredScales()
{
    m_filteredScales.clear();
    
    for (const auto& entry : m_allScales)
    {
        if (matchesCategory(entry) && matchesSearch(entry))
        {
            m_filteredScales.push_back(entry);
        }
    }
    
    m_selectedIndex = m_filteredScales.empty() ? -1 : 0;
    m_scaleList->updateContent();
    repaint();
}

bool ScaleBrowser::ScaleBrowserContent::matchesSearch(const ScaleEntry& entry) const
{
    if (m_searchText.isEmpty())
        return true;
    
    juce::String searchLower = m_searchText.toLowerCase();
    return entry.name.toLowerCase().contains(searchLower) ||
           entry.category.toLowerCase().contains(searchLower) ||
           entry.description.toLowerCase().contains(searchLower);
}

bool ScaleBrowser::ScaleBrowserContent::matchesCategory(const ScaleEntry& entry) const
{
    if (m_currentCategory == ScaleCategory::All)
        return true;
    
    return entry.category == getCategoryName(m_currentCategory);
}

juce::String ScaleBrowser::ScaleBrowserContent::getCategoryName(ScaleCategory cat) const
{
    switch (cat)
    {
        case ScaleCategory::All: return "All";
        case ScaleCategory::Common: return "Common";
        case ScaleCategory::Modal: return "Modal";
        case ScaleCategory::Jazz: return "Jazz";
        case ScaleCategory::World: return "World";
        case ScaleCategory::Synthetic: return "Synthetic";
        case ScaleCategory::Custom: return "Custom";
        default: return "Unknown";
    }
}

juce::String ScaleBrowser::ScaleBrowserContent::getIntervalString(const std::vector<int>& intervals) const
{
    juce::String result;
    for (size_t i = 0; i < intervals.size(); ++i)
    {
        if (i > 0) result += "-";
        result += juce::String(intervals[i]);
    }
    return result;
}

} // namespace UI
} // namespace HAM