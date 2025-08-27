/*
  ==============================================================================

    ScaleSlotManager.h
    Global scale slot management system (Metropolix-style)
    
    Features:
    - 8 global scale slots shared by all tracks
    - Each slot can hold factory scale or user scale with root
    - Bar-quantized scale switching
    - Thread-safe real-time operation
    - Singleton pattern for global access

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Scale.h"
#include <array>
#include <atomic>
#include <memory>

namespace HAM {

//==============================================================================
/**
 * ScaleSlot represents a single scale slot that can hold a scale and optional root
 */
struct ScaleSlot
{
    Scale scale;                    // The scale definition
    int rootNote = -1;              // Optional root (-1 = use global root)
    juce::String displayName;       // User-friendly name (e.g., "G Dorian")
    bool isUserScale = false;       // True if user-defined (stores root)
    
    ScaleSlot() = default;
    ScaleSlot(const Scale& s, const juce::String& name) 
        : scale(s), displayName(name) {}
};

//==============================================================================
/**
 * ScaleSlotManager manages 8 global scale slots for the entire application
 * Singleton pattern ensures single global instance
 */
class ScaleSlotManager
{
public:
    //==========================================================================
    // Singleton access
    static ScaleSlotManager& getInstance();
    
    //==========================================================================
    // Scale slot management
    
    /** Load a scale into a slot (0-7) */
    void setSlot(int slotIndex, const Scale& scale, const juce::String& name);
    
    /** Load a user scale with specific root into a slot */
    void setUserSlot(int slotIndex, const Scale& scale, int rootNote, const juce::String& name);
    
    /** Get scale from slot */
    const ScaleSlot& getSlot(int slotIndex) const;
    
    /** Get currently active slot index */
    int getActiveSlotIndex() const { return m_activeSlot.load(); }
    
    /** Get currently active scale */
    const Scale& getActiveScale() const;
    
    /** Get effective root for a track (global + track offset) */
    int getEffectiveRoot(int trackRootOffset = 0) const;
    
    //==========================================================================
    // Scale switching
    
    /** Select a scale slot (immediate UI update, quantized audio change) */
    void selectSlot(int slotIndex);
    
    /** Check if scale change is pending */
    bool isChangePending() const { return m_pendingChange.load(); }
    
    /** Get pending slot index */
    int getPendingSlot() const { return m_pendingSlot; }
    
    /** Execute pending scale change (called at bar boundary by audio engine) */
    void executePendingChange();
    
    /** Cancel pending scale change */
    void cancelPendingChange();
    
    /** Check if there's a pending scale change */
    bool hasPendingChange() const { return m_pendingChange.load(); }
    
    /** Apply the pending scale change (alias for executePendingChange) */
    void applyPendingChange() { executePendingChange(); }
    
    //==========================================================================
    // Global root control
    
    /** Set global root note (0-11, C=0) */
    void setGlobalRoot(int rootNote);
    
    /** Get global root note */
    int getGlobalRoot() const { return m_globalRoot.load(); }
    
    //==========================================================================
    // Initialization
    
    /** Initialize with default scales */
    void initializeDefaults();
    
    //==========================================================================
    // Serialization
    
    /** Save state to ValueTree */
    juce::ValueTree toValueTree() const;
    
    /** Load state from ValueTree */
    void fromValueTree(const juce::ValueTree& tree);
    
    //==========================================================================
    // Listener interface
    
    class Listener
    {
    public:
        virtual ~Listener() = default;
        
        /** Called when a slot is selected (UI should update immediately) */
        virtual void scaleSlotSelected(int slotIndex) {}
        
        /** Called when scale actually changes (at bar boundary) */
        virtual void scaleChanged(int slotIndex) {}
        
        /** Called when global root changes */
        virtual void globalRootChanged(int rootNote) {}
    };
    
    void addListener(Listener* listener);
    void removeListener(Listener* listener);
    
private:
    //==========================================================================
    ScaleSlotManager();
    ~ScaleSlotManager() = default;
    
    // Scale slots (8 slots as per Metropolix)
    std::array<ScaleSlot, 8> m_slots;
    
    // Active slot and pending changes
    std::atomic<int> m_activeSlot{0};
    std::atomic<bool> m_pendingChange{false};
    std::atomic<int> m_pendingSlot{0};
    
    // Global root note (0-11, C=0)
    std::atomic<int> m_globalRoot{0};
    
    // Thread safety
    mutable juce::ReadWriteLock m_slotLock;
    
    // Listeners
    juce::ListenerList<Listener> m_listeners;
    
    // Helper to notify listeners
    void notifySlotSelected(int slotIndex);
    void notifyScaleChanged(int slotIndex);
    void notifyGlobalRootChanged(int rootNote);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScaleSlotManager)
};

//==============================================================================
/**
 * Helper class to convert between scale degrees and MIDI notes
 * This is the core mapping logic used throughout the sequencer
 */
class ScaleDegreeMapper
{
public:
    /** Convert scale degree to MIDI note using current scale and root */
    static int degreeToMidiNote(
        int scaleDegree,           // Scale degree (can be negative or > scale size)
        const Scale& scale,        // Active scale
        int effectiveRoot,         // Root note after track offset (0-11)
        int baseOctave = 5         // Base octave for degree 0 (C4 = octave 5)
    );
    
    /** Convert MIDI note to nearest scale degree */
    static int midiNoteToDegree(
        int midiNote,
        const Scale& scale,
        int effectiveRoot,
        int baseOctave = 5
    );
    
    /** Get octave offset for a scale degree */
    static int getDegreeOctave(int scaleDegree, int scaleSize);
    
    /** Get degree within octave (0 to scaleSize-1) */
    static int getDegreeinOctave(int scaleDegree, int scaleSize);
};

} // namespace HAM