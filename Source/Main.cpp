/*
  ==============================================================================

    HAM - Hardware Accumulator Mode Sequencer
    Copyright (c) 2024 Philip Krieger

  ==============================================================================
*/

#include <JuceHeader.h>
#include "MainComponent.h"

//==============================================================================
class HAMApplication : public juce::JUCEApplication
{
public:
    //==============================================================================
    HAMApplication() {}

    const juce::String getApplicationName() override       { return ProjectInfo::projectName; }
    const juce::String getApplicationVersion() override    { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed() override             { return false; }

    //==============================================================================
    void initialise(const juce::String& commandLine) override
    {
        // Handle command line arguments for testing
        if (commandLine.contains("--version"))
        {
            std::cout << "HAM Version " << ProjectInfo::versionString << std::endl;
            quit();
            return;
        }
        
        if (commandLine.contains("--test-mode"))
        {
            // Enable test mode flags
            m_testMode = true;
        }
        
        // Create main window
        mainWindow.reset(new MainWindow(getApplicationName()));
    }

    void shutdown() override
    {
        mainWindow = nullptr;
    }

    //==============================================================================
    void systemRequestedQuit() override
    {
        quit();
    }

    void anotherInstanceStarted(const juce::String& commandLine) override
    {
        // Focus the existing window if another instance tries to start
        if (mainWindow != nullptr)
            mainWindow->toFront(true);
    }

    //==============================================================================
    class MainWindow : public juce::DocumentWindow
    {
    public:
        MainWindow(juce::String name)
            : DocumentWindow(name,
                           juce::Colour(0xFF0A0A0A), // Dark background
                           DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar(true);
            setContentOwned(new MainComponent(), true);

           #if JUCE_IOS || JUCE_ANDROID
            setFullScreen(true);
           #else
            // Default window size
            setResizable(true, true);
            centreWithSize(1400, 900);
            
            // Minimum size
            setResizeLimits(1024, 768, 2560, 1600);
           #endif

            setVisible(true);
        }

        void closeButtonPressed() override
        {
            JUCEApplication::getInstance()->systemRequestedQuit();
        }

    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
    };

private:
    std::unique_ptr<MainWindow> mainWindow;
    bool m_testMode = false;
};

//==============================================================================
// This macro generates the main() routine that launches the app
START_JUCE_APPLICATION(HAMApplication)