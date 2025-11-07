#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  struct procinfo info;
  int pid;
  
  if(argc < 2) {
    // If no PID provided, get info about current process
    pid = getpid();
    printf("Getting info for current process (PID: %d)\n", pid);
  } else {
    pid = atoi(argv[1]);
    printf("Getting info for process PID: %d\n", pid);
  }
  
  if(getprocinfo(pid, &info) < 0) {
    printf("Error: Process with PID %d not found\n", pid);
    exit(1);
  }
  
  printf("\n=== Process Performance Information ===\n");
  printf("PID:            %d\n", info.pid);
  printf("Name:           %s\n", info.name);
  printf("CPU Ticks:      %ld\n", info.cpu_ticks);
  printf("Scheduled:      %ld times\n", info.sched_count);
  printf("=======================================\n\n");
  
  exit(0);
}
