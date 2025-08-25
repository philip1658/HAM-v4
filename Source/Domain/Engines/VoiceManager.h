/*
  ==============================================================================

    VoiceManager.h
    Voice allocation and management system for Mono/Poly modes
    Supports up to 64 simultaneous voices with intelligent voice stealing

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <memory>

namespace HAM {

//==============================================================================
/**
    Manages voice allocation for MIDI notes.
    Supports Mono and Poly modes with up to 64 voices.
    Implements intelligent voice stealing when voice limit is reached.
*/
class VoiceManager
{
public:
    //==========================================================================
    // Constants
    static constexpr int MAX_VOICES = 64;
    static constexpr int DEFAULT_POLY_VOICES = 16;
    
    //==========================================================================
    // Voice Modes
    enum class VoiceMode
    {
        MONO,           // One voice, new notes cut previous
        POLY,           // Multiple voices up to limit
        MONO_LEGATO,    // Mono with legato (no retriggering)
        MONO_RETRIG,    // Mono with retriggering on each note
        UNISON          // All voices play same note (future)
    };
    
    //==========================================================================
    // Voice Stealing Modes
    enum class StealingMode
    {
        OLDEST,         // Steal oldest playing note
        LOWEST,         // Steal lowest pitch
        HIGHEST,        // Steal highest pitch
        QUIETEST,       // Steal note with lowest velocity
        NONE            // Don't steal (ignore new notes)
    };
    
    //==========================================================================
    // Voice State
    struct Voice
    {
        std::atomic<bool> active{false};
        std::atomic<int> noteNumber{-1};
        std::atomic<int> velocity{0};
        std::atomic<int> channel{1};
        std::atomic<int64_t> startTime{0};  // For age tracking
        std::atomic<float> pitchBend{0.0f};
        std::atomic<float> pressure{0.0f};  // Aftertouch
        
        // MPE parameters
        std::atomic<float> slide{0.0f};     // MPE Y-axis
        std::atomic<float> glideBend{0.0f}; // MPE pitch glide
        
        // Voice identification
        int voiceId{0};  // Unique voice identifier
        
        void reset()
        {
            active.store(false);
            noteNumber.store(-1);
            velocity.store(0);
            startTime.store(0);
            pitchBend.store(0.0f);
            pressure.store(0.0f);
            slide.store(0.0f);
            glideBend.store(0.0f);
        }
        
        void startNote(int note, int vel, int ch)
        {
            noteNumber.store(note);
            velocity.store(vel);
            channel.store(ch);
            startTime.store(juce::Time::getHighResolutionTicks());
            active.store(true);
        }
        
        void stopNote()
        {
            active.store(false);
            // Keep other parameters for release phase
        }
    };
    
    //==========================================================================
    // Statistics
    struct Statistics
    {
        std::atomic<int> activeVoices{0};
        std::atomic<int> totalNotesPlayed{0};
        std::atomic<int> notesStolen{0};
        std::atomic<int> peakVoiceCount{0};
        std::atomic<float> cpuUsage{0.0f};
        
        // Additional stealing statistics
        std::atomic<int> oldestStolen{0};      // Notes stolen via oldest strategy
        std::atomic<int> lowestStolen{0};      // Notes stolen via lowest pitch strategy
        std::atomic<int> highestStolen{0};     // Notes stolen via highest pitch strategy
        std::atomic<int> quietestStolen{0};    // Notes stolen via quietest strategy
        std::atomic<int> lastStolenNote{-1};   // Last note that was stolen
        std::atomic<int64_t> lastStealTime{0}; // Timestamp of last steal
        
        void reset()
        {
            activeVoices.store(0);
            totalNotesPlayed.store(0);
            notesStolen.store(0);
            peakVoiceCount.store(0);
            cpuUsage.store(0.0f);
            oldestStolen.store(0);
            lowestStolen.store(0);
            highestStolen.store(0);
            quietestStolen.store(0);
            lastStolenNote.store(-1);
            lastStealTime.store(0);
        }
    };
    
    //==========================================================================
    // Construction
    VoiceManager();
    ~VoiceManager();
    
    //==========================================================================
    // Voice Mode Control
    
    /** Set voice mode (Mono/Poly) */
    void setVoiceMode(VoiceMode mode);
    
    /** Get current voice mode */
    VoiceMode getVoiceMode() const { return m_voiceMode; }
    
    /** Set maximum polyphony (1-64) */
    void setMaxVoices(int maxVoices);
    
    /** Get maximum voices */
    int getMaxVoices() const { return m_maxVoices; }
    
    /** Set voice stealing mode */
    void setStealingMode(StealingMode mode) { m_stealingMode = mode; }
    
    /** Get voice stealing mode */
    StealingMode getStealingMode() const { return m_stealingMode; }
    
    //==========================================================================
    // Note Management
    
    /** Start a note (returns voice index or -1 if failed) */
    int noteOn(int noteNumber, int velocity, int channel = 1);
    
    /** Stop a note */
    void noteOff(int noteNumber, int channel = 1);
    
    /** Stop all notes on a channel */
    void allNotesOff(int channel = 0);  // 0 = all channels
    
    /** Panic - immediately stop all voices */
    void panic();
    
    //==========================================================================
    // Voice Query
    
    /** Get voice by index */
    Voice* getVoice(int index);
    const Voice* getVoice(int index) const;
    
    /** Find voice playing a specific note */
    Voice* findVoiceForNote(int noteNumber, int channel = 0);
    
    /** Get number of active voices (cached for performance) */
    int getActiveVoiceCount() const { return m_activeVoiceCount.load(); }
    
    /** Get active voices into pre-allocated array */
    int getActiveVoices(Voice** outVoices, int maxVoices);
    
    /** Get active voices into pre-allocated array (const version) */
    int getActiveVoices(const Voice** outVoices, int maxVoices) const;
    
    /** Check if note is playing */
    bool isNotePlaying(int noteNumber, int channel = 0) const;
    
    //==========================================================================
    // MPE Support
    
    /** Set pitch bend for a voice */
    void setPitchBend(int voiceId, float bend) { setVoiceParameter(voiceId, bend, &Voice::pitchBend); }
    
    /** Set pressure (aftertouch) for a voice */
    void setPressure(int voiceId, float pressure) { setVoiceParameter(voiceId, pressure, &Voice::pressure); }
    
    /** Set slide (MPE Y-axis) for a voice */
    void setSlide(int voiceId, float slide) { setVoiceParameter(voiceId, slide, &Voice::slide); }
    
    /** Enable/disable MPE mode */
    void setMPEEnabled(bool enabled) { m_mpeEnabled = enabled; }
    
    /** Check if MPE is enabled */
    bool isMPEEnabled() const { return m_mpeEnabled; }
    
    //==========================================================================
    // Legato & Glide
    
    /** Enable/disable legato mode */
    void setLegatoEnabled(bool enabled) { m_legatoEnabled = enabled; }
    
    /** Set glide time in milliseconds */
    void setGlideTime(float ms) { m_glideTimeMs = ms; }
    
    /** Get glide time */
    float getGlideTime() const { return m_glideTimeMs; }
    
    //==========================================================================
    // Statistics
    
    /** Get voice statistics */
    const Statistics& getStatistics() const { return m_statistics; }
    
    /** Reset statistics */
    void resetStatistics() { m_statistics.reset(); }
    
    //==========================================================================
    // Real-time Safe Operations
    
    /** Process voices (called from audio thread) */
    void processVoices();
    
    /** Check if manager is real-time safe */
    bool isRealTimeSafe() const { return true; }  // All operations are lock-free
    
private:
    //==========================================================================
    // Internal State
    
    // Voice pool
    std::array<Voice, MAX_VOICES> m_voices;
    std::atomic<int> m_nextVoiceId{1};
    
    // Voice mode
    std::atomic<VoiceMode> m_voiceMode{VoiceMode::POLY};
    std::atomic<int> m_maxVoices{DEFAULT_POLY_VOICES};
    std::atomic<StealingMode> m_stealingMode{StealingMode::OLDEST};
    
    // MPE and legato
    std::atomic<bool> m_mpeEnabled{false};
    std::atomic<bool> m_legatoEnabled{false};
    std::atomic<float> m_glideTimeMs{0.0f};
    
    // Statistics
    mutable Statistics m_statistics;
    
    // Cached active voice count for performance
    std::atomic<int> m_activeVoiceCount{0};
    
    // Last played note (for mono modes)
    std::atomic<int> m_lastNoteNumber{-1};
    std::atomic<int> m_lastVoiceIndex{-1};
    
    // Pre-allocated array for voice queries (avoid allocations)
    // Currently unused but kept for future optimization
    // mutable std::array<Voice*, MAX_VOICES> m_activeVoiceCache;
    
    //==========================================================================
    // Internal Methods
    
    /** Find a free voice slot */
    int findFreeVoice();
    
    /** Steal a voice based on stealing mode */
    int stealVoice();
    
    /** Find oldest voice */
    int findOldestVoice();
    
    /** Find lowest pitch voice */
    int findLowestVoice();
    
    /** Find highest pitch voice */
    int findHighestVoice();
    
    /** Find quietest voice */
    int findQuietestVoice();
    
    /** Update statistics */
    void updateStatistics();
    
    /** Handle mono mode note on */
    int handleMonoNoteOn(int noteNumber, int velocity, int channel);
    
    /** Handle poly mode note on */
    int handlePolyNoteOn(int noteNumber, int velocity, int channel);
    
    /** Handle unison mode note on */
    int handleUnisonNoteOn(int noteNumber, int velocity, int channel);
    
    /** Safe voice access helper */
    Voice& voiceAt(int index) { return m_voices[static_cast<size_t>(index)]; }
    const Voice& voiceAt(int index) const { return m_voices[static_cast<size_t>(index)]; }
    
    /** Generic voice parameter setter (template for MPE params) */
    template<typename T>
    void setVoiceParameter(int voiceId, T value, std::atomic<T> Voice::* member)
    {
        if (voiceId >= 0 && voiceId < MAX_VOICES)
        {
            (voiceAt(voiceId).*member).store(value);
        }
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VoiceManager)
};

} // namespace HAM