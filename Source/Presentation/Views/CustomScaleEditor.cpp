/*
  ==============================================================================

    CustomScaleEditor.cpp
    Implementation of custom scale editor

  ==============================================================================
*/

#include "CustomScaleEditor.h"

namespace HAM {
namespace UI {

//==============================================================================
// InteractivePianoKeyboard implementation
//==============================================================================
CustomScaleEditor::ScaleEditorContent::InteractivePianoKeyboard::InteractivePianoKeyboard()
{
    // Initialize key layout (C major scale pattern)
    const bool blackKeyPattern[12] = {
        false, true, false, true, false, false,  // C, C#, D, D#, E, F
        true, false, true, false, true, false     // F#, G, G#, A, A#, B
    };
    
    for (int i = 0; i < 12; ++i)
    {
        m_keys[i].isBlackKey = blackKeyPattern[i];
    }
}

void CustomScaleEditor::ScaleEditorContent::InteractivePianoKeyboard::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Background
    g.setColour(juce::Colour(0xFF1A1A1A));
    g.fillRoundedRectangle(bounds, 5.0f);
    
    // Draw white keys first
    for (int i = 0; i < 12; ++i)
    {
        if (!m_keys[i].isBlackKey)
        {
            drawWhiteKey(g, m_keys[i], i);
        }
    }
    
    // Draw black keys on top
    for (int i = 0; i < 12; ++i)
    {
        if (m_keys[i].isBlackKey)
        {
            drawBlackKey(g, m_keys[i], i);
        }
    }
    
    // Draw root note indicator
    if (m_rootNote >= 0 && m_rootNote < 12)
    {
        auto& rootKey = m_keys[m_rootNote];
        g.setColour(juce::Colour(0xFFFFAA00).withAlpha(0.5f));  // Gold
        g.drawRoundedRectangle(rootKey.bounds, 2.0f, 2.0f);
    }
}

void CustomScaleEditor::ScaleEditorContent::InteractivePianoKeyboard::resized()
{
    auto bounds = getLocalBounds().toFloat();
    bounds = bounds.reduced(5.0f);
    
    // Calculate white key positions
    float whiteKeyWidth = bounds.getWidth() / 7.0f;  // 7 white keys in octave
    float whiteKeyHeight = bounds.getHeight();
    
    int whiteKeyIndex = 0;
    for (int i = 0; i < 12; ++i)
    {
        if (!m_keys[i].isBlackKey)
        {
            m_keys[i].bounds = juce::Rectangle<float>(
                bounds.getX() + whiteKeyIndex * whiteKeyWidth,
                bounds.getY(),
                whiteKeyWidth - 2,  // Small gap between keys
                whiteKeyHeight
            );
            whiteKeyIndex++;
        }
    }
    
    // Calculate black key positions
    float blackKeyWidth = whiteKeyWidth * 0.6f;
    float blackKeyHeight = whiteKeyHeight * 0.65f;
    
    // Position black keys between white keys
    m_keys[1].bounds = m_keys[0].bounds.withWidth(blackKeyWidth)
                                       .withHeight(blackKeyHeight)
                                       .translated(whiteKeyWidth * 0.7f, 0);
    
    m_keys[3].bounds = m_keys[2].bounds.withWidth(blackKeyWidth)
                                       .withHeight(blackKeyHeight)
                                       .translated(whiteKeyWidth * 0.7f, 0);
    
    m_keys[6].bounds = m_keys[5].bounds.withWidth(blackKeyWidth)
                                       .withHeight(blackKeyHeight)
                                       .translated(whiteKeyWidth * 0.7f, 0);
    
    m_keys[8].bounds = m_keys[7].bounds.withWidth(blackKeyWidth)
                                       .withHeight(blackKeyHeight)
                                       .translated(whiteKeyWidth * 0.7f, 0);
    
    m_keys[10].bounds = m_keys[9].bounds.withWidth(blackKeyWidth)
                                        .withHeight(blackKeyHeight)
                                        .translated(whiteKeyWidth * 0.7f, 0);
}

void CustomScaleEditor::ScaleEditorContent::InteractivePianoKeyboard::drawWhiteKey(
    juce::Graphics& g, const KeyInfo& key, int noteNum)
{
    // Key background
    juce::Colour keyColor = key.isSelected ? 
        juce::Colour(0xFF00FF88).withAlpha(0.8f) :  // Mint when selected
        (key.isHovered ? 
            juce::Colours::white.withAlpha(0.95f) :
            juce::Colours::white.withAlpha(0.9f));
    
    g.setColour(keyColor);
    g.fillRect(key.bounds);
    
    // Key border
    g.setColour(juce::Colours::black);
    g.drawRect(key.bounds, 1.0f);
    
    // Note name
    const char* noteNames[] = {"C", "D", "E", "F", "G", "A", "B"};
    int whiteNoteIndex = 0;
    for (int i = 0; i <= noteNum; ++i)
    {
        if (!m_keys[i].isBlackKey && i < noteNum)
            whiteNoteIndex++;
    }
    
    g.setFont(10.0f);
    g.setColour(juce::Colours::black);
    g.drawText(noteNames[whiteNoteIndex % 7],
              key.bounds.removeFromBottom(20),
              juce::Justification::centred);
    
    // Cent offset indicator
    if (key.isSelected && std::abs(key.centOffset) > 0.01f)
    {
        g.setFont(8.0f);
        g.setColour(juce::Colour(0xFF0088FF));
        juce::String centText = (key.centOffset > 0 ? "+" : "") + 
                               juce::String(key.centOffset, 1) + "¢";
        g.drawText(centText, key.bounds.removeFromTop(20),
                  juce::Justification::centred);
    }
}

void CustomScaleEditor::ScaleEditorContent::InteractivePianoKeyboard::drawBlackKey(
    juce::Graphics& g, const KeyInfo& key, int noteNum)
{
    // Key background
    juce::Colour keyColor = key.isSelected ?
        juce::Colour(0xFF00FF88).withAlpha(0.9f) :  // Mint when selected
        (key.isHovered ?
            juce::Colours::black.withAlpha(0.8f) :
            juce::Colours::black.withAlpha(0.9f));
    
    g.setColour(keyColor);
    g.fillRect(key.bounds);
    
    // Key border
    g.setColour(juce::Colour(0xFF3A3A3A));
    g.drawRect(key.bounds, 0.5f);
    
    // Cent offset indicator for black keys
    if (key.isSelected && std::abs(key.centOffset) > 0.01f)
    {
        g.setFont(7.0f);
        g.setColour(juce::Colour(0xFF00DDFF));
        juce::String centText = (key.centOffset > 0 ? "+" : "") +
                               juce::String(key.centOffset, 1);
        g.drawText(centText, key.bounds.reduced(2),
                  juce::Justification::centredTop);
    }
}

void CustomScaleEditor::ScaleEditorContent::InteractivePianoKeyboard::mouseDown(
    const juce::MouseEvent& event)
{
    int keyIndex = getKeyAtPosition(event.getPosition());
    if (keyIndex >= 0)
    {
        // Toggle selection
        m_keys[keyIndex].isSelected = !m_keys[keyIndex].isSelected;
        
        if (onNoteToggled)
            onNoteToggled(keyIndex);
        
        if (onNotePreview && m_keys[keyIndex].isSelected)
            onNotePreview(keyIndex);
        
        repaint();
    }
}

int CustomScaleEditor::ScaleEditorContent::InteractivePianoKeyboard::getKeyAtPosition(
    juce::Point<int> pos) const
{
    // Check black keys first (they're on top)
    for (int i = 0; i < 12; ++i)
    {
        if (m_keys[i].isBlackKey && m_keys[i].bounds.contains(pos.toFloat()))
            return i;
    }
    
    // Then check white keys
    for (int i = 0; i < 12; ++i)
    {
        if (!m_keys[i].isBlackKey && m_keys[i].bounds.contains(pos.toFloat()))
            return i;
    }
    
    return -1;
}

std::vector<int> CustomScaleEditor::ScaleEditorContent::InteractivePianoKeyboard::getSelectedNotes() const
{
    std::vector<int> selected;
    for (int i = 0; i < 12; ++i)
    {
        if (m_keys[i].isSelected)
            selected.push_back(i);
    }
    return selected;
}

//==============================================================================
// IntervalDisplay implementation
//==============================================================================
CustomScaleEditor::ScaleEditorContent::IntervalDisplay::IntervalDisplay()
{
}

void CustomScaleEditor::ScaleEditorContent::IntervalDisplay::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Background
    g.setColour(juce::Colour(0xFF0A0A0A));
    g.fillRoundedRectangle(bounds, 5.0f);
    
    // Border
    g.setColour(juce::Colour(0xFF3A3A3A));
    g.drawRoundedRectangle(bounds, 5.0f, 1.0f);
    
    if (m_intervals.empty())
    {
        g.setFont(14.0f);
        g.setColour(juce::Colours::white.withAlpha(0.5f));
        g.drawText("Select notes to see intervals",
                  bounds, juce::Justification::centred);
        return;
    }
    
    // Draw intervals
    float x = 10;
    float y = 10;
    
    g.setFont(12.0f);
    for (size_t i = 0; i < m_intervals.size(); ++i)
    {
        int interval = m_intervals[i];
        
        // Interval box
        auto intervalBounds = juce::Rectangle<float>(x, y, 80, 30);
        
        g.setColour(getIntervalColor(interval).withAlpha(0.2f));
        g.fillRoundedRectangle(intervalBounds, 3.0f);
        
        g.setColour(getIntervalColor(interval));
        g.drawRoundedRectangle(intervalBounds, 3.0f, 1.0f);
        
        // Interval text
        g.setColour(juce::Colours::white);
        g.drawText(getIntervalName(interval),
                  intervalBounds,
                  juce::Justification::centred);
        
        x += 90;
        if (x > bounds.getWidth() - 100)
        {
            x = 10;
            y += 40;
        }
    }
}

void CustomScaleEditor::ScaleEditorContent::IntervalDisplay::setIntervals(
    const std::vector<int>& intervals)
{
    m_intervals = intervals;
    repaint();
}

juce::String CustomScaleEditor::ScaleEditorContent::IntervalDisplay::getIntervalName(
    int semitones) const
{
    switch (semitones)
    {
        case 1: return "m2";   // Minor second
        case 2: return "M2";   // Major second
        case 3: return "m3";   // Minor third
        case 4: return "M3";   // Major third
        case 5: return "P4";   // Perfect fourth
        case 6: return "TT";   // Tritone
        case 7: return "P5";   // Perfect fifth
        case 8: return "m6";   // Minor sixth
        case 9: return "M6";   // Major sixth
        case 10: return "m7";  // Minor seventh
        case 11: return "M7";  // Major seventh
        case 12: return "P8";  // Octave
        default: return juce::String(semitones) + "st";
    }
}

juce::Colour CustomScaleEditor::ScaleEditorContent::IntervalDisplay::getIntervalColor(
    int semitones) const
{
    switch (semitones)
    {
        case 4:  // Major third
        case 5:  // Perfect fourth
        case 7:  // Perfect fifth
        case 12: // Octave
            return juce::Colour(0xFF00FF88);  // Mint - consonant
        
        case 2:  // Major second
        case 9:  // Major sixth
        case 11: // Major seventh
            return juce::Colour(0xFF00AAFF);  // Blue - mildly consonant
        
        case 1:  // Minor second
        case 6:  // Tritone
            return juce::Colour(0xFFFF4444);  // Red - dissonant
        
        default:
            return juce::Colour(0xFF888888);  // Gray - neutral
    }
}

//==============================================================================
// ScaleEditorContent implementation
//==============================================================================
CustomScaleEditor::ScaleEditorContent::ScaleEditorContent()
    : PulseComponent("CustomScaleEditor")
{
    // Name editor
    m_nameEditor = std::make_unique<juce::TextEditor>();
    m_nameEditor->setText("Custom Scale");
    m_nameEditor->setFont(16.0f);
    addAndMakeVisible(m_nameEditor.get());
    
    // Piano keyboard
    m_keyboard = std::make_unique<InteractivePianoKeyboard>();
    m_keyboard->onNoteToggled = [this](int note) {
        updateIntervalDisplay();
    };
    addAndMakeVisible(m_keyboard.get());
    
    // Interval display
    m_intervalDisplay = std::make_unique<IntervalDisplay>();
    addAndMakeVisible(m_intervalDisplay.get());
    
    // Template dropdown
    m_templateDropdown = std::make_unique<PulseDropdown>("Templates");
    m_templateDropdown->addItem("Major", 1);
    m_templateDropdown->addItem("Natural Minor", 2);
    m_templateDropdown->addItem("Harmonic Minor", 3);
    m_templateDropdown->addItem("Melodic Minor", 4);
    m_templateDropdown->addItem("Pentatonic Major", 5);
    m_templateDropdown->addItem("Pentatonic Minor", 6);
    m_templateDropdown->addItem("Blues", 7);
    m_templateDropdown->addItem("Whole Tone", 8);
    m_templateDropdown->addItem("Chromatic", 9);
    m_templateDropdown->onChange = [this]() {
        auto selectedId = m_templateDropdown->getSelectedId();
        if (selectedId > 0)
        {
            loadTemplate(m_templateDropdown->getItemText(selectedId - 1));
        }
    };
    addAndMakeVisible(m_templateDropdown.get());
    
    // Cent adjustment slider
    m_centSlider = std::make_unique<PulseHorizontalSlider>("Cents");
    m_centSlider->setRange(-50.0, 50.0, 1.0);
    m_centSlider->setValue(0.0);
    m_centSlider->onValueChange = [this](double value) {
        if (m_selectedNoteForCents >= 0)
        {
            m_keyboard->setCentOffset(m_selectedNoteForCents, static_cast<float>(value));
        }
    };
    addAndMakeVisible(m_centSlider.get());
    
    m_centLabel = std::make_unique<juce::Label>("", "Microtonal adjustment (cents)");
    m_centLabel->setFont(12.0f);
    m_centLabel->setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.7f));
    addAndMakeVisible(m_centLabel.get());
    
    // Control buttons
    m_previewButton = std::make_unique<PulseButton>("Preview", PulseButton::Outline);
    m_previewButton->onClick = [this]() { previewScale(); };
    addAndMakeVisible(m_previewButton.get());
    
    m_clearButton = std::make_unique<PulseButton>("Clear", PulseButton::Ghost);
    m_clearButton->onClick = [this]() {
        m_keyboard->clearSelection();
        updateIntervalDisplay();
    };
    addAndMakeVisible(m_clearButton.get());
    
    m_cancelButton = std::make_unique<PulseButton>("Cancel", PulseButton::Outline);
    m_cancelButton->onClick = [this]() {
        if (onCancelClicked)
            onCancelClicked();
    };
    addAndMakeVisible(m_cancelButton.get());
    
    m_saveButton = std::make_unique<PulseButton>("Save Scale", PulseButton::Solid);
    m_saveButton->onClick = [this]() {
        if (onSaveClicked)
            onSaveClicked();
    };
    addAndMakeVisible(m_saveButton.get());
}

CustomScaleEditor::ScaleEditorContent::~ScaleEditorContent()
{
}

void CustomScaleEditor::ScaleEditorContent::paint(juce::Graphics& g)
{
    // Background
    g.fillAll(juce::Colour(0xFF000000));
    
    // Title
    g.setFont(18.0f);
    g.setColour(juce::Colours::white);
    g.drawText("Custom Scale Editor", 10, 10, getWidth() - 20, 30,
              juce::Justification::centred);
}

void CustomScaleEditor::ScaleEditorContent::resized()
{
    auto bounds = getLocalBounds();
    
    // Title area
    bounds.removeFromTop(50);
    
    // Name editor
    auto nameBounds = bounds.removeFromTop(40).reduced(20, 5);
    m_nameEditor->setBounds(nameBounds);
    
    // Template dropdown
    auto templateBounds = bounds.removeFromTop(40).reduced(20, 5);
    m_templateDropdown->setBounds(templateBounds);
    
    // Piano keyboard
    auto keyboardBounds = bounds.removeFromTop(150).reduced(20, 10);
    m_keyboard->setBounds(keyboardBounds);
    
    // Interval display
    auto intervalBounds = bounds.removeFromTop(100).reduced(20, 5);
    m_intervalDisplay->setBounds(intervalBounds);
    
    // Cent adjustment
    auto centBounds = bounds.removeFromTop(60).reduced(20, 5);
    m_centLabel->setBounds(centBounds.removeFromTop(20));
    m_centSlider->setBounds(centBounds);
    
    // Buttons
    auto buttonBounds = bounds.removeFromBottom(50);
    int buttonWidth = 100;
    int spacing = 10;
    
    m_previewButton->setBounds(buttonBounds.removeFromLeft(buttonWidth).reduced(5));
    buttonBounds.removeFromLeft(spacing);
    
    m_clearButton->setBounds(buttonBounds.removeFromLeft(buttonWidth).reduced(5));
    
    m_saveButton->setBounds(buttonBounds.removeFromRight(buttonWidth).reduced(5));
    buttonBounds.removeFromRight(spacing);
    
    m_cancelButton->setBounds(buttonBounds.removeFromRight(buttonWidth).reduced(5));
}

void CustomScaleEditor::ScaleEditorContent::updateIntervalDisplay()
{
    auto selectedNotes = m_keyboard->getSelectedNotes();
    
    std::vector<int> intervals;
    if (!selectedNotes.empty())
    {
        // Calculate intervals from root
        for (size_t i = 1; i < selectedNotes.size(); ++i)
        {
            intervals.push_back(selectedNotes[i] - selectedNotes[0]);
        }
    }
    
    m_intervalDisplay->setIntervals(intervals);
}

void CustomScaleEditor::ScaleEditorContent::loadTemplate(const juce::String& templateName)
{
    m_keyboard->clearSelection();
    
    // Define template scales
    std::vector<int> notes;
    
    if (templateName == "Major")
        notes = {0, 2, 4, 5, 7, 9, 11};  // C D E F G A B
    else if (templateName == "Natural Minor")
        notes = {0, 2, 3, 5, 7, 8, 10};  // C D Eb F G Ab Bb
    else if (templateName == "Harmonic Minor")
        notes = {0, 2, 3, 5, 7, 8, 11};  // C D Eb F G Ab B
    else if (templateName == "Melodic Minor")
        notes = {0, 2, 3, 5, 7, 9, 11};  // C D Eb F G A B
    else if (templateName == "Pentatonic Major")
        notes = {0, 2, 4, 7, 9};         // C D E G A
    else if (templateName == "Pentatonic Minor")
        notes = {0, 3, 5, 7, 10};        // C Eb F G Bb
    else if (templateName == "Blues")
        notes = {0, 3, 5, 6, 7, 10};     // C Eb F F# G Bb
    else if (templateName == "Whole Tone")
        notes = {0, 2, 4, 6, 8, 10};     // C D E F# G# A#
    else if (templateName == "Chromatic")
        notes = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};  // All notes
    
    // Apply template
    for (int note : notes)
    {
        m_keyboard->setNoteSelected(note, true);
    }
    
    m_nameEditor->setText(templateName);
    updateIntervalDisplay();
}

Scale CustomScaleEditor::ScaleEditorContent::getScale() const
{
    Scale scale;
    auto notes = m_keyboard->getSelectedNotes();
    
    // Convert selected notes to scale intervals
    if (!notes.empty())
    {
        scale.setRootNote(notes[0]);
        
        std::vector<int> intervals;
        for (size_t i = 0; i < notes.size(); ++i)
        {
            intervals.push_back(notes[i]);
        }
        scale.setIntervals(intervals);
        
        // Apply cent offsets if any
        for (int note : notes)
        {
            float cents = m_keyboard->getCentOffset(note);
            if (std::abs(cents) > 0.01f)
            {
                // Store microtonal adjustment (implementation depends on Scale class)
                // scale.setCentOffset(note, cents);
            }
        }
    }
    
    return scale;
}

void CustomScaleEditor::ScaleEditorContent::previewScale()
{
    // TODO: Send MIDI preview of the scale
    auto selectedNotes = m_keyboard->getSelectedNotes();
    
    DBG("Preview scale with " << selectedNotes.size() << " notes");
    for (int note : selectedNotes)
    {
        DBG("  Note: " << note);
    }
}

//==============================================================================
// CustomScaleEditor implementation
//==============================================================================
CustomScaleEditor::CustomScaleEditor(const juce::String& initialScaleName)
    : DialogWindow("Custom Scale Editor", 
                   juce::Colour(0xFF1A1A1A),
                   true,
                   true)
{
    m_content = std::make_unique<ScaleEditorContent>();
    
    m_content->onSaveClicked = [this]() {
        if (onScaleCreated)
        {
            onScaleCreated(m_content->getScale(), m_content->getScaleName());
        }
        closeButtonPressed();
    };
    
    m_content->onCancelClicked = [this]() {
        closeButtonPressed();
    };
    
    setContentOwned(m_content.get(), false);
    
    // Set dialog size
    centreWithSize(700, 600);
    
    // Make visible
    setVisible(true);
}

CustomScaleEditor::~CustomScaleEditor()
{
}

void CustomScaleEditor::closeButtonPressed()
{
    setVisible(false);
    
    // Auto-delete after closing
    delete this;
}

bool CustomScaleEditor::keyPressed(const juce::KeyPress& key)
{
    if (key.getKeyCode() == juce::KeyPress::escapeKey)
    {
        closeButtonPressed();
        return true;
    }
    
    return false;
}

void CustomScaleEditor::showScaleEditor(
    std::function<void(const Scale&, const juce::String&)> callback)
{
    auto* editor = new CustomScaleEditor();
    editor->onScaleCreated = callback;
}

} // namespace UI
} // namespace HAM