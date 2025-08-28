#!/bin/bash

# Complete HAM MIDI Test - Auto-run and capture
echo "========================================"
echo "HAM Mono Mode Complete MIDI Test"
echo "========================================"

cd /Users/philipkrieger/Desktop/Clone_Ham/HAM

# Check if HAM exists
if [ ! -f "build-release/HAM_artefacts/Release/CloneHAM.app/Contents/MacOS/CloneHAM" ]; then
    echo "❌ HAM not found! Building..."
    if [ ! -d "build-release" ]; then
        mkdir build-release && cd build-release
        /opt/homebrew/bin/cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release
        cd ..
    fi
    cd build-release
    /opt/homebrew/bin/ninja -j8
    cd ..
    
    if [ ! -f "build-release/HAM_artefacts/Release/CloneHAM.app/Contents/MacOS/CloneHAM" ]; then
        echo "❌ Build failed!"
        exit 1
    fi
fi

echo "✅ HAM application ready"

# Create test results directory
mkdir -p test_results
timestamp=$(date +"%Y%m%d_%H%M%S")
result_dir="test_results/midi_test_$timestamp"
mkdir -p "$result_dir"

echo "📁 Test results will be saved to: $result_dir"

# Check MIDI devices
echo ""
echo "🎹 Available MIDI devices:"
~/bin/receivemidi list

# Start MIDI capture in background
echo ""
echo "🎧 Starting MIDI capture on IAC-Treiber Bus 1..."
~/bin/receivemidi dev "IAC-Treiber Bus 1" > "$result_dir/midi_raw_capture.txt" &
RECEIVEMIDI_PID=$!
echo "MIDI capture started with PID: $RECEIVEMIDI_PID"

# Give capture a moment to start
sleep 1

# Start HAM application
echo ""
echo "🚀 Starting HAM application..."
echo "HAM will auto-play for 10 seconds to capture a full mono mode test"

# Start HAM in background
./build-release/HAM_artefacts/Release/CloneHAM.app/Contents/MacOS/CloneHAM &
HAM_PID=$!
echo "HAM started with PID: $HAM_PID"

# Wait a moment for HAM to initialize
sleep 3

echo ""
echo "⏰ Capturing MIDI for 10 seconds..."
echo "Expected: 2.5 full pattern loops (4s each loop)"

# Wait for 10 seconds to capture data
for i in {10..1}; do
    printf "\r⏱️  Remaining: %2ds" $i
    sleep 1
done
echo -e "\n"

# Stop MIDI capture
echo "🛑 Stopping MIDI capture..."
kill $RECEIVEMIDI_PID 2>/dev/null

# Stop HAM
echo "🛑 Stopping HAM..."
kill $HAM_PID 2>/dev/null

# Wait for processes to close cleanly
sleep 2

# Analyze results
echo ""
echo "📊 Analyzing captured MIDI data..."

if [ -f "$result_dir/midi_raw_capture.txt" ]; then
    total_lines=$(wc -l < "$result_dir/midi_raw_capture.txt")
    
    if [ "$total_lines" -eq 0 ]; then
        echo "❌ No MIDI data captured!"
        echo ""
        echo "Troubleshooting steps:"
        echo "1. Check HAM MIDI output settings"
        echo "2. Verify IAC Driver is enabled in Audio MIDI Setup"
        echo "3. Check that HAM is actually playing"
        echo "4. Try manual test with: ~/bin/sendmidi dev \"IAC-Treiber Bus 1\" on 60 100"
    else
        echo "✅ Captured $total_lines MIDI events"
        
        # Extract note events
        note_ons=$(grep -c "note-on" "$result_dir/midi_raw_capture.txt" 2>/dev/null || echo "0")
        note_offs=$(grep -c "note-off" "$result_dir/midi_raw_capture.txt" 2>/dev/null || echo "0")
        
        echo "   📈 Note ON events: $note_ons"
        echo "   📉 Note OFF events: $note_offs"
        
        # Expected numbers for 10 seconds @ 120 BPM, 8 stages per 4s loop
        # = 2.5 loops = 20 stages = 20 note on/off pairs
        expected_notes=20
        
        echo "   🎯 Expected notes: ~$expected_notes (2.5 loops × 8 stages)"
        
        if [ "$note_ons" -gt 15 ] && [ "$note_ons" -lt 25 ]; then
            echo "   ✅ Note count within expected range"
        else
            echo "   ⚠️  Note count outside expected range"
        fi
        
        # Create analysis report
        cat > "$result_dir/analysis_report.txt" << EOF
HAM Mono Mode MIDI Test Analysis
===============================
Test Date: $(date)
Test Duration: 10 seconds
Expected Pattern: 8 stages, 4s loop, mono mode

Raw Data:
- Total MIDI events: $total_lines
- Note ON events: $note_ons  
- Note OFF events: $note_offs

Expected vs Actual:
- Expected notes: ~20 (2.5 loops × 8 stages)
- Actual notes: $note_ons
- Variance: $(($note_ons - 20))

Mono Mode Verification:
To verify mono mode behavior, check that:
1. Each Note ON is immediately preceded by Note OFF
2. No overlapping notes (Note ON before previous Note OFF)
3. Consistent timing intervals (~0.5s between stages)
4. All notes on same channel (channel 1)
5. Same note number (60 = C4) for all events

Raw MIDI data saved in: midi_raw_capture.txt
EOF
        
        echo "   📄 Analysis report: $result_dir/analysis_report.txt"
        
        # Show first few MIDI events for quick inspection
        echo ""
        echo "📋 First 10 MIDI events:"
        head -10 "$result_dir/midi_raw_capture.txt" | while IFS= read -r line; do
            echo "   $line"
        done
        
        if [ "$total_lines" -gt 10 ]; then
            echo "   ... ($((total_lines - 10)) more events)"
        fi
    fi
else
    echo "❌ MIDI capture file not created!"
fi

# Compare with expected pattern
echo ""
echo "📚 Reference Files Created:"
echo "   📊 Expected pattern: HAM_MONO_MODE_MIDI_ANALYSIS.md"
echo "   📁 Test results: $result_dir/"
echo "   🔧 Test tools: test_midi_simple.sh, build_midi_test.sh"

echo ""
echo "🏁 Test Complete!"
echo ""
echo "Next Steps:"
echo "1. Review captured MIDI in: $result_dir/midi_raw_capture.txt"
echo "2. Compare with expected timing in: HAM_MONO_MODE_MIDI_ANALYSIS.md"
echo "3. Verify mono mode behavior (no overlapping notes)"
echo "4. Check timing accuracy (0.5s intervals @ 120 BPM)"
echo ""
echo "For detailed analysis, examine the raw capture file and look for:"
echo "- Consistent 0.5s intervals between note-on events"
echo "- 0.25s gate length (note-off 0.25s after note-on)"  
echo "- No overlapping notes (true mono behavior)"
echo "- All events on channel 1"
echo "- Note number 60 (C4)"