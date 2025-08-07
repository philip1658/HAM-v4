cmake_minimum_required(VERSION 3.22)

# UI Designer Tool - Separate from main HAM app
project(HAMUIDesigner VERSION 1.0.0)

# C++ Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# macOS specific settings
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build universal binary")

# Add JUCE from build directory
add_subdirectory(../../build/_deps/juce-src ${CMAKE_CURRENT_BINARY_DIR}/JUCE)

# Define the UI Designer app
juce_add_gui_app(HAMUIDesigner
    PRODUCT_NAME "HAM UI Designer"
    COMPANY_NAME "Philip Krieger"
    BUNDLE_ID "com.philipkrieger.ham.uidesigner"
    VERSION "1.0.0"
)

# Generate JUCE header
juce_generate_juce_header(HAMUIDesigner)

# Source files
target_sources(HAMUIDesigner PRIVATE
    Source/Main.cpp
    Source/MainWindow.cpp
    Source/MainWindow.h
    Source/ComponentGallery.cpp
    Source/ComponentGallery.h
    
    # Reuse the Pulse Component Library
    ../../Source/UI/PulseComponentLibrary.cpp
    ../../Source/UI/PulseComponentLibrary.h
)

# Compile definitions
target_compile_definitions(HAMUIDesigner PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_MODAL_LOOPS_PERMITTED=1
    UI_DESIGNER_MODE=1
)

# Include directories
target_include_directories(HAMUIDesigner PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Source
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Source
)

# Link with JUCE modules
target_link_libraries(HAMUIDesigner PRIVATE
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

# Copy to Desktop after build
add_custom_command(TARGET HAMUIDesigner POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "$<TARGET_BUNDLE_DIR:HAMUIDesigner>"
    "$ENV{HOME}/Desktop/HAM UI Designer.app"
    COMMENT "Copying HAM UI Designer to Desktop..."
)