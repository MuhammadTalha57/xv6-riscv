# CEP Report: Multi-Level Feedback Queue (MLFQ) Scheduler Implementation

**Project**: Implementation of MLFQ Scheduler in xv6-riscv  
**Date**: November 2024  
**Status**: ✅ Complete and Verified

---

## Table of Contents

1. [Introduction](#introduction)
2. [Problem Statement](#problem-statement)
3. [Design and Implementation](#design-and-implementation)
4. [Code Changes](#code-changes)
5. [Testing](#testing)
6. [Results and Verification](#results-and-verification)
7. [Conclusion](#conclusion)
8. [Appendix](#appendix)

---

## 1. Introduction

This project implements a Multi-Level Feedback Queue (MLFQ) scheduler for the xv6-riscv operating system. The MLFQ scheduler improves upon the default round-robin scheduler by providing priority-based scheduling that adapts to process behavior, giving higher priority to I/O-bound processes while preventing CPU-bound processes from starving lower-priority tasks.

### Objectives

- Replace the round-robin scheduler with an MLFQ scheduler
- Implement priority-based process scheduling with 4 priority levels
- Support automatic process demotion based on CPU usage
- Implement priority boost mechanism to prevent starvation
- Maintain system responsiveness for interactive processes

---

## 2. Problem Statement

The original xv6 scheduler uses a simple round-robin approach where all processes receive equal CPU time regardless of their characteristics. This leads to:

1. **Poor responsiveness**: I/O-bound processes (like interactive applications) wait as long as CPU-bound processes
2. **Inefficient scheduling**: No differentiation between process types
3. **No priority management**: All processes treated equally

The MLFQ scheduler addresses these issues by:
- Assigning higher priority to I/O-bound processes
- Automatically demoting CPU-bound processes
- Providing periodic priority boosts to prevent starvation

---

## 3. Design and Implementation

### 3.1 MLFQ Architecture

The implementation uses **4 priority queues** (numbered 0-3, where 0 is highest priority):

- **Queue 0**: Highest priority, time allotment = 1 tick
- **Queue 1**: High priority, time allotment = 2 ticks
- **Queue 2**: Medium priority, time allotment = 4 ticks
- **Queue 3**: Lowest priority, time allotment = 8 ticks

### 3.2 Key Components

#### 3.2.1 Data Structures

**Process Structure Extensions** (`kernel/proc.h`):
```c
// MLFQ Fields
int queue_level;             // Current Queue (0 = Highest Priority)
int time_in_queue;           // Ticks used in current queue
struct proc* next;           // Next Process in queue (For Linked List)
```

**Queue Structure** (`kernel/proc.c`):
```c
#define NMLFQ 4                 // Number of Queues
#define BOOST_INTERVAL 100      // Priority boost every 100 ticks

int time_allotment[NMLFQ] = {1, 2, 4, 8};

struct {
  struct proc* head;
  struct proc* tail;
  struct spinlock lock;
} mlfq[NMLFQ];
```

#### 3.2.2 Core Functions

1. **`mlfq_init()`**: Initializes all queues and locks
2. **`mlfq_enqueue(p, queue)`**: Adds process to specified queue
3. **`mlfq_dequeue(queue)`**: Removes process from queue head
4. **`mlfq_remove(p)`**: Removes specific process from its queue
5. **`mlfq_tick()`**: Tracks time and handles priority boost
6. **`mlfq_boost()`**: Moves all processes to highest priority queue

### 3.3 Scheduling Algorithm

1. **Process Selection**: Scheduler checks queues from highest (0) to lowest (3) priority
2. **Time Tracking**: Each timer interrupt increments `time_in_queue` for running process
3. **Demotion**: When `time_in_queue >= time_allotment[queue_level]`, process is demoted
4. **Priority Boost**: Every 100 ticks, all processes are boosted to queue 0

### 3.4 Process Lifecycle Integration

- **Fork/Userinit**: New processes start at queue 0 (highest priority)
- **Exit**: Processes removed from queue before termination
- **Sleep**: Processes removed from queue when sleeping
- **Wakeup**: Processes re-enqueued at current priority level

---

## 4. Code Changes

### 4.1 Files Modified

#### `kernel/proc.h`
- Added 3 MLFQ fields to `struct proc` (lines 109-112)

#### `kernel/proc.c`
- MLFQ configuration and data structures (lines 9-25)
- Modified `scheduler()` function (lines 475-554)
- Modified `yield()` function with demotion logic (lines 585-609)
- Modified `allocproc()` to initialize MLFQ fields (lines 146-149)
- Modified `userinit()` to enqueue init process (lines 258-266)
- Modified `kfork()` to enqueue new processes (lines 339-347)
- Modified `kexit()` to remove process from queue (lines 403-406)
- Modified `sleep()` to remove process from queue (lines 660-663)
- Modified `wakeup()` to re-enqueue processes (lines 691-693)
- Implemented `mlfq_init()` (lines 868-878)
- Implemented `mlfq_enqueue()` (lines 880-899)
- Implemented `mlfq_dequeue()` (lines 901-917)
- Implemented `mlfq_remove()` (lines 919-945)
- Implemented `mlfq_tick()` (lines 804-825)
- Implemented `mlfq_boost()` (lines 827-866)

#### `kernel/defs.h`
- Added function declarations (lines 104-109)

#### `kernel/main.c`
- Added `mlfq_init()` call (line 31)

#### `kernel/trap.c`
- Integrated `mlfq_tick()` in `usertrap()` (line 85)
- Integrated `mlfq_tick()` in `kerneltrap()` (line 160)

#### `Makefile`
- Added `_mlfqtest` to UPROGS (line 146)

### 4.2 New Files

#### `user/mlfqtest.c`
- Comprehensive test program for MLFQ scheduler
- Tests basic scheduling, priority boost, fairness, and responsiveness

### 4.3 Summary of Changes

| Component | Changes |
|----------|---------|
| Data Structures | Added 3 fields to `struct proc`, 4 queue structures |
| Functions | 6 new MLFQ functions, 2 modified core functions |
| Integration Points | 5 process lifecycle functions modified |
| Initialization | 1 initialization function, 1 call site |
| Testing | 1 test program, test infrastructure |

---

## 5. Testing

### 5.1 Test Program: `mlfqtest`

The test program (`user/mlfqtest.c`) verifies:

1. **Basic Scheduling**: CPU-bound vs I/O-bound process behavior
2. **Priority Boost**: Long-running processes get priority boost every 100 ticks
3. **Fairness**: Multiple CPU-bound processes get fair CPU time
4. **Responsiveness**: I/O-bound processes remain responsive with CPU hogs

### 5.2 Test Infrastructure

Created automated testing tools:
- `test-mlfq.py`: Python script for automated testing
- `run-tests.sh`: Bash script for quick testing
- `generate-report.sh`: Report generation tool

### 5.3 Running Tests

```bash
# Automated testing
./run-tests.sh

# Or manually
make qemu
# In xv6 shell:
mlfqtest
```

### 5.4 Expected Behavior

- ✅ I/O-bound processes complete faster (stay at higher priority)
- ✅ CPU-bound processes are demoted (move to lower queues)
- ✅ No process starves (priority boost every 100 ticks)
- ✅ Fair CPU allocation among similar processes

---

## 6. Results and Verification

### 6.1 Requirements Verification

All CEP requirements have been satisfied:

| Requirement | Status | Details |
|------------|--------|---------|
| 1. MLFQ Fields in proc.h | ✅ | queue_level, time_in_queue, next |
| 2. MLFQ Configuration | ✅ | 4 queues, time allotments, locks |
| 3. mlfq_init() & Call | ✅ | Implemented, declared, called |
| 4. Queue Operations | ✅ | enqueue, dequeue, remove |
| 5. Scheduler Modification | ✅ | Priority-based selection |
| 6. Timer Interrupt Handling | ✅ | Time tracking, demotion, boost |
| 7. Priority Boost | ✅ | Every 100 ticks |
| 8. Process Lifecycle | ✅ | Fork, exit, sleep, wakeup |
| 9. Function Declarations | ✅ | All in defs.h |
| 10. Build System | ✅ | mlfqtest in Makefile |

### 6.2 Build Status

✅ **Kernel compiles successfully** without errors or warnings

### 6.3 Code Quality

- ✅ Proper locking (no race conditions)
- ✅ Lock ordering to prevent deadlocks
- ✅ Error handling
- ✅ Code comments and documentation

### 6.4 Test Results

- ✅ Test program executes successfully
- ✅ All test phases complete
- ✅ No panics or errors observed
- ✅ Expected behavior verified

---

## 7. Conclusion

### 7.1 Achievements

1. **Successfully implemented MLFQ scheduler** with 4 priority levels
2. **Integrated with xv6 process lifecycle** (fork, exit, sleep, wakeup)
3. **Implemented priority boost mechanism** to prevent starvation
4. **Created comprehensive test suite** for verification
5. **All CEP requirements satisfied**

### 7.2 Key Features

- **Priority-based scheduling**: Processes start at highest priority
- **Automatic demotion**: CPU-bound processes move to lower queues
- **Time-based allocation**: Different time allotments per queue
- **Starvation prevention**: Periodic priority boost every 100 ticks
- **I/O optimization**: I/O-bound processes maintain higher priority

### 7.3 Performance Characteristics

- **Time Complexity**: O(n) for scheduler (n = number of queues)
- **Space Complexity**: O(1) per process (3 additional fields)
- **Lock Contention**: Minimal (per-queue locks)

### 7.4 Future Enhancements (Optional)

1. Configurable time allotments and boost interval
2. Per-CPU queues for multi-core systems
3. Statistics tracking (queue distribution, wait times)
4. Debug output for queue states

---

## 8. Appendix

### 8.1 File Locations

**Core Implementation**:
- `kernel/proc.h`: Process structure with MLFQ fields
- `kernel/proc.c`: MLFQ implementation (all functions)
- `kernel/defs.h`: Function declarations
- `kernel/main.c`: Initialization call
- `kernel/trap.c`: Timer interrupt integration

**Testing**:
- `user/mlfqtest.c`: Test program
- `test-mlfq.py`: Automated test script
- `run-tests.sh`: Test runner
- `TESTING.md`: Testing documentation

**Documentation**:
- `CEP_VERIFICATION.md`: Detailed requirement verification
- `MLFQ_IMPLEMENTATION_SUMMARY.md`: Implementation overview
- `REQUIREMENTS_CHECKLIST.md`: Quick checklist
- `CEP_REPORT.md`: This document

### 8.2 Key Code Snippets

#### Scheduler Selection Logic
```c
// Iterate through queues from highest to lowest priority
for(int q = 0; q < NMLFQ; q++) {
  acquire(&mlfq[q].lock);
  if(mlfq[q].head != 0) {
    // Find first RUNNABLE process
    // Remove from queue and schedule
  }
}
```

#### Demotion Logic
```c
if(p->time_in_queue >= time_allotment[p->queue_level]) {
  if(p->queue_level < NMLFQ - 1) {
    p->queue_level++;  // Demote to lower queue
  }
  p->time_in_queue = 0;
}
```

#### Priority Boost
```c
if(time_since_boost >= BOOST_INTERVAL) {
  time_since_boost = 0;
  mlfq_boost();  // Move all to queue 0
}
```

### 8.3 Testing Output Example

```
========================================
   MLFQ Scheduler Verification Tests
========================================

=== Test 1: Basic MLFQ Scheduling ===
Creating processes with different characteristics...
[Process execution with C/I/M indicators]

=== Test 2: Priority Boost Test ===
[Long-running process with periodic boost]

=== Test 3: Fairness Test ===
[Multiple CPU-bound processes]

=== Test 4: Responsiveness Test ===
[I/O-bound process with CPU hog]

========================================
     All MLFQ Tests Completed!
========================================
```

### 8.4 References

- xv6-riscv source code
- Operating Systems: Three Easy Pieces (MLFQ chapter)
- Original MLFQ paper by Corbato et al.

---

## Submission Checklist

- [x] All code changes implemented
- [x] All requirements satisfied
- [x] Code compiles without errors
- [x] Test program created and working
- [x] Documentation complete
- [x] Verification report generated


