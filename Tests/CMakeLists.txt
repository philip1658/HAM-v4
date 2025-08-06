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

add_test(NAME TransportTests COMMAND TransportTests)

# VoiceManager Tests
juce_add_console_app(VoiceManagerTests
    PRODUCT_NAME "VoiceManagerTests"
)

juce_generate_juce_header(VoiceManagerTests)

target_sources(VoiceManagerTests PRIVATE
    VoiceManagerTests.cpp
    ../Source/Engine/Voice/VoiceManager.cpp
    ../Source/Engine/Voice/VoiceManager.h
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
    ../Source/Engine/Voice/VoiceManager.cpp
    ../Source/Engine/Voice/VoiceManager.h
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

add_test(NAME ChannelManagerTests COMMAND ChannelManagerTests)