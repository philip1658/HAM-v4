#pragma once

#include <chrono>
#include <atomic>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

namespace HAM::Performance {

/**
 * Performance thresholds that must not be exceeded
 * Based on HAM v4.0 architecture requirements
 */
struct PerformanceThresholds {
    static constexpr double MAX_CPU_USAGE_PERCENT = 5.0;
    static constexpr double MAX_MIDI_JITTER_MS = 0.1;
    static constexpr double MAX_AUDIO_LATENCY_MS = 5.0;
    static constexpr size_t MAX_MEMORY_MB = 128;
    static constexpr double REGRESSION_THRESHOLD_PERCENT = 10.0;
};

/**
 * Statistical metrics for performance analysis
 */
struct StatisticalMetrics {
    double mean = 0.0;
    double median = 0.0;
    double stddev = 0.0;
    double min = 0.0;
    double max = 0.0;
    double p95 = 0.0;  // 95th percentile
    double p99 = 0.0;  // 99th percentile
    
    void calculate(const std::vector<double>& samples) {
        if (samples.empty()) return;
        
        // Create sorted copy for percentiles
        std::vector<double> sorted = samples;
        std::sort(sorted.begin(), sorted.end());
        
        // Min/Max
        min = sorted.front();
        max = sorted.back();
        
        // Mean
        double sum = 0.0;
        for (double v : samples) sum += v;
        mean = sum / samples.size();
        
        // Median
        size_t mid = sorted.size() / 2;
        median = (sorted.size() % 2 == 0) 
            ? (sorted[mid-1] + sorted[mid]) / 2.0 
            : sorted[mid];
        
        // Standard deviation
        double variance = 0.0;
        for (double v : samples) {
            double diff = v - mean;
            variance += diff * diff;
        }
        stddev = std::sqrt(variance / samples.size());
        
        // Percentiles
        size_t p95_idx = static_cast<size_t>(sorted.size() * 0.95);
        size_t p99_idx = static_cast<size_t>(sorted.size() * 0.99);
        p95 = sorted[std::min(p95_idx, sorted.size() - 1)];
        p99 = sorted[std::min(p99_idx, sorted.size() - 1)];
    }
};

/**
 * CPU usage measurement
 */
class CPUMonitor {
public:
    CPUMonitor() = default;
    
    void startMeasurement() {
        start_time_ = std::chrono::high_resolution_clock::now();
        start_cpu_time_ = getCPUTime();
    }
    
    void endMeasurement() {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto end_cpu_time = getCPUTime();
        
        auto wall_time = std::chrono::duration<double>(end_time - start_time_).count();
        auto cpu_time = end_cpu_time - start_cpu_time_;
        
        if (wall_time > 0) {
            double usage = (cpu_time / wall_time) * 100.0;
            samples_.push_back(usage);
        }
    }
    
    StatisticalMetrics getMetrics() const {
        StatisticalMetrics metrics;
        metrics.calculate(samples_);
        return metrics;
    }
    
    void reset() { samples_.clear(); }
    
private:
    std::chrono::high_resolution_clock::time_point start_time_;
    double start_cpu_time_ = 0.0;
    std::vector<double> samples_;
    
    double getCPUTime() const {
        // Platform-specific implementation would go here
        // For now, using clock() as approximation
        return static_cast<double>(clock()) / CLOCKS_PER_SEC;
    }
};

/**
 * Memory usage tracking
 */
class MemoryMonitor {
public:
    struct MemoryStats {
        size_t current_bytes = 0;
        size_t peak_bytes = 0;
        size_t allocation_count = 0;
        size_t deallocation_count = 0;
    };
    
    void recordAllocation(size_t bytes) {
        current_bytes_.fetch_add(bytes);
        allocation_count_.fetch_add(1);
        
        size_t current = current_bytes_.load();
        size_t peak = peak_bytes_.load();
        while (current > peak && !peak_bytes_.compare_exchange_weak(peak, current));
    }
    
    void recordDeallocation(size_t bytes) {
        current_bytes_.fetch_sub(bytes);
        deallocation_count_.fetch_add(1);
    }
    
    MemoryStats getStats() const {
        return {
            current_bytes_.load(),
            peak_bytes_.load(),
            allocation_count_.load(),
            deallocation_count_.load()
        };
    }
    
    void reset() {
        current_bytes_ = 0;
        peak_bytes_ = 0;
        allocation_count_ = 0;
        deallocation_count_ = 0;
    }
    
private:
    std::atomic<size_t> current_bytes_{0};
    std::atomic<size_t> peak_bytes_{0};
    std::atomic<size_t> allocation_count_{0};
    std::atomic<size_t> deallocation_count_{0};
};

/**
 * Latency measurement
 */
class LatencyMonitor {
public:
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = Clock::time_point;
    
    void recordLatency(double latency_ms) {
        samples_.push_back(latency_ms);
    }
    
    void startEvent(const std::string& event_id) {
        event_starts_[event_id] = Clock::now();
    }
    
    void endEvent(const std::string& event_id) {
        auto it = event_starts_.find(event_id);
        if (it != event_starts_.end()) {
            auto duration = Clock::now() - it->second;
            double latency_ms = std::chrono::duration<double, std::milli>(duration).count();
            recordLatency(latency_ms);
            event_starts_.erase(it);
        }
    }
    
    StatisticalMetrics getMetrics() const {
        StatisticalMetrics metrics;
        metrics.calculate(samples_);
        return metrics;
    }
    
    double getJitter() const {
        if (samples_.size() < 2) return 0.0;
        
        double sum_diff_sq = 0.0;
        for (size_t i = 1; i < samples_.size(); ++i) {
            double diff = samples_[i] - samples_[i-1];
            sum_diff_sq += diff * diff;
        }
        
        return std::sqrt(sum_diff_sq / (samples_.size() - 1));
    }
    
    void reset() { 
        samples_.clear();
        event_starts_.clear();
    }
    
private:
    std::vector<double> samples_;
    std::unordered_map<std::string, TimePoint> event_starts_;
};

/**
 * Thread contention monitoring
 */
class ThreadContentionMonitor {
public:
    void recordLockWait(double wait_ms) {
        lock_wait_times_.push_back(wait_ms);
        total_lock_waits_.fetch_add(1);
    }
    
    void recordContention() {
        contention_count_.fetch_add(1);
    }
    
    struct ContentionStats {
        StatisticalMetrics lock_wait_metrics;
        size_t total_contentions;
        size_t total_lock_waits;
    };
    
    ContentionStats getStats() const {
        ContentionStats stats;
        stats.lock_wait_metrics.calculate(lock_wait_times_);
        stats.total_contentions = contention_count_.load();
        stats.total_lock_waits = total_lock_waits_.load();
        return stats;
    }
    
    void reset() {
        lock_wait_times_.clear();
        contention_count_ = 0;
        total_lock_waits_ = 0;
    }
    
private:
    std::vector<double> lock_wait_times_;
    std::atomic<size_t> contention_count_{0};
    std::atomic<size_t> total_lock_waits_{0};
};

/**
 * Comprehensive performance snapshot
 */
struct PerformanceSnapshot {
    StatisticalMetrics cpu_usage;
    MemoryMonitor::MemoryStats memory;
    StatisticalMetrics audio_latency;
    StatisticalMetrics midi_latency;
    double midi_jitter;
    ThreadContentionMonitor::ContentionStats thread_contention;
    std::chrono::system_clock::time_point timestamp;
    
    bool meetsThresholds() const {
        return cpu_usage.p99 < PerformanceThresholds::MAX_CPU_USAGE_PERCENT &&
               midi_jitter < PerformanceThresholds::MAX_MIDI_JITTER_MS &&
               audio_latency.p99 < PerformanceThresholds::MAX_AUDIO_LATENCY_MS &&
               (memory.peak_bytes / (1024 * 1024)) < PerformanceThresholds::MAX_MEMORY_MB;
    }
};

} // namespace HAM::Performance