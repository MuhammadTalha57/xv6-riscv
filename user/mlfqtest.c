//
// MLFQ Scheduler Verification Test
// Tests various aspects of Multi-Level Feedback Queue scheduling
//

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define NUM_CHILDREN 5
#define CPU_BOUND_ITERS 100000
#define IO_BOUND_ITERS 50

void
print(const char *s)
{
  write(1, s, strlen(s));
}

void
print_num(const char *prefix, int num)
{
  char buf[64];
  char *p = buf;
  
  // Copy prefix
  const char *s = prefix;
  while(*s)
    *p++ = *s++;
  
  // Convert number to string
  if(num == 0) {
    *p++ = '0';
  } else {
    char tmp[20];
    int i = 0;
    int n = num;
    if(n < 0) {
      *p++ = '-';
      n = -n;
    }
    while(n > 0) {
      tmp[i++] = '0' + (n % 10);
      n /= 10;
    }
    while(i > 0)
      *p++ = tmp[--i];
  }
  *p++ = '\n';
  
  write(1, buf, p - buf);
}

// CPU-bound process - should be demoted to lower priority queues
void
cpu_bound_task(int id)
{
  print_num("CPU-bound process started, PID: ", getpid());
  
  volatile long sum = 0;
  for(int i = 0; i < CPU_BOUND_ITERS; i++) {
    for(int j = 0; j < 100; j++) {
      sum += i * j;
    }
    
    // Periodically report progress
    if(i % 10000 == 0) {
      write(1, "C", 1);  // C for CPU-bound
    }
  }
  
  print_num("CPU-bound process finished, PID: ", getpid());
  exit(0);
}

// I/O-bound process - should stay at higher priority
void
io_bound_task(int id)
{
  print_num("I/O-bound process started, PID: ", getpid());
  
  for(int i = 0; i < IO_BOUND_ITERS; i++) {
    // Simulate I/O by sleeping
    pause(1);  // Small pause to simulate I/O wait
    write(1, "I", 1);  // I for I/O-bound
  }
  
  print_num("I/O-bound process finished, PID: ", getpid());
  exit(0);
}

// Mixed workload - alternates between CPU and I/O
void
mixed_task(int id)
{
  print_num("Mixed process started, PID: ", getpid());
  
  for(int i = 0; i < 20; i++) {
    // CPU burst
    volatile long sum = 0;
    for(int j = 0; j < 5000; j++) {
      sum += i * j;
    }
    
    // I/O operation
    pause(1);
    write(1, "M", 1);  // M for Mixed
  }
  
  print_num("Mixed process finished, PID: ", getpid());
  exit(0);
}

// Test basic MLFQ functionality
void
test_basic_scheduling(void)
{
  print("\n=== Test 1: Basic MLFQ Scheduling ===\n");
  print("Creating processes with different characteristics...\n\n");
  
  int count = 0;
  
  // Create 2 CPU-bound processes
  for(int i = 0; i < 2; i++) {
    int pid = fork();
    if(pid < 0) {
      print("Fork failed!\n");
      exit(1);
    }
    if(pid == 0) {
      cpu_bound_task(i);
    }
    count++;
  }
  
  // Create 2 I/O-bound processes
  for(int i = 0; i < 2; i++) {
    int pid = fork();
    if(pid < 0) {
      print("Fork failed!\n");
      exit(1);
    }
    if(pid == 0) {
      io_bound_task(i);
    }
    count++;
  }
  
  // Create 1 mixed process
  int pid = fork();
  if(pid < 0) {
    print("Fork failed!\n");
    exit(1);
  }
  if(pid == 0) {
    mixed_task(0);
  }
  count++;
  
  print("\nAll processes created. Waiting for completion...\n");
  print("Legend: C=CPU-bound, I=I/O-bound, M=Mixed\n\n");
  
  // Wait for all children
  for(int i = 0; i < count; i++) {
    wait(0);
  }
  
  print("\n\nTest 1 Complete: All processes finished.\n");
  print("Expected behavior:\n");
  print("- I/O-bound processes (I) should complete faster\n");
  print("- CPU-bound processes (C) should be demoted and finish slower\n");
  print("- Mixed processes (M) should have intermediate performance\n");
}

// Test priority boost mechanism
void
test_priority_boost(void)
{
  print("\n=== Test 2: Priority Boost Test ===\n");
  print("Running long CPU-bound process to test priority boost...\n\n");
  
  int pid = fork();
  if(pid < 0) {
    print("Fork failed!\n");
    exit(1);
  }
  
  if(pid == 0) {
    // Long-running CPU-bound process
    volatile long sum = 0;
    for(int i = 0; i < 200000; i++) {
      sum += i;
      if(i % 20000 == 0) {
        write(1, ".", 1);
      }
    }
    print("\nLong CPU-bound process finished\n");
    exit(0);
  }
  
  wait(0);
  print("\nTest 2 Complete: Process should have experienced priority boost\n");
  print("Expected: Priority boost every ~100 ticks prevents starvation\n");
}

// Test fairness among similar processes
void
test_fairness(void)
{
  print("\n=== Test 3: Fairness Test ===\n");
  print("Creating multiple CPU-bound processes...\n\n");
  
  int num_procs = 4;
  
  for(int i = 0; i < num_procs; i++) {
    int pid = fork();
    if(pid < 0) {
      print("Fork failed!\n");
      exit(1);
    }
    
    if(pid == 0) {
      print_num("Process started: ", getpid());
      volatile long sum = 0;
      for(int j = 0; j < 50000; j++) {
        sum += j;
        if(j % 10000 == 0) {
          char c = '0' + i;
          write(1, &c, 1);
        }
      }
      print_num("Process finished: ", getpid());
      exit(0);
    }
  }
  
  print("\nWaiting for all processes to complete...\n");
  for(int i = 0; i < num_procs; i++) {
    wait(0);
  }
  
  print("\nTest 3 Complete: All processes finished\n");
  print("Expected: All CPU-bound processes should get fair CPU time\n");
}

// Test responsiveness with interactive-like processes
void
test_responsiveness(void)
{
  print("\n=== Test 4: Responsiveness Test ===\n");
  print("Testing system responsiveness with mixed workloads...\n\n");
  
  // Create one CPU hog
  int pid1 = fork();
  if(pid1 == 0) {
    volatile long sum = 0;
    for(int i = 0; i < 100000; i++) {
      sum += i;
    }
    print("CPU hog finished\n");
    exit(0);
  }
  
  // Create interactive-like process that should remain responsive
  int pid2 = fork();
  if(pid2 == 0) {
    for(int i = 0; i < 30; i++) {
      pause(2);  // Simulate user input wait
      write(1, "R", 1);  // R for Responsive
    }
    print("\nResponsive process finished\n");
    exit(0);
  }
  
  wait(0);
  wait(0);
  
  print("\nTest 4 Complete\n");
  print("Expected: Responsive process (R) should not be starved by CPU hog\n");
}

int
main(int argc, char *argv[])
{
  print("\n");
  print("========================================\n");
  print("   MLFQ Scheduler Verification Tests   \n");
  print("========================================\n");
  
  test_basic_scheduling();
  
  print("\n--- Pausing between tests ---\n");
  pause(10);
  
  test_priority_boost();
  
  print("\n--- Pausing between tests ---\n");
  pause(10);
  
  test_fairness();
  
  print("\n--- Pausing between tests ---\n");
  pause(10);
  
  test_responsiveness();
  
  print("\n");
  print("========================================\n");
  print("     All MLFQ Tests Completed!         \n");
  print("========================================\n");
  print("\nMLFQ appears to be working correctly if:\n");
  print("1. I/O-bound processes responded quickly\n");
  print("2. CPU-bound processes were demoted\n");
  print("3. No process was starved\n");
  print("4. Fair CPU allocation among similar processes\n");
  print("\n");
  
  exit(0);
}
