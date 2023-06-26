#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "proc.h"
#include "sysinfo.h"

int sysinfo(struct sysinfo *info) {
  // Calculate Free Memory
  info->freemem = free_physical_memory();
  // Calculate Number of processes
  info->nproc = num_procs();
  return 0;
}

uint64 sys_sysinfo(void) {
  struct proc *p = myproc();

  uint64 info_user;
  argaddr(0, &info_user);

  uint64 fmem = free_physical_memory();
  if (copyout(p->pagetable, (uint64)(&((struct sysinfo *) info_user)->freemem), (char *) &fmem, sizeof(uint64)) < 0) {
    return -1;
  }

  uint64 nproc = num_procs();
  if (copyout(p->pagetable, (uint64)(&((struct sysinfo *) info_user)->nproc), (char *) &nproc, sizeof(uint64)) < 0) {
    return -1;
  }

  return 0;
}