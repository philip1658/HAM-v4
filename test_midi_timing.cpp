/*
  ==============================================================================

    test_midi_timing.cpp
    Test for verifying MIDI note on/off timing in division 1

  ==============================================================================
*/

#include <iostream>
#include <vector>
#include <iomanip>
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "Source/Domain/Models/Track.h"
#include "Source/Domain/Models/Stage.h"
#include "Source/Domain/Models/Pattern.h"
#include "Source/Domain/Clock/MasterClock.h"
#include "Source/Domain/Engines/SequencerEngine.h"
#include "Source/Domain/Processors/MidiEventGenerator.h"

using namespace HAM;

class MidiTimingTest : public juce::AudioProcessor
{
public:
    MidiTimingTest()
        : AudioProcessor(BusesProperties()
            .withInput("Input", juce::AudioChannelSet::stereo(), true)
            .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    {
        setupTestPattern();
    }

    void prepareToPlay(double sampleRate, int samplesPerBlock) override
    {
        m_sampleRate = sampleRate;
        m_samplesPerBlock = samplesPerBlock;
        
        // Initialize clock
        m_clock.setSampleRate(sampleRate);
        m_clock.setBufferSize(samplesPerBlock);
        m_clock.setBpm(120.0); // 120 BPM = 0.5 seconds per beat
        
        // Initialize sequencer
        m_sequencer.prepareToPlay(sampleRate, samplesPerBlock);
        
        std::cout << "\n=== MIDI Timing Test Setup ===" << std::endl;
        std::cout << "Sample Rate: " << sampleRate << " Hz" << std::endl;
        std::cout << "Buffer Size: " << samplesPerBlock << " samples" << std::endl;
        std::cout << "BPM: 120 (0.5 seconds per beat)" << std::endl;
        
        // Calculate timing expectations
        double samplesPerBeat = (60.0 / 120.0) * sampleRate; // 24000 samples at 48kHz
        double samplesPerPulse = samplesPerBeat / 24.0; // 1000 samples per pulse at 24 PPQN
        
        std::cout << "Samples per beat: " << samplesPerBeat << std::endl;
        std::cout << "Samples per pulse (24 PPQN): " << samplesPerPulse << std::endl;
        std::cout << "Expected note duration (division 1): " << samplesPerBeat << " samples" << std::endl;
        std::cout << "==============================\n" << std::endl;
    }

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override
    {
        buffer.clear();
        midiMessages.clear();
        
        // Process one block through the sequencer
        auto events = m_sequencer.processBlock(m_samplesPerBlock);
        
        // Track MIDI events
        for (const auto& event : events)
        {
            midiMessages.addEvent(event.message, event.sampleOffset);
            
            // Log the event with timing info
            MidiEventInfo info;
            info.timestamp = m_totalSamples + event.sampleOffset;
            info.message = event.message;
            info.sampleOffset = event.sampleOffset;
            info.trackIndex = event.trackIndex;
            info.stageIndex = event.stageIndex;
            
            m_capturedEvents.push_back(info);
            
            // Print real-time event info
            if (event.message.isNoteOn())
            {
                std::cout << "[" << std::setw(8) << info.timestamp << " samples] "
                          << "NOTE ON  - Track " << event.trackIndex 
                          << ", Stage " << event.stageIndex
                          << ", Note " << event.message.getNoteNumber()
                          << ", Vel " << (int)event.message.getVelocity()
                          << ", Offset " << event.sampleOffset << std::endl;
            }
            else if (event.message.isNoteOff())
            {
                std::cout << "[" << std::setw(8) << info.timestamp << " samples] "
                          << "NOTE OFF - Track " << event.trackIndex 
                          << ", Stage " << event.stageIndex
                          << ", Note " << event.message.getNoteNumber()
                          << ", Offset " << event.sampleOffset << std::endl;
            }
        }
        
        m_totalSamples += m_samplesPerBlock;
        
        // Run analysis after processing some blocks
        if (m_totalSamples >= m_sampleRate * 4) // After 4 seconds
        {
            analyzeTimings();
            m_totalSamples = 0; // Reset for next analysis
            m_capturedEvents.clear();
        }
        
        // Advance clock
        m_clock.processBlock(m_samplesPerBlock);
    }

    const juce::String getName() const override { return "MidiTimingTest"; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}
    void releaseResources() override {}
    double getTailLengthSeconds() const override { return 0.0; }

private:
    void setupTestPattern()
    {
        // Create a simple pattern with division 1
        auto pattern = std::make_unique<Pattern>();
        pattern->setBpm(120.0);
        
        // Create track with division 1
        auto track = std::make_unique<Track>();
        track->setMidiChannel(1);
        track->setDivision(1); // Division 1 - one note per beat
        track->setGateLength(0.8f); // 80% gate length
        
        // Setup 8 stages with simple pitches
        for (int i = 0; i < 8; ++i)
        {
            Stage stage;
            stage.setPitch(60 + i); // C4, C#4, D4, etc.
            stage.setVelocity(100);
            stage.setGateType(static_cast<int>(GateType::MULTIPLE));
            stage.setPulseCount(1); // 1 pulse per stage for division 1
            stage.setRatchetCount(0, 1); // No ratchets
            stage.setProbability(100.0f); // Always trigger
            track->setStage(i, stage);
        }
        
        pattern->addTrack(std::move(track));
        m_sequencer.setPattern(std::move(pattern));
    }

    void analyzeTimings()
    {
        std::cout << "\n=== TIMING ANALYSIS (Division 1) ===" << std::endl;
        
        if (m_capturedEvents.size() < 2)
        {
            std::cout << "Not enough events captured for analysis" << std::endl;
            return;
        }
        
        // Group events by note number
        std::map<int, std::vector<MidiEventInfo>> noteEvents;
        for (const auto& event : m_capturedEvents)
        {
            int note = event.message.getNoteNumber();
            noteEvents[note].push_back(event);
        }
        
        // Analyze each note's on/off pairs
        std::cout << "\nNote On/Off Pair Analysis:" << std::endl;
        std::cout << "----------------------------------------" << std::endl;
        
        double expectedNoteDuration = (60.0 / 120.0) * m_sampleRate * 0.8; // 80% gate length
        double expectedNoteSpacing = (60.0 / 120.0) * m_sampleRate; // Full beat
        
        for (auto& [note, events] : noteEvents)
        {
            std::cout << "\nNote " << note << " (MIDI " << juce::MidiMessage::getMidiNoteName(note, true, true, 4) << "):" << std::endl;
            
            // Find on/off pairs
            for (size_t i = 0; i < events.size() - 1; ++i)
            {
                if (events[i].message.isNoteOn() && events[i + 1].message.isNoteOff())
                {
                    int64_t duration = events[i + 1].timestamp - events[i].timestamp;
                    double durationMs = (duration / m_sampleRate) * 1000.0;
                    double expectedMs = (expectedNoteDuration / m_sampleRate) * 1000.0;
                    double error = durationMs - expectedMs;
                    
                    std::cout << "  On->Off: " << duration << " samples (" 
                              << std::fixed << std::setprecision(2) << durationMs << " ms)"
                              << " | Expected: " << expectedMs << " ms"
                              << " | Error: " << (error > 0 ? "+" : "") << error << " ms";
                    
                    if (std::abs(error) > 1.0) // More than 1ms error
                    {
                        std::cout << " ⚠️ TIMING ERROR";
                    }
                    else
                    {
                        std::cout << " ✓";
                    }
                    std::cout << std::endl;
                }
            }
            
            // Check spacing between consecutive note-ons
            std::cout << "\n  Note spacing:" << std::endl;
            int64_t lastOnTime = -1;
            for (const auto& event : events)
            {
                if (event.message.isNoteOn())
                {
                    if (lastOnTime >= 0)
                    {
                        int64_t spacing = event.timestamp - lastOnTime;
                        double spacingMs = (spacing / m_sampleRate) * 1000.0;
                        double expectedSpacingMs = (expectedNoteSpacing / m_sampleRate) * 1000.0;
                        double error = spacingMs - expectedSpacingMs;
                        
                        std::cout << "  Between notes: " << spacing << " samples (" 
                                  << std::fixed << std::setprecision(2) << spacingMs << " ms)"
                                  << " | Expected: " << expectedSpacingMs << " ms"
                                  << " | Error: " << (error > 0 ? "+" : "") << error << " ms";
                        
                        if (std::abs(error) > 1.0) // More than 1ms error
                        {
                            std::cout << " ⚠️ SPACING ERROR";
                        }
                        else
                        {
                            std::cout << " ✓";
                        }
                        std::cout << std::endl;
                    }
                    lastOnTime = event.timestamp;
                }
            }
        }
        
        // Overall statistics
        std::cout << "\n=== OVERALL STATISTICS ===" << std::endl;
        std::cout << "Total events captured: " << m_capturedEvents.size() << std::endl;
        
        int noteOns = 0, noteOffs = 0;
        for (const auto& event : m_capturedEvents)
        {
            if (event.message.isNoteOn()) noteOns++;
            if (event.message.isNoteOff()) noteOffs++;
        }
        
        std::cout << "Note ONs: " << noteOns << std::endl;
        std::cout << "Note OFFs: " << noteOffs << std::endl;
        
        if (noteOns != noteOffs)
        {
            std::cout << "⚠️ WARNING: Unmatched note on/off counts!" << std::endl;
        }
        else
        {
            std::cout << "✓ Note on/off counts match" << std::endl;
        }
        
        std::cout << "==================================\n" << std::endl;
    }

    struct MidiEventInfo
    {
        int64_t timestamp;
        juce::MidiMessage message;
        int sampleOffset;
        int trackIndex;
        int stageIndex;
    };

    MasterClock m_clock;
    SequencerEngine m_sequencer;
    double m_sampleRate = 48000.0;
    int m_samplesPerBlock = 128;
    int64_t m_totalSamples = 0;
    std::vector<MidiEventInfo> m_capturedEvents;
};

// Test harness
int main()
{
    std::cout << "Starting HAM MIDI Timing Test for Division 1..." << std::endl;
    
    // Initialize JUCE
    juce::ScopedJuceInitialiser_GUI juceInit;
    
    // Create test processor
    MidiTimingTest testProcessor;
    
    // Prepare processor
    testProcessor.prepareToPlay(48000.0, 128);
    
    // Create buffers
    juce::AudioBuffer<float> audioBuffer(2, 128);
    juce::MidiBuffer midiBuffer;
    
    // Process several blocks to capture timing data
    std::cout << "\nProcessing audio blocks..." << std::endl;
    for (int i = 0; i < 200; ++i) // Process 200 blocks (about 4 seconds at 48kHz/128)
    {
        testProcessor.processBlock(audioBuffer, midiBuffer);
    }
    
    std::cout << "\nTest complete!" << std::endl;
    return 0;
}