/*
  ==============================================================================

    TimingDiagnostics.h
    Comprehensive timing diagnostics and monitoring system
    
    Provides real-time monitoring of timing accuracy, jitter detection,
    and performance metrics for the HAM sequencer.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "TimingConstants.h"
#include <atomic>
#include <vector>
#include <chrono>
#include <mutex>

namespace HAM {

//==============================================================================
/**
    Comprehensive timing diagnostics system for monitoring sequencer accuracy
    
    Tracks timing precision, detects jitter, monitors performance metrics,
    and provides alerts for timing-related issues.
*/
class TimingDiagnostics
{
public:
    //==========================================================================
    // Diagnostic Data Structures
    
    struct TimingMetrics
    {
        // Jitter measurements (in milliseconds)
        float currentJitter = 0.0f;
        float averageJitter = 0.0f;
        float maxJitter = 0.0f;
        float minJitter = 0.0f;
        
        // Timing accuracy
        double averageDeviation = 0.0;
        double maxDeviation = 0.0;
        int64_t totalSamples = 0;
        
        // Performance metrics
        float cpuUsage = 0.0f;
        int droppedEvents = 0;
        int queuedEvents = 0;
        
        // External sync metrics
        float externalBpmStability = 0.0f;
        double clockDriftMs = 0.0;
        int syncLossCount = 0;
        
        // Update timestamp
        int64_t lastUpdateTime = 0;
    };
    
    struct AlertInfo
    {
        enum Level { INFO, WARNING, CRITICAL };
        
        Level level;
        juce::String message;
        juce::String source;
        int64_t timestamp;
        int count;  // How many times this alert has occurred
    };
    
    //==========================================================================
    // Construction/Destruction
    TimingDiagnostics();
    ~TimingDiagnostics();
    
    //==========================================================================
    // Timing Measurement
    
    /** Record a timing event for jitter analysis
        @param expectedTimeMs Expected timing in milliseconds
        @param actualTimeMs Actual timing in milliseconds
        @param source Identifier for the timing source (e.g., "MasterClock", "MIDI")
    */
    void recordTimingEvent(double expectedTimeMs, double actualTimeMs, const juce::String& source);
    
    /** Record MIDI event timing
        @param scheduledSample When the event was supposed to occur
        @param actualSample When the event actually occurred
        @param bufferSize Current buffer size for context
    */
    void recordMidiEventTiming(int scheduledSample, int actualSample, int bufferSize);
    
    /** Record buffer processing performance
        @param processingTimeMs Time taken to process buffer
        @param bufferSizeMs Duration of buffer in milliseconds
    */
    void recordBufferPerformance(double processingTimeMs, double bufferSizeMs);
    
    //==========================================================================
    // External Sync Monitoring
    
    /** Record external BPM measurement for stability analysis */
    void recordExternalBPM(float bpm, const juce::String& source);
    
    /** Record clock drift measurement */
    void recordClockDrift(double driftMs, const juce::String& source);
    
    /** Record sync loss event */
    void recordSyncLoss(const juce::String& reason);
    
    //==========================================================================
    // Data Access
    
    /** Get current timing metrics (thread-safe) */
    TimingMetrics getTimingMetrics() const;
    
    /** Get recent alerts (thread-safe) */
    std::vector<AlertInfo> getRecentAlerts(int maxCount = 10) const;
    
    /** Check if there are any critical timing issues */
    bool hasCriticalIssues() const;
    
    /** Get jitter trend over time (last N measurements) */
    std::vector<float> getJitterTrend(int numSamples = 100) const;
    
    //==========================================================================
    // Configuration
    
    /** Enable/disable automatic alert generation */
    void setAlertsEnabled(bool enabled) { m_alertsEnabled.store(enabled); }
    
    /** Set jitter threshold for warnings (milliseconds) */
    void setJitterWarningThreshold(float thresholdMs) { m_jitterWarningThreshold = thresholdMs; }
    
    /** Set jitter threshold for critical alerts (milliseconds) */
    void setJitterCriticalThreshold(float thresholdMs) { m_jitterCriticalThreshold = thresholdMs; }
    
    /** Reset all diagnostic data */
    void reset();
    
private:
    //==========================================================================
    // Internal Data
    
    // Thread-safe metrics
    std::atomic<float> m_currentJitter{0.0f};
    std::atomic<float> m_averageJitter{0.0f};
    std::atomic<float> m_maxJitter{0.0f};
    std::atomic<double> m_averageDeviation{0.0};
    std::atomic<int64_t> m_totalSamples{0};
    std::atomic<int> m_droppedEvents{0};
    std::atomic<int> m_queuedEvents{0};
    std::atomic<float> m_cpuUsage{0.0f};
    
    // Jitter history for trend analysis
    mutable std::mutex m_jitterMutex;
    std::vector<float> m_jitterHistory;
    static constexpr size_t MAX_JITTER_HISTORY = 1000;
    
    // BPM stability tracking
    mutable std::mutex m_bpmMutex;
    std::vector<float> m_recentBPMs;
    std::chrono::steady_clock::time_point m_lastBpmTime;
    static constexpr size_t MAX_BPM_HISTORY = 50;
    
    // Alert system
    std::atomic<bool> m_alertsEnabled{true};
    mutable std::mutex m_alertsMutex;
    std::vector<AlertInfo> m_alerts;
    static constexpr size_t MAX_ALERTS = 100;
    
    // Thresholds
    float m_jitterWarningThreshold{TimingConstants::TIMING_JITTER_THRESHOLD_MS};
    float m_jitterCriticalThreshold{TimingConstants::TIMING_JITTER_THRESHOLD_MS * 2.0f};
    
    // Running statistics
    double m_jitterSum{0.0};
    double m_deviationSum{0.0};
    int64_t m_measurementCount{0};
    
    // Performance tracking
    std::chrono::steady_clock::time_point m_lastUpdateTime;
    
    //==========================================================================
    // Internal Methods
    
    /** Update jitter statistics */
    void updateJitterStatistics(float jitter);
    
    /** Generate alert if thresholds are exceeded */
    void checkAndGenerateAlerts(float jitter, const juce::String& source);
    
    /** Add alert to the system */
    void addAlert(AlertInfo::Level level, const juce::String& message, const juce::String& source);
    
    /** Calculate BPM stability (coefficient of variation) */
    float calculateBpmStability() const;
    
    /** Trim containers to maximum size */
    void trimContainers();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimingDiagnostics)
};

//==============================================================================
/**
    Singleton access to global timing diagnostics
*/
class TimingDiagnosticsManager
{
public:
    static TimingDiagnostics& getInstance() 
    {
        static TimingDiagnostics instance;
        return instance;
    }
    
private:
    TimingDiagnosticsManager() = default;
    ~TimingDiagnosticsManager() = default;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimingDiagnosticsManager)
};

//==============================================================================
// Convenience macros for easy integration

#define HAM_RECORD_TIMING(expected, actual, source) \
    TimingDiagnosticsManager::getInstance().recordTimingEvent(expected, actual, source)

#define HAM_RECORD_MIDI_TIMING(scheduled, actual, bufferSize) \
    TimingDiagnosticsManager::getInstance().recordMidiEventTiming(scheduled, actual, bufferSize)

#define HAM_RECORD_BUFFER_PERFORMANCE(processingTime, bufferDuration) \
    TimingDiagnosticsManager::getInstance().recordBufferPerformance(processingTime, bufferDuration)

#define HAM_RECORD_SYNC_LOSS(reason) \
    TimingDiagnosticsManager::getInstance().recordSyncLoss(reason)

} // namespace HAM