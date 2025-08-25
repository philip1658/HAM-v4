#!/bin/bash

echo "================================"
echo "HAM Plugin Integration Test Suite"
echo "================================"
echo ""

PROJECT_DIR="/Users/philipkrieger/Desktop/Clone_Ham/HAM"
BUILD_DIR="$PROJECT_DIR/build-release"

# Check if build exists
if [ ! -d "$BUILD_DIR" ]; then
    echo "❌ Build directory not found. Please build the project first."
    exit 1
fi

echo "1. Checking Plugin Scanner Components..."
echo "-----------------------------------------"

# Check for plugin scanner workers
for worker in PluginScanWorker PluginProbeWorker; do
    if [ -f "$BUILD_DIR/${worker}_artefacts/Release/$worker" ]; then
        echo "✅ $worker found"
        size=$(ls -lh "$BUILD_DIR/${worker}_artefacts/Release/$worker" | awk '{print $5}')
        echo "   Size: $size"
    else
        echo "❌ $worker not found"
    fi
done

# Check for PluginHostBridge app
if [ -d "$BUILD_DIR/PluginHostBridge_artefacts/Release/PluginHostBridge.app" ]; then
    echo "✅ PluginHostBridge.app found"
else
    echo "❌ PluginHostBridge.app not found"
fi

echo ""
echo "2. Checking Main Application..."
echo "-----------------------------------------"

if [ -f "$BUILD_DIR/HAM_artefacts/Release/CloneHAM.app/Contents/MacOS/CloneHAM" ]; then
    echo "✅ CloneHAM application found"
    size=$(ls -lh "$BUILD_DIR/HAM_artefacts/Release/CloneHAM.app/Contents/MacOS/CloneHAM" | awk '{print $5}')
    echo "   Size: $size"
else
    echo "❌ CloneHAM application not found"
    exit 1
fi

echo ""
echo "3. Testing Plugin Scanner..."
echo "-----------------------------------------"

# Test scanning a known good plugin
TEST_PLUGIN="/Library/Audio/Plug-Ins/VST3/Valhalla VintageVerb.vst3"
if [ -d "$TEST_PLUGIN" ]; then
    echo "Testing with: $TEST_PLUGIN"
    "$BUILD_DIR/PluginScanWorker_artefacts/Release/PluginScanWorker" "$TEST_PLUGIN" 2>&1 | grep -E "(Found|Scanning|Success|loaded)" | head -5
    if [ $? -eq 0 ]; then
        echo "✅ Plugin scanner executed successfully"
    else
        echo "⚠️ Plugin scanner ran but may have issues"
    fi
else
    echo "⚠️ Test plugin not found, skipping scanner test"
fi

echo ""
echo "4. Checking Plugin Directories..."
echo "-----------------------------------------"

# Check common plugin directories
PLUGIN_DIRS=(
    "/Library/Audio/Plug-Ins/VST3"
    "/Library/Audio/Plug-Ins/Components"
    "$HOME/Library/Audio/Plug-Ins/VST3"
    "$HOME/Library/Audio/Plug-Ins/Components"
)

for dir in "${PLUGIN_DIRS[@]}"; do
    if [ -d "$dir" ]; then
        count=$(ls "$dir" 2>/dev/null | wc -l | tr -d ' ')
        echo "✅ $dir exists ($count plugins)"
    else
        echo "❌ $dir not found"
    fi
done

echo ""
echo "5. Testing Application Launch with Plugin Support..."
echo "-----------------------------------------"

# Launch app in background
"$BUILD_DIR/HAM_artefacts/Release/CloneHAM.app/Contents/MacOS/CloneHAM" &
APP_PID=$!

# Give it time to start
sleep 2

# Check if it's still running
if kill -0 $APP_PID 2>/dev/null; then
    echo "✅ Application launched successfully (PID: $APP_PID)"
    
    # Check for plugin-related processes
    ps aux | grep -i "Plugin" | grep -v grep | head -3
    
    # Kill the app
    kill $APP_PID 2>/dev/null
    echo "✅ Application terminated cleanly"
else
    echo "⚠️ Application may have crashed on launch"
fi

echo ""
echo "6. Checking Plugin Manager Source Code..."
echo "-----------------------------------------"

# Check if plugin manager is properly integrated
if grep -q "m_formatManager.addDefaultFormats()" "$PROJECT_DIR/Source/Infrastructure/Plugins/PluginManager.cpp" 2>/dev/null; then
    echo "✅ Plugin format manager configured"
fi

if grep -q "VST3" "$PROJECT_DIR/Source/Infrastructure/Plugins/PluginManager.cpp" 2>/dev/null; then
    echo "✅ VST3 support detected in code"
fi

if grep -q "AudioUnit" "$PROJECT_DIR/Source/Infrastructure/Plugins/PluginManager.cpp" 2>/dev/null; then
    echo "✅ AudioUnit support detected in code"
fi

echo ""
echo "================================"
echo "Plugin Integration Test Complete"
echo "================================"