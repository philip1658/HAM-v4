#!/bin/bash

# HAM MIDI Routing Test
# Verifies track separation and channel routing

echo "========================================="
echo "HAM MIDI Routing Test"
echo "========================================="
echo ""

# Check if HAM is built
if [ ! -f "../build/HAM.app/Contents/MacOS/HAM" ]; then
    echo "❌ ERROR: HAM.app not found. Run ./build.sh first"
    exit 1
fi

echo "Testing MIDI routing system..."
echo ""

echo "TEST 1: Track Buffer Separation"
echo "--------------------------------"
echo "Creating 4 tracks with different patterns..."
echo ""

# Launch HAM with multi-track test
../build/HAM.app/Contents/MacOS/HAM --test-midi-routing &
HAM_PID=$!

echo "HAM running with 4-track test pattern (PID: $HAM_PID)"
echo ""
echo "Track 1: C Major scale"
echo "Track 2: E Minor scale"
echo "Track 3: G Major arpeggio"
echo "Track 4: Drum pattern"
echo ""
echo "Open MIDI Monitor.app and observe:"
echo ""
echo "Expected on Channel 1:"
echo "  - All 4 tracks outputting"
echo "  - No data mixing between tracks"
echo "  - Each track maintains its pattern"
echo ""
read -p "Are tracks separated correctly? (y/n): " separation_result

if [ "$separation_result" = "y" ]; then
    echo "✅ PASS: Track separation working"
else
    echo "❌ FAIL: Tracks are mixing or interfering"
fi

echo ""
echo "TEST 2: Channel 1 Plugin Routing"
echo "---------------------------------"
echo "All plugins should receive on Channel 1..."
echo ""
echo "Check in MIDI Monitor:"
echo "  - ALL events on Channel 1"
echo "  - Channels 2-15 should be empty"
echo ""
read -p "Is everything on Channel 1? (y/n): " channel_result

if [ "$channel_result" = "y" ]; then
    echo "✅ PASS: Channel routing correct"
else
    echo "❌ FAIL: Wrong channel routing detected"
fi

echo ""
echo "TEST 3: Debug Monitor (Channel 16)"
echo "-----------------------------------"
echo "Debug duplicates should appear on Channel 16..."
echo ""
echo "Check in MIDI Monitor:"
echo "  - Channel 16 has duplicate of all events"
echo "  - Timing information included"
echo "  - Can compare with Channel 1"
echo ""
read -p "Is debug channel 16 working? (y/n): " debug_result

if [ "$debug_result" = "y" ]; then
    echo "✅ PASS: Debug monitor working"
else
    echo "❌ FAIL: Debug channel not working"
fi

echo ""
echo "TEST 4: Buffer Overflow Protection"
echo "-----------------------------------"
echo "Testing with rapid MIDI data..."
echo ""

# Send rapid MIDI data
kill $HAM_PID 2>/dev/null
../build/HAM.app/Contents/MacOS/HAM --test-midi-stress &
HAM_PID=$!

echo "Sending 1000 events/second for 10 seconds..."
sleep 10

echo ""
echo "Check for:"
echo "  - No dropped events"
echo "  - No buffer overflow messages"
echo "  - Timing remains accurate"
echo ""
read -p "Did stress test pass? (y/n): " stress_result

if [ "$stress_result" = "y" ]; then
    echo "✅ PASS: Buffer handling robust"
else
    echo "❌ FAIL: Buffer issues under load"
fi

# Cleanup
kill $HAM_PID 2>/dev/null

echo ""
echo "TEST 5: MIDI Timing Accuracy"
echo "-----------------------------"
echo "Analyzing timing between events..."
echo ""

# Run timing analysis
../build/HAM.app/Contents/MacOS/HAM --test-midi-timing > /tmp/midi_timing.log 2>&1 &
HAM_PID=$!

sleep 5
kill $HAM_PID 2>/dev/null

# Analyze timing log
if grep -q "ERROR" /tmp/midi_timing.log; then
    echo "❌ Timing errors detected:"
    grep "ERROR" /tmp/midi_timing.log
else
    echo "✅ PASS: MIDI timing accurate"
fi

echo ""
echo "========================================="
echo "MIDI Routing Test Summary"
echo "========================================="
echo ""

# Summary
all_pass=true
[ "$separation_result" != "y" ] && all_pass=false
[ "$channel_result" != "y" ] && all_pass=false
[ "$debug_result" != "y" ] && all_pass=false
[ "$stress_result" != "y" ] && all_pass=false

if [ "$all_pass" = true ]; then
    echo "🎉 ALL TESTS PASSED!"
    echo ""
    echo "MIDI routing is working correctly:"
    echo "  ✓ Track buffers separated"
    echo "  ✓ Channel 1 routing correct"
    echo "  ✓ Debug monitor functional"
    echo "  ✓ Buffer handling robust"
else
    echo "⚠️  SOME TESTS FAILED"
    echo ""
    echo "MIDI routing needs attention:"
    [ "$separation_result" != "y" ] && echo "  ✗ Fix track separation"
    [ "$channel_result" != "y" ] && echo "  ✗ Fix channel routing"
    [ "$debug_result" != "y" ] && echo "  ✗ Fix debug monitor"
    [ "$stress_result" != "y" ] && echo "  ✗ Fix buffer handling"
fi

echo ""
echo "========================================="