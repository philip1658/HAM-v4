#!/usr/bin/env swift

import Foundation
import ApplicationServices
import Cocoa

// Test script for HAM MIDI routing functionality
print("HAM MIDI Routing Test Starting...")

// Wait for HAM to fully load
Thread.sleep(forTimeInterval: 2.0)

// Function to find HAM application
func findHAMApp() -> NSRunningApplication? {
    let apps = NSRunningApplication.runningApplications(withBundleIdentifier: "com.yourcompany.CloneHAM")
    return apps.first
}

// Test the MIDI routing functionality
func testMIDIRouting() {
    print("Looking for HAM application...")
    
    guard let hamApp = findHAMApp() else {
        print("HAM application not found. Please ensure it's running.")
        return
    }
    
    print("Found HAM application: \(hamApp.localizedName ?? "Unknown")")
    
    // In a real test, we would:
    // 1. Find and interact with UI elements
    // 2. Change MIDI routing modes
    // 3. Start playback
    // 4. Verify MIDI output
    
    print("HAM is running with PID: \(hamApp.processIdentifier)")
    print("To manually test:")
    print("1. In HAM, find the TrackSidebar")
    print("2. Locate the MIDI routing dropdown below the channel selector")
    print("3. Change from 'Plugin Only' to 'External Only'")
    print("4. Start playback to generate MIDI output")
    print("5. Monitor external MIDI with: ~/bin/receivemidi dev \"IAC-Treiber Bus 1\" ts")
}

testMIDIRouting()
print("Test script completed.")