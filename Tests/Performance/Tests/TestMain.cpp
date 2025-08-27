#include <JuceHeader.h>
#include "../Core/PerformanceMetrics.h"
#include <iostream>

using namespace HAM::Performance;

/**
 * Main entry point for performance tests
 */
class PerformanceTestRunner : public juce::UnitTestRunner {
public:
    void logMessage(const String& message) override {
        std::cout << message << std::endl;
    }
};

int main(int argc, char* argv[]) {
    juce::initialiseJuce_GUI();
    
    std::cout << "\n";
    std::cout << "================================================\n";
    std::cout << "   HAM Performance Test Suite\n";
    std::cout << "================================================\n";
    std::cout << "Performance Requirements:\n";
    std::cout << "  • CPU Usage: < " << PerformanceThresholds::MAX_CPU_USAGE_PERCENT << "%\n";
    std::cout << "  • MIDI Jitter: < " << PerformanceThresholds::MAX_MIDI_JITTER_MS << "ms\n";
    std::cout << "  • Audio Latency: < " << PerformanceThresholds::MAX_AUDIO_LATENCY_MS << "ms\n";
    std::cout << "  • Memory Usage: < " << PerformanceThresholds::MAX_MEMORY_MB << "MB\n";
    std::cout << "================================================\n\n";
    
    PerformanceTestRunner runner;
    runner.runAllTests();
    
    int numFailures = runner.getNumFailures();
    
    if (numFailures == 0) {
        std::cout << "\n✅ All performance tests passed!\n";
    } else {
        std::cout << "\n❌ " << numFailures << " test(s) failed!\n";
    }
    
    // Generate performance snapshot
    auto snapshot = capturePerformanceSnapshot();
    
    std::cout << "\n================================================\n";
    std::cout << "   Performance Snapshot\n";
    std::cout << "================================================\n";
    
    if (snapshot.meetsThresholds()) {
        std::cout << "✅ All performance metrics within thresholds\n";
    } else {
        std::cout << "⚠️ Some metrics exceed thresholds\n";
        
        if (snapshot.cpu_usage.max >= PerformanceThresholds::MAX_CPU_USAGE_PERCENT) {
            std::cout << "  • CPU: " << snapshot.cpu_usage.max << "% (threshold: " 
                     << PerformanceThresholds::MAX_CPU_USAGE_PERCENT << "%)\n";
        }
        if (snapshot.midi_jitter >= PerformanceThresholds::MAX_MIDI_JITTER_MS) {
            std::cout << "  • Jitter: " << snapshot.midi_jitter << "ms (threshold: "
                     << PerformanceThresholds::MAX_MIDI_JITTER_MS << "ms)\n";
        }
        if (snapshot.audio_latency.max >= PerformanceThresholds::MAX_AUDIO_LATENCY_MS) {
            std::cout << "  • Latency: " << snapshot.audio_latency.max << "ms (threshold: "
                     << PerformanceThresholds::MAX_AUDIO_LATENCY_MS << "ms)\n";
        }
    }
    
    std::cout << "================================================\n\n";
    
    juce::shutdownJuce_GUI();
    
    return numFailures > 0 ? 1 : 0;
}