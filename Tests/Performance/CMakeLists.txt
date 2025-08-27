# Performance Testing Suite for HAM
# Uses Google Benchmark for microbenchmarking

find_package(benchmark REQUIRED)
find_package(Threads REQUIRED)

# Performance monitoring library
add_library(HAMPerformanceLib STATIC
    Core/PerformanceMetrics.h
    Core/PerformanceMetrics.cpp
    Core/PerformanceReporter.h
    Core/PerformanceReporter.cpp
    Core/RegressionDetector.h
    Core/RegressionDetector.cpp
    Core/BenchmarkHelpers.h
    Core/BenchmarkHelpers.cpp
)

target_include_directories(HAMPerformanceLib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/Source
)

target_link_libraries(HAMPerformanceLib PUBLIC
    benchmark::benchmark
    juce::juce_core
    juce::juce_audio_basics
    juce::juce_events
)

# Macro benchmark executable
juce_add_console_app(HAMPerformanceBenchmarks
    PRODUCT_NAME "HAMPerformanceBenchmarks"
)

juce_generate_juce_header(HAMPerformanceBenchmarks)

target_sources(HAMPerformanceBenchmarks PRIVATE
    # Core benchmarks
    Benchmarks/AudioProcessorBenchmark.cpp
    Benchmarks/MasterClockBenchmark.cpp
    Benchmarks/MidiGenerationBenchmark.cpp
    Benchmarks/LockFreeQueueBenchmark.cpp
    Benchmarks/PatternProcessingBenchmark.cpp
    Benchmarks/VoiceManagerBenchmark.cpp
    
    # Main runner
    BenchmarkMain.cpp
    
    # Source files needed for benchmarks
    ${CMAKE_SOURCE_DIR}/Source/Infrastructure/Audio/HAMAudioProcessor.cpp
    ${CMAKE_SOURCE_DIR}/Source/Infrastructure/Audio/HAMAudioProcessor.h
    ${CMAKE_SOURCE_DIR}/Source/Domain/Clock/MasterClock.cpp
    ${CMAKE_SOURCE_DIR}/Source/Domain/Clock/MasterClock.h
    ${CMAKE_SOURCE_DIR}/Source/Domain/Clock/AsyncPatternEngine.cpp
    ${CMAKE_SOURCE_DIR}/Source/Domain/Clock/AsyncPatternEngine.h
    ${CMAKE_SOURCE_DIR}/Source/Domain/Processors/MidiEventGenerator.cpp
    ${CMAKE_SOURCE_DIR}/Source/Domain/Processors/MidiEventGenerator.h
    ${CMAKE_SOURCE_DIR}/Source/Domain/Engines/VoiceManager.cpp
    ${CMAKE_SOURCE_DIR}/Source/Domain/Engines/VoiceManager.h
    ${CMAKE_SOURCE_DIR}/Source/Domain/Engines/SequencerEngine.cpp
    ${CMAKE_SOURCE_DIR}/Source/Domain/Engines/SequencerEngine.h
    ${CMAKE_SOURCE_DIR}/Source/Domain/Engines/GateEngine.cpp
    ${CMAKE_SOURCE_DIR}/Source/Domain/Engines/GateEngine.h
    ${CMAKE_SOURCE_DIR}/Source/Domain/Engines/PitchEngine.cpp
    ${CMAKE_SOURCE_DIR}/Source/Domain/Engines/PitchEngine.h
    ${CMAKE_SOURCE_DIR}/Source/Domain/Engines/AccumulatorEngine.cpp
    ${CMAKE_SOURCE_DIR}/Source/Domain/Engines/AccumulatorEngine.h
    ${CMAKE_SOURCE_DIR}/Source/Domain/Models/Pattern.cpp
    ${CMAKE_SOURCE_DIR}/Source/Domain/Models/Pattern.h
    ${CMAKE_SOURCE_DIR}/Source/Domain/Models/Track.cpp
    ${CMAKE_SOURCE_DIR}/Source/Domain/Models/Track.h
    ${CMAKE_SOURCE_DIR}/Source/Domain/Models/Stage.cpp
    ${CMAKE_SOURCE_DIR}/Source/Domain/Models/Stage.h
    ${CMAKE_SOURCE_DIR}/Source/Domain/Models/Scale.cpp
    ${CMAKE_SOURCE_DIR}/Source/Domain/Models/Scale.h
    ${CMAKE_SOURCE_DIR}/Source/Domain/Services/MidiRouter.cpp
    ${CMAKE_SOURCE_DIR}/Source/Domain/Services/MidiRouter.h
    ${CMAKE_SOURCE_DIR}/Source/Domain/Services/ChannelManager.cpp
    ${CMAKE_SOURCE_DIR}/Source/Domain/Services/ChannelManager.h
    ${CMAKE_SOURCE_DIR}/Source/Domain/Transport/Transport.cpp
    ${CMAKE_SOURCE_DIR}/Source/Domain/Transport/Transport.h
    ${CMAKE_SOURCE_DIR}/Source/Domain/Transport/SyncManager.cpp
    ${CMAKE_SOURCE_DIR}/Source/Domain/Transport/SyncManager.h
    ${CMAKE_SOURCE_DIR}/Source/Domain/Processors/TrackProcessor.cpp
    ${CMAKE_SOURCE_DIR}/Source/Domain/Processors/TrackProcessor.h
    ${CMAKE_SOURCE_DIR}/Source/Domain/Processors/PatternScheduler.cpp
    ${CMAKE_SOURCE_DIR}/Source/Domain/Processors/PatternScheduler.h
)

target_compile_definitions(HAMPerformanceBenchmarks PRIVATE
    JUCE_USE_CURL=0
    JUCE_MODAL_LOOPS_PERMITTED=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
    HAM_PERFORMANCE_TESTING=1
)

target_include_directories(HAMPerformanceBenchmarks PRIVATE
    ${CMAKE_SOURCE_DIR}/Source
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(HAMPerformanceBenchmarks PRIVATE
    HAMPerformanceLib
    benchmark::benchmark
    benchmark::benchmark_main
    juce::juce_core
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_processors
    juce::juce_data_structures
    juce::juce_events
    juce::juce_graphics
    juce::juce_recommended_config_flags
    Threads::Threads
)

# Integration test executable
juce_add_console_app(HAMPerformanceTests
    PRODUCT_NAME "HAMPerformanceTests"
)

juce_generate_juce_header(HAMPerformanceTests)

target_sources(HAMPerformanceTests PRIVATE
    Tests/PerformanceIntegrationTest.cpp
    Tests/CPUUsageTest.cpp
    Tests/MemoryUsageTest.cpp
    Tests/LatencyTest.cpp
    Tests/JitterTest.cpp
    Tests/ThreadContentionTest.cpp
    
    # Main test runner
    TestMain.cpp
)

target_compile_definitions(HAMPerformanceTests PRIVATE
    JUCE_UNIT_TESTS=1
    JUCE_USE_CURL=0
    JUCE_MODAL_LOOPS_PERMITTED=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
    HAM_PERFORMANCE_TESTING=1
)

target_include_directories(HAMPerformanceTests PRIVATE
    ${CMAKE_SOURCE_DIR}/Source
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(HAMPerformanceTests PRIVATE
    HAMPerformanceLib
    juce::juce_core
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_events
    juce::juce_recommended_config_flags
)

# Regression detector tool
add_executable(HAMRegressionDetector
    Tools/RegressionDetectorMain.cpp
)

target_link_libraries(HAMRegressionDetector PRIVATE
    HAMPerformanceLib
)

# Add test targets
enable_testing()
add_test(NAME PerformanceBenchmarks COMMAND HAMPerformanceBenchmarks --benchmark_out=benchmark_results.json --benchmark_out_format=json)
add_test(NAME PerformanceTests COMMAND HAMPerformanceTests)

# Custom target for regression testing
add_custom_target(performance_regression
    COMMAND HAMPerformanceBenchmarks --benchmark_out=current_results.json --benchmark_out_format=json
    COMMAND HAMRegressionDetector baseline_results.json current_results.json
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running performance regression tests..."
)

# Custom target for performance report
add_custom_target(performance_report
    COMMAND HAMPerformanceBenchmarks --benchmark_out=report.json --benchmark_out_format=json
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/Scripts/generate_report.py report.json
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating performance report..."
)