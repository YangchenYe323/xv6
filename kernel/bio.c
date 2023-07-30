// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

struct {
  struct spinlock lock;
  // Canonical buffer store
  struct buf buffers[NBUF];

  // A hash table of buckets where each bucket is a linked list
  // of pointers to buffer in buf
  struct spinlock bucket_locks[NBUCKET];
  struct buf hashtable[NBUCKET];
} bcache;

int hash(uint dev, uint blockno) {
  uint a = dev, b = blockno;
  int h = ((a + b) * (a + b + 1) / 2 + a) % NBUCKET;
  return h;
}

void insert_bucket(int bucket, struct buf *b) {
  struct buf *old_head = &bcache.hashtable[bucket];
  b->next = old_head;
  b->prev = old_head->prev;
  old_head->prev->next = b;
  old_head->prev = b;
}

void remove_from_bucket(struct buf *b) {
  if (b->prev && b->next) {
    b->prev->next = b->next;
    b->next->prev = b->prev;
    b->prev = 0;
    b->next = 0;
  }
}

void
binit(void)
{
  initlock(&bcache.lock, "bcache");
  for (int i = 0; i < NBUCKET; i++) {
    initlock(&bcache.bucket_locks[i], "bcache.bucket");
    bcache.hashtable[i].prev = &bcache.hashtable[i];
    bcache.hashtable[i].next = &bcache.hashtable[i];
  }

  for (int i = 0; i < NBUF; i++) {
    initsleeplock(&bcache.buffers[i].lock, "bcache.buffer");
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  int bucket = hash(dev, blockno);

  acquire(&bcache.bucket_locks[bucket]); 
  // Is the block already cached?
  for(b = bcache.hashtable[bucket].next; b != &(bcache.hashtable[bucket]); b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.bucket_locks[bucket]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&bcache.bucket_locks[bucket]);

  // Not cached. Find a free buffer from buf
  int bbucket = -1;
  acquire(&bcache.lock);
  struct buf *empty = 0;
  for(int i = 0; i < NBUF; i++){
    b = &bcache.buffers[i];
    if (b->dev == dev && b->blockno == blockno) {
      b->refcnt++;
      release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
    
    if (empty) continue;

    bbucket = hash(b->dev, b->blockno);
    acquire(&bcache.bucket_locks[bbucket]);

    if(b->refcnt == 0 && !empty) {
      empty = b;
    } else {
      release(&bcache.bucket_locks[bbucket]);
    }
  }

  if (!empty)
    panic("bget: no buffers");
  
  if (bbucket != bucket) {
    remove_from_bucket(empty);
  }

  empty->dev = dev;
  empty->blockno = blockno;
  empty->refcnt = 1;
  empty->valid = 0;

  release(&bcache.bucket_locks[bbucket]);
  if (bbucket != bucket) {
    acquire(&bcache.bucket_locks[bucket]);
    insert_bucket(bucket, empty);
    release(&bcache.bucket_locks[bucket]);
  }

  release(&bcache.lock);
  acquiresleep(&empty->lock);
  return empty;
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  int bucket = hash(b->dev, b->blockno);
  acquire(&bcache.bucket_locks[bucket]);
  b->refcnt--;
  release(&bcache.bucket_locks[bucket]);
}

void
bpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt++;
  release(&bcache.lock);
}

void
bunpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt--;
  release(&bcache.lock);
}


