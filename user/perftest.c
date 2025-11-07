#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// Compute-intensive function to consume CPU
void
work(int iterations)
{
  int sum = 0;
  for(int i = 0; i < iterations; i++) {
    sum += i * i;
    if(i % 10000 == 0) {
      // Yield occasionally to let other processes run
      pause(1);
    }
  }
}

int
main(int argc, char *argv[])
{
  struct procinfo info_before, info_after;
  int pid = getpid();
  int iterations = 100000;
  
  if(argc > 1) {
    iterations = atoi(argv[1]);
  }
  
  printf("Performance Analysis Test\n");
  printf("=========================\n");
  printf("Process PID: %d\n", pid);
  printf("Work iterations: %d\n\n", iterations);
  
  // Get initial performance metrics
  if(getprocinfo(pid, &info_before) < 0) {
    printf("Error: Failed to get initial process info\n");
    exit(1);
  }
  
  printf("BEFORE work:\n");
  printf("  CPU Ticks:     %ld\n", info_before.cpu_ticks);
  printf("  Scheduled:     %ld times\n\n", info_before.sched_count);
  
  // Do some CPU-intensive work
  printf("Doing work...\n");
  work(iterations);
  
  // Get performance metrics after work
  if(getprocinfo(pid, &info_after) < 0) {
    printf("Error: Failed to get final process info\n");
    exit(1);
  }
  
  printf("\nAFTER work:\n");
  printf("  CPU Ticks:     %ld\n", info_after.cpu_ticks);
  printf("  Scheduled:     %ld times\n\n", info_after.sched_count);
  
  printf("DIFFERENCE:\n");
  printf("  CPU Ticks:     +%ld\n", info_after.cpu_ticks - info_before.cpu_ticks);
  printf("  Scheduled:     +%ld times\n", info_after.sched_count - info_before.sched_count);
  printf("=========================\n");
  
  exit(0);
}
