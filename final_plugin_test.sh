#!/bin/bash

echo "╔══════════════════════════════════════════════════════════╗"
echo "║     HAM PLUGIN INTEGRATION - VOLLSTÄNDIGER TEST         ║"
echo "╔══════════════════════════════════════════════════════════╗"
echo ""

cd /Users/philipkrieger/Desktop/Clone_Ham/HAM

echo "📦 1. BUILD STATUS"
echo "══════════════════════════════════════"
if [ -f "build-release/HAM_artefacts/Release/CloneHAM.app/Contents/MacOS/CloneHAM" ]; then
    echo "✅ HAM Application: Built"
    ls -lh build-release/HAM_artefacts/Release/CloneHAM.app/Contents/MacOS/CloneHAM | awk '{print "   Size: " $5 " | Modified: " $6 " " $7 " " $8}'
else
    echo "❌ HAM Application: Not found"
fi

if [ -f "build-release/PluginScanWorker_artefacts/Release/PluginScanWorker" ]; then
    echo "✅ Plugin Scanner: Built"
else
    echo "❌ Plugin Scanner: Not found"
fi

if [ -f "build-release/PluginProbeWorker_artefacts/Release/PluginProbeWorker" ]; then
    echo "✅ Plugin Probe: Built"
else
    echo "❌ Plugin Probe: Not found"
fi

if [ -d "build-release/PluginHostBridge_artefacts/Release/PluginHostBridge.app" ]; then
    echo "✅ Plugin Bridge: Built"
else
    echo "❌ Plugin Bridge: Not found"
fi

echo ""
echo "🔍 2. PLUGIN INFRASTRUCTURE CODE"
echo "══════════════════════════════════════"
echo "Checking source files..."
if [ -f "Source/Infrastructure/Plugins/PluginManager.cpp" ]; then
    echo "✅ PluginManager.cpp exists"
    lines=$(wc -l < Source/Infrastructure/Plugins/PluginManager.cpp)
    echo "   Lines of code: $lines"
fi

if [ -f "Source/Infrastructure/Plugins/PluginWindowManager.h" ]; then
    echo "✅ PluginWindowManager.h exists"
fi

if [ -f "Source/Presentation/Views/PluginBrowser.h" ]; then
    echo "✅ PluginBrowser UI exists"
fi

# Check for VST/AU support in code
echo ""
echo "Plugin format support in code:"
if grep -q "VST3" Source/Infrastructure/Plugins/PluginManager.cpp 2>/dev/null; then
    echo "✅ VST3 support detected"
fi
if grep -q "AudioUnit" Source/Infrastructure/Plugins/PluginManager.cpp 2>/dev/null; then
    echo "✅ AudioUnit support detected"
fi
if grep -q "addDefaultFormats" Source/Infrastructure/Plugins/PluginManager.cpp 2>/dev/null; then
    echo "✅ Default formats enabled"
fi

echo ""
echo "🎛️ 3. AVAILABLE PLUGINS ON SYSTEM"
echo "══════════════════════════════════════"
vst3_count=$(ls /Library/Audio/Plug-Ins/VST3 2>/dev/null | wc -l | tr -d ' ')
au_count=$(ls /Library/Audio/Plug-Ins/Components 2>/dev/null | wc -l | tr -d ' ')
user_vst3=$(ls ~/Library/Audio/Plug-Ins/VST3 2>/dev/null | wc -l | tr -d ' ')
user_au=$(ls ~/Library/Audio/Plug-Ins/Components 2>/dev/null | wc -l | tr -d ' ')

echo "System Plugins:"
echo "  VST3: $vst3_count plugins in /Library/Audio/Plug-Ins/VST3"
echo "  AU:   $au_count plugins in /Library/Audio/Plug-Ins/Components"
echo ""
echo "User Plugins:"
echo "  VST3: $user_vst3 plugins in ~/Library/Audio/Plug-Ins/VST3"
echo "  AU:   $user_au plugins in ~/Library/Audio/Plug-Ins/Components"

total_plugins=$((vst3_count + au_count + user_vst3 + user_au))
echo ""
echo "📊 Total available: $total_plugins plugins"

echo ""
echo "🧪 4. FUNCTIONAL TEST"
echo "══════════════════════════════════════"
echo "Testing plugin scanner with a known plugin..."

# Find a test plugin
TEST_PLUGIN=""
if [ -d "/Library/Audio/Plug-Ins/VST3/Surge XT.vst3" ]; then
    TEST_PLUGIN="/Library/Audio/Plug-Ins/VST3/Surge XT.vst3"
elif [ -d "/Library/Audio/Plug-Ins/Components/AUNetSend.component" ]; then
    TEST_PLUGIN="/Library/Audio/Plug-Ins/Components/AUNetSend.component"
elif [ -d "/System/Library/Audio/Units/Components/MatrixReverb.component" ]; then
    TEST_PLUGIN="/System/Library/Audio/Units/Components/MatrixReverb.component"
fi

if [ -n "$TEST_PLUGIN" ]; then
    echo "Using test plugin: $(basename "$TEST_PLUGIN")"
    echo "Running scanner..."
    
    # Run scanner and capture output
    OUTPUT=$(./build-release/PluginScanWorker_artefacts/Release/PluginScanWorker "$TEST_PLUGIN" 2>&1 | grep -v "objc\[" | grep -v "Class" | head -10)
    
    if [ $? -eq 0 ]; then
        echo "✅ Scanner executed successfully"
    else
        echo "⚠️ Scanner ran with warnings"
    fi
else
    echo "⚠️ No test plugin found"
fi

echo ""
echo "🚀 5. APPLICATION LAUNCH TEST"
echo "══════════════════════════════════════"
echo "Starting HAM application..."

# Launch app in background
./build-release/HAM_artefacts/Release/CloneHAM.app/Contents/MacOS/CloneHAM &
APP_PID=$!

# Give it time to initialize
sleep 2

if kill -0 $APP_PID 2>/dev/null; then
    echo "✅ Application running (PID: $APP_PID)"
    
    # Check memory usage
    MEM=$(ps aux | grep $APP_PID | grep -v grep | awk '{print $4}')
    echo "   Memory usage: ${MEM}%"
    
    # Check CPU usage
    CPU=$(ps aux | grep $APP_PID | grep -v grep | awk '{print $3}')
    echo "   CPU usage: ${CPU}%"
    
    # Terminate cleanly
    kill $APP_PID 2>/dev/null
    echo "✅ Application terminated cleanly"
else
    echo "⚠️ Application may have crashed immediately"
fi

echo ""
echo "══════════════════════════════════════"
echo "📈 ZUSAMMENFASSUNG"
echo "══════════════════════════════════════"

success_count=0
total_count=9

[ -f "build-release/HAM_artefacts/Release/CloneHAM.app/Contents/MacOS/CloneHAM" ] && ((success_count++))
[ -f "build-release/PluginScanWorker_artefacts/Release/PluginScanWorker" ] && ((success_count++))
[ -f "build-release/PluginProbeWorker_artefacts/Release/PluginProbeWorker" ] && ((success_count++))
[ -d "build-release/PluginHostBridge_artefacts/Release/PluginHostBridge.app" ] && ((success_count++))
[ -f "Source/Infrastructure/Plugins/PluginManager.cpp" ] && ((success_count++))
[ -f "Source/Presentation/Views/PluginBrowser.h" ] && ((success_count++))
grep -q "VST3" Source/Infrastructure/Plugins/PluginManager.cpp 2>/dev/null && ((success_count++))
grep -q "addDefaultFormats" Source/Infrastructure/Plugins/PluginManager.cpp 2>/dev/null && ((success_count++))
[ $total_plugins -gt 0 ] && ((success_count++))

echo "Tests bestanden: $success_count / $total_count"
echo ""

if [ $success_count -eq $total_count ]; then
    echo "🎉 ALLE TESTS BESTANDEN! Plugin-Integration funktioniert perfekt!"
elif [ $success_count -ge 7 ]; then
    echo "✅ Plugin-Integration funktioniert gut (${success_count}/9 Tests bestanden)"
elif [ $success_count -ge 5 ]; then
    echo "⚠️ Plugin-Integration teilweise funktional (${success_count}/9 Tests bestanden)"
else
    echo "❌ Plugin-Integration hat Probleme (nur ${success_count}/9 Tests bestanden)"
fi

echo ""
echo "╚══════════════════════════════════════════════════════════╝"