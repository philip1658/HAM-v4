#include "AdvancedComponents.h"

namespace HAM {

//==============================================================================
// StageCard Implementation
//==============================================================================
StageCard::StageCard(const juce::String& name, int stageNumber)
    : PulseComponent(name), 
      stageNum(stageNumber),
      pitchSlider(std::make_unique<PulseVerticalSlider>("PITCH", stageNumber % 8)),
      pulseSlider(std::make_unique<PulseVerticalSlider>("PULSE", (stageNumber + 1) % 8)),
      velocitySlider(std::make_unique<PulseVerticalSlider>("VEL", (stageNumber + 2) % 8)),
      gateSlider(std::make_unique<PulseVerticalSlider>("GATE", (stageNumber + 3) % 8))
{
    // Add sliders to component
    
    addAndMakeVisible(pitchSlider.get());
    addAndMakeVisible(pulseSlider.get());
    addAndMakeVisible(velocitySlider.get());
    addAndMakeVisible(gateSlider.get());
    
    // Create buttons
    skipButton = std::make_unique<PulseButton>("SKIP", PulseButton::Outline);
    hamButton = std::make_unique<PulseButton>("HAM", PulseButton::Gradient);
    
    addAndMakeVisible(skipButton.get());
    addAndMakeVisible(hamButton.get());
}

void StageCard::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Draw card background with multi-layer shadow
    drawMultiLayerShadow(g, bounds, 3, 3.0f);
    
    // Card background gradient
    fillWithGradient(g, bounds,
                    PulseColors::BG_DARK,
                    PulseColors::BG_DARKEST);
    
    // Border (highlighted if active)
    g.setColour(isHighlighted ? 
               PulseColors::TRACK_CYAN.withAlpha(0.8f) : 
               PulseColors::BG_LIGHT.withAlpha(0.3f));
    g.drawRoundedRectangle(bounds, 3.0f, isHighlighted ? 2.0f : 1.0f);
    
    // Stage number header
    auto headerBounds = bounds.removeFromTop(30);
    g.setColour(PulseColors::BG_MID.withAlpha(0.5f));
    g.fillRoundedRectangle(headerBounds, 3.0f);
    
    g.setFont(juce::Font(juce::FontOptions(14.0f * scaleFactor).withName("Helvetica Neue").withStyle(juce::Font::bold)));
    g.setColour(PulseColors::TEXT_PRIMARY);
    g.drawText("STAGE " + juce::String(stageNum), 
              headerBounds, juce::Justification::centred);
    
    // Glow effect when highlighted
    if (isHighlighted)
    {
        UIUtils::drawGlow(g, bounds, PulseColors::TRACK_CYAN, 0.5f);
    }
}

void StageCard::resized()
{
    auto bounds = getLocalBounds();
    
    // Skip header
    bounds.removeFromTop(35);
    
    // 2x2 grid for sliders
    auto sliderArea = bounds.removeFromTop(getHeight() - 100);
    int sliderWidth = sliderArea.getWidth() / 2;
    int sliderHeight = sliderArea.getHeight() / 2;
    
    // Position sliders in 2x2 grid
    pitchSlider->setBounds(0, 0, sliderWidth, sliderHeight);
    pulseSlider->setBounds(sliderWidth, 0, sliderWidth, sliderHeight);
    velocitySlider->setBounds(0, sliderHeight, sliderWidth, sliderHeight);
    gateSlider->setBounds(sliderWidth, sliderHeight, sliderWidth, sliderHeight);
    
    // Buttons at bottom
    auto buttonArea = bounds.removeFromBottom(35);
    int buttonWidth = buttonArea.getWidth() / 2 - 10;
    
    skipButton->setBounds(buttonArea.getX() + 5, buttonArea.getY(),
                          buttonWidth, buttonArea.getHeight() - 10);
    hamButton->setBounds(buttonArea.getRight() - buttonWidth - 5, buttonArea.getY(),
                        buttonWidth, buttonArea.getHeight() - 10);
}

//==============================================================================
// ScaleSlotSelector Implementation
//==============================================================================
ScaleSlotSelector::ScaleSlotSelector(const juce::String& name)
    : PulseComponent(name)
{
    // Initialize slot names
    for (int i = 0; i < 8; ++i)
    {
        slots[i].name = "Slot " + juce::String(i + 1);
        slots[i].isActive = (i == 0); // First slot active by default
    }
}

void ScaleSlotSelector::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Background
    g.setColour(PulseColors::BG_VOID);
    g.fillRoundedRectangle(bounds, 3.0f);
    
    // Draw each slot
    for (int i = 0; i < 8; ++i)
    {
        auto& slot = slots[i];
        
        // Slot background
        if (slot.isActive)
        {
            fillWithGradient(g, slot.bounds,
                           PulseColors::TRACK_MINT.withAlpha(0.3f),
                           PulseColors::TRACK_MINT.withAlpha(0.1f));
        }
        else if (slot.hoverAmount > 0.01f)
        {
            g.setColour(PulseColors::BG_LIGHT.withAlpha(slot.hoverAmount * 0.5f));
            g.fillRoundedRectangle(slot.bounds, 2.0f);
        }
        else
        {
            g.setColour(PulseColors::BG_DARK);
            g.fillRoundedRectangle(slot.bounds, 2.0f);
        }
        
        // Slot border
        g.setColour(slot.isActive ? 
                   PulseColors::TRACK_MINT.withAlpha(0.8f) :
                   PulseColors::BG_LIGHT.withAlpha(0.3f));
        g.drawRoundedRectangle(slot.bounds, 2.0f, 1.0f);
        
        // Slot text
        g.setFont(juce::Font(juce::FontOptions(11.0f * scaleFactor).withName("Helvetica Neue")));
        g.setColour(slot.isActive ? PulseColors::TEXT_PRIMARY : PulseColors::TEXT_SECONDARY);
        g.drawText(slot.name, slot.bounds, juce::Justification::centred);
        
        // Animate hover
        slot.hoverAmount = smoothValue(slot.hoverAmount, 
                                       (i == hoveredSlot) ? 1.0f : 0.0f, 0.2f);
    }
}

void ScaleSlotSelector::resized()
{
    auto bounds = getLocalBounds().toFloat().reduced(5);
    float slotHeight = bounds.getHeight() / 8.0f;
    
    for (int i = 0; i < 8; ++i)
    {
        slots[i].bounds = bounds.removeFromTop(slotHeight).reduced(2);
    }
}

void ScaleSlotSelector::mouseDown(const juce::MouseEvent& event)
{
    int slot = getSlotAtPosition(event.position.toInt());
    if (slot >= 0)
    {
        setSelectedSlot(slot);
    }
}

void ScaleSlotSelector::mouseMove(const juce::MouseEvent& event)
{
    hoveredSlot = getSlotAtPosition(event.position.toInt());
    repaint();
}

void ScaleSlotSelector::mouseExit(const juce::MouseEvent&)
{
    hoveredSlot = -1;
    repaint();
}

void ScaleSlotSelector::setSelectedSlot(int slot)
{
    if (slot >= 0 && slot < 8)
    {
        // Deactivate all slots
        for (auto& s : slots)
            s.isActive = false;
        
        // Activate selected slot
        slots[slot].isActive = true;
        selectedSlot = slot;
        
        if (onSlotSelected)
            onSlotSelected(slot);
        
        repaint();
    }
}

void ScaleSlotSelector::setSlotName(int slot, const juce::String& name)
{
    if (slot >= 0 && slot < 8)
    {
        slots[slot].name = name;
        repaint();
    }
}

int ScaleSlotSelector::getSlotAtPosition(juce::Point<int> pos)
{
    for (int i = 0; i < 8; ++i)
    {
        if (slots[i].bounds.contains(pos.toFloat()))
            return i;
    }
    return -1;
}

//==============================================================================
// GatePatternEditor Implementation
//==============================================================================
GatePatternEditor::GatePatternEditor(const juce::String& name)
    : PulseComponent(name)
{
}

void GatePatternEditor::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Background
    g.setColour(PulseColors::BG_VOID);
    g.fillRoundedRectangle(bounds, 3.0f);
    
    // Draw grid
    float stepWidth = bounds.getWidth() / 8.0f;
    
    for (int i = 0; i < 8; ++i)
    {
        auto stepBounds = juce::Rectangle<float>(
            bounds.getX() + i * stepWidth,
            bounds.getY(),
            stepWidth,
            bounds.getHeight()
        ).reduced(2);
        
        // Step background
        g.setColour(PulseColors::BG_DARK);
        g.fillRoundedRectangle(stepBounds, 2.0f);
        
        // Gate value bar
        float barHeight = stepBounds.getHeight() * gatePattern[i];
        auto barBounds = stepBounds.withHeight(barHeight)
                                  .withBottomY(stepBounds.getBottom());
        
        // Gate bar gradient
        fillWithGradient(g, barBounds,
                        PulseColors::TRACK_CYAN.withAlpha(0.9f),
                        PulseColors::TRACK_CYAN.withAlpha(0.5f));
        
        // Step border
        g.setColour(PulseColors::BG_LIGHT.withAlpha(0.3f));
        g.drawRoundedRectangle(stepBounds, 2.0f, 0.5f);
        
        // Step number
        g.setFont(juce::Font(juce::FontOptions(9.0f).withName("Helvetica Neue")));
        g.setColour(PulseColors::TEXT_DIMMED);
        g.drawText(juce::String(i + 1), 
                  stepBounds.removeFromBottom(15),
                  juce::Justification::centred);
    }
}

void GatePatternEditor::resized()
{
    // Nothing specific needed here
}

void GatePatternEditor::mouseDown(const juce::MouseEvent& event)
{
    draggedStep = getStepAtPosition(event.position.toInt());
    if (draggedStep >= 0)
    {
        float newValue = getValueAtPosition(event.position.y);
        gatePattern[draggedStep] = newValue;
        
        if (onGateChanged)
            onGateChanged(draggedStep, newValue);
        
        repaint();
    }
}

void GatePatternEditor::mouseDrag(const juce::MouseEvent& event)
{
    if (draggedStep >= 0)
    {
        float newValue = getValueAtPosition(event.position.y);
        gatePattern[draggedStep] = newValue;
        
        if (onGateChanged)
            onGateChanged(draggedStep, newValue);
        
        repaint();
    }
}

void GatePatternEditor::setPattern(const std::array<float, 8>& pattern)
{
    gatePattern = pattern;
    repaint();
}

int GatePatternEditor::getStepAtPosition(juce::Point<int> pos)
{
    float stepWidth = getWidth() / 8.0f;
    int step = static_cast<int>(pos.x / stepWidth);
    return (step >= 0 && step < 8) ? step : -1;
}

float GatePatternEditor::getValueAtPosition(int y)
{
    float value = 1.0f - (static_cast<float>(y) / getHeight());
    return juce::jlimit(0.0f, 1.0f, value);
}

//==============================================================================
// PitchTrajectoryVisualizer Implementation
//==============================================================================
PitchTrajectoryVisualizer::PitchTrajectoryVisualizer(const juce::String& name)
    : PulseComponent(name)
{
    startTimerHz(30); // 30 FPS for smooth animation
}

PitchTrajectoryVisualizer::~PitchTrajectoryVisualizer()
{
    stopTimer();
}

void PitchTrajectoryVisualizer::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Background with subtle gradient
    fillWithGradient(g, bounds,
                    PulseColors::BG_VOID,
                    PulseColors::BG_DARKEST);
    
    // Draw grid lines
    g.setColour(PulseColors::BG_LIGHT.withAlpha(0.2f));
    
    // Horizontal lines (pitch levels)
    for (int i = -2; i <= 2; ++i)
    {
        float y = bounds.getCentreY() + (i * bounds.getHeight() / 6.0f);
        g.drawHorizontalLine(static_cast<int>(y), bounds.getX(), bounds.getRight());
    }
    
    // Vertical lines (time divisions)
    for (int i = 1; i < 8; ++i)
    {
        float x = bounds.getX() + (i * bounds.getWidth() / 8.0f);
        g.drawVerticalLine(static_cast<int>(x), bounds.getY(), bounds.getBottom());
    }
    
    // Draw center line (zero reference)
    g.setColour(PulseColors::BG_LIGHT.withAlpha(0.5f));
    g.drawHorizontalLine(static_cast<int>(bounds.getCentreY()), 
                        bounds.getX(), bounds.getRight());
    
    // Draw trajectory path
    if (trajectory.size() > 1)
    {
        juce::Path path;
        bool firstPoint = true;
        
        for (const auto& point : trajectory)
        {
            auto pos = valueToPoint(point.pitch, point.time);
            
            if (firstPoint)
            {
                path.startNewSubPath(pos);
                firstPoint = false;
            }
            else
            {
                path.lineTo(pos);
            }
        }
        
        // Draw path with glow
        g.setColour(PulseColors::TRACK_CYAN.withAlpha(0.3f));
        g.strokePath(path, juce::PathStrokeType(4.0f));
        
        g.setColour(PulseColors::TRACK_CYAN.withAlpha(0.8f));
        g.strokePath(path, juce::PathStrokeType(2.0f));
    }
    
    // Draw current accumulator position with spring animation
    float displayY = bounds.getCentreY() - (springPosition * bounds.getHeight() / 4.0f);
    
    // Accumulator indicator with glow
    g.setColour(PulseColors::TRACK_MINT.withAlpha(0.3f));
    g.fillEllipse(bounds.getRight() - 20, displayY - 8, 16, 16);
    
    g.setColour(PulseColors::TRACK_MINT);
    g.fillEllipse(bounds.getRight() - 18, displayY - 6, 12, 12);
    
    // Draw value text
    g.setFont(juce::Font(juce::FontOptions(10.0f).withName("Helvetica Neue")));
    g.setColour(PulseColors::TEXT_PRIMARY);
    g.drawText(juce::String(static_cast<int>(currentAccumulatorValue)),
              bounds.removeFromTop(20), juce::Justification::topRight);
}

void PitchTrajectoryVisualizer::resized()
{
    // Nothing specific needed here
}

void PitchTrajectoryVisualizer::addPitchPoint(float pitch, float time)
{
    trajectory.push_back({pitch, time, 1.0f});
    
    // Limit trajectory size
    while (trajectory.size() > maxPoints)
    {
        trajectory.pop_front();
    }
    
    repaint();
}

void PitchTrajectoryVisualizer::clearTrajectory()
{
    trajectory.clear();
    repaint();
}

void PitchTrajectoryVisualizer::setAccumulatorValue(float value)
{
    currentAccumulatorValue = value;
    // Spring will animate to this position
    repaint();
}

void PitchTrajectoryVisualizer::setScaleRange(int min, int max)
{
    scaleMin = min;
    scaleMax = max;
    repaint();
}

void PitchTrajectoryVisualizer::timerCallback()
{
    updateSpringAnimation();
    
    // Fade out old trajectory points
    for (auto& point : trajectory)
    {
        point.alpha *= 0.98f;
    }
    
    repaint();
}

void PitchTrajectoryVisualizer::updateSpringAnimation()
{
    springPosition = UIUtils::calculateSpring(springPosition, 
                                             currentAccumulatorValue,
                                             springVelocity);
}

juce::Point<float> PitchTrajectoryVisualizer::valueToPoint(float pitch, float time)
{
    auto bounds = getLocalBounds().toFloat();
    
    float x = bounds.getX() + (time * bounds.getWidth());
    float normalizedPitch = (pitch - scaleMin) / static_cast<float>(scaleMax - scaleMin);
    float y = bounds.getBottom() - (normalizedPitch * bounds.getHeight());
    
    return {x, y};
}

} // namespace HAM