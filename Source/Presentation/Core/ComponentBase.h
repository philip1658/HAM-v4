/*
  ==============================================================================

    ComponentBase.h
    Base component definitions for HAM UI

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

namespace HAM {
namespace UI {

//==============================================================================
/**
 * Base class for HAM UI components
 * Provides common functionality and styling
 */
class ComponentBase : public juce::Component
{
public:
    ComponentBase() = default;
    virtual ~ComponentBase() = default;
    
protected:
    // Common color palette
    static const juce::Colour BackgroundColor;
    static const juce::Colour PrimaryColor;
    static const juce::Colour SecondaryColor;
    static const juce::Colour AccentColor;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ComponentBase)
};

} // namespace UI
} // namespace HAM