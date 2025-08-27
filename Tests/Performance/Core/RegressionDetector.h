#pragma once

#include "PerformanceMetrics.h"
#include <vector>
#include <map>
#include <string>
#include <fstream>
#include <json/json.h> // Will use nlohmann/json if available, else basic parsing

namespace HAM::Performance {

/**
 * Performance regression detection
 * Compares current performance against baseline to detect regressions
 */
class RegressionDetector {
public:
    struct BenchmarkResult {
        std::string name;
        double mean_time_ns;
        double median_time_ns;
        double stddev_time_ns;
        double min_time_ns;
        double max_time_ns;
        int64_t iterations;
        double cpu_time_ns;
        double real_time_ns;
        
        // Custom HAM metrics
        double cpu_usage_percent = 0.0;
        double memory_mb = 0.0;
        double midi_jitter_ms = 0.0;
        double audio_latency_ms = 0.0;
    };
    
    struct Regression {
        std::string benchmark_name;
        std::string metric_name;
        double baseline_value;
        double current_value;
        double regression_percent;
        bool is_critical;  // True if exceeds HAM thresholds
        
        std::string toString() const {
            std::stringstream ss;
            ss << benchmark_name << "/" << metric_name << ": "
               << std::fixed << std::setprecision(2)
               << baseline_value << " -> " << current_value 
               << " (" << (regression_percent > 0 ? "+" : "") 
               << regression_percent << "%)";
            if (is_critical) {
                ss << " [CRITICAL]";
            }
            return ss.str();
        }
    };
    
    struct ComparisonReport {
        std::vector<Regression> regressions;
        std::vector<std::string> improvements;
        std::map<std::string, BenchmarkResult> baseline_results;
        std::map<std::string, BenchmarkResult> current_results;
        
        bool hasRegressions() const { return !regressions.empty(); }
        bool hasCriticalRegressions() const {
            return std::any_of(regressions.begin(), regressions.end(),
                              [](const Regression& r) { return r.is_critical; });
        }
        
        void printSummary() const {
            std::cout << "=== Performance Comparison Report ===" << std::endl;
            std::cout << "Benchmarks compared: " << current_results.size() << std::endl;
            
            if (!improvements.empty()) {
                std::cout << "\n✅ Improvements (" << improvements.size() << "):" << std::endl;
                for (const auto& imp : improvements) {
                    std::cout << "  • " << imp << std::endl;
                }
            }
            
            if (!regressions.empty()) {
                std::cout << "\n⚠️ Regressions (" << regressions.size() << "):" << std::endl;
                for (const auto& reg : regressions) {
                    std::cout << "  • " << reg.toString() << std::endl;
                }
            }
            
            if (hasCriticalRegressions()) {
                std::cout << "\n🔴 CRITICAL: Performance regressions exceed HAM thresholds!" << std::endl;
            }
        }
    };
    
    /**
     * Load benchmark results from JSON file
     */
    std::map<std::string, BenchmarkResult> loadResults(const std::string& filename);
    
    /**
     * Compare two sets of benchmark results
     */
    ComparisonReport compare(const std::map<std::string, BenchmarkResult>& baseline,
                             const std::map<std::string, BenchmarkResult>& current);
    
    /**
     * Check if a single metric violates HAM performance thresholds
     */
    bool violatesThresholds(const BenchmarkResult& result) const;
    
    /**
     * Calculate regression percentage between two values
     */
    double calculateRegressionPercent(double baseline, double current) const {
        if (baseline == 0) return 0;
        return ((current - baseline) / baseline) * 100.0;
    }
    
    /**
     * Set custom regression threshold (default is 10%)
     */
    void setRegressionThreshold(double percent) {
        regression_threshold_percent_ = percent;
    }
    
private:
    double regression_threshold_percent_ = PerformanceThresholds::REGRESSION_THRESHOLD_PERCENT;
    
    bool isRegression(double baseline, double current) const {
        double percent_change = calculateRegressionPercent(baseline, current);
        return percent_change > regression_threshold_percent_;
    }
    
    bool isImprovement(double baseline, double current) const {
        double percent_change = calculateRegressionPercent(baseline, current);
        return percent_change < -regression_threshold_percent_;
    }
};

/**
 * Performance baseline manager
 * Manages baseline performance data for regression detection
 */
class BaselineManager {
public:
    /**
     * Save current performance results as new baseline
     */
    void saveBaseline(const std::map<std::string, RegressionDetector::BenchmarkResult>& results,
                      const std::string& filename = "baseline_results.json");
    
    /**
     * Load existing baseline
     */
    std::map<std::string, RegressionDetector::BenchmarkResult> loadBaseline(
        const std::string& filename = "baseline_results.json");
    
    /**
     * Check if baseline exists
     */
    bool baselineExists(const std::string& filename = "baseline_results.json") const;
    
    /**
     * Get baseline age in days
     */
    double getBaselineAge(const std::string& filename = "baseline_results.json") const;
    
    /**
     * Archive current baseline before updating
     */
    void archiveBaseline(const std::string& baseline_file = "baseline_results.json");
    
private:
    std::string getArchiveFilename() const;
};

/**
 * Continuous performance monitoring
 * Tracks performance over time and detects trends
 */
class PerformanceTrendAnalyzer {
public:
    struct TrendData {
        std::vector<double> values;
        std::vector<std::chrono::system_clock::time_point> timestamps;
        
        double getSlope() const;  // Linear regression slope
        bool isDeterioriating(double threshold = 0.01) const;
        bool isImproving(double threshold = 0.01) const;
    };
    
    /**
     * Add performance data point
     */
    void addDataPoint(const std::string& metric_name, double value);
    
    /**
     * Get trend for specific metric
     */
    TrendData getTrend(const std::string& metric_name) const;
    
    /**
     * Detect metrics with deteriorating trends
     */
    std::vector<std::string> getDeterioriatingMetrics() const;
    
    /**
     * Generate trend report
     */
    void generateTrendReport(std::ostream& out) const;
    
    /**
     * Save trend data to file
     */
    void saveTrendData(const std::string& filename) const;
    
    /**
     * Load trend data from file
     */
    void loadTrendData(const std::string& filename);
    
private:
    std::map<std::string, TrendData> trends_;
    static constexpr size_t MAX_HISTORY_SIZE = 1000;
};

} // namespace HAM::Performance