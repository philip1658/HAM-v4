# HAM Performance Testing Suite

Comprehensive performance regression testing system for the HAM (Happy Accident Machine) project.

## Overview

This performance testing suite ensures HAM meets its strict real-time performance requirements:
- **CPU Usage**: < 5%
- **MIDI Jitter**: < 0.1ms
- **Audio Latency**: < 5ms
- **Memory Usage**: < 128MB

## Components

### 1. Performance Benchmarks (`Benchmarks/`)
- **AudioProcessorBenchmark**: Tests the critical `processBlock()` function
- **MasterClockBenchmark**: Validates timing accuracy and jitter
- **MidiGenerationBenchmark**: Measures MIDI event generation performance
- **LockFreeQueueBenchmark**: Tests UI-to-audio thread communication
- **PatternProcessingBenchmark**: Benchmarks pattern and sequencing operations
- **VoiceManagerBenchmark**: Tests voice allocation and management

### 2. Integration Tests (`Tests/`)
- **PerformanceIntegrationTest**: Full system performance validation
- **CPUUsageTest**: Dedicated CPU usage testing under various loads
- **JitterTest**: MIDI timing jitter verification
- **MemoryUsageTest**: Memory allocation and leak detection
- **LatencyTest**: Audio processing latency measurement
- **ThreadContentionTest**: Multi-threading performance analysis

### 3. Core Infrastructure (`Core/`)
- **PerformanceMetrics**: Real-time performance monitoring
- **RegressionDetector**: Automatic regression detection (>10% degradation)
- **PerformanceReporter**: Report generation in multiple formats
- **BenchmarkHelpers**: Utilities for benchmark creation

## Usage

### Running Benchmarks

```bash
# Build the project with benchmarks enabled
cmake -B build -DHAM_BUILD_BENCHMARKS=ON
cmake --build build

# Run all benchmarks
./build/Tests/Performance/HAMPerformanceBenchmarks

# Run with JSON output for CI
./build/Tests/Performance/HAMPerformanceBenchmarks \
  --benchmark_out=results.json \
  --benchmark_out_format=json

# Compare against baseline
./build/Tests/Performance/HAMPerformanceBenchmarks \
  --benchmark_out=current.json \
  --benchmark_out_format=json

./build/Tests/Performance/HAMRegressionDetector \
  baseline.json current.json \
  --threshold 10
```

### Running Integration Tests

```bash
# Run all performance tests
./build/Tests/Performance/HAMPerformanceTests

# Generate performance report
./build/Tests/Performance/Scripts/generate_report.py results.json \
  --output performance_report.html
```

### CI/CD Integration

The suite includes GitHub Actions workflow for continuous monitoring:

```yaml
# .github/workflows/performance-tests.yml
- Runs on every push to main/develop
- Runs on pull requests
- Daily scheduled runs
- Automatic regression detection
- Performance dashboard generation
```

## Performance Thresholds

All tests validate against these thresholds:

| Metric | Threshold | Description |
|--------|-----------|-------------|
| CPU Usage | < 5% | Maximum CPU utilization |
| MIDI Jitter | < 0.1ms | Timing accuracy requirement |
| Audio Latency | < 5ms | Maximum processing delay |
| Memory | < 128MB | Peak memory usage |

## Regression Detection

The system automatically detects performance regressions:

1. **Baseline Creation**: First run creates baseline
2. **Continuous Comparison**: Each run compares against baseline
3. **Threshold Detection**: Fails if >10% degradation
4. **Trend Analysis**: Tracks performance over time

## Report Formats

Generated reports are available in multiple formats:
- **Text**: Console output
- **JSON**: Machine-readable format
- **HTML**: Interactive web dashboard
- **CSV**: Spreadsheet compatible
- **Markdown**: GitHub/GitLab compatible

## Development

### Adding New Benchmarks

```cpp
#include "../Core/BenchmarkHelpers.h"

static void BM_YourBenchmark(benchmark::State& state) {
    // Setup
    YourClass obj;
    
    // Benchmark loop
    for (auto _ : state) {
        obj.performOperation();
    }
    
    // Report custom metrics
    state.counters["metric_name"] = value;
}
HAM_BENCHMARK(BM_YourBenchmark);
```

### Adding New Tests

```cpp
class YourPerformanceTest : public juce::UnitTest {
public:
    YourPerformanceTest() : UnitTest("Your Test") {}
    
    void runTest() override {
        beginTest("Test Case");
        
        // Performance monitoring
        CPUMonitor cpu;
        cpu.startMeasurement();
        
        // Your test code
        
        cpu.endMeasurement();
        auto metrics = cpu.getMetrics();
        
        // Validate
        expect(metrics.max < threshold, "Performance exceeded");
    }
};
```

## Monitoring Tools Integration

The suite integrates with:
- **Instruments.app** (macOS)
- **Tracy Profiler**
- **Google Benchmark**
- **PVS-Studio**
- **Valgrind** (Linux)

## Results Interpretation

### Pass Criteria
✅ All metrics within thresholds
✅ No regressions > 10%
✅ Stable performance trends

### Fail Criteria
❌ Any metric exceeds threshold
❌ Regression > 10% detected
❌ Deteriorating trends

## License

Part of the HAM project - see main LICENSE file.