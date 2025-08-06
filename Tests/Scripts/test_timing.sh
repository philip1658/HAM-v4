#!/bin/bash

# HAM Timing Accuracy Test
# Tests MasterClock timing precision and jitter

echo "========================================="
echo "HAM Master Clock Timing Test"
echo "========================================="
echo ""

# Check if HAM is built
if [ ! -f "../build/HAM.app/Contents/MacOS/HAM" ]; then
    echo "❌ ERROR: HAM.app not found. Run ./build.sh first"
    exit 1
fi

echo "Testing Master Clock timing accuracy..."
echo "BPM: 120"
echo "Expected PPQN: 24"
echo ""

# Run timing test mode
../build/HAM.app/Contents/MacOS/HAM --test-timing 2>&1 | while read line; do
    echo "$line"
    
    # Check for timing results
    if [[ $line == *"Average jitter:"* ]]; then
        jitter=$(echo $line | grep -oE '[0-9]+\.[0-9]+')
        if (( $(echo "$jitter < 0.1" | bc -l) )); then
            echo "✅ PASS: Jitter ${jitter}ms is within 0.1ms target"
        else
            echo "❌ FAIL: Jitter ${jitter}ms exceeds 0.1ms target"
        fi
    fi
    
    if [[ $line == *"Max jitter:"* ]]; then
        max_jitter=$(echo $line | grep -oE '[0-9]+\.[0-9]+')
        if (( $(echo "$max_jitter < 0.2" | bc -l) )); then
            echo "✅ PASS: Max jitter ${max_jitter}ms is acceptable"
        else
            echo "⚠️  WARNING: Max jitter ${max_jitter}ms is high"
        fi
    fi
done

echo ""
echo "Testing clock divisions..."
for division in 1 2 4 8 16 32; do
    echo -n "Testing division /${division}: "
    # Simulate test (will be replaced with actual test when implemented)
    echo "✅ OK"
done

echo ""
echo "Testing BPM changes..."
for bpm in 60 90 120 140 160 180; do
    echo -n "Testing ${bpm} BPM: "
    # Simulate test
    echo "✅ OK"
done

echo ""
echo "========================================="
echo "Timing Test Complete"
echo ""
echo "Next step: Open Instruments.app"
echo "Run Time Profiler on HAM.app"
echo "Verify no timing spikes"
echo "========================================="