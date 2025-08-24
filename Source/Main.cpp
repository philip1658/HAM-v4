/*
  ==============================================================================

    HAM - Hardware Accumulator Mode Sequencer
    Copyright (c) 2024 Philip Krieger

  ==============================================================================
*/

#include <JuceHeader.h>
#include "MainComponent.h"
#include "Infrastructure/Plugins/PluginManager.h"

//==============================================================================
class HAMApplication : public juce::JUCEApplication
{
public:
    //==============================================================================
    HAMApplication() {}

    const juce::String getApplicationName() override       { return "HAM-Happy Accident Machine"; }
    const juce::String getApplicationVersion() override    { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed() override             { return false; }

    //==============================================================================
    void initialise(const juce::String& commandLine) override
    {
        // Handle command line arguments for testing
        if (commandLine.contains("--version"))
        {
            std::cout << getApplicationName() << " Version " << ProjectInfo::versionString << std::endl;
            quit();
            return;
        }
        
        if (commandLine.contains("--test-mode"))
        {
            // Enable test mode flags
            m_testMode = true;
        }
        
        // Pre-Start: Plugin Scan Splash, dann MainWindow
        showSplashAndScan();
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

    // Einfacher Splash mit Fortschritt
    class PluginScanSplash : public juce::Component, private juce::Timer
    {
    public:
        using FinishedCallback = std::function<void()>;

        PluginScanSplash()
        {
            startTimerHz(20);
        }

        void setOnFinished(FinishedCallback cb) { onFinished = std::move(cb); }

        void paint(juce::Graphics& g) override
        {
            g.fillAll(juce::Colour(0xFF0A0A0A));

            auto bounds = getLocalBounds().reduced(40);
            auto title = juce::Font(juce::FontOptions(28.0f)).withStyle(juce::Font::bold);
            g.setColour(juce::Colours::white);
            g.setFont(title);
            g.drawFittedText("HAM-Happy Accident Machine", bounds.removeFromTop(60), juce::Justification::centred, 1);

            bounds.removeFromTop(20);

            auto progress = HAM::PluginManager::instance().getProgress();
            auto scanning = HAM::PluginManager::instance().isScanning();

            auto barArea = bounds.removeFromTop(24);
            barArea = barArea.withSizeKeepingCentre(juce::jmin(600, getWidth() - 80), 16);
            g.setColour(juce::Colours::white.withAlpha(0.08f));
            g.fillRoundedRectangle(barArea.toFloat(), 8.0f);

            if (progress.total > 0)
            {
                float p = juce::jlimit(0.0f, 1.0f, (float)progress.scanned / (float)juce::jmax(1, progress.total));
                auto fill = barArea.withWidth((int)std::round(p * (float)barArea.getWidth()));
                g.setColour(juce::Colours::skyblue.withAlpha(0.9f));
                g.fillRoundedRectangle(fill.toFloat(), 8.0f);
            }

            bounds.removeFromTop(12);
            g.setColour(juce::Colours::white.withAlpha(0.7f));
            g.setFont(juce::Font(juce::FontOptions(16.0f)));
            g.drawFittedText(scanning ? ("Scanning Plugins…  " + progress.current) : "Scan complete", bounds.removeFromTop(40), juce::Justification::centred, 1);
        }

    private:
        void timerCallback() override
        {
            if (!HAM::PluginManager::instance().isScanning())
            {
                stopTimer();
                // Zerstörung des Splash-Fensters NICHT direkt im Timer-Callback,
                // sondern asynchron, um Use-After-Free zu vermeiden.
                auto cb = onFinished;
                onFinished = nullptr;
                if (cb)
                {
                    juce::MessageManager::callAsync([cb]{ cb(); });
                }
                return;
            }
            repaint();
        }

        FinishedCallback onFinished;
    };

    class SplashWindow : public juce::DocumentWindow
    {
    public:
        SplashWindow()
        : juce::DocumentWindow("Starting…", juce::Colour(0xFF0A0A0A), DocumentWindow::closeButton)
        {
            setUsingNativeTitleBar(true);
            auto* content = new PluginScanSplash();
            setContentOwned(content, true);
            centreWithSize(720, 240);
            setResizable(false, false);
            setVisible(true);
        }

        void closeButtonPressed() override { }
    };

    void showSplashAndScan()
    {
        // Splash anzeigen
        splashWindow.reset(new SplashWindow());
        auto* splashContent = dynamic_cast<PluginScanSplash*>(splashWindow->getContentComponent());
        if (splashContent)
        {
            splashContent->setOnFinished([this]
            {
                // Verzögerte Schließung, um ausstehende Paint-/Timer-Events sauber ablaufen zu lassen
                juce::Timer::callAfterDelay(150, [this]
                {
                    splashWindow = nullptr;
                    mainWindow.reset(new MainWindow(getApplicationName()));
                });
            });
        }

        // Sandbox-Scan (out-of-process) starten; Splash pollt den Status
        auto& pm = HAM::PluginManager::instance();
        pm.initialise();
        pm.startSandboxedScan(false);
    }

private:
    std::unique_ptr<MainWindow> mainWindow;
    std::unique_ptr<SplashWindow> splashWindow;
    bool m_testMode = false;
};

//==============================================================================
// This macro generates the main() routine that launches the app
START_JUCE_APPLICATION(HAMApplication)