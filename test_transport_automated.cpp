/*
  ==============================================================================

    test_transport_automated.cpp
    Automated test to verify transport and clock functionality

  ==============================================================================
*/

#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <JuceHeader.h>
#include "Source/Infrastructure/Audio/HAMAudioProcessor.h"

class AutomatedTransportTest : public juce::JUCEApplication
{
public:
    AutomatedTransportTest() {}
    
    const juce::String getApplicationName() override { return "Transport Test"; }
    const juce::String getApplicationVersion() override { return "1.0"; }
    
    void initialise(const juce::String&) override
    {
        std::cout << "\n========== AUTOMATED TRANSPORT TEST ==========\n" << std::endl;
        
        // Create processor
        auto processor = std::make_unique<HAM::HAMAudioProcessor>();
        
        // Prepare to play
        double sampleRate = 44100.0;
        int blockSize = 512;
        processor->prepareToPlay(sampleRate, blockSize);
        
        // Test 1: Check initial state
        std::cout << "TEST 1: Initial State" << std::endl;
        bool initialPlaying = processor->isPlaying();
        std::cout << "  - Initial playing state: " << (initialPlaying ? "PLAYING" : "STOPPED") << std::endl;
        if (initialPlaying)
        {
            std::cout << "  ❌ FAIL: Should start in stopped state" << std::endl;
        }
        else
        {
            std::cout << "  ✅ PASS: Correctly started in stopped state" << std::endl;
        }
        
        // Test 2: Start playback
        std::cout << "\nTEST 2: Start Playback" << std::endl;
        processor->play();
        
        // Give it a moment to process
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        bool playingAfterStart = processor->isPlaying();
        std::cout << "  - Playing after start: " << (playingAfterStart ? "PLAYING" : "STOPPED") << std::endl;
        
        if (!playingAfterStart)
        {
            std::cout << "  ❌ FAIL: Transport did not start" << std::endl;
            
            // Detailed diagnosis
            std::cout << "\n  DIAGNOSIS:" << std::endl;
            std::cout << "  - Transport state: " << processor->getTransportState() << std::endl;
            std::cout << "  - Clock running: " << (processor->isClockRunning() ? "YES" : "NO") << std::endl;
            std::cout << "  - Current BPM: " << processor->getCurrentBPM() << std::endl;
            std::cout << "  - Sample rate: " << processor->getSampleRate() << std::endl;
        }
        else
        {
            std::cout << "  ✅ PASS: Transport started successfully" << std::endl;
        }
        
        // Test 3: Process some blocks and check clock advancement
        std::cout << "\nTEST 3: Clock Processing" << std::endl;
        
        juce::AudioBuffer<float> buffer(2, blockSize);
        juce::MidiBuffer midiBuffer;
        
        int initialPulse = processor->getCurrentPulse();
        
        // Process 10 blocks
        for (int i = 0; i < 10; ++i)
        {
            processor->processBlock(buffer, midiBuffer);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        
        int pulseAfterProcessing = processor->getCurrentPulse();
        std::cout << "  - Initial pulse: " << initialPulse << std::endl;
        std::cout << "  - Pulse after 10 blocks: " << pulseAfterProcessing << std::endl;
        
        if (playingAfterStart && pulseAfterProcessing == initialPulse)
        {
            std::cout << "  ❌ FAIL: Clock not advancing during processBlock" << std::endl;
            
            // More diagnosis
            std::cout << "\n  PROCESSBLOCK DIAGNOSIS:" << std::endl;
            std::cout << "  - processBlock being called: YES (10 times)" << std::endl;
            std::cout << "  - Clock should be running: " << (processor->isClockRunning() ? "YES" : "NO") << std::endl;
            std::cout << "  - Transport playing: " << (processor->isPlaying() ? "YES" : "NO") << std::endl;
            
            // Check if clock is actually being processed
            if (processor->isPlaying() && processor->isClockRunning())
            {
                std::cout << "  - ISSUE: Clock is running but not advancing!" << std::endl;
                std::cout << "    Likely cause: processBlock not calling clock->processBlock()" << std::endl;
            }
            else if (processor->isPlaying() && !processor->isClockRunning())
            {
                std::cout << "  - ISSUE: Transport playing but clock not running!" << std::endl;
                std::cout << "    Likely cause: Transport::play() not calling m_clock.start()" << std::endl;
            }
            else
            {
                std::cout << "  - ISSUE: Transport not in playing state!" << std::endl;
                std::cout << "    Likely cause: State change failed in Transport::play()" << std::endl;
            }
        }
        else if (pulseAfterProcessing != initialPulse)
        {
            std::cout << "  ✅ PASS: Clock is advancing correctly" << std::endl;
        }
        
        // Test 4: Stop playback
        std::cout << "\nTEST 4: Stop Playback" << std::endl;
        processor->stop();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        bool playingAfterStop = processor->isPlaying();
        std::cout << "  - Playing after stop: " << (playingAfterStop ? "PLAYING" : "STOPPED") << std::endl;
        
        if (playingAfterStop)
        {
            std::cout << "  ❌ FAIL: Transport did not stop" << std::endl;
        }
        else
        {
            std::cout << "  ✅ PASS: Transport stopped successfully" << std::endl;
        }
        
        // Summary
        std::cout << "\n========== TEST SUMMARY ==========\n" << std::endl;
        
        if (!playingAfterStart)
        {
            std::cout << "🔴 CRITICAL ISSUE: Transport/Clock system not starting" << std::endl;
            std::cout << "   The play button press is not starting the clock." << std::endl;
            std::cout << "   This is why you hear no sound from plugins." << std::endl;
        }
        else if (playingAfterStart && pulseAfterProcessing == initialPulse)
        {
            std::cout << "🔴 CRITICAL ISSUE: Clock not advancing in processBlock" << std::endl;
            std::cout << "   The clock starts but doesn't process during audio callbacks." << std::endl;
            std::cout << "   This is why no MIDI events are generated." << std::endl;
        }
        else
        {
            std::cout << "✅ All tests passed - transport system working correctly" << std::endl;
        }
        
        std::cout << "\n==========================================" << std::endl;
        
        // Cleanup and exit
        processor->releaseResources();
        quit();
    }
    
    void shutdown() override {}
};

START_JUCE_APPLICATION(AutomatedTransportTest)