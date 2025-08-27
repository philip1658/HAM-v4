#include <JuceHeader.h>
#include "../Core/PerformanceMetrics.h"
#include "Infrastructure/Audio/HAMAudioProcessor.h"

using namespace HAM;
using namespace HAM::Performance;

/**
 * Dedicated CPU usage tests
 */
class CPUUsageTest : public juce::UnitTest {
public:
    CPUUsageTest() : UnitTest("CPU Usage Test") {}
    
    void runTest() override {
        beginTest("Idle CPU Usage");
        testIdleCPU();
        
        beginTest("Light Load CPU Usage");
        testLightLoad();
        
        beginTest("Normal Load CPU Usage");
        testNormalLoad();
        
        beginTest("Heavy Load CPU Usage");
        testHeavyLoad();
        
        beginTest("CPU Usage vs Buffer Size");
        testBufferSizeImpact();
        
        beginTest("CPU Usage vs Sample Rate");
        testSampleRateImpact();
    }
    
private:
    void testIdleCPU() {
        HAMAudioProcessor processor;
        processor.prepareToPlay(48000.0, 512);
        
        juce::AudioBuffer<float> audioBuffer(2, 512);
        juce::MidiBuffer emptyMidi;
        
        CPUMonitor monitor;
        monitor.startMeasurement();
        
        // Process with no MIDI events
        for (int i = 0; i < 100; ++i) {
            processor.processBlock(audioBuffer, emptyMidi);
        }
        
        monitor.endMeasurement();
        auto metrics = monitor.getMetrics();
        
        logMessage("Idle CPU: " + String(metrics.mean, 2) + "%");
        expect(metrics.mean < 1.0, "Idle CPU usage too high");
    }
    
    void testLightLoad() {
        HAMAudioProcessor processor;
        processor.prepareToPlay(48000.0, 512);
        
        juce::AudioBuffer<float> audioBuffer(2, 512);
        juce::MidiBuffer midiBuffer;
        
        // Add light MIDI load (10 events per buffer)
        for (int i = 0; i < 10; ++i) {
            midiBuffer.addEvent(
                juce::MidiMessage::noteOn(1, 60 + i, (uint8_t)100),
                i * 50
            );
        }
        
        CPUMonitor monitor;
        monitor.startMeasurement();
        
        for (int i = 0; i < 100; ++i) {
            processor.processBlock(audioBuffer, midiBuffer);
            midiBuffer.clear();
        }
        
        monitor.endMeasurement();
        auto metrics = monitor.getMetrics();
        
        logMessage("Light Load CPU: " + String(metrics.mean, 2) + "%");
        expect(metrics.mean < 2.0, "Light load CPU usage too high");
    }
    
    void testNormalLoad() {
        HAMAudioProcessor processor;
        processor.prepareToPlay(48000.0, 512);
        
        juce::AudioBuffer<float> audioBuffer(2, 512);
        juce::MidiBuffer midiBuffer;
        
        CPUMonitor monitor;
        monitor.startMeasurement();
        
        // Simulate normal sequencer operation
        for (int buffer = 0; buffer < 100; ++buffer) {
            // Add typical MIDI pattern
            for (int i = 0; i < 4; ++i) {
                midiBuffer.addEvent(
                    juce::MidiMessage::noteOn(1, 60 + i * 4, (uint8_t)100),
                    i * 128
                );
                midiBuffer.addEvent(
                    juce::MidiMessage::noteOff(1, 60 + i * 4, (uint8_t)64),
                    i * 128 + 64
                );
            }
            
            processor.processBlock(audioBuffer, midiBuffer);
            midiBuffer.clear();
        }
        
        monitor.endMeasurement();
        auto metrics = monitor.getMetrics();
        
        logMessage("Normal Load CPU: " + String(metrics.mean, 2) + "%");
        expect(metrics.mean < 3.0, "Normal load CPU usage too high");
        expect(metrics.max < PerformanceThresholds::MAX_CPU_USAGE_PERCENT,
               "Peak CPU exceeds threshold");
    }
    
    void testHeavyLoad() {
        HAMAudioProcessor processor;
        processor.prepareToPlay(48000.0, 512);
        
        juce::AudioBuffer<float> audioBuffer(2, 512);
        juce::MidiBuffer midiBuffer;
        
        CPUMonitor monitor;
        monitor.startMeasurement();
        
        // Heavy MIDI load
        for (int buffer = 0; buffer < 100; ++buffer) {
            for (int i = 0; i < 50; ++i) {
                midiBuffer.addEvent(
                    juce::MidiMessage::noteOn((i % 8) + 1, 36 + i, (uint8_t)(64 + i)),
                    i * 10
                );
            }
            
            processor.processBlock(audioBuffer, midiBuffer);
            midiBuffer.clear();
        }
        
        monitor.endMeasurement();
        auto metrics = monitor.getMetrics();
        
        logMessage("Heavy Load CPU: " + String(metrics.mean, 2) + "%");
        logMessage("Heavy Load Peak: " + String(metrics.max, 2) + "%");
        
        expect(metrics.max < PerformanceThresholds::MAX_CPU_USAGE_PERCENT,
               "Heavy load CPU exceeds threshold");
    }
    
    void testBufferSizeImpact() {
        const int bufferSizes[] = {64, 128, 256, 512, 1024, 2048};
        
        for (int bufferSize : bufferSizes) {
            HAMAudioProcessor processor;
            processor.prepareToPlay(48000.0, bufferSize);
            
            juce::AudioBuffer<float> audioBuffer(2, bufferSize);
            juce::MidiBuffer midiBuffer;
            
            // Add proportional MIDI events
            for (int i = 0; i < bufferSize / 64; ++i) {
                midiBuffer.addEvent(
                    juce::MidiMessage::noteOn(1, 60 + i, (uint8_t)100),
                    i * 64
                );
            }
            
            CPUMonitor monitor;
            monitor.startMeasurement();
            
            // Process equivalent amount of audio
            int iterations = 51200 / bufferSize; // Same total samples
            for (int i = 0; i < iterations; ++i) {
                processor.processBlock(audioBuffer, midiBuffer);
            }
            
            monitor.endMeasurement();
            auto metrics = monitor.getMetrics();
            
            logMessage("Buffer " + String(bufferSize) + ": CPU " + 
                      String(metrics.mean, 2) + "%");
        }
    }
    
    void testSampleRateImpact() {
        const double sampleRates[] = {44100.0, 48000.0, 88200.0, 96000.0};
        
        for (double sampleRate : sampleRates) {
            HAMAudioProcessor processor;
            processor.prepareToPlay(sampleRate, 512);
            
            juce::AudioBuffer<float> audioBuffer(2, 512);
            juce::MidiBuffer midiBuffer;
            
            for (int i = 0; i < 10; ++i) {
                midiBuffer.addEvent(
                    juce::MidiMessage::noteOn(1, 60 + i, (uint8_t)100),
                    i * 50
                );
            }
            
            CPUMonitor monitor;
            monitor.startMeasurement();
            
            for (int i = 0; i < 100; ++i) {
                processor.processBlock(audioBuffer, midiBuffer);
            }
            
            monitor.endMeasurement();
            auto metrics = monitor.getMetrics();
            
            logMessage(String(sampleRate/1000.0, 1) + "kHz: CPU " + 
                      String(metrics.mean, 2) + "%");
        }
    }
};

static CPUUsageTest cpuUsageTest;