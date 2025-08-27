/*
  ==============================================================================

    MainWindow.cpp
    Window management and menu bar handling for HAM sequencer

  ==============================================================================
*/

#include "MainWindow.h"

namespace HAM {
namespace UI {

//==============================================================================
MainWindow::MainWindow()
{
    createMenuBar();
    setSize(1600, 1000);
}

MainWindow::~MainWindow() = default;

//==============================================================================
void MainWindow::setWindowTitle(const juce::String& title)
{
    if (auto* topLevel = getTopLevelComponent())
    {
        if (auto* peer = topLevel->getPeer())
        {
            peer->setTitle(title);
        }
    }
}

void MainWindow::setWindowSize(int width, int height)
{
    setSize(width, height);
    if (auto* topLevel = getTopLevelComponent())
    {
        topLevel->centreWithSize(width, height);
    }
}

void MainWindow::centerOnScreen()
{
    if (auto* topLevel = getTopLevelComponent())
    {
        topLevel->centreWithSize(getWidth(), getHeight());
    }
}

//==============================================================================
bool MainWindow::handleKeyPress(const juce::KeyPress& key)
{
    // Global keyboard shortcuts
    const auto cmd = juce::ModifierKeys::commandModifier;
    const auto shift = juce::ModifierKeys::shiftModifier;
    
    if (key.getModifiers().isCommandDown())
    {
        if (key.getKeyCode() == 'N')
        {
            if (onNewProject) onNewProject();
            return true;
        }
        else if (key.getKeyCode() == 'O')
        {
            if (onOpenProject) onOpenProject();
            return true;
        }
        else if (key.getKeyCode() == 'S')
        {
            if (key.getModifiers().isShiftDown())
            {
                if (onSaveProjectAs) onSaveProjectAs();
            }
            else
            {
                if (onSaveProject) onSaveProject();
            }
            return true;
        }
        else if (key.getKeyCode() == 'E')
        {
            if (onExportMidi) onExportMidi();
            return true;
        }
        else if (key.getKeyCode() == ',')
        {
            if (onShowSettings) onShowSettings();
            return true;
        }
    }
    
    // Space bar for play/pause
    if (key.getKeyCode() == juce::KeyPress::spaceKey)
    {
        juce::Logger::writeToLog("MainWindow: Space key pressed - triggering play/stop");
        if (onTogglePlayStop)
        {
            onTogglePlayStop();
            return true;
        }
        return false;
    }
    
    return false;
}

//==============================================================================
void MainWindow::paint(juce::Graphics& g)
{
    // Window background
    g.fillAll(juce::Colour(0xFF0A0A0A));
}

void MainWindow::resized()
{
    auto bounds = getLocalBounds();
    
    if (m_menuBar)
    {
        m_menuBar->setBounds(bounds.removeFromTop(24));
    }
}

//==============================================================================
void MainWindow::createMenuBar()
{
    // TODO: Implement menu bar model
    // For now, we'll handle menus programmatically via keyboard shortcuts
}

void MainWindow::handleFileMenu(int menuItemID)
{
    switch (menuItemID)
    {
        case FileNew:
            if (onNewProject) onNewProject();
            break;
        case FileOpen:
            if (onOpenProject) onOpenProject();
            break;
        case FileSave:
            if (onSaveProject) onSaveProject();
            break;
        case FileSaveAs:
            if (onSaveProjectAs) onSaveProjectAs();
            break;
        case FileExportMidi:
            if (onExportMidi) onExportMidi();
            break;
        case FileExit:
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
            break;
    }
}

void MainWindow::handleEditMenu(int menuItemID)
{
    // TODO: Implement edit menu actions
}

void MainWindow::handleViewMenu(int menuItemID)
{
    switch (menuItemID)
    {
        case ViewFullscreen:
            m_isFullscreen = !m_isFullscreen;
            // TODO: Toggle fullscreen mode
            break;
        case ViewResetLayout:
            // TODO: Reset UI layout to defaults
            break;
        case ViewShowMidiMonitor:
            // TODO: Toggle MIDI monitor visibility
            break;
    }
}

void MainWindow::handleHelpMenu(int menuItemID)
{
    switch (menuItemID)
    {
        case HelpAbout:
            if (onShowAbout) onShowAbout();
            break;
        case HelpDocumentation:
            // TODO: Open documentation
            juce::URL("https://github.com/philip-kr/HAM/wiki").launchInDefaultBrowser();
            break;
    }
}

} // namespace UI
} // namespace HAM