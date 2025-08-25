#!/bin/bash

echo "=== Testing HAM Plugin System ==="

# 1. Check if app exists
if [ -f ~/Desktop/CloneHAM.app/Contents/MacOS/CloneHAM ]; then
    echo "✅ App exists on Desktop"
else
    echo "❌ App not found on Desktop"
    exit 1
fi

# 2. Run app briefly to trigger initialization
echo "Starting app briefly to trigger initialization..."
timeout 3 ~/Desktop/CloneHAM.app/Contents/MacOS/CloneHAM 2>/dev/null &
APP_PID=$!
sleep 2
kill $APP_PID 2>/dev/null

# 3. Check if directories were created
echo ""
echo "Checking application directories..."
if [ -d ~/Library/Application\ Support/HAM ]; then
    echo "✅ HAM directory created"
else
    echo "❌ HAM directory not created"
fi

if [ -d ~/Library/Application\ Support/CloneHAM ]; then
    echo "✅ CloneHAM directory created"
else
    echo "❌ CloneHAM directory not created"
fi

# 4. Check for plugin scanner tools
echo ""
echo "Checking plugin scanner tools..."
if [ -f ~/Desktop/Clone_Ham/HAM/build/PluginScanWorker_artefacts/Release/PluginScanWorker ]; then
    echo "✅ PluginScanWorker built"
else
    echo "❌ PluginScanWorker not found"
fi

# 5. Check if plugin list exists
echo ""
echo "Checking for plugin cache..."
if [ -f ~/Library/Application\ Support/HAM/plugin_list.xml ]; then
    echo "✅ Plugin list exists"
    echo "Plugin count: $(grep -c "<PLUGIN " ~/Library/Application\ Support/HAM/plugin_list.xml)"
else
    echo "⚠️ No plugin list yet (will be created after first scan)"
fi

echo ""
echo "=== Test Complete ==="
echo "To run the app: open ~/Desktop/CloneHAM.app"
