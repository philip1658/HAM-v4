#!/bin/bash

# HAM Daily Test Suite
# Quick validation for daily development

echo "========================================="
echo "CloneHAM Daily Test Suite"
echo "Date: $(date)"
echo "========================================="
echo ""

# Track results
all_pass=true

# Function to run test and check result
run_test() {
    local test_name=$1
    local test_command=$2
    
    echo -n "Running $test_name... "
    
    if eval $test_command > /dev/null 2>&1; then
        echo "✅ PASS"
        return 0
    else
        echo "❌ FAIL"
        all_pass=false
        return 1
    fi
}

# Change to CloneHAM directory
cd /Users/philipkrieger/Desktop/CloneHAM

echo "1. Build Check"
echo "--------------"
run_test "Build" "./build.sh"

if [ "$all_pass" = false ]; then
    echo ""
    echo "❌ Build failed. Stopping tests."
    exit 1
fi

echo ""
echo "2. Unit Tests"
echo "-------------"
if [ -f "./build/Tests/RunAllTests" ]; then
    run_test "Domain Models" "./build/Tests/DomainModelTests"
    run_test "Audio Engine" "./build/Tests/AudioEngineTests"
    run_test "MIDI Routing" "./build/Tests/MidiRoutingTests"
else
    echo "⚠️  Unit tests not built yet"
fi

echo ""
echo "3. Performance Check"
echo "--------------------"
if [ -f "./Tests/Scripts/test_performance.sh" ]; then
    run_test "Quick Performance" "cd Tests/Scripts && ./test_performance.sh --quick"
else
    echo "⚠️  Performance tests not available"
fi

echo ""
echo "4. Smoke Test"
echo "-------------"
echo -n "Checking if CloneHAM launches... "
if ./build/CloneHAM.app/Contents/MacOS/CloneHAM --version > /dev/null 2>&1; then
    echo "✅ PASS"
else
    echo "❌ FAIL"
    all_pass=false
fi

echo ""
echo "5. Code Quality"
echo "---------------"
echo -n "Checking for TODOs... "
todo_count=$(grep -r "TODO" Source/ --include="*.cpp" --include="*.h" 2>/dev/null | wc -l)
echo "Found $todo_count TODOs"

echo -n "Checking for FIXMEs... "
fixme_count=$(grep -r "FIXME" Source/ --include="*.cpp" --include="*.h" 2>/dev/null | wc -l)
if [ $fixme_count -gt 0 ]; then
    echo "⚠️  Found $fixme_count FIXMEs"
else
    echo "✅ None found"
fi

echo ""
echo "========================================="
echo "Daily Test Summary"
echo "========================================="
echo ""

if [ "$all_pass" = true ]; then
    echo "✅ All daily tests PASSED!"
    echo ""
    echo "Ready for development."
    
    # Update last successful test timestamp
    echo "$(date)" > .last_successful_daily_test
    
    exit 0
else
    echo "❌ Some tests FAILED!"
    echo ""
    echo "Please fix issues before continuing."
    exit 1
fi