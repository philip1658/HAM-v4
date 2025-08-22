#!/bin/bash

# HAM Voice Manager Test
# Critical test for Mono/Poly voice modes

echo "========================================="
echo "HAM Voice Manager Test (CRITICAL!)"
echo "========================================="
echo ""
echo "This tests the most critical feature that's"
echo "hard to retrofit later. Pay attention!"
echo ""

# Check if CloneHAM is built
if [ ! -f "../build/CloneHAM.app/Contents/MacOS/CloneHAM" ]; then
    echo "❌ ERROR: CloneHAM.app not found. Run ./build.sh first"
    exit 1
fi

echo "Starting Voice Manager tests..."
echo ""

echo "TEST 1: Mono Mode Voice Cutting"
echo "---------------------------------"
echo "1. Set Track 1 to MONO mode"
echo "2. Play notes C-E-G in rapid succession"
echo "3. Each new note should immediately cut the previous"
echo ""
echo "Expected in MIDI Monitor:"
echo "  Note On C  → Note Off C → Note On E → Note Off E → Note On G"
echo ""
read -p "Press ENTER when ready to test MONO mode..."

# Launch CloneHAM in test mode
../build/CloneHAM.app/Contents/MacOS/CloneHAM --test-voice-mono &
HAM_PID=$!

echo "CloneHAM running in MONO test mode (PID: $HAM_PID)"
echo ""
echo "✓ Play test pattern now"
echo "✓ Check MIDI Monitor on Channel 1"
echo ""
read -p "Did notes cut correctly? (y/n): " mono_result

if [ "$mono_result" = "y" ]; then
    echo "✅ PASS: Mono mode working correctly"
else
    echo "❌ FAIL: Mono mode not cutting notes properly"
fi

echo ""
echo "TEST 2: Poly Mode Voice Overlap"
echo "---------------------------------"
echo "1. Set Track 1 to POLY mode"
echo "2. Play notes C-E-G in rapid succession"
echo "3. All notes should overlap and sustain"
echo ""
echo "Expected in MIDI Monitor:"
echo "  Note On C → Note On E → Note On G (all sustaining)"
echo ""
read -p "Press ENTER when ready to test POLY mode..."

# Kill previous instance
kill $HAM_PID 2>/dev/null

# Launch CloneHAM in poly test mode
../build/CloneHAM.app/Contents/MacOS/CloneHAM --test-voice-poly &
HAM_PID=$!

echo "CloneHAM running in POLY test mode (PID: $HAM_PID)"
echo ""
echo "✓ Play test pattern now"
echo "✓ Check MIDI Monitor on Channel 1"
echo ""
read -p "Did notes overlap correctly? (y/n): " poly_result

if [ "$poly_result" = "y" ]; then
    echo "✅ PASS: Poly mode working correctly"
else
    echo "❌ FAIL: Poly mode not overlapping properly"
fi

echo ""
echo "TEST 3: Voice Stealing (16 Voice Limit)"
echo "----------------------------------------"
echo "1. Stay in POLY mode"
echo "2. Play more than 16 notes without releasing"
echo "3. Oldest notes should disappear (steal)"
echo ""
echo "Expected: When playing 17th note, 1st note gets Note Off"
echo ""
read -p "Press ENTER when ready to test voice stealing..."

echo ""
echo "✓ Play 17+ notes now (chromatic run works well)"
echo "✓ Watch for oldest notes disappearing in MIDI Monitor"
echo ""
read -p "Did voice stealing work (oldest first)? (y/n): " steal_result

if [ "$steal_result" = "y" ]; then
    echo "✅ PASS: Voice stealing working correctly"
else
    echo "❌ FAIL: Voice stealing not working properly"
fi

# Cleanup
kill $HAM_PID 2>/dev/null

echo ""
echo "TEST 4: Performance Check"
echo "-------------------------"
echo "Checking for allocations in audio thread..."
echo ""

# Run with allocation checking
echo "Running allocation check (30 seconds)..."
instruments -t Allocations -D /tmp/cloneham_voice_alloc.trace \
    ../build/CloneHAM.app/Contents/MacOS/CloneHAM --test-voice-performance &
INST_PID=$!

sleep 30
kill $INST_PID 2>/dev/null

echo ""
echo "Check /tmp/ham_voice_alloc.trace in Instruments"
echo "There should be NO allocations in audio thread"
echo ""

echo "========================================="
echo "Voice Manager Test Summary"
echo "========================================="
echo ""

# Summary
if [ "$mono_result" = "y" ] && [ "$poly_result" = "y" ] && [ "$steal_result" = "y" ]; then
    echo "🎉 ALL TESTS PASSED!"
    echo ""
    echo "Voice Manager is working correctly."
    echo "This critical feature is properly implemented."
else
    echo "⚠️  SOME TESTS FAILED"
    echo ""
    echo "Voice Manager needs fixes before proceeding."
    echo "This is CRITICAL to get right early!"
fi

echo ""
echo "========================================="