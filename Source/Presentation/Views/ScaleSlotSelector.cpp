/*
  ==============================================================================

    ScaleSlotSelector.cpp
    Implementation of scale slot selection UI

  ==============================================================================
*/

#include "ScaleSlotSelector.h"

namespace HAM {
namespace UI {

//==============================================================================
// ScaleSlotButton implementation

ScaleSlotSelector::ScaleSlotButton::ScaleSlotButton(int slotIndex)
    : PulseComponent("ScaleSlot_" + juce::String(slotIndex + 1)),
      m_slotIndex(slotIndex)
{
}

void ScaleSlotSelector::ScaleSlotButton::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().reduced(2);
    
    // Background with hover/active animation
    float alpha = 0.3f + (m_hoverAnimation * 0.2f) + (m_activeAnimation * 0.3f);
    
    if (m_slotInfo.isEmpty)
    {
        // Empty slot - dotted border
        g.setColour(juce::Colour(0xFF3A3A3A).withAlpha(alpha));
        g.drawRect(bounds, 2.0f);
        
        // Draw dotted pattern manually
        g.setColour(juce::Colour(0xFF5A5A5A));
        float dashLength = 4.0f;
        float gapLength = 4.0f;
        
        // Top border dashes
        for (float x = bounds.getX(); x < bounds.getRight(); x += dashLength + gapLength)
        {
            g.drawLine(x, bounds.getY(), 
                      juce::jmin(x + dashLength, (float)bounds.getRight()), 
                      bounds.getY(), 1.0f);
        }
        
        // Slot number in center
        g.setFont(14.0f);
        g.setColour(juce::Colour(0xFF5A5A5A));
        g.drawText(juce::String(m_slotIndex + 1), bounds, juce::Justification::centred);
    }
    else
    {
        // Filled slot
        auto slotColor = getParentComponent() ? 
            static_cast<ScaleSlotSelector*>(getParentComponent())->getSlotColor(m_slotIndex) :
            juce::Colour(0xFF00FF88);
        
        // Background gradient
        g.setGradientFill(juce::ColourGradient(
            slotColor.withAlpha(alpha),
            bounds.getCentreX(), bounds.getY(),
            slotColor.withAlpha(alpha * 0.3f),
            bounds.getCentreX(), bounds.getBottom(),
            false
        ));
        g.fillRoundedRectangle(bounds.toFloat(), 3.0f);
        
        // Border
        float borderWidth = m_isActive ? 2.0f : 1.0f;
        g.setColour(slotColor.withAlpha(0.8f + (m_activeAnimation * 0.2f)));
        g.drawRoundedRectangle(bounds.toFloat(), 3.0f, borderWidth);
        
        // Pending indicator (pulsing dot)
        if (m_isPending)
        {
            float pulsePhase = std::sin(juce::Time::getMillisecondCounter() * 0.005f) * 0.5f + 0.5f;
            g.setColour(slotColor.withAlpha(0.5f + pulsePhase * 0.5f));
            g.fillEllipse(bounds.getRight() - 10, bounds.getY() + 5, 6, 6);
        }
        
        // Scale name (smaller font for narrow slots)
        g.setFont(10.0f);
        g.setColour(juce::Colours::white.withAlpha(0.9f));
        g.drawText(m_slotInfo.displayName, 
                  bounds.reduced(3, 2).removeFromTop(16), 
                  juce::Justification::centred);
        
        // Root note (tiny font)
        g.setFont(8.0f);
        g.setColour(juce::Colours::white.withAlpha(0.6f));
        auto rootText = static_cast<ScaleSlotSelector*>(getParentComponent())->
            getRootNoteString(m_slotInfo.rootNote);
        g.drawText(rootText,
                  bounds.reduced(3, 2).removeFromBottom(12),
                  juce::Justification::centred);
    }
    
    // Browse button (small "..." icon in corner)
    auto browseButtonBounds = juce::Rectangle<float>(
        bounds.getRight() - 18, bounds.getBottom() - 14, 16, 12);
    
    // Only show on hover or if slot is empty
    if (m_isHovered || m_slotInfo.isEmpty)
    {
        g.setColour(juce::Colour(0xFF7A7A7A).withAlpha(m_isHovered ? 0.8f : 0.4f));
        g.fillRoundedRectangle(browseButtonBounds, 2.0f);
        g.setColour(juce::Colours::white.withAlpha(0.7f));
        g.setFont(10.0f);
        g.drawText("...", browseButtonBounds, juce::Justification::centred);
        m_browseButtonArea = browseButtonBounds.toNearestInt();
    }
    
    // Active slot highlight
    if (m_isActive)
    {
        // Outer glow
        g.setColour(juce::Colour(0xFF00FF88).withAlpha(0.3f * m_activeAnimation));
        g.drawRoundedRectangle(bounds.expanded(2).toFloat(), 5.0f, 2.0f);
    }
}

void ScaleSlotSelector::ScaleSlotButton::mouseDown(const juce::MouseEvent& event)
{
    // Check if click is on browse button
    if (m_browseButtonArea.contains(event.getPosition()))
    {
        if (onBrowseClick)
            onBrowseClick();
    }
    else
    {
        // Normal slot click
        if (onClick)
            onClick();
    }
}

void ScaleSlotSelector::ScaleSlotButton::mouseEnter(const juce::MouseEvent&)
{
    m_isHovered = true;
    m_hoverAnimation = 1.0f;
    repaint();
}

void ScaleSlotSelector::ScaleSlotButton::mouseExit(const juce::MouseEvent&)
{
    m_isHovered = false;
    m_hoverAnimation = 0.0f;
    repaint();
}

void ScaleSlotSelector::ScaleSlotButton::setSlotInfo(const ScaleSlotViewModel::SlotInfo& info)
{
    m_slotInfo = info;
    repaint();
}

void ScaleSlotSelector::ScaleSlotButton::setActive(bool active)
{
    if (m_isActive != active)
    {
        m_isActive = active;
        m_activeAnimation = active ? 1.0f : 0.0f;
        repaint();
    }
}

void ScaleSlotSelector::ScaleSlotButton::setPending(bool pending)
{
    if (m_isPending != pending)
    {
        m_isPending = pending;
        repaint();
    }
}

void ScaleSlotSelector::ScaleSlotButton::setHovered(bool hovered)
{
    if (m_isHovered != hovered)
    {
        m_isHovered = hovered;
        m_hoverAnimation = hovered ? 1.0f : 0.0f;
        repaint();
    }
}

//==============================================================================
// ScaleSlotSelector implementation

ScaleSlotSelector::ScaleSlotSelector()
    : PulseComponent("ScaleSlotSelector")
{
    // Enable keyboard focus for shortcuts
    setWantsKeyboardFocus(true);
    
    // Create 8 scale slot buttons
    for (int i = 0; i < 8; ++i)
    {
        m_slotButtons[i] = std::make_unique<ScaleSlotButton>(i);
        m_slotButtons[i]->onClick = [this, i]() { handleSlotClick(i); };
        m_slotButtons[i]->onBrowseClick = [this, i]() { 
            // Open scale browser for this slot
            if (onScaleBrowserRequested)
                onScaleBrowserRequested(i);
        };
        addAndMakeVisible(m_slotButtons[i].get());
    }
    
    // Create navigation arrows with custom arrow graphics
    m_leftArrowButton = std::make_unique<ArrowButton>("Left Arrow", ArrowButton::Direction::Left);
    m_leftArrowButton->onClick = [this]() { handleLeftArrowClick(); };
    addAndMakeVisible(m_leftArrowButton.get());
    
    m_rightArrowButton = std::make_unique<ArrowButton>("Right Arrow", ArrowButton::Direction::Right);
    m_rightArrowButton->onClick = [this]() { handleRightArrowClick(); };
    addAndMakeVisible(m_rightArrowButton.get());
    
    // Create auto-mode button
    m_autoModeButton = std::make_unique<PulseButton>("AUTO", PulseButton::Outline);
    m_autoModeButton->onClick = [this]() {
        if (m_viewModel)
        {
            // Toggle auto mode state
            if (m_viewModel->isAutoProgressionActive())
            {
                m_viewModel->stopAutoProgression();
                m_autoModeButton->setButtonText("AUTO");
            }
            else
            {
                m_viewModel->startAutoProgression();
                m_autoModeButton->setButtonText("AUTO ✓");
            }
        }
    };
    addAndMakeVisible(m_autoModeButton.get());
    
    // Create auto-mode menu button
    m_autoModeMenuButton = std::make_unique<PulseButton>("1 BAR", PulseButton::Outline);
    m_autoModeMenuButton->onClick = [this]() { handleAutoModeMenu(); };
    addAndMakeVisible(m_autoModeMenuButton.get());
    
    // Create root note button with musical symbol
    m_rootNoteButton = std::make_unique<PulseButton>("♪ C", PulseButton::Solid);
    m_rootNoteButton->onClick = [this]() { handleRootNoteMenu(); };
    addAndMakeVisible(m_rootNoteButton.get());
    
    // Start animation timer for smooth updates
    startTimerHz(30);
}

ScaleSlotSelector::~ScaleSlotSelector()
{
    stopTimer();
    
    if (m_viewModel)
        m_viewModel->removeChangeListener(this);
}

void ScaleSlotSelector::paint(juce::Graphics& g)
{
    // REMOVED: Background panel and border frame - no longer needed
    // The topbar itself will have a unified frame
    
    // Auto-progression progress bar (if active)
    if (m_viewModel && m_viewModel->isAutoProgressionActive())
    {
        float progress = m_viewModel->getAutoProgressionProgress();
        
        // Draw a thin progress line at the bottom
        auto progressBounds = getLocalBounds().removeFromBottom(2).toFloat();
        g.setColour(juce::Colour(0xFF00FF88).withAlpha(0.3f));
        g.fillRect(progressBounds);
        
        g.setColour(juce::Colour(0xFF00FF88));
        g.fillRect(progressBounds.withWidth(progressBounds.getWidth() * progress));
    }
}

void ScaleSlotSelector::resized()
{
    auto bounds = getLocalBounds();
    
    // UNIFIED LAYOUT: Use exact same dimensions as TransportBar
    const int UNIFIED_BUTTON_HEIGHT = 36;  // Same height everywhere
    const int SCALE_SPACING = 4;           // Tighter spacing for scale buttons
    const int UNIFIED_SPACING = 8;         // Normal spacing for other elements
    
    int buttonY = (bounds.getHeight() - UNIFIED_BUTTON_HEIGHT) / 2;  // Center vertically
    int currentX = SCALE_SPACING;
    
    // Left arrow (unified height)
    m_leftArrowButton->setBounds(currentX, buttonY, 28, UNIFIED_BUTTON_HEIGHT);
    currentX += 28 + SCALE_SPACING;
    
    // 8 scale slots (optimized for space with 120px total shift)
    int slotWidth = 43;  // Reduced to fit with two slot widths shift
    for (int i = 0; i < 8; ++i)
    {
        m_slotButtons[i]->setBounds(currentX, buttonY, slotWidth, UNIFIED_BUTTON_HEIGHT);
        currentX += slotWidth + SCALE_SPACING;  // Tight spacing between slots
    }
    
    // Right arrow (unified height)
    m_rightArrowButton->setBounds(currentX, buttonY, 28, UNIFIED_BUTTON_HEIGHT);
    currentX += 28 + UNIFIED_SPACING;
    
    // Root note button (smaller)
    m_rootNoteButton->setBounds(currentX, buttonY, 40, UNIFIED_BUTTON_HEIGHT);
    currentX += 40 + UNIFIED_SPACING;
    
    // Auto mode button (smaller)
    m_autoModeButton->setBounds(currentX, buttonY, 45, UNIFIED_BUTTON_HEIGHT);
    currentX += 45;
    
    // Auto mode menu (smaller)
    m_autoModeMenuButton->setBounds(currentX, buttonY, 50, UNIFIED_BUTTON_HEIGHT);
}

void ScaleSlotSelector::layoutSlots()
{
    // This method is no longer needed since we handle slot layout in resized()
    // Keep it for compatibility but it does nothing now
}

void ScaleSlotSelector::setViewModel(ScaleSlotViewModel* viewModel)
{
    if (m_viewModel)
        m_viewModel->removeChangeListener(this);
    
    m_viewModel = viewModel;
    
    if (m_viewModel)
    {
        m_viewModel->addChangeListener(this);
        updateSlotStates();
    }
}

void ScaleSlotSelector::changeListenerCallback(juce::ChangeBroadcaster*)
{
    // Update UI from view model changes
    updateSlotStates();
}

void ScaleSlotSelector::updateSlotStates()
{
    if (!m_viewModel)
        return;
    
    auto allSlotInfo = m_viewModel->getAllSlotInfo();
    int activeIndex = m_viewModel->getActiveSlotIndex();
    int pendingIndex = m_viewModel->getPendingSlotIndex();
    
    for (int i = 0; i < 8; ++i)
    {
        m_slotButtons[i]->setSlotInfo(allSlotInfo[i]);
        m_slotButtons[i]->setActive(i == activeIndex);
        m_slotButtons[i]->setPending(i == pendingIndex);
    }
    
    // Update root note display
    int globalRoot = m_viewModel->getGlobalRoot();
    m_rootNoteButton->setButtonText("♪ " + getRootNoteString(globalRoot));
    
    // Update auto mode display
    // Update auto button text based on state
    if (m_viewModel->isAutoProgressionActive())
        m_autoModeButton->setButtonText("AUTO ✓");
    else
        m_autoModeButton->setButtonText("AUTO");
    m_autoModeMenuButton->setButtonText(getAutoModeString(m_viewModel->getAutoMode()));
    
    repaint();
}

void ScaleSlotSelector::timerCallback()
{
    // Smooth animations
    for (auto& button : m_slotButtons)
    {
        button->repaint();
    }
    
    // Update progress animation
    if (m_viewModel && m_viewModel->isAutoProgressionActive())
    {
        m_autoProgressAnimation = m_viewModel->getAutoProgressionProgress();
        repaint(m_slotsArea);
    }
}

void ScaleSlotSelector::handleSlotClick(int slotIndex)
{
    if (!m_viewModel)
        return;
    
    auto slotInfo = m_viewModel->getSlotInfo(slotIndex);
    
    if (slotInfo.isEmpty)
    {
        // Empty slot - open browser to load scale
        if (onScaleBrowserRequested)
            onScaleBrowserRequested(slotIndex);
    }
    else if (slotInfo.isActive)
    {
        // Active slot - open menu to edit/clear
        juce::PopupMenu menu;
        menu.addItem(1, "Edit Scale", true);
        menu.addItem(2, "Clear Slot", true);
        menu.addSeparator();
        menu.addItem(3, "Copy Scale", true);
        menu.addItem(4, "Paste Scale", false); // Disabled for now
        
        menu.showMenuAsync(juce::PopupMenu::Options(),
            [this, slotIndex](int result)
            {
                switch (result)
                {
                    case 1: // Edit
                        if (onScaleBrowserRequested)
                            onScaleBrowserRequested(slotIndex);
                        break;
                    case 2: // Clear
                        m_viewModel->clearSlot(slotIndex);
                        break;
                    case 3: // Copy
                        if (m_viewModel && !m_viewModel->getSlotInfo(slotIndex).isEmpty)
                        {
                            // Copy scale to clipboard
                            auto slotInfo = m_viewModel->getSlotInfo(slotIndex);
                            auto clipboardData = juce::JSON::parse(juce::String("{"
                                "\"scaleName\":\"") + slotInfo.displayName + 
                                "\",\"scaleType\":\"" + slotInfo.scaleType + 
                                "\",\"rootNote\":" + juce::String(slotInfo.rootNote) + "}");
                            
                            if (clipboardData.isObject())
                            {
                                juce::SystemClipboard::copyTextToClipboard(juce::JSON::toString(clipboardData));
                                juce::Logger::writeToLog("Scale copied to clipboard: " + slotInfo.displayName);
                            }
                        }
                        break;
                }
            });
    }
    else
    {
        // Inactive slot - select it
        m_viewModel->selectSlot(slotIndex);
    }
}

void ScaleSlotSelector::handleLeftArrowClick()
{
    if (m_viewModel)
        m_viewModel->selectPreviousSlot();
}

void ScaleSlotSelector::handleRightArrowClick()
{
    if (m_viewModel)
        m_viewModel->selectNextSlot();
}

void ScaleSlotSelector::handleAutoModeToggle()
{
    if (!m_viewModel)
        return;
    
    bool newState = !m_viewModel->isAutoProgressionActive();
    
    if (newState)
        m_viewModel->startAutoProgression();
    else
        m_viewModel->stopAutoProgression();
}

void ScaleSlotSelector::handleAutoModeMenu()
{
    if (!m_viewModel)
        return;
    
    juce::PopupMenu menu;
    
    auto currentMode = m_viewModel->getAutoMode();
    
    menu.addItem(1, "Off", true, currentMode == ScaleSlotViewModel::AutoMode::OFF);
    menu.addSeparator();
    menu.addItem(2, "1/4 Bar", true, currentMode == ScaleSlotViewModel::AutoMode::QUARTER_BAR);
    menu.addItem(3, "1 Bar", true, currentMode == ScaleSlotViewModel::AutoMode::ONE_BAR);
    menu.addItem(4, "2 Bars", true, currentMode == ScaleSlotViewModel::AutoMode::TWO_BARS);
    menu.addItem(5, "4 Bars", true, currentMode == ScaleSlotViewModel::AutoMode::FOUR_BARS);
    menu.addItem(6, "8 Bars", true, currentMode == ScaleSlotViewModel::AutoMode::EIGHT_BARS);
    menu.addItem(7, "16 Bars", true, currentMode == ScaleSlotViewModel::AutoMode::SIXTEEN_BARS);
    
    menu.showMenuAsync(juce::PopupMenu::Options(),
        [this](int result)
        {
            if (!m_viewModel || result == 0)
                return;
            
            ScaleSlotViewModel::AutoMode newMode;
            switch (result)
            {
                case 1: newMode = ScaleSlotViewModel::AutoMode::OFF; break;
                case 2: newMode = ScaleSlotViewModel::AutoMode::QUARTER_BAR; break;
                case 3: newMode = ScaleSlotViewModel::AutoMode::ONE_BAR; break;
                case 4: newMode = ScaleSlotViewModel::AutoMode::TWO_BARS; break;
                case 5: newMode = ScaleSlotViewModel::AutoMode::FOUR_BARS; break;
                case 6: newMode = ScaleSlotViewModel::AutoMode::EIGHT_BARS; break;
                case 7: newMode = ScaleSlotViewModel::AutoMode::SIXTEEN_BARS; break;
                default: return;
            }
            
            m_viewModel->setAutoMode(newMode);
        });
}

void ScaleSlotSelector::handleRootNoteMenu()
{
    if (!m_viewModel)
        return;
    
    juce::PopupMenu menu;
    int currentRoot = m_viewModel->getGlobalRoot();
    
    for (int i = 0; i < 12; ++i)
    {
        menu.addItem(i + 1, getRootNoteString(i), true, i == currentRoot);
    }
    
    menu.showMenuAsync(juce::PopupMenu::Options(),
        [this](int result)
        {
            if (m_viewModel && result > 0)
            {
                m_viewModel->setGlobalRoot(result - 1);
                if (onRootNoteChanged)
                    onRootNoteChanged(result - 1);
            }
        });
}

void ScaleSlotSelector::mouseDown(const juce::MouseEvent& event)
{
    // Handle clicks on main component (not buttons)
}

void ScaleSlotSelector::mouseMove(const juce::MouseEvent& event)
{
    int slotIndex = getSlotUnderMouse(event.getPosition());
    
    if (slotIndex != m_hoveredSlotIndex)
    {
        if (m_hoveredSlotIndex >= 0)
            m_slotButtons[m_hoveredSlotIndex]->setHovered(false);
        
        if (slotIndex >= 0)
            m_slotButtons[slotIndex]->setHovered(true);
        
        m_hoveredSlotIndex = slotIndex;
    }
}

void ScaleSlotSelector::mouseExit(const juce::MouseEvent&)
{
    if (m_hoveredSlotIndex >= 0)
    {
        m_slotButtons[m_hoveredSlotIndex]->setHovered(false);
        m_hoveredSlotIndex = -1;
    }
}

bool ScaleSlotSelector::keyPressed(const juce::KeyPress& key)
{
    // Number keys 1-8 select scale slots
    if (key.getKeyCode() >= '1' && key.getKeyCode() <= '8')
    {
        int slotIndex = key.getKeyCode() - '1';  // Convert to 0-based index
        if (slotIndex < 8)
        {
            handleSlotClick(slotIndex);
            return true;
        }
    }
    
    // Left/Right arrow keys for navigation
    if (key.getKeyCode() == juce::KeyPress::leftKey)
    {
        handleLeftArrowClick();
        return true;
    }
    else if (key.getKeyCode() == juce::KeyPress::rightKey)
    {
        handleRightArrowClick();
        return true;
    }
    
    // 'A' key toggles auto mode
    if (key.getKeyCode() == 'A' || key.getKeyCode() == 'a')
    {
        handleAutoModeMenu();  // Fixed: Use existing function
        return true;
    }
    
    // 'R' key opens root note selector
    if (key.getKeyCode() == 'R' || key.getKeyCode() == 'r')
    {
        handleRootNoteMenu();  // Fixed: Use existing function
        return true;
    }
    
    return false;  // Key not handled
}

int ScaleSlotSelector::getSlotUnderMouse(const juce::Point<int>& position)
{
    for (int i = 0; i < 8; ++i)
    {
        if (m_slotButtons[i]->getBounds().contains(position))
            return i;
    }
    return -1;
}

juce::Colour ScaleSlotSelector::getSlotColor(int slotIndex) const
{
    if (slotIndex >= 0 && slotIndex < 8)
        return m_slotColors[slotIndex];
    return juce::Colour(0xFF00FF88);
}

juce::String ScaleSlotSelector::getRootNoteString(int rootNote) const
{
    if (rootNote >= 0 && rootNote < 12)
        return m_noteNames[rootNote];
    return "C";
}

juce::String ScaleSlotSelector::getAutoModeString(ScaleSlotViewModel::AutoMode mode) const
{
    switch (mode)
    {
        case ScaleSlotViewModel::AutoMode::OFF: return "OFF";
        case ScaleSlotViewModel::AutoMode::QUARTER_BAR: return "1/4 BAR";
        case ScaleSlotViewModel::AutoMode::ONE_BAR: return "1 BAR";
        case ScaleSlotViewModel::AutoMode::TWO_BARS: return "2 BARS";
        case ScaleSlotViewModel::AutoMode::FOUR_BARS: return "4 BARS";
        case ScaleSlotViewModel::AutoMode::EIGHT_BARS: return "8 BARS";
        case ScaleSlotViewModel::AutoMode::SIXTEEN_BARS: return "16 BARS";
        default: return "1 BAR";
    }
}

} // namespace UI
} // namespace HAM