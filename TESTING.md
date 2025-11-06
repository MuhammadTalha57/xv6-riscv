# MLFQ Scheduler Testing Guide

This document describes how to test the MLFQ (Multi-Level Feedback Queue) scheduler implementation in xv6.

## Quick Start

### Option 1: Automated Test Script
```bash
./run-tests.sh
```

This will:
1. Clean and build xv6
2. Run MLFQ tests
3. Generate a report in `mlfq_test_report.txt`

### Option 2: Python Test Script
```bash
# Run only MLFQ test
python3 test-mlfq.py --mlfq-only

# Run all tests (including basic system tests)
python3 test-mlfq.py

# Specify custom report file
python3 test-mlfq.py --report my_report.txt
```

### Option 3: Manual Testing
```bash
# Build xv6
make clean
make kernel/kernel fs.img

# Run QEMU
make qemu

# In xv6 shell, run:
mlfqtest
```

## Test Components

### 1. MLFQ Test (`mlfqtest`)
The main test program that verifies:
- **Basic Scheduling**: CPU-bound vs I/O-bound process behavior
- **Priority Boost**: Long-running processes get priority boost every 100 ticks
- **Fairness**: Multiple CPU-bound processes get fair CPU time
- **Responsiveness**: I/O-bound processes remain responsive even with CPU hogs

### 2. Basic System Tests
- `echo`: Basic command execution
- `cat`: File reading
- `ls`: Directory listing
- `mkdir`: Directory creation

### 3. Comprehensive User Tests
- `usertests`: Full xv6 user test suite (takes 5-10 minutes)

## Expected Behavior

### MLFQ Characteristics
1. **4 Priority Queues**: Highest (0) to Lowest (3)
2. **Time Allotments**: 1, 2, 4, 8 ticks per queue level
3. **Demotion**: Processes exceeding time allotment move to lower queue
4. **Priority Boost**: All processes boosted to queue 0 every 100 ticks
5. **I/O-Bound Priority**: Processes that yield (I/O) stay at higher priority

### Test Output
When `mlfqtest` runs successfully, you should see:
- Process creation messages
- Progress indicators (C=CPU-bound, I=I/O-bound, M=Mixed)
- Completion messages for each test phase
- Final summary: "All MLFQ Tests Completed!"

## Troubleshooting

### Build Issues
```bash
# Clean and rebuild
make clean
make kernel/kernel fs.img
```

### Test Timeout
If tests timeout, increase timeout in `test-mlfq.py`:
- Look for `timeout = 120` and increase if needed

### QEMU Not Found
```bash
# Check QEMU version (needs >= 7.2)
qemu-system-riscv64 --version

# Install if needed (Ubuntu/Debian)
sudo apt-get install qemu-system-riscv64
```

### Test Hangs
- Check if QEMU is running: `ps aux | grep qemu`
- Kill stuck processes: `pkill -9 qemu`
- Restart tests

## Report Format

The generated report includes:
- **Summary**: Total tests, passed/failed/errors
- **Success Rate**: Percentage of passing tests
- **Detailed Results**: Status, duration, output for each test
- **Timestamps**: When tests were run

## Manual Verification

To manually verify MLFQ is working:

1. **Run mlfqtest** and observe:
   - I/O-bound processes (I) should complete faster
   - CPU-bound processes (C) should be demoted and finish slower
   - Mixed processes (M) should have intermediate performance

2. **Check for priority boost**:
   - Long-running processes should periodically get priority boost
   - No process should be starved indefinitely

3. **Verify fairness**:
   - Multiple similar processes should get roughly equal CPU time

## Files

- `test-mlfq.py`: Python test script with QEMU automation
- `run-tests.sh`: Bash script for automated testing
- `user/mlfqtest.c`: MLFQ test program source
- `mlfq_test_report.txt`: Generated test report

## Next Steps

After successful testing:
1. Review the test report
2. Check for any warnings or failures
3. Verify MLFQ behavior matches expectations
4. Document any issues found

