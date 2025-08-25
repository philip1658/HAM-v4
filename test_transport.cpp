/*
  ==============================================================================

    test_transport.cpp
    Quick test to verify transport control functionality

  ==============================================================================
*/

#include <iostream>
#include <chrono>
#include <thread>
#include "Source/Domain/Clock/MasterClock.h"
#include "Source/Domain/Transport/Transport.h"

int main()
{
    std::cout << "=== HAM Transport Test ===" << std::endl;
    
    // Create clock and transport
    HAM::MasterClock clock;
    HAM::Transport transport(clock);
    
    // Set BPM
    clock.setBPM(120.0f);
    std::cout << "BPM set to: " << clock.getBPM() << std::endl;
    
    // Test play
    std::cout << "\nStarting transport..." << std::endl;
    transport.play();
    
    // Check if playing
    if (transport.isPlaying()) {
        std::cout << "✓ Transport is playing" << std::endl;
    } else {
        std::cout << "✗ Transport failed to start!" << std::endl;
    }
    
    // Check if clock is running
    if (clock.isRunning()) {
        std::cout << "✓ Clock is running" << std::endl;
    } else {
        std::cout << "✗ Clock failed to start!" << std::endl;
    }
    
    // Simulate some processing
    std::cout << "\nProcessing 10 blocks..." << std::endl;
    for (int i = 0; i < 10; ++i) {
        clock.processBlock(44100.0, 512);
        
        // Print position every 2 blocks
        if (i % 2 == 0) {
            std::cout << "Bar: " << clock.getCurrentBar() 
                     << " Beat: " << clock.getCurrentBeat()
                     << " Pulse: " << clock.getCurrentPulse() << std::endl;
        }
        
        // Small delay to simulate real-time
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Test stop
    std::cout << "\nStopping transport..." << std::endl;
    transport.stop();
    
    if (!transport.isPlaying()) {
        std::cout << "✓ Transport stopped" << std::endl;
    } else {
        std::cout << "✗ Transport failed to stop!" << std::endl;
    }
    
    if (!clock.isRunning()) {
        std::cout << "✓ Clock stopped" << std::endl;
    } else {
        std::cout << "✗ Clock failed to stop!" << std::endl;
    }
    
    std::cout << "\n=== Test Complete ===" << std::endl;
    
    return 0;
}