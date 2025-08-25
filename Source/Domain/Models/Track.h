/*
  ==============================================================================

    Track.h
    Represents a track containing 8 stages with MIDI routing

  ==============================================================================
*/

#pragma once

#include <array>
#include <memory>
#include <JuceHeader.h>
#include "Stage.h"
#include "Scale.h"

namespace HAM {

//==============================================================================
/**
 * Voice mode determines how overlapping notes are handled
 */
enum class VoiceMode
{
    MONO,       // New notes cut previous notes immediately
    POLY        // Notes can overlap (up to voice limit)
};

//==============================================================================
/**
 * Track direction for stage advancement
 */
enum class Direction
{
    FORWARD,
    BACKWARD,
    PENDULUM,
    RANDOM
};

//==============================================================================
/**
 * Accumulator mode determines what triggers accumulation
 */
enum class AccumulatorMode
{
    OFF,        // No accumulation
    STAGE,      // Accumulate on stage advance
    PULSE,      // Accumulate on pulse advance
    RATCHET,    // Accumulate on ratchet trigger
    PENDULUM    // Ping-pong accumulation
};

//==============================================================================
/**
 * Track contains 8 stages and manages sequencing behavior
 */
class Track
{
public:
    //==========================================================================
    // Construction
    Track();
    ~Track() = default;
    
    // Copy and move semantics
    Track(const Track&) = default;
    Track& operator=(const Track&) = default;
    Track(Track&&) = default;
    Track& operator=(Track&&) = default;
    
    //==========================================================================
    // Stage Management
    
    /** Get a stage by index (0-7) */
    Stage& getStage(int index);
    const Stage& getStage(int index) const;
    
    /** Get all stages */
    std::array<Stage, 8>& getStages() { return m_stages; }
    const std::array<Stage, 8>& getStages() const { return m_stages; }
    
    //==========================================================================
    // Track Parameters
    
    /** Set track name */
    void setName(const juce::String& name) { m_name = name; }
    const juce::String& getName() const { return m_name; }
    
    /** Set track color */
    void setColor(juce::Colour color) { m_color = color; }
    juce::Colour getColor() const { return m_color; }
    
    /** Enable/disable track */
    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }
    
    /** Mute track */
    void setMuted(bool muted) { m_muted = muted; }
    bool isMuted() const { return m_muted; }
    
    /** Solo track */
    void setSolo(bool solo) { m_solo = solo; }
    bool isSolo() const { return m_solo; }
    
    /** Set track volume (0.0 - 1.0) */
    void setVolume(float volume);
    float getVolume() const { return m_volume; }
    
    /** Set track pan (-1.0 to 1.0, center = 0.0) */
    void setPan(float pan);
    float getPan() const { return m_pan; }
    
    //==========================================================================
    // MIDI Configuration
    
    /** Set MIDI output channel (1-16) */
    void setMidiChannel(int channel);
    int getMidiChannel() const { return m_midiChannel; }
    
    /** Set voice mode */
    void setVoiceMode(VoiceMode mode) { m_voiceMode = mode; }
    VoiceMode getVoiceMode() const { return m_voiceMode; }
    
    /** Set max polyphony (1-16 voices) */
    void setMaxVoices(int voices);
    int getMaxVoices() const { return m_maxVoices; }
    
    //==========================================================================
    // Sequencing Parameters
    
    /** Set playback direction */
    void setDirection(Direction dir) { m_direction = dir; }
    Direction getDirection() const { return m_direction; }
    
    /** Set stage length (1-8 stages) */
    void setLength(int length);
    int getLength() const { return m_length; }
    
    /** Set clock division (1-64) */
    void setDivision(int division);
    int getDivision() const { return m_division; }
    
    /** Set swing amount (50-75%) */
    void setSwing(float swing);
    float getSwing() const { return m_swing; }
    
    /** Set octave offset (-4 to +4) */
    void setOctaveOffset(int octaves);
    int getOctaveOffset() const { return m_octaveOffset; }
    
    //==========================================================================
    // Accumulator Settings
    
    /** Set accumulator mode */
    void setAccumulatorMode(AccumulatorMode mode) { m_accumulatorMode = mode; }
    AccumulatorMode getAccumulatorMode() const { return m_accumulatorMode; }
    
    /** Set accumulator offset per trigger */
    void setAccumulatorOffset(int offset) { m_accumulatorOffset = offset; }
    int getAccumulatorOffset() const { return m_accumulatorOffset; }
    
    /** Set accumulator reset point */
    void setAccumulatorReset(int resetValue) { m_accumulatorReset = resetValue; }
    int getAccumulatorReset() const { return m_accumulatorReset; }
    
    /** Get current accumulator value */
    int getAccumulatorValue() const { return m_accumulatorValue; }
    void setAccumulatorValue(int value) { m_accumulatorValue = value; }
    
    //==========================================================================
    // State Management
    
    /** Get current stage index */
    int getCurrentStageIndex() const { return m_currentStageIndex; }
    void setCurrentStageIndex(int index);
    
    /** Reset track to initial state */
    void reset();
    
    /** Reset playback position only */
    void resetPosition();
    
    //==========================================================================
    // Serialization
    
    /** Convert to ValueTree for saving */
    juce::ValueTree toValueTree() const;
    
    /** Load from ValueTree */
    void fromValueTree(const juce::ValueTree& tree);
    
    //==========================================================================
    // Scale Assignment
    
    /** Set scale ID for quantization */
    void setScaleId(const juce::String& scaleId) { m_scaleId = scaleId; }
    const juce::String& getScaleId() const { return m_scaleId; }
    
    /** Set root note for scale (0-11, C=0) */
    void setRootNote(int root);
    int getRootNote() const { return m_rootNote; }
    
    /** Get scale object from ScaleManager (for compatibility with MidiEventGenerator) */
    Scale* getScale() const;
    
    //==========================================================================
    // Helper Methods for MidiEventGenerator
    
    /** Check if accumulator is enabled (not OFF) */
    bool hasAccumulator() const { return m_accumulatorMode != AccumulatorMode::OFF; }
    
    /** Get track index - note: this needs to be set externally by the container */
    int getIndex() const { return m_trackIndex; }
    void setIndex(int index) { m_trackIndex = index; }
    
private:
    //==========================================================================
    // Stages
    std::array<Stage, 8> m_stages;
    
    // Track info
    juce::String m_name = "Track";
    juce::Colour m_color{0xFF00FF88};  // Default mint green
    bool m_enabled = true;
    bool m_muted = false;
    bool m_solo = false;
    float m_volume = 0.8f;              // Default 80% volume
    float m_pan = 0.0f;                 // Center pan
    
    // MIDI settings
    int m_midiChannel = 1;              // MIDI channel 1-16
    VoiceMode m_voiceMode = VoiceMode::MONO;
    int m_maxVoices = 1;                // Max polyphony
    
    // Sequencing
    Direction m_direction = Direction::FORWARD;
    int m_length = 8;                   // Active stages (1-8)
    int m_division = 1;                 // Clock division
    float m_swing = 50.0f;              // Swing amount (50-75%)
    int m_octaveOffset = 0;             // Octave transpose
    
    // Accumulator
    AccumulatorMode m_accumulatorMode = AccumulatorMode::OFF;
    int m_accumulatorOffset = 0;        // Offset per trigger
    int m_accumulatorReset = 0;         // Reset threshold
    int m_accumulatorValue = 0;         // Current value
    
    // Scale
    juce::String m_scaleId = "chromatic";
    int m_rootNote = 0;                 // C = 0
    
    // Playback state
    int m_currentStageIndex = 0;
    int m_trackIndex = 0;               // Track index in sequence
    
    JUCE_LEAK_DETECTOR(Track)
};

} // namespace HAM