#!/bin/bash

# HAM Code Coverage Script
# This script builds the project with coverage enabled, runs tests, and generates a coverage report

set -e

echo "========================================="
echo "HAM Code Coverage Report Generator"
echo "========================================="

# Clean previous build
echo "Cleaning previous build..."
rm -rf build-coverage
mkdir build-coverage
cd build-coverage

# Configure with coverage enabled
echo "Configuring build with coverage enabled..."
/opt/homebrew/bin/cmake .. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DHAM_BUILD_TESTS=ON \
    -DHAM_ENABLE_COVERAGE=ON \
    -G "Unix Makefiles"

# Build
echo "Building project and tests..."
make -j8

# Run tests
echo "Running tests..."
/opt/homebrew/bin/ctest --output-on-failure || true

# Generate coverage report
echo "Generating coverage report..."
/opt/homebrew/bin/lcov --directory . --capture --output-file coverage.info

# Remove external files from coverage
echo "Filtering coverage data..."
/opt/homebrew/bin/lcov --remove coverage.info \
    '/usr/*' \
    '/opt/*' \
    '*/JUCE/*' \
    '*/Tests/*' \
    '*/build-coverage/*' \
    --output-file coverage.info

# Generate HTML report
echo "Generating HTML report..."
/opt/homebrew/bin/genhtml coverage.info \
    --output-directory coverage-report \
    --title "HAM Code Coverage Report" \
    --legend \
    --show-details

# Calculate coverage percentage
echo "========================================="
echo "Coverage Summary:"
/opt/homebrew/bin/lcov --summary coverage.info

# Open report in browser
echo "========================================="
echo "Coverage report generated in: build-coverage/coverage-report/index.html"
echo "Opening in browser..."
open coverage-report/index.html

echo "========================================="
echo "Coverage report complete!"