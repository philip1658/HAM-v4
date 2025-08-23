/*
  ==============================================================================

    MainComponent.h
    Main component for HAM sequencer - Refactored to use modular architecture

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <memory>

namespace HAM {
namespace UI {
    class MainWindow;
    class AppController;
    class UICoordinator;
}
}

//==============================================================================
/**
 * Main component for HAM sequencer
 * Now acts as a thin coordinator between MainWindow, AppController, and UICoordinator
 * All business logic moved to AppController
 * All UI orchestration moved to UICoordinator
 * All window management moved to MainWindow
 */
class MainComponent : public juce::Component
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    //==============================================================================
    void paint(juce::Graphics&) override;
    void paintOverChildren(juce::Graphics&) override;
    void resized() override;
    
    bool keyPressed(const juce::KeyPress& key) override;

private:
    //==============================================================================
    // Core modules (refactored from monolithic Impl)
    std::unique_ptr<HAM::UI::MainWindow> m_mainWindow;
    std::unique_ptr<HAM::UI::AppController> m_appController;
    std::unique_ptr<HAM::UI::UICoordinator> m_uiCoordinator;
    
    // Look and feel
    juce::LookAndFeel_V4 m_pulseLookAndFeel;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};