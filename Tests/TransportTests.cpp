/*
  ==============================================================================

    TransportTests.cpp
    Unit tests for Transport and SyncManager

  ==============================================================================
*/

#include <JuceHeader.h>
#include "../Source/Domain/Clock/MasterClock.h"
#include "../Source/Domain/Transport/Transport.h"
#include "../Source/Domain/Transport/SyncManager.h"

using namespace HAM;

//==============================================================================
class TestTransportListener : public Transport::Listener
{
public:
    bool started = false;
    bool stopped = false;
    bool paused = false;
    bool recordingStarted = false;
    bool recordingStopped = false;
    Transport::Position lastPosition;
    Transport::SyncMode lastSyncMode = Transport::SyncMode::INTERNAL;
    bool loopingChanged = false;
    bool lastLoopState = false;
    
    void onTransportStart() override { started = true; }
    void onTransportStop() override { stopped = true; }
    void onTransportPause() override { paused = true; }
    void onRecordingStart() override { recordingStarted = true; }
    void onRecordingStop() override { recordingStopped = true; }
    void onPositionChanged(const Transport::Position& position) override { lastPosition = position; }
    void onSyncModeChanged(Transport::SyncMode mode) override { lastSyncMode = mode; }
    void onLoopStateChanged(bool looping) override { loopingChanged = true; lastLoopState = looping; }
    
    void clear()
    {
        started = stopped = paused = false;
        recordingStarted = recordingStopped = false;
        loopingChanged = false;
    }
};

//==============================================================================
class TransportTests : public juce::UnitTest
{
public:
    TransportTests() : UnitTest("Transport Tests") {}
    
    void runTest() override
    {
        beginTest("Transport Default State");
        {
            MasterClock clock;
            Transport transport(clock);
            
            expect(transport.isStopped());
            expect(!transport.isPlaying());
            expect(!transport.isRecording());
            expect(!transport.isPaused());
            expect(transport.getSyncMode() == Transport::SyncMode::INTERNAL);
        }
        
        beginTest("Transport Play/Stop");
        {
            MasterClock clock;
            Transport transport(clock);
            TestTransportListener listener;
            transport.addListener(&listener);
            
            transport.play();
            expect(transport.isPlaying());
            expect(!transport.isStopped());
            
            transport.stop(false);
            expect(transport.isStopped());
            expect(!transport.isPlaying());
            
            transport.removeListener(&listener);
        }
        
        beginTest("Transport Position Control");
        {
            MasterClock clock;
            Transport transport(clock);
            
            transport.setPosition(4, 2, 12);
            auto pos = transport.getCurrentPosition();
            expect(pos.bar == 4);
            expect(pos.beat == 2);
            expect(pos.pulse == 12);
            
            transport.returnToZero();
            pos = transport.getCurrentPosition();
            expect(pos.bar == 0);
            expect(pos.beat == 0);
            expect(pos.pulse == 0);
            
            transport.moveByBars(3);
            pos = transport.getCurrentPosition();
            expect(pos.bar == 3);
        }
        
        beginTest("Transport Loop Control");
        {
            MasterClock clock;
            Transport transport(clock);
            
            expect(!transport.isLooping());
            
            transport.setLoopPoints(4, 8);
            expect(transport.getLoopStartBar() == 4);
            expect(transport.getLoopEndBar() == 8);
            
            transport.setLooping(true);
            expect(transport.isLooping());
        }
        
        beginTest("Transport Recording");
        {
            MasterClock clock;
            Transport transport(clock);
            TestTransportListener listener;
            transport.addListener(&listener);
            
            transport.record(false, 0);  // No count-in
            expect(transport.isRecording());
            expect(transport.isPlaying());  // Recording implies playing
            
            transport.stopRecording();
            expect(!transport.isRecording());
            expect(transport.isPlaying());  // Should continue playing
            
            transport.removeListener(&listener);
        }
        
        beginTest("Transport Sync Modes");
        {
            MasterClock clock;
            Transport transport(clock);
            
            expect(transport.getSyncMode() == Transport::SyncMode::INTERNAL);
            expect(!transport.isExternalSync());
            
            transport.setSyncMode(Transport::SyncMode::MIDI_CLOCK);
            expect(transport.getSyncMode() == Transport::SyncMode::MIDI_CLOCK);
            expect(transport.isExternalSync());
            
            transport.setSyncMode(Transport::SyncMode::INTERNAL);
            expect(!transport.isExternalSync());
        }
        
        beginTest("Transport Time Signature");
        {
            MasterClock clock;
            Transport transport(clock);
            
            expect(transport.getTimeSignatureNumerator() == 4);
            expect(transport.getTimeSignatureDenominator() == 4);
            
            transport.setTimeSignature(3, 4);
            expect(transport.getTimeSignatureNumerator() == 3);
            expect(transport.getTimeSignatureDenominator() == 4);
            
            transport.setTimeSignature(7, 8);
            expect(transport.getTimeSignatureNumerator() == 7);
            expect(transport.getTimeSignatureDenominator() == 8);
        }
        
        beginTest("Position String Formatting");
        {
            MasterClock clock;
            Transport transport(clock);
            
            transport.setPosition(0, 0, 0);
            expect(transport.getPositionString() == "001.1.00");
            
            transport.setPosition(11, 3, 23);
            expect(transport.getPositionString() == "012.4.23");
        }
    }
};

//==============================================================================
class SyncManagerTests : public juce::UnitTest
{
public:
    SyncManagerTests() : UnitTest("SyncManager Tests") {}
    
    void runTest() override
    {
        beginTest("SyncManager Default State");
        {
            MasterClock clock;
            Transport transport(clock);
            SyncManager sync(clock, transport);
            
            auto status = sync.getStatus();
            expect(!status.isReceivingMidiClock);
            expect(!status.isSendingMidiClock);
            expect(!status.isLinkEnabled);
            expect(!status.isMTCEnabled);
            expect(!status.isHostSyncEnabled);
        }
        
        beginTest("MIDI Clock Enable/Disable");
        {
            MasterClock clock;
            Transport transport(clock);
            SyncManager sync(clock, transport);
            
            sync.setMidiClockInputEnabled(true);
            expect(transport.getSyncMode() == Transport::SyncMode::MIDI_CLOCK);
            
            sync.setMidiClockInputEnabled(false);
            expect(transport.getSyncMode() == Transport::SyncMode::INTERNAL);
            
            sync.setMidiClockOutputEnabled(true);
            expect(sync.isSendingMidiClock());
            
            sync.setMidiClockOutputEnabled(false);
            expect(!sync.isSendingMidiClock());
        }
        
        beginTest("MIDI Clock Message Processing");
        {
            MasterClock clock;
            Transport transport(clock);
            SyncManager sync(clock, transport);
            
            sync.setMidiClockInputEnabled(true);
            
            // Test MIDI Start
            juce::MidiMessage startMsg(SyncManager::MIDI_CLOCK_START);
            sync.processMidiMessage(startMsg);
            expect(transport.isPlaying());
            
            // Test MIDI Stop
            juce::MidiMessage stopMsg(SyncManager::MIDI_CLOCK_STOP);
            sync.processMidiMessage(stopMsg);
            expect(transport.isStopped());
            
            // Test Song Position
            uint8_t songPosData[] = {SyncManager::MIDI_SONG_POSITION, 0x10, 0x00};  // 16 sixteenths = bar 1
            juce::MidiMessage songPosMsg(songPosData, 3);
            sync.processMidiMessage(songPosMsg);
            
            auto pos = transport.getCurrentPosition();
            expect(pos.bar == 1);
        }
        
        beginTest("Ableton Link Preparation");
        {
            MasterClock clock;
            Transport transport(clock);
            SyncManager sync(clock, transport);
            
            // Link is not implemented yet but infrastructure should work
            expect(!sync.isLinkAvailable());
            
            sync.setLinkEnabled(true);
            expect(sync.isLinkEnabled());
            expect(transport.getSyncMode() == Transport::SyncMode::ABLETON_LINK);
            
            sync.setLinkEnabled(false);
            expect(!sync.isLinkEnabled());
            expect(transport.getSyncMode() == Transport::SyncMode::INTERNAL);
        }
        
        beginTest("Host Sync Processing");
        {
            MasterClock clock;
            Transport transport(clock);
            SyncManager sync(clock, transport);
            
            sync.setHostSyncEnabled(true);
            
            // Create mock playhead info
            juce::AudioPlayHead::PositionInfo info;
            info.setIsPlaying(true);
            info.setBpm(140.0);
            info.setPpqPosition(8.0);  // 2 bars in 4/4
            
            sync.processHostPlayhead(info);
            
            expectWithinAbsoluteError(clock.getBPM(), 140.0f, 0.01f);
            
            auto pos = transport.getCurrentPosition();
            expect(pos.bar == 2);  // 8 quarters = 2 bars
        }
        
        beginTest("Drift Compensation");
        {
            MasterClock clock;
            Transport transport(clock);
            SyncManager sync(clock, transport);
            
            sync.setDriftCompensationEnabled(true);
            sync.setDriftCompensationStrength(0.5f);
            
            // Drift compensation should be active
            expectWithinAbsoluteError(sync.getClockDrift(), 0.0, 0.001);
        }
        
        beginTest("Statistics Reset");
        {
            MasterClock clock;
            Transport transport(clock);
            SyncManager sync(clock, transport);
            
            sync.resetStatistics();
            
            auto status = sync.getStatus();
            expect(status.droppedClocks == 0);
            expect(status.totalClocksReceived == 0);
            expectWithinAbsoluteError(status.clockDrift, 0.0, 0.001);
        }
    }
};

//==============================================================================
static TransportTests transportTests;
static SyncManagerTests syncManagerTests;

//==============================================================================
int main(int argc, char* argv[])
{
    juce::UnitTestRunner runner;
    runner.runAllTests();
    
    int numPassed = 0;
    int numFailed = 0;
    
    for (int i = 0; i < runner.getNumResults(); ++i)
    {
        auto* result = runner.getResult(i);
        if (result->failures > 0)
            numFailed++;
        else
            numPassed++;
            
        std::cout << result->unitTestName << ": "
                  << (result->failures > 0 ? "FAILED" : "PASSED")
                  << " (" << result->passes << " passes, "
                  << result->failures << " failures)\n";
    }
    
    std::cout << "\n========================================\n";
    std::cout << "Total: " << numPassed << " passed, " << numFailed << " failed\n";
    std::cout << "========================================\n";
    
    return numFailed > 0 ? 1 : 0;
}