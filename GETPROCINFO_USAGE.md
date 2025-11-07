# Using the getprocinfo System Call

## Overview
The `getprocinfo(int pid, struct procinfo *info)` system call retrieves performance metrics for a specific process in xv6.

## System Call Signature
```c
int getprocinfo(int pid, struct procinfo *info);
```

**Parameters:**
- `pid`: Process ID of the target process
- `info`: Pointer to a `struct procinfo` where the information will be stored

**Return Value:**
- `0` on success
- `-1` if the process with the given PID is not found

## Data Structure
```c
struct procinfo {
  int pid;                     // Process ID
  uint64 cpu_ticks;            // Total CPU ticks consumed
  uint64 sched_count;          // Number of times scheduled
  char name[16];               // Process name
};
```

## Usage Examples

### 1. Using the procinfo test program (Simple)

In the xv6 shell, run:

```bash
$ procinfo
```

This will show performance metrics for the current process.

Or specify a PID:

```bash
$ procinfo 3
```

This will show metrics for process with PID 3.

### 2. Using the perftest program (Performance Analysis)

Run the performance analysis test:

```bash
$ perftest
```

This program:
- Gets initial performance metrics
- Performs CPU-intensive work
- Gets final performance metrics
- Shows the difference (CPU ticks and schedule count)

You can specify the number of iterations:

```bash
$ perftest 200000
```

### 3. Using in your own C program

Here's a complete example:

```c
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(void) {
    struct procinfo info;
    int pid = getpid();
    
    // Get info for current process
    if(getprocinfo(pid, &info) < 0) {
        printf("Error: Failed to get process info\n");
        exit(1);
    }
    
    // Display the information
    printf("Process Info:\n");
    printf("  PID:           %d\n", info.pid);
    printf("  Name:          %s\n", info.name);
    printf("  CPU Ticks:     %ld\n", info.cpu_ticks);
    printf("  Scheduled:     %ld times\n", info.sched_count);
    
    exit(0);
}
```

### 4. Monitoring another process

To monitor another process (like the shell), first find its PID:

```bash
$ ps              # If you have a ps command
```

Or run a program in background and get its info:

```c
int main(void) {
    int pid = fork();
    
    if(pid == 0) {
        // Child process - do some work
        for(int i = 0; i < 1000000; i++) {
            // CPU-intensive work
        }
        exit(0);
    } else {
        // Parent process - monitor child
        struct procinfo info;
        
        pause(10);  // Let child run for a bit
        
        if(getprocinfo(pid, &info) == 0) {
            printf("Child process (PID %d):\n", pid);
            printf("  CPU Ticks:  %ld\n", info.cpu_ticks);
            printf("  Scheduled:  %ld times\n", info.sched_count);
        }
        
        wait(0);  // Wait for child to finish
    }
    exit(0);
}
```

## Testing in xv6

1. **Start xv6:**
   ```bash
   $ make qemu
   ```

2. **Test the basic procinfo command:**
   ```bash
   $ procinfo
   Getting info for current process (PID: 3)
   
   === Process Performance Information ===
   PID:            3
   Name:           procinfo
   CPU Ticks:      5
   Scheduled:      2 times
   =======================================
   ```

3. **Test performance analysis:**
   ```bash
   $ perftest
   Performance Analysis Test
   =========================
   Process PID: 3
   Work iterations: 100000
   
   BEFORE work:
     CPU Ticks:     2
     Scheduled:     1 times
   
   Doing work...
   
   AFTER work:
     CPU Ticks:     145
     Scheduled:     8 times
   
   DIFFERENCE:
     CPU Ticks:     +143
     Scheduled:     +7 times
   =========================
   ```

4. **Monitor the init process (PID 1):**
   ```bash
   $ procinfo 1
   Getting info for process PID: 1
   
   === Process Performance Information ===
   PID:            1
   Name:           init
   CPU Ticks:      12
   Scheduled:      5 times
   =======================================
   ```

## Use Cases

1. **Performance Profiling**: Measure how much CPU time a process consumes
2. **Scheduler Analysis**: Track how often a process is scheduled
3. **Process Monitoring**: Get real-time performance metrics
4. **Debugging**: Understand process behavior and resource usage
5. **Optimization**: Identify CPU-intensive processes

## Notes

- CPU ticks are incremented on each timer interrupt when the process is running
- Schedule count tracks how many times the process has been selected by the scheduler
- The MLFQ scheduler in xv6 affects how processes are scheduled
- Performance metrics are tracked from process creation
