#!/bin/bash

# Simple Report Generator
# Generates a test report from manual test runs

REPORT_FILE="mlfq_test_report.txt"
TIMESTAMP=$(date '+%Y-%m-%d %H:%M:%S')

echo "=========================================="
echo "MLFQ Test Report Generator"
echo "=========================================="
echo ""
echo "This script helps generate a report from test results."
echo "Run 'make qemu' and execute 'mlfqtest' manually, then"
echo "provide the output when prompted."
echo ""

read -p "Did mlfqtest complete successfully? (y/n): " success
read -p "Did you see 'All MLFQ Tests Completed!'? (y/n): " completed
read -p "Any errors or panics? (y/n): " errors

echo ""
echo "Generating report..."

cat > "$REPORT_FILE" << EOF
================================================================================
MLFQ Scheduler Test Report
================================================================================
Generated: $TIMESTAMP

SUMMARY
--------------------------------------------------------------------------------
Test: mlfqtest
Status: $([ "$success" = "y" ] && echo "PASS ✓" || echo "FAIL ✗")
Completed: $([ "$completed" = "y" ] && echo "Yes" || echo "No")
Errors: $([ "$errors" = "y" ] && echo "Yes" || echo "No")

DETAILED RESULTS
--------------------------------------------------------------------------------

[$( [ "$success" = "y" ] && echo "✓" || echo "✗")] mlfqtest
  Status: $([ "$success" = "y" ] && echo "PASS" || echo "FAIL")
  Completion: $([ "$completed" = "y" ] && echo "Completed" || echo "Not Completed")
  Errors: $([ "$errors" = "y" ] && echo "Errors detected" || echo "No errors")

TEST DESCRIPTION
--------------------------------------------------------------------------------
The MLFQ test verifies:
1. Basic Scheduling: CPU-bound vs I/O-bound process behavior
2. Priority Boost: Long-running processes get priority boost every 100 ticks
3. Fairness: Multiple CPU-bound processes get fair CPU time
4. Responsiveness: I/O-bound processes remain responsive

Expected Behavior:
- I/O-bound processes (I) should complete faster
- CPU-bound processes (C) should be demoted and finish slower
- Mixed processes (M) should have intermediate performance
- No process should be starved indefinitely

================================================================================
END OF REPORT
================================================================================
EOF

echo ""
echo "Report generated: $REPORT_FILE"
echo ""
cat "$REPORT_FILE"

