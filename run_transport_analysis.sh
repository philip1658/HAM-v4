#!/bin/bash

echo "=========================================="
echo "PVS-Studio Transport System Analysis"
echo "=========================================="

# Create analysis directory
mkdir -p pvs_analysis

# Run PVS-Studio on transport-related files
echo "Analyzing Transport and Clock system..."

pvs-studio-analyzer analyze \
    -o pvs_analysis/transport.log \
    -e build \
    -j8 \
    Source/Domain/Transport/Transport.cpp \
    Source/Domain/Clock/MasterClock.cpp \
    Source/Infrastructure/Audio/HAMAudioProcessor.cpp \
    Source/Domain/Engines/SequencerEngine.cpp 2>/dev/null

# Convert to readable format
echo "Converting analysis results..."
plog-converter \
    -a "GA:1,2;64:1,2;OP:1,2" \
    -t tasklist \
    -o pvs_analysis/transport_issues.txt \
    pvs_analysis/transport.log

echo ""
echo "Analysis Results:"
echo "-----------------"

if [ -f pvs_analysis/transport_issues.txt ]; then
    # Filter for critical issues related to our problem
    grep -E "(clock|Clock|transport|Transport|play|Play|start|Start|processBlock)" pvs_analysis/transport_issues.txt | head -20
    
    # Count issues
    ISSUE_COUNT=$(wc -l < pvs_analysis/transport_issues.txt)
    echo ""
    echo "Total issues found: $ISSUE_COUNT"
else
    echo "No analysis output generated. Checking with simpler analysis..."
    
    # Fallback to cppcheck if PVS-Studio isn't working
    echo ""
    echo "Running cppcheck analysis..."
    /opt/homebrew/bin/cppcheck \
        --enable=all \
        --std=c++20 \
        --suppress=missingInclude \
        Source/Domain/Transport/Transport.cpp \
        Source/Domain/Clock/MasterClock.cpp \
        Source/Infrastructure/Audio/HAMAudioProcessor.cpp 2>&1 | \
        grep -v "information:" | head -20
fi

echo ""
echo "=========================================="
echo "Quick Code Review"
echo "=========================================="

# Check for common issues
echo "Checking for potential issues..."

# Check if Transport::play() actually starts the clock
echo ""
echo "1. Does Transport::play() call m_clock.start()?"
grep -n "m_clock.start()" Source/Domain/Transport/Transport.cpp || echo "   ❌ NOT FOUND - This is the problem!"

# Check if processBlock checks isPlaying
echo ""
echo "2. Does processBlock check if transport is playing?"
grep -n "isPlaying()" Source/Infrastructure/Audio/HAMAudioProcessor.cpp || echo "   ❌ NOT FOUND"

# Check if clock processBlock is called
echo ""
echo "3. Does HAMAudioProcessor::processBlock call m_masterClock->processBlock?"
grep -n "m_masterClock->processBlock" Source/Infrastructure/Audio/HAMAudioProcessor.cpp || echo "   ❌ NOT FOUND - Clock not being processed!"

# Check if sequencer processBlock is called
echo ""
echo "4. Does HAMAudioProcessor::processBlock call m_sequencerEngine->processBlock?"
grep -n "m_sequencerEngine->processBlock" Source/Infrastructure/Audio/HAMAudioProcessor.cpp || echo "   ❌ NOT FOUND - Sequencer not being processed!"

echo ""
echo "=========================================="