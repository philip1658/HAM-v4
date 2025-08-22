/*
  ==============================================================================

    HAMAudioProcessor.h
    Main audio processor for HAM sequencer
    Implements JUCE AudioProcessor interface for real-time audio processing
    Coordinates all engines and handles lock-free communication with UI

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../../Domain/Engines/SequencerEngine.h"
#include "../../Domain/Engines/GateEngine.h"
#include "../../Domain/Engines/PitchEngine.h"
#include "../../Domain/Engines/AccumulatorEngine.h"
#include "../../Domain/Engines/VoiceManager.h"
#include "../../Domain/Clock/MasterClock.h"
#include "../../Domain/Services/MidiRouter.h"
#include "../../Domain/Services/ChannelManager.h"
#include "../../Domain/Transport/Transport.h"
#include "../Messaging/LockFreeMessageQueue.h"
#include "../Messaging/MessageDispatcher.h"
#include "../Messaging/MessageTypes.h"
#include <memory>
#include <atomic>

namespace HAM
{

/**
 * Main audio processor for HAM sequencer
 * 
 * Design principles:
 * - Real-time safe processBlock (no allocations, no locks)
 * - Lock-free message queue for UI communication
 * - All engines properly coordinated
 * - Sample-accurate MIDI timing
 */
class HAMAudioProcessor : public juce::AudioProcessor,
                          private MasterClock::Listener
{
public:
    //==============================================================================
    // Constructor/Destructor
    
    HAMAudioProcessor();
    ~HAMAudioProcessor() override;
    
    //==============================================================================
    // AudioProcessor overrides
    
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    
    //==============================================================================
    // Editor
    
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    
    //==============================================================================
    // Program/preset management
    
    const juce::String getName() const override { return "HAM Sequencer"; }
    
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}
    
    //==============================================================================
    // State management
    
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;
    
    //==============================================================================
    // Properties
    
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    
    //==============================================================================
    // UI Communication (lock-free)
    
    /** Get the message dispatcher for UI communication */
    MessageDispatcher& getMessageDispatcher() { return *m_messageDispatcher; }
    
    /** Process messages from UI (called from processBlock) */
    void processUIMessages();
    
    /** Expose MidiRouter for configuration (debug monitor, etc.) */
    MidiRouter* getMidiRouter() { return m_midiRouter.get(); }
    
    //==============================================================================
    // Transport control
    
    void play();
    void stop();
    void pause();
    bool isPlaying() const { return m_transport->isPlaying(); }
    
    void setBPM(float bpm);
    float getBPM() const { return m_masterClock->getBPM(); }
    
    //==============================================================================
    // Pattern management
    
    void loadPattern(int patternIndex);
    void savePattern(int patternIndex);
    Pattern* getCurrentPattern() { return m_currentPattern.get(); }
    
    //==============================================================================
    // Track management
    
    Track* getTrack(int index);
    int getNumTracks() const;
    void addTrack();
    void removeTrack(int index);
    
private:
    //==============================================================================
    // MasterClock::Listener overrides
    
    void onClockPulse(int pulseNumber) override;
    void onClockStart() override;
    void onClockStop() override;
    void onClockReset() override;
    void onTempoChanged(float newBPM) override;
    
    //==============================================================================
    // Internal processing
    
    /** Process one buffer of audio/MIDI */
    void processBuffer(juce::AudioBuffer<float>& buffer, 
                       juce::MidiBuffer& midiMessages,
                       int numSamples);
    
    /** Render events from engines to MIDI buffer */
    void renderMidiEvents(juce::MidiBuffer& midiMessages, int numSamples);
    
    // TODO: Implement when MessageTypes are defined
    // /** Handle parameter changes from UI */
    // void handleParameterChange(const ParameterChangeMessage& msg);
    // 
    // /** Handle transport commands from UI */
    // void handleTransportCommand(const TransportMessage& msg);
    
    //==============================================================================
    // Core components (Domain layer)
    
    std::unique_ptr<MasterClock> m_masterClock;
    std::unique_ptr<Transport> m_transport;
    std::unique_ptr<SequencerEngine> m_sequencerEngine;
    std::unique_ptr<VoiceManager> m_voiceManager;
    std::unique_ptr<MidiRouter> m_midiRouter;
    std::unique_ptr<ChannelManager> m_channelManager;
    
    // Per-track processors
    // TODO: Re-enable when TrackGateProcessor is implemented
    // std::vector<std::unique_ptr<TrackGateProcessor>> m_gateProcessors;
    std::vector<std::unique_ptr<PitchEngine>> m_pitchEngines;
    std::vector<std::unique_ptr<AccumulatorEngine>> m_accumulatorEngines;
    
    //==============================================================================
    // Infrastructure components
    
    std::unique_ptr<LockFreeMessageQueue<UIToEngineMessage, 2048>> m_messageQueue;
    std::unique_ptr<MessageDispatcher> m_messageDispatcher;
    
    //==============================================================================
    // State
    
    std::unique_ptr<Pattern> m_currentPattern;
    std::atomic<bool> m_isProcessing{false};
    
    // Audio parameters
    double m_currentSampleRate{48000.0};
    int m_currentBlockSize{512};
    
    // Performance monitoring
    std::atomic<float> m_cpuUsage{0.0f};
    std::atomic<int> m_droppedMessages{0};
    
    //==============================================================================
    // MIDI buffers for lock-free processing
    
    juce::MidiBuffer m_incomingMidi;
    juce::MidiBuffer m_outgoingMidi;
    
    // Pre-allocated MIDI event buffer (avoiding allocations in audio thread)
    std::vector<SequencerEngine::MidiEvent> m_midiEventBuffer;
    
    // Thread safety (using atomic flags instead of locks)
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HAMAudioProcessor)
};

} // namespace HAM