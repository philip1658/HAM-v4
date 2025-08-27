#!/bin/bash

# HAM Coverage Upload Script
# Uploads coverage data to codecov.io and other coverage services

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

echo -e "${BLUE}HAM Coverage Upload Service${NC}"
echo "============================"

# Check if codecov CLI is available
check_codecov() {
    if ! command -v codecovcli &> /dev/null; then
        echo -e "${YELLOW}codecovcli not found. Installing...${NC}"
        if command -v brew &> /dev/null; then
            brew install codecov/codecov/codecov-cli
        elif command -v pip3 &> /dev/null; then
            pip3 install codecov-cli
        else
            echo -e "${RED}Cannot install codecovcli. Please install manually.${NC}"
            exit 1
        fi
    fi
}

# Upload to codecov.io
upload_codecov() {
    echo -e "${YELLOW}Uploading to codecov.io...${NC}"
    
    # Check for codecov token
    if [ -z "${CODECOV_TOKEN}" ]; then
        echo -e "${YELLOW}CODECOV_TOKEN not set. Attempting upload without token...${NC}"
        echo "Note: Public repos don't require a token, but private repos do."
    fi
    
    cd "${PROJECT_ROOT}"
    
    # Upload XML coverage report
    if [ -f "${COVERAGE_DIR}/coverage.xml" ]; then
        if [ -n "${CODECOV_TOKEN}" ]; then
            codecovcli upload-process \
                --token "${CODECOV_TOKEN}" \
                --file "${COVERAGE_DIR}/coverage.xml" \
                --flags unittests \
                --name "HAM-Coverage"
        else
            codecovcli upload-process \
                --file "${COVERAGE_DIR}/coverage.xml" \
                --flags unittests \
                --name "HAM-Coverage"
        fi
        
        if [ $? -eq 0 ]; then
            echo -e "${GREEN}Successfully uploaded to codecov.io${NC}"
        else
            echo -e "${RED}Failed to upload to codecov.io${NC}"
        fi
    else
        echo -e "${RED}Coverage XML file not found. Run coverage generation first.${NC}"
        exit 1
    fi
}

# Generate coverage badges
generate_badges() {
    echo -e "${YELLOW}Generating coverage badges...${NC}"
    
    # Extract coverage percentage from summary
    if [ -f "${COVERAGE_DIR}/coverage_summary.txt" ]; then
        local coverage_percent=$(grep "TOTAL" "${COVERAGE_DIR}/coverage_summary.txt" | awk '{print $4}' | sed 's/%//')
        
        if [ -n "$coverage_percent" ]; then
            echo "Coverage: ${coverage_percent}%" > "${COVERAGE_DIR}/badge_info.txt"
            echo -e "${GREEN}Coverage percentage: ${coverage_percent}%${NC}"
            
            # Generate shield.io badge URL
            local badge_color="red"
            if (( $(echo "$coverage_percent > 80" | bc -l) )); then
                badge_color="brightgreen"
            elif (( $(echo "$coverage_percent > 60" | bc -l) )); then
                badge_color="yellow"
            elif (( $(echo "$coverage_percent > 40" | bc -l) )); then
                badge_color="orange"
            fi
            
            local badge_url="https://img.shields.io/badge/coverage-${coverage_percent}%25-${badge_color}"
            echo "Badge URL: ${badge_url}" >> "${COVERAGE_DIR}/badge_info.txt"
            echo -e "${BLUE}Badge URL: ${badge_url}${NC}"
        fi
    fi
}

# Create coverage report summary for GitHub
create_github_summary() {
    echo -e "${YELLOW}Creating GitHub summary...${NC}"
    
    local summary_file="${COVERAGE_DIR}/github_summary.md"
    
    cat > "$summary_file" << EOF
# HAM Code Coverage Report

## Coverage Summary

\`\`\`
$(cat "${COVERAGE_DIR}/coverage_summary.txt" 2>/dev/null || echo "Coverage data not available")
\`\`\`

## Reports Generated

- **HTML Report**: Detailed line-by-line coverage
- **XML Report**: Machine-readable format for CI/CD
- **JSON Report**: Structured data for analysis

## Quick Stats

EOF

    # Add coverage percentage if available
    if [ -f "${COVERAGE_DIR}/badge_info.txt" ]; then
        echo "$(cat "${COVERAGE_DIR}/badge_info.txt")" >> "$summary_file"
    fi
    
    echo -e "${GREEN}GitHub summary created: ${summary_file}${NC}"
}

# Validate coverage files
validate_coverage() {
    echo -e "${YELLOW}Validating coverage files...${NC}"
    
    local files_to_check=(
        "${COVERAGE_DIR}/coverage.xml"
        "${COVERAGE_DIR}/coverage.json"
        "${COVERAGE_DIR}/coverage_summary.txt"
    )
    
    for file in "${files_to_check[@]}"; do
        if [ -f "$file" ]; then
            echo -e "${GREEN}✓${NC} Found: $(basename "$file")"
        else
            echo -e "${RED}✗${NC} Missing: $(basename "$file")"
        fi
    done
}

# Display upload summary
display_summary() {
    echo ""
    echo -e "${BLUE}Upload Summary:${NC}"
    echo "==============="
    echo "Project: HAM (Happy Accident Machine)"
    echo "Coverage files location: ${COVERAGE_DIR}"
    
    if [ -f "${COVERAGE_DIR}/badge_info.txt" ]; then
        echo ""
        cat "${COVERAGE_DIR}/badge_info.txt"
    fi
    
    echo ""
    echo -e "${GREEN}Available reports:${NC}"
    echo "- HTML: ${COVERAGE_DIR}/html/index.html"
    echo "- GitHub Summary: ${COVERAGE_DIR}/github_summary.md"
    echo "- Badge Info: ${COVERAGE_DIR}/badge_info.txt"
}

# Main execution
main() {
    cd "${PROJECT_ROOT}"
    
    validate_coverage
    check_codecov
    upload_codecov
    generate_badges
    create_github_summary
    display_summary
    
    echo ""
    echo -e "${GREEN}Coverage upload complete!${NC}"
}

# Handle command line arguments
case "${1:-upload}" in
    "upload")
        main
        ;;
    "validate")
        validate_coverage
        ;;
    "badges")
        generate_badges
        ;;
    "summary")
        create_github_summary
        ;;
    *)
        echo "Usage: $0 [upload|validate|badges|summary]"
        echo "  upload   - Upload coverage and generate all reports (default)"
        echo "  validate - Check if coverage files exist"
        echo "  badges   - Generate coverage badges only"
        echo "  summary  - Create GitHub summary only"
        ;;
esac