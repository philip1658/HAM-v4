#!/bin/bash

# CloneHAM Build Script
# One-click build that keeps app inside clone folder (no Desktop copy)

set -e  # Exit on error

echo "========================================="
echo "CloneHAM Sequencer Build Script"
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
if [ ! -d "build" ]; then
    echo "Creating build directory..."
    mkdir build
fi

cd build

# Configure
echo "Configuring project..."
$CMAKE_CMD .. -DCMAKE_BUILD_TYPE=Release

echo ""
echo "Building CloneHAM..."
echo ""

# Build with all available cores
if [ "$(uname)" == "Darwin" ]; then
    CORES=$(sysctl -n hw.ncpu)
else
    CORES=$(nproc)
fi

echo "Building with $CORES cores..."
$CMAKE_CMD --build . --config Release -j$CORES

# Check if build succeeded
if [ $? -eq 0 ]; then
    echo ""
    echo -e "${GREEN}✅ Build successful!${NC}"
    
    APP_PATH="$(pwd)/HAM_artefacts/Release/CloneHAM.app"
    if [ -d "$APP_PATH" ]; then
        echo -e "${GREEN}✅ App available at:${NC} $APP_PATH"
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
echo "Build Complete"
echo "========================================="