#pragma once

#include <JuceHeader.h>
#include "ComponentBase.h"

namespace HAM {

//==============================================================================
/**
 * Basic UI Components for HAM
 * Contains fundamental UI elements: Sliders, Buttons, Toggles, Dropdowns
 */

//==============================================================================
// PulseVerticalSlider - Line indicator without thumb (22px track width)
class PulseVerticalSlider : public PulseComponent
{
public:
    PulseVerticalSlider(const juce::String& name, int trackColorIndex);
    ~PulseVerticalSlider() override = default;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseEnter(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;
    
    void setValue(float newValue) { value = juce::jlimit(0.0f, 1.0f, newValue); repaint(); }
    float getValue() const { return value; }
    
    void setLabel(const juce::String& text) { label = text; repaint(); }
    void setValueLabel(const juce::String& text) { valueLabel = text; repaint(); }
    
private:
    float value = 0.5f;
    int trackColorIdx = 0;
    juce::String label;
    juce::String valueLabel;
    
    void updateValueFromMouse(const juce::MouseEvent& event);
    juce::Colour getTrackColor() const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PulseVerticalSlider)
};

//==============================================================================
// PulseHorizontalSlider - Optional thumb for horizontal orientation
class PulseHorizontalSlider : public PulseComponent
{
public:
    explicit PulseHorizontalSlider(const juce::String& name, bool showThumb = true);
    ~PulseHorizontalSlider() override = default;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    
    void setValue(float newValue) { value = juce::jlimit(0.0f, 1.0f, newValue); repaint(); }
    float getValue() const { return value; }
    
private:
    float value = 0.5f;
    bool hasThumb = true;
    
    void updateValueFromMouse(const juce::MouseEvent& event);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PulseHorizontalSlider)
};

//==============================================================================
// PulseButton - Multiple styles (Solid, Outline, Ghost, Gradient)
class PulseButton : public PulseComponent
{
public:
    enum Style { Solid, Outline, Ghost, Gradient };
    
    explicit PulseButton(const juce::String& name, Style style = Solid);
    ~PulseButton() override = default;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseEnter(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;
    
    void setButtonText(const juce::String& text) { buttonText = text; repaint(); }
    std::function<void()> onClick;
    
private:
    Style buttonStyle;
    juce::String buttonText;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PulseButton)
};

//==============================================================================
// PulseToggle - iOS-style animated switch
class PulseToggle : public PulseComponent
{
public:
    PulseToggle(const juce::String& name);
    ~PulseToggle() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseEnter(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;
    
    void setToggleState(bool state) { isOn = state; animateToggle(); }
    bool getToggleState() const { return isOn; }
    
    std::function<void(bool)> onStateChanged;
    
private:
    bool isOn = false;
    float toggleAnimation = 0.0f;
    
    void animateToggle();
    void timerCallback();
    
    class ToggleAnimator;
    std::unique_ptr<ToggleAnimator> animator;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PulseToggle)
};

//==============================================================================
// PulseDropdown - 3-layer shadow with gradient
class PulseDropdown : public PulseComponent
{
public:
    PulseDropdown(const juce::String& name);
    ~PulseDropdown() override = default;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    
    void addItem(const juce::String& item) { items.add(item); }
    void setSelectedIndex(int index);
    int getSelectedIndex() const { return selectedIndex; }
    
    std::function<void(int)> onSelectionChanged;
    
private:
    juce::StringArray items;
    int selectedIndex = 0;
    bool isOpen = false;
    
    void showPopup();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PulseDropdown)
};

} // namespace HAM