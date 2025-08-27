#include "../Core/RegressionDetector.h"
#include <iostream>
#include <string>

using namespace HAM::Performance;

void printUsage(const char* program) {
    std::cout << "Usage: " << program << " <baseline.json> <current.json> [options]\n";
    std::cout << "\nOptions:\n";
    std::cout << "  --threshold <percent>  Set regression threshold (default: 10%)\n";
    std::cout << "  --save-baseline        Save current results as new baseline\n";
    std::cout << "  --verbose             Show detailed comparison\n";
    std::cout << "  --help                Show this help message\n";
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printUsage(argv[0]);
        return 1;
    }
    
    std::string baseline_file = argv[1];
    std::string current_file = argv[2];
    
    // Parse options
    double threshold = 10.0;
    bool save_baseline = false;
    bool verbose = false;
    
    for (int i = 3; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--threshold" && i + 1 < argc) {
            threshold = std::stod(argv[++i]);
        } else if (arg == "--save-baseline") {
            save_baseline = true;
        } else if (arg == "--verbose") {
            verbose = true;
        } else if (arg == "--help") {
            printUsage(argv[0]);
            return 0;
        }
    }
    
    try {
        RegressionDetector detector;
        detector.setRegressionThreshold(threshold);
        
        // Load results
        std::cout << "Loading baseline: " << baseline_file << "\n";
        auto baseline = detector.loadResults(baseline_file);
        
        std::cout << "Loading current results: " << current_file << "\n";
        auto current = detector.loadResults(current_file);
        
        // Compare
        std::cout << "\nComparing performance...\n";
        auto report = detector.compare(baseline, current);
        
        // Print summary
        report.printSummary();
        
        // Verbose output
        if (verbose) {
            std::cout << "\n=== Detailed Comparison ===" << std::endl;
            
            for (const auto& [name, result] : current) {
                auto baseline_it = baseline.find(name);
                if (baseline_it != baseline.end()) {
                    double change = detector.calculateRegressionPercent(
                        baseline_it->second.real_time_ns, 
                        result.real_time_ns
                    );
                    
                    std::cout << name << ":\n";
                    std::cout << "  Baseline: " << baseline_it->second.real_time_ns / 1e6 << " ms\n";
                    std::cout << "  Current:  " << result.real_time_ns / 1e6 << " ms\n";
                    std::cout << "  Change:   " << (change > 0 ? "+" : "") << change << "%\n\n";
                }
            }
        }
        
        // Save baseline if requested
        if (save_baseline) {
            BaselineManager manager;
            std::cout << "\nSaving current results as new baseline...\n";
            manager.saveBaseline(current, baseline_file);
            std::cout << "✅ Baseline updated: " << baseline_file << "\n";
        }
        
        // Exit code based on regressions
        if (report.hasCriticalRegressions()) {
            std::cout << "\n❌ FAILED: Critical performance regressions detected!\n";
            return 2;
        } else if (report.hasRegressions()) {
            std::cout << "\n⚠️ WARNING: Performance regressions detected.\n";
            return 1;
        } else {
            std::cout << "\n✅ SUCCESS: No performance regressions detected!\n";
            return 0;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 3;
    }
}