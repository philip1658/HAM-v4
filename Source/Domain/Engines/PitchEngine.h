/*
  ==============================================================================

    PitchEngine.h
    Pitch processing and quantization engine for HAM sequencer
    Handles scale quantization, octave offsets, and note range limiting

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../Models/Scale.h"
#include "../Models/Stage.h"
#include <atomic>
#include <array>
#include <memory>

namespace HAM
{

/**
 * Pitch engine for processing and quantizing MIDI notes
 * Thread-safe for real-time audio processing
 */
class PitchEngine
{
public:
    //==============================================================================
    // Configuration
    
    enum class QuantizationMode
    {
        SCALE,      // Quantize to current scale
        CHROMATIC,  // No quantization, all notes pass
        CHORD,      // Quantize to chord tones
        CUSTOM      // User-defined scale
    };
    
    struct PitchResult
    {
        int midiNote;           // Final MIDI note (0-127)
        int octave;            // Octave offset applied
        int scaleDegree;       // Degree in current scale
        bool wasQuantized;     // Whether quantization was applied
        float pitchBend;       // Additional pitch bend (-1.0 to 1.0)
    };
    
    //==============================================================================
    // Constructor/Destructor
    
    PitchEngine();
    ~PitchEngine() = default;
    
    //==============================================================================
    // Pitch Processing
    
    /**
     * Process pitch for a stage
     * @param stage The stage to process
     * @param baseNote Root note for the pattern
     * @param accumulatorOffset Pitch offset from accumulator
     * @return Processed pitch result
     */
    PitchResult processPitch(const Stage& stage, 
                           int baseNote = 60,
                           int accumulatorOffset = 0);
    
    /**
     * Quantize a MIDI note to the current scale
     * @param midiNote The note to quantize (0-127)
     * @param snapUp Whether to snap up or down to nearest scale tone
     * @return Quantized MIDI note
     */
    int quantizeToScale(int midiNote, bool snapUp = true);
    
    /**
     * Apply octave offset to a note
     * @param midiNote The base note
     * @param octaveOffset Octave offset (-4 to +4)
     * @return Note with octave offset applied
     */
    int applyOctaveOffset(int midiNote, int octaveOffset);
    
    /**
     * Limit note to MIDI range
     * @param midiNote The note to limit
     * @return Note clamped to 0-127
     */
    int limitToMidiRange(int midiNote);
    
    //==============================================================================
    // Scale Management
    
    /**
     * Set the current scale
     * @param scale The scale to use for quantization
     */
    void setScale(const Scale& scale);
    
    /**
     * Get the current scale
     * @return Current scale
     */
    const Scale& getScale() const { return *m_currentScale; }
    
    /**
     * Set quantization mode
     * @param mode The quantization mode
     */
    void setQuantizationMode(QuantizationMode mode) { m_quantizationMode.store(static_cast<int>(mode)); }
    
    /**
     * Get quantization mode
     * @return Current quantization mode
     */
    QuantizationMode getQuantizationMode() const { return static_cast<QuantizationMode>(m_quantizationMode.load()); }
    
    //==============================================================================
    // Chord Quantization
    
    /**
     * Set chord tones for chord quantization mode
     * @param chordTones Array of MIDI notes in the chord
     * @param numTones Number of chord tones (max 7)
     */
    void setChordTones(const int* chordTones, int numTones);
    
    /**
     * Clear chord tones
     */
    void clearChordTones();
    
    //==============================================================================
    // Custom Scale
    
    /**
     * Set custom scale intervals
     * @param intervals Array of semitone intervals from root
     * @param numIntervals Number of intervals (max 12)
     */
    void setCustomScale(const int* intervals, int numIntervals);
    
    //==============================================================================
    // Range Limiting
    
    /**
     * Set note range limits
     * @param minNote Minimum MIDI note (0-127)
     * @param maxNote Maximum MIDI note (0-127)
     */
    void setNoteRange(int minNote, int maxNote);
    
    /**
     * Get minimum note
     * @return Minimum MIDI note
     */
    int getMinNote() const { return m_minNote.load(); }
    
    /**
     * Get maximum note
     * @return Maximum MIDI note
     */
    int getMaxNote() const { return m_maxNote.load(); }
    
    //==============================================================================
    // Real-time safe parameter updates
    
    void setRootNote(int root) { m_rootNote.store(std::clamp(root, 0, 127)); }
    int getRootNote() const { return m_rootNote.load(); }
    
    void setOctaveRange(int range) { m_octaveRange.store(std::clamp(range, 1, 8)); }
    int getOctaveRange() const { return m_octaveRange.load(); }
    
    void setTransposition(int semitones) { m_transposition.store(std::clamp(semitones, -24, 24)); }
    int getTransposition() const { return m_transposition.load(); }
    
private:
    //==============================================================================
    // Internal state (all atomic for thread safety)
    
    std::unique_ptr<Scale> m_currentScale;
    std::atomic<int> m_quantizationMode{0};
    std::atomic<int> m_rootNote{60};
    std::atomic<int> m_octaveRange{2};
    std::atomic<int> m_transposition{0};
    std::atomic<int> m_minNote{0};
    std::atomic<int> m_maxNote{127};
    
    // Chord tones for chord quantization
    std::array<std::atomic<int>, 7> m_chordTones;
    std::atomic<int> m_numChordTones{0};
    
    // Custom scale intervals
    std::array<std::atomic<int>, 12> m_customIntervals;
    std::atomic<int> m_numCustomIntervals{0};
    
    //==============================================================================
    // Internal helpers
    
    int quantizeToChord(int midiNote);
    int quantizeToCustomScale(int midiNote);
    int findNearestScaleTone(int midiNote, const std::vector<int>& scaleTones, bool snapUp);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PitchEngine)
};

//==============================================================================
/**
 * Track pitch processor - manages pitch for all 8 stages
 */
class TrackPitchProcessor
{
public:
    TrackPitchProcessor();
    ~TrackPitchProcessor() = default;
    
    /**
     * Process pitch for current stage
     * @param track The track to process
     * @param currentStage Current stage index (0-7)
     * @param accumulatorValue Current accumulator value
     * @return Processed pitch result
     */
    PitchEngine::PitchResult processTrackPitch(
        const class Track* track,
        int currentStage,
        int accumulatorValue);
    
    /**
     * Update scale for the track
     * @param scale New scale to use
     */
    void updateScale(const Scale& scale);
    
    /**
     * Reset pitch processor state
     */
    void reset();
    
    /** Get the pitch engine for configuration */
    PitchEngine& getPitchEngine() { return m_pitchEngine; }
    
private:
    PitchEngine m_pitchEngine;
    int m_lastProcessedStage{-1};
    int m_baseNote{60};
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackPitchProcessor)
};

} // namespace HAM