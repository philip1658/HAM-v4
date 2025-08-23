/*
  ==============================================================================

    MainWindow.h
    Window management and menu bar handling for HAM sequencer

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <memory>

namespace HAM {
namespace UI {

//==============================================================================
/**
 * Manages the main application window, menu bar, and window-level events
 * Part of the refactored MainComponent architecture
 */
class MainWindow : public juce::Component
{
public:
    //==============================================================================
    MainWindow();
    ~MainWindow() override;

    //==============================================================================
    // Window management
    void setWindowTitle(const juce::String& title);
    void setWindowSize(int width, int height);
    void centerOnScreen();
    
    // Menu callbacks
    std::function<void()> onNewProject;
    std::function<void()> onOpenProject;
    std::function<void()> onSaveProject;
    std::function<void()> onSaveProjectAs;
    std::function<void()> onExportMidi;
    std::function<void()> onShowSettings;
    std::function<void()> onShowAbout;
    
    // Keyboard shortcuts
    bool handleKeyPress(const juce::KeyPress& key);
    
    //==============================================================================
    void paint(juce::Graphics&) override;
    void resized() override;
    
private:
    //==============================================================================
    void createMenuBar();
    void handleFileMenu(int menuItemID);
    void handleEditMenu(int menuItemID);
    void handleViewMenu(int menuItemID);
    void handleHelpMenu(int menuItemID);
    
    // Menu item IDs
    enum MenuItemIDs
    {
        FileNew = 1,
        FileOpen,
        FileSave,
        FileSaveAs,
        FileExportMidi,
        FileExit,
        
        EditUndo = 100,
        EditRedo,
        EditCut,
        EditCopy,
        EditPaste,
        EditSelectAll,
        
        ViewFullscreen = 200,
        ViewResetLayout,
        ViewShowMidiMonitor,
        
        HelpAbout = 300,
        HelpDocumentation
    };
    
    // Components
    std::unique_ptr<juce::MenuBarComponent> m_menuBar;
    std::unique_ptr<juce::MenuBarModel> m_menuModel;
    
    // Window state
    bool m_isFullscreen = false;
    juce::Rectangle<int> m_lastWindowBounds;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
};

} // namespace UI
} // namespace HAM