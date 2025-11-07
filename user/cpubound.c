#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int i, j;
  int sum = 0;
  int pid = getpid();
  
  printf("CPU-bound process started (PID: %d)\n", pid);
  
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
  
  printf("CPU-bound process finished (PID: %d), final sum=%d\n", pid, sum);
  exit(0);
}