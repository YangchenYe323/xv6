// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  // per-cpu spinlock
  // spinlock is needed for memory stealing
  struct spinlock lock[NCPU];
  // per-cpu freelist
  struct run *freelist[NCPU];
} kmem;

void
kinit()
{
  for (int i = 0; i < NCPU; i++) {
    initlock(&kmem.lock[i], "kmem");
  }
  
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  push_off();
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
  pop_off();
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  push_off();

  int cpu = cpuid();

  r = (struct run*)pa;

  acquire(&kmem.lock[cpu]);
  r->next = kmem.freelist[cpu];
  kmem.freelist[cpu] = r;
  release(&kmem.lock[cpu]);

  pop_off();
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  push_off();
  int mycpu = cpuid();

  acquire(&kmem.lock[mycpu]);
  r = kmem.freelist[mycpu];
  if(r) {
    kmem.freelist[mycpu] = r->next;
    release(&kmem.lock[mycpu]);
    memset((char*)r, 5, PGSIZE);
    goto ret;
  }
  release(&kmem.lock[mycpu]);

  // steal memory from other cpu
  for (int i = 0; i < NCPU; i++) {
    if (i != mycpu) {
      acquire(&kmem.lock[i]);
      r = kmem.freelist[i];
      if (r) {
        kmem.freelist[i] = r->next;
      }
      release(&kmem.lock[i]);
    }

    if (r) {
      memset((char*)r, 5, PGSIZE);
      goto ret;
    }
  }

ret:
  pop_off();
  return (void*)r;
}
