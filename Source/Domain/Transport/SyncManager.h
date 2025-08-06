/*
  ==============================================================================

    SyncManager.h
    Manages synchronization between different clock sources
    Handles MIDI Clock, Ableton Link, MTC, and Host sync

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../Clock/MasterClock.h"
#include "Transport.h"
#include <atomic>
#include <memory>

namespace HAM {

//==============================================================================
/**
    Central sync manager for coordinating different clock sources.
    Handles MIDI Clock in/out, prepares for Ableton Link, MTC, etc.
*/
class SyncManager : public MasterClock::Listener,
                    private juce::MidiInputCallback
{
public:
    //==========================================================================
    // MIDI Clock Constants
    static constexpr uint8_t MIDI_CLOCK_TICK = 0xF8;      // 24 ppq
    static constexpr uint8_t MIDI_CLOCK_START = 0xFA;     // Start from beginning
    static constexpr uint8_t MIDI_CLOCK_CONTINUE = 0xFB;  // Continue from position
    static constexpr uint8_t MIDI_CLOCK_STOP = 0xFC;      // Stop
    static constexpr uint8_t MIDI_SONG_POSITION = 0xF2;   // Song position pointer
    
    //==========================================================================
    // Sync Status
    struct Status
    {
        bool isReceivingMidiClock = false;
        bool isSendingMidiClock = false;
        bool isLinkEnabled = false;
        bool isLinkConnected = false;
        int linkPeerCount = 0;
        bool isMTCEnabled = false;
        bool isHostSyncEnabled = false;
        
        // Timing info
        float externalBPM = 0.0f;
        float internalBPM = 120.0f;
        double clockDrift = 0.0;  // In milliseconds
        int droppedClocks = 0;
        int totalClocksReceived = 0;
    };
    
    //==========================================================================
    // Construction
    SyncManager(MasterClock& masterClock, Transport& transport);
    ~SyncManager() override;
    
    //==========================================================================
    // MIDI Clock Input
    
    /** Enable/disable MIDI clock input */
    void setMidiClockInputEnabled(bool enabled);
    
    /** Process incoming MIDI message for clock */
    void processMidiMessage(const juce::MidiMessage& message);
    
    /** Get current MIDI input device */
    juce::String getMidiInputDevice() const { return m_midiInputDevice; }
    
    /** Set MIDI input device */
    void setMidiInputDevice(const juce::String& deviceName);
    
    //==========================================================================
    // MIDI Clock Output
    
    /** Enable/disable MIDI clock output */
    void setMidiClockOutputEnabled(bool enabled);
    
    /** Check if sending MIDI clock */
    bool isSendingMidiClock() const { return m_sendMidiClock; }
    
    /** Get current MIDI output device */
    juce::String getMidiOutputDevice() const { return m_midiOutputDevice; }
    
    /** Set MIDI output device */
    void setMidiOutputDevice(const juce::String& deviceName);
    
    /** Send MIDI start message */
    void sendMidiStart();
    
    /** Send MIDI stop message */
    void sendMidiStop();
    
    /** Send MIDI continue message */
    void sendMidiContinue();
    
    /** Send song position pointer */
    void sendSongPosition(int sixteenthNotes);
    
    //==========================================================================
    // Ableton Link (Preparation)
    
    /** Check if Link is available */
    bool isLinkAvailable() const { return false; }  // TODO: Implement with Link SDK
    
    /** Enable/disable Ableton Link */
    void setLinkEnabled(bool enabled);
    
    /** Check if Link is enabled */
    bool isLinkEnabled() const { return m_linkEnabled; }
    
    /** Get number of Link peers */
    int getLinkPeerCount() const { return 0; }  // TODO: Implement
    
    /** Force Link phase alignment */
    void alignLinkPhase();
    
    //==========================================================================
    // MTC (MIDI Time Code) - Future
    
    /** Enable/disable MTC */
    void setMTCEnabled(bool enabled) { m_mtcEnabled = enabled; }
    
    /** Check if MTC is enabled */
    bool isMTCEnabled() const { return m_mtcEnabled; }
    
    //==========================================================================
    // Host Sync (for plugin version)
    
    /** Enable/disable host sync */
    void setHostSyncEnabled(bool enabled) { m_hostSyncEnabled = enabled; }
    
    /** Check if host sync is enabled */
    bool isHostSyncEnabled() const { return m_hostSyncEnabled; }
    
    /** Process host playhead info (for plugin) */
    void processHostPlayhead(const juce::AudioPlayHead::PositionInfo& info);
    
    //==========================================================================
    // Sync Status
    
    /** Get current sync status */
    Status getStatus() const;
    
    /** Reset sync statistics */
    void resetStatistics();
    
    /** Get estimated external BPM from MIDI clock */
    float getExternalBPM() const { return m_estimatedExternalBPM; }
    
    /** Check if receiving valid external clock */
    bool hasValidExternalClock() const;
    
    //==========================================================================
    // Clock Drift Compensation
    
    /** Enable/disable drift compensation */
    void setDriftCompensationEnabled(bool enabled) { m_driftCompensationEnabled = enabled; }
    
    /** Get current clock drift in ms */
    double getClockDrift() const { return m_clockDrift; }
    
    /** Set drift compensation strength (0.0 - 1.0) */
    void setDriftCompensationStrength(float strength);
    
    //==========================================================================
    // MasterClock::Listener implementation
    
    void onClockPulse(int pulseNumber) override;
    void onClockStart() override;
    void onClockStop() override;
    void onClockReset() override;
    void onTempoChanged(float newBPM) override;
    
private:
    //==========================================================================
    // Internal State
    
    MasterClock& m_masterClock;
    Transport& m_transport;
    
    // MIDI devices
    std::unique_ptr<juce::MidiInput> m_midiInput;
    std::unique_ptr<juce::MidiOutput> m_midiOutput;
    juce::String m_midiInputDevice;
    juce::String m_midiOutputDevice;
    
    // MIDI clock state
    std::atomic<bool> m_receiveMidiClock{false};
    std::atomic<bool> m_sendMidiClock{false};
    std::atomic<int> m_midiClockCounter{0};
    std::atomic<float> m_estimatedExternalBPM{120.0f};
    
    // Timing measurement
    int64_t m_lastMidiClockTime{0};
    double m_midiClockInterval{0.0};
    double m_clockDrift{0.0};
    int m_droppedClocks{0};
    int m_totalClocksReceived{0};
    
    // Drift compensation
    bool m_driftCompensationEnabled{true};
    float m_driftCompensationStrength{0.5f};
    double m_driftAccumulator{0.0};
    
    // Ableton Link preparation
    std::atomic<bool> m_linkEnabled{false};
    // TODO: Add Link session when implementing
    
    // Other sync modes
    std::atomic<bool> m_mtcEnabled{false};
    std::atomic<bool> m_hostSyncEnabled{false};
    
    // Song position
    std::atomic<int> m_songPositionSixteenths{0};
    
    //==========================================================================
    // MidiInputCallback implementation
    
    void handleIncomingMidiMessage(juce::MidiInput* source,
                                   const juce::MidiMessage& message) override;
    
    //==========================================================================
    // Internal Methods
    
    /** Process incoming MIDI clock tick */
    void processMidiClockTick();
    
    /** Calculate BPM from MIDI clock interval */
    void calculateExternalBPM();
    
    /** Apply drift compensation */
    void applyDriftCompensation();
    
    /** Send MIDI clock tick */
    void sendMidiClockTick();
    
    /** Open MIDI devices */
    bool openMidiInput(const juce::String& deviceName);
    bool openMidiOutput(const juce::String& deviceName);
    
    /** Close MIDI devices */
    void closeMidiInput();
    void closeMidiOutput();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SyncManager)
};

} // namespace HAM