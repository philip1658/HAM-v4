cmake_minimum_required(VERSION 3.22)

# Project definition
project(HAM VERSION 0.1.0)

# C++ Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# macOS specific settings
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build universal binary")

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Options
option(HAM_BUILD_TESTS "Build unit tests" ON)
option(DEBUG_MIDI_MONITOR "Enable MIDI debug monitor on channel 16" OFF)

# Download JUCE if not found
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE")
    message(STATUS "JUCE not found. Downloading JUCE 8.0.4...")
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.4
        GIT_SHALLOW    TRUE
        GIT_PROGRESS   TRUE
    )
    FetchContent_MakeAvailable(JUCE)
else()
    add_subdirectory(JUCE)
endif()

# Define the app target
juce_add_gui_app(HAM
    PRODUCT_NAME "HAM"
    COMPANY_NAME "Philip Krieger"
    BUNDLE_ID "com.philipkrieger.ham"
    VERSION "0.1.0"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    PLUGINHOST_AU TRUE
    MICROPHONE_PERMISSION_ENABLED FALSE
    CAMERA_PERMISSION_ENABLED FALSE
)

# Generate JUCE header
juce_generate_juce_header(HAM)

# Source files - Phase 1.2 Domain Models
target_sources(HAM PRIVATE
    Source/Main.cpp
    Source/MainComponent.cpp
    Source/MainComponent.h
    
    # Domain Models
    Source/Domain/Models/Stage.h
    Source/Domain/Models/Stage.cpp
    Source/Domain/Models/Track.h
    Source/Domain/Models/Track.cpp
    Source/Domain/Models/Pattern.h
    Source/Domain/Models/Pattern.cpp
    Source/Domain/Models/Scale.h
    Source/Domain/Models/Scale.cpp
    
    # Clock System
    Source/Domain/Clock/MasterClock.h
    Source/Domain/Clock/MasterClock.cpp
    Source/Domain/Clock/AsyncPatternEngine.h
    Source/Domain/Clock/AsyncPatternEngine.cpp
    
    # Transport & Sync
    Source/Domain/Transport/Transport.h
    Source/Domain/Transport/Transport.cpp
    Source/Domain/Transport/SyncManager.h
    Source/Domain/Transport/SyncManager.cpp
    
    # Engine - Voice Management
    Source/Engine/Voice/VoiceManager.h
    Source/Engine/Voice/VoiceManager.cpp
    
    # Engines - Sequencer
    Source/Domain/Engines/SequencerEngine.h
    Source/Domain/Engines/SequencerEngine.cpp
    
    # Services - MIDI Router
    Source/Domain/Services/MidiRouter.h
    Source/Domain/Services/MidiRouter.cpp
    
    # Services - Channel Manager
    Source/Domain/Services/ChannelManager.h
    Source/Domain/Services/ChannelManager.cpp
    
    # Engines - Gate Engine
    Source/Domain/Engines/GateEngine.h
    Source/Domain/Engines/GateEngine.cpp
    
    # Engines - Pitch Engine
    Source/Domain/Engines/PitchEngine.h
    Source/Domain/Engines/PitchEngine.cpp
    
    # Engines - Accumulator Engine
    Source/Domain/Engines/AccumulatorEngine.h
    Source/Domain/Engines/AccumulatorEngine.cpp
    
    # UI Components
    Source/UI/ComponentShowcase.h
    Source/UI/ComponentShowcase.cpp
    Source/UI/PulseComponentLibrary.h
    Source/UI/PulseComponentLibrary.cpp
    
    # Presentation Layer - Core
    Source/Presentation/Core/DesignSystem.h
    Source/Presentation/Core/BaseComponent.h
    
    # Presentation Layer - ViewModels
    Source/Presentation/ViewModels/StageViewModel.h
    Source/Presentation/ViewModels/TrackViewModel.h
    Source/Presentation/ViewModels/TransportViewModel.h
    
    # Presentation Layer - Components
    
    # Presentation Layer - Views
)

# Compile definitions
target_compile_definitions(HAM PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_MODAL_LOOPS_PERMITTED=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
    JUCE_PLUGINHOST_VST3=1
    JUCE_PLUGINHOST_AU=1
    $<$<BOOL:${DEBUG_MIDI_MONITOR}>:DEBUG_MIDI_MONITOR=1>
)

# Include directories
target_include_directories(HAM PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Source
)

# Link with JUCE modules
target_link_libraries(HAM PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_dsp
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

# Platform-specific settings
if(APPLE)
    target_link_libraries(HAM PRIVATE
        "-framework CoreAudio"
        "-framework CoreMIDI"
        "-framework AudioToolbox"
        "-framework Accelerate"
    )
endif()

# Copy app to Desktop after build
if(APPLE)
    add_custom_command(TARGET HAM POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E remove_directory 
            "$ENV{HOME}/Desktop/HAM.app"
        COMMAND ${CMAKE_COMMAND} -E copy_directory 
            "$<TARGET_BUNDLE_DIR:HAM>" 
            "$ENV{HOME}/Desktop/HAM.app"
        COMMENT "Copying HAM.app to Desktop..."
    )
endif()

# Tests
if(HAM_BUILD_TESTS)
    enable_testing()
    add_subdirectory(Tests)
endif()

# Print build info
message(STATUS "")
message(STATUS "HAM Build Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  macOS Target: ${CMAKE_OSX_DEPLOYMENT_TARGET}")
message(STATUS "  Architectures: ${CMAKE_OSX_ARCHITECTURES}")
message(STATUS "  MIDI Monitor: ${DEBUG_MIDI_MONITOR}")
message(STATUS "  Build Tests: ${HAM_BUILD_TESTS}")
message(STATUS "")