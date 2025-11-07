#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int i, j;
  int sum = 0;
  int pid = getpid();
  struct procinfo info_before, info_after;
  
  printf("CPU-bound process started (PID: %d)\n", pid);
  
  // Get initial performance metrics
  if(getprocinfo(pid, &info_before) < 0) {
    printf("Error: Failed to get initial process info\n");
    exit(1);
  }
  
  printf("\n=== INITIAL METRICS ===\n");
  printf("CPU Ticks:     %ld\n", info_before.cpu_ticks);
  printf("Scheduled:     %ld times\n\n", info_before.sched_count);
  
  // Perform intensive computation
  for(i = 0; i < 100000; i++) {
    for(j = 0; j < 1000; j++) {
      sum += i * j;
    }
    // Print progress occasionally
    if(i % 10000 == 0) {
      printf("CPU[%d]: iteration %d, sum=%d\n", pid, i, sum);
    }
  }
  
  // Get final performance metrics
  if(getprocinfo(pid, &info_after) < 0) {
    printf("Error: Failed to get final process info\n");
    exit(1);
  }
  
  printf("\nCPU-bound process finished (PID: %d), final sum=%d\n", pid, sum);
  
  printf("\n=== FINAL METRICS ===\n");
  printf("CPU Ticks:     %ld\n", info_after.cpu_ticks);
  printf("Scheduled:     %ld times\n\n", info_after.sched_count);
  
  printf("=== PERFORMANCE ANALYSIS ===\n");
  printf("CPU Ticks Used:      %ld\n", info_after.cpu_ticks - info_before.cpu_ticks);
  printf("Times Scheduled:     %ld\n", info_after.sched_count - info_before.sched_count);
  printf("Avg Ticks/Schedule:  %ld\n", 
         (info_after.cpu_ticks - info_before.cpu_ticks) / 
         (info_after.sched_count - info_before.sched_count > 0 ? 
          info_after.sched_count - info_before.sched_count : 1));
  printf("============================\n\n");
  
  exit(0);
}