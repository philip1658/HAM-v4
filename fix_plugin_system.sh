#!/bin/bash

# Fix plugin system for Clone_Ham/HAM project
# This script will fix all identified issues

echo "=== HAM Plugin System Fix Script ==="
echo "Starting comprehensive fix..."

cd /Users/philipkrieger/Desktop/Clone_Ham/HAM

# Step 1: Clean build directory
echo "1. Cleaning build directory..."
rm -rf build
mkdir build
cd build

# Step 2: Build with CMake
echo "2. Building project with plugin support..."
/opt/homebrew/bin/cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_MAKE_PROGRAM=/opt/homebrew/bin/ninja
/opt/homebrew/bin/ninja -j8

# Step 3: Check if plugin tools were built
echo "3. Checking plugin tool builds..."
if [ -f "PluginScanWorker_artefacts/Release/PluginScanWorker" ]; then
    echo "✅ PluginScanWorker built successfully"
else
    echo "❌ PluginScanWorker not built"
fi

if [ -f "PluginHostBridge_artefacts/Release/PluginHostBridge.app/Contents/MacOS/PluginHostBridge" ]; then
    echo "✅ PluginHostBridge built successfully"
else
    echo "❌ PluginHostBridge not built"
fi

# Step 4: Create application data directory
echo "4. Creating application data directory..."
mkdir -p ~/Library/Application\ Support/HAM
mkdir -p ~/Library/Application\ Support/CloneHAM

# Step 5: Test plugin scanner
echo "5. Testing plugin scanner..."
if [ -f "PluginScanWorker_artefacts/Release/PluginScanWorker" ]; then
    echo "Running plugin scanner..."
    timeout 10 ./PluginScanWorker_artefacts/Release/PluginScanWorker 2>&1 | head -5
    
    # Check if plugin list was created
    if [ -f ~/Library/Application\ Support/CloneHAM/Plugins.xml ]; then
        echo "✅ Plugin list created successfully"
        echo "Found plugins:"
        grep "<PLUGIN " ~/Library/Application\ Support/CloneHAM/Plugins.xml | head -5
    else
        echo "⚠️ No plugin list created"
    fi
fi

# Step 6: Test the main app
echo "6. Testing main application..."
if [ -f "HAM_artefacts/Release/CloneHAM.app/Contents/MacOS/CloneHAM" ]; then
    echo "✅ Main app built successfully"
    echo "App location: $(pwd)/HAM_artefacts/Release/CloneHAM.app"
else
    echo "❌ Main app not built"
fi

echo ""
echo "=== Fix Complete ==="
echo "To run the app: open $(pwd)/HAM_artefacts/Release/CloneHAM.app"
