#!/bin/bash

# Build MIDI Test Tool for HAM
echo "Building HAM MIDI Test Tool..."

# Create build directory
mkdir -p build_test
cd build_test

# Create temporary CMakeLists.txt for test
cat > CMakeLists.txt << 'EOF'
cmake_minimum_required(VERSION 3.22)
project(HAMMidiTest)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find JUCE
find_package(PkgConfig REQUIRED)

# Add JUCE modules path
list(APPEND CMAKE_MODULE_PATH "/Users/philipkrieger/JUCE/extras/CMake")
find_package(JUCE CONFIG REQUIRED)

# Create executable
juce_add_console_app(HAMMidiTest 
    PRODUCT_NAME "HAM MIDI Test")

target_sources(HAMMidiTest PRIVATE
    ../test_midi_mono_mode.cpp)

target_compile_definitions(HAMMidiTest PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_STANDALONE_APPLICATION=1)

target_link_libraries(HAMMidiTest PRIVATE
    juce::juce_core
    juce::juce_events  
    juce::juce_audio_basics
    juce::juce_audio_devices)

# macOS specific settings
if(APPLE)
    target_link_libraries(HAMMidiTest PRIVATE
        "-framework CoreMIDI"
        "-framework CoreAudio" 
        "-framework AudioToolbox")
endif()
EOF

# Build with CMake
echo "Configuring CMake..."
/opt/homebrew/bin/cmake . -DCMAKE_BUILD_TYPE=Release

echo "Building..."
/opt/homebrew/bin/cmake --build . --config Release

if [ -f "./HAMMidiTest_artefacts/Release/HAMMidiTest" ]; then
    echo "✓ Build successful!"
    echo "Executable: ./HAMMidiTest_artefacts/Release/HAMMidiTest"
    
    # Copy to desktop for easy access
    cp "./HAMMidiTest_artefacts/Release/HAMMidiTest" "/Users/philipkrieger/Desktop/HAM_MIDI_Test"
    echo "✓ Copied to Desktop as: HAM_MIDI_Test"
else
    echo "✗ Build failed!"
    exit 1
fi