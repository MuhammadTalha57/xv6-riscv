#!/bin/bash

# MLFQ Test Runner Script
# This script builds xv6, runs tests, and generates a report

set -e

REPORT_FILE="mlfq_test_report.txt"
TIMESTAMP=$(date '+%Y-%m-%d %H:%M:%S')

echo "=========================================="
echo "MLFQ Scheduler Test Suite"
echo "=========================================="
echo "Timestamp: $TIMESTAMP"
echo ""

# Clean and build
echo "[1/4] Building xv6..."
make clean > /dev/null 2>&1 || true
if ! make kernel/kernel fs.img > build.log 2>&1; then
    echo "ERROR: Build failed. Check build.log for details."
    exit 1
fi
echo "✓ Build successful"

# Check if mlfqtest was built
if [ ! -f "user/_mlfqtest" ]; then
    echo "ERROR: mlfqtest not found. Make sure it's in UPROGS in Makefile."
    exit 1
fi
echo "✓ mlfqtest binary found"

# Run tests using Python test script
echo ""
echo "[2/4] Running MLFQ tests..."
if python3 test-mlfq.py --mlfq-only --report "$REPORT_FILE" 2>&1 | tee test_output.log; then
    echo "✓ Tests completed"
else
    echo "⚠ Some tests may have failed. Check $REPORT_FILE for details."
fi

# Display report
echo ""
echo "[3/4] Test Results:"
echo "=========================================="
if [ -f "$REPORT_FILE" ]; then
    cat "$REPORT_FILE"
else
    echo "Report file not found."
fi

echo ""
echo "[4/4] Summary"
echo "=========================================="
echo "Report saved to: $REPORT_FILE"
echo "Build log: build.log"
echo "Test output: test_output.log"
echo ""
echo "To view detailed results, check $REPORT_FILE"

