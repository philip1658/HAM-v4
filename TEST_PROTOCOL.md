# HAM Test Protocol & Verification System

## 🔒 Test Gate System Overview

This document defines the testing and approval protocol for HAM development. **No feature is considered complete until Philip has manually tested and approved it.**

## 🚦 Approval Workflow

```
Developer → Test Evidence → Philip Tests → Approval/Rejection → Update Roadmap
   ↓            ↓              ↓                ↓                    ↓
Code Done   Screenshots    Manual Test    ✅ or ❌           Only if ✅
            Logs/Videos    Verification
```

## 📋 Test Evidence Requirements

### For Each Feature Completion:

1. **Code Completion Checklist**
   - [ ] Feature implemented
   - [ ] No compiler warnings
   - [ ] Memory leaks checked
   - [ ] Thread safety verified
   - [ ] Performance within budget

2. **Required Evidence by Feature Type**

| Feature Type | Required Evidence | Format |
|-------------|-------------------|---------|
| UI Component | Screenshot | PNG in `/Tests/Evidence/` |
| Audio Engine | Performance Log | TXT file with metrics |
| MIDI Feature | Monitor Output | Screenshot + timing log |
| Timing Critical | Profiler Results | Instruments screenshot |
| Domain Model | Unit Test Report | Console output capture |
| Pattern Morphing | Morph Demo Video | MP4 + parameter logs |
| Channel Assignment | MIDI Channel Map | JSON + visual diagram |
| Async Patterns | Scene Switch Demo | Video + timing analysis |
| CI/CD Pipeline | Build Logs | JSON + artifacts |
| Visual Regression | Diff Images | PNG before/after/diff |

3. **Evidence Location**
   ```
   HAM/Tests/Evidence/
   ├── Phase1/
   │   ├── 1.1_project_setup/
   │   ├── 1.2_domain_models/
   │   └── 1.3_master_clock/
   ├── Phase2/
   │   ├── 2.1_voice_manager/
   │   ├── 2.2_sequencer_engine/
   │   ├── 2.3_midi_router/
   │   └── 2.4_channel_manager/
   ├── Phase3/
   │   └── 3.4_pattern_morphing/
   ├── Phase6/
   │   ├── 6.0_pattern_morphing_system/
   │   ├── 6.4_scene_manager_ui/
   │   └── 6.5_multichannel_visualizer/
   └── Phase7/
       ├── 7.1_automated_testing/
       ├── 7.2_log_analysis/
       └── 7.3_visual_regression/
   ```

## 🧪 Test Categories

### 1. Unit Tests (Automated)
```bash
# Run all unit tests
./build/Tests/RunAllTests

# Run specific test suite
./build/Tests/DomainModelTests
./build/Tests/AudioEngineTests
./build/Tests/MidiRoutingTests
```

**Pass Criteria**: 100% pass rate, no memory leaks

### 2. Integration Tests (Semi-Automated)
```bash
# Test audio engine integration
./test_audio_integration.sh

# Test MIDI routing
./test_midi_integration.sh
```

**Pass Criteria**: All subsystems communicate correctly

### 3. Performance Tests (Automated Metrics)
```bash
# CPU usage test
./test_performance.sh --cpu

# Memory usage test
./test_performance.sh --memory

# Timing accuracy test
./test_performance.sh --timing
```

**Pass Criteria**:
- CPU < 5% @ 48kHz with 1 track
- Memory stable over 10 minutes
- MIDI jitter < 0.1ms

### 4. Manual UI Tests (Philip Only)

#### Checklist for UI Components:
- [ ] Visual appearance matches design
- [ ] All controls responsive
- [ ] Animations smooth (30+ FPS)
- [ ] Keyboard shortcuts work
- [ ] Mouse interactions correct
- [ ] Resize behavior proper
- [ ] Dark theme consistent

#### Checklist for Audio Features:
- [ ] No clicks or pops
- [ ] Timing accurate (by ear)
- [ ] All parameters affect sound
- [ ] Start/stop clean
- [ ] No dropouts at low buffer

## 🔍 Verification Methods by Phase

### Phase 1: Foundation Tests

#### 1.1 Project Setup
```bash
# Philip verifies:
cd /Users/philipkrieger/Desktop/HAM
./build.sh

# Expected:
# - Build completes in < 30 seconds
# - HAM.app appears on Desktop
# - App launches without crash
# - Window shows with dark theme
```

#### 1.2 Domain Models
```bash
# Philip verifies:
./build/Tests/DomainModelTests

# Expected output:
[==========] Running 25 tests
[----------] Track Model Tests
[ RUN      ] Track.CreateWithDefaults
[       OK ] Track.CreateWithDefaults (0 ms)
...
[==========] 25 tests PASSED
```

#### 1.3 Master Clock
```bash
# Philip verifies:
./test_timing.sh

# Expected output:
Testing Master Clock...
BPM: 120
PPQN: 24
Samples per tick: 2000
Jitter analysis over 1000 ticks:
  Average: 0.002ms
  Max: 0.08ms
  Status: PASS ✅
```

### Phase 2: Core Audio Tests

#### 2.1 Voice Manager (CRITICAL!)
```bash
# Philip verifies:
./test_voice_manager.sh

# Then manually:
1. Open HAM.app
2. Set Track 1 to MONO
3. Play C-E-G rapidly → Should cut notes
4. Set Track 1 to POLY  
5. Play C-E-G rapidly → Should overlap
6. Play 17+ notes → Oldest should disappear

# Monitor in MIDI Monitor.app on Channel 1
```

**Special Attention**: This is the most critical feature to get right early!

#### 2.2 Sequencer Engine
```bash
# Philip verifies:
1. Open HAM.app
2. Create 8-stage pattern with different pitches
3. Press Play
4. Record MIDI in Logic/Ableton for 2 loops

# Verify:
- 8 stages play in order
- Pattern loops seamlessly
- No timing drift over 2 minutes
```

#### 2.3 MIDI Router
```bash
# Philip verifies:
./test_midi_routing.sh

# In MIDI Monitor:
- Channel 1: Track outputs
- Channel 16: Debug duplicates
- No data on channels 2-15
- Each track separate
```

### Phase 3: Advanced Engine Tests

#### 3.1 Gate Engine
```bash
# Philip tests each gate type:
1. MULTIPLE: Each ratchet gets gate
2. HOLD: One long gate
3. SINGLE: Only first ratchet
4. REST: No output

# With Oscilloscope or DAW:
- Verify gate lengths
- Check ratchet timing
```

#### 3.2 Pitch Engine
```bash
# Philip verifies scales:
1. Set C Major scale
2. Play chromatic pattern
3. Only C-D-E-F-G-A-B should output

# Test octave offsets:
- Set +2 octaves
- Verify 24 semitones higher
```

#### 3.3 Accumulator
```bash
# Philip verifies:
1. Set accumulator +1
2. Stage plays C first time
3. Stage plays C# second time
4. Stage plays D third time
5. Test reset function
```

## 📊 Performance Benchmarks

### Minimum Requirements (M3 Max)

| Metric | Target | Fail Threshold |
|--------|--------|----------------|
| CPU Usage (1 track) | < 3% | > 5% |
| CPU Usage (8 tracks) | < 10% | > 20% |
| Memory Base | < 200MB | > 300MB |
| Memory with Plugins | < 500MB | > 1GB |
| MIDI Jitter | < 0.05ms | > 0.1ms |
| Audio Latency | < 5ms | > 10ms |
| UI Frame Rate | > 30 FPS | < 20 FPS |

### Test Commands
```bash
# CPU Profile
instruments -t "Time Profiler" -D cpu_profile.trace HAM.app

# Memory Profile  
instruments -t "Leaks" -D memory_profile.trace HAM.app

# Allocation Check
instruments -t "Allocations" -D allocations.trace HAM.app
```

## ✅ Approval Criteria

### For Philip to Approve ✅:

1. **Functional Requirements**
   - Feature works as specified
   - No crashes or hangs
   - Integrates with existing features

2. **Performance Requirements**
   - Meets CPU budget
   - No memory leaks
   - Timing accurate

3. **Quality Requirements**
   - Code is clean
   - No obvious bugs
   - UI looks professional

### Rejection Reasons ❌:

- **Critical**: Crashes, data loss, audio dropouts
- **Major**: Wrong behavior, poor performance
- **Minor**: Visual glitches, small timing issues

## 📝 Test Feedback Format

### Philip's Feedback Template:

```markdown
## Test Results for [TASK_ID]

**Status**: ✅ APPROVED / ❌ FAILED

### What Worked:
- Feature A works correctly
- Performance is good
- UI looks nice

### Issues Found:
- [ ] Issue 1: Description
- [ ] Issue 2: Description

### Required Fixes:
1. Fix Issue 1 by...
2. Fix Issue 2 by...

### Retest Required:
Yes/No

---
Philip - [DATE]
```

## 🔄 Continuous Testing

### Daily Test Routine:

```bash
#!/bin/bash
# daily_test.sh

echo "Running HAM Daily Tests..."

# 1. Build check
./build.sh || exit 1

# 2. Unit tests
./build/Tests/RunAllTests || exit 1

# 3. Performance check
./test_performance.sh --quick || exit 1

# 4. Smoke test
./HAM.app/Contents/MacOS/HAM --test-mode || exit 1

echo "✅ All daily tests passed!"
```

### Before Each Commit:

```bash
# pre-commit-test.sh
./test_performance.sh --quick
./build/Tests/RunAllTests
```

## 🚨 Emergency Procedures

### If Critical Bug Found:

1. **Stop all development**
2. **Document the bug** with:
   - Steps to reproduce
   - Screenshot/video
   - Log files
3. **Revert if necessary**
4. **Fix with highest priority**
5. **Full retest of affected systems**

### Performance Regression:

1. **Profile immediately**
2. **Git bisect to find cause**
3. **Revert or fix**
4. **Add performance test**

## 📁 Test Artifacts

### Keep These Files:

```
HAM/Tests/
├── Evidence/           # Screenshots, logs
├── Reports/           # Test reports
├── Benchmarks/        # Performance baselines
└── Scripts/           # Test scripts
```

### Test Script Library:

- `test_timing.sh` - Clock and timing accuracy
- `test_voice_manager.sh` - Voice allocation tests  
- `test_midi_routing.sh` - MIDI routing verification
- `test_channel_manager.sh` - Multi-channel assignment tests
- `test_pattern_morphing.sh` - Pattern interpolation validation
- `test_async_patterns.sh` - Scene switching verification
- `test_performance.sh` - CPU/Memory benchmarks
- `test_audio_integration.sh` - Full audio path
- `test_ui_responsiveness.sh` - UI performance
- `test_visual_regression.sh` - Screenshot comparison
- `test_ci_pipeline.sh` - CI/CD validation
- `daily_test.sh` - Quick daily validation

---

## 🎯 Golden Rule

**No feature is complete until Philip says: "Approved ✅"**

This ensures quality and prevents accumulation of technical debt. Every feature must meet our high standards before moving forward.

## 🤖 Automated CI/CD Testing

### GitHub Actions Pipeline

```yaml
# .github/workflows/ham-ci.yml
name: HAM CI/CD Pipeline

on:
  push:
    branches: [ main, develop ]
  pull_request:
    branches: [ main ]

jobs:
  build-and-test:
    runs-on: macos-latest
    steps:
      - uses: actions/checkout@v3
      
      - name: Setup Build Environment
        run: |
          brew install cmake
          xcode-select --install
      
      - name: Build HAM
        run: |
          ./build.sh
          
      - name: Run Unit Tests
        run: |
          ./build/Tests/RunAllTests
          
      - name: Performance Benchmarks
        run: |
          ./test_performance.sh --automated
          
      - name: Screenshot Tests
        run: |
          ./test_visual_regression.sh --baseline
          
      - name: Upload Test Artifacts
        uses: actions/upload-artifact@v3
        with:
          name: test-evidence
          path: Tests/Evidence/
```

### Screenshot Validation System

```cpp
// Tests/ScreenshotValidator.cpp
class ScreenshotValidator {
    struct ComparisonResult {
        float similarity;        // 0.0-1.0
        bool passed;            // >95% similarity
        Image differenceImage;  // Highlighted differences
        String failureReason;
    };
    
public:
    ComparisonResult compareScreenshots(const File& baseline, 
                                       const File& current) {
        Image baseImg = ImageFileFormat::loadFrom(baseline);
        Image currImg = ImageFileFormat::loadFrom(current);
        
        // Pixel-by-pixel comparison with tolerance
        return performComparison(baseImg, currImg);
    }
    
    void updateBaseline(const File& newBaseline, const String& testName) {
        // Update baseline after manual approval
        File baselineDir("/Tests/Baselines/");
        newBaseline.copyFileTo(baselineDir.getChildFile(testName + ".png"));
    }
};
```

### Log Parser & Analysis

```cpp
// Tests/LogAnalyzer.cpp
class LogAnalyzer {
    enum class Severity { Debug, Info, Warning, Error, Critical };
    
    struct LogPattern {
        String pattern;
        Severity severity;
        String description;
        bool isRegression;  // Performance regression indicator
    };
    
    std::vector<LogPattern> m_knownPatterns = {
        {"CPU usage: (\\d+)%", Severity::Warning, "High CPU usage", true},
        {"MIDI jitter: ([0-9.]+)ms", Severity::Error, "Timing issues", true},
        {"Memory leak detected", Severity::Critical, "Memory leak", false},
        {"Plugin crashed", Severity::Critical, "Plugin instability", false}
    };
    
public:
    AnalysisResult analyzeLogs(const File& logFile) {
        // Parse log file and detect patterns
        // Flag performance regressions
        // Generate actionable reports
    }
};
```

### Automated Performance Monitoring

```bash
#!/bin/bash
# test_performance_automated.sh

echo "Starting automated performance tests..."

# CPU Usage Test
echo "Testing CPU usage..."
./HAM.app/Contents/MacOS/HAM --test-mode --duration=60 > cpu_test.log 2>&1 &
HAM_PID=$!

# Monitor CPU usage
for i in {1..60}; do
    CPU_USAGE=$(ps -p $HAM_PID -o %cpu | tail -1)
    echo "Sample $i: CPU $CPU_USAGE%" >> cpu_results.log
    sleep 1
done

kill $HAM_PID

# Analyze results
AVG_CPU=$(awk '{sum+=$3} END {print sum/NR}' cpu_results.log)
if (( $(echo "$AVG_CPU > 5.0" | bc -l) )); then
    echo "FAIL: CPU usage $AVG_CPU% exceeds 5% threshold"
    exit 1
else
    echo "PASS: CPU usage $AVG_CPU% within acceptable range"
fi

# Memory Test
echo "Testing memory usage..."
# Similar monitoring for memory leaks

# MIDI Timing Test
echo "Testing MIDI timing accuracy..."
# Automated timing analysis

echo "All performance tests completed."
```

### Automated + Manual Approval Process

1. **Automated CI/CD** runs all tests and generates evidence
2. **Philip reviews** automated results and test evidence
3. **Manual testing** for user experience and edge cases
4. **Final approval** only after both automated and manual validation pass

This hybrid approach ensures both technical correctness and user experience quality.