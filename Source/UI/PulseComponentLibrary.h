#pragma once

#include <JuceHeader.h>
#include <map>
#include <memory>
#include <array>
#include <deque>

namespace HAM {

//==============================================================================
/**
 * EXACT Pulse Component Library
 * 
 * Perfect recreation of all Pulse UI components with:
 * - Multi-layer shadows
 * - Gradient fills  
 * - Hover/click animations
 * - Line indicators (no thumbs on vertical sliders)
 * - 22px track width on vertical sliders
 * - Glass effects
 * - Spring animations
 */
class PulseComponentLibrary : public juce::Component,
                              private juce::Timer
{
public:
    PulseComponentLibrary();
    ~PulseComponentLibrary() override;
    
    //==============================================================================
    // Grid System (24x24)
    juce::Rectangle<int> getGridCell(char row, int col, int rowSpan = 1, int colSpan = 1) const;
    juce::String getGridPosition(const juce::Point<int>& point) const;
    
    //==============================================================================
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseMove(const juce::MouseEvent& event) override;
    
public:
    //==============================================================================
    // Pulse Color Palette (EXACT from Pulse)
    struct PulseColors
    {
        // Background hierarchy
        static const juce::Colour BG_VOID;        // #000000 - Deepest black
        static const juce::Colour BG_DARKEST;     // #0A0A0A
        static const juce::Colour BG_DARK;        // #1A1A1A  
        static const juce::Colour BG_MID;         // #2A2A2A
        static const juce::Colour BG_LIGHT;       // #3A3A3A
        static const juce::Colour BG_RAISED;      // #4A4A4A
        static const juce::Colour BG_HIGHLIGHT;   // #5A5A5A
        
        // Text colors
        static const juce::Colour TEXT_PRIMARY;   // #FFFFFF
        static const juce::Colour TEXT_SECONDARY; // #CCCCCC
        static const juce::Colour TEXT_DIMMED;    // #888888
        static const juce::Colour TEXT_DISABLED;  // #555555
        
        // Track colors (Pulse accent colors)
        static const juce::Colour TRACK_MINT;     // #00FF88 - Primary accent
        static const juce::Colour TRACK_CYAN;     // #00D9FF
        static const juce::Colour TRACK_PINK;     // #FF0088
        static const juce::Colour TRACK_AMBER;    // #FFAA00
        static const juce::Colour TRACK_PURPLE;   // #FF00FF
        static const juce::Colour TRACK_BLUE;     // #0088FF
        static const juce::Colour TRACK_RED;      // #FF0044
        static const juce::Colour TRACK_YELLOW;   // #FFFF00
        
        // Special effects
        static const juce::Colour GLOW_CYAN;      // #00FFFF with alpha
        static const juce::Colour GLOW_GREEN;     // #00FF00 with alpha
        static const juce::Colour ERROR_RED;      // #FF0000
        static const juce::Colour WARNING_AMBER;  // #FFAA00
    };
    
    //==============================================================================
    // Base resizable component with animations
    class PulseComponent : public juce::Component
    {
    public:
        PulseComponent(const juce::String& name) : componentName(name) {}
        virtual ~PulseComponent() = default;
        
        const juce::String& getName() const { return componentName; }
        void setScaleFactor(float scale) { scaleFactor = scale; resized(); }
        
    protected:
        juce::String componentName;
        float scaleFactor = 1.0f;
        
        // Animation helpers
        float hoverAmount = 0.0f;
        float clickAnimation = 0.0f;
        float glowIntensity = 0.0f;
        bool isHovering = false;
        bool isPressed = false;
        
        // Common animation speed
        static constexpr float ANIMATION_SPEED = 0.08f;
        static constexpr float HOVER_FADE = 0.08f;
        static constexpr float CLICK_DECAY = 0.1f;
        
        // Multi-layer shadow helper
        void drawMultiLayerShadow(juce::Graphics& g, juce::Rectangle<float> bounds, 
                                 int layers = 3, float cornerRadius = 4.0f);
    };
    
    //==============================================================================
    // VERTICAL SLIDER (Pulse-Style with Line Indicator)
    class PulseVerticalSlider : public PulseComponent
    {
    public:
        PulseVerticalSlider(const juce::String& name, int trackColorIndex = 0);
        
        void paint(juce::Graphics& g) override;
        void mouseDown(const juce::MouseEvent& event) override;
        void mouseDrag(const juce::MouseEvent& event) override;
        void mouseEnter(const juce::MouseEvent& event) override;
        void mouseExit(const juce::MouseEvent& event) override;
        
        void setValue(float newValue) { value = targetValue = newValue; }
        float getValue() const { return value; }
        
    private:
        float value = 0.5f;
        float targetValue = 0.5f;
        float displayedValue = 0.5f; // Smoothed display value
        juce::Colour trackColor;
        int colorIndex;
        
        // Pulse-specific dimensions
        static constexpr float TRACK_WIDTH = 22.0f;  // Exact Pulse width
        static constexpr float LINE_THICKNESS = 2.0f;
        static constexpr float CORNER_RADIUS = 11.0f; // Half of track width
    };
    
    //==============================================================================
    // HORIZONTAL SLIDER (Pulse-Style with Thumb)
    class PulseHorizontalSlider : public PulseComponent
    {
    public:
        PulseHorizontalSlider(const juce::String& name, bool showThumb = true);
        
        void paint(juce::Graphics& g) override;
        void mouseDown(const juce::MouseEvent& event) override;
        void mouseDrag(const juce::MouseEvent& event) override;
        
    private:
        float value = 0.5f;
        bool hasThumb;
        juce::Colour trackColor;
        
        static constexpr float TRACK_HEIGHT = 20.0f;
        static constexpr float THUMB_SIZE = 16.0f;
    };
    
    //==============================================================================
    // MODERN BUTTON (Multi-layer shadows + gradients)
    class PulseButton : public PulseComponent
    {
    public:
        enum Style { Solid, Outline, Ghost, Gradient };
        
        PulseButton(const juce::String& name, Style style = Solid);
        
        void paint(juce::Graphics& g) override;
        void mouseEnter(const juce::MouseEvent& event) override;
        void mouseExit(const juce::MouseEvent& event) override;
        void mouseDown(const juce::MouseEvent& event) override;
        void mouseUp(const juce::MouseEvent& event) override;
        
    private:
        Style buttonStyle;
        juce::Colour baseColor;
    };
    
    //==============================================================================
    // TOGGLE SWITCH (iOS-style animated)
    class PulseToggle : public PulseComponent
    {
    public:
        PulseToggle(const juce::String& name);
        
        void paint(juce::Graphics& g) override;
        void mouseDown(const juce::MouseEvent& event) override;
        
        bool getToggleState() const { return isOn; }
        void setToggleState(bool state) { isOn = state; thumbPosition = state ? 1.0f : 0.0f; }
        
    private:
        bool isOn = false;
        float thumbPosition = 0.0f;      // Animated 0-1
        float backgroundOpacity = 0.0f;  // Color transition
        
        static constexpr float SWITCH_WIDTH = 48.0f;
        static constexpr float SWITCH_HEIGHT = 28.0f;
        static constexpr float THUMB_SIZE = 24.0f;
    };
    
    //==============================================================================
    // DROPDOWN (3-layer shadow + gradient)
    class PulseDropdown : public PulseComponent
    {
    public:
        PulseDropdown(const juce::String& name);
        
        void paint(juce::Graphics& g) override;
        void mouseEnter(const juce::MouseEvent& event) override;
        void mouseExit(const juce::MouseEvent& event) override;
        void mouseDown(const juce::MouseEvent& event) override;
        
    private:
        juce::String selectedText = "Select...";
        void drawArrow(juce::Graphics& g, juce::Rectangle<float> bounds);
    };
    
    //==============================================================================
    // PANEL/BACKGROUND (Glass effects + gradients)
    class PulsePanel : public PulseComponent
    {
    public:
        enum Style { Flat, Raised, Recessed, Glass, TrackControl };
        
        PulsePanel(const juce::String& name, Style style);
        
        void paint(juce::Graphics& g) override;
        
    private:
        Style panelStyle;
        
        void drawTrackControlBackground(juce::Graphics& g);
    };
    
    //==============================================================================
    // STAGE CARD (2x2 Slider Grid)
    class StageCard : public PulseComponent
    {
    public:
        StageCard(const juce::String& name, int stageNumber);
        
        void paint(juce::Graphics& g) override;
        void resized() override;
        
    private:
        int stage;
        std::unique_ptr<PulseVerticalSlider> pitchSlider;
        std::unique_ptr<PulseVerticalSlider> pulseSlider;
        std::unique_ptr<PulseVerticalSlider> velocitySlider;
        std::unique_ptr<PulseVerticalSlider> gateSlider;
        std::unique_ptr<PulseButton> hamButton;
        
        static constexpr int CARD_WIDTH = 140;
        static constexpr int CARD_HEIGHT = 420;
    };
    
    //==============================================================================
    // SCALE SLOT SELECTOR (8 slots)
    class ScaleSlotSelector : public PulseComponent
    {
    public:
        ScaleSlotSelector(const juce::String& name);
        
        void paint(juce::Graphics& g) override;
        void mouseDown(const juce::MouseEvent& event) override;
        
        void setSelectedSlot(int slot) { selectedSlot = juce::jlimit(0, 7, slot); }
        int getSelectedSlot() const { return selectedSlot; }
        
    private:
        int selectedSlot = 0;
        int hoveredSlot = -1;
        std::array<juce::String, 8> slotNames = {
            "Major", "Minor", "Dorian", "Phrygian",
            "Lydian", "Mixolyd", "Aeolian", "Locrian"
        };
        
        juce::Rectangle<float> getSlotBounds(int slot) const;
    };
    
    //==============================================================================
    // GATE PATTERN EDITOR (8-step pattern)
    class GatePatternEditor : public PulseComponent
    {
    public:
        GatePatternEditor(const juce::String& name);
        
        void paint(juce::Graphics& g) override;
        void mouseDown(const juce::MouseEvent& event) override;
        void mouseDrag(const juce::MouseEvent& event) override;
        
    private:
        std::array<float, 8> gateValues = {1, 1, 0.5f, 1, 0, 1, 0.75f, 1};
        int draggedStep = -1;
        
        juce::Rectangle<float> getStepBounds(int step) const;
    };
    
    //==============================================================================
    // PITCH TRAJECTORY VISUALIZER
    class PitchTrajectoryVisualizer : public PulseComponent
    {
    public:
        PitchTrajectoryVisualizer(const juce::String& name);
        
        void paint(juce::Graphics& g) override;
        void addPitchPoint(float pitch, int64_t timestamp = 0);
        void setCurrentStage(int stage) { currentStage = stage; }
        
    private:
        struct PitchPoint {
            float pitch;
            int64_t timestamp;
            float x, y; // Calculated positions
        };
        
        std::deque<PitchPoint> pitchHistory;
        std::array<float, 8> stagePitches = {0, 0.2f, -0.3f, 0.5f, 0.1f, -0.2f, 0.4f, 0};
        int currentStage = 0;
        
        // Spring animation for smooth trajectory
        class SpringAnimation {
        public:
            float position = 0.0f;
            float velocity = 0.0f;
            float target = 0.0f;
            
            void update(float deltaTime);
        };
        
        SpringAnimation xSpring, ySpring;
        
        static constexpr int MAX_HISTORY = 256;
        static constexpr float GRID_SIZE = 8.0f;
        
        void drawGrid(juce::Graphics& g);
        void drawTrajectory(juce::Graphics& g);
        void drawStageMarkers(juce::Graphics& g);
    };
    
    //==============================================================================
    // TRACK CONTROL PANEL (with gradient background)
    class TrackControlPanel : public PulseComponent
    {
    public:
        TrackControlPanel(const juce::String& name, int trackNumber);
        
        void paint(juce::Graphics& g) override;
        void resized() override;
        
    private:
        int track;
        juce::Colour trackColor;
        
        // Controls
        std::unique_ptr<PulseVerticalSlider> volumeSlider;
        std::unique_ptr<PulseDropdown> channelSelector;
        std::unique_ptr<PulseToggle> muteToggle;
        std::unique_ptr<PulseToggle> soloToggle;
        
        void drawGradientBackground(juce::Graphics& g);
    };
    
    //==============================================================================
    // Grid system
    struct GridInfo {
        int cellWidth = 50;
        int cellHeight = 50;
        bool showGrid = true;
        bool showLabels = true;
        juce::Point<int> hoveredCell;
    };
    
    GridInfo grid;
    juce::Label positionLabel;
    
    // Component storage
    std::map<juce::String, std::unique_ptr<PulseComponent>> components;
    
    // Timer for animations
    void timerCallback() override;
    
    // Helper to create all showcase components
    void createAllComponents();
    void layoutComponents();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PulseComponentLibrary)
};

} // namespace HAM