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
option(HAM_BUILD_BENCHMARKS "Build performance benchmarks" ON)
option(DEBUG_MIDI_MONITOR "Enable MIDI debug monitor on channel 16" OFF)
option(HAM_ENABLE_COVERAGE "Enable code coverage" OFF)
option(HAM_ENABLE_PERFORMANCE_MONITORING "Enable performance monitoring in builds" OFF)

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
    PRODUCT_NAME "CloneHAM"
    COMPANY_NAME "Philip Krieger"
    BUNDLE_ID "com.philipkrieger.cloneham"
    VERSION "0.1.0"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    PLUGINHOST_AU TRUE        # Enable AudioUnit hosting on macOS
    PLUGINHOST_VST3 TRUE       # Enable VST3 hosting
    PLUGINHOST_VST FALSE       # VST2 is deprecated
    PLUGINHOST_LV2 FALSE       # LV2 not needed for now
    PLUGINHOST_ARA FALSE       # ARA not needed
    MICROPHONE_PERMISSION_ENABLED FALSE
    CAMERA_PERMISSION_ENABLED FALSE
    HARDENED_RUNTIME_ENABLED TRUE
    HARDENED_RUNTIME_OPTIONS "com.apple.security.device.audio-input"
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
    Source/Domain/Models/ScaleSlotManager.h
    Source/Domain/Models/ScaleSlotManager.cpp
    
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
    Source/Domain/Engines/VoiceManager.h
    Source/Domain/Engines/VoiceManager.cpp
    
    # Engines - Sequencer
    Source/Domain/Engines/SequencerEngine.h
    Source/Domain/Engines/SequencerEngine.cpp
    
    # Services - MIDI Router
    Source/Domain/Services/MidiRouter.h
    Source/Domain/Services/MidiRouter.cpp
    
    # Services - Channel Manager
    Source/Domain/Services/ChannelManager.h
    Source/Domain/Services/ChannelManager.cpp
    
    # Services - Track Manager
    Source/Domain/Services/TrackManager.h
    Source/Domain/Services/TrackManager.cpp
    
    # Services - Preset Manager
    Source/Domain/Services/PresetManager.h
    Source/Domain/Services/PresetManager.cpp
    
    # Engines - Gate Engine
    Source/Domain/Engines/GateEngine.h
    Source/Domain/Engines/GateEngine.cpp
    
    # Engines - Pitch Engine
    Source/Domain/Engines/PitchEngine.h
    Source/Domain/Engines/PitchEngine.cpp
    
    # Engines - Accumulator Engine
    Source/Domain/Engines/AccumulatorEngine.h
    Source/Domain/Engines/AccumulatorEngine.cpp
    
    # Presentation Layer - Core
    Source/Presentation/Core/DesignSystem.h
    Source/Presentation/Core/BaseComponent.h
    Source/Presentation/Core/ComponentBase.h
    Source/Presentation/Core/ComponentBase.cpp
    Source/Presentation/Core/MainWindow.h
    Source/Presentation/Core/MainWindow.cpp
    Source/Presentation/Core/AppController.h
    Source/Presentation/Core/AppController.cpp
    Source/Presentation/Core/UICoordinator.h
    Source/Presentation/Core/UICoordinator.cpp
    
    # Presentation Layer - ViewModels
    Source/Presentation/ViewModels/StageViewModel.h
    Source/Presentation/ViewModels/TrackViewModel.h
    Source/Presentation/ViewModels/TransportViewModel.h
    
    # Presentation Layer - Components
    
    # Presentation Layer - Views
    Source/Presentation/Views/TrackSidebar.h
    Source/Presentation/Views/TrackSidebar.cpp
    Source/Presentation/Views/StageGrid.h
    Source/Presentation/Views/StageGrid.cpp
    Source/Presentation/Views/MainEditor.h
    Source/Presentation/Views/PluginBrowser.h
    Source/Presentation/Views/MixerView.h
    Source/Presentation/Views/MixerView.cpp
    Source/Presentation/Views/ScaleSlotSelector.h
    Source/Presentation/Views/ScaleSlotSelector.cpp
    Source/Presentation/Views/PulseScaleSwitcher.h
    Source/Presentation/Views/PulseScaleSwitcher.cpp
    Source/Presentation/Views/ScaleBrowser.h
    Source/Presentation/Views/ScaleBrowser.cpp
    
    # Presentation Layer - ViewModels
    Source/Presentation/ViewModels/ScaleSlotViewModel.h
    Source/Presentation/ViewModels/ScaleSlotViewModel.cpp
    
    # UI Components - New Modular Architecture
    Source/UI/ComponentBase.h
    Source/UI/ComponentBase.cpp
    Source/UI/BasicComponents.h
    Source/UI/BasicComponents.cpp
    Source/UI/AdvancedComponents.h
    Source/UI/AdvancedComponents.cpp
    Source/UI/LayoutComponents.h
    Source/UI/LayoutComponents.cpp
    
    # Infrastructure Layer - Audio
    Source/Infrastructure/Audio/HAMAudioProcessor.h
    Source/Infrastructure/Audio/HAMAudioProcessor.cpp
    
    # Infrastructure Layer - Messaging
    Source/Infrastructure/Messaging/LockFreeMessageQueue.h
    Source/Infrastructure/Messaging/MessageDispatcher.h
    Source/Infrastructure/Messaging/MessageTypes.h

    # Infrastructure Layer - Plugins
    Source/Infrastructure/Plugins/PluginManager.h
    Source/Infrastructure/Plugins/PluginManager.cpp
    Source/Infrastructure/Plugins/PluginSandbox.h
    Source/Infrastructure/Plugins/PluginSandbox.cpp
    
    # Domain Layer - Processors
    Source/Domain/Processors/TrackProcessor.h
    Source/Domain/Processors/TrackProcessor.cpp
    Source/Domain/Processors/MidiEventGenerator.h
    Source/Domain/Processors/MidiEventGenerator.cpp
    Source/Domain/Processors/PatternScheduler.h
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

# Code coverage configuration
if(HAM_ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
        # Coverage flags for main target
        target_compile_options(HAM PRIVATE 
            --coverage 
            -fprofile-arcs 
            -ftest-coverage
            -fprofile-instr-generate
            -fcoverage-mapping
        )
        target_link_options(HAM PRIVATE --coverage)
        
        # Coverage will be added to plugin tools after they're defined
        
        # Find coverage tools
        find_program(LCOV lcov PATHS /opt/homebrew/bin /usr/local/bin)
        find_program(GENHTML genhtml PATHS /opt/homebrew/bin /usr/local/bin)
        find_program(GCOVR gcovr PATHS /opt/homebrew/bin /usr/local/bin)
        
        if(LCOV AND GENHTML)
            add_custom_target(coverage
                COMMAND ${LCOV} --directory . --capture --output-file coverage.info
                COMMAND ${LCOV} --remove coverage.info '/usr/*' '*/JUCE/*' '*/Tests/*' '*/Tools/*' '*/build*/*' --output-file coverage.info
                COMMAND ${GENHTML} coverage.info --output-directory coverage-report --title "HAM Coverage" --show-details --legend
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMENT "Generating lcov HTML coverage report..."
            )
            
            add_custom_target(coverage-clean
                COMMAND ${LCOV} --directory . --zerocounters
                COMMAND rm -rf coverage.info coverage-report *.gcda *.gcno
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMENT "Cleaning coverage data..."
            )
        else()
            message(WARNING "lcov/genhtml not found. HTML coverage reports will not be available.")
        endif()
        
        if(GCOVR)
            add_custom_target(coverage-xml
                COMMAND ${GCOVR} --xml-pretty --exclude-unreachable-branches 
                    --filter ../Source/ --exclude ../Source/Tests/ --exclude ../Source/Tools/
                    --output coverage.xml
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMENT "Generating gcovr XML coverage report..."
            )
            
            add_custom_target(coverage-json
                COMMAND ${GCOVR} --json-pretty --exclude-unreachable-branches
                    --filter ../Source/ --exclude ../Source/Tests/ --exclude ../Source/Tools/
                    --output coverage.json
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMENT "Generating gcovr JSON coverage report..."
            )
        else()
            message(WARNING "gcovr not found. XML/JSON coverage reports will not be available.")
        endif()
        
        # Combined coverage target
        add_custom_target(coverage-all
            DEPENDS coverage coverage-xml coverage-json
            COMMENT "Generating all coverage reports..."
        )
        
        message(STATUS "Code coverage enabled. Use 'make coverage-all' to generate reports.")
    else()
        message(WARNING "Code coverage requires GCC or Clang compiler.")
    endif()
endif()

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
    # juce::juce_recommended_warning_flags # Temporarily disabled for build
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

# Sandbox Scanner Worker (out-of-process plugin scanner)
juce_add_console_app(PluginScanWorker
    PRODUCT_NAME "PluginScanWorker"
)

juce_generate_juce_header(PluginScanWorker)

target_sources(PluginScanWorker PRIVATE
    Tools/PluginScanner/PluginScanWorkerMain.cpp
)

target_compile_definitions(PluginScanWorker PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_MODAL_LOOPS_PERMITTED=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
    JUCE_PLUGINHOST_VST3=1
    JUCE_PLUGINHOST_AU=1
)

target_link_libraries(PluginScanWorker PRIVATE
    juce::juce_core
    juce::juce_events
    juce::juce_audio_formats
    juce::juce_audio_processors
)

if(APPLE)
    target_link_libraries(PluginScanWorker PRIVATE
        "-framework CoreAudio"
        "-framework CoreMIDI"
        "-framework AudioToolbox"
        "-framework AudioUnit"
        "-framework CoreAudioKit"
        "-framework Accelerate"
    )
endif()

# Add coverage to PluginScanWorker if enabled
if(HAM_ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_compile_options(PluginScanWorker PRIVATE --coverage -fprofile-arcs -ftest-coverage)
    target_link_options(PluginScanWorker PRIVATE --coverage)
endif()

# Plugin Probe Worker (out-of-process plugin instantiation probe)
juce_add_console_app(PluginProbeWorker
    PRODUCT_NAME "PluginProbeWorker"
)

juce_generate_juce_header(PluginProbeWorker)

target_sources(PluginProbeWorker PRIVATE
    Tools/PluginScanner/PluginProbeWorkerMain.cpp
)

target_compile_definitions(PluginProbeWorker PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_MODAL_LOOPS_PERMITTED=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
    JUCE_PLUGINHOST_VST3=1
    JUCE_PLUGINHOST_AU=1
)

target_link_libraries(PluginProbeWorker PRIVATE
    juce::juce_core
    juce::juce_events
    juce::juce_audio_formats
    juce::juce_audio_processors
)

if(APPLE)
    target_link_libraries(PluginProbeWorker PRIVATE
        "-framework CoreAudio"
        "-framework CoreMIDI"
        "-framework AudioToolbox"
        "-framework AudioUnit"
        "-framework CoreAudioKit"
        "-framework Accelerate"
    )
endif()

# Add coverage to PluginProbeWorker if enabled  
if(HAM_ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_compile_options(PluginProbeWorker PRIVATE --coverage -fprofile-arcs -ftest-coverage)
    target_link_options(PluginProbeWorker PRIVATE --coverage)
endif()

# Plugin Host Bridge (out-of-process plugin UI/hosting)
juce_add_gui_app(PluginHostBridge
    PRODUCT_NAME "PluginHostBridge"
)

juce_generate_juce_header(PluginHostBridge)

target_sources(PluginHostBridge PRIVATE
    Tools/PluginScanner/PluginHostBridgeMain.cpp
)

target_compile_definitions(PluginHostBridge PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_MODAL_LOOPS_PERMITTED=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
    JUCE_PLUGINHOST_VST3=1
    JUCE_PLUGINHOST_AU=1
)

target_link_libraries(PluginHostBridge PRIVATE
    juce::juce_core
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_gui_extra
    juce::juce_audio_utils
)

if(APPLE)
    target_link_libraries(PluginHostBridge PRIVATE
        "-framework CoreAudio"
        "-framework CoreMIDI"
        "-framework AudioToolbox"
        "-framework AudioUnit"
        "-framework CoreAudioKit"
        "-framework Accelerate"
    )
endif()

# Plugin Sandbox Host (out-of-process plugin host for crash protection)
juce_add_console_app(PluginSandboxHost
    PRODUCT_NAME "PluginSandboxHost"
)

juce_generate_juce_header(PluginSandboxHost)

target_sources(PluginSandboxHost PRIVATE
    Tools/PluginSandboxHost/PluginSandboxHostMain.cpp
    Source/Infrastructure/Plugins/PluginSandbox.cpp
    Source/Infrastructure/Plugins/PluginSandbox.h
)

target_compile_definitions(PluginSandboxHost PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_MODAL_LOOPS_PERMITTED=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
    JUCE_PLUGINHOST_VST3=1
    JUCE_PLUGINHOST_AU=1
)

target_link_libraries(PluginSandboxHost PRIVATE
    juce::juce_core
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_processors
    juce::juce_audio_formats
    juce::juce_events
    juce::juce_recommended_config_flags
)

# Platform-specific linking for sandboxed host
if(APPLE)
    target_link_libraries(PluginSandboxHost PRIVATE
        "-framework CoreAudio"
        "-framework CoreMIDI"
        "-framework AudioToolbox"
        "-framework AudioUnit"
        "-framework CoreAudioKit"
        "-framework Accelerate"
    )
endif()

target_include_directories(PluginSandboxHost PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Note: No external copy (Desktop) to keep all artifacts inside clone folder

# Tests
if(HAM_BUILD_TESTS)
    enable_testing()
    add_subdirectory(Tests)
endif()

# Print build info
message(STATUS "")
message(STATUS "CloneHAM Build Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  macOS Target: ${CMAKE_OSX_DEPLOYMENT_TARGET}")
message(STATUS "  Architectures: ${CMAKE_OSX_ARCHITECTURES}")
message(STATUS "  MIDI Monitor: ${DEBUG_MIDI_MONITOR}")
message(STATUS "  Build Tests: ${HAM_BUILD_TESTS}")
message(STATUS "")
# Plugin System Components (if not already added)
set(PLUGIN_SOURCES
    Source/Infrastructure/Plugins/PluginWindowManager.h
    Source/Presentation/Views/MixerView.h
)

# Add to target sources
target_sources(HAM PRIVATE ${PLUGIN_SOURCES})
