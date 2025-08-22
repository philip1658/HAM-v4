/*
  ==============================================================================

    VoiceManagerTests.cpp
    Unit tests for VoiceManager

  ==============================================================================
*/

#include <JuceHeader.h>
#include "../Source/Domain/Engines/VoiceManager.h"

using namespace HAM;

//==============================================================================
class VoiceManagerTests : public juce::UnitTest
{
public:
    VoiceManagerTests() : UnitTest("VoiceManager Tests") {}
    
    void runTest() override
    {
        beginTest("Voice Manager Default State");
        {
            VoiceManager vm;
            expect(vm.getVoiceMode() == VoiceManager::VoiceMode::POLY);
            expect(vm.getMaxVoices() == VoiceManager::DEFAULT_POLY_VOICES);
            expect(vm.getActiveVoiceCount() == 0);
            expect(vm.getStealingMode() == VoiceManager::StealingMode::OLDEST);
        }
        
        beginTest("Poly Mode Note Allocation");
        {
            VoiceManager vm;
            vm.setVoiceMode(VoiceManager::VoiceMode::POLY);
            vm.setMaxVoices(4);  // Limit to 4 for testing
            
            // Play 4 notes
            int v1 = vm.noteOn(60, 100, 1);  // C4
            int v2 = vm.noteOn(64, 100, 1);  // E4
            int v3 = vm.noteOn(67, 100, 1);  // G4
            int v4 = vm.noteOn(72, 100, 1);  // C5
            
            expect(v1 >= 0 && v1 < 4);
            expect(v2 >= 0 && v2 < 4);
            expect(v3 >= 0 && v3 < 4);
            expect(v4 >= 0 && v4 < 4);
            expect(vm.getActiveVoiceCount() == 4);
            
            // Release one note
            vm.noteOff(64, 1);
            expect(vm.getActiveVoiceCount() == 3);
            
            // Play another note - should reuse freed voice
            int v5 = vm.noteOn(69, 100, 1);  // A4
            expect(v5 >= 0 && v5 < 4);
            expect(vm.getActiveVoiceCount() == 4);
        }
        
        beginTest("Mono Mode Behavior");
        {
            VoiceManager vm;
            vm.setVoiceMode(VoiceManager::VoiceMode::MONO);
            
            // Play first note
            int v1 = vm.noteOn(60, 100, 1);
            std::cout << "[DBG] Mono: v1=" << v1 << ", active=" << vm.getActiveVoiceCount() << "\n";
            expect(v1 == 0);  // Mono always uses voice 0
            expect(vm.getActiveVoiceCount() == 1);
            
            // Play second note - should cut first
            int v2 = vm.noteOn(64, 100, 1);
            std::cout << "[DBG] Mono: v2=" << v2 << ", active=" << vm.getActiveVoiceCount() << "\n";
            expect(v2 == 0);  // Still voice 0
            expect(vm.getActiveVoiceCount() == 1);
            
            auto* voice = vm.getVoice(0);
            expect(voice != nullptr);
            expect(voice->noteNumber.load() == 64);
            
            // Release current note
            vm.noteOff(64, 1);
            std::cout << "[DBG] Mono: after noteOff active=" << vm.getActiveVoiceCount() << "\n";
            expect(vm.getActiveVoiceCount() == 0);
        }
        
        beginTest("Voice Stealing - Oldest Mode");
        {
            VoiceManager vm;
            vm.setVoiceMode(VoiceManager::VoiceMode::POLY);
            vm.setMaxVoices(3);
            vm.setStealingMode(VoiceManager::StealingMode::OLDEST);
            
            // Fill all voices
            vm.noteOn(60, 100, 1);
            juce::Thread::sleep(10);  // Small delay to ensure different timestamps
            vm.noteOn(64, 100, 1);
            juce::Thread::sleep(10);
            vm.noteOn(67, 100, 1);
            
            expect(vm.getActiveVoiceCount() == 3);
            
            // Play another note - should steal oldest (60)
            vm.noteOn(72, 100, 1);
            
            expect(vm.getActiveVoiceCount() == 3);
            expect(!vm.isNotePlaying(60, 1));  // First note should be gone
            expect(vm.isNotePlaying(64, 1));
            expect(vm.isNotePlaying(67, 1));
            expect(vm.isNotePlaying(72, 1));
            
            const auto& stats = vm.getStatistics();
            expect(stats.notesStolen.load() == 1);
        }
        
        beginTest("Voice Stealing - Lowest Mode");
        {
            VoiceManager vm;
            vm.setVoiceMode(VoiceManager::VoiceMode::POLY);
            vm.setMaxVoices(3);
            vm.setStealingMode(VoiceManager::StealingMode::LOWEST);
            
            // Fill all voices
            vm.noteOn(60, 100, 1);  // Lowest
            vm.noteOn(64, 100, 1);
            vm.noteOn(67, 100, 1);
            
            // Play another note - should steal lowest (60)
            vm.noteOn(72, 100, 1);
            
            expect(!vm.isNotePlaying(60, 1));  // Lowest should be gone
            expect(vm.isNotePlaying(64, 1));
            expect(vm.isNotePlaying(67, 1));
            expect(vm.isNotePlaying(72, 1));
        }
        
        beginTest("Voice Stealing - Highest Mode");
        {
            VoiceManager vm;
            vm.setVoiceMode(VoiceManager::VoiceMode::POLY);
            vm.setMaxVoices(3);
            vm.setStealingMode(VoiceManager::StealingMode::HIGHEST);
            
            // Fill all voices
            vm.noteOn(60, 100, 1);
            vm.noteOn(64, 100, 1);
            vm.noteOn(67, 100, 1);  // Highest
            
            // Play another note - should steal highest (67)
            vm.noteOn(72, 100, 1);
            
            expect(vm.isNotePlaying(60, 1));
            expect(vm.isNotePlaying(64, 1));
            expect(!vm.isNotePlaying(67, 1));  // Highest should be gone
            expect(vm.isNotePlaying(72, 1));
        }
        
        beginTest("Voice Stealing - Quietest Mode");
        {
            VoiceManager vm;
            vm.setVoiceMode(VoiceManager::VoiceMode::POLY);
            vm.setMaxVoices(3);
            vm.setStealingMode(VoiceManager::StealingMode::QUIETEST);
            
            // Fill all voices with different velocities
            vm.noteOn(60, 100, 1);
            vm.noteOn(64, 50, 1);   // Quietest
            vm.noteOn(67, 80, 1);
            
            // Play another note - should steal quietest (64)
            vm.noteOn(72, 100, 1);
            
            expect(vm.isNotePlaying(60, 1));
            expect(!vm.isNotePlaying(64, 1));  // Quietest should be gone
            expect(vm.isNotePlaying(67, 1));
            expect(vm.isNotePlaying(72, 1));
        }
        
        beginTest("All Notes Off");
        {
            VoiceManager vm;
            vm.setMaxVoices(8);
            
            // Play notes on different channels
            vm.noteOn(60, 100, 1);
            vm.noteOn(64, 100, 2);
            vm.noteOn(67, 100, 1);
            vm.noteOn(72, 100, 3);
            
            expect(vm.getActiveVoiceCount() == 4);
            
            // All notes off on channel 1
            vm.allNotesOff(1);
            expect(vm.getActiveVoiceCount() == 2);  // Only ch2 and ch3 remain
            
            // All notes off on all channels
            vm.allNotesOff(0);
            expect(vm.getActiveVoiceCount() == 0);
        }
        
        beginTest("Panic Function");
        {
            VoiceManager vm;
            
            // Play several notes
            vm.noteOn(60, 100, 1);
            vm.noteOn(64, 100, 1);
            vm.noteOn(67, 100, 1);
            
            expect(vm.getActiveVoiceCount() == 3);
            
            // Panic should immediately stop all
            vm.panic();
            expect(vm.getActiveVoiceCount() == 0);
            
            // All voices should be reset
            for (int i = 0; i < 3; ++i)
            {
                auto* voice = vm.getVoice(i);
                expect(!voice->active.load());
                expect(voice->noteNumber.load() == -1);
            }
        }
        
        beginTest("MPE Parameters");
        {
            VoiceManager vm;
            vm.setMPEEnabled(true);
            
            int voiceId = vm.noteOn(60, 100, 1);
            expect(voiceId >= 0);
            
            // Set MPE parameters
            vm.setPitchBend(voiceId, 0.5f);
            vm.setPressure(voiceId, 0.7f);
            vm.setSlide(voiceId, 0.3f);
            
            auto* voice = vm.getVoice(voiceId);
            expect(voice != nullptr);
            expectWithinAbsoluteError(voice->pitchBend.load(), 0.5f, 0.001f);
            expectWithinAbsoluteError(voice->pressure.load(), 0.7f, 0.001f);
            expectWithinAbsoluteError(voice->slide.load(), 0.3f, 0.001f);
        }
        
        beginTest("64 Voice Polyphony");
        {
            VoiceManager vm;
            vm.setVoiceMode(VoiceManager::VoiceMode::POLY);
            vm.setMaxVoices(64);  // Maximum voices
            
            // Play 64 notes
            for (int i = 0; i < 64; ++i)
            {
                int voiceId = vm.noteOn(36 + i, 100, 1);  // C2 to D#7
                expect(voiceId >= 0 && voiceId < 64);
            }
            
            expect(vm.getActiveVoiceCount() == 64);
            
            // Try to play one more - should steal
            vm.setStealingMode(VoiceManager::StealingMode::OLDEST);
            int extraVoice = vm.noteOn(100, 100, 1);
            expect(extraVoice >= 0);  // Should have stolen a voice
            expect(vm.getActiveVoiceCount() == 64);  // Still 64 voices
            
            const auto& stats = vm.getStatistics();
            expect(stats.notesStolen.load() == 1);
            expect(stats.peakVoiceCount.load() == 64);
        }
        
        beginTest("Statistics Tracking");
        {
            VoiceManager vm;
            vm.resetStatistics();
            
            // Play some notes
            vm.noteOn(60, 100, 1);
            vm.noteOn(64, 100, 1);
            vm.noteOn(67, 100, 1);
            
            const auto& stats = vm.getStatistics();
            expect(stats.totalNotesPlayed.load() == 3);
            expect(stats.activeVoices.load() == 3);
            expect(stats.peakVoiceCount.load() == 3);
            
            // Release one
            vm.noteOff(64, 1);
            const auto& stats2 = vm.getStatistics();
            expect(stats2.activeVoices.load() == 2);
            expect(stats2.peakVoiceCount.load() == 3);  // Peak remains
        }
        
        beginTest("Real-Time Safety");
        {
            VoiceManager vm;
            expect(vm.isRealTimeSafe());
            
            // All operations should be lock-free
            // This is verified by the atomic operations used throughout
        }
    }
};

//==============================================================================
static VoiceManagerTests voiceManagerTests;

//==============================================================================
int main(int /*argc*/, char* /*argv*/[])
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