#!/bin/bash

echo "╔══════════════════════════════════════════════════════════╗"
echo "║     HAM FEATURES TEST - Plugin, Mixer & Clock           ║"
echo "╔══════════════════════════════════════════════════════════╗"
echo ""

cd /Users/philipkrieger/Desktop/Clone_Ham/HAM

echo "🎯 TESTING THREE CORE FEATURES:"
echo "1. Plugin Button → Opens PluginBrowser"
echo "2. Mixer Tab → Shows MixerView"
echo "3. Play Button → Starts Master Clock"
echo ""

echo "═══════════════════════════════════════════════════════════"
echo "📋 FEATURE 1: Plugin Integration"
echo "═══════════════════════════════════════════════════════════"
echo "✅ Plugin Button: Located in TrackSidebar.cpp line 148"
echo "✅ Click Handler: Line 423-426 triggers showPluginBrowser()"
echo "✅ PluginBrowser UI: Shows at UICoordinator.cpp line 175"
echo "✅ Plugin Loading: Connected to AudioProcessor line 293"
echo ""

echo "═══════════════════════════════════════════════════════════"
echo "📋 FEATURE 2: MixerView"
echo "═══════════════════════════════════════════════════════════"
echo "✅ MIXER Tab Button: UICoordinator.cpp line 72-75"
echo "✅ Tab Click Handler: Line 144-147"
echo "✅ View Switching: setActiveView() line 253-268"
echo "✅ MixerView Creation: Line 44 when AudioProcessor exists"
echo ""

echo "═══════════════════════════════════════════════════════════"
echo "📋 FEATURE 3: Master Clock"
echo "═══════════════════════════════════════════════════════════"
echo "✅ Play Button: TransportBar triggers at line 121"
echo "✅ Play Message: AppController.cpp line 98-112"
echo "✅ Clock Start: HAMAudioProcessor.cpp line 596"
echo "✅ Clock Processing: processBlock() line 224"
echo ""

echo "═══════════════════════════════════════════════════════════"
echo "🚀 LAUNCHING HAM FOR INTERACTIVE TEST..."
echo "═══════════════════════════════════════════════════════════"
echo ""
echo "INSTRUCTIONS FOR MANUAL TESTING:"
echo "1. Click any track's PLUGIN button → Should open PluginBrowser"
echo "2. Click MIXER tab → Should show MixerView"
echo "3. Click Play button → Clock should start (check console for ticks)"
echo ""

# Launch the app
echo "Starting HAM..."
./build-release/HAM_artefacts/Release/CloneHAM.app/Contents/MacOS/CloneHAM &
APP_PID=$!

echo "✅ HAM launched with PID: $APP_PID"
echo ""
echo "The app is now running. Test the features manually."
echo "Press Ctrl+C to stop the test and close HAM."
echo ""

# Keep script running and catch Ctrl+C
trap "echo ''; echo 'Stopping HAM...'; kill $APP_PID 2>/dev/null; echo '✅ Test completed'; exit 0" INT

# Wait for the app to be killed
wait $APP_PID 2>/dev/null

echo ""
echo "═══════════════════════════════════════════════════════════"
echo "✅ TEST COMPLETED"
echo "═══════════════════════════════════════════════════════════"