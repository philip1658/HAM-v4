#include "LayoutComponents.h"

namespace HAM {

//==============================================================================
// PulsePanel Implementation
//==============================================================================
PulsePanel::PulsePanel(const juce::String& name, Style style)
    : PulseComponent(name), panelStyle(style)
{
}

void PulsePanel::paint(juce::Graphics& g)
{
    switch (panelStyle)
    {
        case Flat:
            drawFlatStyle(g);
            break;
        case Raised:
            drawRaisedStyle(g);
            break;
        case Recessed:
            drawRecessedStyle(g);
            break;
        case Glass:
            drawGlassStyle(g);
            break;
        case TrackControl:
            drawTrackControlStyle(g);
            break;
    }
    
    // Draw title if present
    if (!panelTitle.isEmpty())
    {
        auto bounds = getLocalBounds().toFloat();
        auto titleBounds = bounds.removeFromTop(25);
        
        g.setFont(juce::Font(juce::FontOptions(12.0f * scaleFactor).withName("Helvetica Neue")));
        g.setColour(PulseColors::TEXT_PRIMARY);
        g.drawText(panelTitle, titleBounds, juce::Justification::centred);
    }
}

void PulsePanel::resized()
{
    // Nothing specific needed here
}

void PulsePanel::drawFlatStyle(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    g.setColour(PulseColors::BG_DARK);
    g.fillRoundedRectangle(bounds, 3.0f);
    
    if (showBorder)
    {
        g.setColour(PulseColors::BG_LIGHT.withAlpha(0.3f));
        g.drawRoundedRectangle(bounds, 3.0f, 1.0f);
    }
}

void PulsePanel::drawRaisedStyle(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Multi-layer shadow for raised effect
    drawMultiLayerShadow(g, bounds, 3, 3.0f);
    
    // Background gradient
    fillWithGradient(g, bounds,
                    PulseColors::BG_RAISED,
                    PulseColors::BG_MID);
    
    // Highlight edge
    g.setColour(PulseColors::BG_HIGHLIGHT.withAlpha(0.3f));
    g.drawRoundedRectangle(bounds.reduced(1), 3.0f, 0.5f);
}

void PulsePanel::drawRecessedStyle(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Inner shadow for recessed effect
    g.setColour(PulseColors::BG_VOID.withAlpha(0.5f));
    g.drawRoundedRectangle(bounds.expanded(1), 3.0f, 2.0f);
    
    // Background darker than surroundings
    g.setColour(PulseColors::BG_DARKEST);
    g.fillRoundedRectangle(bounds, 3.0f);
    
    // Dark inner edge
    g.setColour(PulseColors::BG_VOID.withAlpha(0.3f));
    g.drawRoundedRectangle(bounds.reduced(1), 3.0f, 1.0f);
}

void PulsePanel::drawGlassStyle(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Glass background with transparency
    g.setColour(PulseColors::BG_DARK.withAlpha(0.8f));
    g.fillRoundedRectangle(bounds, 3.0f);
    
    // Glass reflection gradient
    auto reflectionBounds = bounds.removeFromTop(bounds.getHeight() * 0.5f);
    juce::ColourGradient glassGradient(
        juce::Colours::white.withAlpha(0.1f), reflectionBounds.getCentreX(), reflectionBounds.getY(),
        juce::Colours::transparentWhite, reflectionBounds.getCentreX(), reflectionBounds.getBottom(), false);
    g.setGradientFill(glassGradient);
    g.fillRoundedRectangle(reflectionBounds, 3.0f);
    
    // Glass edge
    g.setColour(PulseColors::BG_LIGHT.withAlpha(0.5f));
    g.drawRoundedRectangle(bounds, 3.0f, 1.0f);
}

void PulsePanel::drawTrackControlStyle(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Gradient background for track control
    fillWithGradient(g, bounds,
                    PulseColors::BG_MID.withAlpha(0.9f),
                    PulseColors::BG_DARK);
    
    // Side accent strip (like Pulse)
    auto accentStrip = bounds.removeFromLeft(4);
    g.setColour(PulseColors::TRACK_CYAN);
    g.fillRect(accentStrip);
    
    // Border
    g.setColour(PulseColors::BG_LIGHT.withAlpha(0.4f));
    g.drawRoundedRectangle(bounds, 3.0f, 1.0f);
}

//==============================================================================
// TrackControlPanel Implementation
//==============================================================================
TrackControlPanel::TrackControlPanel(const juce::String& name, int trackNumber)
    : PulseComponent(name), trackNum(trackNumber)
{
    trackName = "Track " + juce::String(trackNumber);
    
    // Create control buttons
    muteButton = std::make_unique<juce::TextButton>("M");
    soloButton = std::make_unique<juce::TextButton>("S");
    armButton = std::make_unique<juce::TextButton>("R");
    
    muteButton->onClick = [this]() 
    { 
        isMuted = !isMuted;
        if (onMuteChanged) onMuteChanged(isMuted);
        repaint();
    };
    
    soloButton->onClick = [this]() 
    { 
        isSoloed = !isSoloed;
        if (onSoloChanged) onSoloChanged(isSoloed);
        repaint();
    };
    
    armButton->onClick = [this]() 
    { 
        isArmed = !isArmed;
        if (onArmChanged) onArmChanged(isArmed);
        repaint();
    };
    
    addAndMakeVisible(muteButton.get());
    addAndMakeVisible(soloButton.get());
    addAndMakeVisible(armButton.get());
}

void TrackControlPanel::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Background with gradient
    juce::Colour trackColor = getTrackColor();
    
    if (isSelected)
    {
        fillWithGradient(g, bounds,
                        trackColor.withAlpha(0.3f),
                        trackColor.withAlpha(0.1f));
    }
    else
    {
        fillWithGradient(g, bounds,
                        PulseColors::BG_MID,
                        PulseColors::BG_DARK);
    }
    
    // Track color strip on left
    auto colorStrip = bounds.removeFromLeft(5);
    g.setColour(trackColor);
    g.fillRect(colorStrip);
    
    // Border
    g.setColour(isSelected ? trackColor.withAlpha(0.8f) : PulseColors::BG_LIGHT.withAlpha(0.3f));
    g.drawRoundedRectangle(bounds, 3.0f, 1.0f);
    
    // Track name
    auto nameBounds = bounds.removeFromTop(30);
    g.setFont(juce::Font(juce::FontOptions(14.0f * scaleFactor).withName("Helvetica Neue")));
    g.setColour(PulseColors::TEXT_PRIMARY);
    g.drawText(trackName, nameBounds.reduced(10, 0), juce::Justification::centredLeft);
    
    // Status indicators
    if (isMuted)
    {
        g.setColour(PulseColors::TEXT_DIMMED);
        g.drawText("MUTED", nameBounds.reduced(10, 0), juce::Justification::centredRight);
    }
    else if (isSoloed)
    {
        g.setColour(PulseColors::TRACK_YELLOW);
        g.drawText("SOLO", nameBounds.reduced(10, 0), juce::Justification::centredRight);
    }
    else if (isArmed)
    {
        g.setColour(PulseColors::ERROR_RED);
        g.drawText("ARMED", nameBounds.reduced(10, 0), juce::Justification::centredRight);
    }
}

void TrackControlPanel::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromLeft(10); // Skip color strip
    bounds.removeFromTop(35);  // Skip name area
    
    auto buttonArea = bounds.removeFromBottom(25).reduced(5);
    int buttonWidth = buttonArea.getWidth() / 3;
    
    muteButton->setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(2));
    soloButton->setBounds(buttonArea.removeFromLeft(buttonWidth).reduced(2));
    armButton->setBounds(buttonArea.reduced(2));
}

juce::Colour TrackControlPanel::getTrackColor() const
{
    return PulseColors::getTrackColor(trackNum - 1);
}

//==============================================================================
// GridSystem Implementation
//==============================================================================
GridSystem::GridSystem(int width, int height, int columns, int rows)
    : totalWidth(width), totalHeight(height), 
      numColumns(columns), numRows(rows)
{
    cellWidth = totalWidth / numColumns;
    cellHeight = totalHeight / numRows;
}

juce::Rectangle<int> GridSystem::getCell(char row, int col, int rowSpan, int colSpan) const
{
    int rowIndex = row - 'A';
    return getCell(rowIndex, col - 1, rowSpan, colSpan);
}

juce::Rectangle<int> GridSystem::getCell(int rowIndex, int colIndex, int rowSpan, int colSpan) const
{
    if (rowIndex < 0 || rowIndex >= numRows || colIndex < 0 || colIndex >= numColumns)
        return {};
    
    return juce::Rectangle<int>(
        colIndex * cellWidth,
        rowIndex * cellHeight,
        cellWidth * colSpan,
        cellHeight * rowSpan
    );
}

juce::String GridSystem::getPositionString(const juce::Point<int>& point) const
{
    int col = (point.x / cellWidth) + 1;
    int row = (point.y / cellHeight);
    
    if (col < 1 || col > numColumns || row < 0 || row >= numRows)
        return "--";
    
    char rowChar = static_cast<char>('A' + row);
    return juce::String::charToString(rowChar) + juce::String(col);
}

void GridSystem::drawGrid(juce::Graphics& g, juce::Colour gridColor)
{
    if (!showGrid)
        return;
    
    g.setColour(gridColor);
    
    // Draw vertical lines
    for (int i = 1; i < numColumns; ++i)
    {
        int x = i * cellWidth;
        g.drawVerticalLine(x, 0, totalHeight);
    }
    
    // Draw horizontal lines
    for (int i = 1; i < numRows; ++i)
    {
        int y = i * cellHeight;
        g.drawHorizontalLine(y, 0, totalWidth);
    }
    
    // Draw labels
    g.setFont(juce::Font(juce::FontOptions(8.0f).withName("Helvetica Neue")));
    g.setColour(gridColor.withAlpha(0.5f));
    
    // Column numbers
    for (int i = 0; i < numColumns; ++i)
    {
        g.drawText(juce::String(i + 1),
                  i * cellWidth, 0, cellWidth, 15,
                  juce::Justification::centred);
    }
    
    // Row letters
    for (int i = 0; i < numRows; ++i)
    {
        char rowChar = static_cast<char>('A' + i);
        g.drawText(juce::String::charToString(rowChar),
                  0, i * cellHeight, 15, cellHeight,
                  juce::Justification::centred);
    }
}

//==============================================================================
// SectionContainer Implementation
//==============================================================================
SectionContainer::SectionContainer(const juce::String& name)
    : PulseComponent(name)
{
}

void SectionContainer::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Section background
    g.setColour(PulseColors::BG_DARK.withAlpha(0.5f));
    g.fillRoundedRectangle(bounds, 3.0f);
    
    // Section header
    if (!sectionTitle.isEmpty())
    {
        auto headerBounds = bounds.removeFromTop(30);
        
        // Header background
        fillWithGradient(g, headerBounds,
                        PulseColors::BG_MID,
                        PulseColors::BG_DARK);
        
        // Header text
        g.setFont(juce::Font(juce::FontOptions(12.0f * scaleFactor).withName("Helvetica Neue")));
        g.setColour(PulseColors::TEXT_PRIMARY);
        g.drawText(sectionTitle, 
                  headerBounds.reduced(10, 0),
                  juce::Justification::centredLeft);
        
        // Collapse button if collapsible
        if (isCollapsible && collapseButton)
        {
            // Button is handled by JUCE component system
        }
    }
    
    // Border
    g.setColour(PulseColors::BG_LIGHT.withAlpha(0.3f));
    g.drawRoundedRectangle(bounds, 3.0f, 1.0f);
    
    // Content area opacity based on collapse state
    if (contentComponent && collapsed)
    {
        g.setColour(PulseColors::BG_VOID.withAlpha(1.0f - collapseAnimation));
        g.fillRoundedRectangle(bounds.removeFromTop(bounds.getHeight() * collapseAnimation), 3.0f);
    }
}

void SectionContainer::resized()
{
    auto bounds = getLocalBounds();
    
    // Header area
    if (!sectionTitle.isEmpty())
    {
        auto headerBounds = bounds.removeFromTop(30);
        
        if (isCollapsible && !collapseButton)
        {
            collapseButton = std::make_unique<juce::TextButton>(collapsed ? "+" : "-");
            collapseButton->onClick = [this]() { setCollapsed(!collapsed); };
            addAndMakeVisible(collapseButton.get());
        }
        
        if (collapseButton)
        {
            collapseButton->setBounds(headerBounds.removeFromRight(30).reduced(5));
        }
    }
    
    // Content area
    if (contentComponent)
    {
        if (collapsed)
        {
            contentComponent->setBounds(bounds.withHeight(static_cast<int>(bounds.getHeight() * collapseAnimation)));
        }
        else
        {
            contentComponent->setBounds(bounds);
        }
    }
}

void SectionContainer::setCollapsed(bool shouldCollapse)
{
    if (collapsed != shouldCollapse)
    {
        collapsed = shouldCollapse;
        
        if (collapseButton)
        {
            collapseButton->setButtonText(collapsed ? "+" : "-");
        }
        
        animateCollapse();
    }
}

void SectionContainer::setContentComponent(std::unique_ptr<juce::Component> content)
{
    if (contentComponent)
    {
        removeChildComponent(contentComponent.get());
    }
    
    contentComponent = std::move(content);
    
    if (contentComponent)
    {
        addAndMakeVisible(contentComponent.get());
        resized();
    }
}

void SectionContainer::animateCollapse()
{
    // Simple animation - in production would use Timer for smooth animation
    collapseAnimation = collapsed ? 0.0f : 1.0f;
    resized();
    repaint();
}

} // namespace HAM