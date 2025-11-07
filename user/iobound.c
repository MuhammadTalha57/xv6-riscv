#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

int
main(int argc, char *argv[])
{
  int i;
  int fd;
  char buf[512];
  int pid = getpid();
  struct procinfo info_before, info_after;
  
  printf("I/O-bound process started (PID: %d)\n", pid);
  
  // Get initial performance metrics
  if(getprocinfo(pid, &info_before) < 0) {
    printf("Error: Failed to get initial process info\n");
    exit(1);
  }
  
  printf("\n=== INITIAL METRICS ===\n");
  printf("CPU Ticks:     %ld\n", info_before.cpu_ticks);
  printf("Scheduled:     %ld times\n\n", info_before.sched_count);
  
  // Create a test file
  fd = open("iotest.txt", O_CREATE | O_WRONLY);
  if(fd < 0) {
    printf("Error: cannot create file\n");
    exit(1);
  }
  
  // Perform many I/O operations
  // Max file size in xv6: 268 blocks * 1024 bytes = ~274KB
  // Using 512-byte writes: 274432 / 512 = ~536 blocks max
  // Use 500 iterations to stay well within limit
  for(i = 0; i < 500; i++) {
    // Fill buffer with data
    memset(buf, 'A' + (i % 26), sizeof(buf));
    
    // Write to file (I/O operation)
    if(write(fd, buf, sizeof(buf)) != sizeof(buf)) {
      printf("Error: write failed\n");
      close(fd);
      exit(1);
    }
    
    // Print progress occasionally
    if(i % 100 == 0) {
      printf("I/O[%d]: wrote block %d\n", pid, i);
    }
  }
  
  close(fd);
  
  // Now read the file back
  fd = open("iotest.txt", O_RDONLY);
  if(fd < 0) {
    printf("Error: cannot open file for reading\n");
    exit(1);
  }
  
  for(i = 0; i < 500; i++) {
    if(read(fd, buf, sizeof(buf)) != sizeof(buf)) {
      printf("Error: read failed\n");
      close(fd);
      exit(1);
    }
    
    if(i % 100 == 0) {
      printf("I/O[%d]: read block %d\n", pid, i);
    }
  }
  
  close(fd);
  unlink("iotest.txt");
  
  // Get final performance metrics
  if(getprocinfo(pid, &info_after) < 0) {
    printf("Error: Failed to get final process info\n");
    exit(1);
  }
  
  printf("\nI/O-bound process finished (PID: %d)\n", pid);
  
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