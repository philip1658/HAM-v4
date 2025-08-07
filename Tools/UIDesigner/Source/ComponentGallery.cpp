#include "ComponentGallery.h"

//==============================================================================
ComponentGallery::ComponentGallery()
{
    // Title
    titleLabel.setText("HAM UI Designer - Pulse Component Library", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(24.0f));
    titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xFF00FF88));
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);
    
    // Info
    infoLabel.setText("Scroll to view all components | Click and drag to test interactions", 
                     juce::dontSendNotification);
    infoLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFCCCCCC));
    infoLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(infoLabel);
    
    // Buttons
    exportButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF00FF88));
    exportButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFF000000));
    addAndMakeVisible(exportButton);
    
    gridToggle.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF00D9FF));
    gridToggle.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFF000000));
    addAndMakeVisible(gridToggle);
    
    // Create scrollable component view
    componentView = std::make_unique<ScrollableComponentView>();
    
    // Create viewport for scrolling
    viewport = std::make_unique<juce::Viewport>();
    viewport->setViewedComponent(componentView.get(), false);
    viewport->setScrollBarsShown(true, false);
    viewport->getVerticalScrollBar().setColour(juce::ScrollBar::backgroundColourId, 
                                               juce::Colour(0xFF1A1A1A));
    viewport->getVerticalScrollBar().setColour(juce::ScrollBar::thumbColourId, 
                                               juce::Colour(0xFF00FF88));
    addAndMakeVisible(viewport.get());
    
    // Create the actual Pulse component library
    pulseLibrary = std::make_unique<HAM::PulseComponentLibrary>();
    
    // Populate with organized components
    createComponentSections();
}

ComponentGallery::~ComponentGallery()
{
}

void ComponentGallery::paint(juce::Graphics& g)
{
    // Dark background
    g.fillAll(juce::Colour(0xFF000000));
    
    // Top bar background
    g.setColour(juce::Colour(0xFF1A1A1A));
    g.fillRect(0, 0, getWidth(), 100);
}

void ComponentGallery::resized()
{
    auto bounds = getLocalBounds();
    
    // Top bar
    auto topBar = bounds.removeFromTop(100);
    titleLabel.setBounds(topBar.removeFromTop(40).reduced(10, 5));
    infoLabel.setBounds(topBar.removeFromTop(30).reduced(10, 0));
    
    auto buttonArea = topBar.reduced(10, 0);
    exportButton.setBounds(buttonArea.removeFromLeft(120).reduced(5));
    gridToggle.setBounds(buttonArea.removeFromLeft(120).reduced(5));
    
    // Viewport takes remaining space
    viewport->setBounds(bounds.reduced(10));
    
    // Update component view size
    if (componentView)
    {
        componentView->setBounds(0, 0, viewport->getWidth() - 20, componentView->getTotalHeight());
    }
}

void ComponentGallery::createComponentSections()
{
    int yPos = 0;
    
    // SECTION 1: SLIDERS
    componentView->addSection("SLIDERS", yPos);
    yPos += 50;
    
    // Vertical sliders in a row
    for (int i = 0; i < 8; ++i)
    {
        auto slider = std::make_unique<HAM::PulseComponentLibrary::PulseVerticalSlider>(
            "V" + juce::String(i+1), i);
        componentView->addComponent(std::move(slider), 
                                   "VSLIDER_" + juce::String(i+1),
                                   100 + i * 60, yPos, 40, 200);
    }
    
    // Horizontal sliders
    yPos += 220;
    auto hslider1 = std::make_unique<HAM::PulseComponentLibrary::PulseHorizontalSlider>("HSLIDER_1", true);
    componentView->addComponent(std::move(hslider1), "HSLIDER_WITH_THUMB", 100, yPos, 300, 40);
    
    auto hslider2 = std::make_unique<HAM::PulseComponentLibrary::PulseHorizontalSlider>("HSLIDER_2", false);
    componentView->addComponent(std::move(hslider2), "HSLIDER_NO_THUMB", 450, yPos, 300, 40);
    
    yPos += 80;
    
    // SECTION 2: BUTTONS & TOGGLES
    componentView->addSection("BUTTONS & TOGGLES", yPos);
    yPos += 50;
    
    // Buttons
    auto btnSolid = std::make_unique<HAM::PulseComponentLibrary::PulseButton>(
        "SOLID", HAM::PulseComponentLibrary::PulseButton::Solid);
    componentView->addComponent(std::move(btnSolid), "BUTTON_SOLID", 100, yPos, 120, 40);
    
    auto btnOutline = std::make_unique<HAM::PulseComponentLibrary::PulseButton>(
        "OUTLINE", HAM::PulseComponentLibrary::PulseButton::Outline);
    componentView->addComponent(std::move(btnOutline), "BUTTON_OUTLINE", 240, yPos, 120, 40);
    
    auto btnGhost = std::make_unique<HAM::PulseComponentLibrary::PulseButton>(
        "GHOST", HAM::PulseComponentLibrary::PulseButton::Ghost);
    componentView->addComponent(std::move(btnGhost), "BUTTON_GHOST", 380, yPos, 120, 40);
    
    auto btnGradient = std::make_unique<HAM::PulseComponentLibrary::PulseButton>(
        "GRADIENT", HAM::PulseComponentLibrary::PulseButton::Gradient);
    componentView->addComponent(std::move(btnGradient), "BUTTON_GRADIENT", 520, yPos, 120, 40);
    
    // Toggles
    yPos += 60;
    auto toggle1 = std::make_unique<HAM::PulseComponentLibrary::PulseToggle>("MUTE");
    componentView->addComponent(std::move(toggle1), "TOGGLE_MUTE", 100, yPos, 100, 40);
    
    auto toggle2 = std::make_unique<HAM::PulseComponentLibrary::PulseToggle>("SOLO");
    componentView->addComponent(std::move(toggle2), "TOGGLE_SOLO", 220, yPos, 100, 40);
    
    auto toggle3 = std::make_unique<HAM::PulseComponentLibrary::PulseToggle>("MONO");
    componentView->addComponent(std::move(toggle3), "TOGGLE_MONO", 340, yPos, 100, 40);
    
    yPos += 80;
    
    // SECTION 3: DROPDOWNS
    componentView->addSection("DROPDOWNS", yPos);
    yPos += 50;
    
    auto dropdown1 = std::make_unique<HAM::PulseComponentLibrary::PulseDropdown>("SCALE");
    componentView->addComponent(std::move(dropdown1), "DROPDOWN_SCALE", 100, yPos, 200, 40);
    
    auto dropdown2 = std::make_unique<HAM::PulseComponentLibrary::PulseDropdown>("CHANNEL");
    componentView->addComponent(std::move(dropdown2), "DROPDOWN_CHANNEL", 320, yPos, 200, 40);
    
    yPos += 80;
    
    // SECTION 4: PANELS
    componentView->addSection("PANELS & BACKGROUNDS", yPos);
    yPos += 50;
    
    auto panelFlat = std::make_unique<HAM::PulseComponentLibrary::PulsePanel>(
        "FLAT", HAM::PulseComponentLibrary::PulsePanel::Flat);
    componentView->addComponent(std::move(panelFlat), "PANEL_FLAT", 100, yPos, 200, 120);
    
    auto panelRaised = std::make_unique<HAM::PulseComponentLibrary::PulsePanel>(
        "RAISED", HAM::PulseComponentLibrary::PulsePanel::Raised);
    componentView->addComponent(std::move(panelRaised), "PANEL_RAISED", 320, yPos, 200, 120);
    
    auto panelGlass = std::make_unique<HAM::PulseComponentLibrary::PulsePanel>(
        "GLASS", HAM::PulseComponentLibrary::PulsePanel::Glass);
    componentView->addComponent(std::move(panelGlass), "PANEL_GLASS", 540, yPos, 200, 120);
    
    auto panelTrack = std::make_unique<HAM::PulseComponentLibrary::PulsePanel>(
        "TRACK BG", HAM::PulseComponentLibrary::PulsePanel::TrackControl);
    componentView->addComponent(std::move(panelTrack), "PANEL_TRACK", 760, yPos, 200, 120);
    
    yPos += 160;
    
    // SECTION 5: SPECIAL COMPONENTS
    componentView->addSection("SPECIAL COMPONENTS", yPos);
    yPos += 50;
    
    // Scale Slot Selector
    auto scaleSlots = std::make_unique<HAM::PulseComponentLibrary::ScaleSlotSelector>("SCALES");
    componentView->addComponent(std::move(scaleSlots), "SCALE_SLOTS", 100, yPos, 600, 60);
    
    yPos += 80;
    
    // Gate Pattern Editor
    auto gatePattern = std::make_unique<HAM::PulseComponentLibrary::GatePatternEditor>("GATES");
    componentView->addComponent(std::move(gatePattern), "GATE_PATTERN", 100, yPos, 600, 120);
    
    yPos += 140;
    
    // Pitch Trajectory Visualizer
    auto pitchViz = std::make_unique<HAM::PulseComponentLibrary::PitchTrajectoryVisualizer>("PITCH");
    componentView->addComponent(std::move(pitchViz), "PITCH_VISUALIZER", 100, yPos, 400, 250);
    
    yPos += 280;
    
    // SECTION 6: STAGE CARDS
    componentView->addSection("STAGE CARDS", yPos);
    yPos += 50;
    
    // 4 Stage cards in a row
    for (int i = 0; i < 4; ++i)
    {
        auto stageCard = std::make_unique<HAM::PulseComponentLibrary::StageCard>(
            "STAGE_" + juce::String(i+1), i+1);
        componentView->addComponent(std::move(stageCard), 
                                   "STAGE_CARD_" + juce::String(i+1),
                                   100 + i * 160, yPos, 140, 420);
    }
    
    yPos += 450;
    
    // SECTION 7: TRACK CONTROLS
    componentView->addSection("TRACK CONTROL PANELS", yPos);
    yPos += 50;
    
    // 2 Track control panels
    for (int i = 0; i < 2; ++i)
    {
        auto trackCtrl = std::make_unique<HAM::PulseComponentLibrary::TrackControlPanel>(
            "TRACK_" + juce::String(i+1), i+1);
        componentView->addComponent(std::move(trackCtrl), 
                                   "TRACK_CONTROL_" + juce::String(i+1),
                                   100 + i * 420, yPos, 400, 200);
    }
    
    yPos += 250;
}

//==============================================================================
// SCROLLABLE COMPONENT VIEW IMPLEMENTATION
ComponentGallery::ScrollableComponentView::ScrollableComponentView()
{
    setSize(1000, 2000); // Will be adjusted based on content
}

void ComponentGallery::ScrollableComponentView::paint(juce::Graphics& g)
{
    // Background
    g.fillAll(juce::Colour(0xFF0A0A0A));
    
    // Grid
    g.setColour(juce::Colour(0xFF1A1A1A).withAlpha(0.3f));
    for (int x = 0; x < getWidth(); x += GRID_SIZE)
    {
        g.drawVerticalLine(x, 0, getHeight());
    }
    for (int y = 0; y < getHeight(); y += GRID_SIZE)
    {
        g.drawHorizontalLine(y, 0, getWidth());
    }
    
    // Section headers
    g.setFont(juce::Font(18.0f));
    for (const auto& section : sections)
    {
        // Section background
        g.setColour(juce::Colour(0xFF1A1A1A));
        g.fillRect(0, section.yPosition, getWidth(), 40);
        
        // Section title
        g.setColour(juce::Colour(0xFF00FF88));
        g.drawText(section.title, 20, section.yPosition, getWidth() - 40, 40,
                  juce::Justification::centredLeft);
        
        // Separator line
        g.setColour(juce::Colour(0xFF00FF88).withAlpha(0.3f));
        g.drawHorizontalLine(section.yPosition + 40, 20, getWidth() - 20);
    }
    
    // Component labels
    g.setFont(juce::Font(10.0f));
    g.setColour(juce::Colour(0xFF888888));
    for (const auto& comp : components)
    {
        auto labelBounds = comp.bounds.translated(0, -15).withHeight(12);
        g.drawText(comp.name, labelBounds, juce::Justification::centredLeft);
    }
}

void ComponentGallery::ScrollableComponentView::resized()
{
    // Components are positioned absolutely, no need to resize
}

void ComponentGallery::ScrollableComponentView::addSection(const juce::String& title, int yPosition)
{
    sections.push_back({title, yPosition});
    totalHeight = juce::jmax(totalHeight, yPosition + SECTION_SPACING);
}

void ComponentGallery::ScrollableComponentView::addComponent(std::unique_ptr<juce::Component> comp,
                                                             const juce::String& name,
                                                             int x, int y, int width, int height)
{
    auto bounds = juce::Rectangle<int>(x, y, width, height);
    comp->setBounds(bounds);
    addAndMakeVisible(comp.get());
    
    components.push_back({std::move(comp), name, bounds});
    totalHeight = juce::jmax(totalHeight, y + height + MARGIN);
    
    // Update parent size
    setSize(getWidth(), totalHeight);
}