/*
  ==============================================================================

    PulseScaleSwitcher.cpp
    Implementation of Pulse-style Scale Slot Manager

  ==============================================================================
*/

#include "PulseScaleSwitcher.h"

namespace HAM {
namespace UI {

//==============================================================================
// SlotButton implementation
//==============================================================================
PulseScaleSwitcher::SlotButton::SlotButton(int index)
    : m_index(index)
{
    setWantsKeyboardFocus(false);
}

void PulseScaleSwitcher::SlotButton::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Background - dark with subtle gradient
    auto bgColor = juce::Colour(0xFF1A1A1A);
    if (m_isPending)
    {
        // Flashing animation for pending change
        float flash = (std::sin(juce::Time::getMillisecondCounterHiRes() * 0.008f) + 1.0f) * 0.5f;
        bgColor = bgColor.interpolatedWith(getSlotColor(), flash * 0.3f);
    }
    else if (m_isActive)
    {
        bgColor = juce::Colour(0xFF2A2A2A);
    }
    
    g.setColour(bgColor);
    g.fillRoundedRectangle(bounds, 3.0f);
    
    // Border
    float borderWidth = m_isActive ? 2.0f : 1.0f;
    auto borderColor = m_isActive ? getSlotColor() : 
                      m_isHovered ? juce::Colour(0xFF606060) : 
                      juce::Colour(0xFF404040);
    
    g.setColour(borderColor);
    g.drawRoundedRectangle(bounds.reduced(borderWidth * 0.5f), 3.0f, borderWidth);
    
    // Active indicator (small LED-style dot)
    if (m_isActive)
    {
        auto ledBounds = bounds.removeFromLeft(8).removeFromTop(8).translated(4, 4);
        g.setColour(getSlotColor().brighter());
        g.fillEllipse(ledBounds);
    }
    
    // Slot number
    g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    g.setColour(juce::Colour(0xFF808080));
    g.drawText(juce::String(m_index + 1), 
               bounds.removeFromLeft(20).reduced(2),
               juce::Justification::centred);
    
    // Scale name
    g.setFont(juce::FontOptions(11.0f));
    g.setColour(m_isActive ? juce::Colours::white : juce::Colour(0xFFCCCCCC));
    
    auto nameToDisplay = m_displayName.isEmpty() ? "EMPTY" : m_displayName;
    if (nameToDisplay.length() > 8)
        nameToDisplay = nameToDisplay.substring(0, 6) + "..";
    
    g.drawText(nameToDisplay,
               bounds.reduced(2),
               juce::Justification::centredLeft);
    
    // Modified indicator (small dot)
    if (m_isModified)
    {
        g.setColour(juce::Colour(0xFFFFAA00));
        g.fillEllipse(bounds.removeFromRight(8).removeFromBottom(8)
                           .reduced(2).toFloat());
    }
}

void PulseScaleSwitcher::SlotButton::mouseDown(const juce::MouseEvent& event)
{
    if (event.mods.isRightButtonDown())
    {
        if (onRightClick)
            onRightClick();
    }
    else
    {
        if (onClick)
            onClick();
    }
}

void PulseScaleSwitcher::SlotButton::mouseEnter(const juce::MouseEvent&)
{
    m_isHovered = true;
    repaint();
}

void PulseScaleSwitcher::SlotButton::mouseExit(const juce::MouseEvent&)
{
    m_isHovered = false;
    repaint();
}

void PulseScaleSwitcher::SlotButton::setScale(const Scale* scale)
{
    m_scale = scale;
    if (scale)
    {
        m_displayName = scale->getName();
    }
    else
    {
        m_displayName = "";
    }
    repaint();
}

void PulseScaleSwitcher::SlotButton::setActive(bool active)
{
    if (m_isActive != active)
    {
        m_isActive = active;
        repaint();
    }
}

void PulseScaleSwitcher::SlotButton::setPending(bool pending)
{
    if (m_isPending != pending)
    {
        m_isPending = pending;
        repaint();
    }
}

void PulseScaleSwitcher::SlotButton::setModified(bool modified)
{
    if (m_isModified != modified)
    {
        m_isModified = modified;
        repaint();
    }
}

juce::Colour PulseScaleSwitcher::SlotButton::getSlotColor() const
{
    // Pulse-style colors for each slot
    const juce::Colour slotColors[8] = {
        juce::Colour(0xFF00FFAA), // Mint
        juce::Colour(0xFF00AAFF), // Cyan
        juce::Colour(0xFFFF00AA), // Magenta
        juce::Colour(0xFFFFAA00), // Orange
        juce::Colour(0xFFAAFF00), // Lime
        juce::Colour(0xFF00FF00), // Green
        juce::Colour(0xFFFF0080), // Pink
        juce::Colour(0xFF8080FF)  // Lavender
    };
    
    return slotColors[m_index % 8];
}

//==============================================================================
// RootButton implementation
//==============================================================================
PulseScaleSwitcher::RootButton::RootButton()
{
    setWantsKeyboardFocus(false);
}

void PulseScaleSwitcher::RootButton::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Background
    g.setColour(juce::Colour(0xFF2A2A2A));
    g.fillRoundedRectangle(bounds, 3.0f);
    
    // Border
    g.setColour(juce::Colour(0xFF606060));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 3.0f, 1.0f);
    
    // Root label
    g.setFont(juce::FontOptions(10.0f));
    g.setColour(juce::Colour(0xFF808080));
    g.drawText("ROOT", bounds.removeFromTop(12), juce::Justification::centred);
    
    // Root note
    g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
    g.setColour(juce::Colours::white);
    g.drawText(getRootName(), bounds, juce::Justification::centred);
}

void PulseScaleSwitcher::RootButton::mouseDown(const juce::MouseEvent&)
{
    showRootMenu();
}

void PulseScaleSwitcher::RootButton::setRootNote(int note)
{
    m_rootNote = note % 12;
    repaint();
}

void PulseScaleSwitcher::RootButton::showRootMenu()
{
    juce::PopupMenu menu;
    const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    
    for (int i = 0; i < 12; ++i)
    {
        menu.addItem(i + 1, noteNames[i], true, i == m_rootNote);
    }
    
    menu.showMenuAsync(juce::PopupMenu::Options()
                      .withTargetComponent(this),
                      [this](int result)
    {
        if (result > 0)
        {
            m_rootNote = result - 1;
            repaint();
            
            if (onRootChanged)
                onRootChanged(m_rootNote);
        }
    });
}

juce::String PulseScaleSwitcher::RootButton::getRootName() const
{
    const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    return noteNames[m_rootNote % 12];
}

//==============================================================================
// AutoModeButton implementation
//==============================================================================
PulseScaleSwitcher::AutoModeButton::AutoModeButton()
{
    setWantsKeyboardFocus(false);
}

void PulseScaleSwitcher::AutoModeButton::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Background
    auto bgColor = m_enabled ? juce::Colour(0xFF003300) : juce::Colour(0xFF2A2A2A);
    g.setColour(bgColor);
    g.fillRoundedRectangle(bounds, 3.0f);
    
    // Border
    auto borderColor = m_enabled ? juce::Colour(0xFF00FF00) : juce::Colour(0xFF606060);
    g.setColour(borderColor);
    g.drawRoundedRectangle(bounds.reduced(0.5f), 3.0f, 1.0f);
    
    // Text
    g.setFont(juce::FontOptions(11.0f));
    g.setColour(m_enabled ? juce::Colours::white : juce::Colour(0xFFCCCCCC));
    
    juce::String text = "AUTO";
    if (m_enabled)
        text += " " + juce::String(m_intervalBars) + "b";
    
    g.drawText(text, bounds, juce::Justification::centred);
}

void PulseScaleSwitcher::AutoModeButton::mouseDown(const juce::MouseEvent& event)
{
    if (event.mods.isRightButtonDown())
    {
        showIntervalMenu();
    }
    else
    {
        setEnabled(!m_enabled);
        if (onModeChanged)
            onModeChanged(m_enabled, m_intervalBars);
    }
}

void PulseScaleSwitcher::AutoModeButton::setEnabled(bool enabled)
{
    m_enabled = enabled;
    repaint();
}

void PulseScaleSwitcher::AutoModeButton::setInterval(int bars)
{
    m_intervalBars = bars;
    repaint();
}

void PulseScaleSwitcher::AutoModeButton::showIntervalMenu()
{
    juce::PopupMenu menu;
    
    menu.addItem(1, "1 Bar", true, m_intervalBars == 1);
    menu.addItem(2, "2 Bars", true, m_intervalBars == 2);
    menu.addItem(3, "4 Bars", true, m_intervalBars == 4);
    menu.addItem(4, "8 Bars", true, m_intervalBars == 8);
    menu.addItem(5, "16 Bars", true, m_intervalBars == 16);
    
    menu.showMenuAsync(juce::PopupMenu::Options()
                      .withTargetComponent(this),
                      [this](int result)
    {
        if (result > 0)
        {
            const int intervals[] = {1, 2, 4, 8, 16};
            m_intervalBars = intervals[result - 1];
            repaint();
            
            if (onModeChanged)
                onModeChanged(m_enabled, m_intervalBars);
        }
    });
}

//==============================================================================
// PulseScaleSwitcher implementation
//==============================================================================
PulseScaleSwitcher::PulseScaleSwitcher()
{
    // Create slot buttons
    for (int i = 0; i < 8; ++i)
    {
        m_slotButtons[i] = std::make_unique<SlotButton>(i);
        m_slotButtons[i]->onClick = [this, i]() { handleSlotSelection(i); };
        m_slotButtons[i]->onRightClick = [this, i]() { handleSlotEdit(i); };
        addAndMakeVisible(m_slotButtons[i].get());
    }
    
    // Create root button
    m_rootButton = std::make_unique<RootButton>();
    m_rootButton->onRootChanged = [this](int root)
    {
        if (onRootNoteChanged)
            onRootNoteChanged(root);
    };
    addAndMakeVisible(m_rootButton.get());
    
    // Create auto mode button
    m_autoButton = std::make_unique<AutoModeButton>();
    m_autoButton->onModeChanged = [this](bool enabled, int bars)
    {
        if (onAutoModeChanged)
            onAutoModeChanged(enabled, bars);
    };
    addAndMakeVisible(m_autoButton.get());
    
    // Create status label
    m_statusLabel = std::make_unique<juce::Label>();
    m_statusLabel->setFont(juce::FontOptions(10.0f));
    m_statusLabel->setColour(juce::Label::textColourId, juce::Colour(0xFF808080));
    m_statusLabel->setJustificationType(juce::Justification::centred);
    addAndMakeVisible(m_statusLabel.get());
    
    // Start animation timer
    startTimerHz(30);
    
    // Set initial state
    m_slotButtons[0]->setActive(true);
    
    setSize(800, 40);
}

PulseScaleSwitcher::~PulseScaleSwitcher()
{
    stopTimer();
}

void PulseScaleSwitcher::paint(juce::Graphics& g)
{
    // Background
    g.fillAll(juce::Colour(0xFF0A0A0A));
    
    // Progress bar for pending change
    if (m_scaleManager && m_scaleManager->isChangePending() && m_barProgress > 0.0f)
    {
        auto bounds = getLocalBounds().removeFromBottom(2).toFloat();
        
        // Background track
        g.setColour(juce::Colour(0xFF303030));
        g.fillRect(bounds);
        
        // Progress fill
        g.setColour(juce::Colour(0xFF00FF88));
        g.fillRect(bounds.withWidth(bounds.getWidth() * m_barProgress));
    }
}

void PulseScaleSwitcher::resized()
{
    layoutComponents();
}

void PulseScaleSwitcher::timerCallback()
{
    // Update animations based on ScaleSlotManager state
    if (m_scaleManager && m_scaleManager->isChangePending())
    {
        m_pendingFlashPhase += 0.1f;
        if (m_pendingFlashPhase > juce::MathConstants<float>::twoPi)
            m_pendingFlashPhase -= juce::MathConstants<float>::twoPi;
        
        // Update pending slot display
        int pendingSlot = m_scaleManager->getPendingSlot();
        if (pendingSlot >= 0 && pendingSlot < 8)
        {
            m_slotButtons[pendingSlot]->repaint();
        }
    }
    
    // Update progress animation
    if (m_progressAnimation != m_barProgress)
    {
        m_progressAnimation += (m_barProgress - m_progressAnimation) * 0.2f;
        repaint(getLocalBounds().removeFromBottom(2));
    }
}

void PulseScaleSwitcher::setScaleSlotManager(ScaleSlotManager* manager)
{
    m_scaleManager = manager;
    updateSlotDisplays();
}

void PulseScaleSwitcher::setBarProgress(float progress)
{
    m_barProgress = juce::jlimit(0.0f, 1.0f, progress);
    
    // Check if we've crossed the bar boundary (bar start)
    if (m_scaleManager && m_barProgress < 0.1f && m_scaleManager->isChangePending())
    {
        // Execute the pending change at bar boundary
        m_scaleManager->executePendingChange();
        
        // Update display to reflect the change
        updateSlotDisplays();
    }
}

void PulseScaleSwitcher::layoutComponents()
{
    auto bounds = getLocalBounds().reduced(4);
    
    // Root button on the left
    m_rootButton->setBounds(bounds.removeFromLeft(ROOT_WIDTH));
    bounds.removeFromLeft(SPACING);
    
    // 8 scale slots in the middle
    for (int i = 0; i < 8; ++i)
    {
        m_slotButtons[i]->setBounds(bounds.removeFromLeft(SLOT_WIDTH));
        if (i < 7)
            bounds.removeFromLeft(SPACING);
    }
    
    bounds.removeFromLeft(SPACING * 2);
    
    // Auto mode button
    m_autoButton->setBounds(bounds.removeFromLeft(AUTO_WIDTH));
    
    // Status label takes remaining space
    bounds.removeFromLeft(SPACING);
    m_statusLabel->setBounds(bounds);
}

void PulseScaleSwitcher::updateSlotDisplays()
{
    if (!m_scaleManager)
        return;
    
    m_activeSlot = m_scaleManager->getActiveSlotIndex();
    m_pendingSlot = m_scaleManager->isChangePending() ? 
                    m_scaleManager->getPendingSlot() : -1;
    
    for (int i = 0; i < 8; ++i)
    {
        const auto& slot = m_scaleManager->getSlot(i);
        m_slotButtons[i]->setScale(&slot.scale);
        m_slotButtons[i]->setModified(slot.isUserScale);  // User scales are modified slots
        m_slotButtons[i]->setActive(i == m_activeSlot);
        m_slotButtons[i]->setPending(i == m_pendingSlot);
    }
    
    m_rootButton->setRootNote(m_scaleManager->getGlobalRoot());
    
    // Update status label
    if (m_pendingSlot >= 0)
    {
        m_statusLabel->setText("Scale change pending...", juce::dontSendNotification);
        m_statusLabel->setColour(juce::Label::textColourId, 
                                 juce::Colours::orange.withAlpha(0.8f));
    }
    else
    {
        m_statusLabel->setText("Ready", juce::dontSendNotification);
        m_statusLabel->setColour(juce::Label::textColourId, 
                                 juce::Colours::green.withAlpha(0.6f));
    }
    
    repaint();
}

void PulseScaleSwitcher::handleSlotSelection(int slotIndex)
{
    if (!m_scaleManager)
        return;
    
    if (slotIndex == m_activeSlot && !m_scaleManager->isChangePending())
        return; // Already active and no pending change
    
    // Use ScaleSlotManager's bar-quantized switching
    m_scaleManager->selectSlot(slotIndex);
    
    // Update display to reflect pending change
    updateSlotDisplays();
    
    // Notify callback
    if (onSlotSelected)
        onSlotSelected(slotIndex);
}

void PulseScaleSwitcher::handleSlotEdit(int slotIndex)
{
    if (onSlotEditRequested)
        onSlotEditRequested(slotIndex);
}

} // namespace UI
} // namespace HAM