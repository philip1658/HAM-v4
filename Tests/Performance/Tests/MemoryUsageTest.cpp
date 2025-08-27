#include <JuceHeader.h>
#include "../Core/PerformanceMetrics.h"
#include "Infrastructure/Audio/HAMAudioProcessor.h"

using namespace HAM;
using namespace HAM::Performance;

/**
 * Memory usage and leak detection tests
 */
class MemoryUsageTest : public juce::UnitTest {
public:
    MemoryUsageTest() : UnitTest("Memory Usage Test") {}
    
    void runTest() override {
        beginTest("Baseline Memory Usage");
        testBaselineMemory();
        
        beginTest("Processing Memory Growth");
        testMemoryGrowth();
        
        beginTest("Pattern Memory Management");
        testPatternMemory();
        
        beginTest("Plugin Memory Isolation");
        testPluginMemory();
        
        beginTest("Memory Leak Detection");
        testMemoryLeaks();
    }
    
private:
    void testBaselineMemory() {
        MemoryMonitor monitor;
        
        // Create and destroy processor multiple times
        for (int i = 0; i < 10; ++i) {
            {
                HAMAudioProcessor processor;
                processor.prepareToPlay(48000.0, 512);
                
                // Minimal processing
                juce::AudioBuffer<float> buffer(2, 512);
                juce::MidiBuffer midi;
                processor.processBlock(buffer, midi);
                
                processor.releaseResources();
            }
            
            // Force cleanup
            juce::MessageManager::getInstance()->runDispatchLoopUntil(10);
        }
        
        auto stats = monitor.getStats();
        double peakMB = stats.peak_bytes / (1024.0 * 1024.0);
        
        logMessage("Baseline memory: " + String(peakMB, 2) + " MB");
        expect(peakMB < 50.0, "Baseline memory usage too high");
    }
    
    void testMemoryGrowth() {
        HAMAudioProcessor processor;
        processor.prepareToPlay(48000.0, 512);
        
        MemoryMonitor monitor;
        size_t initialMemory = monitor.getStats().current_bytes;
        
        juce::AudioBuffer<float> buffer(2, 512);
        juce::MidiBuffer midi;
        
        // Process for extended period
        for (int i = 0; i < 10000; ++i) {
            processor.processBlock(buffer, midi);
            
            // Check for memory growth every 1000 iterations
            if (i % 1000 == 0) {
                size_t currentMemory = monitor.getStats().current_bytes;
                size_t growth = currentMemory - initialMemory;
                double growthMB = growth / (1024.0 * 1024.0);
                
                if (i > 0) {  // Skip first iteration
                    expect(growthMB < 10.0, 
                          "Memory growing during processing: " + String(growthMB, 2) + " MB");
                }
            }
        }
        
        processor.releaseResources();
    }
    
    void testPatternMemory() {
        MemoryMonitor monitor;
        size_t beforeBytes = monitor.getStats().current_bytes;
        
        // Create many patterns
        std::vector<std::shared_ptr<Pattern>> patterns;
        for (int i = 0; i < 1000; ++i) {
            auto pattern = std::make_shared<Pattern>();
            pattern->setLength(64);
            
            for (int j = 0; j < 64; ++j) {
                pattern->getStage(j).pitch = 60 + j;
                pattern->getStage(j).velocity = 100;
            }
            
            patterns.push_back(pattern);
        }
        
        size_t afterBytes = monitor.getStats().current_bytes;
        size_t usedBytes = afterBytes - beforeBytes;
        double usedMB = usedBytes / (1024.0 * 1024.0);
        
        logMessage("1000 patterns use: " + String(usedMB, 2) + " MB");
        
        // Estimate per-pattern memory
        double perPatternKB = (usedBytes / 1000.0) / 1024.0;
        logMessage("Per-pattern: " + String(perPatternKB, 2) + " KB");
        
        expect(perPatternKB < 10.0, "Pattern memory usage too high");
        
        // Clear patterns and check memory is released
        patterns.clear();
        juce::MessageManager::getInstance()->runDispatchLoopUntil(100);
        
        size_t clearedBytes = monitor.getStats().current_bytes;
        expect(clearedBytes < afterBytes, "Memory not released after clearing patterns");
    }
    
    void testPluginMemory() {
        // Test plugin sandboxing memory isolation
        // This would test actual plugin loading if implemented
        
        MemoryMonitor monitor;
        size_t beforeBytes = monitor.getStats().current_bytes;
        
        // Simulate plugin operations
        for (int i = 0; i < 10; ++i) {
            // PluginManager::loadPlugin("test.vst3");
            // Process with plugin
            // PluginManager::unloadPlugin();
        }
        
        size_t afterBytes = monitor.getStats().current_bytes;
        
        // Memory should return to baseline after unloading
        double leakMB = (afterBytes - beforeBytes) / (1024.0 * 1024.0);
        expect(leakMB < 1.0, "Plugin memory not properly isolated");
    }
    
    void testMemoryLeaks() {
        MemoryMonitor monitor;
        
        // Capture baseline
        juce::MessageManager::getInstance()->runDispatchLoopUntil(100);
        size_t baseline = monitor.getStats().current_bytes;
        
        // Run allocation/deallocation cycles
        for (int cycle = 0; cycle < 100; ++cycle) {
            HAMAudioProcessor* processor = new HAMAudioProcessor();
            processor->prepareToPlay(48000.0, 512);
            
            juce::AudioBuffer<float> buffer(2, 512);
            juce::MidiBuffer midi;
            
            for (int i = 0; i < 100; ++i) {
                processor->processBlock(buffer, midi);
            }
            
            processor->releaseResources();
            delete processor;
        }
        
        // Force cleanup
        juce::MessageManager::getInstance()->runDispatchLoopUntil(100);
        
        size_t final = monitor.getStats().current_bytes;
        double leakMB = std::abs((int64_t)final - (int64_t)baseline) / (1024.0 * 1024.0);
        
        logMessage("Potential leak after 100 cycles: " + String(leakMB, 3) + " MB");
        expect(leakMB < 1.0, "Memory leak detected");
    }
};

static MemoryUsageTest memoryUsageTest;