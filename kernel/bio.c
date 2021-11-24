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

#define NBUCKETS 7
#define NULL 0

struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head;

  // struct buf buckets[NBUCKETS];// buckets

  // struct spinlock bucket_locks[NBUCKETS];// to control concurrent access on the same bucket.

} bcache[NBUCKETS];

uint
index(uint blockno)
{
   return blockno % NBUCKETS;
}

//// single init
// void
// binit(void)
// {
//   struct buf *b;

//   initlock(&bcache.lock, "bcache");

//   // Create linked list of buffers
//   bcache.head.prev = &bcache.head;
//   bcache.head.next = &bcache.head;
//   for(b = bcache.buf; b < bcache.buf+NBUF; b++){
//     b->next = bcache.head.next;
//     b->prev = &bcache.head;
//     initsleeplock(&b->lock, "buffer");
//     bcache.head.next->prev = b;
//     bcache.head.next = b;
//   }
// }

//// array init
void
binit(void)
{
  struct buf *b = NULL;

  for(int i=0; i<NBUCKETS; i++)
  {
    initlock(&bcache[i].lock,"bcache[i]");
    bcache[i].head.prev = &bcache[i].head;
    bcache[i].head.next = &bcache[i].head;

    for(b=bcache[i].buf; b<bcache[i].buf+NBUF; b++)
    {
      b->next = bcache[i].head.next;
      b->prev = &bcache[i].head;
      initsleeplock(&b->lock,"buffer");
      bcache[i].head.next->prev = b;
      bcache[i].head.next = b;
    }
  }
}

//// single bget
// // Look through buffer cache for block on device dev.
// // If not found, allocate a buffer.
// // In either case, return locked buffer.
// static struct buf*
// bget(uint dev, uint blockno)
// {
//   struct buf *b;

//   acquire(&bcache.lock);

//   // Is the block already cached?
//   for(b = bcache.head.next; b != &bcache.head; b = b->next){
//     if(b->dev == dev && b->blockno == blockno){
//       b->refcnt++;
//       release(&bcache.lock);
//       acquiresleep(&b->lock);
//       return b;
//     }
//   }

//   // Not cached.
//   // Recycle the least recently used (LRU) unused buffer.
//   for(b = bcache.head.prev; b != &bcache.head; b = b->prev){
//     if(b->refcnt == 0) {
//       b->dev = dev;
//       b->blockno = blockno;
//       b->valid = 0;
//       b->refcnt = 1;
//       release(&bcache.lock);
//       acquiresleep(&b->lock);
//       return b;
//     }
//   }
//   panic("bget: no buffers");
// }

//// for-array bget
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b = NULL;
  struct buf *min_b = 0;// keep last recent buf
  
  int i =  index(blockno);
  uint min_ts = -1;// keep min ts
  //! min_ts should not be 0, becease the ticks at idle is 0, so that idle can not bget.

  acquire(&bcache[i].lock);

  for(b=bcache[i].buf; b<bcache[i].buf+NBUF; b++)
  {
    if(b->dev==dev && b->blockno==blockno)
    {
      b->refcnt++;
      release(&bcache[i].lock);
      acquiresleep(&b->lock);
      return b;
    }
    // to find the buf of min_ts
    if(b->refcnt==0 && b->timestamp<min_ts)
    {
      min_b=b;
      min_ts=b->timestamp;
    }

  }
  //choose min_b, the most recent buf.
  b = min_b;
  if(b != NULL)
  {
    b->dev=dev;
    b->blockno=blockno;
    b->valid = 0;
    b->refcnt = 1;
    release(&bcache[i].lock);
    acquiresleep(&b->lock);
    return b;
  }

  //no found
  panic("bget: no buffers");

}


//// accessor no change
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
//\\accessor no change

//// single relse
// // Release a locked buffer.
// // Move to the head of the most-recently-used list.
// void
// brelse(struct buf *b)
// {
//   if(!holdingsleep(&b->lock))
//     panic("brelse");

//   releasesleep(&b->lock);

//   acquire(&bcache.lock);
//   b->refcnt--;
//   if (b->refcnt == 0) {
//     // no one is waiting for it.
//     b->next->prev = b->prev;
//     b->prev->next = b->next;
//     b->next = bcache.head.next;
//     b->prev = &bcache.head;
//     bcache.head.next->prev = b;
//     bcache.head.next = b;
//   }
  
//   release(&bcache.lock);
// }

//// relse and 
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");
 
  releasesleep(&b->lock);
  int i=index(b->blockno);

  acquire(&bcache[i].lock);
  b->refcnt--;
  
  if (b->refcnt == 0) {
    b->timestamp = ticks;//call ticks extern
  }
  release(&bcache[i].lock);

}


void
bpin(struct buf *b) {
  int i=index(b->blockno);
  acquire(&bcache[i].lock);
  b->refcnt++;
  release(&bcache[i].lock);
}

void
bunpin(struct buf *b) {
  int i=index(b->blockno);
  acquire(&bcache[i].lock);
  b->refcnt--;
  release(&bcache[i].lock);
}


