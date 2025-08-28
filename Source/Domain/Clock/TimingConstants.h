/*
  ==============================================================================

    TimingConstants.h
    Centralized timing constants and precision definitions
    
    Eliminates magic numbers throughout the HAM codebase for better 
    maintainability and precision control.

  ==============================================================================
*/

#pragma once

#include <cstdint>

namespace HAM {
namespace TimingConstants {

//==============================================================================
// Clock Resolution
static constexpr int PPQN = 24;                    // Pulses per quarter note
static constexpr int PULSES_PER_BAR_4_4 = 96;     // 4 beats * 24 PPQN
static constexpr int BEATS_PER_BAR = 4;           // Standard 4/4 time

//==============================================================================
// Sample Rate Definitions
static constexpr double DEFAULT_SAMPLE_RATE = 48000.0;
static constexpr double FALLBACK_SAMPLE_RATE = 44100.0;
static constexpr double MIN_SAMPLE_RATE = 8000.0;
static constexpr double MAX_SAMPLE_RATE = 192000.0;

//==============================================================================
// BPM Range and Defaults
static constexpr float MIN_BPM = 20.0f;
static constexpr float MAX_BPM = 999.0f;
static constexpr float DEFAULT_BPM = 120.0f;

//==============================================================================
// High-Precision Timing
// Use 64-bit integers for sample-accurate timing without precision loss
using PreciseSampleCount = int64_t;
static constexpr PreciseSampleCount PRECISION_MULTIPLIER = 1000000LL;  // 1 million for sub-sample precision

//==============================================================================
// MIDI Clock Constants
static constexpr double MIDI_CLOCKS_PER_QUARTER = 24.0;
static constexpr double SECONDS_PER_MINUTE = 60.0;
static constexpr double MIDI_CLOCK_TIMEOUT_SECONDS = 0.1;    // Consider clock lost after 100ms

//==============================================================================
// Buffer and Event Handling
static constexpr int MIN_BUFFER_SIZE = 32;
static constexpr int MAX_BUFFER_SIZE = 4096;
static constexpr int DEFAULT_BUFFER_SIZE = 512;

// Event queue sizes (must be power of 2 for lock-free queues)
static constexpr size_t MIDI_EVENT_QUEUE_SIZE = 2048;
static constexpr size_t UI_MESSAGE_QUEUE_SIZE = 1024;

//==============================================================================
// Timing Tolerances and Smoothing
static constexpr float TIMING_JITTER_THRESHOLD_MS = 0.1f;   // Maximum acceptable jitter
static constexpr float BPM_SMOOTHING_FACTOR = 0.1f;         // For external BPM calculation
static constexpr float DRIFT_SMOOTHING_FACTOR = 0.9f;       // For drift accumulation

//==============================================================================
// Voice and Ratchet Constants
static constexpr int MAX_VOICES = 64;
static constexpr int MAX_RATCHETS_PER_PULSE = 8;
static constexpr float DEFAULT_GATE_LENGTH = 0.9f;          // 90% gate length
static constexpr float RATCHET_GATE_LENGTH = 0.9f;          // 90% for ratchets

//==============================================================================
// Velocity and Modulation
static constexpr int MIN_VELOCITY = 1;
static constexpr int MAX_VELOCITY = 127;
static constexpr int DEFAULT_VELOCITY = 100;

static constexpr float MIN_MODULATION = 0.0f;
static constexpr float MAX_MODULATION = 1.0f;

//==============================================================================
// Humanization and Randomization
static constexpr float MAX_TIMING_RANDOMIZATION_MS = 10.0f; // Maximum timing variation
static constexpr int MAX_VELOCITY_RANDOMIZATION = 20;       // Maximum velocity variation

//==============================================================================
// Tempo Glide Constants
static constexpr float DEFAULT_TEMPO_GLIDE_MS = 100.0f;
static constexpr float MIN_TEMPO_GLIDE_MS = 1.0f;
static constexpr float MAX_TEMPO_GLIDE_MS = 5000.0f;

//==============================================================================
// Pattern and Song Structure
static constexpr int MAX_TRACKS = 64;
static constexpr int STAGES_PER_TRACK = 8;
static constexpr int MAX_PATTERNS = 128;
static constexpr int MAX_SCENES = 16;

//==============================================================================
// Performance Monitoring
static constexpr float CPU_WARNING_THRESHOLD = 75.0f;       // Warn at 75% CPU
static constexpr float CPU_CRITICAL_THRESHOLD = 90.0f;      // Critical at 90% CPU
static constexpr int PERFORMANCE_UPDATE_INTERVAL_MS = 100;   // Update every 100ms

//==============================================================================
// Utility Functions for High-Precision Arithmetic

/** Convert samples to precise sample count */
inline constexpr PreciseSampleCount toPreciseSamples(double samples) {
    return static_cast<PreciseSampleCount>(samples * PRECISION_MULTIPLIER);
}

/** Convert precise sample count back to samples */
inline constexpr double fromPreciseSamples(PreciseSampleCount preciseSamples) {
    return static_cast<double>(preciseSamples) / PRECISION_MULTIPLIER;
}

/** Calculate samples per pulse with high precision */
inline constexpr PreciseSampleCount calculatePreciseSamplesPerPulse(double bpm, double sampleRate) {
    double samplesPerQuarter = (SECONDS_PER_MINUTE / bpm) * sampleRate;
    double samplesPerPulse = samplesPerQuarter / PPQN;
    return toPreciseSamples(samplesPerPulse);
}

/** Calculate BPM from MIDI clock interval */
inline double calculateBPMFromInterval(double intervalSeconds) {
    if (intervalSeconds <= 0.0) return DEFAULT_BPM;
    return SECONDS_PER_MINUTE / (intervalSeconds * MIDI_CLOCKS_PER_QUARTER);
}

/** Clamp BPM to valid range */
inline float clampBPM(float bpm) {
    if (bpm < MIN_BPM) return MIN_BPM;
    if (bpm > MAX_BPM) return MAX_BPM;
    return bpm;
}

/** Check if sample rate is valid */
inline bool isValidSampleRate(double sampleRate) {
    return sampleRate >= MIN_SAMPLE_RATE && sampleRate <= MAX_SAMPLE_RATE;
}

} // namespace TimingConstants
} // namespace HAM