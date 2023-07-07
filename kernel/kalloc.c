// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

// The xv6 has 128 * 1024 * 1024 bytes of phsical memory, which is
// 32768 physical pages. Each page needs 4 bits for reference counting 
// (xv6 supports at most 64 processes) (We give it one bytes for ease of implementation)
// So 32768 pages needs 32768 bytes of memory for reference counting,
// which is in turn 8 pages.
// The [end..end+RC_MEM] physical memory range is used for the reference counting
// array, and physical memory allocation starts at end+RC_MEM
// Note that it is almost certainly wasteful to give whole 8 pages for reference counting
// because kernel text areas and the ref counting areas themselves don't need reference counting
// anyway
#define RC_MEM 8 * PGSIZE
#define PA2IDX(pa) (((uint64) pa - KERNBASE) / PGSIZE)

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  // Clear the reference count memory region
  memset((void *)end, 0, RC_MEM);
  freerange(end + RC_MEM, (void*)PHYSTOP);
}

static void kfree_init(void *);

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE) {
    kfree_init(p);
  }
}

static void
kfree_init(void *pa) {
  struct run *r;
  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
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
  
  // Every pa is mapped in the kernel page table so the ref count is always >= 1,
  // only free the page when no process is referencing it.
  if ((uint64) pa > KERNBASE && knumreference(pa) != 1) {
    printf("free referenced page %p, %d, %d\n", pa, knumreference(pa), PA2IDX(pa));
    // panic("free referenced page %p, %d\n", pa, knumreference(pa));
    panic("kfree");
  }

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}

// Add a reference count to pa as it is mapped to another
// virtual address.
void
kreference(void *pa) {
  acquire(&kmem.lock);
  end[PA2IDX(pa)]++;
  release(&kmem.lock);
}

void kdereference(void *pa) {
  acquire(&kmem.lock);
  end[PA2IDX(pa)]--;
  release(&kmem.lock);
}

char knumreference(void *pa) {
  acquire(&kmem.lock);
  char ans = end[PA2IDX(pa)];
  release(&kmem.lock);
  return ans;
}