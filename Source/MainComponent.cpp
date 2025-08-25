/*
  ==============================================================================

    MainComponent.cpp
    Main component for HAM sequencer - Refactored to use modular architecture

  ==============================================================================
*/

#include "MainComponent.h"
#include "Presentation/Core/MainWindow.h"
#include "Presentation/Core/AppController.h"
#include "Presentation/Core/UICoordinator.h"

//==============================================================================
MainComponent::MainComponent()
{
    // Set up custom look and feel
    setLookAndFeel(&m_pulseLookAndFeel);
    
    // Create core modules in dependency order
    m_appController = std::make_unique<HAM::UI::AppController>();
    m_uiCoordinator = std::make_unique<HAM::UI::UICoordinator>(*m_appController);
    m_mainWindow = std::make_unique<HAM::UI::MainWindow>();
    
    // Connect the AudioProcessor from AppController to UICoordinator
    if (m_appController->getAudioProcessor())
    {
        m_uiCoordinator->setAudioProcessor(m_appController->getAudioProcessor());
    }
    
    // Add UI coordinator as the main content
    addAndMakeVisible(m_uiCoordinator.get());
    
    // Set up window callbacks
    m_mainWindow->onNewProject = [this]()
    {
        m_appController->newProject();
    };
    
    m_mainWindow->onOpenProject = [this]()
    {
        auto fileChooser = std::make_shared<juce::FileChooser>(
            "Open HAM Project", 
            juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
            "*.ham");
        
        fileChooser->launchAsync(juce::FileBrowserComponent::openMode | 
                                juce::FileBrowserComponent::canSelectFiles,
            [this, fileChooser](const juce::FileChooser& fc)
            {
                auto results = fc.getResults();
                if (results.size() > 0)
                {
                    m_appController->loadProject(results[0]);
                }
            });
    };
    
    m_mainWindow->onSaveProject = [this]()
    {
        // If we have a current file, save to it
        // Otherwise, prompt for save location
        m_mainWindow->onSaveProjectAs();
    };
    
    m_mainWindow->onSaveProjectAs = [this]()
    {
        auto fileChooser = std::make_shared<juce::FileChooser>(
            "Save HAM Project", 
            juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
            "*.ham");
        
        fileChooser->launchAsync(juce::FileBrowserComponent::saveMode | 
                                juce::FileBrowserComponent::canSelectFiles,
            [this, fileChooser](const juce::FileChooser& fc)
            {
                auto results = fc.getResults();
                if (results.size() > 0)
                {
                    m_appController->saveProject(results[0]);
                }
            });
    };
    
    m_mainWindow->onExportMidi = [this]()
    {
        auto fileChooser = std::make_shared<juce::FileChooser>(
            "Export MIDI", 
            juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
            "*.mid");
        
        fileChooser->launchAsync(juce::FileBrowserComponent::saveMode | 
                                juce::FileBrowserComponent::canSelectFiles,
            [fileChooser](const juce::FileChooser& fc)
            {
                auto results = fc.getResults();
                if (results.size() > 0)
                {
                    // TODO: Implement MIDI export
                }
            });
    };
    
    m_mainWindow->onShowSettings = [this]()
    {
        m_uiCoordinator->showSettings();
    };
    
    m_mainWindow->onShowAbout = [this]()
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::InfoIcon,
            "About HAM",
            "Happy Accident Machine v4.0\n\n"
            "A revolutionary MIDI sequencer inspired by Intellijel Metropolix\n\n"
            "Built with JUCE 8.0.4 and modern C++20\n"
            "© 2025 Philip Krieger"
        );
    };
    
    // Set initial window properties
    setSize(1600, 1000);
    m_mainWindow->setWindowTitle("HAM - Happy Accident Machine");
}

MainComponent::~MainComponent()
{
    setLookAndFeel(nullptr);
}

//==============================================================================
void MainComponent::paint(juce::Graphics& g)
{
    // Background is handled by UICoordinator
    g.fillAll(juce::Colour(0xFF0A0A0A));
}

void MainComponent::paintOverChildren(juce::Graphics& g)
{
    // Optional: Add any overlay effects here
}

void MainComponent::resized()
{
    // UICoordinator fills the entire component
    if (m_uiCoordinator)
    {
        m_uiCoordinator->setBounds(getLocalBounds());
    }
}

bool MainComponent::keyPressed(const juce::KeyPress& key)
{
    // First let MainWindow handle global shortcuts
    if (m_mainWindow && m_mainWindow->handleKeyPress(key))
        return true;
    
    // Then let UICoordinator handle view-specific shortcuts
    if (m_uiCoordinator && m_uiCoordinator->keyPressed(key))
        return true;
    
    return false;
}