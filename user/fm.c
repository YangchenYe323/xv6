// User program that calls fmem and print available physical
// memory (bytes) on the system

#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
  printf("Free memory on system is %d bytes\n", fmem());
  return 0;
}
