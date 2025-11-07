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
  
  printf("I/O-bound process started (PID: %d)\n", pid);
  
  // Create a test file
  fd = open("iotest.txt", O_CREATE | O_WRONLY);
  if(fd < 0) {
    printf("Error: cannot create file\n");
    exit(1);
  }
  
  // Perform many I/O operations
  for(i = 0; i < 1000; i++) {
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
  
  for(i = 0; i < 1000; i++) {
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
  
  printf("I/O-bound process finished (PID: %d)\n", pid);
  exit(0);
}