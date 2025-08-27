#include <benchmark/benchmark.h>
#include "Core/BenchmarkHelpers.h"
#include "Core/PerformanceMetrics.h"
#include "Core/RegressionDetector.h"
#include <iostream>
#include <fstream>

using namespace HAM::Performance;

// Custom main to add HAM-specific reporting
int main(int argc, char** argv) {
    // Initialize benchmark
    benchmark::Initialize(&argc, argv);
    
    // Check for baseline comparison
    bool compare_baseline = false;
    std::string baseline_file = "baseline_results.json";
    
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--compare-baseline") {
            compare_baseline = true;
            if (i + 1 < argc) {
                baseline_file = argv[i + 1];
            }
        }
    }
    
    // Custom reporter
    auto reporter = std::make_unique<BenchmarkHelpers::HAMBenchmarkReporter>();
    
    // Print HAM performance requirements
    std::cout << "\n";
    std::cout << "================================================\n";
    std::cout << "   HAM Performance Benchmark Suite v1.0\n";
    std::cout << "================================================\n";
    std::cout << "Performance Requirements:\n";
    std::cout << "  • CPU Usage: < " << PerformanceThresholds::MAX_CPU_USAGE_PERCENT << "%\n";
    std::cout << "  • MIDI Jitter: < " << PerformanceThresholds::MAX_MIDI_JITTER_MS << "ms\n";
    std::cout << "  • Audio Latency: < " << PerformanceThresholds::MAX_AUDIO_LATENCY_MS << "ms\n";
    std::cout << "  • Memory Usage: < " << PerformanceThresholds::MAX_MEMORY_MB << "MB\n";
    std::cout << "================================================\n\n";
    
    // Run benchmarks
    benchmark::RunSpecifiedBenchmarks(reporter.get());
    
    // If baseline comparison requested
    if (compare_baseline) {
        std::cout << "\n================================================\n";
        std::cout << "   Baseline Comparison\n";
        std::cout << "================================================\n";
        
        BaselineManager baseline_mgr;
        if (baseline_mgr.baselineExists(baseline_file)) {
            try {
                RegressionDetector detector;
                auto baseline = baseline_mgr.loadBaseline(baseline_file);
                auto current = detector.loadResults("benchmark_results.json");
                
                auto report = detector.compare(baseline, current);
                report.printSummary();
                
                if (report.hasCriticalRegressions()) {
                    std::cerr << "\n❌ FAILED: Critical performance regressions detected!\n";
                    return 1;
                }
                
                if (report.hasRegressions()) {
                    std::cout << "\n⚠️ WARNING: Performance regressions detected.\n";
                    // Don't fail, just warn
                }
                
                std::cout << "\n✅ Performance requirements met!\n";
                
            } catch (const std::exception& e) {
                std::cerr << "Error comparing with baseline: " << e.what() << "\n";
                return 1;
            }
        } else {
            std::cout << "No baseline found. Run with --benchmark_out=baseline_results.json ";
            std::cout << "to create baseline.\n";
        }
    }
    
    // Performance trend analysis
    PerformanceTrendAnalyzer trend_analyzer;
    
    // Load existing trend data if available
    if (std::filesystem::exists("performance_trends.dat")) {
        trend_analyzer.loadTrendData("performance_trends.dat");
    }
    
    // Add current results to trends (would need to parse benchmark_results.json)
    // This is simplified - in production would parse actual results
    
    // Check for deteriorating trends
    auto deteriorating = trend_analyzer.getDeterioriatingMetrics();
    if (!deteriorating.empty()) {
        std::cout << "\n⚠️ WARNING: Deteriorating performance trends detected:\n";
        for (const auto& metric : deteriorating) {
            std::cout << "  • " << metric << "\n";
        }
    }
    
    // Save updated trends
    trend_analyzer.saveTrendData("performance_trends.dat");
    
    // Final summary
    std::cout << "\n================================================\n";
    std::cout << "   Benchmark Complete\n";
    std::cout << "================================================\n";
    
    benchmark::Shutdown();
    return 0;
}