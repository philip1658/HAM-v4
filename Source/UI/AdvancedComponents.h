#pragma once

#include <JuceHeader.h>
#include "ComponentBase.h"
#include "BasicComponents.h"
#include <array>
#include <deque>

namespace HAM {

//==============================================================================
/**
 * Advanced UI Components for HAM
 * Contains complex widgets: StageCard, Pattern Editors, Visualizers
 */

//==============================================================================
// StageCard - 140x420px with 2x2 slider grid
class StageCard : public PulseComponent
{
public:
    StageCard(const juce::String& name, int stageNumber);
    ~StageCard() override = default;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Access to sliders
    PulseVerticalSlider* getPitchSlider() { return pitchSlider.get(); }
    PulseVerticalSlider* getPulseSlider() { return pulseSlider.get(); }
    PulseVerticalSlider* getVelocitySlider() { return velocitySlider.get(); }
    PulseVerticalSlider* getGateSlider() { return gateSlider.get(); }
    
    void setHighlighted(bool highlight) { isHighlighted = highlight; repaint(); }
    
    // Set scale degree information for visual indicators
    void setScaleDegree(int degree, bool isTonic = false, bool isDominant = false, bool isSubdominant = false) {
        m_scaleDegree = degree;
        m_isTonic = isTonic;
        m_isDominant = isDominant;
        m_isSubdominant = isSubdominant;
        repaint();
    }
    
private:
    int stageNum;
    bool isHighlighted = false;
    
    std::unique_ptr<PulseVerticalSlider> pitchSlider;
    std::unique_ptr<PulseVerticalSlider> pulseSlider;
    std::unique_ptr<PulseVerticalSlider> velocitySlider;
    std::unique_ptr<PulseVerticalSlider> gateSlider;
    
    std::unique_ptr<PulseButton> skipButton;
    std::unique_ptr<PulseButton> hamButton;
    
    // Scale degree information
    int m_scaleDegree = 0;  // 0 = not in scale, 1-7 = scale degree
    bool m_isTonic = false;
    bool m_isDominant = false;
    bool m_isSubdominant = false;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StageCard)
};

//==============================================================================
// ScaleSlotSelector - 8 scale slots with hover effects
class ScaleSlotSelector : public PulseComponent
{
public:
    ScaleSlotSelector(const juce::String& name);
    ~ScaleSlotSelector() override = default;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseMove(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;
    
    void setSelectedSlot(int slot);
    int getSelectedSlot() const { return selectedSlot; }
    
    void setSlotName(int slot, const juce::String& name);
    
    std::function<void(int)> onSlotSelected;
    
private:
    struct SlotInfo
    {
        juce::Rectangle<float> bounds;
        juce::String name;
        float hoverAmount = 0.0f;
        bool isActive = false;
    };
    
    std::array<SlotInfo, 8> slots;
    int selectedSlot = 0;
    int hoveredSlot = -1;
    
    int getSlotAtPosition(juce::Point<int> pos);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScaleSlotSelector)
};

//==============================================================================
// GatePatternEditor - 8-step pattern editor with drag
class GatePatternEditor : public PulseComponent
{
public:
    GatePatternEditor(const juce::String& name);
    ~GatePatternEditor() override = default;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    
    void setPattern(const std::array<float, 8>& pattern);
    std::array<float, 8> getPattern() const { return gatePattern; }
    
    std::function<void(int, float)> onGateChanged;
    
private:
    std::array<float, 8> gatePattern = {1, 1, 1, 1, 1, 1, 1, 1};
    int draggedStep = -1;
    
    int getStepAtPosition(juce::Point<int> pos);
    float getValueAtPosition(int y);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GatePatternEditor)
};

//==============================================================================
// PitchTrajectoryVisualizer - Spring-animated pitch visualization
class PitchTrajectoryVisualizer : public PulseComponent,
                                 private juce::Timer
{
public:
    PitchTrajectoryVisualizer(const juce::String& name);
    ~PitchTrajectoryVisualizer() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    void addPitchPoint(float pitch, float time);
    void clearTrajectory();
    
    void setAccumulatorValue(float value);
    void setScaleRange(int min, int max);
    
private:
    struct PitchPoint
    {
        float pitch;
        float time;
        float alpha = 1.0f;
    };
    
    std::deque<PitchPoint> trajectory;
    static constexpr size_t maxPoints = 128;
    
    float currentAccumulatorValue = 0.0f;
    float springPosition = 0.0f;
    float springVelocity = 0.0f;
    
    int scaleMin = -12;
    int scaleMax = 12;
    
    void timerCallback() override;
    void updateSpringAnimation();
    
    juce::Point<float> valueToPoint(float pitch, float time);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PitchTrajectoryVisualizer)
};

} // namespace HAM