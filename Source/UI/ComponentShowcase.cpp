#include "ComponentShowcase.h"

namespace HAM {

// Color definitions
const juce::Colour ComponentShowcase::Colors::BG_DARKEST(0xFF000000);
const juce::Colour ComponentShowcase::Colors::BG_DARK(0xFF0A0A0A);
const juce::Colour ComponentShowcase::Colors::BG_MID(0xFF1A1A1A);
const juce::Colour ComponentShowcase::Colors::BG_LIGHT(0xFF2A2A2A);
const juce::Colour ComponentShowcase::Colors::BG_LIGHTER(0xFF3A3A3A);

const juce::Colour ComponentShowcase::Colors::TEXT_PRIMARY(0xFFFFFFFF);
const juce::Colour ComponentShowcase::Colors::TEXT_SECONDARY(0xFFCCCCCC);
const juce::Colour ComponentShowcase::Colors::TEXT_DIMMED(0xFF888888);

const juce::Colour ComponentShowcase::Colors::TRACK_MINT(0xFF00FF88);
const juce::Colour ComponentShowcase::Colors::TRACK_CYAN(0xFF00D9FF);
const juce::Colour ComponentShowcase::Colors::TRACK_MAGENTA(0xFFFF00FF);
const juce::Colour ComponentShowcase::Colors::TRACK_ORANGE(0xFFFFAA00);

//==============================================================================
ComponentShowcase::ComponentShowcase()
{
    setSize(1200, 800);
    
    // Position label to show current grid coordinates
    positionLabel.setText("Grid: --", juce::dontSendNotification);
    positionLabel.setColour(juce::Label::textColourId, Colors::TEXT_SECONDARY);
    positionLabel.setJustificationType(juce::Justification::topRight);
    addAndMakeVisible(positionLabel);
    
    createShowcaseComponents();
    layoutComponents();
    
    // Start timer for smooth animations (30 FPS to avoid repaints)
    startTimerHz(30);
}

ComponentShowcase::~ComponentShowcase()
{
    stopTimer();
}

//==============================================================================
juce::Component* ComponentShowcase::getComponentByName(const juce::String& name)
{
    auto it = components.find(name);
    return it != components.end() ? it->second.get() : nullptr;
}

juce::Rectangle<int> ComponentShowcase::getGridCell(char row, int col, int rowSpan, int colSpan) const
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

juce::String ComponentShowcase::getGridPosition(const juce::Point<int>& point) const
{
    int col = (point.x / grid.cellWidth) + 1;
    int row = (point.y / grid.cellHeight);
    
    if (col < 1 || col > 24 || row < 0 || row >= 24)
        return "--";
    
    char rowChar = 'A' + row;
    return juce::String::charToString(rowChar) + juce::String(col);
}

//==============================================================================
void ComponentShowcase::paint(juce::Graphics& g)
{
    // Background
    g.fillAll(Colors::BG_DARKEST);
    
    // Draw grid if enabled
    if (grid.showGrid)
    {
        g.setColour(Colors::BG_LIGHT.withAlpha(0.2f));
        
        // Vertical lines
        for (int i = 0; i <= 24; ++i)
        {
            int x = i * grid.cellWidth;
            g.drawLine(x, 0, x, getHeight(), 0.5f);
        }
        
        // Horizontal lines
        for (int i = 0; i <= 24; ++i)
        {
            int y = i * grid.cellHeight;
            g.drawLine(0, y, getWidth(), y, 0.5f);
        }
    }
    
    // Draw grid labels if enabled
    if (grid.showLabels)
    {
        g.setFont(10.0f);
        g.setColour(Colors::TEXT_DIMMED);
        
        // Column numbers (1-24)
        for (int i = 0; i < 24; ++i)
        {
            auto bounds = juce::Rectangle<int>(
                i * grid.cellWidth, 0,
                grid.cellWidth, 15
            );
            g.drawText(juce::String(i + 1), bounds, juce::Justification::centred);
        }
        
        // Row letters (A-X)
        for (int i = 0; i < 24; ++i)
        {
            char letter = 'A' + i;
            auto bounds = juce::Rectangle<int>(
                0, i * grid.cellHeight,
                15, grid.cellHeight
            );
            g.drawText(juce::String::charToString(letter), bounds, juce::Justification::centred);
        }
    }
    
    // Highlight hovered cell
    if (grid.hoveredCell.x >= 0 && grid.hoveredCell.y >= 0)
    {
        auto cellBounds = juce::Rectangle<int>(
            grid.hoveredCell.x * grid.cellWidth,
            grid.hoveredCell.y * grid.cellHeight,
            grid.cellWidth,
            grid.cellHeight
        );
        
        g.setColour(Colors::TRACK_CYAN.withAlpha(0.1f));
        g.fillRect(cellBounds);
    }
}

void ComponentShowcase::resized()
{
    // Update grid cell size based on window size
    grid.cellWidth = getWidth() / 24;
    grid.cellHeight = (getHeight() - 30) / 24; // Leave space for position label
    
    // Position label at top right
    positionLabel.setBounds(getWidth() - 100, 5, 90, 20);
    
    // Re-layout all components
    layoutComponents();
}

void ComponentShowcase::mouseMove(const juce::MouseEvent& event)
{
    auto pos = event.getPosition();
    grid.hoveredCell.x = pos.x / grid.cellWidth;
    grid.hoveredCell.y = pos.y / grid.cellHeight;
    
    positionLabel.setText("Grid: " + getGridPosition(pos), juce::dontSendNotification);
    repaint();
}

//==============================================================================
void ComponentShowcase::timerCallback()
{
    // Update animations for components that need them
    for (auto& [name, comp] : components)
    {
        if (comp->isVisible())
            comp->repaint();
    }
}

void ComponentShowcase::createShowcaseComponents()
{
    // VERTICAL SLIDERS
    components["VSLIDER_SMALL_1"] = std::make_unique<VerticalSlider>("VSLIDER_SMALL_1", VerticalSlider::Style::Small);
    components["VSLIDER_SMALL_2"] = std::make_unique<VerticalSlider>("VSLIDER_SMALL_2", VerticalSlider::Style::Small);
    components["VSLIDER_LARGE_1"] = std::make_unique<VerticalSlider>("VSLIDER_LARGE_1", VerticalSlider::Style::Large);
    components["VSLIDER_LARGE_2"] = std::make_unique<VerticalSlider>("VSLIDER_LARGE_2", VerticalSlider::Style::Large);
    
    // HORIZONTAL SLIDERS
    components["HSLIDER_SMALL_1"] = std::make_unique<HorizontalSlider>("HSLIDER_SMALL_1", HorizontalSlider::Style::Small);
    components["HSLIDER_SMALL_2"] = std::make_unique<HorizontalSlider>("HSLIDER_SMALL_2", HorizontalSlider::Style::Small);
    components["HSLIDER_LARGE_1"] = std::make_unique<HorizontalSlider>("HSLIDER_LARGE_1", HorizontalSlider::Style::Large);
    components["HSLIDER_LARGE_2"] = std::make_unique<HorizontalSlider>("HSLIDER_LARGE_2", HorizontalSlider::Style::Large);
    
    // BUTTONS
    components["BUTTON_SMALL_SOLID"] = std::make_unique<ModernButton>("BUTTON_SMALL_SOLID", ModernButton::Style::Small, ModernButton::Type::Solid);
    components["BUTTON_SMALL_OUTLINE"] = std::make_unique<ModernButton>("BUTTON_SMALL_OUTLINE", ModernButton::Style::Small, ModernButton::Type::Outline);
    components["BUTTON_LARGE_SOLID"] = std::make_unique<ModernButton>("BUTTON_LARGE_SOLID", ModernButton::Style::Large, ModernButton::Type::Solid);
    components["BUTTON_LARGE_GHOST"] = std::make_unique<ModernButton>("BUTTON_LARGE_GHOST", ModernButton::Style::Large, ModernButton::Type::Ghost);
    
    // TOGGLES
    components["TOGGLE_SMALL_1"] = std::make_unique<ToggleSwitch>("TOGGLE_SMALL_1", ToggleSwitch::Style::Small);
    components["TOGGLE_SMALL_2"] = std::make_unique<ToggleSwitch>("TOGGLE_SMALL_2", ToggleSwitch::Style::Small);
    components["TOGGLE_LARGE_1"] = std::make_unique<ToggleSwitch>("TOGGLE_LARGE_1", ToggleSwitch::Style::Large);
    components["TOGGLE_LARGE_2"] = std::make_unique<ToggleSwitch>("TOGGLE_LARGE_2", ToggleSwitch::Style::Large);
    
    // DROPDOWNS
    components["DROPDOWN_SMALL_1"] = std::make_unique<Dropdown>("DROPDOWN_SMALL_1", Dropdown::Style::Small);
    components["DROPDOWN_SMALL_2"] = std::make_unique<Dropdown>("DROPDOWN_SMALL_2", Dropdown::Style::Small);
    components["DROPDOWN_LARGE_1"] = std::make_unique<Dropdown>("DROPDOWN_LARGE_1", Dropdown::Style::Large);
    components["DROPDOWN_LARGE_2"] = std::make_unique<Dropdown>("DROPDOWN_LARGE_2", Dropdown::Style::Large);
    
    // PANELS/BACKGROUNDS
    components["PANEL_FLAT"] = std::make_unique<Panel>("PANEL_FLAT", Panel::Style::Flat);
    components["PANEL_RAISED"] = std::make_unique<Panel>("PANEL_RAISED", Panel::Style::Raised);
    components["PANEL_RECESSED"] = std::make_unique<Panel>("PANEL_RECESSED", Panel::Style::Recessed);
    components["PANEL_GLASS"] = std::make_unique<Panel>("PANEL_GLASS", Panel::Style::Glass);
    
    // Add all components to view
    for (auto& [name, comp] : components)
    {
        addAndMakeVisible(comp.get());
    }
}

void ComponentShowcase::layoutComponents()
{
    // Layout components on the grid
    // You can now reference these by name and position them using the grid
    
    // Example layout - Vertical Sliders
    if (auto* comp = getComponentByName("VSLIDER_SMALL_1"))
        comp->setBounds(getGridCell('B', 2, 4, 1));
    
    if (auto* comp = getComponentByName("VSLIDER_SMALL_2"))
        comp->setBounds(getGridCell('B', 4, 4, 1));
    
    if (auto* comp = getComponentByName("VSLIDER_LARGE_1"))
        comp->setBounds(getGridCell('B', 6, 6, 2));
    
    if (auto* comp = getComponentByName("VSLIDER_LARGE_2"))
        comp->setBounds(getGridCell('B', 9, 6, 2));
    
    // Horizontal Sliders
    if (auto* comp = getComponentByName("HSLIDER_SMALL_1"))
        comp->setBounds(getGridCell('I', 2, 1, 4));
    
    if (auto* comp = getComponentByName("HSLIDER_SMALL_2"))
        comp->setBounds(getGridCell('J', 2, 1, 4));
    
    if (auto* comp = getComponentByName("HSLIDER_LARGE_1"))
        comp->setBounds(getGridCell('L', 2, 2, 6));
    
    if (auto* comp = getComponentByName("HSLIDER_LARGE_2"))
        comp->setBounds(getGridCell('N', 2, 2, 6));
    
    // Buttons
    if (auto* comp = getComponentByName("BUTTON_SMALL_SOLID"))
        comp->setBounds(getGridCell('B', 12, 1, 3));
    
    if (auto* comp = getComponentByName("BUTTON_SMALL_OUTLINE"))
        comp->setBounds(getGridCell('B', 16, 1, 3));
    
    if (auto* comp = getComponentByName("BUTTON_LARGE_SOLID"))
        comp->setBounds(getGridCell('D', 12, 2, 4));
    
    if (auto* comp = getComponentByName("BUTTON_LARGE_GHOST"))
        comp->setBounds(getGridCell('D', 17, 2, 4));
    
    // Toggles
    if (auto* comp = getComponentByName("TOGGLE_SMALL_1"))
        comp->setBounds(getGridCell('G', 12, 1, 2));
    
    if (auto* comp = getComponentByName("TOGGLE_SMALL_2"))
        comp->setBounds(getGridCell('G', 15, 1, 2));
    
    if (auto* comp = getComponentByName("TOGGLE_LARGE_1"))
        comp->setBounds(getGridCell('G', 18, 1, 3));
    
    // Dropdowns
    if (auto* comp = getComponentByName("DROPDOWN_SMALL_1"))
        comp->setBounds(getGridCell('I', 12, 1, 4));
    
    if (auto* comp = getComponentByName("DROPDOWN_LARGE_1"))
        comp->setBounds(getGridCell('K', 12, 2, 6));
    
    // Panels
    if (auto* comp = getComponentByName("PANEL_FLAT"))
        comp->setBounds(getGridCell('P', 2, 4, 8));
    
    if (auto* comp = getComponentByName("PANEL_RAISED"))
        comp->setBounds(getGridCell('P', 11, 4, 8));
}

//==============================================================================
// VERTICAL SLIDER IMPLEMENTATION
ComponentShowcase::VerticalSlider::VerticalSlider(const juce::String& name, Style s)
    : ResizableComponent(name), style(s)
{
    // Set track color based on style
    trackColor = (style == Style::Small) ? Colors::TRACK_MINT : Colors::TRACK_CYAN;
}

void ComponentShowcase::VerticalSlider::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    float trackWidth = (style == Style::Small) ? 20.0f : 30.0f;
    trackWidth *= scaleFactor;
    
    auto trackBounds = bounds.withSizeKeepingCentre(trackWidth, bounds.getHeight() - 10);
    
    // Track shadow
    g.setColour(juce::Colour(0xff000000).withAlpha(0.3f));
    g.fillRoundedRectangle(trackBounds.translated(0, 1), trackWidth * 0.3f);
    
    // Track background gradient
    juce::ColourGradient trackGradient(
        Colors::BG_LIGHT, trackBounds.getCentreX(), trackBounds.getY(),
        Colors::BG_MID, trackBounds.getCentreX(), trackBounds.getBottom(), false);
    g.setGradientFill(trackGradient);
    g.fillRoundedRectangle(trackBounds, trackWidth * 0.3f);
    
    // Value fill
    float valueY = trackBounds.getY() + (1.0f - value) * trackBounds.getHeight();
    auto fillBounds = trackBounds.withTop(valueY);
    
    // Glow effect
    if (glowIntensity > 0.01f)
    {
        g.setColour(trackColor.withAlpha(glowIntensity * 0.3f));
        g.fillRoundedRectangle(fillBounds.expanded(2.0f), trackWidth * 0.3f + 2.0f);
    }
    
    // Main fill
    g.setColour(trackColor.withAlpha(0.9f));
    g.fillRoundedRectangle(fillBounds, trackWidth * 0.3f);
    
    // Line indicator at current value
    g.setColour(Colors::TEXT_PRIMARY);
    g.drawLine(trackBounds.getX(), valueY, trackBounds.getRight(), valueY, 2.0f);
    
    // Label
    g.setFont(10.0f * scaleFactor);
    g.setColour(Colors::TEXT_SECONDARY);
    g.drawText(name, bounds.removeFromBottom(15), juce::Justification::centred);
}

void ComponentShowcase::VerticalSlider::mouseDown(const juce::MouseEvent& event)
{
    mouseDrag(event);
    glowIntensity = 0.8f;
}

void ComponentShowcase::VerticalSlider::mouseDrag(const juce::MouseEvent& event)
{
    float newValue = 1.0f - (event.position.y / getHeight());
    value = targetValue = juce::jlimit(0.0f, 1.0f, newValue);
    repaint();
}

//==============================================================================
// HORIZONTAL SLIDER IMPLEMENTATION
ComponentShowcase::HorizontalSlider::HorizontalSlider(const juce::String& name, Style s)
    : ResizableComponent(name), style(s)
{
    trackColor = (style == Style::Small) ? Colors::TRACK_MAGENTA : Colors::TRACK_ORANGE;
}

void ComponentShowcase::HorizontalSlider::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    float trackHeight = (style == Style::Small) ? 20.0f : 30.0f;
    trackHeight *= scaleFactor;
    
    auto trackBounds = bounds.withSizeKeepingCentre(bounds.getWidth() - 10, trackHeight);
    
    // Track background
    g.setColour(Colors::BG_LIGHT);
    g.fillRoundedRectangle(trackBounds, trackHeight * 0.3f);
    
    // Value fill
    auto fillBounds = trackBounds.withWidth(value * trackBounds.getWidth());
    g.setColour(trackColor);
    g.fillRoundedRectangle(fillBounds, trackHeight * 0.3f);
    
    // Thumb
    float thumbX = trackBounds.getX() + value * trackBounds.getWidth();
    g.setColour(Colors::TEXT_PRIMARY);
    g.fillEllipse(thumbX - 6, trackBounds.getCentreY() - 6, 12, 12);
    
    // Label
    g.setFont(10.0f * scaleFactor);
    g.setColour(Colors::TEXT_SECONDARY);
    g.drawText(name, bounds, juce::Justification::centredBottom);
}

void ComponentShowcase::HorizontalSlider::mouseDown(const juce::MouseEvent& event)
{
    mouseDrag(event);
}

void ComponentShowcase::HorizontalSlider::mouseDrag(const juce::MouseEvent& event)
{
    float newValue = event.position.x / getWidth();
    value = juce::jlimit(0.0f, 1.0f, newValue);
    repaint();
}

//==============================================================================
// BUTTON IMPLEMENTATION
ComponentShowcase::ModernButton::ModernButton(const juce::String& name, Style s, Type t)
    : ResizableComponent(name), style(s), type(t)
{
}

void ComponentShowcase::ModernButton::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(2);
    float cornerRadius = (style == Style::Small) ? 4.0f : 6.0f;
    cornerRadius *= scaleFactor;
    
    // Background based on type
    if (type == Type::Solid)
    {
        g.setColour(Colors::TRACK_MINT.withAlpha(0.9f + hoverAmount * 0.1f));
        g.fillRoundedRectangle(bounds, cornerRadius);
    }
    else if (type == Type::Outline)
    {
        g.setColour(Colors::TRACK_CYAN.withAlpha(0.5f + hoverAmount * 0.5f));
        g.drawRoundedRectangle(bounds, cornerRadius, 2.0f);
    }
    else // Ghost
    {
        g.setColour(Colors::BG_LIGHTER.withAlpha(hoverAmount));
        g.fillRoundedRectangle(bounds, cornerRadius);
    }
    
    // Click animation
    if (clickAnimation > 0.01f)
    {
        g.setColour(Colors::TEXT_PRIMARY.withAlpha(clickAnimation * 0.3f));
        g.fillRoundedRectangle(bounds.expanded(clickAnimation * 4), cornerRadius);
    }
    
    // Text
    g.setFont((style == Style::Small ? 12.0f : 16.0f) * scaleFactor);
    g.setColour(type == Type::Solid ? Colors::BG_DARKEST : Colors::TEXT_PRIMARY);
    g.drawText(name, bounds, juce::Justification::centred);
}

void ComponentShowcase::ModernButton::mouseEnter(const juce::MouseEvent&)
{
    isHovering = true;
    hoverAmount = 0.8f;
    repaint();
}

void ComponentShowcase::ModernButton::mouseExit(const juce::MouseEvent&)
{
    isHovering = false;
    hoverAmount = 0.0f;
    repaint();
}

void ComponentShowcase::ModernButton::mouseDown(const juce::MouseEvent&)
{
    isPressed = true;
    clickAnimation = 1.0f;
    repaint();
}

void ComponentShowcase::ModernButton::mouseUp(const juce::MouseEvent&)
{
    isPressed = false;
    clickAnimation = 0.0f;
    repaint();
}

//==============================================================================
// TOGGLE IMPLEMENTATION
ComponentShowcase::ToggleSwitch::ToggleSwitch(const juce::String& name, Style s)
    : ResizableComponent(name), style(s)
{
}

void ComponentShowcase::ToggleSwitch::paint(juce::Graphics& g)
{
    float switchWidth = (style == Style::Small) ? 40.0f : 60.0f;
    float switchHeight = (style == Style::Small) ? 20.0f : 30.0f;
    float thumbSize = switchHeight - 4;
    
    switchWidth *= scaleFactor;
    switchHeight *= scaleFactor;
    thumbSize *= scaleFactor;
    
    auto switchBounds = getLocalBounds().toFloat()
        .withSizeKeepingCentre(switchWidth, switchHeight);
    
    // Background track
    g.setColour(isOn ? Colors::TRACK_MINT.withAlpha(0.3f) : Colors::BG_LIGHT);
    g.fillRoundedRectangle(switchBounds, switchHeight * 0.5f);
    
    // Thumb
    float thumbX = switchBounds.getX() + 2 + (isOn ? switchWidth - thumbSize - 4 : 0);
    float thumbY = switchBounds.getCentreY() - thumbSize * 0.5f;
    
    g.setColour(isOn ? Colors::TRACK_MINT : Colors::TEXT_SECONDARY);
    g.fillEllipse(thumbX, thumbY, thumbSize, thumbSize);
    
    // Label
    g.setFont(10.0f * scaleFactor);
    g.setColour(Colors::TEXT_SECONDARY);
    g.drawText(name, getLocalBounds().removeFromBottom(15), juce::Justification::centred);
}

void ComponentShowcase::ToggleSwitch::mouseDown(const juce::MouseEvent&)
{
    isOn = !isOn;
    repaint();
}

//==============================================================================
// DROPDOWN IMPLEMENTATION
ComponentShowcase::Dropdown::Dropdown(const juce::String& name, Style s)
    : ResizableComponent(name), style(s)
{
}

void ComponentShowcase::Dropdown::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(2);
    float cornerRadius = 4.0f * scaleFactor;
    
    // Multi-layer shadow
    for (int i = 3; i > 0; --i)
    {
        float offset = i * 1.0f;
        g.setColour(juce::Colour(0xff000000).withAlpha(0.1f));
        g.fillRoundedRectangle(bounds.translated(0, offset).expanded(i), cornerRadius);
    }
    
    // Background gradient
    juce::ColourGradient bgGradient(
        Colors::BG_LIGHT.withAlpha(0.9f), bounds.getCentreX(), bounds.getY(),
        Colors::BG_MID.withAlpha(0.7f), bounds.getCentreX(), bounds.getBottom(), false);
    g.setGradientFill(bgGradient);
    g.fillRoundedRectangle(bounds, cornerRadius);
    
    // Hover glow
    if (hoverAmount > 0.01f)
    {
        g.setColour(Colors::TRACK_CYAN.withAlpha(hoverAmount * 0.3f));
        g.drawRoundedRectangle(bounds, cornerRadius, 2.0f);
    }
    
    // Text
    auto textBounds = bounds.reduced(8, 0);
    g.setFont((style == Style::Small ? 12.0f : 14.0f) * scaleFactor);
    g.setColour(Colors::TEXT_PRIMARY);
    g.drawText(selectedText, textBounds, juce::Justification::centredLeft);
    
    // Arrow
    drawArrow(g, bounds.removeFromRight(20));
    
    // Label
    g.setFont(10.0f * scaleFactor);
    g.setColour(Colors::TEXT_SECONDARY);
    g.drawText(name, getLocalBounds().removeFromBottom(15), juce::Justification::centred);
}

void ComponentShowcase::Dropdown::drawArrow(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    juce::Path arrow;
    float size = 6.0f * scaleFactor;
    arrow.addTriangle(
        bounds.getCentreX() - size, bounds.getCentreY() - size * 0.5f,
        bounds.getCentreX() + size, bounds.getCentreY() - size * 0.5f,
        bounds.getCentreX(), bounds.getCentreY() + size * 0.5f
    );
    
    g.setColour(Colors::TEXT_SECONDARY);
    g.fillPath(arrow);
}

void ComponentShowcase::Dropdown::mouseEnter(const juce::MouseEvent&)
{
    isHovering = true;
    hoverAmount = 0.8f;
    repaint();
}

void ComponentShowcase::Dropdown::mouseExit(const juce::MouseEvent&)
{
    isHovering = false;
    hoverAmount = 0.0f;
    repaint();
}

void ComponentShowcase::Dropdown::mouseDown(const juce::MouseEvent&)
{
    // Would show popup menu here
    selectedText = "Item " + juce::String(juce::Random::getSystemRandom().nextInt(5) + 1);
    repaint();
}

//==============================================================================
// PANEL IMPLEMENTATION
ComponentShowcase::Panel::Panel(const juce::String& name, Style s)
    : ResizableComponent(name), style(s)
{
}

void ComponentShowcase::Panel::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    float cornerRadius = 8.0f * scaleFactor;
    
    switch (style)
    {
        case Style::Flat:
            g.setColour(Colors::BG_MID);
            g.fillRoundedRectangle(bounds, cornerRadius);
            break;
            
        case Style::Raised:
            // Shadow
            g.setColour(juce::Colour(0xff000000).withAlpha(0.3f));
            g.fillRoundedRectangle(bounds.translated(0, 2), cornerRadius);
            // Main
            g.setColour(Colors::BG_LIGHT);
            g.fillRoundedRectangle(bounds, cornerRadius);
            // Highlight
            g.setColour(Colors::BG_LIGHTER.withAlpha(0.3f));
            g.drawRoundedRectangle(bounds.reduced(1), cornerRadius - 1, 1.0f);
            break;
            
        case Style::Recessed:
            // Inner shadow
            g.setColour(juce::Colour(0xff000000).withAlpha(0.5f));
            g.drawRoundedRectangle(bounds, cornerRadius, 2.0f);
            // Main
            g.setColour(Colors::BG_DARK);
            g.fillRoundedRectangle(bounds.reduced(2), cornerRadius - 2);
            break;
            
        case Style::Glass:
            // Glass effect with gradient
            juce::ColourGradient glassGradient(
                Colors::BG_LIGHTER.withAlpha(0.2f), bounds.getCentreX(), bounds.getY(),
                Colors::BG_MID.withAlpha(0.1f), bounds.getCentreX(), bounds.getBottom(), false);
            g.setGradientFill(glassGradient);
            g.fillRoundedRectangle(bounds, cornerRadius);
            // Border
            g.setColour(Colors::TEXT_DIMMED.withAlpha(0.3f));
            g.drawRoundedRectangle(bounds, cornerRadius, 0.5f);
            break;
    }
    
    // Label
    g.setFont(12.0f * scaleFactor);
    g.setColour(Colors::TEXT_SECONDARY);
    g.drawText(name, bounds.reduced(10), juce::Justification::topLeft);
}

} // namespace HAM