/*
  ==============================================================================

    MidiTimingAnalyzer.h
    Analyzes MIDI timing for note on/off events

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <map>
#include <iostream>
#include <iomanip>

namespace HAM
{

class MidiTimingAnalyzer
{
public:
    struct MidiEvent
    {
        int64_t timestamp;          // Absolute sample position
        juce::MidiMessage message;
        int trackIndex;
        int stageIndex;
        int ratchetIndex;
    };
    
    struct TimingReport
    {
        double averageNoteDuration;
        double expectedNoteDuration;
        double maxTimingError;
        int unmatchedNoteOns;
        int unmatchedNoteOffs;
        bool hasTimingIssues;
        std::vector<std::string> issues;
    };
    
    MidiTimingAnalyzer(double sampleRate = 48000.0, double bpm = 120.0)
        : m_sampleRate(sampleRate)
        , m_bpm(bpm)
    {
    }
    
    void reset()
    {
        m_events.clear();
        m_currentSample = 0;
    }
    
    void addEvent(const juce::MidiMessage& message, int sampleOffset, 
                  int trackIndex = 0, int stageIndex = 0, int ratchetIndex = 0)
    {
        MidiEvent event;
        event.timestamp = m_currentSample + sampleOffset;
        event.message = message;
        event.trackIndex = trackIndex;
        event.stageIndex = stageIndex;
        event.ratchetIndex = ratchetIndex;
        m_events.push_back(event);
    }
    
    void advanceTime(int samples)
    {
        m_currentSample += samples;
    }
    
    TimingReport analyzeTimingForDivision(int division, float gateLength = 0.8f)
    {
        TimingReport report;
        report.hasTimingIssues = false;
        report.maxTimingError = 0.0;
        report.unmatchedNoteOns = 0;
        report.unmatchedNoteOffs = 0;
        
        // Calculate expected timings based on division
        double beatsPerBar = 4.0;
        double samplesPerBeat = (60.0 / m_bpm) * m_sampleRate;
        double notesPerBar = beatsPerBar * division;
        double samplesPerNote = (beatsPerBar * samplesPerBeat) / notesPerBar;
        double expectedDuration = samplesPerNote * gateLength;
        
        report.expectedNoteDuration = expectedDuration;
        
        // Group events by note number
        std::map<int, std::vector<MidiEvent>> noteGroups;
        for (const auto& event : m_events)
        {
            noteGroups[event.message.getNoteNumber()].push_back(event);
        }
        
        // Analyze each note's timing
        std::vector<double> durations;
        std::vector<double> spacings;
        
        for (auto& [noteNum, events] : noteGroups)
        {
            // Find note on/off pairs
            for (size_t i = 0; i < events.size(); ++i)
            {
                if (events[i].message.isNoteOn())
                {
                    // Find matching note off
                    bool foundOff = false;
                    for (size_t j = i + 1; j < events.size(); ++j)
                    {
                        if (events[j].message.isNoteOff())
                        {
                            foundOff = true;
                            double duration = events[j].timestamp - events[i].timestamp;
                            durations.push_back(duration);
                            
                            double error = std::abs(duration - expectedDuration);
                            if (error > report.maxTimingError)
                            {
                                report.maxTimingError = error;
                            }
                            
                            // Check for timing issues
                            double errorMs = (error / m_sampleRate) * 1000.0;
                            if (errorMs > 1.0) // More than 1ms error
                            {
                                report.hasTimingIssues = true;
                                std::stringstream ss;
                                ss << "Note " << noteNum << " duration error: " 
                                   << std::fixed << std::setprecision(2) << errorMs << "ms";
                                report.issues.push_back(ss.str());
                            }
                            break;
                        }
                    }
                    
                    if (!foundOff)
                    {
                        report.unmatchedNoteOns++;
                        report.hasTimingIssues = true;
                        std::stringstream ss;
                        ss << "Note " << noteNum << " has unmatched note ON at sample " 
                           << events[i].timestamp;
                        report.issues.push_back(ss.str());
                    }
                }
            }
            
            // Check note spacing
            int64_t lastOnTime = -1;
            for (const auto& event : events)
            {
                if (event.message.isNoteOn())
                {
                    if (lastOnTime >= 0)
                    {
                        double spacing = event.timestamp - lastOnTime;
                        spacings.push_back(spacing);
                        
                        double expectedSpacing = samplesPerNote;
                        double spacingError = std::abs(spacing - expectedSpacing);
                        double spacingErrorMs = (spacingError / m_sampleRate) * 1000.0;
                        
                        if (spacingErrorMs > 1.0)
                        {
                            report.hasTimingIssues = true;
                            std::stringstream ss;
                            ss << "Note " << noteNum << " spacing error: " 
                               << std::fixed << std::setprecision(2) << spacingErrorMs << "ms";
                            report.issues.push_back(ss.str());
                        }
                    }
                    lastOnTime = event.timestamp;
                }
            }
        }
        
        // Calculate average duration
        if (!durations.empty())
        {
            double sum = 0;
            for (double d : durations) sum += d;
            report.averageNoteDuration = sum / durations.size();
        }
        else
        {
            report.averageNoteDuration = 0;
        }
        
        // Count unmatched note offs
        int totalNoteOns = 0, totalNoteOffs = 0;
        for (const auto& event : m_events)
        {
            if (event.message.isNoteOn()) totalNoteOns++;
            if (event.message.isNoteOff()) totalNoteOffs++;
        }
        
        if (totalNoteOffs > totalNoteOns)
        {
            report.unmatchedNoteOffs = totalNoteOffs - totalNoteOns;
            report.hasTimingIssues = true;
            report.issues.push_back("More note OFFs than note ONs detected");
        }
        
        return report;
    }
    
    void printDetailedReport(int division = 1)
    {
        std::cout << "\n=== MIDI TIMING ANALYSIS (Division " << division << ") ===" << std::endl;
        std::cout << "BPM: " << m_bpm << " | Sample Rate: " << m_sampleRate << " Hz" << std::endl;
        std::cout << "Total Events: " << m_events.size() << std::endl;
        
        auto report = analyzeTimingForDivision(division);
        
        std::cout << "\n--- Timing Report ---" << std::endl;
        std::cout << "Expected Note Duration: " << report.expectedNoteDuration 
                  << " samples (" << (report.expectedNoteDuration / m_sampleRate * 1000.0) 
                  << " ms)" << std::endl;
        std::cout << "Average Note Duration: " << report.averageNoteDuration 
                  << " samples (" << (report.averageNoteDuration / m_sampleRate * 1000.0) 
                  << " ms)" << std::endl;
        std::cout << "Max Timing Error: " << report.maxTimingError 
                  << " samples (" << (report.maxTimingError / m_sampleRate * 1000.0) 
                  << " ms)" << std::endl;
        
        if (report.unmatchedNoteOns > 0)
        {
            std::cout << "⚠️  Unmatched Note ONs: " << report.unmatchedNoteOns << std::endl;
        }
        if (report.unmatchedNoteOffs > 0)
        {
            std::cout << "⚠️  Unmatched Note OFFs: " << report.unmatchedNoteOffs << std::endl;
        }
        
        if (report.hasTimingIssues)
        {
            std::cout << "\n⚠️  TIMING ISSUES DETECTED:" << std::endl;
            for (const auto& issue : report.issues)
            {
                std::cout << "  - " << issue << std::endl;
            }
        }
        else
        {
            std::cout << "\n✅ All timings within tolerance (< 1ms error)" << std::endl;
        }
        
        std::cout << "================================\n" << std::endl;
    }
    
private:
    double m_sampleRate;
    double m_bpm;
    int64_t m_currentSample = 0;
    std::vector<MidiEvent> m_events;
};

} // namespace HAM