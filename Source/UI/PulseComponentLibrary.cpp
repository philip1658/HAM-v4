#include "PulseComponentLibrary.h"

namespace HAM {

//==============================================================================
// EXACT Pulse Color Definitions
const juce::Colour PulseComponentLibrary::PulseColors::BG_VOID(0xFF000000);
const juce::Colour PulseComponentLibrary::PulseColors::BG_DARKEST(0xFF0A0A0A);
const juce::Colour PulseComponentLibrary::PulseColors::BG_DARK(0xFF1A1A1A);
const juce::Colour PulseComponentLibrary::PulseColors::BG_MID(0xFF2A2A2A);
const juce::Colour PulseComponentLibrary::PulseColors::BG_LIGHT(0xFF3A3A3A);
const juce::Colour PulseComponentLibrary::PulseColors::BG_RAISED(0xFF4A4A4A);
const juce::Colour PulseComponentLibrary::PulseColors::BG_HIGHLIGHT(0xFF5A5A5A);

const juce::Colour PulseComponentLibrary::PulseColors::TEXT_PRIMARY(0xFFFFFFFF);
const juce::Colour PulseComponentLibrary::PulseColors::TEXT_SECONDARY(0xFFCCCCCC);
const juce::Colour PulseComponentLibrary::PulseColors::TEXT_DIMMED(0xFF888888);
const juce::Colour PulseComponentLibrary::PulseColors::TEXT_DISABLED(0xFF555555);

const juce::Colour PulseComponentLibrary::PulseColors::TRACK_MINT(0xFF00FF88);
const juce::Colour PulseComponentLibrary::PulseColors::TRACK_CYAN(0xFF00D9FF);
const juce::Colour PulseComponentLibrary::PulseColors::TRACK_PINK(0xFFFF0088);
const juce::Colour PulseComponentLibrary::PulseColors::TRACK_AMBER(0xFFFFAA00);
const juce::Colour PulseComponentLibrary::PulseColors::TRACK_PURPLE(0xFFFF00FF);
const juce::Colour PulseComponentLibrary::PulseColors::TRACK_BLUE(0xFF0088FF);
const juce::Colour PulseComponentLibrary::PulseColors::TRACK_RED(0xFFFF0044);
const juce::Colour PulseComponentLibrary::PulseColors::TRACK_YELLOW(0xFFFFFF00);

const juce::Colour PulseComponentLibrary::PulseColors::GLOW_CYAN(0x4400FFFF);
const juce::Colour PulseComponentLibrary::PulseColors::GLOW_GREEN(0x4400FF00);
const juce::Colour PulseComponentLibrary::PulseColors::ERROR_RED(0xFFFF0000);
const juce::Colour PulseComponentLibrary::PulseColors::WARNING_AMBER(0xFFFFAA00);

//==============================================================================
PulseComponentLibrary::PulseComponentLibrary()
{
    setSize(1400, 900);
    
    positionLabel.setText("Grid: --", juce::dontSendNotification);
    positionLabel.setColour(juce::Label::textColourId, PulseColors::TEXT_SECONDARY);
    positionLabel.setJustificationType(juce::Justification::topRight);
    addAndMakeVisible(positionLabel);
    
    createAllComponents();
    layoutComponents();
    
    startTimerHz(60); // 60 FPS for smooth animations
}

PulseComponentLibrary::~PulseComponentLibrary()
{
    stopTimer();
}

//==============================================================================
juce::Rectangle<int> PulseComponentLibrary::getGridCell(char row, int col, int rowSpan, int colSpan) const
{
    int rowIndex = row - 'A';
    int colIndex = col - 1;
    
    if (rowIndex < 0 || rowIndex >= 24 || colIndex < 0 || colIndex >= 24)
        return {};
    
    return juce::Rectangle<int>(
        colIndex * grid.cellWidth,
        rowIndex * grid.cellHeight,
        grid.cellWidth * colSpan,
        grid.cellHeight * rowSpan
    );
}

juce::String PulseComponentLibrary::getGridPosition(const juce::Point<int>& point) const
{
    int col = (point.x / grid.cellWidth) + 1;
    int row = (point.y / grid.cellHeight);
    
    if (col < 1 || col > 24 || row < 0 || row >= 24)
        return "--";
    
    char rowChar = static_cast<char>('A' + row);
    return juce::String::charToString(rowChar) + juce::String(col);
}

//==============================================================================
void PulseComponentLibrary::paint(juce::Graphics& g)
{
    // Pulse-style dark void background
    g.fillAll(PulseColors::BG_VOID);
    
    // Subtle gradient overlay
    juce::ColourGradient bgGradient(
        PulseColors::BG_DARKEST.withAlpha(0.8f), 0, 0,
        PulseColors::BG_VOID, getWidth(), getHeight(), true);
    g.setGradientFill(bgGradient);
    g.fillAll();
    
    // Grid
    if (grid.showGrid)
    {
        g.setColour(PulseColors::BG_LIGHT.withAlpha(0.15f));
        
        for (int i = 0; i <= 24; ++i)
        {
            int x = i * grid.cellWidth;
            g.drawLine(x, 0, x, getHeight(), 0.5f);
            
            int y = i * grid.cellHeight;
            g.drawLine(0, y, getWidth(), y, 0.5f);
        }
    }
    
    // Grid labels
    if (grid.showLabels)
    {
        g.setFont(juce::Font("Helvetica Neue", 10.0f, juce::Font::plain));
        g.setColour(PulseColors::TEXT_DIMMED.withAlpha(0.5f));
        
        for (int i = 0; i < 24; ++i)
        {
            // Column numbers
            auto colBounds = juce::Rectangle<int>(i * grid.cellWidth, 0, grid.cellWidth, 15);
            g.drawText(juce::String(i + 1), colBounds, juce::Justification::centred);
            
            // Row letters
            char letter = static_cast<char>('A' + i);
            auto rowBounds = juce::Rectangle<int>(0, i * grid.cellHeight, 15, grid.cellHeight);
            g.drawText(juce::String::charToString(letter), rowBounds, juce::Justification::centred);
        }
    }
    
    // Hover highlight with glow
    if (grid.hoveredCell.x >= 0 && grid.hoveredCell.y >= 0)
    {
        auto cellBounds = juce::Rectangle<int>(
            grid.hoveredCell.x * grid.cellWidth,
            grid.hoveredCell.y * grid.cellHeight,
            grid.cellWidth,
            grid.cellHeight
        );
        
        g.setColour(PulseColors::GLOW_CYAN);
        g.fillRect(cellBounds);
        g.setColour(PulseColors::TRACK_CYAN.withAlpha(0.3f));
        g.drawRect(cellBounds, 1);
    }
}

void PulseComponentLibrary::resized()
{
    grid.cellWidth = getWidth() / 24;
    grid.cellHeight = (getHeight() - 30) / 24;
    
    positionLabel.setBounds(getWidth() - 100, 5, 90, 20);
    
    layoutComponents();
}

void PulseComponentLibrary::mouseMove(const juce::MouseEvent& event)
{
    auto pos = event.getPosition();
    grid.hoveredCell.x = pos.x / grid.cellWidth;
    grid.hoveredCell.y = pos.y / grid.cellHeight;
    
    positionLabel.setText("Grid: " + getGridPosition(pos), juce::dontSendNotification);
    repaint();
}

//==============================================================================
void PulseComponentLibrary::timerCallback()
{
    // Update all component animations
    for (auto& [name, comp] : components)
    {
        comp->repaint();
    }
}

void PulseComponentLibrary::createAllComponents()
{
    // VERTICAL SLIDERS (Pulse-style with line indicators)
    for (int i = 0; i < 8; ++i)
    {
        auto name = "VSLIDER_" + juce::String(i + 1);
        components[name] = std::make_unique<PulseVerticalSlider>(name, i);
    }
    
    // HORIZONTAL SLIDERS
    components["HSLIDER_1"] = std::make_unique<PulseHorizontalSlider>("HSLIDER_1", true);
    components["HSLIDER_2"] = std::make_unique<PulseHorizontalSlider>("HSLIDER_2", false);
    
    // BUTTONS (various styles)
    components["BTN_SOLID"] = std::make_unique<PulseButton>("PLAY", PulseButton::Solid);
    components["BTN_OUTLINE"] = std::make_unique<PulseButton>("STOP", PulseButton::Outline);
    components["BTN_GHOST"] = std::make_unique<PulseButton>("RECORD", PulseButton::Ghost);
    components["BTN_GRADIENT"] = std::make_unique<PulseButton>("HAM", PulseButton::Gradient);
    
    // TOGGLES
    components["TOGGLE_MUTE"] = std::make_unique<PulseToggle>("MUTE");
    components["TOGGLE_SOLO"] = std::make_unique<PulseToggle>("SOLO");
    components["TOGGLE_MONO"] = std::make_unique<PulseToggle>("MONO");
    
    // DROPDOWNS
    components["DROPDOWN_SCALE"] = std::make_unique<PulseDropdown>("SCALE");
    components["DROPDOWN_CHANNEL"] = std::make_unique<PulseDropdown>("CHANNEL");
    
    // PANELS
    components["PANEL_FLAT"] = std::make_unique<PulsePanel>("FLAT", PulsePanel::Flat);
    components["PANEL_RAISED"] = std::make_unique<PulsePanel>("RAISED", PulsePanel::Raised);
    components["PANEL_GLASS"] = std::make_unique<PulsePanel>("GLASS", PulsePanel::Glass);
    components["PANEL_TRACK"] = std::make_unique<PulsePanel>("TRACK_BG", PulsePanel::TrackControl);
    
    // STAGE CARDS
    for (int i = 0; i < 8; ++i)
    {
        auto name = "STAGE_" + juce::String(i + 1);
        components[name] = std::make_unique<StageCard>(name, i + 1);
    }
    
    // SCALE SLOT SELECTOR
    components["SCALE_SLOTS"] = std::make_unique<ScaleSlotSelector>("SCALE_SLOTS");
    
    // GATE PATTERN EDITOR
    components["GATE_PATTERN"] = std::make_unique<GatePatternEditor>("GATE_PATTERN");
    
    // PITCH TRAJECTORY VISUALIZER
    components["PITCH_VIZ"] = std::make_unique<PitchTrajectoryVisualizer>("PITCH_VIZ");
    
    // TRACK CONTROL PANELS
    for (int i = 0; i < 4; ++i)
    {
        auto name = "TRACK_CTRL_" + juce::String(i + 1);
        components[name] = std::make_unique<TrackControlPanel>(name, i + 1);
    }
    
    // Add all to view
    for (auto& [name, comp] : components)
    {
        addAndMakeVisible(comp.get());
    }
}

void PulseComponentLibrary::layoutComponents()
{
    // Row B-H: Vertical Sliders (8 track colors)
    for (int i = 0; i < 8; ++i)
    {
        if (auto* comp = components["VSLIDER_" + juce::String(i + 1)].get())
            comp->setBounds(getGridCell('B' + i, 2, 6, 1));
    }
    
    // Row B: Horizontal Sliders
    if (auto* comp = components["HSLIDER_1"].get())
        comp->setBounds(getGridCell('B', 4, 1, 4));
    if (auto* comp = components["HSLIDER_2"].get())
        comp->setBounds(getGridCell('C', 4, 1, 4));
    
    // Row E: Buttons
    if (auto* comp = components["BTN_SOLID"].get())
        comp->setBounds(getGridCell('E', 4, 1, 2));
    if (auto* comp = components["BTN_OUTLINE"].get())
        comp->setBounds(getGridCell('E', 7, 1, 2));
    if (auto* comp = components["BTN_GHOST"].get())
        comp->setBounds(getGridCell('F', 4, 1, 2));
    if (auto* comp = components["BTN_GRADIENT"].get())
        comp->setBounds(getGridCell('F', 7, 1, 2));
    
    // Row H: Toggles
    if (auto* comp = components["TOGGLE_MUTE"].get())
        comp->setBounds(getGridCell('H', 4, 1, 2));
    if (auto* comp = components["TOGGLE_SOLO"].get())
        comp->setBounds(getGridCell('H', 7, 1, 2));
    if (auto* comp = components["TOGGLE_MONO"].get())
        comp->setBounds(getGridCell('I', 4, 1, 2));
    
    // Row J: Dropdowns
    if (auto* comp = components["DROPDOWN_SCALE"].get())
        comp->setBounds(getGridCell('J', 4, 1, 3));
    if (auto* comp = components["DROPDOWN_CHANNEL"].get())
        comp->setBounds(getGridCell('J', 8, 1, 3));
    
    // Row L-O: Panels
    if (auto* comp = components["PANEL_FLAT"].get())
        comp->setBounds(getGridCell('L', 4, 2, 4));
    if (auto* comp = components["PANEL_RAISED"].get())
        comp->setBounds(getGridCell('L', 9, 2, 4));
    if (auto* comp = components["PANEL_GLASS"].get())
        comp->setBounds(getGridCell('N', 4, 2, 4));
    if (auto* comp = components["PANEL_TRACK"].get())
        comp->setBounds(getGridCell('N', 9, 2, 4));
    
    // Row B-F: Stage Cards (right side)
    for (int i = 0; i < 4; ++i)
    {
        if (auto* comp = components["STAGE_" + juce::String(i + 1)].get())
            comp->setBounds(getGridCell('B', 10 + i * 3, 8, 3));
    }
    
    // Row K: Scale Slot Selector
    if (auto* comp = components["SCALE_SLOTS"].get())
        comp->setBounds(getGridCell('K', 10, 2, 12));
    
    // Row M: Gate Pattern Editor
    if (auto* comp = components["GATE_PATTERN"].get())
        comp->setBounds(getGridCell('M', 10, 2, 12));
    
    // Row O: Pitch Trajectory Visualizer
    if (auto* comp = components["PITCH_VIZ"].get())
        comp->setBounds(getGridCell('O', 10, 6, 12));
    
    // Row Q-T: Track Control Panels
    for (int i = 0; i < 4; ++i)
    {
        if (auto* comp = components["TRACK_CTRL_" + juce::String(i + 1)].get())
            comp->setBounds(getGridCell('Q' + i, 4, 2, 8));
    }
}

//==============================================================================
// BASE COMPONENT HELPER
void PulseComponentLibrary::PulseComponent::drawMultiLayerShadow(juce::Graphics& g, 
                                                                 juce::Rectangle<float> bounds,
                                                                 int layers, 
                                                                 float cornerRadius)
{
    // Draw multiple shadow layers for depth (Pulse-style)
    for (int i = layers; i > 0; --i)
    {
        float offset = i * 1.0f;
        float expansion = i * 0.5f;
        float alpha = 0.15f / i;
        
        g.setColour(juce::Colour(0xFF000000).withAlpha(alpha));
        g.fillRoundedRectangle(bounds.translated(0, offset).expanded(expansion), cornerRadius);
    }
}

//==============================================================================
// VERTICAL SLIDER IMPLEMENTATION (EXACT Pulse Copy)
PulseComponentLibrary::PulseVerticalSlider::PulseVerticalSlider(const juce::String& name, int trackColorIndex)
    : PulseComponent(name), colorIndex(trackColorIndex)
{
    // Select track color
    const juce::Colour trackColors[] = {
        PulseColors::TRACK_MINT, PulseColors::TRACK_CYAN,
        PulseColors::TRACK_PINK, PulseColors::TRACK_AMBER,
        PulseColors::TRACK_PURPLE, PulseColors::TRACK_BLUE,
        PulseColors::TRACK_RED, PulseColors::TRACK_YELLOW
    };
    
    trackColor = trackColors[trackColorIndex % 8];
}

void PulseComponentLibrary::PulseVerticalSlider::paint(juce::Graphics& g)
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
    float valueY = trackBounds.getY() + (1.0f - displayedValue) * trackBounds.getHeight();
    
    // Value fill (from bottom to current value)
    auto fillBounds = trackBounds.withTop(valueY);
    
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
    g.setFont(juce::Font("Helvetica Neue", 10.0f * scaleFactor, juce::Font::plain));
    g.setColour(PulseColors::TEXT_SECONDARY);
    g.drawText(componentName, bounds.removeFromBottom(15), juce::Justification::centred);
    
    // Smooth animation
    displayedValue += (value - displayedValue) * ANIMATION_SPEED;
}

void PulseComponentLibrary::PulseVerticalSlider::mouseDown(const juce::MouseEvent& event)
{
    mouseDrag(event);
    glowIntensity = 1.0f;
}

void PulseComponentLibrary::PulseVerticalSlider::mouseDrag(const juce::MouseEvent& event)
{
    float newValue = 1.0f - (event.position.y / getHeight());
    value = targetValue = juce::jlimit(0.0f, 1.0f, newValue);
}

void PulseComponentLibrary::PulseVerticalSlider::mouseEnter(const juce::MouseEvent&)
{
    isHovering = true;
    hoverAmount = 1.0f;
}

void PulseComponentLibrary::PulseVerticalSlider::mouseExit(const juce::MouseEvent&)
{
    isHovering = false;
    hoverAmount = 0.0f;
    glowIntensity *= 0.9f;
}

//==============================================================================
// HORIZONTAL SLIDER IMPLEMENTATION
PulseComponentLibrary::PulseHorizontalSlider::PulseHorizontalSlider(const juce::String& name, bool showThumb)
    : PulseComponent(name), hasThumb(showThumb)
{
    trackColor = PulseColors::TRACK_CYAN;
}

void PulseComponentLibrary::PulseHorizontalSlider::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    float scaledHeight = TRACK_HEIGHT * scaleFactor;
    auto trackBounds = bounds.withSizeKeepingCentre(bounds.getWidth() - 20, scaledHeight);
    
    // Multi-layer shadow
    drawMultiLayerShadow(g, trackBounds, 2, scaledHeight * 0.3f);
    
    // Track background
    g.setColour(PulseColors::BG_DARK.darker(0.3f));
    g.fillRoundedRectangle(trackBounds, scaledHeight * 0.3f);
    
    // Inner shadow
    g.setColour(PulseColors::BG_VOID.withAlpha(0.5f));
    g.drawRoundedRectangle(trackBounds.reduced(0.5f), scaledHeight * 0.3f - 0.5f, 1.0f);
    
    // Filled portion
    auto fillBounds = trackBounds.withWidth(value * trackBounds.getWidth());
    g.setColour(trackColor.withAlpha(0.3f));
    g.fillRoundedRectangle(fillBounds, scaledHeight * 0.3f);
    
    if (hasThumb)
    {
        // Thumb position
        float thumbX = trackBounds.getX() + value * trackBounds.getWidth();
        float thumbY = trackBounds.getCentreY();
        float scaledThumbSize = THUMB_SIZE * scaleFactor;
        
        // Thumb shadow
        g.setColour(PulseColors::BG_VOID.withAlpha(0.3f));
        g.fillEllipse(thumbX - scaledThumbSize/2, thumbY - scaledThumbSize/2 + 1, 
                     scaledThumbSize, scaledThumbSize);
        
        // Thumb gradient
        juce::ColourGradient thumbGradient(
            trackColor.brighter(0.2f), thumbX, thumbY - scaledThumbSize/2,
            trackColor.darker(0.2f), thumbX, thumbY + scaledThumbSize/2, false);
        g.setGradientFill(thumbGradient);
        g.fillEllipse(thumbX - scaledThumbSize/2, thumbY - scaledThumbSize/2, 
                     scaledThumbSize, scaledThumbSize);
        
        // Thumb highlight
        g.setColour(PulseColors::BG_HIGHLIGHT.withAlpha(0.3f));
        g.drawEllipse(thumbX - scaledThumbSize/2 + 1, thumbY - scaledThumbSize/2 + 1, 
                     scaledThumbSize - 2, scaledThumbSize - 2, 0.5f);
    }
    
    // Label
    g.setFont(juce::Font("Helvetica Neue", 10.0f * scaleFactor, juce::Font::plain));
    g.setColour(PulseColors::TEXT_SECONDARY);
    g.drawText(componentName, bounds.removeFromBottom(15), juce::Justification::centred);
}

void PulseComponentLibrary::PulseHorizontalSlider::mouseDown(const juce::MouseEvent& event)
{
    mouseDrag(event);
}

void PulseComponentLibrary::PulseHorizontalSlider::mouseDrag(const juce::MouseEvent& event)
{
    float newValue = event.position.x / getWidth();
    value = juce::jlimit(0.0f, 1.0f, newValue);
    repaint();
}

//==============================================================================
// BUTTON IMPLEMENTATION
PulseComponentLibrary::PulseButton::PulseButton(const juce::String& name, Style style)
    : PulseComponent(name), buttonStyle(style)
{
    baseColor = (style == Solid || style == Gradient) ? PulseColors::TRACK_MINT : PulseColors::TRACK_CYAN;
}

void PulseComponentLibrary::PulseButton::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(3);
    float cornerRadius = 6.0f * scaleFactor;
    
    // Multi-layer shadow (Pulse signature)
    if (buttonStyle == Solid || buttonStyle == Gradient)
    {
        drawMultiLayerShadow(g, bounds, 3, cornerRadius);
    }
    
    // Button background
    switch (buttonStyle)
    {
        case Solid:
            g.setColour(baseColor.withAlpha(0.9f + hoverAmount * 0.1f));
            g.fillRoundedRectangle(bounds, cornerRadius);
            break;
            
        case Outline:
            g.setColour(baseColor.withAlpha(0.3f + hoverAmount * 0.3f));
            g.drawRoundedRectangle(bounds, cornerRadius, 2.0f);
            break;
            
        case Ghost:
            if (hoverAmount > 0.01f)
            {
                g.setColour(PulseColors::BG_LIGHT.withAlpha(hoverAmount * 0.5f));
                g.fillRoundedRectangle(bounds, cornerRadius);
            }
            break;
            
        case Gradient:
            {
                juce::ColourGradient btnGradient(
                    baseColor.withAlpha(0.9f), bounds.getCentreX(), bounds.getY(),
                    baseColor.darker(0.3f).withAlpha(0.9f), bounds.getCentreX(), bounds.getBottom(), false);
                g.setGradientFill(btnGradient);
                g.fillRoundedRectangle(bounds, cornerRadius);
                
                // Glass effect
                auto glassBounds = bounds.removeFromTop(bounds.getHeight() * 0.4f);
                g.setColour(PulseColors::TEXT_PRIMARY.withAlpha(0.1f));
                g.fillRoundedRectangle(glassBounds, cornerRadius);
            }
            break;
    }
    
    // Click pulse animation
    if (clickAnimation > 0.01f)
    {
        g.setColour(PulseColors::TEXT_PRIMARY.withAlpha(clickAnimation * 0.3f));
        g.drawRoundedRectangle(bounds.expanded(clickAnimation * 4), cornerRadius + 2, 2.0f);
    }
    
    // Hover glow
    if (isHovering && hoverAmount > 0.01f)
    {
        g.setColour(baseColor.withAlpha(hoverAmount * 0.2f));
        g.drawRoundedRectangle(bounds.expanded(2), cornerRadius + 2, 2.0f);
    }
    
    // Text
    g.setFont(juce::Font("Helvetica Neue", 14.0f * scaleFactor, juce::Font::bold));
    g.setColour(buttonStyle == Solid || buttonStyle == Gradient ? 
                PulseColors::BG_VOID : PulseColors::TEXT_PRIMARY);
    g.drawText(componentName, bounds, juce::Justification::centred);
    
    // Update animations
    hoverAmount *= 0.95f;
    clickAnimation *= 0.9f;
}

void PulseComponentLibrary::PulseButton::mouseEnter(const juce::MouseEvent&)
{
    isHovering = true;
    hoverAmount = 1.0f;
}

void PulseComponentLibrary::PulseButton::mouseExit(const juce::MouseEvent&)
{
    isHovering = false;
}

void PulseComponentLibrary::PulseButton::mouseDown(const juce::MouseEvent&)
{
    isPressed = true;
    clickAnimation = 1.0f;
}

void PulseComponentLibrary::PulseButton::mouseUp(const juce::MouseEvent&)
{
    isPressed = false;
}

//==============================================================================
// TOGGLE IMPLEMENTATION (iOS-style)
PulseComponentLibrary::PulseToggle::PulseToggle(const juce::String& name)
    : PulseComponent(name)
{
}

void PulseComponentLibrary::PulseToggle::paint(juce::Graphics& g)
{
    float scaledWidth = SWITCH_WIDTH * scaleFactor;
    float scaledHeight = SWITCH_HEIGHT * scaleFactor;
    float scaledThumbSize = THUMB_SIZE * scaleFactor;
    
    auto switchBounds = getLocalBounds().toFloat()
        .withSizeKeepingCentre(scaledWidth, scaledHeight);
    
    // Track shadow
    drawMultiLayerShadow(g, switchBounds, 2, scaledHeight * 0.5f);
    
    // Track background
    juce::Colour trackBg = isOn ? 
        PulseColors::TRACK_MINT.withAlpha(0.3f) : 
        PulseColors::BG_DARK;
    
    g.setColour(trackBg);
    g.fillRoundedRectangle(switchBounds, scaledHeight * 0.5f);
    
    // Inner shadow
    g.setColour(PulseColors::BG_VOID.withAlpha(0.5f));
    g.drawRoundedRectangle(switchBounds.reduced(0.5f), scaledHeight * 0.5f - 0.5f, 1.0f);
    
    // Animated thumb position
    thumbPosition += (isOn ? 1.0f - thumbPosition : 0.0f - thumbPosition) * ANIMATION_SPEED;
    float thumbX = switchBounds.getX() + 2 + thumbPosition * (scaledWidth - scaledThumbSize - 4);
    float thumbY = switchBounds.getCentreY() - scaledThumbSize * 0.5f;
    
    // Thumb shadow
    g.setColour(PulseColors::BG_VOID.withAlpha(0.4f));
    g.fillEllipse(thumbX, thumbY + 1, scaledThumbSize, scaledThumbSize);
    
    // Thumb
    juce::Colour thumbColor = isOn ? PulseColors::TRACK_MINT : PulseColors::TEXT_SECONDARY;
    g.setColour(thumbColor);
    g.fillEllipse(thumbX, thumbY, scaledThumbSize, scaledThumbSize);
    
    // Thumb highlight
    g.setColour(PulseColors::TEXT_PRIMARY.withAlpha(0.2f));
    g.drawEllipse(thumbX + 1, thumbY + 1, scaledThumbSize - 2, scaledThumbSize - 2, 0.5f);
    
    // Label
    g.setFont(juce::Font("Helvetica Neue", 10.0f * scaleFactor, juce::Font::plain));
    g.setColour(PulseColors::TEXT_SECONDARY);
    g.drawText(componentName, getLocalBounds().removeFromBottom(15), juce::Justification::centred);
}

void PulseComponentLibrary::PulseToggle::mouseDown(const juce::MouseEvent&)
{
    isOn = !isOn;
    repaint();
}

//==============================================================================
// DROPDOWN IMPLEMENTATION
PulseComponentLibrary::PulseDropdown::PulseDropdown(const juce::String& name)
    : PulseComponent(name)
{
}

void PulseComponentLibrary::PulseDropdown::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(2);
    float cornerRadius = 4.0f * scaleFactor;
    
    // Multi-layer shadow (3 layers - Pulse signature)
    drawMultiLayerShadow(g, bounds, 3, cornerRadius);
    
    // Background gradient (dark to darker)
    juce::ColourGradient bgGradient(
        PulseColors::BG_LIGHT.withAlpha(0.9f), bounds.getCentreX(), bounds.getY(),
        PulseColors::BG_MID.withAlpha(0.7f), bounds.getCentreX(), bounds.getBottom(), false);
    g.setGradientFill(bgGradient);
    g.fillRoundedRectangle(bounds, cornerRadius);
    
    // Inner highlight
    g.setColour(PulseColors::BG_HIGHLIGHT.withAlpha(0.2f));
    g.drawRoundedRectangle(bounds.reduced(1), cornerRadius - 1, 0.5f);
    
    // Hover glow (cyan)
    if (hoverAmount > 0.01f)
    {
        g.setColour(PulseColors::GLOW_CYAN.withAlpha(hoverAmount));
        g.drawRoundedRectangle(bounds, cornerRadius, 2.0f);
    }
    
    // Text
    auto textBounds = bounds.reduced(8, 0);
    g.setFont(juce::Font("Helvetica Neue", 12.0f * scaleFactor, juce::Font::plain));
    g.setColour(PulseColors::TEXT_PRIMARY);
    g.drawText(selectedText, textBounds, juce::Justification::centredLeft);
    
    // Arrow
    drawArrow(g, bounds.removeFromRight(25));
    
    // Label
    g.setFont(juce::Font("Helvetica Neue", 10.0f * scaleFactor, juce::Font::plain));
    g.setColour(PulseColors::TEXT_SECONDARY);
    g.drawText(componentName, getLocalBounds().removeFromBottom(15), juce::Justification::centred);
    
    // Update animation
    hoverAmount *= 0.95f;
}

void PulseComponentLibrary::PulseDropdown::drawArrow(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    juce::Path arrow;
    float size = 6.0f * scaleFactor;
    arrow.addTriangle(
        bounds.getCentreX() - size, bounds.getCentreY() - size * 0.5f,
        bounds.getCentreX() + size, bounds.getCentreY() - size * 0.5f,
        bounds.getCentreX(), bounds.getCentreY() + size * 0.5f
    );
    
    g.setColour(PulseColors::TEXT_SECONDARY);
    g.fillPath(arrow);
}

void PulseComponentLibrary::PulseDropdown::mouseEnter(const juce::MouseEvent&)
{
    isHovering = true;
    hoverAmount = 1.0f;
}

void PulseComponentLibrary::PulseDropdown::mouseExit(const juce::MouseEvent&)
{
    isHovering = false;
}

void PulseComponentLibrary::PulseDropdown::mouseDown(const juce::MouseEvent&)
{
    // Cycle through some example items
    static int index = 0;
    const juce::String items[] = {"Channel 1", "Channel 2", "Channel 3", "Channel 4"};
    selectedText = items[++index % 4];
    repaint();
}

//==============================================================================
// PANEL IMPLEMENTATION
PulseComponentLibrary::PulsePanel::PulsePanel(const juce::String& name, Style style)
    : PulseComponent(name), panelStyle(style)
{
}

void PulseComponentLibrary::PulsePanel::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    float cornerRadius = 8.0f * scaleFactor;
    
    switch (panelStyle)
    {
        case Flat:
            g.setColour(PulseColors::BG_MID);
            g.fillRoundedRectangle(bounds, cornerRadius);
            break;
            
        case Raised:
            // Multi-layer shadow
            drawMultiLayerShadow(g, bounds, 3, cornerRadius);
            
            // Gradient background
            {
                juce::ColourGradient raisedGradient(
                    PulseColors::BG_LIGHT, bounds.getCentreX(), bounds.getY(),
                    PulseColors::BG_MID, bounds.getCentreX(), bounds.getBottom(), false);
                g.setGradientFill(raisedGradient);
                g.fillRoundedRectangle(bounds, cornerRadius);
            }
            
            // Top highlight
            g.setColour(PulseColors::BG_HIGHLIGHT.withAlpha(0.3f));
            g.drawRoundedRectangle(bounds.reduced(1), cornerRadius - 1, 1.0f);
            break;
            
        case Recessed:
            // Inner shadow effect
            g.setColour(PulseColors::BG_VOID.withAlpha(0.7f));
            g.drawRoundedRectangle(bounds, cornerRadius, 3.0f);
            
            g.setColour(PulseColors::BG_DARK);
            g.fillRoundedRectangle(bounds.reduced(2), cornerRadius - 2);
            break;
            
        case Glass:
            // Glass effect with multiple gradients
            {
                // Base gradient
                juce::ColourGradient glassGradient(
                    PulseColors::BG_LIGHT.withAlpha(0.15f), bounds.getCentreX(), bounds.getY(),
                    PulseColors::BG_MID.withAlpha(0.05f), bounds.getCentreX(), bounds.getBottom(), false);
                g.setGradientFill(glassGradient);
                g.fillRoundedRectangle(bounds, cornerRadius);
                
                // Glass reflection
                auto reflectionBounds = bounds.removeFromTop(bounds.getHeight() * 0.5f);
                g.setColour(PulseColors::TEXT_PRIMARY.withAlpha(0.05f));
                g.fillRoundedRectangle(reflectionBounds, cornerRadius);
                
                // Border
                g.setColour(PulseColors::TEXT_DIMMED.withAlpha(0.2f));
                g.drawRoundedRectangle(bounds, cornerRadius, 0.5f);
            }
            break;
            
        case TrackControl:
            drawTrackControlBackground(g);
            return;
    }
    
    // Label
    g.setFont(juce::Font("Helvetica Neue", 12.0f * scaleFactor, juce::Font::plain));
    g.setColour(PulseColors::TEXT_SECONDARY.withAlpha(0.5f));
    g.drawText(componentName, bounds.reduced(10), juce::Justification::topLeft);
}

void PulseComponentLibrary::PulsePanel::drawTrackControlBackground(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Complex gradient background (Pulse track control style)
    juce::ColourGradient trackGradient(
        PulseColors::BG_DARK.withAlpha(0.9f), bounds.getX(), bounds.getCentreY(),
        PulseColors::BG_MID.withAlpha(0.7f), bounds.getRight(), bounds.getCentreY(), false);
    
    trackGradient.addColour(0.3, PulseColors::BG_LIGHT.withAlpha(0.8f));
    trackGradient.addColour(0.7, PulseColors::BG_DARK.withAlpha(0.85f));
    
    g.setGradientFill(trackGradient);
    g.fillRoundedRectangle(bounds, 8.0f);
    
    // Subtle pattern overlay
    for (int i = 0; i < bounds.getWidth(); i += 20)
    {
        g.setColour(PulseColors::BG_VOID.withAlpha(0.05f));
        g.drawVerticalLine(bounds.getX() + i, bounds.getY(), bounds.getBottom());
    }
    
    // Edge highlights
    g.setColour(PulseColors::BG_HIGHLIGHT.withAlpha(0.2f));
    g.drawRoundedRectangle(bounds.reduced(1), 7.0f, 0.5f);
    
    // Label
    g.setFont(juce::Font("Helvetica Neue", 10.0f * scaleFactor, juce::Font::plain));
    g.setColour(PulseColors::TEXT_DIMMED);
    g.drawText("TRACK BG", bounds.reduced(10), juce::Justification::topLeft);
}

//==============================================================================
// STAGE CARD IMPLEMENTATION
PulseComponentLibrary::StageCard::StageCard(const juce::String& name, int stageNumber)
    : PulseComponent(name), stage(stageNumber)
{
    // Create 2x2 slider grid
    pitchSlider = std::make_unique<PulseVerticalSlider>("PITCH", 0);
    pulseSlider = std::make_unique<PulseVerticalSlider>("PULSE", 1);
    velocitySlider = std::make_unique<PulseVerticalSlider>("VEL", 2);
    gateSlider = std::make_unique<PulseVerticalSlider>("GATE", 3);
    hamButton = std::make_unique<PulseButton>("HAM", PulseButton::Gradient);
    
    addAndMakeVisible(pitchSlider.get());
    addAndMakeVisible(pulseSlider.get());
    addAndMakeVisible(velocitySlider.get());
    addAndMakeVisible(gateSlider.get());
    addAndMakeVisible(hamButton.get());
}

void PulseComponentLibrary::StageCard::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Card background with gradient
    juce::ColourGradient cardGradient(
        PulseColors::BG_DARK, bounds.getCentreX(), bounds.getY(),
        PulseColors::BG_MID.darker(0.2f), bounds.getCentreX(), bounds.getBottom(), false);
    g.setGradientFill(cardGradient);
    g.fillRoundedRectangle(bounds, 8.0f);
    
    // Card border
    g.setColour(PulseColors::BG_LIGHT.withAlpha(0.5f));
    g.drawRoundedRectangle(bounds, 8.0f, 1.0f);
    
    // Stage number
    g.setFont(juce::Font("Helvetica Neue", 16.0f, juce::Font::bold));
    g.setColour(PulseColors::TEXT_SECONDARY);
    g.drawText("STAGE " + juce::String(stage), bounds.removeFromTop(30), juce::Justification::centred);
}

void PulseComponentLibrary::StageCard::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop(30); // Stage label
    bounds.reduce(10, 10);
    
    // 2x2 grid layout
    int sliderWidth = bounds.getWidth() / 2;
    int sliderHeight = (bounds.getHeight() - 40) / 2; // Leave space for button
    
    pitchSlider->setBounds(0, 0, sliderWidth, sliderHeight);
    pulseSlider->setBounds(sliderWidth, 0, sliderWidth, sliderHeight);
    velocitySlider->setBounds(0, sliderHeight, sliderWidth, sliderHeight);
    gateSlider->setBounds(sliderWidth, sliderHeight, sliderWidth, sliderHeight);
    
    // HAM button at bottom
    hamButton->setBounds(bounds.withTop(bounds.getBottom() - 35).withHeight(35));
}

//==============================================================================
// SCALE SLOT SELECTOR IMPLEMENTATION
PulseComponentLibrary::ScaleSlotSelector::ScaleSlotSelector(const juce::String& name)
    : PulseComponent(name)
{
}

void PulseComponentLibrary::ScaleSlotSelector::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Background
    g.setColour(PulseColors::BG_DARK);
    g.fillRoundedRectangle(bounds, 4.0f);
    
    // Draw 8 slots
    for (int i = 0; i < 8; ++i)
    {
        auto slotBounds = getSlotBounds(i);
        
        bool isSelected = (i == selectedSlot);
        bool isHovered = (i == hoveredSlot);
        
        // Slot background
        if (isSelected)
        {
            g.setColour(PulseColors::TRACK_MINT.withAlpha(0.3f));
            g.fillRoundedRectangle(slotBounds, 3.0f);
            
            g.setColour(PulseColors::TRACK_MINT);
            g.drawRoundedRectangle(slotBounds, 3.0f, 2.0f);
        }
        else if (isHovered)
        {
            g.setColour(PulseColors::BG_LIGHT);
            g.fillRoundedRectangle(slotBounds, 3.0f);
        }
        else
        {
            g.setColour(PulseColors::BG_MID);
            g.fillRoundedRectangle(slotBounds, 3.0f);
        }
        
        // Slot number
        g.setFont(juce::Font("Helvetica Neue", 10.0f, juce::Font::bold));
        g.setColour(isSelected ? PulseColors::TRACK_MINT : PulseColors::TEXT_SECONDARY);
        g.drawText(juce::String(i + 1), slotBounds.removeFromTop(15), juce::Justification::centred);
        
        // Scale name
        g.setFont(juce::Font("Helvetica Neue", 9.0f, juce::Font::plain));
        g.setColour(PulseColors::TEXT_DIMMED);
        g.drawText(slotNames[i], slotBounds, juce::Justification::centred);
    }
    
    // Label
    g.setFont(juce::Font("Helvetica Neue", 10.0f, juce::Font::plain));
    g.setColour(PulseColors::TEXT_SECONDARY);
    g.drawText(componentName, bounds.removeFromBottom(15), juce::Justification::centred);
}

juce::Rectangle<float> PulseComponentLibrary::ScaleSlotSelector::getSlotBounds(int slot) const
{
    auto bounds = getLocalBounds().toFloat();
    bounds.removeFromBottom(15); // Label space
    bounds.reduce(5, 5);
    
    float slotWidth = bounds.getWidth() / 8.0f;
    return bounds.removeFromLeft(slotWidth * (slot + 1)).removeFromRight(slotWidth - 2);
}

void PulseComponentLibrary::ScaleSlotSelector::mouseDown(const juce::MouseEvent& event)
{
    for (int i = 0; i < 8; ++i)
    {
        if (getSlotBounds(i).contains(event.position))
        {
            selectedSlot = i;
            repaint();
            break;
        }
    }
}

//==============================================================================
// GATE PATTERN EDITOR IMPLEMENTATION
PulseComponentLibrary::GatePatternEditor::GatePatternEditor(const juce::String& name)
    : PulseComponent(name)
{
}

void PulseComponentLibrary::GatePatternEditor::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Background
    g.setColour(PulseColors::BG_DARK);
    g.fillRoundedRectangle(bounds, 4.0f);
    
    // Draw 8 gate steps
    for (int i = 0; i < 8; ++i)
    {
        auto stepBounds = getStepBounds(i);
        float gateValue = gateValues[i];
        
        // Step background
        g.setColour(PulseColors::BG_MID);
        g.fillRoundedRectangle(stepBounds, 2.0f);
        
        // Gate value bar
        if (gateValue > 0.01f)
        {
            auto barBounds = stepBounds;
            barBounds.removeFromTop(stepBounds.getHeight() * (1.0f - gateValue));
            
            juce::Colour barColor = gateValue > 0.8f ? PulseColors::TRACK_MINT :
                                   gateValue > 0.5f ? PulseColors::TRACK_CYAN :
                                                     PulseColors::TRACK_AMBER;
            
            g.setColour(barColor.withAlpha(0.8f));
            g.fillRoundedRectangle(barBounds, 2.0f);
            
            // Glow effect
            g.setColour(barColor.withAlpha(0.3f));
            g.drawRoundedRectangle(barBounds.expanded(1), 3.0f, 2.0f);
        }
        
        // Step number
        g.setFont(juce::Font("Helvetica Neue", 9.0f, juce::Font::plain));
        g.setColour(PulseColors::TEXT_DIMMED);
        g.drawText(juce::String(i + 1), stepBounds.removeFromBottom(12), juce::Justification::centred);
    }
    
    // Label
    g.setFont(juce::Font("Helvetica Neue", 10.0f, juce::Font::plain));
    g.setColour(PulseColors::TEXT_SECONDARY);
    g.drawText(componentName, bounds.removeFromBottom(15), juce::Justification::centred);
}

juce::Rectangle<float> PulseComponentLibrary::GatePatternEditor::getStepBounds(int step) const
{
    auto bounds = getLocalBounds().toFloat();
    bounds.removeFromBottom(15); // Label space
    bounds.reduce(5, 5);
    
    float stepWidth = bounds.getWidth() / 8.0f;
    return bounds.removeFromLeft(stepWidth * (step + 1)).removeFromRight(stepWidth - 3).reduced(2, 0);
}

void PulseComponentLibrary::GatePatternEditor::mouseDown(const juce::MouseEvent& event)
{
    for (int i = 0; i < 8; ++i)
    {
        if (getStepBounds(i).contains(event.position))
        {
            draggedStep = i;
            mouseDrag(event);
            break;
        }
    }
}

void PulseComponentLibrary::GatePatternEditor::mouseDrag(const juce::MouseEvent& event)
{
    if (draggedStep >= 0 && draggedStep < 8)
    {
        auto stepBounds = getStepBounds(draggedStep);
        float relativeY = (stepBounds.getBottom() - event.position.y) / stepBounds.getHeight();
        gateValues[draggedStep] = juce::jlimit(0.0f, 1.0f, relativeY);
        repaint();
    }
}

//==============================================================================
// PITCH TRAJECTORY VISUALIZER IMPLEMENTATION
PulseComponentLibrary::PitchTrajectoryVisualizer::PitchTrajectoryVisualizer(const juce::String& name)
    : PulseComponent(name)
{
}

void PulseComponentLibrary::PitchTrajectoryVisualizer::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Background with subtle gradient
    juce::ColourGradient bgGradient(
        PulseColors::BG_DARK, bounds.getCentreX(), bounds.getCentreY(),
        PulseColors::BG_VOID, bounds.getWidth() * 0.7f, bounds.getHeight() * 0.7f, true);
    g.setGradientFill(bgGradient);
    g.fillRoundedRectangle(bounds, 8.0f);
    
    // Draw grid
    drawGrid(g);
    
    // Draw stage markers
    drawStageMarkers(g);
    
    // Draw trajectory
    drawTrajectory(g);
    
    // Border
    g.setColour(PulseColors::BG_LIGHT.withAlpha(0.5f));
    g.drawRoundedRectangle(bounds, 8.0f, 1.0f);
    
    // Label
    g.setFont(juce::Font("Helvetica Neue", 10.0f, juce::Font::plain));
    g.setColour(PulseColors::TEXT_SECONDARY);
    g.drawText(componentName, bounds.removeFromBottom(15), juce::Justification::centred);
}

void PulseComponentLibrary::PitchTrajectoryVisualizer::drawGrid(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(10);
    
    // Horizontal lines (pitch levels)
    g.setColour(PulseColors::BG_LIGHT.withAlpha(0.2f));
    for (int i = 0; i <= 8; ++i)
    {
        float y = bounds.getY() + (i * bounds.getHeight() / 8.0f);
        g.drawHorizontalLine(y, bounds.getX(), bounds.getRight());
    }
    
    // Vertical lines (time/stages)
    for (int i = 0; i <= 8; ++i)
    {
        float x = bounds.getX() + (i * bounds.getWidth() / 8.0f);
        g.drawVerticalLine(x, bounds.getY(), bounds.getBottom());
    }
    
    // Center lines (stronger)
    g.setColour(PulseColors::BG_LIGHT.withAlpha(0.4f));
    g.drawHorizontalLine(bounds.getCentreY(), bounds.getX(), bounds.getRight());
    g.drawVerticalLine(bounds.getCentreX(), bounds.getY(), bounds.getBottom());
}

void PulseComponentLibrary::PitchTrajectoryVisualizer::drawStageMarkers(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(10);
    
    for (int i = 0; i < 8; ++i)
    {
        float x = bounds.getX() + (i * bounds.getWidth() / 7.0f);
        float y = bounds.getCentreY() - (stagePitches[i] * bounds.getHeight() * 0.3f);
        
        // Marker
        g.setColour(i == currentStage ? PulseColors::TRACK_MINT : PulseColors::TRACK_CYAN);
        g.fillEllipse(x - 4, y - 4, 8, 8);
        
        // Stage number
        g.setFont(juce::Font("Helvetica Neue", 8.0f, juce::Font::plain));
        g.setColour(PulseColors::TEXT_DIMMED);
        g.drawText(juce::String(i + 1), juce::Rectangle<float>(x - 10, y + 8, 20, 10), 
                  juce::Justification::centred);
    }
}

void PulseComponentLibrary::PitchTrajectoryVisualizer::drawTrajectory(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(10);
    
    juce::Path trajectory;
    bool firstPoint = true;
    
    for (int i = 0; i < 8; ++i)
    {
        float x = bounds.getX() + (i * bounds.getWidth() / 7.0f);
        float y = bounds.getCentreY() - (stagePitches[i] * bounds.getHeight() * 0.3f);
        
        if (firstPoint)
        {
            trajectory.startNewSubPath(x, y);
            firstPoint = false;
        }
        else
        {
            trajectory.lineTo(x, y);
        }
    }
    
    // Draw trajectory with glow
    g.setColour(PulseColors::TRACK_MINT.withAlpha(0.3f));
    g.strokePath(trajectory, juce::PathStrokeType(4.0f));
    
    g.setColour(PulseColors::TRACK_MINT);
    g.strokePath(trajectory, juce::PathStrokeType(2.0f));
}

void PulseComponentLibrary::PitchTrajectoryVisualizer::addPitchPoint(float pitch, int64_t timestamp)
{
    PitchPoint point;
    point.pitch = pitch;
    point.timestamp = timestamp;
    
    pitchHistory.push_back(point);
    
    if (pitchHistory.size() > MAX_HISTORY)
        pitchHistory.pop_front();
    
    repaint();
}

void PulseComponentLibrary::PitchTrajectoryVisualizer::SpringAnimation::update(float deltaTime)
{
    float force = (target - position) * 200.0f; // Stiffness
    velocity += force * deltaTime;
    velocity *= std::pow(1.0f - 15.0f * deltaTime, 2); // Damping
    position += velocity * deltaTime;
}

//==============================================================================
// TRACK CONTROL PANEL IMPLEMENTATION
PulseComponentLibrary::TrackControlPanel::TrackControlPanel(const juce::String& name, int trackNumber)
    : PulseComponent(name), track(trackNumber)
{
    // Select track color
    const juce::Colour trackColors[] = {
        PulseColors::TRACK_MINT, PulseColors::TRACK_CYAN,
        PulseColors::TRACK_PINK, PulseColors::TRACK_AMBER
    };
    trackColor = trackColors[(trackNumber - 1) % 4];
    
    // Create controls
    volumeSlider = std::make_unique<PulseVerticalSlider>("VOL", trackNumber - 1);
    channelSelector = std::make_unique<PulseDropdown>("CH");
    muteToggle = std::make_unique<PulseToggle>("M");
    soloToggle = std::make_unique<PulseToggle>("S");
    
    addAndMakeVisible(volumeSlider.get());
    addAndMakeVisible(channelSelector.get());
    addAndMakeVisible(muteToggle.get());
    addAndMakeVisible(soloToggle.get());
}

void PulseComponentLibrary::TrackControlPanel::paint(juce::Graphics& g)
{
    drawGradientBackground(g);
    
    // Track number
    g.setFont(juce::Font("Helvetica Neue", 14.0f, juce::Font::bold));
    g.setColour(trackColor);
    g.drawText("TRACK " + juce::String(track), getLocalBounds().removeFromTop(25), 
              juce::Justification::centred);
}

void PulseComponentLibrary::TrackControlPanel::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop(25); // Track label
    bounds.reduce(10, 10);
    
    // Layout controls
    volumeSlider->setBounds(bounds.removeFromLeft(30));
    bounds.removeFromLeft(10);
    
    auto controlArea = bounds.removeFromTop(80);
    channelSelector->setBounds(controlArea.removeFromTop(35));
    controlArea.removeFromTop(5);
    
    auto toggleArea = controlArea.removeFromTop(35);
    muteToggle->setBounds(toggleArea.removeFromLeft(toggleArea.getWidth() / 2));
    soloToggle->setBounds(toggleArea);
}

void PulseComponentLibrary::TrackControlPanel::drawGradientBackground(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Multi-gradient background (Pulse track style)
    juce::ColourGradient gradient(
        trackColor.withAlpha(0.1f), bounds.getX(), bounds.getCentreY(),
        PulseColors::BG_DARK.withAlpha(0.9f), bounds.getRight(), bounds.getCentreY(), false);
    
    gradient.addColour(0.2, PulseColors::BG_MID.withAlpha(0.8f));
    gradient.addColour(0.8, trackColor.withAlpha(0.05f));
    
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(bounds, 8.0f);
    
    // Border with track color
    g.setColour(trackColor.withAlpha(0.3f));
    g.drawRoundedRectangle(bounds, 8.0f, 1.0f);
}

} // namespace HAM