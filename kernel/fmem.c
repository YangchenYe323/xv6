#include "types.h"
#include "riscv.h"
#include "defs.h"

// Return the amount of free physical memory on the system
uint64 sys_fmem(void)
{ 
  return kfreemem();
}