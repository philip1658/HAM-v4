#!/bin/bash

# HAM Performance Test Suite
# Comprehensive performance verification

echo "========================================="
echo "HAM Performance Test Suite"
echo "========================================="
echo ""

# Parse arguments
TEST_TYPE=${1:-"all"}

# Check if HAM is built
if [ ! -f "../build/HAM.app/Contents/MacOS/HAM" ]; then
    echo "❌ ERROR: HAM.app not found. Run ./build.sh first"
    exit 1
fi

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Performance thresholds
CPU_TARGET=5.0
CPU_FAIL=10.0
MEMORY_TARGET=200
MEMORY_FAIL=300
JITTER_TARGET=0.05
JITTER_FAIL=0.1

echo "Performance Targets (M3 Max):"
echo "  CPU: < ${CPU_TARGET}% (fail at ${CPU_FAIL}%)"
echo "  Memory: < ${MEMORY_TARGET}MB (fail at ${MEMORY_FAIL}MB)"
echo "  Jitter: < ${JITTER_TARGET}ms (fail at ${JITTER_FAIL}ms)"
echo ""

# Function to test CPU usage
test_cpu() {
    echo "TEST: CPU Usage"
    echo "---------------"
    echo "Running with 1 track for 30 seconds..."
    
    # Start HAM with performance monitoring
    ../build/HAM.app/Contents/MacOS/HAM --test-performance-cpu &
    HAM_PID=$!
    
    sleep 2  # Let it stabilize
    
    # Sample CPU usage
    total_cpu=0
    samples=0
    max_cpu=0
    
    for i in {1..28}; do
        cpu=$(ps aux | grep $HAM_PID | grep -v grep | awk '{print $3}')
        if [ ! -z "$cpu" ]; then
            total_cpu=$(echo "$total_cpu + $cpu" | bc)
            samples=$((samples + 1))
            
            # Track max
            if (( $(echo "$cpu > $max_cpu" | bc -l) )); then
                max_cpu=$cpu
            fi
            
            # Show progress
            echo -n "."
        fi
        sleep 1
    done
    echo ""
    
    kill $HAM_PID 2>/dev/null
    
    # Calculate average
    if [ $samples -gt 0 ]; then
        avg_cpu=$(echo "scale=2; $total_cpu / $samples" | bc)
        
        echo "  Average CPU: ${avg_cpu}%"
        echo "  Peak CPU: ${max_cpu}%"
        
        if (( $(echo "$avg_cpu < $CPU_TARGET" | bc -l) )); then
            echo -e "${GREEN}✅ PASS: CPU usage within target${NC}"
            return 0
        elif (( $(echo "$avg_cpu < $CPU_FAIL" | bc -l) )); then
            echo -e "${YELLOW}⚠️  WARNING: CPU usage high but acceptable${NC}"
            return 1
        else
            echo -e "${RED}❌ FAIL: CPU usage too high${NC}"
            return 2
        fi
    else
        echo -e "${RED}❌ ERROR: Could not measure CPU${NC}"
        return 2
    fi
}

# Function to test memory usage
test_memory() {
    echo ""
    echo "TEST: Memory Usage"
    echo "------------------"
    echo "Running memory test for 60 seconds..."
    
    # Start HAM
    ../build/HAM.app/Contents/MacOS/HAM --test-performance-memory &
    HAM_PID=$!
    
    sleep 2  # Let it stabilize
    
    # Get initial memory
    initial_mem=$(ps aux | grep $HAM_PID | grep -v grep | awk '{print $6}')
    initial_mem_mb=$((initial_mem / 1024))
    echo "  Initial memory: ${initial_mem_mb}MB"
    
    # Monitor for leaks
    echo -n "  Monitoring"
    max_mem=$initial_mem
    
    for i in {1..58}; do
        current_mem=$(ps aux | grep $HAM_PID | grep -v grep | awk '{print $6}')
        if [ ! -z "$current_mem" ]; then
            if (( current_mem > max_mem )); then
                max_mem=$current_mem
            fi
        fi
        echo -n "."
        sleep 1
    done
    echo ""
    
    # Get final memory
    final_mem=$(ps aux | grep $HAM_PID | grep -v grep | awk '{print $6}')
    final_mem_mb=$((final_mem / 1024))
    max_mem_mb=$((max_mem / 1024))
    
    kill $HAM_PID 2>/dev/null
    
    echo "  Final memory: ${final_mem_mb}MB"
    echo "  Peak memory: ${max_mem_mb}MB"
    
    # Check for leaks
    leak_mb=$((final_mem_mb - initial_mem_mb))
    echo "  Memory growth: ${leak_mb}MB"
    
    if [ $final_mem_mb -lt $MEMORY_TARGET ] && [ $leak_mb -lt 10 ]; then
        echo -e "${GREEN}✅ PASS: Memory usage good, no leaks${NC}"
        return 0
    elif [ $final_mem_mb -lt $MEMORY_FAIL ]; then
        echo -e "${YELLOW}⚠️  WARNING: Memory usage acceptable${NC}"
        return 1
    else
        echo -e "${RED}❌ FAIL: Memory usage too high or leaking${NC}"
        return 2
    fi
}

# Function to test timing accuracy
test_timing() {
    echo ""
    echo "TEST: MIDI Timing Accuracy"
    echo "--------------------------"
    echo "Analyzing timing jitter..."
    
    # Run timing test
    ../build/HAM.app/Contents/MacOS/HAM --test-timing-accuracy > /tmp/timing_test.log 2>&1 &
    HAM_PID=$!
    
    echo -n "  Collecting samples"
    for i in {1..10}; do
        echo -n "."
        sleep 1
    done
    echo ""
    
    kill $HAM_PID 2>/dev/null
    
    # Parse results
    if [ -f /tmp/timing_test.log ]; then
        avg_jitter=$(grep "Average jitter:" /tmp/timing_test.log | awk '{print $3}')
        max_jitter=$(grep "Max jitter:" /tmp/timing_test.log | awk '{print $3}')
        
        if [ ! -z "$avg_jitter" ]; then
            echo "  Average jitter: ${avg_jitter}ms"
            echo "  Max jitter: ${max_jitter}ms"
            
            if (( $(echo "$avg_jitter < $JITTER_TARGET" | bc -l) )); then
                echo -e "${GREEN}✅ PASS: Timing accuracy excellent${NC}"
                return 0
            elif (( $(echo "$avg_jitter < $JITTER_FAIL" | bc -l) )); then
                echo -e "${YELLOW}⚠️  WARNING: Timing acceptable${NC}"
                return 1
            else
                echo -e "${RED}❌ FAIL: Timing jitter too high${NC}"
                return 2
            fi
        fi
    fi
    
    echo -e "${RED}❌ ERROR: Could not measure timing${NC}"
    return 2
}

# Function to run quick tests
test_quick() {
    echo "Running quick performance check..."
    echo ""
    
    ../build/HAM.app/Contents/MacOS/HAM --test-quick &
    HAM_PID=$!
    
    sleep 5
    
    # Quick CPU check
    cpu=$(ps aux | grep $HAM_PID | grep -v grep | awk '{print $3}')
    mem=$(ps aux | grep $HAM_PID | grep -v grep | awk '{print $6}')
    mem_mb=$((mem / 1024))
    
    kill $HAM_PID 2>/dev/null
    
    echo "Quick Check Results:"
    echo "  CPU: ${cpu}%"
    echo "  Memory: ${mem_mb}MB"
    
    if (( $(echo "$cpu < $CPU_FAIL" | bc -l) )) && [ $mem_mb -lt $MEMORY_FAIL ]; then
        echo -e "${GREEN}✅ Quick check PASSED${NC}"
        return 0
    else
        echo -e "${RED}❌ Quick check FAILED${NC}"
        return 2
    fi
}

# Main test execution
case $TEST_TYPE in
    --cpu)
        test_cpu
        exit $?
        ;;
    --memory)
        test_memory
        exit $?
        ;;
    --timing)
        test_timing
        exit $?
        ;;
    --quick)
        test_quick
        exit $?
        ;;
    all|*)
        echo "Running all performance tests..."
        echo ""
        
        # Track results
        cpu_result=0
        mem_result=0
        timing_result=0
        
        # Run all tests
        test_cpu
        cpu_result=$?
        
        test_memory
        mem_result=$?
        
        test_timing
        timing_result=$?
        
        # Summary
        echo ""
        echo "========================================="
        echo "Performance Test Summary"
        echo "========================================="
        echo ""
        
        if [ $cpu_result -eq 0 ]; then
            echo -e "${GREEN}✅ CPU Usage: PASS${NC}"
        elif [ $cpu_result -eq 1 ]; then
            echo -e "${YELLOW}⚠️  CPU Usage: WARNING${NC}"
        else
            echo -e "${RED}❌ CPU Usage: FAIL${NC}"
        fi
        
        if [ $mem_result -eq 0 ]; then
            echo -e "${GREEN}✅ Memory Usage: PASS${NC}"
        elif [ $mem_result -eq 1 ]; then
            echo -e "${YELLOW}⚠️  Memory Usage: WARNING${NC}"
        else
            echo -e "${RED}❌ Memory Usage: FAIL${NC}"
        fi
        
        if [ $timing_result -eq 0 ]; then
            echo -e "${GREEN}✅ Timing Accuracy: PASS${NC}"
        elif [ $timing_result -eq 1 ]; then
            echo -e "${YELLOW}⚠️  Timing Accuracy: WARNING${NC}"
        else
            echo -e "${RED}❌ Timing Accuracy: FAIL${NC}"
        fi
        
        echo ""
        
        # Overall result
        if [ $cpu_result -eq 0 ] && [ $mem_result -eq 0 ] && [ $timing_result -eq 0 ]; then
            echo -e "${GREEN}🎉 ALL PERFORMANCE TESTS PASSED!${NC}"
            echo "HAM is performing within specifications."
            exit 0
        elif [ $cpu_result -le 1 ] && [ $mem_result -le 1 ] && [ $timing_result -le 1 ]; then
            echo -e "${YELLOW}⚠️  PERFORMANCE ACCEPTABLE WITH WARNINGS${NC}"
            echo "Some optimization may be beneficial."
            exit 1
        else
            echo -e "${RED}❌ PERFORMANCE TESTS FAILED${NC}"
            echo "Critical performance issues need addressing."
            exit 2
        fi
        ;;
esac