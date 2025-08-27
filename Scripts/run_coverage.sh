#!/bin/bash

# HAM Code Coverage Report Generator
# Generates comprehensive coverage reports using lcov and gcovr

set -e

PROJECT_ROOT="/Users/philipkrieger/Desktop/Clone_Ham/HAM"
BUILD_DIR="${PROJECT_ROOT}/build-coverage"
COVERAGE_DIR="${BUILD_DIR}/coverage-report"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}HAM Coverage Report Generator${NC}"
echo "==============================="

# Check if tools are available
check_tools() {
    local missing_tools=()
    
    if ! command -v lcov &> /dev/null; then
        missing_tools+=("lcov")
    fi
    
    if ! command -v genhtml &> /dev/null; then
        missing_tools+=("genhtml")
    fi
    
    if ! command -v gcovr &> /dev/null; then
        missing_tools+=("gcovr")
    fi
    
    if [ ${#missing_tools[@]} -ne 0 ]; then
        echo -e "${RED}Missing required tools:${NC} ${missing_tools[*]}"
        echo "Install with: brew install lcov gcovr"
        exit 1
    fi
}

# Clean previous coverage data
clean_coverage() {
    echo -e "${YELLOW}Cleaning previous coverage data...${NC}"
    rm -rf "${BUILD_DIR}"
    mkdir -p "${BUILD_DIR}"
}

# Build with coverage enabled
build_with_coverage() {
    echo -e "${YELLOW}Building with coverage enabled...${NC}"
    cd "${BUILD_DIR}"
    
    /opt/homebrew/bin/cmake .. \
        -G Ninja \
        -DCMAKE_BUILD_TYPE=Debug \
        -DHAM_ENABLE_COVERAGE=ON \
        -DHAM_BUILD_TESTS=ON
    
    /opt/homebrew/bin/ninja -j8
    
    if [ $? -ne 0 ]; then
        echo -e "${RED}Build failed!${NC}"
        exit 1
    fi
}

# Run tests to generate coverage data
run_tests() {
    echo -e "${YELLOW}Running tests to generate coverage data...${NC}"
    cd "${BUILD_DIR}"
    
    # Run CTest
    ctest --output-on-failure --verbose
    
    # Also run the main application briefly if possible
    echo -e "${YELLOW}Running main application for additional coverage...${NC}"
    timeout 5 ./HAM_artefacts/Debug/CloneHAM.app/Contents/MacOS/CloneHAM --no-gui || true
}

# Generate lcov HTML report
generate_lcov_report() {
    echo -e "${YELLOW}Generating lcov HTML report...${NC}"
    cd "${BUILD_DIR}"
    
    # Capture coverage data
    lcov --directory . --capture --output-file coverage.info
    
    # Remove unwanted files from coverage
    lcov --remove coverage.info \
        '/usr/*' \
        '*/JUCE/*' \
        '*/Tests/*' \
        '*/build*/*' \
        '*/Tools/*' \
        --output-file coverage_filtered.info
    
    # Generate HTML report
    genhtml coverage_filtered.info \
        --output-directory "${COVERAGE_DIR}/html" \
        --title "HAM Coverage Report" \
        --show-details \
        --legend
    
    echo -e "${GREEN}HTML report generated in: ${COVERAGE_DIR}/html${NC}"
}

# Generate gcovr reports
generate_gcovr_reports() {
    echo -e "${YELLOW}Generating gcovr reports...${NC}"
    cd "${BUILD_DIR}"
    
    # XML report for CI/CD
    gcovr --xml-pretty --exclude-unreachable-branches \
        --filter ../Source/ \
        --exclude ../Source/Tests/ \
        --exclude ../Source/Tools/ \
        --output "${COVERAGE_DIR}/coverage.xml"
    
    # JSON report
    gcovr --json-pretty --exclude-unreachable-branches \
        --filter ../Source/ \
        --exclude ../Source/Tests/ \
        --exclude ../Source/Tools/ \
        --output "${COVERAGE_DIR}/coverage.json"
    
    # Text summary
    gcovr --exclude-unreachable-branches \
        --filter ../Source/ \
        --exclude ../Source/Tests/ \
        --exclude ../Source/Tools/ \
        > "${COVERAGE_DIR}/coverage_summary.txt"
    
    echo -e "${GREEN}gcovr reports generated in: ${COVERAGE_DIR}${NC}"
}

# Display coverage summary
display_summary() {
    echo -e "${BLUE}Coverage Summary:${NC}"
    echo "==============="
    
    if [ -f "${COVERAGE_DIR}/coverage_summary.txt" ]; then
        cat "${COVERAGE_DIR}/coverage_summary.txt"
    fi
    
    echo ""
    echo -e "${GREEN}Reports generated:${NC}"
    echo "- HTML Report: ${COVERAGE_DIR}/html/index.html"
    echo "- XML Report: ${COVERAGE_DIR}/coverage.xml"
    echo "- JSON Report: ${COVERAGE_DIR}/coverage.json"
    echo "- Text Summary: ${COVERAGE_DIR}/coverage_summary.txt"
}

# Main execution
main() {
    cd "${PROJECT_ROOT}"
    
    check_tools
    clean_coverage
    build_with_coverage
    run_tests
    generate_lcov_report
    generate_gcovr_reports
    display_summary
    
    echo ""
    echo -e "${GREEN}Coverage analysis complete!${NC}"
    echo -e "Open HTML report: ${BLUE}open ${COVERAGE_DIR}/html/index.html${NC}"
}

# Run main function
main "$@"