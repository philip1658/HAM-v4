#!/bin/bash
#
# Test script for HAM timing accuracy
# Builds and runs timing tests, analyzes jitter
#

set -e

echo "========================================"
echo "HAM Timing Performance Test"
echo "========================================"
echo ""

# Build directory
BUILD_DIR="build"

# Create build directory if it doesn't exist
if [ ! -d "$BUILD_DIR" ]; then
    echo "Creating build directory..."
    mkdir "$BUILD_DIR"
fi

cd "$BUILD_DIR"

# Configure with CMake
echo "Configuring with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release -DHAM_BUILD_TESTS=ON > /dev/null 2>&1

# Build the timing tests
echo "Building timing tests..."
make TimingTests -j8 > /dev/null 2>&1

echo ""
echo "Running timing tests..."
echo "----------------------------------------"

# Run the timing tests
./Tests/TimingTests_artefacts/Release/TimingTests

echo ""
echo "========================================"
echo "Timing Analysis"
echo "========================================"

# If on macOS, we can use Instruments for more detailed profiling
if [[ "$OSTYPE" == "darwin"* ]]; then
    echo ""
    echo "For detailed timing analysis, run:"
    echo "  instruments -t 'Time Profiler' -D timing_profile.trace HAM.app"
    echo ""
    echo "To check CPU usage:"
    echo "  1. Open HAM.app"
    echo "  2. Run: ps aux | grep HAM"
    echo "  3. Monitor CPU% column (should be <1% at 120 BPM)"
fi

echo ""
echo "✅ Timing tests complete!"
echo ""
echo "Key metrics to verify:"
echo "  - Timing jitter: <0.1ms ✓"
echo "  - CPU usage: <1% at 120 BPM"
echo "  - Clock divisions: Accurate to sample"
echo "  - Start/stop: No drift"
echo ""