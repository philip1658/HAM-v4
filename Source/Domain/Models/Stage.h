/*
  ==============================================================================

    Stage.h
    Represents a single stage (step) in the sequencer

  ==============================================================================
*/

#pragma once

#include <array>
#include <vector>
#include <map>
#include <cmath>
#include <JuceHeader.h>

namespace HAM {

//==============================================================================
/**
 * Gate types determine how notes are triggered within a stage
 */
enum class GateType
{
    SUSTAINED,  // Single sustained note for entire stage duration (DEFAULT)
    MULTIPLE,   // Individual gate per ratchet
    HOLD,       // Single sustained gate across all ratchets  
    SINGLE,     // Gate on first ratchet only
    REST        // No gate output (silence)
};

//==============================================================================
/**
 * Modulation settings for HAM Editor features
 */
struct ModulationSettings
{
    float pitchBend = 0.0f;        // -1.0 to +1.0
    float modWheel = 0.0f;         // 0.0 to 1.0
    float aftertouch = 0.0f;       // 0.0 to 1.0
    bool enabled = false;
};

//==============================================================================
/**
 * CC (Control Change) mapping for hardware/plugin control
 */
struct CCMapping
{
    int ccNumber = 1;              // MIDI CC number (0-127)
    float minValue = 0.0f;         // Minimum mapped value
    float maxValue = 1.0f;         // Maximum mapped value
    int targetChannel = 1;         // Target MIDI channel (1-16)
    bool enabled = false;
};

//==============================================================================
/**
 * Skip conditions for stage playback
 */
enum class SkipCondition
{
    NEVER,          // Never skip
    EVERY_2,        // Skip every 2nd time
    EVERY_3,        // Skip every 3rd time
    EVERY_4,        // Skip every 4th time
    FILL,           // Only play during fills
    NO_FILL,        // Skip during fills
    RANDOM          // Random skip based on probability
};

//==============================================================================
/**
 * Velocity curve types for dynamic expression
 */
enum class VelocityCurveType
{
    LINEAR,         // Direct 1:1 mapping
    EXPONENTIAL,    // Exponential curve (softer at low values)
    LOGARITHMIC,    // Logarithmic curve (harder at low values)
    S_CURVE,        // S-shaped curve (compressed at extremes)
    INVERTED,       // Inverted linear (high becomes low)
    FIXED,          // Always use fixed velocity
    RANDOM,         // Random variation around base velocity
    CUSTOM          // User-defined curve points
};

//==============================================================================
/**
 * Velocity curve configuration for a stage
 */
struct VelocityCurve
{
    VelocityCurveType type = VelocityCurveType::LINEAR;
    float amount = 1.0f;           // Curve strength (0.0-1.0)
    float randomization = 0.0f;    // Random variation amount (0.0-1.0)
    int fixedVelocity = 100;       // For FIXED type
    
    // For CUSTOM type - up to 8 curve points
    std::array<float, 8> customPoints{0.0f, 0.143f, 0.286f, 0.429f, 
                                      0.571f, 0.714f, 0.857f, 1.0f};
    
    /** Apply curve to input velocity (0-127) */
    int applyToVelocity(int inputVelocity, float randomValue = 0.0f) const;
};

//==============================================================================
/**
 * Stage represents a single step in the sequencer pattern
 * Each stage contains pitch, velocity, gate, and timing information
 */
class Stage
{
public:
    //==========================================================================
    // Construction
    Stage();
    ~Stage() = default;
    
    // Copy and move semantics
    Stage(const Stage&) = default;
    Stage& operator=(const Stage&) = default;
    Stage(Stage&&) = default;
    Stage& operator=(Stage&&) = default;
    
    //==========================================================================
    // Core Parameters
    
    /** Sets the MIDI pitch (0-127) */
    void setPitch(int pitch);
    int getPitch() const { return m_pitch; }
    
    /** Sets the gate length (0.0-1.0, relative to pulse length) */
    void setGate(float gate);
    float getGate() const { return m_gate; }
    float getGateLength() const { return m_gate; }  // Alias for compatibility
    
    /** Sets the MIDI velocity (0-127) */
    void setVelocity(int velocity);
    int getVelocity() const { return m_velocity; }
    
    /** Set velocity curve configuration */
    void setVelocityCurve(const VelocityCurve& curve) { m_velocityCurve = curve; }
    const VelocityCurve& getVelocityCurve() const { return m_velocityCurve; }
    VelocityCurve& getVelocityCurve() { return m_velocityCurve; }
    
    /** Apply velocity curve to current velocity */
    int getProcessedVelocity(float randomValue = 0.0f) const 
    { 
        return m_velocityCurve.applyToVelocity(m_velocity, randomValue); 
    }
    
    /** Sets the number of pulses for this stage (1-8) */
    void setPulseCount(int count);
    int getPulseCount() const { return m_pulseCount; }
    
    //==========================================================================
    // Ratcheting
    
    /** Sets ratchet count for a specific pulse (1-8 ratchets) */
    void setRatchetCount(int pulseIndex, int ratchetCount);
    int getRatchetCount(int pulseIndex) const;
    const std::array<int, 8>& getRatchets() const { return m_ratchets; }
    
    /** Set ratchet probability (0-1) */
    void setRatchetProbability(float prob) { m_ratchetProbability = juce::jlimit(0.0f, 1.0f, prob); }
    float getRatchetProbability() const { return m_ratchetProbability; }
    
    //==========================================================================
    // Gate Control
    
    /** Sets the gate type for this stage */
    void setGateType(GateType type) { m_gateType = type; }
    GateType getGateType() const { return m_gateType; }
    
    // Overload for int (for GateEngine compatibility)
    void setGateType(int type) { 
        m_gateType = static_cast<GateType>(juce::jlimit(0, 4, type)); 
    }
    int getGateTypeAsInt() const { return static_cast<int>(m_gateType); }
    
    /** Enable/disable gate stretching (distributes ratchets evenly) */
    void setGateStretching(bool enabled) { m_gateStretching = enabled; }
    bool isGateStretching() const { return m_gateStretching; }
    
    //==========================================================================
    // Pitch Modulation
    
    /** Set octave offset (-4 to +4) */
    void setOctave(int octave) { m_octave = juce::jlimit(-4, 4, octave); }
    int getOctave() const { return m_octave; }
    
    /** Set pitch bend amount (-1.0 to 1.0) */
    void setPitchBend(float bend) { m_pitchBend = juce::jlimit(-1.0f, 1.0f, bend); }
    float getPitchBend() const { return m_pitchBend; }
    
    //==========================================================================
    // Probability & Conditions
    
    /** Sets probability for this stage to play (0-100%) */
    void setProbability(float probability);
    float getProbability() const { return m_probability; }
    
    /** Skip this stage on certain conditions */
    void setSkipOnFirstLoop(bool skip) { m_skipOnFirstLoop = skip; }
    bool shouldSkipOnFirstLoop() const { return m_skipOnFirstLoop; }
    
    /** Swing amount for this stage (-0.5 to +0.5) */
    void setSwing(float swing) { m_swing = juce::jlimit(-0.5f, 0.5f, swing); }
    float getSwing() const { return m_swing; }
    
    /** Set skip probability (0-1, 0=never skip, 1=always skip) */
    void setSkipProbability(float probability) { m_skipProbability = juce::jlimit(0.0f, 1.0f, probability); }
    float getSkipProbability() const { return m_skipProbability; }
    
    /** Set skip condition */
    void setSkipCondition(SkipCondition condition) { m_skipCondition = condition; }
    SkipCondition getSkipCondition() const { return m_skipCondition; }
    
    //==========================================================================
    // HAM Editor Features
    
    /** Get/set modulation settings */
    ModulationSettings& getModulation() { return m_modulation; }
    const ModulationSettings& getModulation() const { return m_modulation; }
    
    /** Add a CC mapping */
    void addCCMapping(const CCMapping& mapping);
    void removeCCMapping(int index);
    const std::vector<CCMapping>& getCCMappings() const { return m_ccMappings; }
    
    //==========================================================================
    // Helper Methods for MidiEventGenerator
    
    /** Check if modulation is enabled */
    bool hasModulation() const { return m_modulation.enabled; }
    
    /** Check if pitch bend is enabled (non-zero value) */
    bool hasPitchBend() const { return std::abs(m_pitchBend) > 0.001f; }
    
    /** Get CC mappings as a map (ccNumber -> value) for MIDI event generation */
    std::map<int, int> getCCMappingsAsMap() const;
    
    //==========================================================================
    // Slide & Glide
    
    /** Enable slide to next stage */
    void setSlide(bool enabled) { m_slide = enabled; }
    bool hasSlide() const { return m_slide; }
    
    /** Set slide time (0.0-1.0, relative to stage length) */
    void setSlideTime(float time);
    float getSlideTime() const { return m_slideTime; }
    
    //==========================================================================
    // Serialization
    
    /** Convert to ValueTree for saving */
    juce::ValueTree toValueTree() const;
    
    /** Load from ValueTree */
    void fromValueTree(const juce::ValueTree& tree);
    
    //==========================================================================
    // State Query
    
    /** Check if this stage is in default state */
    bool isDefault() const;
    
    /** Reset to default values */
    void reset();
    
private:
    //==========================================================================
    // Core parameters
    int m_pitch = 0;                       // Scale degree (0 = root note)
    float m_gate = 0.5f;                   // Gate length (50%)
    int m_velocity = 100;                  // MIDI velocity
    VelocityCurve m_velocityCurve;         // Velocity curve configuration
    int m_pulseCount = 1;                  // Number of pulses
    
    // Ratcheting (per pulse)
    std::array<int, 8> m_ratchets{1,1,1,1,1,1,1,1}; // Ratchets per pulse
    GateType m_gateType = GateType::SUSTAINED;
    bool m_gateStretching = false;
    float m_ratchetProbability = 1.0f;  // Always trigger ratchets by default
    
    // Probability and conditions
    float m_probability = 100.0f;          // Always play by default (percent 0-100)
    bool m_skipOnFirstLoop = false;
    float m_skipProbability = 0.0f;        // Never skip by default
    float m_swing = 0.0f;                  // No swing by default
    SkipCondition m_skipCondition = SkipCondition::NEVER;
    
    // Slide/glide
    bool m_slide = false;
    float m_slideTime = 0.1f;
    
    // Pitch modulation
    int m_octave = 0;           // Octave offset (-4 to +4)
    float m_pitchBend = 0.0f;   // Pitch bend amount (-1.0 to 1.0)
    
    // HAM Editor features
    ModulationSettings m_modulation;
    std::vector<CCMapping> m_ccMappings;
    
    JUCE_LEAK_DETECTOR(Stage)
};

} // namespace HAM