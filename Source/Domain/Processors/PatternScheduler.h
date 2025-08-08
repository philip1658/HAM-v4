/*
  ==============================================================================

    PatternScheduler.h
    Handles pattern transitions, chaining, and morphing
    Lock-free implementation for real-time audio processing

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../Models/Pattern.h"
#include <atomic>
#include <memory>
#include <array>

namespace HAM
{

/**
 * Manages pattern scheduling and transitions
 * Lock-free thread-safe for real-time audio processing
 */
class PatternScheduler
{
public:
    //==============================================================================
    // Transition modes
    
    enum class TransitionMode
    {
        IMMEDIATE,      // Switch immediately
        NEXT_BEAT,      // Switch on next beat
        NEXT_BAR,       // Switch on next bar
        NEXT_PATTERN,   // Switch at pattern end
        QUANTIZED_4,    // Switch on 4-beat boundary
        QUANTIZED_8,    // Switch on 8-beat boundary
        QUANTIZED_16    // Switch on 16-beat boundary
    };
    
    enum class ChainMode
    {
        OFF,            // No chaining
        SEQUENTIAL,     // Play patterns in order
        RANDOM,         // Random pattern selection
        PLAYLIST        // Follow playlist order
    };
    
    //==============================================================================
    // Pattern queue entry
    
    struct QueuedPattern
    {
        std::shared_ptr<Pattern> pattern;
        TransitionMode transitionMode{TransitionMode::IMMEDIATE};
        int64_t scheduledPosition{0};  // Sample position for transition
        float morphAmount{0.0f};        // For morphing transitions (0.0-1.0)
        std::atomic<bool> isPending{false};
        std::atomic<bool> isValid{false};
    };
    
    //==============================================================================
    // Constants
    
    static constexpr int MAX_QUEUED_PATTERNS = 16;
    static constexpr int MAX_CHAIN_LENGTH = 128;
    
    //==============================================================================
    // Constructor/Destructor
    
    PatternScheduler();
    ~PatternScheduler() = default;
    
    //==============================================================================
    // Pattern Management
    
    /**
     * Queue a pattern for playback (lock-free)
     * @param pattern Pattern to queue
     * @param mode Transition mode
     * @param morphTime Time to morph (0 for instant)
     * @return True if successfully queued
     */
    bool queuePattern(std::shared_ptr<Pattern> pattern, 
                     TransitionMode mode,
                     float morphTime = 0.0f);
    
    /**
     * Get current active pattern
     * @return Current pattern or nullptr
     */
    std::shared_ptr<Pattern> getCurrentPattern() const;
    
    /**
     * Get next queued pattern
     * @return Next pattern or nullptr
     */
    std::shared_ptr<Pattern> getNextPattern() const;
    
    /**
     * Process pattern transitions
     * @param currentSamplePosition Current position in samples
     * @param samplesPerBeat Samples per beat
     * @param beatsPerBar Beats per bar
     * @return True if pattern changed
     */
    bool processTransitions(int64_t currentSamplePosition,
                           int samplesPerBeat,
                           int beatsPerBar);
    
    //==============================================================================
    // Pattern Chaining
    
    /** Set chain mode */
    void setChainMode(ChainMode mode) { m_chainMode.store(static_cast<int>(mode)); }
    
    /** Get chain mode */
    ChainMode getChainMode() const { return static_cast<ChainMode>(m_chainMode.load()); }
    
    /** Set pattern chain (list of pattern indices) */
    void setPatternChain(const std::vector<int>& chain);
    
    /** Add pattern to chain */
    bool addToChain(int patternIndex);
    
    /** Clear pattern chain */
    void clearChain();
    
    //==============================================================================
    // Pattern Morphing
    
    /**
     * Get morphed pattern data
     * @param morphAmount Current morph position (0.0-1.0)
     * @return Morphed pattern or current if not morphing
     */
    std::shared_ptr<Pattern> getMorphedPattern(float morphAmount) const;
    
    /** Check if currently morphing */
    bool isMorphing() const { return m_isMorphing.load(); }
    
    /** Get current morph amount */
    float getMorphAmount() const { return m_morphAmount.load(); }
    
    /** Set morph speed (patterns per second) */
    void setMorphSpeed(float speed) { m_morphSpeed.store(speed); }
    
    //==============================================================================
    // Pattern Bank Management
    
    /** Set pattern bank for chain mode */
    void setPatternBank(const std::vector<std::shared_ptr<Pattern>>& bank);
    
    /** Get pattern from bank */
    std::shared_ptr<Pattern> getPatternFromBank(int index) const;
    
    /** Get bank size */
    int getBankSize() const { return m_bankSize.load(); }
    
    //==============================================================================
    // Transition Utilities
    
    /**
     * Calculate next transition point
     * @param currentPosition Current sample position
     * @param mode Transition mode
     * @param samplesPerBeat Samples per beat
     * @param beatsPerBar Beats per bar
     * @return Sample position for transition
     */
    int64_t calculateTransitionPoint(int64_t currentPosition,
                                    TransitionMode mode,
                                    int samplesPerBeat,
                                    int beatsPerBar);
    
    /** Clear all queued patterns */
    void clearQueue();
    
    /** Reset scheduler state */
    void reset();
    
    //==============================================================================
    // State queries
    
    /** Check if patterns are queued */
    bool hasQueuedPatterns() const;
    
    /** Get queue size */
    int getQueueSize() const { return m_queueSize.load(); }
    
    /** Get current chain position */
    int getChainPosition() const { return m_chainPosition.load(); }
    
private:
    //==============================================================================
    // Lock-free queue implementation
    
    std::array<QueuedPattern, MAX_QUEUED_PATTERNS> m_patternQueue;
    std::atomic<int> m_queueHead{0};
    std::atomic<int> m_queueTail{0};
    std::atomic<int> m_queueSize{0};
    
    //==============================================================================
    // Internal state (atomic for thread safety)
    
    std::atomic<std::shared_ptr<Pattern>> m_currentPattern{nullptr};
    std::atomic<std::shared_ptr<Pattern>> m_nextPattern{nullptr};
    
    std::atomic<int> m_chainMode{0};
    std::atomic<bool> m_isMorphing{false};
    std::atomic<float> m_morphAmount{0.0f};
    std::atomic<float> m_morphSpeed{1.0f};
    std::atomic<int> m_chainPosition{0};
    std::atomic<int> m_bankSize{0};
    std::atomic<int> m_chainLength{0};
    
    // Pattern bank for chaining (fixed size for lock-free access)
    std::array<std::shared_ptr<Pattern>, 128> m_patternBank;
    std::array<int, MAX_CHAIN_LENGTH> m_patternChain;
    
    // Morphing state
    std::atomic<std::shared_ptr<Pattern>> m_morphSourcePattern{nullptr};
    std::atomic<std::shared_ptr<Pattern>> m_morphTargetPattern{nullptr};
    std::atomic<int64_t> m_morphStartPosition{0};
    std::atomic<int64_t> m_morphEndPosition{0};
    
    // Random generator for random chain mode
    std::mt19937 m_randomGenerator;
    
    //==============================================================================
    // Internal helpers
    
    void processChainAdvance();
    void processMorphing(int64_t currentPosition);
    std::shared_ptr<Pattern> selectNextChainPattern();
    void startMorphTransition(std::shared_ptr<Pattern> targetPattern, float morphTime);
    
    // Lock-free queue helpers
    bool pushToQueue(const QueuedPattern& pattern);
    bool popFromQueue(QueuedPattern& pattern);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PatternScheduler)
};

} // namespace HAM