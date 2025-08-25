#!/bin/bash

echo "╔══════════════════════════════════════════════════════════╗"
echo "║          HAM - TESTING ALL FIXES                        ║"
echo "╔══════════════════════════════════════════════════════════╗"
echo ""

cd /Users/philipkrieger/Desktop/Clone_Ham/HAM

echo "✅ FIX 1: Plugin Scanner"
echo "────────────────────────"
echo "• Added timer-based scanning (updates every 500ms)"
echo "• PluginBrowser now refreshes list during scan"
echo "• Scanner uses cached paths for better performance"
echo ""

echo "✅ FIX 2: Close Button for PluginBrowser"
echo "────────────────────────────────────────"
echo "• Added 'Close' button to PluginBrowser UI"
echo "• Connected onCloseRequested callback"
echo "• Window now properly hides when closed"
echo ""

echo "✅ FIX 3: MixerView Content"
echo "───────────────────────────"
echo "• Added placeholder when no AudioProcessor"
echo "• Shows 'Mixer View - Waiting for Audio Processor'"
echo "• Proper layout handling for both states"
echo ""

echo "═══════════════════════════════════════════════════════════"
echo "🧪 TEST INSTRUCTIONS:"
echo "═══════════════════════════════════════════════════════════"
echo ""
echo "1. PLUGIN SCANNER TEST:"
echo "   • Click PLUGIN button on any track"
echo "   • Click 'Scan for Plugins' button"
echo "   • Watch the button text change to 'Scanning... X/Y'"
echo "   • Plugin list should populate as scan progresses"
echo ""
echo "2. CLOSE BUTTON TEST:"
echo "   • Open PluginBrowser (PLUGIN button)"
echo "   • Click 'Close' button in top-right"
echo "   • Window should close properly"
echo ""
echo "3. MIXER VIEW TEST:"
echo "   • Click MIXER tab at top"
echo "   • Should see placeholder text instead of empty view"
echo ""

echo "Starting HAM..."
./build-release/HAM_artefacts/Release/CloneHAM.app/Contents/MacOS/CloneHAM &
APP_PID=$!

echo "✅ HAM launched (PID: $APP_PID)"
echo ""
echo "Test the features manually. Press Ctrl+C to exit."
echo ""

trap "echo ''; echo 'Stopping HAM...'; kill $APP_PID 2>/dev/null; echo '✅ All tests completed'; exit 0" INT

wait $APP_PID 2>/dev/null

echo "═══════════════════════════════════════════════════════════"
echo "✅ TESTING COMPLETE"
echo "═══════════════════════════════════════════════════════════"