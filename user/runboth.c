#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int pid1, pid2;
  
  printf("Starting both programs...\n");
  
  // Fork first child for CPU-bound
  pid1 = fork();
  if(pid1 < 0) {
    printf("fork failed\n");
    exit(1);
  }
  
  if(pid1 == 0) {
    // Child 1: run CPU-bound
    char *args[] = { "cpubound", 0 };
    exec("cpubound", args);
    printf("exec cpubound failed\n");
    exit(1);
  }
  
  // Fork second child for I/O-bound
  pid2 = fork();
  if(pid2 < 0) {
    printf("fork failed\n");
    exit(1);
  }
  
  if(pid2 == 0) {
    // Child 2: run I/O-bound
    char *args[] = { "iobound", 0 };
    exec("iobound", args);
    printf("exec iobound failed\n");
    exit(1);
  }
  
  // Parent: wait for both children to complete
  printf("Parent: waiting for both processes (PIDs: %d, %d)\n", pid1, pid2);
  wait(0);
  wait(0);
  
  printf("Both processes completed!\n");
  exit(0);
}