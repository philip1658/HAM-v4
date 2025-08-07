#include "MainWindow.h"
#include "ComponentGallery.h"

MainWindow::MainWindow(const juce::String& name)
    : DocumentWindow(name,
                    juce::Desktop::getInstance().getDefaultLookAndFeel()
                        .findColour(ResizableWindow::backgroundColourId),
                    DocumentWindow::allButtons)
{
    setUsingNativeTitleBar(true);
    
    // Create the component gallery
    gallery = std::make_unique<ComponentGallery>();
    setContentOwned(gallery.release(), true);
    
    // Set window properties
    setResizable(true, true);
    centreWithSize(1600, 1000);
    setVisible(true);
    
    // Dark theme
    getLookAndFeel().setColour(ResizableWindow::backgroundColourId, juce::Colour(0xFF000000));
}

MainWindow::~MainWindow()
{
}

void MainWindow::closeButtonPressed()
{
    juce::JUCEApplication::getInstance()->systemRequestedQuit();
}