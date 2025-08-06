/*
  ==============================================================================

    Scale.h
    Musical scale definitions for pitch quantization

  ==============================================================================
*/

#pragma once

#include <vector>
#include <map>
#include <JuceHeader.h>

namespace HAM {

//==============================================================================
/**
 * Scale represents a musical scale for pitch quantization
 */
class Scale
{
public:
    //==========================================================================
    // Construction
    Scale();
    Scale(const juce::String& name, const std::vector<int>& intervals);
    ~Scale() = default;
    
    // Copy and move semantics
    Scale(const Scale&) = default;
    Scale& operator=(const Scale&) = default;
    Scale(Scale&&) = default;
    Scale& operator=(Scale&&) = default;
    
    //==========================================================================
    // Scale Properties
    
    /** Get scale name */
    const juce::String& getName() const { return m_name; }
    void setName(const juce::String& name) { m_name = name; }
    
    /** Get scale intervals (semitones from root) */
    const std::vector<int>& getIntervals() const { return m_intervals; }
    void setIntervals(const std::vector<int>& intervals);
    
    /** Get number of notes in scale */
    int getSize() const { return static_cast<int>(m_intervals.size()); }
    
    /** Check if scale is chromatic (all 12 notes) */
    bool isChromatic() const;
    
    //==========================================================================
    // Quantization
    
    /** Quantize a MIDI note to the nearest scale degree */
    int quantize(int midiNote, int rootNote = 0) const;
    
    /** Get scale degree for a MIDI note (0-based, -1 if not in scale) */
    int getDegree(int midiNote, int rootNote = 0) const;
    
    /** Get MIDI note for a scale degree */
    int getNoteForDegree(int degree, int rootNote = 0, int octave = 4) const;
    
    /** Check if a note is in the scale */
    bool contains(int midiNote, int rootNote = 0) const;
    
    //==========================================================================
    // Preset Scales
    
    /** Get a preset scale by ID */
    static Scale getPresetScale(const juce::String& scaleId);
    
    /** Get list of all preset scale IDs */
    static juce::StringArray getPresetScaleIds();
    
    /** Get display name for a preset scale ID */
    static juce::String getPresetScaleName(const juce::String& scaleId);
    
    //==========================================================================
    // Common Scales
    static const Scale Chromatic;
    static const Scale Major;
    static const Scale Minor;
    static const Scale HarmonicMinor;
    static const Scale MelodicMinor;
    static const Scale Dorian;
    static const Scale Phrygian;
    static const Scale Lydian;
    static const Scale Mixolydian;
    static const Scale Locrian;
    static const Scale PentatonicMajor;
    static const Scale PentatonicMinor;
    static const Scale Blues;
    static const Scale WholeTone;
    static const Scale Diminished;
    static const Scale Augmented;
    
    //==========================================================================
    // Serialization
    
    /** Convert to ValueTree for saving */
    juce::ValueTree toValueTree() const;
    
    /** Load from ValueTree */
    void fromValueTree(const juce::ValueTree& tree);
    
    /** Convert to string representation */
    juce::String toString() const;
    
    /** Parse from string representation */
    static Scale fromString(const juce::String& str);
    
private:
    //==========================================================================
    juce::String m_name;
    std::vector<int> m_intervals;  // Semitones from root
    
    // Helper for finding nearest note
    int findNearestScaleNote(int chromaticPitch) const;
    
    // Static preset scale definitions
    static std::map<juce::String, Scale> createPresetScales();
    static const std::map<juce::String, Scale> s_presetScales;
    
    // Allow ScaleManager to access private members
    friend class ScaleManager;
    
    JUCE_LEAK_DETECTOR(Scale)
};

//==============================================================================
/**
 * ScaleManager manages available scales and custom scale definitions
 */
class ScaleManager
{
public:
    //==========================================================================
    // Singleton access
    static ScaleManager& getInstance();
    
    //==========================================================================
    // Scale Management
    
    /** Add a custom scale */
    void addCustomScale(const juce::String& id, const Scale& scale);
    
    /** Remove a custom scale */
    void removeCustomScale(const juce::String& id);
    
    /** Get a scale by ID (preset or custom) */
    Scale getScale(const juce::String& id) const;
    
    /** Check if scale exists */
    bool hasScale(const juce::String& id) const;
    
    /** Get all available scale IDs */
    juce::StringArray getAllScaleIds() const;
    
    /** Get custom scale IDs only */
    juce::StringArray getCustomScaleIds() const;
    
    //==========================================================================
    // Persistence
    
    /** Save custom scales to file */
    void saveCustomScales(const juce::File& file);
    
    /** Load custom scales from file */
    void loadCustomScales(const juce::File& file);
    
    /** Get default custom scales file */
    juce::File getDefaultCustomScalesFile() const;
    
private:
    //==========================================================================
    ScaleManager() = default;
    ~ScaleManager() = default;
    
    // Prevent copying
    ScaleManager(const ScaleManager&) = delete;
    ScaleManager& operator=(const ScaleManager&) = delete;
    
    std::map<juce::String, Scale> m_customScales;
    
    JUCE_LEAK_DETECTOR(ScaleManager)
};

} // namespace HAM