/*
  ==============================================================================

    PitchEngine.cpp
    Pitch processing and quantization implementation

  ==============================================================================
*/

#include "PitchEngine.h"
#include "../Models/Track.h"
#include <algorithm>
#include <cmath>

namespace HAM
{

//==============================================================================
PitchEngine::PitchEngine()
{
    // Initialize with C Major scale
    m_currentScale = std::make_unique<Scale>(Scale::Major);
    
    // Initialize chord tones to invalid
    for (auto& tone : m_chordTones)
        tone.store(-1);
    
    // Initialize custom intervals
    for (auto& interval : m_customIntervals)
        interval.store(-1);
}

//==============================================================================
PitchEngine::PitchResult PitchEngine::processPitch(const Stage& stage, 
                                                  int baseNote,
                                                  int accumulatorOffset)
{
    PitchResult result;
    
    // Start with stage pitch + base note + accumulator offset
    int rawPitch = stage.getPitch() + baseNote + accumulatorOffset;
    
    // Apply transposition
    rawPitch += m_transposition.load();
    
    // Apply octave offset from stage
    int octaveOffset = stage.getOctave();
    rawPitch = applyOctaveOffset(rawPitch, octaveOffset);
    
    // Quantize based on mode
    QuantizationMode mode = getQuantizationMode();
    result.wasQuantized = (mode != QuantizationMode::CHROMATIC);
    
    switch (mode)
    {
        case QuantizationMode::SCALE:
            result.midiNote = quantizeToScale(rawPitch);
            break;
            
        case QuantizationMode::CHORD:
            result.midiNote = quantizeToChord(rawPitch);
            break;
            
        case QuantizationMode::CUSTOM:
            result.midiNote = quantizeToCustomScale(rawPitch);
            break;
            
        case QuantizationMode::CHROMATIC:
        default:
            result.midiNote = rawPitch;
            break;
    }
    
    // Apply note range limiting
    result.midiNote = limitToMidiRange(result.midiNote);
    
    // Calculate additional info
    result.octave = (result.midiNote / 12) - 1;
    
    // Calculate scale degree if quantized to scale
    if (mode == QuantizationMode::SCALE && m_currentScale)
    {
        int noteInOctave = result.midiNote % 12;
        int rootInOctave = m_rootNote.load() % 12;
        int intervalFromRoot = (noteInOctave - rootInOctave + 12) % 12;
        
        auto intervals = m_currentScale->getIntervals();
        auto it = std::find(intervals.begin(), intervals.end(), intervalFromRoot);
        if (it != intervals.end())
        {
            result.scaleDegree = static_cast<int>(std::distance(intervals.begin(), it));
        }
        else
        {
            result.scaleDegree = -1; // Not in scale (shouldn't happen after quantization)
        }
    }
    else
    {
        result.scaleDegree = -1;
    }
    
    // Calculate pitch bend if stage has pitch modulation
    result.pitchBend = stage.getPitchBend();
    
    return result;
}

//==============================================================================
int PitchEngine::quantizeToScale(int midiNote, bool snapUp)
{
    if (!m_currentScale)
        return midiNote;
    
    // Get scale intervals
    auto intervals = m_currentScale->getIntervals();
    if (intervals.empty())
        return midiNote;
    
    int root = m_rootNote.load();
    int octave = midiNote / 12;
    int noteInOctave = midiNote % 12;
    int rootInOctave = root % 12;
    
    // Build scale tones for this octave
    std::vector<int> scaleTones;
    for (int interval : intervals)
    {
        int tone = octave * 12 + rootInOctave + interval;
        scaleTones.push_back(tone);
    }
    
    // Also add tones from adjacent octaves for better quantization
    if (octave > 0)
    {
        for (int interval : intervals)
        {
            int tone = (octave - 1) * 12 + rootInOctave + interval;
            scaleTones.push_back(tone);
        }
    }
    if (octave < 10)
    {
        for (int interval : intervals)
        {
            int tone = (octave + 1) * 12 + rootInOctave + interval;
            scaleTones.push_back(tone);
        }
    }
    
    // Sort tones
    std::sort(scaleTones.begin(), scaleTones.end());
    
    // Find nearest scale tone
    return findNearestScaleTone(midiNote, scaleTones, snapUp);
}

//==============================================================================
int PitchEngine::quantizeToChord(int midiNote)
{
    int numTones = m_numChordTones.load();
    if (numTones == 0)
        return midiNote; // No chord tones, return unquantized
    
    // Build list of chord tones across all octaves
    std::vector<int> chordTones;
    for (int octave = 0; octave < 11; ++octave)
    {
        for (int i = 0; i < numTones; ++i)
        {
            int tone = m_chordTones[i].load();
            if (tone >= 0)
            {
                int transposed = (tone % 12) + (octave * 12);
                if (transposed >= 0 && transposed <= 127)
                    chordTones.push_back(transposed);
            }
        }
    }
    
    if (chordTones.empty())
        return midiNote;
    
    // Find nearest chord tone
    return findNearestScaleTone(midiNote, chordTones, true);
}

//==============================================================================
int PitchEngine::quantizeToCustomScale(int midiNote)
{
    int numIntervals = m_numCustomIntervals.load();
    if (numIntervals == 0)
        return midiNote; // No custom scale, return unquantized
    
    int root = m_rootNote.load();
    int octave = midiNote / 12;
    int rootInOctave = root % 12;
    
    // Build custom scale tones
    std::vector<int> scaleTones;
    for (int oct = std::max(0, octave - 1); oct <= std::min(10, octave + 1); ++oct)
    {
        for (int i = 0; i < numIntervals; ++i)
        {
            int interval = m_customIntervals[i].load();
            if (interval >= 0)
            {
                int tone = oct * 12 + rootInOctave + interval;
                if (tone >= 0 && tone <= 127)
                    scaleTones.push_back(tone);
            }
        }
    }
    
    if (scaleTones.empty())
        return midiNote;
    
    std::sort(scaleTones.begin(), scaleTones.end());
    return findNearestScaleTone(midiNote, scaleTones, true);
}

//==============================================================================
int PitchEngine::findNearestScaleTone(int midiNote, const std::vector<int>& scaleTones, bool snapUp)
{
    if (scaleTones.empty())
        return midiNote;
    
    // Find the closest tone
    auto it = std::lower_bound(scaleTones.begin(), scaleTones.end(), midiNote);
    
    if (it == scaleTones.end())
    {
        // Note is higher than all scale tones
        return scaleTones.back();
    }
    else if (it == scaleTones.begin())
    {
        // Note is lower than all scale tones
        return scaleTones.front();
    }
    else
    {
        // Note is between two scale tones
        int higher = *it;
        int lower = *(--it);
        
        int distToHigher = higher - midiNote;
        int distToLower = midiNote - lower;
        
        if (distToHigher == distToLower)
        {
            // Equal distance, use snap direction
            return snapUp ? higher : lower;
        }
        else
        {
            // Return closest
            return (distToHigher < distToLower) ? higher : lower;
        }
    }
}

//==============================================================================
int PitchEngine::applyOctaveOffset(int midiNote, int octaveOffset)
{
    return midiNote + (octaveOffset * 12);
}

//==============================================================================
int PitchEngine::limitToMidiRange(int midiNote)
{
    int minNote = m_minNote.load();
    int maxNote = m_maxNote.load();
    
    // First clamp to MIDI range
    midiNote = std::clamp(midiNote, 0, 127);
    
    // Then apply user-defined range
    return std::clamp(midiNote, minNote, maxNote);
}

//==============================================================================
void PitchEngine::reset()
{
    // Reset to default state
    m_transposition.store(0);
    m_rootNote.store(60); // Middle C
    
    // Clear chord tones
    for (auto& tone : m_chordTones)
        tone.store(-1);
    
    // Clear custom intervals
    for (auto& interval : m_customIntervals)
        interval.store(-1);
    
    // Reset to C Major scale
    m_currentScale = std::make_unique<Scale>(Scale::Major);
}

//==============================================================================
void PitchEngine::setScale(const Scale& scale)
{
    m_currentScale = std::make_unique<Scale>(scale);
    // Root note is managed separately, not part of Scale
}

//==============================================================================
void PitchEngine::setChordTones(const int* chordTones, int numTones)
{
    numTones = std::min(numTones, 7);
    
    for (int i = 0; i < numTones; ++i)
    {
        m_chordTones[i].store(std::clamp(chordTones[i], 0, 127));
    }
    
    // Clear remaining tones
    for (int i = numTones; i < 7; ++i)
    {
        m_chordTones[i].store(-1);
    }
    
    m_numChordTones.store(numTones);
}

//==============================================================================
void PitchEngine::clearChordTones()
{
    for (auto& tone : m_chordTones)
        tone.store(-1);
    m_numChordTones.store(0);
}

//==============================================================================
void PitchEngine::setCustomScale(const int* intervals, int numIntervals)
{
    numIntervals = std::min(numIntervals, 12);
    
    for (int i = 0; i < numIntervals; ++i)
    {
        m_customIntervals[i].store(std::clamp(intervals[i], 0, 11));
    }
    
    // Clear remaining intervals
    for (int i = numIntervals; i < 12; ++i)
    {
        m_customIntervals[i].store(-1);
    }
    
    m_numCustomIntervals.store(numIntervals);
}

//==============================================================================
void PitchEngine::setNoteRange(int minNote, int maxNote)
{
    // Ensure valid range
    minNote = std::clamp(minNote, 0, 127);
    maxNote = std::clamp(maxNote, 0, 127);
    
    if (minNote > maxNote)
        std::swap(minNote, maxNote);
    
    m_minNote.store(minNote);
    m_maxNote.store(maxNote);
}

//==============================================================================
// TrackPitchProcessor implementation
//==============================================================================

TrackPitchProcessor::TrackPitchProcessor()
{
}

PitchEngine::PitchResult TrackPitchProcessor::processTrackPitch(
    const Track* track,
    int currentStage,
    int accumulatorValue)
{
    PitchEngine::PitchResult result;
    
    if (!track || currentStage < 0 || currentStage >= 8)
    {
        result.midiNote = 60;
        result.octave = 4;
        result.scaleDegree = 0;
        result.wasQuantized = false;
        result.pitchBend = 0.0f;
        return result;
    }
    
    // Get the current stage
    const Stage& stage = track->getStage(currentStage);
    
    // Process pitch with accumulator offset
    result = m_pitchEngine.processPitch(stage, m_baseNote, accumulatorValue);
    
    // Update tracking
    m_lastProcessedStage = currentStage;
    
    return result;
}

void TrackPitchProcessor::updateScale(const Scale& scale)
{
    m_pitchEngine.setScale(scale);
}

void TrackPitchProcessor::reset()
{
    m_lastProcessedStage = -1;
}

} // namespace HAM