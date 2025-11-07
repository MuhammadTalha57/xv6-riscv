#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int pid_cpu, pid_io;
  struct procinfo info_cpu, info_io;
  
  printf("\n========================================\n");
  printf("  CPU vs I/O Performance Comparison\n");
  printf("========================================\n\n");
  
  // Fork CPU-bound process
  pid_cpu = fork();
  if(pid_cpu == 0) {
    // Child: Run CPU-bound work
    exec("cpubound", argv);
    printf("exec cpubound failed\n");
    exit(1);
  }
  
  // Fork I/O-bound process
  pid_io = fork();
  if(pid_io == 0) {
    // Child: Run I/O-bound work
    exec("iobound", argv);
    printf("exec iobound failed\n");
    exit(1);
  }
  
  // Parent: Wait for both to complete
  wait(0);
  wait(0);
  
  // Give processes time to fully complete
  pause(5);
  
  // Now compare their metrics
  printf("\n========================================\n");
  printf("  PERFORMANCE COMPARISON\n");
  printf("========================================\n\n");
  
  if(getprocinfo(pid_cpu, &info_cpu) == 0) {
    printf("CPU-Bound Process (PID %d):\n", pid_cpu);
    printf("  Name:           %s\n", info_cpu.name);
    printf("  CPU Ticks:      %ld\n", info_cpu.cpu_ticks);
    printf("  Scheduled:      %ld times\n", info_cpu.sched_count);
    printf("  Ticks/Schedule: %ld\n\n", 
           info_cpu.cpu_ticks / (info_cpu.sched_count > 0 ? info_cpu.sched_count : 1));
  } else {
    printf("CPU-Bound Process: Already terminated\n\n");
  }
  
  if(getprocinfo(pid_io, &info_io) == 0) {
    printf("I/O-Bound Process (PID %d):\n", pid_io);
    printf("  Name:           %s\n", info_io.name);
    printf("  CPU Ticks:      %ld\n", info_io.cpu_ticks);
    printf("  Scheduled:      %ld times\n", info_io.sched_count);
    printf("  Ticks/Schedule: %ld\n\n", 
           info_io.cpu_ticks / (info_io.sched_count > 0 ? info_io.sched_count : 1));
  } else {
    printf("I/O-Bound Process: Already terminated\n\n");
  }
  
  printf("========================================\n");
  printf("Analysis:\n");
  printf("- CPU-bound: High CPU ticks, fewer schedules\n");
  printf("- I/O-bound: Lower CPU ticks, more schedules\n");
  printf("========================================\n\n");
  
  exit(0);
}
