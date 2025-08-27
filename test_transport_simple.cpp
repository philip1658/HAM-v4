/*
  ==============================================================================

    test_transport_simple.cpp
    Simple test to diagnose transport/clock issues

  ==============================================================================
*/

#include <iostream>
#include <thread>
#include <chrono>

// Forward declarations to check basic functionality
namespace HAM {
    class MasterClock;
    class Transport;
    class SequencerEngine;
}

int main()
{
    std::cout << "\n========== TRANSPORT DIAGNOSTIC TEST ==========\n" << std::endl;
    
    // Test the app binary directly
    std::cout << "Testing CloneHAM app transport system...\n" << std::endl;
    
    // Launch the app with test parameters
    system("echo 'Testing transport...' > /tmp/ham_test.log");
    
    // Use otool to check if symbols are present
    std::cout << "Checking for transport symbols in app binary:" << std::endl;
    system("otool -L ~/Desktop/CloneHAM.app/Contents/MacOS/CloneHAM | head -5");
    
    std::cout << "\nChecking for Transport::play symbol:" << std::endl;
    system("nm ~/Desktop/CloneHAM.app/Contents/MacOS/CloneHAM 2>/dev/null | grep 'Transport.*play' | head -3");
    
    std::cout << "\nChecking for MasterClock::start symbol:" << std::endl;
    system("nm ~/Desktop/CloneHAM.app/Contents/MacOS/CloneHAM 2>/dev/null | grep 'MasterClock.*start' | head -3");
    
    std::cout << "\nChecking for processBlock symbols:" << std::endl;
    system("nm ~/Desktop/CloneHAM.app/Contents/MacOS/CloneHAM 2>/dev/null | grep 'processBlock' | head -5");
    
    // Test with simple MIDI output check
    std::cout << "\n========== MIDI OUTPUT TEST ==========\n" << std::endl;
    std::cout << "Starting app and checking for MIDI activity..." << std::endl;
    
    // Start the app in background, let it run briefly, then kill it
    system("(~/Desktop/CloneHAM.app/Contents/MacOS/CloneHAM &) && sleep 2 && pkill CloneHAM");
    
    // Check if any log output was generated
    std::cout << "\nChecking system logs for HAM activity:" << std::endl;
    system("log show --predicate 'process == \"CloneHAM\"' --last 1m 2>/dev/null | grep -E '(Transport|Clock|play|start)' | head -10");
    
    std::cout << "\n==========================================\n" << std::endl;
    
    return 0;
}