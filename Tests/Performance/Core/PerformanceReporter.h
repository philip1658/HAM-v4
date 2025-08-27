#pragma once

#include "PerformanceMetrics.h"
#include "RegressionDetector.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <vector>
#include <map>

namespace HAM::Performance {

/**
 * Performance report generation and formatting
 */
class PerformanceReporter {
public:
    enum class Format {
        Text,
        JSON,
        HTML,
        CSV,
        Markdown
    };
    
    /**
     * Generate performance report from snapshot
     */
    void generateReport(const PerformanceSnapshot& snapshot, 
                       Format format = Format::Text,
                       std::ostream& out = std::cout);
    
    /**
     * Generate comparison report
     */
    void generateComparisonReport(const RegressionDetector::ComparisonReport& report,
                                 Format format = Format::Text,
                                 std::ostream& out = std::cout);
    
    /**
     * Generate trend report
     */
    void generateTrendReport(const PerformanceTrendAnalyzer& analyzer,
                            Format format = Format::Text,
                            std::ostream& out = std::cout);
    
    /**
     * Save report to file
     */
    void saveReport(const std::string& filename, const std::string& content);
    
    /**
     * Format a performance metric value
     */
    std::string formatMetric(const std::string& name, double value, const std::string& unit = "");
    
    /**
     * Check if metric violates threshold
     */
    bool violatesThreshold(const std::string& metric_name, double value) const;
    
private:
    void generateTextReport(const PerformanceSnapshot& snapshot, std::ostream& out);
    void generateJSONReport(const PerformanceSnapshot& snapshot, std::ostream& out);
    void generateHTMLReport(const PerformanceSnapshot& snapshot, std::ostream& out);
    void generateCSVReport(const PerformanceSnapshot& snapshot, std::ostream& out);
    void generateMarkdownReport(const PerformanceSnapshot& snapshot, std::ostream& out);
    
    std::string getStatusEmoji(bool passed) const {
        return passed ? "✅" : "❌";
    }
    
    std::string getStatusText(bool passed) const {
        return passed ? "PASS" : "FAIL";
    }
    
    std::string formatTime(std::chrono::system_clock::time_point time) const;
};

/**
 * Performance dashboard generator
 * Creates comprehensive performance dashboards with charts
 */
class PerformanceDashboard {
public:
    struct ChartData {
        std::string title;
        std::string x_label;
        std::string y_label;
        std::vector<double> x_values;
        std::vector<double> y_values;
        std::vector<std::string> labels;
    };
    
    /**
     * Generate performance dashboard HTML
     */
    std::string generateDashboard(const std::vector<PerformanceSnapshot>& history);
    
    /**
     * Generate CPU usage chart
     */
    ChartData generateCPUChart(const std::vector<PerformanceSnapshot>& history);
    
    /**
     * Generate memory usage chart
     */
    ChartData generateMemoryChart(const std::vector<PerformanceSnapshot>& history);
    
    /**
     * Generate latency distribution chart
     */
    ChartData generateLatencyChart(const std::vector<PerformanceSnapshot>& history);
    
    /**
     * Generate jitter timeline
     */
    ChartData generateJitterTimeline(const std::vector<PerformanceSnapshot>& history);
    
private:
    std::string generateChartHTML(const ChartData& data) const;
    std::string generateChartJS(const std::vector<ChartData>& charts) const;
};

/**
 * CI/CD integration helper
 * Formats reports for various CI systems
 */
class CIReporter {
public:
    enum class CISystem {
        GitHub,
        GitLab,
        Jenkins,
        CircleCI,
        TravisCI
    };
    
    /**
     * Generate CI-specific report
     */
    std::string generateCIReport(const PerformanceSnapshot& snapshot, CISystem system);
    
    /**
     * Generate GitHub Actions annotations
     */
    void generateGitHubAnnotations(const std::vector<RegressionDetector::Regression>& regressions,
                                   std::ostream& out = std::cout);
    
    /**
     * Generate JUnit XML report for Jenkins
     */
    std::string generateJUnitXML(const std::map<std::string, bool>& test_results);
    
    /**
     * Check if running in CI environment
     */
    bool isRunningInCI() const;
    
    /**
     * Get CI system from environment
     */
    CISystem detectCISystem() const;
};

} // namespace HAM::Performance