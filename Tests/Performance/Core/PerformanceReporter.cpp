#include "PerformanceReporter.h"
#include <ctime>
#include <iomanip>
#include <cstdlib>

namespace HAM::Performance {

void PerformanceReporter::generateReport(const PerformanceSnapshot& snapshot, 
                                        Format format, std::ostream& out) {
    switch (format) {
        case Format::Text:
            generateTextReport(snapshot, out);
            break;
        case Format::JSON:
            generateJSONReport(snapshot, out);
            break;
        case Format::HTML:
            generateHTMLReport(snapshot, out);
            break;
        case Format::CSV:
            generateCSVReport(snapshot, out);
            break;
        case Format::Markdown:
            generateMarkdownReport(snapshot, out);
            break;
    }
}

void PerformanceReporter::generateTextReport(const PerformanceSnapshot& snapshot, std::ostream& out) {
    out << "=== HAM Performance Report ===" << std::endl;
    out << "Timestamp: " << formatTime(snapshot.timestamp) << std::endl;
    out << std::endl;
    
    out << "CPU Usage:" << std::endl;
    out << "  Mean:   " << std::fixed << std::setprecision(2) << snapshot.cpu_usage.mean << "%" << std::endl;
    out << "  Max:    " << snapshot.cpu_usage.max << "%" << std::endl;
    out << "  P99:    " << snapshot.cpu_usage.p99 << "%" << std::endl;
    out << "  Status: " << getStatusText(snapshot.cpu_usage.max < PerformanceThresholds::MAX_CPU_USAGE_PERCENT) << std::endl;
    out << std::endl;
    
    out << "Memory Usage:" << std::endl;
    out << "  Current: " << (snapshot.memory.current_bytes / 1024.0 / 1024.0) << " MB" << std::endl;
    out << "  Peak:    " << (snapshot.memory.peak_bytes / 1024.0 / 1024.0) << " MB" << std::endl;
    out << "  Status:  " << getStatusText((snapshot.memory.peak_bytes / 1024.0 / 1024.0) < PerformanceThresholds::MAX_MEMORY_MB) << std::endl;
    out << std::endl;
    
    out << "Audio Latency:" << std::endl;
    out << "  Mean:   " << std::fixed << std::setprecision(3) << snapshot.audio_latency.mean << " ms" << std::endl;
    out << "  Max:    " << snapshot.audio_latency.max << " ms" << std::endl;
    out << "  P99:    " << snapshot.audio_latency.p99 << " ms" << std::endl;
    out << "  Status: " << getStatusText(snapshot.audio_latency.max < PerformanceThresholds::MAX_AUDIO_LATENCY_MS) << std::endl;
    out << std::endl;
    
    out << "MIDI Performance:" << std::endl;
    out << "  Latency: " << snapshot.midi_latency.mean << " ms" << std::endl;
    out << "  Jitter:  " << std::fixed << std::setprecision(4) << snapshot.midi_jitter << " ms" << std::endl;
    out << "  Status:  " << getStatusText(snapshot.midi_jitter < PerformanceThresholds::MAX_MIDI_JITTER_MS) << std::endl;
    out << std::endl;
    
    out << "Thread Contention:" << std::endl;
    out << "  Contentions: " << snapshot.thread_contention.total_contentions << std::endl;
    out << "  Lock Waits:  " << snapshot.thread_contention.total_lock_waits << std::endl;
    out << std::endl;
    
    out << "Overall Status: " << (snapshot.meetsThresholds() ? "✅ PASS" : "❌ FAIL") << std::endl;
}

void PerformanceReporter::generateJSONReport(const PerformanceSnapshot& snapshot, std::ostream& out) {
    out << "{\n";
    out << "  \"timestamp\": \"" << formatTime(snapshot.timestamp) << "\",\n";
    out << "  \"cpu_usage\": {\n";
    out << "    \"mean\": " << snapshot.cpu_usage.mean << ",\n";
    out << "    \"max\": " << snapshot.cpu_usage.max << ",\n";
    out << "    \"p99\": " << snapshot.cpu_usage.p99 << "\n";
    out << "  },\n";
    out << "  \"memory\": {\n";
    out << "    \"current_mb\": " << (snapshot.memory.current_bytes / 1024.0 / 1024.0) << ",\n";
    out << "    \"peak_mb\": " << (snapshot.memory.peak_bytes / 1024.0 / 1024.0) << "\n";
    out << "  },\n";
    out << "  \"audio_latency\": {\n";
    out << "    \"mean_ms\": " << snapshot.audio_latency.mean << ",\n";
    out << "    \"max_ms\": " << snapshot.audio_latency.max << ",\n";
    out << "    \"p99_ms\": " << snapshot.audio_latency.p99 << "\n";
    out << "  },\n";
    out << "  \"midi\": {\n";
    out << "    \"latency_ms\": " << snapshot.midi_latency.mean << ",\n";
    out << "    \"jitter_ms\": " << snapshot.midi_jitter << "\n";
    out << "  },\n";
    out << "  \"thread_contention\": {\n";
    out << "    \"contentions\": " << snapshot.thread_contention.total_contentions << ",\n";
    out << "    \"lock_waits\": " << snapshot.thread_contention.total_lock_waits << "\n";
    out << "  },\n";
    out << "  \"meets_thresholds\": " << (snapshot.meetsThresholds() ? "true" : "false") << "\n";
    out << "}\n";
}

void PerformanceReporter::generateMarkdownReport(const PerformanceSnapshot& snapshot, std::ostream& out) {
    out << "# HAM Performance Report\n\n";
    out << "**Generated:** " << formatTime(snapshot.timestamp) << "\n\n";
    
    out << "## Summary\n\n";
    out << "| Metric | Value | Threshold | Status |\n";
    out << "|--------|-------|-----------|--------|\n";
    out << "| CPU Usage | " << std::fixed << std::setprecision(2) << snapshot.cpu_usage.max 
        << "% | <" << PerformanceThresholds::MAX_CPU_USAGE_PERCENT << "% | " 
        << getStatusEmoji(snapshot.cpu_usage.max < PerformanceThresholds::MAX_CPU_USAGE_PERCENT) << " |\n";
    out << "| Memory | " << (snapshot.memory.peak_bytes / 1024.0 / 1024.0) 
        << " MB | <" << PerformanceThresholds::MAX_MEMORY_MB << " MB | "
        << getStatusEmoji((snapshot.memory.peak_bytes / 1024.0 / 1024.0) < PerformanceThresholds::MAX_MEMORY_MB) << " |\n";
    out << "| Audio Latency | " << std::fixed << std::setprecision(3) << snapshot.audio_latency.max 
        << " ms | <" << PerformanceThresholds::MAX_AUDIO_LATENCY_MS << " ms | "
        << getStatusEmoji(snapshot.audio_latency.max < PerformanceThresholds::MAX_AUDIO_LATENCY_MS) << " |\n";
    out << "| MIDI Jitter | " << std::fixed << std::setprecision(4) << snapshot.midi_jitter 
        << " ms | <" << PerformanceThresholds::MAX_MIDI_JITTER_MS << " ms | "
        << getStatusEmoji(snapshot.midi_jitter < PerformanceThresholds::MAX_MIDI_JITTER_MS) << " |\n";
    
    out << "\n## Detailed Metrics\n\n";
    
    out << "### CPU Usage\n";
    out << "- Mean: " << snapshot.cpu_usage.mean << "%\n";
    out << "- Median: " << snapshot.cpu_usage.median << "%\n";
    out << "- P95: " << snapshot.cpu_usage.p95 << "%\n";
    out << "- P99: " << snapshot.cpu_usage.p99 << "%\n";
    out << "- Max: " << snapshot.cpu_usage.max << "%\n\n";
    
    out << "### Memory\n";
    out << "- Current: " << (snapshot.memory.current_bytes / 1024.0 / 1024.0) << " MB\n";
    out << "- Peak: " << (snapshot.memory.peak_bytes / 1024.0 / 1024.0) << " MB\n";
    out << "- Allocations: " << snapshot.memory.allocation_count << "\n\n";
    
    out << "### Audio Performance\n";
    out << "- Mean Latency: " << snapshot.audio_latency.mean << " ms\n";
    out << "- Max Latency: " << snapshot.audio_latency.max << " ms\n";
    out << "- P99 Latency: " << snapshot.audio_latency.p99 << " ms\n\n";
    
    out << "### MIDI Performance\n";
    out << "- Mean Latency: " << snapshot.midi_latency.mean << " ms\n";
    out << "- Jitter: " << snapshot.midi_jitter << " ms\n\n";
    
    out << "### Thread Performance\n";
    out << "- Contentions: " << snapshot.thread_contention.total_contentions << "\n";
    out << "- Lock Waits: " << snapshot.thread_contention.total_lock_waits << "\n\n";
    
    out << "## Overall Result\n\n";
    out << (snapshot.meetsThresholds() 
        ? "### ✅ **PASS** - All metrics within thresholds\n" 
        : "### ❌ **FAIL** - Some metrics exceed thresholds\n");
}

void PerformanceReporter::generateHTMLReport(const PerformanceSnapshot& snapshot, std::ostream& out) {
    // Simplified HTML report - full implementation would include CSS and charts
    out << "<!DOCTYPE html>\n<html>\n<head>\n";
    out << "<title>HAM Performance Report</title>\n";
    out << "<style>\n";
    out << "body { font-family: Arial, sans-serif; margin: 20px; }\n";
    out << "table { border-collapse: collapse; width: 100%; }\n";
    out << "th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }\n";
    out << "th { background-color: #f2f2f2; }\n";
    out << ".pass { color: green; }\n";
    out << ".fail { color: red; }\n";
    out << "</style>\n</head>\n<body>\n";
    
    out << "<h1>HAM Performance Report</h1>\n";
    out << "<p>Generated: " << formatTime(snapshot.timestamp) << "</p>\n";
    
    out << "<h2>Summary</h2>\n";
    out << "<table>\n";
    out << "<tr><th>Metric</th><th>Value</th><th>Threshold</th><th>Status</th></tr>\n";
    
    // Add metric rows...
    out << "</table>\n";
    
    out << "</body>\n</html>\n";
}

void PerformanceReporter::generateCSVReport(const PerformanceSnapshot& snapshot, std::ostream& out) {
    out << "Metric,Value,Unit,Threshold,Status\n";
    out << "CPU Mean," << snapshot.cpu_usage.mean << ",%," << PerformanceThresholds::MAX_CPU_USAGE_PERCENT << ","
        << getStatusText(snapshot.cpu_usage.mean < PerformanceThresholds::MAX_CPU_USAGE_PERCENT) << "\n";
    out << "CPU Max," << snapshot.cpu_usage.max << ",%," << PerformanceThresholds::MAX_CPU_USAGE_PERCENT << ","
        << getStatusText(snapshot.cpu_usage.max < PerformanceThresholds::MAX_CPU_USAGE_PERCENT) << "\n";
    out << "Memory Peak," << (snapshot.memory.peak_bytes / 1024.0 / 1024.0) << ",MB," 
        << PerformanceThresholds::MAX_MEMORY_MB << ","
        << getStatusText((snapshot.memory.peak_bytes / 1024.0 / 1024.0) < PerformanceThresholds::MAX_MEMORY_MB) << "\n";
    out << "Audio Latency Max," << snapshot.audio_latency.max << ",ms," 
        << PerformanceThresholds::MAX_AUDIO_LATENCY_MS << ","
        << getStatusText(snapshot.audio_latency.max < PerformanceThresholds::MAX_AUDIO_LATENCY_MS) << "\n";
    out << "MIDI Jitter," << snapshot.midi_jitter << ",ms," 
        << PerformanceThresholds::MAX_MIDI_JITTER_MS << ","
        << getStatusText(snapshot.midi_jitter < PerformanceThresholds::MAX_MIDI_JITTER_MS) << "\n";
}

std::string PerformanceReporter::formatTime(std::chrono::system_clock::time_point time) const {
    auto time_t = std::chrono::system_clock::to_time_t(time);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

void PerformanceReporter::saveReport(const std::string& filename, const std::string& content) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file for writing: " + filename);
    }
    file << content;
}

bool PerformanceReporter::violatesThreshold(const std::string& metric_name, double value) const {
    if (metric_name == "cpu_usage") {
        return value > PerformanceThresholds::MAX_CPU_USAGE_PERCENT;
    } else if (metric_name == "memory_mb") {
        return value > PerformanceThresholds::MAX_MEMORY_MB;
    } else if (metric_name == "audio_latency_ms") {
        return value > PerformanceThresholds::MAX_AUDIO_LATENCY_MS;
    } else if (metric_name == "midi_jitter_ms") {
        return value > PerformanceThresholds::MAX_MIDI_JITTER_MS;
    }
    return false;
}

// CIReporter implementation

bool CIReporter::isRunningInCI() const {
    // Check common CI environment variables
    return std::getenv("CI") != nullptr ||
           std::getenv("GITHUB_ACTIONS") != nullptr ||
           std::getenv("GITLAB_CI") != nullptr ||
           std::getenv("JENKINS_HOME") != nullptr ||
           std::getenv("CIRCLECI") != nullptr ||
           std::getenv("TRAVIS") != nullptr;
}

CIReporter::CISystem CIReporter::detectCISystem() const {
    if (std::getenv("GITHUB_ACTIONS")) return CISystem::GitHub;
    if (std::getenv("GITLAB_CI")) return CISystem::GitLab;
    if (std::getenv("JENKINS_HOME")) return CISystem::Jenkins;
    if (std::getenv("CIRCLECI")) return CISystem::CircleCI;
    if (std::getenv("TRAVIS")) return CISystem::TravisCI;
    return CISystem::GitHub; // Default
}

void CIReporter::generateGitHubAnnotations(const std::vector<RegressionDetector::Regression>& regressions,
                                          std::ostream& out) {
    for (const auto& reg : regressions) {
        if (reg.is_critical) {
            out << "::error title=Performance Regression::" 
                << reg.benchmark_name << "/" << reg.metric_name 
                << " regressed by " << reg.regression_percent << "%\n";
        } else {
            out << "::warning title=Performance Regression::"
                << reg.benchmark_name << "/" << reg.metric_name 
                << " regressed by " << reg.regression_percent << "%\n";
        }
    }
}

std::string CIReporter::generateJUnitXML(const std::map<std::string, bool>& test_results) {
    std::stringstream xml;
    xml << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    xml << "<testsuites name=\"HAM Performance Tests\" tests=\"" << test_results.size() << "\">\n";
    xml << "  <testsuite name=\"Performance\">\n";
    
    for (const auto& [name, passed] : test_results) {
        xml << "    <testcase name=\"" << name << "\"";
        if (!passed) {
            xml << ">\n      <failure message=\"Performance threshold exceeded\"/>\n    </testcase>\n";
        } else {
            xml << "/>\n";
        }
    }
    
    xml << "  </testsuite>\n";
    xml << "</testsuites>\n";
    
    return xml.str();
}

} // namespace HAM::Performance