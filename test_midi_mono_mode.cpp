/*
  ==============================================================================

    test_midi_mono_mode.cpp
    Automated MIDI testing for HAM mono mode behavior
    Verifies against expected timing and message patterns
    
  ==============================================================================
*/

#include <JuceHeader.h>
#include <chrono>
#include <vector>
#include <fstream>
#include <iostream>
#include <iomanip>

using namespace std::chrono;

//==============================================================================
struct MidiEventCapture
{
    juce::MidiMessage message;
    high_resolution_clock::time_point timestamp;
    double timeFromStart;
    int sampleOffset;
    
    bool isNoteOn() const { return message.isNoteOn(); }
    bool isNoteOff() const { return message.isNoteOff(); }
    int getNoteNumber() const { return message.getNoteNumber(); }
    int getVelocity() const { return message.getVelocity(); }
    int getChannel() const { return message.getChannel(); }
    
    std::string toString() const 
    {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(3);
        oss << timeFromStart << "s: ";
        
        if (isNoteOn())
            oss << "NOTE ON  Ch:" << getChannel() << " Note:" << getNoteNumber() 
                << " Vel:" << getVelocity();
        else if (isNoteOff())
            oss << "NOTE OFF Ch:" << getChannel() << " Note:" << getNoteNumber();
        else
            oss << "CC/Other Ch:" << getChannel() << " Data:" << message.getDescription();
            
        return oss.str();
    }
};

//==============================================================================
class MidiCaptureCallback : public juce::MidiInputCallback
{
public:
    MidiCaptureCallback() : startTime(high_resolution_clock::now()) {}
    
    void handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message) override
    {
        auto now = high_resolution_clock::now();
        auto elapsed = duration_cast<microseconds>(now - startTime);
        double timeFromStart = elapsed.count() / 1000000.0; // Convert to seconds
        
        MidiEventCapture capture;
        capture.message = message;
        capture.timestamp = now;
        capture.timeFromStart = timeFromStart;
        capture.sampleOffset = static_cast<int>(timeFromStart * 48000.0); // Assume 48kHz
        
        std::lock_guard<std::mutex> lock(eventsMutex);
        capturedEvents.push_back(capture);
        
        // Real-time console output for immediate feedback
        std::cout << capture.toString() << std::endl;
    }
    
    std::vector<MidiEventCapture> getEvents()
    {
        std::lock_guard<std::mutex> lock(eventsMutex);
        return capturedEvents;
    }
    
    void clearEvents()
    {
        std::lock_guard<std::mutex> lock(eventsMutex);
        capturedEvents.clear();
        startTime = high_resolution_clock::now();
    }
    
    void resetTimer()
    {
        startTime = high_resolution_clock::now();
    }
    
private:
    std::vector<MidiEventCapture> capturedEvents;
    std::mutex eventsMutex;
    high_resolution_clock::time_point startTime;
};

//==============================================================================
class MidiMonoModeTest
{
public:
    MidiMonoModeTest()
    {
        // Initialize MIDI
        auto midiInputs = juce::MidiInput::getAvailableDevices();
        
        std::cout << "\n=== Available MIDI Input Devices ===" << std::endl;
        for (int i = 0; i < midiInputs.size(); ++i)
        {
            std::cout << i << ": " << midiInputs[i].name << std::endl;
        }
        
        // Look for HAM or default MIDI input
        int selectedDevice = -1;
        for (int i = 0; i < midiInputs.size(); ++i)
        {
            if (midiInputs[i].name.containsIgnoreCase("HAM") || 
                midiInputs[i].name.containsIgnoreCase("IAC"))
            {
                selectedDevice = i;
                break;
            }
        }
        
        if (selectedDevice == -1 && midiInputs.size() > 0)
        {
            selectedDevice = 0; // Use first available
        }
        
        if (selectedDevice >= 0)
        {
            midiInput = juce::MidiInput::openDevice(midiInputs[selectedDevice].identifier, &callback);
            if (midiInput)
            {
                midiInput->start();
                std::cout << "✓ Listening on: " << midiInputs[selectedDevice].name << std::endl;
            }
            else
            {
                std::cout << "✗ Failed to open MIDI device" << std::endl;
            }
        }
        else
        {
            std::cout << "✗ No MIDI devices available" << std::endl;
        }
    }
    
    ~MidiMonoModeTest()
    {
        if (midiInput)
        {
            midiInput->stop();
            midiInput.reset();
        }
    }
    
    void runTest(double testDurationSeconds = 10.0)
    {
        std::cout << "\n=== Starting MIDI Mono Mode Test ===" << std::endl;
        std::cout << "Duration: " << testDurationSeconds << " seconds" << std::endl;
        std::cout << "Expected: 8 stages @ 120 BPM = 4 second loop" << std::endl;
        std::cout << "Listening for MIDI messages..." << std::endl;
        std::cout << "----------------------------------------" << std::endl;
        
        callback.clearEvents();
        callback.resetTimer();
        
        // Wait for test duration
        auto startTime = high_resolution_clock::now();
        auto endTime = startTime + duration_cast<microseconds>(duration<double>(testDurationSeconds));
        
        while (high_resolution_clock::now() < endTime)
        {
            std::this_thread::sleep_for(milliseconds(10));
        }
        
        // Analyze results
        analyzeResults();
    }
    
private:
    void analyzeResults()
    {
        auto events = callback.getEvents();
        
        std::cout << "\n=== MIDI Capture Analysis ===" << std::endl;
        std::cout << "Total events captured: " << events.size() << std::endl;
        
        if (events.empty())
        {
            std::cout << "⚠️  No MIDI events captured!" << std::endl;
            std::cout << "Check that HAM is running and MIDI routing is correct." << std::endl;
            return;
        }
        
        // Separate note on/off events
        std::vector<MidiEventCapture> noteOnEvents;
        std::vector<MidiEventCapture> noteOffEvents;
        
        for (const auto& event : events)
        {
            if (event.isNoteOn())
                noteOnEvents.push_back(event);
            else if (event.isNoteOff())
                noteOffEvents.push_back(event);
        }
        
        std::cout << "Note ON events: " << noteOnEvents.size() << std::endl;
        std::cout << "Note OFF events: " << noteOffEvents.size() << std::endl;
        
        // Analyze timing patterns
        analyzeMonoModeBehavior(noteOnEvents, noteOffEvents);
        
        // Generate detailed report
        generateReport(events);
    }
    
    void analyzeMonoModeBehavior(const std::vector<MidiEventCapture>& noteOns, 
                                const std::vector<MidiEventCapture>& noteOffs)
    {
        std::cout << "\n=== Mono Mode Behavior Analysis ===" << std::endl;
        
        // Check for overlapping notes (should not happen in mono mode)
        bool foundOverlaps = false;
        for (size_t i = 0; i < noteOns.size() - 1; ++i)
        {
            double noteOnTime = noteOns[i].timeFromStart;
            double nextNoteOnTime = noteOns[i + 1].timeFromStart;
            
            // Find corresponding note off
            auto noteOffIt = std::find_if(noteOffs.begin(), noteOffs.end(),
                [noteOnTime](const MidiEventCapture& event) {
                    return event.timeFromStart > noteOnTime;
                });
            
            if (noteOffIt != noteOffs.end())
            {
                double noteOffTime = noteOffIt->timeFromStart;
                if (noteOffTime > nextNoteOnTime)
                {
                    foundOverlaps = true;
                    std::cout << "⚠️  Overlap detected: Note ON " << nextNoteOnTime 
                              << "s before previous OFF " << noteOffTime << "s" << std::endl;
                }
            }
        }
        
        if (!foundOverlaps)
        {
            std::cout << "✓ No overlapping notes detected - Mono behavior correct" << std::endl;
        }
        
        // Analyze stage timing (should be 0.5s intervals @ 120 BPM)
        if (noteOns.size() >= 2)
        {
            std::vector<double> intervals;
            for (size_t i = 1; i < noteOns.size(); ++i)
            {
                double interval = noteOns[i].timeFromStart - noteOns[i-1].timeFromStart;
                intervals.push_back(interval);
            }
            
            double avgInterval = 0.0;
            for (double interval : intervals)
            {
                avgInterval += interval;
            }
            avgInterval /= intervals.size();
            
            double expectedInterval = 0.5; // 0.5s @ 120 BPM, quarter note division
            double tolerance = 0.005; // 5ms tolerance
            
            std::cout << "Average stage interval: " << std::fixed << std::setprecision(3) 
                      << avgInterval << "s (expected: " << expectedInterval << "s)" << std::endl;
            
            if (std::abs(avgInterval - expectedInterval) < tolerance)
            {
                std::cout << "✓ Stage timing within tolerance" << std::endl;
            }
            else
            {
                std::cout << "⚠️  Stage timing deviation: " 
                          << (avgInterval - expectedInterval) * 1000.0 << "ms" << std::endl;
            }
        }
        
        // Analyze gate length (should be 50% = 0.25s)
        if (noteOns.size() > 0 && noteOffs.size() > 0)
        {
            std::vector<double> gateLengths;
            for (const auto& noteOn : noteOns)
            {
                auto noteOffIt = std::find_if(noteOffs.begin(), noteOffs.end(),
                    [&noteOn](const MidiEventCapture& event) {
                        return event.timeFromStart > noteOn.timeFromStart &&
                               event.getNoteNumber() == noteOn.getNoteNumber() &&
                               event.getChannel() == noteOn.getChannel();
                    });
                
                if (noteOffIt != noteOffs.end())
                {
                    double gateLength = noteOffIt->timeFromStart - noteOn.timeFromStart;
                    gateLengths.push_back(gateLength);
                }
            }
            
            if (!gateLengths.empty())
            {
                double avgGateLength = 0.0;
                for (double length : gateLengths)
                {
                    avgGateLength += length;
                }
                avgGateLength /= gateLengths.size();
                
                double expectedGateLength = 0.25; // 50% of 0.5s
                std::cout << "Average gate length: " << std::fixed << std::setprecision(3)
                          << avgGateLength << "s (expected: " << expectedGateLength << "s)" << std::endl;
                
                if (std::abs(avgGateLength - expectedGateLength) < 0.01)
                {
                    std::cout << "✓ Gate length within tolerance" << std::endl;
                }
                else
                {
                    std::cout << "⚠️  Gate length deviation: " 
                              << (avgGateLength - expectedGateLength) * 1000.0 << "ms" << std::endl;
                }
            }
        }
    }
    
    void generateReport(const std::vector<MidiEventCapture>& events)
    {
        std::string filename = "/Users/philipkrieger/Desktop/Clone_Ham/HAM/midi_test_results.txt";
        std::ofstream report(filename);
        
        if (!report.is_open())
        {
            std::cout << "⚠️  Could not create report file: " << filename << std::endl;
            return;
        }
        
        report << "HAM Mono Mode MIDI Test Results\n";
        report << "Generated: " << std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) << "\n";
        report << "========================================\n\n";
        
        report << "Total Events: " << events.size() << "\n\n";
        
        report << "Detailed Event Log:\n";
        report << "Time      | Type     | Ch | Note | Vel | Sample Offset\n";
        report << "----------|----------|----|----- |-----|-------------\n";
        
        for (const auto& event : events)
        {
            report << std::fixed << std::setprecision(3) << std::setw(8) << event.timeFromStart << "s | ";
            
            if (event.isNoteOn())
                report << "NOTE ON  | ";
            else if (event.isNoteOff())
                report << "NOTE OFF | ";
            else
                report << "OTHER    | ";
                
            report << std::setw(2) << event.getChannel() << " | ";
            report << std::setw(4) << (event.isNoteOn() || event.isNoteOff() ? event.getNoteNumber() : 0) << " | ";
            report << std::setw(3) << (event.isNoteOn() ? event.getVelocity() : 0) << " | ";
            report << std::setw(8) << event.sampleOffset << "\n";
        }
        
        report.close();
        std::cout << "\n✓ Detailed report saved to: " << filename << std::endl;
    }
    
    std::unique_ptr<juce::MidiInput> midiInput;
    MidiCaptureCallback callback;
};

//==============================================================================
int main()
{
    std::cout << "HAM Mono Mode MIDI Analyzer v1.0" << std::endl;
    std::cout << "=================================" << std::endl;
    
    // Initialize JUCE
    juce::ScopedJuceInitialiser_GUI juceInitialiser;
    
    MidiMonoModeTest test;
    
    std::cout << "\nInstructions:" << std::endl;
    std::cout << "1. Start HAM application" << std::endl;
    std::cout << "2. Set mono mode (should be default)" << std::endl;
    std::cout << "3. Press PLAY in HAM" << std::endl;
    std::cout << "4. This test will capture MIDI for 10 seconds" << std::endl;
    std::cout << "\nPress Enter to start test..." << std::endl;
    std::cin.get();
    
    test.runTest(10.0);
    
    std::cout << "\nTest completed. Check results above." << std::endl;
    std::cout << "Press Enter to exit..." << std::endl;
    std::cin.get();
    
    return 0;
}