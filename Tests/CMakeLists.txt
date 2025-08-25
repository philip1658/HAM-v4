# Helper function to add coverage to test targets
function(add_coverage_to_target target_name)
    if(HAM_ENABLE_COVERAGE)
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
            target_compile_options(${target_name} PRIVATE --coverage -fprofile-arcs -ftest-coverage)
            target_link_options(${target_name} PRIVATE --coverage)
        endif()
    endif()
endfunction()

# Add new comprehensive test executables
# MasterClock Tests
juce_add_console_app(MasterClockTests
    PRODUCT_NAME "MasterClockTests"
)

juce_generate_juce_header(MasterClockTests)

target_sources(MasterClockTests PRIVATE
    MasterClockTests.cpp
    ../Source/Domain/Clock/MasterClock.cpp
    ../Source/Domain/Clock/MasterClock.h
)

target_compile_definitions(MasterClockTests PRIVATE
    JUCE_UNIT_TESTS=1
    JUCE_USE_CURL=0
    JUCE_MODAL_LOOPS_PERMITTED=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
)

target_include_directories(MasterClockTests PRIVATE
    ${CMAKE_SOURCE_DIR}/Source
)

target_link_libraries(MasterClockTests PRIVATE
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_audio_basics
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

add_coverage_to_target(MasterClockTests)
add_test(NAME MasterClockTests COMMAND MasterClockTests)

# AsyncPatternEngine Tests
juce_add_console_app(AsyncPatternEngineTests
    PRODUCT_NAME "AsyncPatternEngineTests"
)

juce_generate_juce_header(AsyncPatternEngineTests)

target_sources(AsyncPatternEngineTests PRIVATE
    AsyncPatternEngineTests.cpp
    ../Source/Domain/Clock/AsyncPatternEngine.cpp
    ../Source/Domain/Clock/AsyncPatternEngine.h
    ../Source/Domain/Clock/MasterClock.cpp
    ../Source/Domain/Clock/MasterClock.h
    ../Source/Domain/Models/Pattern.cpp
    ../Source/Domain/Models/Pattern.h
    ../Source/Domain/Models/Stage.cpp
    ../Source/Domain/Models/Stage.h
    ../Source/Domain/Models/Track.cpp
    ../Source/Domain/Models/Track.h
    ../Source/Domain/Models/Scale.cpp
    ../Source/Domain/Models/Scale.h
)

target_compile_definitions(AsyncPatternEngineTests PRIVATE
    JUCE_UNIT_TESTS=1
    JUCE_USE_CURL=0
    JUCE_MODAL_LOOPS_PERMITTED=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
)

target_include_directories(AsyncPatternEngineTests PRIVATE
    ${CMAKE_SOURCE_DIR}/Source
)

target_link_libraries(AsyncPatternEngineTests PRIVATE
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_audio_basics
    juce::juce_graphics
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

add_coverage_to_target(AsyncPatternEngineTests)
add_test(NAME AsyncPatternEngineTests COMMAND AsyncPatternEngineTests)

# MidiEventGenerator Tests
juce_add_console_app(MidiEventGeneratorTests
    PRODUCT_NAME "MidiEventGeneratorTests"
)

juce_generate_juce_header(MidiEventGeneratorTests)

target_sources(MidiEventGeneratorTests PRIVATE
    MidiEventGeneratorTests.cpp
    ../Source/Domain/Processors/MidiEventGenerator.cpp
    ../Source/Domain/Processors/MidiEventGenerator.h
    ../Source/Domain/Models/Stage.cpp
    ../Source/Domain/Models/Stage.h
    ../Source/Domain/Models/Track.cpp
    ../Source/Domain/Models/Track.h
    ../Source/Domain/Models/Pattern.cpp
    ../Source/Domain/Models/Pattern.h
    ../Source/Domain/Models/Scale.cpp
    ../Source/Domain/Models/Scale.h
    ../Source/Domain/Engines/GateEngine.cpp
    ../Source/Domain/Engines/GateEngine.h
    ../Source/Domain/Engines/PitchEngine.cpp
    ../Source/Domain/Engines/PitchEngine.h
)

target_compile_definitions(MidiEventGeneratorTests PRIVATE
    JUCE_UNIT_TESTS=1
    JUCE_USE_CURL=0
    JUCE_MODAL_LOOPS_PERMITTED=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
)

target_include_directories(MidiEventGeneratorTests PRIVATE
    ${CMAKE_SOURCE_DIR}/Source
)

target_link_libraries(MidiEventGeneratorTests PRIVATE
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_audio_basics
    juce::juce_graphics
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

add_coverage_to_target(MidiEventGeneratorTests)
add_test(NAME MidiEventGeneratorTests COMMAND MidiEventGeneratorTests)

# Pattern Tests
juce_add_console_app(PatternModelTests
    PRODUCT_NAME "PatternModelTests"
)

juce_generate_juce_header(PatternModelTests)

target_sources(PatternModelTests PRIVATE
    PatternTests.cpp
    ../Source/Domain/Models/Pattern.cpp
    ../Source/Domain/Models/Pattern.h
    ../Source/Domain/Models/Stage.cpp
    ../Source/Domain/Models/Stage.h
    ../Source/Domain/Models/Track.cpp
    ../Source/Domain/Models/Track.h
    ../Source/Domain/Models/Scale.cpp
    ../Source/Domain/Models/Scale.h
)

target_compile_definitions(PatternModelTests PRIVATE
    JUCE_UNIT_TESTS=1
    JUCE_USE_CURL=0
    JUCE_MODAL_LOOPS_PERMITTED=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
)

target_include_directories(PatternModelTests PRIVATE
    ${CMAKE_SOURCE_DIR}/Source
)

target_link_libraries(PatternModelTests PRIVATE
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_audio_basics
    juce::juce_graphics
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

add_coverage_to_target(PatternModelTests)
add_test(NAME PatternModelTests COMMAND PatternModelTests)

# TrackProcessor Tests
juce_add_console_app(TrackProcessorTests
    PRODUCT_NAME "TrackProcessorTests"
)

juce_generate_juce_header(TrackProcessorTests)

target_sources(TrackProcessorTests PRIVATE
    TrackProcessorTests.cpp
    ../Source/Domain/Processors/TrackProcessor.cpp
    ../Source/Domain/Processors/TrackProcessor.h
    ../Source/Domain/Models/Track.cpp
    ../Source/Domain/Models/Track.h
    ../Source/Domain/Models/Stage.cpp
    ../Source/Domain/Models/Stage.h
    ../Source/Domain/Models/Pattern.cpp
    ../Source/Domain/Models/Pattern.h
    ../Source/Domain/Models/Scale.cpp
    ../Source/Domain/Models/Scale.h
)

target_compile_definitions(TrackProcessorTests PRIVATE
    JUCE_UNIT_TESTS=1
    JUCE_USE_CURL=0
    JUCE_MODAL_LOOPS_PERMITTED=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
)

target_include_directories(TrackProcessorTests PRIVATE
    ${CMAKE_SOURCE_DIR}/Source
)

target_link_libraries(TrackProcessorTests PRIVATE
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_audio_basics
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

add_coverage_to_target(TrackProcessorTests)
add_test(NAME TrackProcessorTests COMMAND TrackProcessorTests)

# PatternScheduler Tests
juce_add_console_app(PatternSchedulerTests
    PRODUCT_NAME "PatternSchedulerTests"
)

juce_generate_juce_header(PatternSchedulerTests)

target_sources(PatternSchedulerTests PRIVATE
    PatternSchedulerTests.cpp
    ../Source/Domain/Processors/PatternScheduler.cpp
    ../Source/Domain/Processors/PatternScheduler.h
    ../Source/Domain/Models/Pattern.cpp
    ../Source/Domain/Models/Pattern.h
    ../Source/Domain/Models/Stage.cpp
    ../Source/Domain/Models/Stage.h
    ../Source/Domain/Models/Track.cpp
    ../Source/Domain/Models/Track.h
    ../Source/Domain/Models/Scale.cpp
    ../Source/Domain/Models/Scale.h
)

target_compile_definitions(PatternSchedulerTests PRIVATE
    JUCE_UNIT_TESTS=1
    JUCE_USE_CURL=0
    JUCE_MODAL_LOOPS_PERMITTED=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
)

target_include_directories(PatternSchedulerTests PRIVATE
    ${CMAKE_SOURCE_DIR}/Source
)

target_link_libraries(PatternSchedulerTests PRIVATE
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

add_coverage_to_target(PatternSchedulerTests)
add_test(NAME PatternSchedulerTests COMMAND PatternSchedulerTests)

# Domain Model Tests
juce_add_console_app(DomainModelTests
    PRODUCT_NAME "DomainModelTests"
)

# Generate JuceHeader for tests
juce_generate_juce_header(DomainModelTests)

target_sources(DomainModelTests PRIVATE
    DomainModelTests.cpp
    ../Source/Domain/Models/Stage.cpp
    ../Source/Domain/Models/Stage.h
    ../Source/Domain/Models/Track.cpp
    ../Source/Domain/Models/Track.h
    ../Source/Domain/Models/Pattern.cpp
    ../Source/Domain/Models/Pattern.h
    ../Source/Domain/Models/Scale.cpp
    ../Source/Domain/Models/Scale.h
)

target_compile_definitions(DomainModelTests PRIVATE
    JUCE_UNIT_TESTS=1
    JUCE_USE_CURL=0
    JUCE_MODAL_LOOPS_PERMITTED=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
)

target_include_directories(DomainModelTests PRIVATE
    ${CMAKE_SOURCE_DIR}/Source
)

target_link_libraries(DomainModelTests PRIVATE
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_audio_basics
    juce::juce_graphics
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

enable_testing()
add_coverage_to_target(DomainModelTests)
add_test(NAME DomainModelTests COMMAND DomainModelTests)

# Timing Tests
juce_add_console_app(TimingTests
    PRODUCT_NAME "TimingTests"
)

juce_generate_juce_header(TimingTests)

target_sources(TimingTests PRIVATE
    TimingTests.cpp
    ../Source/Domain/Clock/MasterClock.cpp
    ../Source/Domain/Clock/MasterClock.h
    ../Source/Domain/Clock/AsyncPatternEngine.cpp
    ../Source/Domain/Clock/AsyncPatternEngine.h
    ../Source/Domain/Models/Pattern.cpp
    ../Source/Domain/Models/Pattern.h
    ../Source/Domain/Models/Track.cpp
    ../Source/Domain/Models/Track.h
    ../Source/Domain/Models/Stage.cpp
    ../Source/Domain/Models/Stage.h
    ../Source/Domain/Models/Scale.cpp
    ../Source/Domain/Models/Scale.h
)

target_compile_definitions(TimingTests PRIVATE
    JUCE_UNIT_TESTS=1
    JUCE_USE_CURL=0
    JUCE_MODAL_LOOPS_PERMITTED=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
)

target_include_directories(TimingTests PRIVATE
    ${CMAKE_SOURCE_DIR}/Source
)

target_link_libraries(TimingTests PRIVATE
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_audio_basics
    juce::juce_graphics
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

add_coverage_to_target(TimingTests)
add_test(NAME TimingTests COMMAND TimingTests)

# Transport Tests
juce_add_console_app(TransportTests
    PRODUCT_NAME "TransportTests"
)

juce_generate_juce_header(TransportTests)

target_sources(TransportTests PRIVATE
    TransportTests.cpp
    ../Source/Domain/Transport/Transport.cpp
    ../Source/Domain/Transport/Transport.h
    ../Source/Domain/Transport/SyncManager.cpp
    ../Source/Domain/Transport/SyncManager.h
    ../Source/Domain/Clock/MasterClock.cpp
    ../Source/Domain/Clock/MasterClock.h
    ../Source/Domain/Clock/AsyncPatternEngine.cpp
    ../Source/Domain/Clock/AsyncPatternEngine.h
    ../Source/Domain/Models/Pattern.cpp
    ../Source/Domain/Models/Pattern.h
    ../Source/Domain/Models/Track.cpp
    ../Source/Domain/Models/Track.h
    ../Source/Domain/Models/Stage.cpp
    ../Source/Domain/Models/Stage.h
    ../Source/Domain/Models/Scale.cpp
    ../Source/Domain/Models/Scale.h
)

target_compile_definitions(TransportTests PRIVATE
    JUCE_UNIT_TESTS=1
    JUCE_USE_CURL=0
    JUCE_MODAL_LOOPS_PERMITTED=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
)

target_include_directories(TransportTests PRIVATE
    ${CMAKE_SOURCE_DIR}/Source
)

target_link_libraries(TransportTests PRIVATE
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_graphics
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

add_coverage_to_target(TransportTests)
add_test(NAME TransportTests COMMAND TransportTests)

# VoiceManager Tests
juce_add_console_app(VoiceManagerTests
    PRODUCT_NAME "VoiceManagerTests"
)

juce_generate_juce_header(VoiceManagerTests)

target_sources(VoiceManagerTests PRIVATE
    VoiceManagerTests.cpp
    ../Source/Domain/Engines/VoiceManager.cpp
    ../Source/Domain/Engines/VoiceManager.h
)

target_compile_definitions(VoiceManagerTests PRIVATE
    JUCE_UNIT_TESTS=1
    JUCE_USE_CURL=0
    JUCE_MODAL_LOOPS_PERMITTED=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
)

target_include_directories(VoiceManagerTests PRIVATE
    ${CMAKE_SOURCE_DIR}/Source
)

target_link_libraries(VoiceManagerTests PRIVATE
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

add_coverage_to_target(VoiceManagerTests)
add_test(NAME VoiceManagerTests COMMAND VoiceManagerTests)

# SequencerEngine Tests
juce_add_console_app(SequencerEngineTests
    PRODUCT_NAME "SequencerEngineTests"
)

juce_generate_juce_header(SequencerEngineTests)

target_sources(SequencerEngineTests PRIVATE
    SequencerEngineTests.cpp
    ../Source/Domain/Engines/SequencerEngine.cpp
    ../Source/Domain/Engines/SequencerEngine.h
    ../Source/Domain/Clock/MasterClock.cpp
    ../Source/Domain/Clock/MasterClock.h
    ../Source/Domain/Clock/AsyncPatternEngine.cpp
    ../Source/Domain/Clock/AsyncPatternEngine.h
    ../Source/Domain/Engines/VoiceManager.cpp
    ../Source/Domain/Engines/VoiceManager.h
    ../Source/Domain/Models/Pattern.cpp
    ../Source/Domain/Models/Pattern.h
    ../Source/Domain/Models/Track.cpp
    ../Source/Domain/Models/Track.h
    ../Source/Domain/Models/Stage.cpp
    ../Source/Domain/Models/Stage.h
    ../Source/Domain/Models/Scale.cpp
    ../Source/Domain/Models/Scale.h
    ../Source/Domain/Transport/Transport.cpp
    ../Source/Domain/Transport/Transport.h
    ../Source/Domain/Transport/SyncManager.cpp
    ../Source/Domain/Transport/SyncManager.h
)

target_compile_definitions(SequencerEngineTests PRIVATE
    JUCE_UNIT_TESTS=1
    JUCE_USE_CURL=0
    JUCE_MODAL_LOOPS_PERMITTED=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
)

target_include_directories(SequencerEngineTests PRIVATE
    ${CMAKE_SOURCE_DIR}/Source
)

target_link_libraries(SequencerEngineTests PRIVATE
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_graphics
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

add_coverage_to_target(SequencerEngineTests)
add_test(NAME SequencerEngineTests COMMAND SequencerEngineTests)

# MidiRouter Tests
juce_add_console_app(MidiRouterTests
    PRODUCT_NAME "MidiRouterTests"
)

juce_generate_juce_header(MidiRouterTests)

target_sources(MidiRouterTests PRIVATE
    MidiRouterTests.cpp
    ../Source/Domain/Services/MidiRouter.cpp
    ../Source/Domain/Services/MidiRouter.h
)

target_compile_definitions(MidiRouterTests PRIVATE
    JUCE_UNIT_TESTS=1
    JUCE_USE_CURL=0
    JUCE_MODAL_LOOPS_PERMITTED=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
)

target_include_directories(MidiRouterTests PRIVATE
    ${CMAKE_SOURCE_DIR}/Source
)

target_link_libraries(MidiRouterTests PRIVATE
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_audio_basics
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

add_coverage_to_target(MidiRouterTests)
add_test(NAME MidiRouterTests COMMAND MidiRouterTests)

# ChannelManager Tests
juce_add_console_app(ChannelManagerTests
    PRODUCT_NAME "ChannelManagerTests"
)

juce_generate_juce_header(ChannelManagerTests)

target_sources(ChannelManagerTests PRIVATE
    ChannelManagerTests.cpp
    ../Source/Domain/Services/ChannelManager.cpp
    ../Source/Domain/Services/ChannelManager.h
)

target_compile_definitions(ChannelManagerTests PRIVATE
    JUCE_UNIT_TESTS=1
    JUCE_USE_CURL=0
    JUCE_MODAL_LOOPS_PERMITTED=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
)

target_include_directories(ChannelManagerTests PRIVATE
    ${CMAKE_SOURCE_DIR}/Source
)

target_link_libraries(ChannelManagerTests PRIVATE
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_audio_basics
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

add_coverage_to_target(ChannelManagerTests)
add_test(NAME ChannelManagerTests COMMAND ChannelManagerTests)

# GateEngine Tests
juce_add_console_app(GateEngineTests
    PRODUCT_NAME "GateEngineTests"
)

juce_generate_juce_header(GateEngineTests)

target_sources(GateEngineTests PRIVATE
    GateEngineTests.cpp
    ../Source/Domain/Engines/GateEngine.cpp
    ../Source/Domain/Engines/GateEngine.h
    ../Source/Domain/Models/Stage.cpp
    ../Source/Domain/Models/Stage.h
    ../Source/Domain/Models/Track.cpp
    ../Source/Domain/Models/Track.h
    ../Source/Domain/Models/Pattern.cpp
    ../Source/Domain/Models/Pattern.h
    ../Source/Domain/Models/Scale.cpp
    ../Source/Domain/Models/Scale.h
)

target_compile_definitions(GateEngineTests PRIVATE
    JUCE_UNIT_TESTS=1
    JUCE_USE_CURL=0
    JUCE_MODAL_LOOPS_PERMITTED=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
)

target_include_directories(GateEngineTests PRIVATE
    ${CMAKE_SOURCE_DIR}/Source
)

target_link_libraries(GateEngineTests PRIVATE
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_audio_basics
    juce::juce_graphics
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

add_coverage_to_target(GateEngineTests)
add_test(NAME GateEngineTests COMMAND GateEngineTests)

# PitchEngine Tests
juce_add_console_app(PitchEngineTests
    PRODUCT_NAME "PitchEngineTests"
)

juce_generate_juce_header(PitchEngineTests)

target_sources(PitchEngineTests PRIVATE
    PitchEngineTests.cpp
    ../Source/Domain/Engines/PitchEngine.cpp
    ../Source/Domain/Engines/PitchEngine.h
    ../Source/Domain/Models/Stage.cpp
    ../Source/Domain/Models/Stage.h
    ../Source/Domain/Models/Track.cpp
    ../Source/Domain/Models/Track.h
    ../Source/Domain/Models/Pattern.cpp
    ../Source/Domain/Models/Pattern.h
    ../Source/Domain/Models/Scale.cpp
    ../Source/Domain/Models/Scale.h
)

target_compile_definitions(PitchEngineTests PRIVATE
    JUCE_UNIT_TESTS=1
    JUCE_USE_CURL=0
    JUCE_MODAL_LOOPS_PERMITTED=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
)

target_include_directories(PitchEngineTests PRIVATE
    ${CMAKE_SOURCE_DIR}/Source
)

target_link_libraries(PitchEngineTests PRIVATE
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_audio_basics
    juce::juce_graphics
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

add_coverage_to_target(PitchEngineTests)
add_test(NAME PitchEngineTests COMMAND PitchEngineTests)

# AccumulatorEngine Tests
juce_add_console_app(AccumulatorEngineTests
    PRODUCT_NAME "AccumulatorEngineTests"
)

juce_generate_juce_header(AccumulatorEngineTests)

target_sources(AccumulatorEngineTests PRIVATE
    AccumulatorEngineTests.cpp
    ../Source/Domain/Engines/AccumulatorEngine.cpp
    ../Source/Domain/Engines/AccumulatorEngine.h
    ../Source/Domain/Models/Stage.cpp
    ../Source/Domain/Models/Stage.h
    ../Source/Domain/Models/Track.cpp
    ../Source/Domain/Models/Track.h
    ../Source/Domain/Models/Pattern.cpp
    ../Source/Domain/Models/Pattern.h
    ../Source/Domain/Models/Scale.cpp
    ../Source/Domain/Models/Scale.h
)

target_compile_definitions(AccumulatorEngineTests PRIVATE
    JUCE_UNIT_TESTS=1
    JUCE_USE_CURL=0
    JUCE_MODAL_LOOPS_PERMITTED=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
)

target_include_directories(AccumulatorEngineTests PRIVATE
    ${CMAKE_SOURCE_DIR}/Source
)

target_link_libraries(AccumulatorEngineTests PRIVATE
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_audio_basics
    juce::juce_graphics
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

add_coverage_to_target(AccumulatorEngineTests)
add_test(NAME AccumulatorEngineTests COMMAND AccumulatorEngineTests)

# All Tests Runner
juce_add_console_app(AllTests
    PRODUCT_NAME "AllTests"
)

juce_generate_juce_header(AllTests)

target_sources(AllTests PRIVATE
    AllTests.cpp
    # Include all test source files
    VoiceManagerTests.cpp
    DomainModelTests.cpp
    SequencerEngineTests.cpp
    TimingTests.cpp
    TransportTests.cpp
    MidiRouterTests.cpp
    ChannelManagerTests.cpp
    GateEngineTests.cpp
    PitchEngineTests.cpp
    AccumulatorEngineTests.cpp
    # Include all necessary implementation files
    ../Source/Domain/Engines/VoiceManager.cpp
    ../Source/Domain/Engines/SequencerEngine.cpp
    ../Source/Domain/Engines/GateEngine.cpp
    ../Source/Domain/Engines/PitchEngine.cpp
    ../Source/Domain/Engines/AccumulatorEngine.cpp
    ../Source/Domain/Clock/MasterClock.cpp
    ../Source/Domain/Clock/AsyncPatternEngine.cpp
    ../Source/Domain/Models/Stage.cpp
    ../Source/Domain/Models/Track.cpp
    ../Source/Domain/Models/Pattern.cpp
    ../Source/Domain/Models/Scale.cpp
    ../Source/Domain/Transport/Transport.cpp
    ../Source/Domain/Transport/SyncManager.cpp
    ../Source/Domain/Services/MidiRouter.cpp
    ../Source/Domain/Services/ChannelManager.cpp
)

target_compile_definitions(AllTests PRIVATE
    JUCE_UNIT_TESTS=1
    JUCE_USE_CURL=0
    JUCE_MODAL_LOOPS_PERMITTED=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
)

target_include_directories(AllTests PRIVATE
    ${CMAKE_SOURCE_DIR}/Source
)

target_link_libraries(AllTests PRIVATE
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

add_coverage_to_target(AllTests)
add_test(NAME AllTests COMMAND AllTests)

# Scale Tests
juce_add_console_app(ScaleTests
    PRODUCT_NAME "ScaleTests"
)

juce_generate_juce_header(ScaleTests)

target_sources(ScaleTests PRIVATE
    ScaleTests.cpp
    ../Source/Domain/Models/Scale.cpp
    ../Source/Domain/Models/Scale.h
)

target_compile_definitions(ScaleTests PRIVATE
    JUCE_UNIT_TESTS=1
    JUCE_USE_CURL=0
    JUCE_MODAL_LOOPS_PERMITTED=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
)

target_include_directories(ScaleTests PRIVATE
    ${CMAKE_SOURCE_DIR}/Source
)

target_link_libraries(ScaleTests PRIVATE
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_audio_basics
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

add_coverage_to_target(ScaleTests)
add_test(NAME ScaleTests COMMAND ScaleTests)

# Stage Tests
juce_add_console_app(StageTests
    PRODUCT_NAME "StageTests"
)

juce_generate_juce_header(StageTests)

target_sources(StageTests PRIVATE
    StageTests.cpp
    ../Source/Domain/Models/Stage.cpp
    ../Source/Domain/Models/Stage.h
)

target_compile_definitions(StageTests PRIVATE
    JUCE_UNIT_TESTS=1
    JUCE_USE_CURL=0
    JUCE_MODAL_LOOPS_PERMITTED=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
)

target_include_directories(StageTests PRIVATE
    ${CMAKE_SOURCE_DIR}/Source
)

target_link_libraries(StageTests PRIVATE
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_audio_basics
    juce::juce_graphics
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

add_coverage_to_target(StageTests)
add_test(NAME StageTests COMMAND StageTests)

# Track Tests
juce_add_console_app(TrackTests
    PRODUCT_NAME "TrackTests"
)

juce_generate_juce_header(TrackTests)

target_sources(TrackTests PRIVATE
    TrackTests.cpp
    ../Source/Domain/Models/Track.cpp
    ../Source/Domain/Models/Track.h
    ../Source/Domain/Models/Stage.cpp
    ../Source/Domain/Models/Stage.h
    ../Source/Domain/Models/Scale.cpp
    ../Source/Domain/Models/Scale.h
)

target_compile_definitions(TrackTests PRIVATE
    JUCE_UNIT_TESTS=1
    JUCE_USE_CURL=0
    JUCE_MODAL_LOOPS_PERMITTED=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
)

target_include_directories(TrackTests PRIVATE
    ${CMAKE_SOURCE_DIR}/Source
)

target_link_libraries(TrackTests PRIVATE
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_audio_basics
    juce::juce_graphics
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

add_coverage_to_target(TrackTests)
add_test(NAME TrackTests COMMAND TrackTests)

# PresetManager Tests
juce_add_console_app(PresetManagerTests
    PRODUCT_NAME "PresetManagerTests"
)

juce_generate_juce_header(PresetManagerTests)

target_sources(PresetManagerTests PRIVATE
    PresetManagerTests.cpp
    ../Source/Domain/Services/PresetManager.cpp
    ../Source/Domain/Services/PresetManager.h
    ../Source/Domain/Models/Pattern.cpp
    ../Source/Domain/Models/Pattern.h
    ../Source/Domain/Models/Track.cpp
    ../Source/Domain/Models/Track.h
    ../Source/Domain/Models/Stage.cpp
    ../Source/Domain/Models/Stage.h
    ../Source/Domain/Models/Scale.cpp
    ../Source/Domain/Models/Scale.h
)

target_compile_definitions(PresetManagerTests PRIVATE
    JUCE_UNIT_TESTS=1
    JUCE_USE_CURL=0
    JUCE_MODAL_LOOPS_PERMITTED=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
)

target_include_directories(PresetManagerTests PRIVATE
    ${CMAKE_SOURCE_DIR}/Source
)

target_link_libraries(PresetManagerTests PRIVATE
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_audio_basics
    juce::juce_graphics
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

add_coverage_to_target(PresetManagerTests)
add_test(NAME PresetManagerTests COMMAND PresetManagerTests)

# SyncManager Tests
juce_add_console_app(SyncManagerTests
    PRODUCT_NAME "SyncManagerTests"
)

juce_generate_juce_header(SyncManagerTests)

target_sources(SyncManagerTests PRIVATE
    SyncManagerTests.cpp
    ../Source/Domain/Transport/SyncManager.cpp
    ../Source/Domain/Transport/SyncManager.h
    ../Source/Domain/Clock/MasterClock.cpp
    ../Source/Domain/Clock/MasterClock.h
)

target_compile_definitions(SyncManagerTests PRIVATE
    JUCE_UNIT_TESTS=1
    JUCE_USE_CURL=0
    JUCE_MODAL_LOOPS_PERMITTED=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
)

target_include_directories(SyncManagerTests PRIVATE
    ${CMAKE_SOURCE_DIR}/Source
)

target_link_libraries(SyncManagerTests PRIVATE
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

add_coverage_to_target(SyncManagerTests)
add_test(NAME SyncManagerTests COMMAND SyncManagerTests)