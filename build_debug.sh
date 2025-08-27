#!/bin/bash

# CloneHAM Debug Build Script
# Builds with debug symbols and timing analyzer enabled

set -e  # Exit on error

echo "========================================="
echo "CloneHAM Debug Build with Timing Analysis"
echo "========================================="
echo ""

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Determine CMake path (prefer homebrew version)
if [ -f "/opt/homebrew/bin/cmake" ]; then
    CMAKE_CMD="/opt/homebrew/bin/cmake"
elif [ -f "/usr/local/bin/cmake" ]; then
    CMAKE_CMD="/usr/local/bin/cmake"
elif command -v cmake &> /dev/null; then
    CMAKE_CMD="cmake"
else
    echo -e "${RED}❌ CMake is not installed${NC}"
    echo "Install with: brew install cmake"
    exit 1
fi

echo "Using CMake: $CMAKE_CMD"
$CMAKE_CMD --version
echo ""

# Create build directory if it doesn't exist
if [ ! -d "build_debug" ]; then
    echo "Creating build_debug directory..."
    mkdir build_debug
fi

cd build_debug

# Configure for Debug build
echo "Configuring project for DEBUG build..."
$CMAKE_CMD .. -DCMAKE_BUILD_TYPE=Debug

echo ""
echo "Building CloneHAM (Debug mode)..."
echo ""

# Build with all available cores
if [ "$(uname)" == "Darwin" ]; then
    CORES=$(sysctl -n hw.ncpu)
else
    CORES=$(nproc)
fi

echo "Building with $CORES cores..."
$CMAKE_CMD --build . --config Debug -j$CORES

# Check if build succeeded
if [ $? -eq 0 ]; then
    echo ""
    echo -e "${GREEN}✅ Debug build successful!${NC}"
    echo -e "${GREEN}✅ MIDI timing analyzer enabled${NC}"
    
    APP_PATH="$(pwd)/HAM_artefacts/Debug/CloneHAM.app"
    if [ -d "$APP_PATH" ]; then
        echo -e "${GREEN}✅ Debug app available at:${NC} $APP_PATH"
        echo ""
        echo -e "${YELLOW}To run with timing analysis:${NC}"
        echo "$APP_PATH/Contents/MacOS/CloneHAM"
        echo ""
        echo -e "${YELLOW}The app will print MIDI timing analysis every 4 seconds${NC}"
    else
        echo -e "${YELLOW}⚠️  App bundle not found at expected path:${NC} $APP_PATH"
        echo "Search with: find $(pwd) -name 'CloneHAM.app'"
    fi
else
    echo ""
    echo -e "${RED}❌ Build failed!${NC}"
    echo "Check the error messages above."
    exit 1
fi

echo ""
echo "========================================="
echo "Debug Build Complete"
echo "========================================="