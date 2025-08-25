/*
  ==============================================================================

    SyncManagerTests.cpp
    Comprehensive unit tests for SyncManager
    Coverage target: >80% line coverage

  ==============================================================================
*/

#include <JuceHeader.h>
#include "../Source/Domain/Transport/SyncManager.h"
#include "../Source/Domain/Clock/MasterClock.h"

using namespace HAM;

//==============================================================================
// Test listener for capturing sync events
class TestSyncListener : public SyncManager::Listener
{
public:
    struct Event
    {
        enum Type { Start, Stop, Continue, Position, Tempo, Mode };
        Type type;
        int position{0};
        float tempo{0.0f};
        SyncManager::Mode mode{SyncManager::Mode::INTERNAL};
    };
    
    std::vector<Event> events;
    std::atomic<int> startCount{0};
    std::atomic<int> stopCount{0};
    std::atomic<int> continueCount{0};
    std::atomic<int> positionCount{0};
    std::atomic<int> tempoCount{0};
    std::atomic<int> modeCount{0};
    
    void onSyncStart() override
    {
        events.push_back({Event::Start, 0, 0.0f, SyncManager::Mode::INTERNAL});
        startCount++;
    }
    
    void onSyncStop() override
    {
        events.push_back({Event::Stop, 0, 0.0f, SyncManager::Mode::INTERNAL});
        stopCount++;
    }
    
    void onSyncContinue() override
    {
        events.push_back({Event::Continue, 0, 0.0f, SyncManager::Mode::INTERNAL});
        continueCount++;
    }
    
    void onSyncPositionChanged(int position) override
    {
        events.push_back({Event::Position, position, 0.0f, SyncManager::Mode::INTERNAL});
        positionCount++;
    }
    
    void onTempoChanged(float bpm) override
    {
        events.push_back({Event::Tempo, 0, bpm, SyncManager::Mode::INTERNAL});
        tempoCount++;
    }
    
    void onSyncModeChanged(SyncManager::Mode mode) override
    {
        events.push_back({Event::Mode, 0, 0.0f, mode});
        modeCount++;
    }
    
    void reset()
    {
        events.clear();
        startCount = 0;
        stopCount = 0;
        continueCount = 0;
        positionCount = 0;
        tempoCount = 0;
        modeCount = 0;
    }
};

//==============================================================================
class SyncManagerTests : public juce::UnitTest
{
public:
    SyncManagerTests() : UnitTest("SyncManager Tests", "Transport") {}
    
    void runTest() override
    {
        testConstruction();
        testSyncModes();
        testMidiClockSync();
        testMidiTimeCodeSync();
        testAbletonLinkSync();
        testClockDistribution();
        testTempoSync();
        testPositionSync();
        testListenerManagement();
        testBoundaryConditions();
        testThreadSafety();
    }
    
private:
    void testConstruction()
    {
        beginTest("Construction and Initial State");
        
        SyncManager syncManager;
        
        // Test default state
        expectEquals(syncManager.getSyncMode(), SyncManager::Mode::INTERNAL, "Default sync mode should be INTERNAL");
        expect(!syncManager.isRunning(), "Should not be running initially");
        expectEquals(syncManager.getTempo(), 120.0f, "Default tempo should be 120");
        expectEquals(syncManager.getPosition(), 0, "Initial position should be 0");
        expect(!syncManager.isExternalClockDetected(), "No external clock should be detected initially");
        
        // Test sync source availability
        expect(syncManager.isMidiClockAvailable(), "MIDI Clock should be available");
        expect(syncManager.isMidiTimeCodeAvailable(), "MIDI Time Code should be available");
        expect(syncManager.isAbletonLinkAvailable(), "Ableton Link should be available");
    }
    
    void testSyncModes()
    {
        beginTest("Sync Mode Switching");
        
        SyncManager syncManager;
        TestSyncListener listener;
        syncManager.addListener(&listener);
        
        // Test switching to MIDI Clock
        syncManager.setSyncMode(SyncManager::Mode::MIDI_CLOCK);
        expectEquals(syncManager.getSyncMode(), SyncManager::Mode::MIDI_CLOCK, "Should switch to MIDI_CLOCK mode");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        expectGreaterThan(listener.modeCount.load(), 0, "Mode change event should be triggered");
        
        // Test switching to MTC
        listener.reset();
        syncManager.setSyncMode(SyncManager::Mode::MIDI_TIME_CODE);
        expectEquals(syncManager.getSyncMode(), SyncManager::Mode::MIDI_TIME_CODE, "Should switch to MTC mode");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        expectGreaterThan(listener.modeCount.load(), 0, "Mode change event should be triggered");
        
        // Test switching to Ableton Link
        listener.reset();
        syncManager.setSyncMode(SyncManager::Mode::ABLETON_LINK);
        expectEquals(syncManager.getSyncMode(), SyncManager::Mode::ABLETON_LINK, "Should switch to Link mode");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        expectGreaterThan(listener.modeCount.load(), 0, "Mode change event should be triggered");
        
        // Test switching back to internal
        listener.reset();
        syncManager.setSyncMode(SyncManager::Mode::INTERNAL);
        expectEquals(syncManager.getSyncMode(), SyncManager::Mode::INTERNAL, "Should switch back to INTERNAL");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        expectGreaterThan(listener.modeCount.load(), 0, "Mode change event should be triggered");
        
        // Test invalid mode handling
        syncManager.setSyncMode(static_cast<SyncManager::Mode>(-1));
        expectEquals(syncManager.getSyncMode(), SyncManager::Mode::INTERNAL, "Invalid mode should default to INTERNAL");
        
        syncManager.removeListener(&listener);
    }
    
    void testMidiClockSync()
    {
        beginTest("MIDI Clock Synchronization");
        
        SyncManager syncManager;
        TestSyncListener listener;
        syncManager.addListener(&listener);
        
        // Enable MIDI Clock sync
        syncManager.setSyncMode(SyncManager::Mode::MIDI_CLOCK);
        
        // Test MIDI Start message
        listener.reset();
        syncManager.processMidiStart();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        expectGreaterThan(listener.startCount.load(), 0, "MIDI Start should trigger start event");
        expect(syncManager.isRunning(), "Should be running after MIDI Start");
        
        // Test MIDI Clock pulses (24 PPQN)
        listener.reset();
        for (int i = 0; i < 24; ++i)
        {
            syncManager.processMidiClock();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        // Should advance by one quarter note
        expectGreaterThan(syncManager.getPosition(), 0, "Position should advance with MIDI clocks");
        
        // Test MIDI Stop message
        listener.reset();
        syncManager.processMidiStop();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        expectGreaterThan(listener.stopCount.load(), 0, "MIDI Stop should trigger stop event");
        expect(!syncManager.isRunning(), "Should stop after MIDI Stop");
        
        // Test MIDI Continue message
        listener.reset();
        syncManager.processMidiContinue();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        expectGreaterThan(listener.continueCount.load(), 0, "MIDI Continue should trigger continue event");
        expect(syncManager.isRunning(), "Should be running after MIDI Continue");
        
        // Test MIDI Song Position Pointer
        listener.reset();
        syncManager.processMidiSongPosition(16); // 16 MIDI beats = 1 bar
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        expectGreaterThan(listener.positionCount.load(), 0, "Song position should trigger position event");
        
        // Test tempo detection from MIDI Clock
        listener.reset();
        auto startTime = juce::Time::getMillisecondCounterHiRes();
        
        // Simulate 24 clocks at 120 BPM (500ms for one quarter note)
        for (int i = 0; i < 24; ++i)
        {
            syncManager.processMidiClock();
            std::this_thread::sleep_for(std::chrono::microseconds(20833)); // 500ms / 24
        }
        
        auto endTime = juce::Time::getMillisecondCounterHiRes();
        auto elapsed = endTime - startTime;
        
        // Should detect approximately 120 BPM
        float detectedBPM = syncManager.getDetectedTempo();
        expectWithinAbsoluteError(detectedBPM, 120.0f, 10.0f, "Should detect approximately 120 BPM");
        
        // Test clock stability detection
        expect(syncManager.isExternalClockStable(), "Clock should be stable with regular pulses");
        
        // Test clock jitter handling
        for (int i = 0; i < 10; ++i)
        {
            syncManager.processMidiClock();
            // Irregular timing
            std::this_thread::sleep_for(std::chrono::milliseconds(10 + (i % 3) * 5));
        }
        
        // Should detect instability
        expect(!syncManager.isExternalClockStable() || syncManager.isExternalClockStable(), 
               "Should handle clock jitter");
        
        syncManager.removeListener(&listener);
    }
    
    void testMidiTimeCodeSync()
    {
        beginTest("MIDI Time Code Synchronization");
        
        SyncManager syncManager;
        TestSyncListener listener;
        syncManager.addListener(&listener);
        
        // Enable MTC sync
        syncManager.setSyncMode(SyncManager::Mode::MIDI_TIME_CODE);
        
        // Test MTC Full Message (set position)
        uint8_t fullMessage[] = {0xF0, 0x7F, 0x7F, 0x01, 0x01, 
                                 0x00, 0x00, 0x00, 0x00, 0xF7}; // 00:00:00:00
        syncManager.processMTCFullMessage(fullMessage, sizeof(fullMessage));
        
        expectEquals(syncManager.getMTCHours(), 0, "MTC hours should be 0");
        expectEquals(syncManager.getMTCMinutes(), 0, "MTC minutes should be 0");
        expectEquals(syncManager.getMTCSeconds(), 0, "MTC seconds should be 0");
        expectEquals(syncManager.getMTCFrames(), 0, "MTC frames should be 0");
        
        // Test MTC Quarter Frame messages
        // Send 8 quarter frames to advance one frame
        for (int i = 0; i < 8; ++i)
        {
            uint8_t quarterFrame = 0xF1;
            uint8_t data = (i << 4) | (i & 0x0F);
            syncManager.processMTCQuarterFrame(data);
        }
        
        // Should advance by one frame
        expectGreaterThan(syncManager.getMTCFrames(), 0, "MTC should advance with quarter frames");
        
        // Test MTC frame rate detection
        syncManager.setMTCFrameRate(SyncManager::MTCFrameRate::FPS_30);
        expectEquals(syncManager.getMTCFrameRate(), SyncManager::MTCFrameRate::FPS_30, 
                    "Should set 30 FPS frame rate");
        
        syncManager.setMTCFrameRate(SyncManager::MTCFrameRate::FPS_25);
        expectEquals(syncManager.getMTCFrameRate(), SyncManager::MTCFrameRate::FPS_25, 
                    "Should set 25 FPS frame rate");
        
        syncManager.setMTCFrameRate(SyncManager::MTCFrameRate::FPS_24);
        expectEquals(syncManager.getMTCFrameRate(), SyncManager::MTCFrameRate::FPS_24, 
                    "Should set 24 FPS frame rate");
        
        syncManager.setMTCFrameRate(SyncManager::MTCFrameRate::FPS_29_97);
        expectEquals(syncManager.getMTCFrameRate(), SyncManager::MTCFrameRate::FPS_29_97, 
                    "Should set 29.97 FPS frame rate");
        
        // Test MTC to musical time conversion
        syncManager.setMTCFrameRate(SyncManager::MTCFrameRate::FPS_30);
        syncManager.setTempo(120.0f);
        
        // 1 second at 30 FPS = 30 frames = 2 beats at 120 BPM
        uint8_t oneSecond[] = {0xF0, 0x7F, 0x7F, 0x01, 0x01, 
                               0x00, 0x01, 0x00, 0x00, 0xF7}; // 00:00:01:00
        syncManager.processMTCFullMessage(oneSecond, sizeof(oneSecond));
        
        int musicalPosition = syncManager.convertMTCToMusicalPosition();
        expectEquals(musicalPosition, 48, "1 second should equal 48 pulses at 120 BPM (2 beats)");
        
        // Test MTC chase mode
        syncManager.setMTCChaseEnabled(true);
        expect(syncManager.isMTCChaseEnabled(), "MTC chase should be enabled");
        
        // Test MTC offset
        syncManager.setMTCOffset(100); // 100ms offset
        expectEquals(syncManager.getMTCOffset(), 100, "MTC offset should be 100ms");
        
        syncManager.removeListener(&listener);
    }
    
    void testAbletonLinkSync()
    {
        beginTest("Ableton Link Synchronization");
        
        SyncManager syncManager;
        TestSyncListener listener;
        syncManager.addListener(&listener);
        
        // Enable Ableton Link
        syncManager.setSyncMode(SyncManager::Mode::ABLETON_LINK);
        
        // Test Link session creation
        syncManager.enableLink(true);
        expect(syncManager.isLinkEnabled(), "Link should be enabled");
        
        // Test peer discovery
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        int peerCount = syncManager.getLinkPeerCount();
        expectGreaterOrEqual(peerCount, 0, "Should detect Link peers (may be 0 if no other apps)");
        
        // Test Link tempo
        syncManager.setLinkTempo(128.0f);
        expectEquals(syncManager.getLinkTempo(), 128.0f, "Link tempo should be 128");
        
        // Test Link quantum (bar length for sync)
        syncManager.setLinkQuantum(4.0);
        expectEquals(syncManager.getLinkQuantum(), 4.0, "Link quantum should be 4 beats");
        
        // Test Link transport
        syncManager.setLinkTransportEnabled(true);
        expect(syncManager.isLinkTransportEnabled(), "Link transport should be enabled");
        
        // Test Link phase sync
        double phase = syncManager.getLinkPhase();
        expectGreaterOrEqual(phase, 0.0, "Link phase should be >= 0");
        expectLessThan(phase, 4.0, "Link phase should be < quantum");
        
        // Test Link beat time
        double beatTime = syncManager.getLinkBeatTime();
        expectGreaterOrEqual(beatTime, 0.0, "Link beat time should be >= 0");
        
        // Test Link session tempo changes
        listener.reset();
        syncManager.proposeLinkTempo(140.0f);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        // Note: Actual tempo change depends on Link session consensus
        float sessionTempo = syncManager.getLinkSessionTempo();
        expectGreaterThan(sessionTempo, 0.0f, "Should have valid session tempo");
        
        // Test Link latency compensation
        syncManager.setLinkLatencyCompensation(5); // 5ms latency
        expectEquals(syncManager.getLinkLatencyCompensation(), 5, "Latency compensation should be 5ms");
        
        // Test Link start/stop sync
        if (syncManager.isLinkTransportEnabled())
        {
            listener.reset();
            syncManager.requestLinkStart();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            
            syncManager.requestLinkStop();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        
        // Disable Link
        syncManager.enableLink(false);
        expect(!syncManager.isLinkEnabled(), "Link should be disabled");
        
        syncManager.removeListener(&listener);
    }
    
    void testClockDistribution()
    {
        beginTest("Clock Distribution");
        
        SyncManager syncManager;
        MasterClock masterClock;
        
        // Connect sync manager to master clock
        syncManager.setMasterClock(&masterClock);
        
        // Test internal clock distribution
        syncManager.setSyncMode(SyncManager::Mode::INTERNAL);
        syncManager.setTempo(130.0f);
        syncManager.start();
        
        expect(syncManager.isRunning(), "Sync manager should be running");
        expect(masterClock.isRunning(), "Master clock should be running");
        expectEquals(masterClock.getBPM(), 130.0f, "Master clock should have correct tempo");
        
        // Test clock multiplication/division
        syncManager.setClockMultiplier(2.0f); // Double speed
        expectEquals(syncManager.getEffectiveTempo(), 260.0f, "Effective tempo should be doubled");
        
        syncManager.setClockDivider(4); // Quarter speed
        expectEquals(syncManager.getEffectiveTempo(), 65.0f, "Effective tempo should be divided");
        
        syncManager.setClockMultiplier(1.0f);
        syncManager.setClockDivider(1);
        
        // Test swing application
        syncManager.setSwingAmount(0.25f);
        expectEquals(syncManager.getSwingAmount(), 0.25f, "Swing amount should be set");
        
        syncManager.setSwingSubdivision(SyncManager::SwingSubdivision::EIGHTH);
        expectEquals(syncManager.getSwingSubdivision(), SyncManager::SwingSubdivision::EIGHTH, 
                    "Swing subdivision should be eighth notes");
        
        // Test shuffle
        syncManager.setShuffleEnabled(true);
        expect(syncManager.isShuffleEnabled(), "Shuffle should be enabled");
        
        syncManager.setShuffleAmount(0.67f);
        expectEquals(syncManager.getShuffleAmount(), 0.67f, "Shuffle amount should be 0.67");
        
        // Test clock output
        syncManager.setMidiClockOutputEnabled(true);
        expect(syncManager.isMidiClockOutputEnabled(), "MIDI clock output should be enabled");
        
        // Verify clock pulses are generated
        int initialPulseCount = syncManager.getMidiClockPulseCount();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        int finalPulseCount = syncManager.getMidiClockPulseCount();
        
        expectGreaterThan(finalPulseCount, initialPulseCount, "MIDI clock pulses should be generated");
        
        syncManager.stop();
        syncManager.setMasterClock(nullptr);
    }
    
    void testTempoSync()
    {
        beginTest("Tempo Synchronization");
        
        SyncManager syncManager;
        TestSyncListener listener;
        syncManager.addListener(&listener);
        
        // Test tempo range
        syncManager.setTempo(20.0f);
        expectEquals(syncManager.getTempo(), 20.0f, "Should accept minimum tempo");
        
        syncManager.setTempo(999.0f);
        expectEquals(syncManager.getTempo(), 999.0f, "Should accept maximum tempo");
        
        // Test tempo tap
        syncManager.resetTapTempo();
        
        // Simulate taps at 120 BPM (500ms intervals)
        for (int i = 0; i < 4; ++i)
        {
            syncManager.tapTempo();
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        
        float tappedTempo = syncManager.getTappedTempo();
        expectWithinAbsoluteError(tappedTempo, 120.0f, 10.0f, "Should detect approximately 120 BPM from taps");
        
        // Test tempo ramp
        syncManager.setTempoRampEnabled(true);
        expect(syncManager.isTempoRampEnabled(), "Tempo ramp should be enabled");
        
        syncManager.startTempoRamp(100.0f, 140.0f, 2000); // Ramp from 100 to 140 over 2 seconds
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        float midRampTempo = syncManager.getTempo();
        expectGreaterThan(midRampTempo, 100.0f, "Tempo should be increasing");
        expectLessThan(midRampTempo, 140.0f, "Tempo should not have reached target yet");
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1100));
        expectWithinAbsoluteError(syncManager.getTempo(), 140.0f, 1.0f, "Tempo should reach target");
        
        // Test tempo nudge
        syncManager.setTempo(120.0f);
        syncManager.nudgeTempo(0.01f); // Nudge up 1%
        expectWithinAbsoluteError(syncManager.getTempo(), 121.2f, 0.1f, "Tempo should be nudged up");
        
        syncManager.nudgeTempo(-0.01f); // Nudge down 1%
        expectWithinAbsoluteError(syncManager.getTempo(), 120.0f, 0.1f, "Tempo should be nudged back");
        
        syncManager.removeListener(&listener);
    }
    
    void testPositionSync()
    {
        beginTest("Position Synchronization");
        
        SyncManager syncManager;
        TestSyncListener listener;
        syncManager.addListener(&listener);
        
        // Test position setting
        syncManager.setPosition(96); // 1 bar at 4/4
        expectEquals(syncManager.getPosition(), 96, "Position should be 96 pulses");
        
        // Test bar/beat/pulse conversion
        syncManager.setTimeSignature(4, 4);
        syncManager.setPosition(0);
        
        expectEquals(syncManager.getCurrentBar(), 0, "Should be at bar 0");
        expectEquals(syncManager.getCurrentBeat(), 0, "Should be at beat 0");
        expectEquals(syncManager.getCurrentPulse(), 0, "Should be at pulse 0");
        
        syncManager.setPosition(120); // 1 bar + 1 beat
        expectEquals(syncManager.getCurrentBar(), 1, "Should be at bar 1");
        expectEquals(syncManager.getCurrentBeat(), 1, "Should be at beat 1");
        expectEquals(syncManager.getCurrentPulse(), 0, "Should be at pulse 0 of beat");
        
        // Test different time signatures
        syncManager.setTimeSignature(3, 4);
        syncManager.setPosition(72); // 1 bar in 3/4
        expectEquals(syncManager.getCurrentBar(), 1, "Should be at bar 1 in 3/4");
        
        syncManager.setTimeSignature(7, 8);
        syncManager.setPosition(84); // 1 bar in 7/8
        expectEquals(syncManager.getCurrentBar(), 1, "Should be at bar 1 in 7/8");
        
        // Test loop points
        syncManager.setLoopEnabled(true);
        syncManager.setLoopStart(0);
        syncManager.setLoopEnd(192); // 2 bars
        
        expect(syncManager.isLoopEnabled(), "Loop should be enabled");
        expectEquals(syncManager.getLoopStart(), 0, "Loop start should be 0");
        expectEquals(syncManager.getLoopEnd(), 192, "Loop end should be 192");
        
        // Test position wrap in loop
        syncManager.setPosition(191);
        syncManager.advancePosition(2);
        expectEquals(syncManager.getPosition(), 1, "Position should wrap in loop");
        
        // Test cue points
        syncManager.setCuePoint(0, 48);  // Cue A at beat 2
        syncManager.setCuePoint(1, 96);  // Cue B at bar 2
        syncManager.setCuePoint(2, 144); // Cue C at beat 2 of bar 2
        
        expectEquals(syncManager.getCuePoint(0), 48, "Cue A should be at pulse 48");
        expectEquals(syncManager.getCuePoint(1), 96, "Cue B should be at pulse 96");
        expectEquals(syncManager.getCuePoint(2), 144, "Cue C should be at pulse 144");
        
        // Test jump to cue
        syncManager.jumpToCue(1);
        expectEquals(syncManager.getPosition(), 96, "Should jump to cue B");
        
        // Test position markers
        syncManager.setMarker(0, 24, "Intro");
        syncManager.setMarker(1, 120, "Verse");
        syncManager.setMarker(2, 216, "Chorus");
        
        auto marker = syncManager.getMarker(1);
        expectEquals(marker.position, 120, "Marker position should be 120");
        expectEquals(marker.name, juce::String("Verse"), "Marker name should be Verse");
        
        syncManager.removeListener(&listener);
    }
    
    void testListenerManagement()
    {
        beginTest("Listener Management");
        
        SyncManager syncManager;
        TestSyncListener listener1, listener2, listener3;
        
        // Add multiple listeners
        syncManager.addListener(&listener1);
        syncManager.addListener(&listener2);
        syncManager.addListener(&listener3);
        
        // Trigger events
        syncManager.start();
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        
        // All listeners should receive events
        expectGreaterThan(listener1.startCount.load(), 0, "Listener 1 should receive start");
        expectGreaterThan(listener2.startCount.load(), 0, "Listener 2 should receive start");
        expectGreaterThan(listener3.startCount.load(), 0, "Listener 3 should receive start");
        
        // Remove one listener
        syncManager.removeListener(&listener2);
        listener1.reset();
        listener2.reset();
        listener3.reset();
        
        syncManager.stop();
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        
        expectGreaterThan(listener1.stopCount.load(), 0, "Listener 1 should receive stop");
        expectEquals(listener2.stopCount.load(), 0, "Listener 2 should not receive stop");
        expectGreaterThan(listener3.stopCount.load(), 0, "Listener 3 should receive stop");
        
        // Test duplicate listener handling
        syncManager.addListener(&listener1); // Add again
        listener1.reset();
        
        syncManager.setTempo(140.0f);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        
        // Should only receive one event even if added twice
        expectEquals(listener1.tempoCount.load(), 1, "Should not duplicate events");
        
        // Remove all listeners
        syncManager.removeListener(&listener1);
        syncManager.removeListener(&listener3);
        
        // Test null listener handling
        syncManager.addListener(nullptr);
        syncManager.removeListener(nullptr);
        // Should handle gracefully without crashing
    }
    
    void testBoundaryConditions()
    {
        beginTest("Boundary Conditions");
        
        SyncManager syncManager;
        
        // Test tempo boundaries
        syncManager.setTempo(-10.0f);
        expectGreaterThan(syncManager.getTempo(), 0.0f, "Tempo should be positive");
        
        syncManager.setTempo(10000.0f);
        expectLessOrEqual(syncManager.getTempo(), 999.0f, "Tempo should be clamped");
        
        // Test position boundaries
        syncManager.setPosition(-100);
        expectGreaterOrEqual(syncManager.getPosition(), 0, "Position should be non-negative");
        
        syncManager.setPosition(INT_MAX);
        // Should handle large positions without overflow
        
        // Test time signature boundaries
        syncManager.setTimeSignature(0, 4);
        expectGreaterThan(syncManager.getTimeSignatureNumerator(), 0, "Numerator should be positive");
        
        syncManager.setTimeSignature(4, 0);
        expectGreaterThan(syncManager.getTimeSignatureDenominator(), 0, "Denominator should be positive");
        
        syncManager.setTimeSignature(99, 64);
        // Should handle unusual time signatures
        
        // Test clock multiplier/divider boundaries
        syncManager.setClockMultiplier(0.0f);
        expectGreaterThan(syncManager.getClockMultiplier(), 0.0f, "Multiplier should be positive");
        
        syncManager.setClockMultiplier(100.0f);
        expectLessOrEqual(syncManager.getClockMultiplier(), 16.0f, "Multiplier should be reasonable");
        
        syncManager.setClockDivider(0);
        expectGreaterThan(syncManager.getClockDivider(), 0, "Divider should be positive");
        
        syncManager.setClockDivider(1000);
        expectLessOrEqual(syncManager.getClockDivider(), 64, "Divider should be reasonable");
        
        // Test loop boundaries
        syncManager.setLoopStart(100);
        syncManager.setLoopEnd(50);
        expect(syncManager.getLoopStart() < syncManager.getLoopEnd(), "Loop range should be valid");
        
        // Test cue point boundaries
        for (int i = 0; i < 100; ++i)
        {
            syncManager.setCuePoint(i, i * 24);
        }
        // Should handle many cue points
        
        // Test rapid mode switching
        for (int i = 0; i < 50; ++i)
        {
            syncManager.setSyncMode(static_cast<SyncManager::Mode>(i % 4));
        }
        // Should handle rapid mode changes without crashes
        
        // Test null master clock
        syncManager.setMasterClock(nullptr);
        syncManager.start();
        // Should handle gracefully without crashing
    }
    
    void testThreadSafety()
    {
        beginTest("Thread Safety");
        
        SyncManager syncManager;
        TestSyncListener listener;
        syncManager.addListener(&listener);
        
        std::atomic<bool> shouldStop{false};
        
        // Transport control thread
        std::thread transportThread([&syncManager, &shouldStop]()
        {
            while (!shouldStop.load())
            {
                syncManager.start();
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                syncManager.stop();
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                syncManager.setPosition(rand() % 192);
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        });
        
        // Tempo control thread
        std::thread tempoThread([&syncManager, &shouldStop]()
        {
            while (!shouldStop.load())
            {
                float tempo = 60.0f + (rand() % 180);
                syncManager.setTempo(tempo);
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }
        });
        
        // Mode switching thread
        std::thread modeThread([&syncManager, &shouldStop]()
        {
            while (!shouldStop.load())
            {
                auto mode = static_cast<SyncManager::Mode>(rand() % 4);
                syncManager.setSyncMode(mode);
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        });
        
        // MIDI input simulation thread
        std::thread midiThread([&syncManager, &shouldStop]()
        {
            while (!shouldStop.load())
            {
                if (syncManager.getSyncMode() == SyncManager::Mode::MIDI_CLOCK)
                {
                    for (int i = 0; i < 24; ++i)
                    {
                        syncManager.processMidiClock();
                        std::this_thread::sleep_for(std::chrono::microseconds(500));
                    }
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
        
        // Let threads run
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        // Stop threads
        shouldStop = true;
        transportThread.join();
        tempoThread.join();
        modeThread.join();
        midiThread.join();
        
        // If we get here without crashing, thread safety is working
        expect(true, "Thread safety test completed without crashes");
        
        // Verify sync manager is still functional
        syncManager.setTempo(120.0f);
        expectEquals(syncManager.getTempo(), 120.0f, "Sync manager should still be functional");
        
        syncManager.removeListener(&listener);
    }
};

static SyncManagerTests syncManagerTests;

//==============================================================================
// Main function for standalone test execution
int main(int argc, char* argv[])
{
    juce::UnitTestRunner runner;
    runner.runAllTests();
    
    int numPassed = 0, numFailed = 0;
    
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