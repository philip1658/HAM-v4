#include "RegressionDetector.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <numeric>
#include <filesystem>

namespace HAM::Performance {

// Simple JSON parser for benchmark results (if nlohmann/json not available)
namespace {
    // Basic JSON parsing helper
    std::map<std::string, RegressionDetector::BenchmarkResult> parseGoogleBenchmarkJSON(const std::string& content) {
        std::map<std::string, RegressionDetector::BenchmarkResult> results;
        
        // This is a simplified parser - in production, use nlohmann/json
        // Parse Google Benchmark JSON format
        size_t pos = content.find("\"benchmarks\":");
        if (pos == std::string::npos) return results;
        
        pos = content.find("[", pos);
        if (pos == std::string::npos) return results;
        
        size_t end = content.find("]", pos);
        if (end == std::string::npos) return results;
        
        // Simple regex-like parsing for each benchmark entry
        size_t bench_start = pos;
        while ((bench_start = content.find("{", bench_start + 1)) < end) {
            size_t bench_end = content.find("}", bench_start);
            if (bench_end == std::string::npos || bench_end > end) break;
            
            std::string bench_str = content.substr(bench_start, bench_end - bench_start + 1);
            
            RegressionDetector::BenchmarkResult result;
            
            // Extract name
            size_t name_pos = bench_str.find("\"name\":");
            if (name_pos != std::string::npos) {
                size_t quote_start = bench_str.find("\"", name_pos + 7);
                size_t quote_end = bench_str.find("\"", quote_start + 1);
                result.name = bench_str.substr(quote_start + 1, quote_end - quote_start - 1);
            }
            
            // Extract real_time
            size_t real_pos = bench_str.find("\"real_time\":");
            if (real_pos != std::string::npos) {
                size_t num_start = real_pos + 12;
                size_t num_end = bench_str.find_first_of(",}", num_start);
                result.real_time_ns = std::stod(bench_str.substr(num_start, num_end - num_start));
            }
            
            // Extract cpu_time
            size_t cpu_pos = bench_str.find("\"cpu_time\":");
            if (cpu_pos != std::string::npos) {
                size_t num_start = cpu_pos + 11;
                size_t num_end = bench_str.find_first_of(",}", num_start);
                result.cpu_time_ns = std::stod(bench_str.substr(num_start, num_end - num_start));
            }
            
            // Extract iterations
            size_t iter_pos = bench_str.find("\"iterations\":");
            if (iter_pos != std::string::npos) {
                size_t num_start = iter_pos + 13;
                size_t num_end = bench_str.find_first_of(",}", num_start);
                result.iterations = std::stoll(bench_str.substr(num_start, num_end - num_start));
            }
            
            if (!result.name.empty()) {
                results[result.name] = result;
            }
            
            bench_start = bench_end;
        }
        
        return results;
    }
}

std::map<std::string, RegressionDetector::BenchmarkResult> 
RegressionDetector::loadResults(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open benchmark results file: " + filename);
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    
    return parseGoogleBenchmarkJSON(buffer.str());
}

RegressionDetector::ComparisonReport 
RegressionDetector::compare(const std::map<std::string, BenchmarkResult>& baseline,
                            const std::map<std::string, BenchmarkResult>& current) {
    ComparisonReport report;
    report.baseline_results = baseline;
    report.current_results = current;
    
    for (const auto& [name, current_result] : current) {
        auto baseline_it = baseline.find(name);
        if (baseline_it == baseline.end()) {
            // New benchmark, skip comparison
            continue;
        }
        
        const auto& baseline_result = baseline_it->second;
        
        // Check real time regression
        if (isRegression(baseline_result.real_time_ns, current_result.real_time_ns)) {
            Regression reg;
            reg.benchmark_name = name;
            reg.metric_name = "real_time";
            reg.baseline_value = baseline_result.real_time_ns;
            reg.current_value = current_result.real_time_ns;
            reg.regression_percent = calculateRegressionPercent(
                baseline_result.real_time_ns, current_result.real_time_ns);
            reg.is_critical = violatesThresholds(current_result);
            report.regressions.push_back(reg);
        } else if (isImprovement(baseline_result.real_time_ns, current_result.real_time_ns)) {
            double improvement = -calculateRegressionPercent(
                baseline_result.real_time_ns, current_result.real_time_ns);
            std::stringstream ss;
            ss << name << "/real_time: " << std::fixed << std::setprecision(1) 
               << improvement << "% faster";
            report.improvements.push_back(ss.str());
        }
        
        // Check CPU time regression
        if (isRegression(baseline_result.cpu_time_ns, current_result.cpu_time_ns)) {
            Regression reg;
            reg.benchmark_name = name;
            reg.metric_name = "cpu_time";
            reg.baseline_value = baseline_result.cpu_time_ns;
            reg.current_value = current_result.cpu_time_ns;
            reg.regression_percent = calculateRegressionPercent(
                baseline_result.cpu_time_ns, current_result.cpu_time_ns);
            reg.is_critical = violatesThresholds(current_result);
            report.regressions.push_back(reg);
        }
        
        // Check custom HAM metrics if available
        if (current_result.cpu_usage_percent > 0) {
            if (current_result.cpu_usage_percent > PerformanceThresholds::MAX_CPU_USAGE_PERCENT) {
                Regression reg;
                reg.benchmark_name = name;
                reg.metric_name = "cpu_usage";
                reg.baseline_value = baseline_result.cpu_usage_percent;
                reg.current_value = current_result.cpu_usage_percent;
                reg.regression_percent = calculateRegressionPercent(
                    baseline_result.cpu_usage_percent, current_result.cpu_usage_percent);
                reg.is_critical = true;
                report.regressions.push_back(reg);
            }
        }
        
        if (current_result.midi_jitter_ms > 0) {
            if (current_result.midi_jitter_ms > PerformanceThresholds::MAX_MIDI_JITTER_MS) {
                Regression reg;
                reg.benchmark_name = name;
                reg.metric_name = "midi_jitter";
                reg.baseline_value = baseline_result.midi_jitter_ms;
                reg.current_value = current_result.midi_jitter_ms;
                reg.regression_percent = calculateRegressionPercent(
                    baseline_result.midi_jitter_ms, current_result.midi_jitter_ms);
                reg.is_critical = true;
                report.regressions.push_back(reg);
            }
        }
    }
    
    return report;
}

bool RegressionDetector::violatesThresholds(const BenchmarkResult& result) const {
    if (result.cpu_usage_percent > PerformanceThresholds::MAX_CPU_USAGE_PERCENT) return true;
    if (result.midi_jitter_ms > PerformanceThresholds::MAX_MIDI_JITTER_MS) return true;
    if (result.audio_latency_ms > PerformanceThresholds::MAX_AUDIO_LATENCY_MS) return true;
    if (result.memory_mb > PerformanceThresholds::MAX_MEMORY_MB) return true;
    return false;
}

// BaselineManager implementation

void BaselineManager::saveBaseline(const std::map<std::string, RegressionDetector::BenchmarkResult>& results,
                                  const std::string& filename) {
    // Archive existing baseline if it exists
    if (baselineExists(filename)) {
        archiveBaseline(filename);
    }
    
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not create baseline file: " + filename);
    }
    
    // Write in Google Benchmark JSON format
    file << "{\n";
    file << "  \"context\": {\n";
    file << "    \"date\": \"" << std::chrono::system_clock::now().time_since_epoch().count() << "\",\n";
    file << "    \"library_build_type\": \"release\"\n";
    file << "  },\n";
    file << "  \"benchmarks\": [\n";
    
    bool first = true;
    for (const auto& [name, result] : results) {
        if (!first) file << ",\n";
        first = false;
        
        file << "    {\n";
        file << "      \"name\": \"" << name << "\",\n";
        file << "      \"iterations\": " << result.iterations << ",\n";
        file << "      \"real_time\": " << result.real_time_ns << ",\n";
        file << "      \"cpu_time\": " << result.cpu_time_ns << ",\n";
        file << "      \"time_unit\": \"ns\"";
        
        // Add custom HAM metrics if present
        if (result.cpu_usage_percent > 0) {
            file << ",\n      \"cpu_usage_percent\": " << result.cpu_usage_percent;
        }
        if (result.memory_mb > 0) {
            file << ",\n      \"memory_mb\": " << result.memory_mb;
        }
        if (result.midi_jitter_ms > 0) {
            file << ",\n      \"midi_jitter_ms\": " << result.midi_jitter_ms;
        }
        if (result.audio_latency_ms > 0) {
            file << ",\n      \"audio_latency_ms\": " << result.audio_latency_ms;
        }
        
        file << "\n    }";
    }
    
    file << "\n  ]\n";
    file << "}\n";
}

std::map<std::string, RegressionDetector::BenchmarkResult> 
BaselineManager::loadBaseline(const std::string& filename) {
    RegressionDetector detector;
    return detector.loadResults(filename);
}

bool BaselineManager::baselineExists(const std::string& filename) const {
    return std::filesystem::exists(filename);
}

double BaselineManager::getBaselineAge(const std::string& filename) const {
    if (!baselineExists(filename)) return -1;
    
    auto file_time = std::filesystem::last_write_time(filename);
    auto now = std::filesystem::file_time_type::clock::now();
    auto duration = now - file_time;
    
    // Convert to days
    auto hours = std::chrono::duration_cast<std::chrono::hours>(duration).count();
    return hours / 24.0;
}

void BaselineManager::archiveBaseline(const std::string& baseline_file) {
    if (!baselineExists(baseline_file)) return;
    
    std::string archive_name = getArchiveFilename();
    std::filesystem::copy_file(baseline_file, archive_name, 
                               std::filesystem::copy_options::overwrite_existing);
}

std::string BaselineManager::getArchiveFilename() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << "baseline_archive_" << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S") << ".json";
    return ss.str();
}

// PerformanceTrendAnalyzer implementation

double PerformanceTrendAnalyzer::TrendData::getSlope() const {
    if (values.size() < 2) return 0.0;
    
    // Simple linear regression
    double n = values.size();
    double sum_x = 0, sum_y = 0, sum_xy = 0, sum_x2 = 0;
    
    for (size_t i = 0; i < values.size(); ++i) {
        sum_x += i;
        sum_y += values[i];
        sum_xy += i * values[i];
        sum_x2 += i * i;
    }
    
    double slope = (n * sum_xy - sum_x * sum_y) / (n * sum_x2 - sum_x * sum_x);
    return slope;
}

bool PerformanceTrendAnalyzer::TrendData::isDeterioriating(double threshold) const {
    return getSlope() > threshold;
}

bool PerformanceTrendAnalyzer::TrendData::isImproving(double threshold) const {
    return getSlope() < -threshold;
}

void PerformanceTrendAnalyzer::addDataPoint(const std::string& metric_name, double value) {
    auto& trend = trends_[metric_name];
    trend.values.push_back(value);
    trend.timestamps.push_back(std::chrono::system_clock::now());
    
    // Limit history size
    if (trend.values.size() > MAX_HISTORY_SIZE) {
        trend.values.erase(trend.values.begin());
        trend.timestamps.erase(trend.timestamps.begin());
    }
}

PerformanceTrendAnalyzer::TrendData 
PerformanceTrendAnalyzer::getTrend(const std::string& metric_name) const {
    auto it = trends_.find(metric_name);
    if (it != trends_.end()) {
        return it->second;
    }
    return TrendData();
}

std::vector<std::string> PerformanceTrendAnalyzer::getDeterioriatingMetrics() const {
    std::vector<std::string> deteriorating;
    
    for (const auto& [name, trend] : trends_) {
        if (trend.isDeterioriating()) {
            deteriorating.push_back(name);
        }
    }
    
    return deteriorating;
}

void PerformanceTrendAnalyzer::generateTrendReport(std::ostream& out) const {
    out << "=== Performance Trend Report ===" << std::endl;
    out << "Metrics tracked: " << trends_.size() << std::endl << std::endl;
    
    for (const auto& [name, trend] : trends_) {
        out << name << ":" << std::endl;
        out << "  Data points: " << trend.values.size() << std::endl;
        
        if (!trend.values.empty()) {
            double min_val = *std::min_element(trend.values.begin(), trend.values.end());
            double max_val = *std::max_element(trend.values.begin(), trend.values.end());
            double avg_val = std::accumulate(trend.values.begin(), trend.values.end(), 0.0) / trend.values.size();
            double slope = trend.getSlope();
            
            out << "  Range: [" << std::fixed << std::setprecision(2) 
                << min_val << " - " << max_val << "]" << std::endl;
            out << "  Average: " << avg_val << std::endl;
            out << "  Trend: " << (slope > 0.01 ? "↗️ Deteriorating" : 
                                   slope < -0.01 ? "↘️ Improving" : "→ Stable") 
                << " (slope: " << slope << ")" << std::endl;
        }
        out << std::endl;
    }
}

} // namespace HAM::Performance