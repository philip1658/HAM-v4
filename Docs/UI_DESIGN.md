# UI Design Documentation

## 🎨 Design System Overview

HAM uses a dark, modern design language inspired by professional audio software with clear visual hierarchy and intuitive interactions.

### Color Palette

```cpp
namespace HAMColors {
    // Primary Colors
    const Colour BACKGROUND_DARK   = Colour(0xFF0A0A0A);  // Near black
    const Colour BACKGROUND_MID    = Colour(0xFF1A1A1A);  // Panel background
    const Colour BACKGROUND_LIGHT  = Colour(0xFF2A2A2A);  // Raised elements
    
    // Accent Colors
    const Colour ACCENT_PRIMARY    = Colour(0xFF00FF88);  // Mint green
    const Colour ACCENT_SECONDARY  = Colour(0xFF00D9FF);  // Cyan
    const Colour ACCENT_WARNING    = Colour(0xFFFFAA00);  // Amber
    const Colour ACCENT_ERROR      = Colour(0xFFFF0044);  // Red
    
    // Track Colors
    const std::array<Colour, 8> TRACK_COLORS = {
        Colour(0xFF00FF88),  // Mint
        Colour(0xFF00D9FF),  // Cyan
        Colour(0xFFFF0088),  // Pink
        Colour(0xFFFFAA00),  // Amber
        Colour(0xFF88FF00),  // Lime
        Colour(0xFFFF00FF),  // Magenta
        Colour(0xFF00FFFF),  // Aqua
        Colour(0xFFFF8800)   // Orange
    };
    
    // UI Elements
    const Colour TEXT_PRIMARY      = Colour(0xFFFFFFFF);  // White
    const Colour TEXT_SECONDARY    = Colour(0xFFB0B0B0);  // Light gray
    const Colour TEXT_DISABLED     = Colour(0xFF606060);  // Dark gray
    const Colour BORDER_SUBTLE     = Colour(0xFF303030);  // Subtle borders
    const Colour BORDER_FOCUSED    = Colour(0xFF00FF88);  // Focus state
}
```

## 🏗️ Component Architecture

### Main Layout Structure

```
┌─────────────────────────────────────────────────────────┐
│                    Transport Bar                        │
├──────────────┬──────────────────────────────────────────┤
│              │                                          │
│   Track      │         Stage Grid                       │
│   Sidebar    │         (8 x N matrix)                  │
│   (Left)     │                                         │
│              │                                          │
├──────────────┴──────────────────────────────────────────┤
│                    HAM Editor Panel                     │
│                 (Collapsible/Optional)                  │
└─────────────────────────────────────────────────────────┘
```

### Component Hierarchy

```cpp
class MainComponent : public juce::Component {
    // Main layout components
    std::unique_ptr<TransportBar> m_transportBar;
    std::unique_ptr<TrackSidebar> m_trackSidebar;
    std::unique_ptr<StageGrid> m_stageGrid;
    std::unique_ptr<HAMEditorPanel> m_hamEditor;
    
    // Layout management
    juce::FlexBox m_mainLayout;
    juce::StretchableLayoutManager m_layoutManager;
    
public:
    void resized() override {
        // Responsive layout
        auto bounds = getLocalBounds();
        
        // Transport bar at top (60px)
        m_transportBar->setBounds(bounds.removeFromTop(60));
        
        // HAM Editor at bottom (collapsible, 200px when open)
        if (m_hamEditor->isVisible()) {
            m_hamEditor->setBounds(bounds.removeFromBottom(200));
        }
        
        // Track sidebar (250px) and stage grid (remaining)
        m_trackSidebar->setBounds(bounds.removeFromLeft(250));
        m_stageGrid->setBounds(bounds);
    }
};
```

## 🎹 Track Sidebar Design

### Track Control Component

```cpp
class TrackControl : public juce::Component {
    // Visual Elements
    juce::TextButton m_muteButton{"M"};
    juce::TextButton m_soloButton{"S"};
    juce::Slider m_volumeFader;
    juce::Label m_trackName;
    
    // Track-specific controls
    juce::ComboBox m_divisionSelector;
    juce::Slider m_swingSlider;
    juce::Slider m_octaveSlider;
    juce::Slider m_pulseLengthSlider;
    juce::TextButton m_voiceModeButton{"MONO"};
    
    // New features
    juce::ComboBox m_midiChannelSelector;    // Channel assignment
    juce::TextButton m_snapshotButton{"SNAP"};  // Quick snapshot
    juce::Slider m_morphSlider;              // Pattern morphing
    
    // Plugin slot
    std::unique_ptr<PluginSlot> m_pluginSlot;
    
    // Visual feedback
    juce::Colour m_trackColor;
    bool m_isSelected = false;
    bool m_isMorphing = false;
    
public:
    void paint(juce::Graphics& g) override {
        // Background with track color accent
        g.fillAll(HAMColors::BACKGROUND_MID);
        
        // Track color indicator strip
        g.setColour(m_trackColor);
        g.fillRect(0, 0, 4, getHeight());
        
        // Selection highlight
        if (m_isSelected) {
            g.setColour(HAMColors::ACCENT_PRIMARY.withAlpha(0.2f));
            g.fillRect(getLocalBounds());
        }
        
        // Subtle border
        g.setColour(HAMColors::BORDER_SUBTLE);
        g.drawRect(getLocalBounds());
    }
};
```

### Plugin Slot UI

```cpp
class PluginSlot : public juce::Component {
    enum State { Empty, Loading, Loaded, Error };
    State m_state = Empty;
    String m_pluginName;
    
public:
    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().reduced(4);
        
        // Background based on state
        switch (m_state) {
            case Empty:
                g.setColour(HAMColors::BACKGROUND_DARK);
                g.fillRoundedRectangle(bounds.toFloat(), 4.0f);
                
                // Dashed border for drop zone
                g.setColour(HAMColors::BORDER_SUBTLE);
                Path dashedBorder;
                dashedBorder.addRoundedRectangle(bounds.toFloat(), 4.0f);
                PathStrokeType stroke(1.0f);
                float dashes[] = {4.0f, 4.0f};
                stroke.createDashedStroke(dashedBorder, dashedBorder, 
                                         dashes, 2);
                g.strokePath(dashedBorder, stroke);
                
                // "Drop Plugin Here" text
                g.setColour(HAMColors::TEXT_DISABLED);
                g.drawText("Drop Plugin", bounds, 
                          Justification::centred);
                break;
                
            case Loaded:
                g.setColour(HAMColors::BACKGROUND_LIGHT);
                g.fillRoundedRectangle(bounds.toFloat(), 4.0f);
                
                // Plugin name
                g.setColour(HAMColors::TEXT_PRIMARY);
                g.drawText(m_pluginName, bounds, 
                          Justification::centred);
                break;
        }
    }
    
    void mouseDown(const MouseEvent& e) override {
        if (m_state == Loaded && e.getNumberOfClicks() == 2) {
            // Double-click opens plugin UI
            openPluginWindow();
        } else if (m_state == Empty) {
            // Single click opens browser
            openPluginBrowser();
        }
    }
};
```

## 📊 Stage Grid Design

### Stage Card Component

```cpp
class StageCard : public juce::Component {
    // Core parameters
    std::unique_ptr<RotaryKnob> m_pitchKnob;
    std::unique_ptr<RotaryKnob> m_gateKnob;
    std::unique_ptr<RotaryKnob> m_velocityKnob;
    std::unique_ptr<RotaryKnob> m_pulseLengthKnob;
    
    // Stage number display
    juce::Label m_stageNumber;
    
    // HAM Editor button
    juce::TextButton m_hamButton{"HAM"};
    
    // New morphing UI
    std::unique_ptr<MorphIndicator> m_morphIndicator;
    
    // Visual state
    bool m_isActive = false;
    bool m_isPlaying = false;
    float m_gateAnimation = 0.0f;
    float m_morphAmount = 0.0f;  // 0.0-1.0 morphing visualization
    
public:
    StageCard(int stageIndex) {
        // Initialize knobs with custom look
        m_pitchKnob = std::make_unique<RotaryKnob>("PITCH");
        m_pitchKnob->setRange(0, 127, 1);
        
        m_gateKnob = std::make_unique<RotaryKnob>("GATE");
        m_gateKnob->setRange(0.0, 1.0, 0.01);
        
        m_velocityKnob = std::make_unique<RotaryKnob>("VEL");
        m_velocityKnob->setRange(0, 127, 1);
        
        m_pulseLengthKnob = std::make_unique<RotaryKnob>("PULSE");
        m_pulseLengthKnob->setRange(1, 8, 1);
        
        // Stage number
        m_stageNumber.setText(String(stageIndex + 1), dontSendNotification);
        m_stageNumber.setFont(Font(24.0f, Font::bold));
        
        // HAM button styling
        m_hamButton.setColour(TextButton::buttonColourId, 
                             HAMColors::ACCENT_PRIMARY);
    }
    
    void paint(Graphics& g) override {
        // Card background
        g.setColour(HAMColors::BACKGROUND_LIGHT);
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 8.0f);
        
        // Playing indicator
        if (m_isPlaying) {
            g.setColour(HAMColors::ACCENT_PRIMARY.withAlpha(m_gateAnimation));
            g.fillRoundedRectangle(getLocalBounds().toFloat(), 8.0f);
        }
        
        // Active stage highlight
        if (m_isActive) {
            g.setColour(HAMColors::ACCENT_PRIMARY);
            g.drawRoundedRectangle(getLocalBounds().toFloat(), 8.0f, 2.0f);
        }
    }
    
    void timerCallback() {
        // Animate gate visualization
        if (m_isPlaying) {
            m_gateAnimation *= 0.95f;  // Decay
            if (m_gateAnimation < 0.01f) {
                m_isPlaying = false;
            }
            repaint();
        }
    }
};
```

### Custom Rotary Knob

```cpp
class RotaryKnob : public juce::Slider {
    String m_label;
    
public:
    RotaryKnob(const String& label) : m_label(label) {
        setSliderStyle(Slider::RotaryVerticalDrag);
        setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
        setRotaryParameters(MathConstants<float>::pi * 1.2f,
                           MathConstants<float>::pi * 2.8f,
                           true);
    }
    
    void paint(Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();
        auto radius = jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto center = bounds.getCentre();
        auto angle = rotaryStartAngle + sliderPos * 
                    (rotaryEndAngle - rotaryStartAngle);
        
        // Knob background
        g.setColour(HAMColors::BACKGROUND_DARK);
        g.fillEllipse(center.x - radius, center.y - radius, 
                     radius * 2.0f, radius * 2.0f);
        
        // Value arc
        Path valuePath;
        valuePath.addCentredArc(center.x, center.y, radius * 0.8f, 
                                radius * 0.8f, 0.0f,
                                rotaryStartAngle, angle, true);
        
        g.setColour(HAMColors::ACCENT_PRIMARY);
        g.strokePath(valuePath, PathStrokeType(3.0f));
        
        // Center dot
        g.fillEllipse(center.x - 3, center.y - 3, 6, 6);
        
        // Label
        g.setColour(HAMColors::TEXT_SECONDARY);
        g.setFont(10.0f);
        g.drawText(m_label, bounds.removeFromBottom(15), 
                  Justification::centred);
        
        // Value display on hover
        if (isMouseOver()) {
            g.setColour(HAMColors::TEXT_PRIMARY);
            g.setFont(12.0f);
            g.drawText(String(getValue(), 1), 
                      Rectangle<float>(center.x - 20, center.y - 6, 40, 12),
                      Justification::centred);
        }
    }
};
```

## 🎛️ HAM Editor Panel

### Extended Stage Editor

```cpp
class HAMEditorPanel : public Component {
    // Tabbed interface for different edit modes
    TabbedComponent m_tabs;
    
    // Edit pages
    std::unique_ptr<GateEditor> m_gateEditor;
    std::unique_ptr<RatchetEditor> m_ratchetEditor;
    std::unique_ptr<ModulationEditor> m_modulationEditor;
    std::unique_ptr<CCMappingEditor> m_ccEditor;
    
    // Currently editing stage
    TrackId m_currentTrack;
    int m_currentStage = -1;
    
public:
    HAMEditorPanel() {
        // Create tab pages
        m_tabs.addTab("Gate", HAMColors::BACKGROUND_MID, 
                     m_gateEditor.get(), false);
        m_tabs.addTab("Ratchet", HAMColors::BACKGROUND_MID, 
                     m_ratchetEditor.get(), false);
        m_tabs.addTab("Modulation", HAMColors::BACKGROUND_MID, 
                     m_modulationEditor.get(), false);
        m_tabs.addTab("CC Map", HAMColors::BACKGROUND_MID, 
                     m_ccEditor.get(), false);
        
        // Styling
        m_tabs.setColour(TabbedComponent::backgroundColourId, 
                        HAMColors::BACKGROUND_DARK);
        m_tabs.setColour(TabbedComponent::outlineColourId, 
                        HAMColors::BORDER_SUBTLE);
    }
};
```

### Gate Pattern Editor

```cpp
class GateEditor : public Component {
    // Visual gate pattern editor
    class GatePatternDisplay : public Component {
        std::array<bool, 8> m_pattern{true, true, false, true, 
                                      true, false, true, false};
        
    public:
        void paint(Graphics& g) override {
            auto bounds = getLocalBounds();
            float stepWidth = bounds.getWidth() / 8.0f;
            
            for (int i = 0; i < 8; ++i) {
                Rectangle<float> stepBounds(i * stepWidth, 0, 
                                           stepWidth - 2, bounds.getHeight());
                
                if (m_pattern[i]) {
                    // Active step
                    g.setColour(HAMColors::ACCENT_PRIMARY);
                    g.fillRoundedRectangle(stepBounds, 4.0f);
                } else {
                    // Inactive step
                    g.setColour(HAMColors::BACKGROUND_LIGHT);
                    g.fillRoundedRectangle(stepBounds, 4.0f);
                }
                
                // Step number
                g.setColour(m_pattern[i] ? HAMColors::BACKGROUND_DARK 
                                         : HAMColors::TEXT_SECONDARY);
                g.drawText(String(i + 1), stepBounds, 
                          Justification::centred);
            }
        }
        
        void mouseDown(const MouseEvent& e) override {
            int step = (e.x * 8) / getWidth();
            if (step >= 0 && step < 8) {
                m_pattern[step] = !m_pattern[step];
                repaint();
            }
        }
    };
    
    GatePatternDisplay m_patternDisplay;
    Slider m_probabilitySlider;
    ComboBox m_gateTypeSelector;
};
```

## 🚀 Transport Bar

### Transport Controls

```cpp
class TransportBar : public Component {
    // Transport buttons
    TextButton m_playButton{"▶"};
    TextButton m_stopButton{"■"};
    TextButton m_recordButton{"●"};
    
    // Tempo control
    Slider m_tempoSlider;
    Label m_tempoLabel;
    
    // Time display
    Label m_timeDisplay;
    
    // Pattern selector with scene management
    ComboBox m_patternSelector;
    TextButton m_sceneUpButton{"▲"};
    TextButton m_sceneDownButton{"▼"};
    Label m_sceneDisplay;       // Current scene indicator
    
    // New morphing controls
    Slider m_globalMorphSlider;  // Global pattern morphing
    TextButton m_morphModeButton{"MORPH"};
    
    // CPU meter
    class CPUMeter : public Component, public Timer {
        float m_cpuUsage = 0.0f;
        
    public:
        void paint(Graphics& g) override {
            auto bounds = getLocalBounds().toFloat();
            
            // Background
            g.setColour(HAMColors::BACKGROUND_DARK);
            g.fillRoundedRectangle(bounds, 4.0f);
            
            // CPU bar
            float barWidth = bounds.getWidth() * (m_cpuUsage / 100.0f);
            Colour barColor = m_cpuUsage < 50 ? HAMColors::ACCENT_PRIMARY
                           : m_cpuUsage < 75 ? HAMColors::ACCENT_WARNING
                           : HAMColors::ACCENT_ERROR;
            
            g.setColour(barColor);
            g.fillRoundedRectangle(bounds.withWidth(barWidth), 4.0f);
            
            // Text
            g.setColour(HAMColors::TEXT_PRIMARY);
            g.drawText(String(m_cpuUsage, 1) + "%", bounds, 
                      Justification::centred);
        }
        
        void timerCallback() override {
            m_cpuUsage = getCurrentCPUUsage();
            repaint();
        }
    };
    
    CPUMeter m_cpuMeter;
};
```

## 🎯 Interaction Design

### Mouse Interactions

```cpp
namespace Interactions {
    // Single Click: Select
    // Double Click: Edit/Open
    // Right Click: Context Menu
    // Drag: Adjust Value
    // Shift+Drag: Fine Adjustment
    // Cmd+Click: Multi-select
    // Alt+Drag: Duplicate
    
    class InteractionHandler {
        void handleStageCardClick(const MouseEvent& e, StageCard* card) {
            if (e.mods.isCommandDown()) {
                // Multi-select
                card->toggleSelection();
            } else if (e.mods.isAltDown()) {
                // Start duplicate drag
                startDuplicateDrag(card);
            } else if (e.getNumberOfClicks() == 2) {
                // Open HAM Editor
                openHAMEditor(card);
            } else {
                // Single select
                selectOnly(card);
            }
        }
    };
}
```

### Keyboard Shortcuts

```cpp
class KeyboardShortcuts : public KeyListener {
    bool keyPressed(const KeyPress& key, Component* source) override {
        // Transport
        if (key == KeyPress::spaceKey) {
            togglePlayStop();
            return true;
        }
        
        // Navigation
        if (key == KeyPress::leftKey) {
            selectPreviousStage();
            return true;
        }
        if (key == KeyPress::rightKey) {
            selectNextStage();
            return true;
        }
        
        // Stage jumps
        if (key >= '1' && key <= '8') {
            jumpToStage(key.getKeyCode() - '1');
            return true;
        }
        
        // Copy/Paste
        if (key == KeyPress('c', ModifierKeys::commandModifier)) {
            copySelectedStages();
            return true;
        }
        if (key == KeyPress('v', ModifierKeys::commandModifier)) {
            pasteStages();
            return true;
        }
        
        return false;
    }
};
```

## 🌈 Visual Feedback

### Animation System

```cpp
class AnimationController {
    struct Animation {
        Component* target;
        String property;
        float startValue;
        float endValue;
        float duration;
        float elapsed;
        std::function<float(float)> easing;
    };
    
    std::vector<Animation> m_animations;
    
public:
    void animateGateTrigger(StageCard* card) {
        Animation anim{
            .target = card,
            .property = "gateAnimation",
            .startValue = 1.0f,
            .endValue = 0.0f,
            .duration = 0.3f,
            .elapsed = 0.0f,
            .easing = Easings::exponentialOut
        };
        
        m_animations.push_back(anim);
    }
    
    void update(float deltaTime) {
        for (auto& anim : m_animations) {
            anim.elapsed += deltaTime;
            float t = anim.elapsed / anim.duration;
            
            if (t >= 1.0f) {
                // Animation complete
                setProperty(anim.target, anim.property, anim.endValue);
                // Remove from list...
            } else {
                // Interpolate
                float easedT = anim.easing(t);
                float value = anim.startValue + 
                             (anim.endValue - anim.startValue) * easedT;
                setProperty(anim.target, anim.property, value);
            }
        }
    }
};
```

## 📱 Responsive Design

### Breakpoints and Scaling

```cpp
class ResponsiveLayout {
    enum class LayoutMode {
        Compact,   // < 1024px width
        Normal,    // 1024-1920px
        Wide       // > 1920px
    };
    
    LayoutMode getCurrentMode() {
        int width = getWidth();
        if (width < 1024) return LayoutMode::Compact;
        if (width > 1920) return LayoutMode::Wide;
        return LayoutMode::Normal;
    }
    
    void applyLayout() {
        switch (getCurrentMode()) {
            case LayoutMode::Compact:
                // Stack sidebar above grid
                m_trackSidebar->setVisible(false);
                m_compactTrackSelector->setVisible(true);
                break;
                
            case LayoutMode::Normal:
                // Standard side-by-side
                m_trackSidebar->setSize(250, getHeight());
                break;
                
            case LayoutMode::Wide:
                // Expanded controls
                m_trackSidebar->setSize(350, getHeight());
                m_hamEditor->setAlwaysVisible(true);
                break;
        }
    }
};
```

## 🎨 Custom Look and Feel

### HAM Look and Feel Class

```cpp
class HAMLookAndFeel : public juce::LookAndFeel_V4 {
public:
    HAMLookAndFeel() {
        // Set color scheme
        setColour(ResizableWindow::backgroundColourId, 
                 HAMColors::BACKGROUND_DARK);
        setColour(Slider::backgroundColourId, 
                 HAMColors::BACKGROUND_MID);
        setColour(Slider::trackColourId, 
                 HAMColors::ACCENT_PRIMARY);
        setColour(TextButton::buttonColourId, 
                 HAMColors::BACKGROUND_LIGHT);
        setColour(TextButton::textColourOffId, 
                 HAMColors::TEXT_PRIMARY);
        
        // Custom fonts
        setDefaultSansSerifTypeface(Typeface::createSystemTypefaceFor(
            BinaryData::InterRegular_ttf,
            BinaryData::InterRegular_ttfSize));
    }
    
    void drawRotarySlider(Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle,
                          float rotaryEndAngle, Slider& slider) override {
        // Custom rotary knob drawing
        // See RotaryKnob implementation above
    }
    
    void drawButtonBackground(Graphics& g, Button& button,
                              const Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override {
        auto bounds = button.getLocalBounds().toFloat();
        
        // Custom button with rounded corners
        Colour bg = backgroundColour;
        if (shouldDrawButtonAsDown) {
            bg = bg.darker(0.2f);
        } else if (shouldDrawButtonAsHighlighted) {
            bg = bg.brighter(0.1f);
        }
        
        g.setColour(bg);
        g.fillRoundedRectangle(bounds, 4.0f);
        
        // Focus indicator
        if (button.hasKeyboardFocus(true)) {
            g.setColour(HAMColors::ACCENT_PRIMARY);
            g.drawRoundedRectangle(bounds, 4.0f, 2.0f);
        }
    }
};
```

## 🔄 State Management

### View Models (MVVM Pattern)

```cpp
class StageViewModel : public juce::ChangeBroadcaster {
    // Domain model reference
    Stage* m_stage;
    
    // UI-specific state
    bool m_isSelected = false;
    bool m_isHovered = false;
    float m_animationPhase = 0.0f;
    
    // Cached values for UI
    std::atomic<int> m_pitch{60};
    std::atomic<float> m_gate{0.5f};
    std::atomic<int> m_velocity{100};
    
public:
    void updateFromDomain() {
        // Pull changes from domain model
        m_pitch = m_stage->getPitch();
        m_gate = m_stage->getGate();
        m_velocity = m_stage->getVelocity();
        
        // Notify UI
        sendChangeMessage();
    }
    
    void setPitch(int pitch) {
        // Update domain model
        m_stage->setPitch(pitch);
        
        // Update cached value
        m_pitch = pitch;
        
        // Notify listeners
        sendChangeMessage();
    }
};
```

## 🎭 Advanced UI Features

### Scene Manager UI

```cpp
class SceneManagerPanel : public Component {
    // Scene slots (64 total)
    struct SceneSlot {
        int slotIndex;
        String sceneName;
        bool hasSnapshot;
        bool isActive;
        Colour slotColor;
        TimeStamp lastModified;
    };
    
    std::array<SceneSlot, 64> m_sceneSlots;
    
    // UI components
    class SceneButton : public TextButton {
        SceneSlot* m_slot;
        
    public:
        void paint(Graphics& g) override {
            // Draw scene slot with visual indicators
            auto bounds = getLocalBounds();
            
            // Background based on state
            if (m_slot->isActive) {
                g.setColour(HAMColors::ACCENT_PRIMARY);
                g.fillRoundedRectangle(bounds.toFloat(), 4.0f);
            } else if (m_slot->hasSnapshot) {
                g.setColour(HAMColors::BACKGROUND_LIGHT);
                g.fillRoundedRectangle(bounds.toFloat(), 4.0f);
            } else {
                g.setColour(HAMColors::BACKGROUND_DARK);
                g.drawRoundedRectangle(bounds.toFloat(), 4.0f, 1.0f);
            }
            
            // Scene number and name
            g.setColour(HAMColors::TEXT_PRIMARY);
            g.drawText(String(m_slot->slotIndex + 1), bounds.removeFromTop(12), 
                      Justification::centred);
            
            if (m_slot->hasSnapshot) {
                g.setColour(HAMColors::TEXT_SECONDARY);
                g.drawText(m_slot->sceneName, bounds, 
                          Justification::centred);
            }
        }
    };
    
    // Scene controls
    TextButton m_saveSceneButton{"SAVE"};
    TextButton m_loadSceneButton{"LOAD"};
    TextButton m_morphSceneButton{"MORPH"};
    Slider m_morphQuantizeSlider;  // Quantization grid
    
public:
    void saveCurrentScene(int slot, const String& name);
    void loadScene(int slot, bool quantized = true);
    void morphToScene(int slot, float amount, bool quantized = true);
};
```

### Pattern Morphing Visualizer

```cpp
class MorphVisualizer : public Component, public Timer {
    struct MorphState {
        PatternSnapshot sourcePattern;
        PatternSnapshot targetPattern;
        float morphAmount;  // 0.0 = source, 1.0 = target
        bool isAnimating;
        float animationSpeed;
    };
    
    MorphState m_morphState;
    
public:
    void paint(Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();
        
        // Draw morph progress bar
        g.setColour(HAMColors::BACKGROUND_DARK);
        g.fillRoundedRectangle(bounds, 4.0f);
        
        // Morph amount visualization
        float morphWidth = bounds.getWidth() * m_morphState.morphAmount;
        g.setColour(HAMColors::ACCENT_PRIMARY.withAlpha(0.8f));
        g.fillRoundedRectangle(bounds.withWidth(morphWidth), 4.0f);
        
        // Stage-by-stage morph indicators
        for (int stage = 0; stage < 8; ++stage) {
            float x = bounds.getX() + (stage * bounds.getWidth() / 8);
            float stageWidth = bounds.getWidth() / 8 - 2;
            
            // Show interpolation between source and target for each stage
            Stage morphedStage = interpolateStage(
                m_morphState.sourcePattern.getStage(stage),
                m_morphState.targetPattern.getStage(stage),
                m_morphState.morphAmount
            );
            
            // Visual representation of morphed parameters
            drawStageMorph(g, Rectangle<float>(x, bounds.getY(), stageWidth, bounds.getHeight()), 
                          morphedStage, stage);
        }
    }
    
    void timerCallback() override {
        if (m_morphState.isAnimating) {
            // Smooth morphing animation
            repaint();
        }
    }
};
```

### Multi-Channel MIDI Visualizer

```cpp
class MidiChannelVisualizer : public Component {
    struct ChannelState {
        int channelNumber;  // 1-16
        TrackId assignedTrack;
        bool isActive;
        int noteCount;
        float activity;     // 0.0-1.0 activity level
        Colour channelColor;
    };
    
    std::array<ChannelState, 16> m_channelStates;
    
public:
    void paint(Graphics& g) override {
        auto bounds = getLocalBounds();
        
        // Draw 16 channel strips
        float channelWidth = bounds.getWidth() / 16.0f;
        
        for (int ch = 0; ch < 16; ++ch) {
            auto& state = m_channelStates[ch];
            Rectangle<float> channelBounds(ch * channelWidth, 0, 
                                          channelWidth - 1, bounds.getHeight());
            
            // Channel background
            if (state.isActive) {
                g.setColour(state.channelColor.withAlpha(0.3f));
                g.fillRoundedRectangle(channelBounds, 2.0f);
            } else {
                g.setColour(HAMColors::BACKGROUND_DARK);
                g.fillRoundedRectangle(channelBounds, 2.0f);
            }
            
            // Activity meter
            if (state.activity > 0.0f) {
                float activityHeight = channelBounds.getHeight() * state.activity;
                Rectangle<float> activityBounds = channelBounds;
                activityBounds.setHeight(activityHeight);
                activityBounds.setY(channelBounds.getBottom() - activityHeight);
                
                g.setColour(state.channelColor);
                g.fillRoundedRectangle(activityBounds, 2.0f);
            }
            
            // Channel number
            g.setColour(HAMColors::TEXT_PRIMARY);
            g.setFont(10.0f);
            g.drawText(String(ch + 1), channelBounds.removeFromTop(12), 
                      Justification::centred);
        }
    }
    
    void updateChannelActivity(int channel, float activity) {
        if (channel >= 1 && channel <= 16) {
            m_channelStates[channel - 1].activity = activity;
            repaint();
        }
    }
};
```

---

*This UI design document provides the visual and interaction framework for HAM's user interface, including advanced features like scene management, pattern morphing, and multi-channel MIDI visualization.*