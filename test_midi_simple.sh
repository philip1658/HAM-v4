#!/bin/bash

# Simple MIDI Testing Script for HAM Mono Mode
# Uses macOS built-in tools and receivemidi

echo "==================================="
echo "HAM Mono Mode MIDI Test (Simple)"  
echo "==================================="

# Check if receivemidi is available
if ! command -v ~/bin/receivemidi &> /dev/null; then
    echo "❌ receivemidi not found at ~/bin/receivemidi"
    echo "Installing receivemidi via Homebrew..."
    /opt/homebrew/bin/brew install gbevin/tools/receivemidi
    
    if ! command -v receivemidi &> /dev/null; then
        echo "❌ Failed to install receivemidi. Exiting."
        exit 1
    fi
fi

# List available MIDI devices
echo "📋 Available MIDI devices:"
~/bin/receivemidi list

echo ""
echo "🎹 Instructions:"
echo "1. Start HAM application"
echo "2. Ensure HAM is outputting to a virtual MIDI device (like IAC Driver)"
echo "3. Set HAM to mono mode (should be default)"
echo "4. Press PLAY in HAM"
echo "5. This script will capture MIDI for 10 seconds"
echo ""
echo "Enter MIDI device name (or press Enter for 'IAC Driver Bus 1'):"
read midi_device

if [ -z "$midi_device" ]; then
    midi_device="IAC Driver Bus 1"
fi

echo "🎧 Listening to: $midi_device"
echo "⏱️  Capturing for 10 seconds..."
echo "----------------------------------------"

# Create output file with timestamp
timestamp=$(date +"%Y%m%d_%H%M%S")
output_file="/Users/philipkrieger/Desktop/Clone_Ham/HAM/midi_capture_${timestamp}.txt"

# Start receivemidi with timeout and save output
timeout 10 ~/bin/receivemidi dev "$midi_device" | tee "$output_file" &
RECEIVEMIDI_PID=$!

# Show a countdown
for i in {10..1}; do
    echo -ne "\r⏱️  Time remaining: ${i}s "
    sleep 1
done

echo -e "\n✅ Capture complete!"

# Kill receivemidi if still running
kill $RECEIVEMIDI_PID 2>/dev/null || true

# Analyze the captured data
echo ""
echo "📊 Analysis Results:"
echo "=================="

if [ -f "$output_file" ]; then
    total_events=$(wc -l < "$output_file")
    note_ons=$(grep -c "note-on" "$output_file" || echo "0")
    note_offs=$(grep -c "note-off" "$output_file" || echo "0")
    
    echo "Total MIDI events: $total_events"
    echo "Note ON events: $note_ons"
    echo "Note OFF events: $note_offs"
    
    if [ "$total_events" -eq 0 ]; then
        echo ""
        echo "⚠️  No MIDI events captured!"
        echo "Troubleshooting:"
        echo "- Check that HAM is running and playing"
        echo "- Verify MIDI routing is correct"
        echo "- Try a different MIDI device"
        echo "- Check if HAM is actually outputting MIDI"
    else
        echo ""
        echo "✅ MIDI events captured successfully!"
        echo "📄 Full capture saved to: $output_file"
        
        # Show timing analysis if we have note events
        if [ "$note_ons" -gt 1 ]; then
            echo ""
            echo "⏱️  Timing Analysis:"
            echo "==================="
            
            # Extract timestamps and calculate intervals
            grep "note-on" "$output_file" | head -8 | while read line; do
                # This is a simplified analysis - receivemidi doesn't provide high-resolution timestamps
                echo "Note ON detected: $line"
            done
            
            echo ""
            echo "💡 For detailed timing analysis, use HAM_MIDI_Test (compiled version)"
        fi
    fi
    
    echo ""
    echo "📁 Files created:"
    echo "- MIDI capture: $output_file"
    echo "- Analysis table: HAM_MONO_MODE_MIDI_ANALYSIS.md"
else
    echo "❌ No output file created"
fi

echo ""
echo "🏁 Test completed!"
echo ""
echo "Next steps:"
echo "1. Compare captured events with expected table in HAM_MONO_MODE_MIDI_ANALYSIS.md"
echo "2. For precise timing analysis, build and run: ./build_midi_test.sh"
echo "3. Check mono mode behavior: no overlapping notes should occur"