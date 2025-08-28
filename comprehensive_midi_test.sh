#!/bin/bash

# HAM MIDI Routing Comprehensive Test Script
echo "========================================"
echo "HAM MIDI Routing Test - Starting"
echo "$(date)"
echo "========================================"

# Test configuration
TEST_DIR="test_results/midi_routing_test_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$TEST_DIR"

echo "Test results will be saved to: $TEST_DIR"

# Function to capture MIDI for a duration
capture_midi() {
    local test_name=$1
    local duration=$2
    local output_file="$TEST_DIR/${test_name}_midi_capture.txt"
    
    echo "📡 Capturing MIDI for $test_name (${duration}s)..."
    
    # Start MIDI capture in background
    timeout ${duration} ~/bin/receivemidi dev "IAC-Treiber Bus 1" ts > "$output_file" 2>&1 &
    local capture_pid=$!
    
    # Wait for capture to complete
    sleep $duration
    wait $capture_pid 2>/dev/null
    
    # Check if we captured any MIDI
    local midi_count=$(wc -l < "$output_file" 2>/dev/null || echo "0")
    echo "   → Captured $midi_count MIDI messages"
    
    return 0
}

# Test 1: Baseline - Check HAM is running
echo ""
echo "🔍 Test 1: Verify HAM Application Status"
echo "----------------------------------------"

if pgrep -f "CloneHAM" > /dev/null; then
    echo "✅ HAM is running"
    HAM_PID=$(pgrep -f "CloneHAM")
    echo "   HAM Process ID: $HAM_PID"
else
    echo "❌ HAM is not running"
    echo "   Please start HAM manually and run this test again"
    exit 1
fi

# Test 2: Verify MIDI devices are available
echo ""
echo "🎹 Test 2: MIDI Device Availability"
echo "-----------------------------------"

if ~/bin/sendmidi list | grep -q "IAC-Treiber Bus 1"; then
    echo "✅ IAC-Treiber Bus 1 is available"
else
    echo "❌ IAC-Treiber Bus 1 not found"
    echo "Available MIDI devices:"
    ~/bin/sendmidi list | sed 's/^/   /'
    exit 1
fi

# Test 3: Test MIDI monitoring setup
echo ""
echo "🔧 Test 3: MIDI Monitoring Setup"
echo "--------------------------------"

echo "Sending test MIDI note..."
~/bin/sendmidi dev "IAC-Treiber Bus 1" on 60 127
sleep 0.1
~/bin/sendmidi dev "IAC-Treiber Bus 1" off 60 0

# Capture the test note
capture_midi "monitoring_test" 2

if [ -s "$TEST_DIR/monitoring_test_midi_capture.txt" ]; then
    echo "✅ MIDI monitoring is working"
    echo "   Test message captured:"
    head -1 "$TEST_DIR/monitoring_test_midi_capture.txt" | sed 's/^/   /'
else
    echo "❌ MIDI monitoring failed"
    exit 1
fi

# Test 4: Default HAM behavior (should be Plugin Only - no external MIDI)
echo ""
echo "🎵 Test 4: Default MIDI Routing (Plugin Only Mode)"
echo "---------------------------------------"

echo "Testing HAM default state (should output NO external MIDI)..."
echo "Capturing MIDI for 5 seconds..."

# In Plugin Only mode, HAM should NOT send MIDI to external devices
capture_midi "default_plugin_only" 5

# Check if HAM generated any external MIDI (it shouldn't in Plugin Only mode)
if [ -s "$TEST_DIR/default_plugin_only_midi_capture.txt" ]; then
    midi_lines=$(wc -l < "$TEST_DIR/default_plugin_only_midi_capture.txt")
    echo "⚠️  Captured $midi_lines MIDI messages in Plugin Only mode"
    echo "   This might indicate the routing is not working as expected"
    echo "   First few messages:"
    head -3 "$TEST_DIR/default_plugin_only_midi_capture.txt" | sed 's/^/   /'
else
    echo "✅ No external MIDI in Plugin Only mode (expected behavior)"
fi

# Test 5: Manual interaction instructions
echo ""
echo "📋 Test 5: Manual UI Testing Instructions"
echo "----------------------------------------"

cat << EOF
To complete the MIDI routing test, please perform these manual steps:

1. 🖱️  FIND THE MIDI ROUTING DROPDOWN:
   - Look for the TrackSidebar (track controls on the side)
   - Find the dropdown labeled "MIDI Routing" below the channel selector
   - It should currently show "Plugin Only"

2. 🔄 TEST EXTERNAL ONLY MODE:
   - Change the dropdown to "External Only" 
   - Start HAM playback (press play button)
   - You should see MIDI messages appearing in the monitoring window
   - Expected: Note ON/OFF messages for C4 (note 60) at 120 BPM

3. 🔀 TEST BOTH MODE:
   - Change the dropdown to "Both"
   - Start playback again
   - Should see the same MIDI messages (goes to both plugins AND external)

4. 📊 EXPECTED MIDI PATTERN:
   Based on HAM_MONO_MODE_MIDI_ANALYSIS.md, you should see:
   - Channel 1 Note ON/OFF messages
   - Note 60 (C4) with velocity 100
   - Timing: Every 0.5 seconds (120 BPM, quarter notes)
   - Gate length: 50% (Note OFF after 0.25s)

EOF

# Test 6: Wait for user to test and capture results
echo ""
echo "⏳ Test 6: Interactive Testing Session"
echo "-------------------------------------"

echo "Starting 30-second MIDI capture for manual testing..."
echo "Please change HAM to 'External Only' mode and start playback now!"

capture_midi "manual_external_only" 30

# Analyze the results
echo ""
echo "📊 Test Results Analysis"
echo "------------------------"

for test_file in "$TEST_DIR"/*_midi_capture.txt; do
    if [ -s "$test_file" ]; then
        filename=$(basename "$test_file")
        test_name=${filename%_midi_capture.txt}
        line_count=$(wc -l < "$test_file")
        
        echo "📄 $test_name: $line_count MIDI messages"
        
        # Show first few lines if any
        if [ $line_count -gt 0 ]; then
            echo "   Sample messages:"
            head -3 "$test_file" | sed 's/^/   /'
            if [ $line_count -gt 3 ]; then
                echo "   ... (and $(($line_count - 3)) more)"
            fi
        fi
        echo
    fi
done

# Generate final report
echo "🎯 FINAL TEST REPORT"
echo "==================="
echo "Test completed at: $(date)"
echo "HAM Process ID: $HAM_PID"
echo "Results saved to: $TEST_DIR"
echo ""
echo "✅ Successfully tested:"
echo "   - HAM application startup"
echo "   - MIDI device availability"
echo "   - External MIDI monitoring setup"
echo "   - Default routing behavior"
echo ""
echo "📋 Manual testing required:"
echo "   - UI dropdown functionality"
echo "   - External Only mode MIDI output"
echo "   - Both mode MIDI output"
echo "   - Pattern timing verification"

echo ""
echo "========================================"
echo "HAM MIDI Routing Test - Complete"
echo "========================================"