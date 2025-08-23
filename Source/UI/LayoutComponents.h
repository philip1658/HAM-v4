#pragma once

#include <JuceHeader.h>
#include "ComponentBase.h"

namespace HAM {

//==============================================================================
/**
 * Layout & Container Components for HAM
 * Contains panels, control panels, and layout helpers
 */

//==============================================================================
// PulsePanel - Various panel styles (Flat, Raised, Recessed, Glass, TrackControl)
class PulsePanel : public PulseComponent
{
public:
    enum Style { Flat, Raised, Recessed, Glass, TrackControl };
    
    PulsePanel(const juce::String& name, Style style = Flat);
    ~PulsePanel() override = default;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    void setPanelTitle(const juce::String& title) { panelTitle = title; repaint(); }
    void setShowBorder(bool show) { showBorder = show; repaint(); }
    
private:
    Style panelStyle;
    juce::String panelTitle;
    bool showBorder = true;
    
    void drawFlatStyle(juce::Graphics& g);
    void drawRaisedStyle(juce::Graphics& g);
    void drawRecessedStyle(juce::Graphics& g);
    void drawGlassStyle(juce::Graphics& g);
    void drawTrackControlStyle(juce::Graphics& g);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PulsePanel)
};

//==============================================================================
// TrackControlPanel - Complete track control with gradient background
class TrackControlPanel : public PulseComponent
{
public:
    TrackControlPanel(const juce::String& name, int trackNumber);
    ~TrackControlPanel() override = default;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    void setTrackName(const juce::String& name) { trackName = name; repaint(); }
    void setMuted(bool mute) { isMuted = mute; repaint(); }
    void setSoloed(bool solo) { isSoloed = solo; repaint(); }
    void setArmed(bool arm) { isArmed = arm; repaint(); }
    
    // Callbacks
    std::function<void(bool)> onMuteChanged;
    std::function<void(bool)> onSoloChanged;
    std::function<void(bool)> onArmChanged;
    std::function<void(int)> onTrackSelected;
    
private:
    int trackNum;
    juce::String trackName;
    bool isMuted = false;
    bool isSoloed = false;
    bool isArmed = false;
    bool isSelected = false;
    
    std::unique_ptr<juce::TextButton> muteButton;
    std::unique_ptr<juce::TextButton> soloButton;
    std::unique_ptr<juce::TextButton> armButton;
    
    juce::Colour getTrackColor() const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackControlPanel)
};

//==============================================================================
// Grid system helper (moved from main library)
class GridSystem
{
public:
    GridSystem(int width, int height, int columns = 24, int rows = 24);
    
    juce::Rectangle<int> getCell(char row, int col, int rowSpan = 1, int colSpan = 1) const;
    juce::Rectangle<int> getCell(int rowIndex, int colIndex, int rowSpan = 1, int colSpan = 1) const;
    juce::String getPositionString(const juce::Point<int>& point) const;
    
    void setShowGrid(bool show) { showGrid = show; }
    bool isShowingGrid() const { return showGrid; }
    
    void drawGrid(juce::Graphics& g, juce::Colour gridColor = juce::Colours::grey.withAlpha(0.2f));
    
    int getCellWidth() const { return cellWidth; }
    int getCellHeight() const { return cellHeight; }
    
private:
    int totalWidth;
    int totalHeight;
    int numColumns;
    int numRows;
    int cellWidth;
    int cellHeight;
    bool showGrid = false;
};

//==============================================================================
// Container component for organizing UI sections
class SectionContainer : public PulseComponent
{
public:
    SectionContainer(const juce::String& name);
    ~SectionContainer() override = default;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    void setTitle(const juce::String& title) { sectionTitle = title; repaint(); }
    void setCollapsible(bool canCollapse) { isCollapsible = canCollapse; }
    void setCollapsed(bool collapsed);
    bool isCollapsed() const { return collapsed; }
    
    // Child component management
    void setContentComponent(std::unique_ptr<juce::Component> content);
    juce::Component* getContentComponent() { return contentComponent.get(); }
    
private:
    juce::String sectionTitle;
    bool isCollapsible = false;
    bool collapsed = false;
    float collapseAnimation = 1.0f;
    
    std::unique_ptr<juce::Component> contentComponent;
    std::unique_ptr<juce::TextButton> collapseButton;
    
    void animateCollapse();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SectionContainer)
};

} // namespace HAM