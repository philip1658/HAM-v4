/*
  ==============================================================================

    Pattern.h
    Represents a pattern containing multiple tracks with morphing capability

  ==============================================================================
*/

#pragma once

#include <vector>
#include <memory>
#include <array>
#include <JuceHeader.h>
#include "Track.h"

namespace HAM {

//==============================================================================
/**
 * Snapshot stores pattern state for morphing
 */
struct Snapshot
{
    juce::String name = "Snapshot";
    juce::ValueTree data;
    int64 timestamp = 0;
    
    // Morphing metadata
    bool canMorph = true;
    float morphWeight = 1.0f;
};

//==============================================================================
/**
 * Pattern contains tracks and manages pattern-level operations
 */
class Pattern
{
public:
    //==========================================================================
    // Constants
    static constexpr int MAX_TRACKS = 128;
    static constexpr int MAX_SNAPSHOTS = 64;
    static constexpr int DEFAULT_TRACK_COUNT = 1;
    
    //==========================================================================
    // Construction
    Pattern();
    ~Pattern() = default;
    
    // Copy and move semantics - no copy due to unique_ptr members
    Pattern(const Pattern&) = delete;
    Pattern& operator=(const Pattern&) = delete;
    Pattern(Pattern&&) = default;
    Pattern& operator=(Pattern&&) = default;
    
    //==========================================================================
    // Pattern Info
    
    /** Set pattern name */
    void setName(const juce::String& name) { m_name = name; }
    const juce::String& getName() const { return m_name; }
    
    /** Set pattern author */
    void setAuthor(const juce::String& author) { m_author = author; }
    const juce::String& getAuthor() const { return m_author; }
    
    /** Set pattern description */
    void setDescription(const juce::String& desc) { m_description = desc; }
    const juce::String& getDescription() const { return m_description; }
    
    /** Set BPM */
    void setBPM(float bpm);
    float getBPM() const { return m_bpm; }
    
    /** Set time signature */
    void setTimeSignature(int numerator, int denominator);
    int getTimeSignatureNumerator() const { return m_timeSignatureNum; }
    int getTimeSignatureDenominator() const { return m_timeSignatureDenom; }
    
    //==========================================================================
    // Track Management
    
    /** Add a new track */
    int addTrack();
    
    /** Remove a track by index */
    void removeTrack(int index);
    
    /** Get track by index */
    Track* getTrack(int index);
    const Track* getTrack(int index) const;
    
    /** Get all tracks */
    std::vector<std::unique_ptr<Track>>& getTracks() { return m_tracks; }
    const std::vector<std::unique_ptr<Track>>& getTracks() const { return m_tracks; }
    
    /** Get track count */
    int getTrackCount() const { return static_cast<int>(m_tracks.size()); }
    
    /** Clear all tracks */
    void clearTracks();
    
    //==========================================================================
    // Scene Management (for Async Pattern Engine)
    
    /** Set current scene index (0-63) */
    void setSceneIndex(int index);
    int getSceneIndex() const { return m_sceneIndex; }
    
    /** Set scene name */
    void setSceneName(int index, const juce::String& name);
    juce::String getSceneName(int index) const;
    
    /** Mark scene as used/empty */
    void setSceneUsed(int index, bool used);
    bool isSceneUsed(int index) const;
    
    //==========================================================================
    // Snapshot Management (for Pattern Morphing)
    
    /** Capture current state as snapshot */
    int captureSnapshot(const juce::String& name = "Snapshot");
    
    /** Recall a snapshot */
    void recallSnapshot(int index);
    
    /** Get snapshot by index */
    Snapshot* getSnapshot(int index);
    const Snapshot* getSnapshot(int index) const;
    
    /** Get snapshot count */
    int getSnapshotCount() const { return static_cast<int>(m_snapshots.size()); }
    
    /** Remove a snapshot */
    void removeSnapshot(int index);
    
    /** Clear all snapshots */
    void clearSnapshots();
    
    //==========================================================================
    // Pattern Morphing
    
    /** Morph between two snapshots (0.0 = snapA, 1.0 = snapB) */
    void morphSnapshots(int indexA, int indexB, float position);
    
    /** Set morph quantization (0 = smooth, 1/4/8/16 = quantized) */
    void setMorphQuantization(int quantization) { m_morphQuantization = quantization; }
    int getMorphQuantization() const { return m_morphQuantization; }
    
    /** Enable/disable morph interpolation */
    void setMorphInterpolation(bool enabled) { m_morphInterpolation = enabled; }
    bool isMorphInterpolation() const { return m_morphInterpolation; }
    
    //==========================================================================
    // Global Pattern Settings
    
    /** Set global swing (affects all tracks) */
    void setGlobalSwing(float swing);
    float getGlobalSwing() const { return m_globalSwing; }
    
    /** Set global gate length multiplier */
    void setGlobalGateLength(float multiplier);
    float getGlobalGateLength() const { return m_globalGateLength; }
    
    /** Set pattern loop length in bars */
    void setLoopLength(int bars);
    int getLoopLength() const { return m_loopLength; }
    
    //==========================================================================
    // State Management
    
    /** Reset pattern to initial state */
    void reset();
    
    /** Reset all playback positions */
    void resetPositions();
    
    /** Check if pattern has been modified */
    bool isModified() const { return m_modified; }
    void setModified(bool modified) { m_modified = modified; }
    
    //==========================================================================
    // Serialization
    
    /** Convert to ValueTree for saving */
    juce::ValueTree toValueTree() const;
    
    /** Load from ValueTree */
    void fromValueTree(const juce::ValueTree& tree);
    
    /** Export to JSON */
    juce::String toJSON() const;
    
    /** Import from JSON */
    bool fromJSON(const juce::String& json);
    
private:
    //==========================================================================
    // Pattern info
    juce::String m_name = "New Pattern";
    juce::String m_author;
    juce::String m_description;
    
    // Timing
    float m_bpm = 120.0f;
    int m_timeSignatureNum = 4;
    int m_timeSignatureDenom = 4;
    int m_loopLength = 4;  // bars
    
    // Tracks
    std::vector<std::unique_ptr<Track>> m_tracks;
    
    // Scene management (Async Pattern Engine)
    int m_sceneIndex = 0;
    std::array<juce::String, 64> m_sceneNames;
    std::array<bool, 64> m_scenesUsed{false};
    
    // Snapshots (Pattern Morphing)
    std::vector<std::unique_ptr<Snapshot>> m_snapshots;
    int m_morphQuantization = 0;  // 0, 1, 4, 8, 16
    bool m_morphInterpolation = true;
    
    // Global modifiers
    float m_globalSwing = 50.0f;
    float m_globalGateLength = 1.0f;
    
    // State
    bool m_modified = false;
    
    // Helper for morphing
    void interpolateValue(float& target, float valueA, float valueB, float position);
    void interpolateStage(Stage& target, const Stage& stageA, const Stage& stageB, float position);
    
    JUCE_LEAK_DETECTOR(Pattern)
};

} // namespace HAM