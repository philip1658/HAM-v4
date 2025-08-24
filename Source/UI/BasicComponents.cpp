#include "BasicComponents.h"

namespace HAM {

//==============================================================================
// Constants for UI consistency
static constexpr float TRACK_WIDTH = 22.0f;  // EXACT Pulse spec
static constexpr float CORNER_RADIUS = 3.0f;
static constexpr float LINE_THICKNESS = 2.0f;
static constexpr float ANIMATION_SPEED = 0.15f;

//==============================================================================
// PulseVerticalSlider Implementation
//==============================================================================
PulseVerticalSlider::PulseVerticalSlider(const juce::String& name, int trackColorIndex)
    : PulseComponent(name), trackColorIdx(trackColorIndex)
{
}

void PulseVerticalSlider::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Calculate track bounds (22px wide - EXACT Pulse dimension)
    float scaledTrackWidth = TRACK_WIDTH * scaleFactor;
    auto trackBounds = bounds.withSizeKeepingCentre(scaledTrackWidth, bounds.getHeight() - 10);
    
    // Track shadow (inset effect)
    g.setColour(PulseColors::BG_VOID.withAlpha(0.5f));
    g.fillRoundedRectangle(trackBounds.translated(0, 1), CORNER_RADIUS * scaleFactor);
    
    // Track background gradient
    juce::ColourGradient trackGradient(
        PulseColors::BG_DARK.darker(0.3f), trackBounds.getCentreX(), trackBounds.getY(),
        PulseColors::BG_MID.darker(0.2f), trackBounds.getCentreX(), trackBounds.getBottom(), false);
    g.setGradientFill(trackGradient);
    g.fillRoundedRectangle(trackBounds, CORNER_RADIUS * scaleFactor);
    
    // Inner highlight for depth (Pulse-style)
    g.setColour(PulseColors::BG_LIGHT.withAlpha(0.3f));
    g.drawRoundedRectangle(trackBounds.reduced(0.5f), CORNER_RADIUS * scaleFactor - 0.5f, 0.5f);
    
    // Calculate value position
    float valueY = trackBounds.getY() + (1.0f - value) * trackBounds.getHeight();
    
    // Value fill (from bottom to current value)
    auto fillBounds = trackBounds.withTop(valueY);
    
    // Get track color
    juce::Colour trackColor = getTrackColor();
    
    // Glow effect when active
    if (glowIntensity > 0.01f)
    {
        g.setColour(trackColor.withAlpha(glowIntensity * 0.4f));
        g.fillRoundedRectangle(fillBounds.expanded(3.0f), CORNER_RADIUS * scaleFactor + 3.0f);
    }
    
    // Main fill with gradient
    juce::ColourGradient fillGradient(
        trackColor.withAlpha(0.9f), trackBounds.getCentreX(), valueY,
        trackColor.withAlpha(0.7f), trackBounds.getCentreX(), trackBounds.getBottom(), false);
    g.setGradientFill(fillGradient);
    g.fillRoundedRectangle(fillBounds, CORNER_RADIUS * scaleFactor);
    
    // LINE INDICATOR (NOT a thumb - this is key Pulse design)
    g.setColour(PulseColors::TEXT_PRIMARY);
    g.fillRect(trackBounds.getX() - 5, valueY - LINE_THICKNESS/2, 
               trackBounds.getWidth() + 10, LINE_THICKNESS);
    
    // Add subtle glow to line
    g.setColour(trackColor.withAlpha(0.6f));
    g.drawLine(trackBounds.getX() - 5, valueY, 
               trackBounds.getRight() + 5, valueY, LINE_THICKNESS * 2);
    
    // Label
    if (!label.isEmpty())
    {
        g.setFont(juce::Font(juce::FontOptions(10.0f * scaleFactor).withName("Helvetica Neue")));
        g.setColour(PulseColors::TEXT_SECONDARY);
        g.drawText(label, bounds.removeFromBottom(15), juce::Justification::centred);
    }
    
    // Value label
    if (!valueLabel.isEmpty())
    {
        g.setFont(juce::Font(juce::FontOptions(9.0f * scaleFactor).withName("Helvetica Neue")));
        g.setColour(PulseColors::TEXT_DIMMED);
        g.drawText(valueLabel, bounds.removeFromTop(15), juce::Justification::centred);
    }
}

void PulseVerticalSlider::resized()
{
    // Nothing specific needed here yet
}

void PulseVerticalSlider::mouseDown(const juce::MouseEvent& event)
{
    updateValueFromMouse(event);
    glowIntensity = 1.0f;
    isPressed = true;
    repaint();
}

void PulseVerticalSlider::mouseDrag(const juce::MouseEvent& event)
{
    updateValueFromMouse(event);
    repaint();
}

void PulseVerticalSlider::mouseUp(const juce::MouseEvent&)
{
    isPressed = false;
    glowIntensity *= 0.9f;
    repaint();
}

void PulseVerticalSlider::mouseEnter(const juce::MouseEvent&)
{
    isHovering = true;
    hoverAmount = 1.0f;
    repaint();
}

void PulseVerticalSlider::mouseExit(const juce::MouseEvent&)
{
    isHovering = false;
    hoverAmount = 0.0f;
    glowIntensity *= 0.9f;
    repaint();
}

void PulseVerticalSlider::updateValueFromMouse(const juce::MouseEvent& event)
{
    float newValue = 1.0f - (event.position.y / getHeight());
    setValue(newValue);
}

juce::Colour PulseVerticalSlider::getTrackColor() const
{
    return PulseColors::getTrackColor(trackColorIdx);
}

//==============================================================================
// PulseHorizontalSlider Implementation
//==============================================================================
PulseHorizontalSlider::PulseHorizontalSlider(const juce::String& name, bool showThumb)
    : PulseComponent(name), hasThumb(showThumb)
{
}

void PulseHorizontalSlider::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    float trackHeight = 8.0f * scaleFactor;
    
    auto trackBounds = bounds.withSizeKeepingCentre(bounds.getWidth() - 20, trackHeight);
    
    // Track shadow
    drawMultiLayerShadow(g, trackBounds, 2, 2.0f);
    
    // Track background
    g.setColour(PulseColors::BG_DARK);
    g.fillRoundedRectangle(trackBounds, CORNER_RADIUS * scaleFactor);
    
    // Track border
    g.setColour(PulseColors::BG_LIGHT.withAlpha(0.3f));
    g.drawRoundedRectangle(trackBounds, CORNER_RADIUS * scaleFactor, 0.5f);
    
    // Value fill
    auto fillBounds = trackBounds.withWidth(trackBounds.getWidth() * value);
    fillWithGradient(g, fillBounds, 
                    PulseColors::TRACK_CYAN.withAlpha(0.9f),
                    PulseColors::TRACK_CYAN.withAlpha(0.7f));
    
    // Thumb (if enabled)
    if (hasThumb)
    {
        float thumbX = trackBounds.getX() + (trackBounds.getWidth() * value);
        float thumbRadius = 8.0f * scaleFactor * (1.0f + hoverAmount * 0.1f);
        
        // Thumb shadow
        drawMultiLayerShadow(g, juce::Rectangle<float>(thumbX - thumbRadius, 
                                                      trackBounds.getCentreY() - thumbRadius,
                                                      thumbRadius * 2, thumbRadius * 2), 3, 3.0f);
        
        // Thumb
        g.setColour(PulseColors::BG_RAISED);
        g.fillEllipse(thumbX - thumbRadius, trackBounds.getCentreY() - thumbRadius,
                     thumbRadius * 2, thumbRadius * 2);
        
        // Thumb highlight
        g.setColour(PulseColors::TEXT_PRIMARY.withAlpha(0.2f + hoverAmount * 0.3f));
        g.drawEllipse(thumbX - thumbRadius, trackBounds.getCentreY() - thumbRadius,
                     thumbRadius * 2, thumbRadius * 2, 1.0f);
    }
    
    // Label
    g.setFont(juce::Font(juce::FontOptions(10.0f * scaleFactor).withName("Helvetica Neue")));
    g.setColour(PulseColors::TEXT_SECONDARY);
    g.drawText(componentName, bounds.removeFromBottom(20), juce::Justification::centred);
}

void PulseHorizontalSlider::resized()
{
    // Nothing specific needed here yet
}

void PulseHorizontalSlider::mouseDown(const juce::MouseEvent& event)
{
    updateValueFromMouse(event);
    isPressed = true;
    repaint();
}

void PulseHorizontalSlider::mouseDrag(const juce::MouseEvent& event)
{
    updateValueFromMouse(event);
    repaint();
}

void PulseHorizontalSlider::mouseUp(const juce::MouseEvent&)
{
    isPressed = false;
    repaint();
}

void PulseHorizontalSlider::updateValueFromMouse(const juce::MouseEvent& event)
{
    float newValue = event.position.x / getWidth();
    setValue(newValue);
}

//==============================================================================
// PulseButton Implementation
//==============================================================================
PulseButton::PulseButton(const juce::String& name, Style style)
    : PulseComponent(name), buttonStyle(style)
{
}

void PulseButton::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(2);
    
    // Draw shadow for raised styles
    if (buttonStyle == Solid || buttonStyle == Gradient)
    {
        drawMultiLayerShadow(g, bounds, 3, 2.0f);
    }
    
    // Button background based on style
    switch (buttonStyle)
    {
        case Solid:
            g.setColour(isPressed ? PulseColors::BG_MID : PulseColors::BG_RAISED);
            g.fillRoundedRectangle(bounds, CORNER_RADIUS * scaleFactor);
            break;
            
        case Outline:
            g.setColour(PulseColors::BG_DARK.withAlpha(0.3f));
            g.fillRoundedRectangle(bounds, CORNER_RADIUS * scaleFactor);
            g.setColour(PulseColors::TRACK_CYAN.withAlpha(0.8f + hoverAmount * 0.2f));
            g.drawRoundedRectangle(bounds, CORNER_RADIUS * scaleFactor, 1.5f);
            break;
            
        case Ghost:
            if (isHovering || isPressed)
            {
                g.setColour(PulseColors::BG_LIGHT.withAlpha(0.2f));
                g.fillRoundedRectangle(bounds, CORNER_RADIUS * scaleFactor);
            }
            break;
            
        case Gradient:
            fillWithGradient(g, bounds,
                           isPressed ? PulseColors::BG_MID : PulseColors::BG_RAISED,
                           isPressed ? PulseColors::BG_DARK : PulseColors::BG_MID);
            break;
    }
    
    // Hover glow
    if (isHovering && !isPressed)
    {
        UIUtils::drawGlow(g, bounds, PulseColors::TRACK_CYAN, hoverAmount * 0.5f);
    }
    
    // Button text
    g.setFont(juce::Font(juce::FontOptions(12.0f * scaleFactor).withName("Helvetica Neue")));
    g.setColour(isPressed ? PulseColors::TEXT_DIMMED : PulseColors::TEXT_PRIMARY);
    g.drawText(buttonText.isEmpty() ? componentName : buttonText, 
              bounds, juce::Justification::centred);
}

void PulseButton::resized()
{
    // Nothing specific needed here yet
}

void PulseButton::mouseDown(const juce::MouseEvent&)
{
    isPressed = true;
    clickAnimation = 1.0f;
    repaint();
}

void PulseButton::mouseUp(const juce::MouseEvent&)
{
    if (isPressed && onClick)
        onClick();
    
    isPressed = false;
    repaint();
}

void PulseButton::mouseEnter(const juce::MouseEvent&)
{
    isHovering = true;
    hoverAmount = 0.0f; // Will animate to 1.0
    repaint();
}

void PulseButton::mouseExit(const juce::MouseEvent&)
{
    isHovering = false;
    isPressed = false;
    repaint();
}

//==============================================================================
// PulseToggle Implementation
//==============================================================================

// Inner animator class for smooth toggle animation
class PulseToggle::ToggleAnimator : public juce::Timer
{
public:
    ToggleAnimator(PulseToggle& owner) : toggle(owner) {}
    
    void timerCallback() override
    {
        toggle.timerCallback();
    }
    
private:
    PulseToggle& toggle;
};

PulseToggle::PulseToggle(const juce::String& name)
    : PulseComponent(name)
{
    animator = std::make_unique<ToggleAnimator>(*this);
}

PulseToggle::~PulseToggle() = default;

void PulseToggle::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    float toggleWidth = 44.0f * scaleFactor;
    float toggleHeight = 24.0f * scaleFactor;
    
    auto toggleBounds = bounds.withSizeKeepingCentre(toggleWidth, toggleHeight);
    
    // Track shadow
    drawMultiLayerShadow(g, toggleBounds, 2, 1.5f);
    
    // Track background
    juce::Colour trackColor = isOn ? 
        PulseColors::TRACK_MINT.withAlpha(0.3f) : 
        PulseColors::BG_DARK;
    
    g.setColour(trackColor);
    g.fillRoundedRectangle(toggleBounds, toggleHeight * 0.5f);
    
    // Track border
    g.setColour(isOn ? PulseColors::TRACK_MINT.withAlpha(0.5f) : PulseColors::BG_LIGHT);
    g.drawRoundedRectangle(toggleBounds, toggleHeight * 0.5f, 1.0f);
    
    // Calculate thumb position
    float thumbRadius = (toggleHeight * 0.4f);
    float thumbX = toggleBounds.getX() + thumbRadius + 2 + 
                  (toggleAnimation * (toggleWidth - thumbRadius * 2 - 4));
    float thumbY = toggleBounds.getCentreY();
    
    // Thumb shadow
    g.setColour(juce::Colours::black.withAlpha(0.3f));
    g.fillEllipse(thumbX - thumbRadius, thumbY - thumbRadius + 1,
                 thumbRadius * 2, thumbRadius * 2);
    
    // Thumb
    g.setColour(isOn ? PulseColors::TEXT_PRIMARY : PulseColors::BG_RAISED);
    g.fillEllipse(thumbX - thumbRadius, thumbY - thumbRadius,
                 thumbRadius * 2, thumbRadius * 2);
    
    // Label
    g.setFont(juce::Font(juce::FontOptions(10.0f * scaleFactor).withName("Helvetica Neue")));
    g.setColour(PulseColors::TEXT_SECONDARY);
    g.drawText(componentName, bounds.removeFromBottom(20), juce::Justification::centred);
}

void PulseToggle::resized()
{
    // Nothing specific needed here yet
}

void PulseToggle::mouseDown(const juce::MouseEvent&)
{
    isOn = !isOn;
    animateToggle();
    
    if (onStateChanged)
        onStateChanged(isOn);
    
    repaint();
}

void PulseToggle::mouseEnter(const juce::MouseEvent&)
{
    isHovering = true;
    repaint();
}

void PulseToggle::mouseExit(const juce::MouseEvent&)
{
    isHovering = false;
    repaint();
}

void PulseToggle::animateToggle()
{
    animator->startTimerHz(60);
}

void PulseToggle::timerCallback()
{
    float target = isOn ? 1.0f : 0.0f;
    toggleAnimation = smoothValue(toggleAnimation, target, 0.2f);
    
    if (std::abs(toggleAnimation - target) < 0.01f)
    {
        toggleAnimation = target;
        animator->stopTimer();
    }
    
    repaint();
}

//==============================================================================
// PulseDropdown Implementation
//==============================================================================
PulseDropdown::PulseDropdown(const juce::String& name)
    : PulseComponent(name)
{
    items.add("Option 1");
    items.add("Option 2");
    items.add("Option 3");
}

void PulseDropdown::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(2);
    
    // Draw multi-layer shadow
    drawMultiLayerShadow(g, bounds, 3, 2.0f);
    
    // Background with gradient
    fillWithGradient(g, bounds,
                    PulseColors::BG_RAISED,
                    PulseColors::BG_MID);
    
    // Border
    g.setColour(PulseColors::BG_LIGHT.withAlpha(0.5f));
    g.drawRoundedRectangle(bounds, CORNER_RADIUS * scaleFactor, 1.0f);
    
    // Selected text
    g.setFont(juce::Font(juce::FontOptions(12.0f * scaleFactor).withName("Helvetica Neue")));
    g.setColour(PulseColors::TEXT_PRIMARY);
    
    juce::String displayText = selectedIndex >= 0 && selectedIndex < items.size() ?
                               items[selectedIndex] : "Select...";
    
    auto textBounds = bounds.reduced(10, 0);
    g.drawText(displayText, textBounds, juce::Justification::centredLeft);
    
    // Dropdown arrow
    float arrowSize = 8.0f * scaleFactor;
    float arrowX = bounds.getRight() - 15;
    float arrowY = bounds.getCentreY();
    
    juce::Path arrow;
    arrow.addTriangle(arrowX - arrowSize/2, arrowY - arrowSize/3,
                     arrowX + arrowSize/2, arrowY - arrowSize/3,
                     arrowX, arrowY + arrowSize/3);
    
    g.setColour(PulseColors::TEXT_SECONDARY);
    g.fillPath(arrow);
}

void PulseDropdown::resized()
{
    // Nothing specific needed here yet
}

void PulseDropdown::mouseDown(const juce::MouseEvent&)
{
    showPopup();
}

void PulseDropdown::setSelectedIndex(int index)
{
    if (index >= 0 && index < items.size())
    {
        selectedIndex = index;
        
        if (onSelectionChanged)
            onSelectionChanged(selectedIndex);
        
        repaint();
    }
}

void PulseDropdown::showPopup()
{
    // For now, just cycle through options
    // In a real implementation, this would show a popup menu
    setSelectedIndex((selectedIndex + 1) % items.size());
}

} // namespace HAM