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
#define NBUCKET 13
#define HASH(id) (id % NBUCKET)

// struct {
//   struct spinlock lock;
//   struct buf buf[NBUF];

//   // Linked list of all buffers, through prev/next.
//   // Sorted by how recently the buffer was used.
//   // head.next is most recent, head.prev is least.
//   struct buf head;
// } bcache;

struct hashbuf
{
  /* data */
  struct buf head;
  struct spinlock lock;
};

struct 
{
  /* data */
  struct hashbuf hash[NBUCKET];
  struct buf buf[NBUF];
}bcache;


void
binit(void)
{
  struct buf *b;
  char lockname[16];

  // Create linked list of buffers
  for(int i = 0 ; i < NBUCKET ; i++){
    snprintf(lockname, sizeof(lockname), "bcache_%d", i);
    initlock(&bcache.hash[i].lock, lockname);
    bcache.hash[i].head.prev = &bcache.hash[i].head;
    bcache.hash[i].head.next = &bcache.hash[i].head;
  }
  // bcache.head.prev = &bcache.head;
  // bcache.head.next = &bcache.head;
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.hash[0].head.next;
    b->prev = &bcache.hash[0].head;
    initsleeplock(&b->lock, "buffer");
    bcache.hash[0].head.next->prev = b;
    bcache.hash[0].head.next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int id = HASH(blockno);
  acquire(&bcache.hash[id].lock);

  // Is the block already cached?
  for(b = bcache.hash[id].head.next; b != &bcache.hash[id].head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.hash[id].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  // for(b = bcache.head.prev; b != &bcache.head; b = b->prev){
  //   if(b->refcnt == 0) {
  //     b->dev = dev;
  //     b->blockno = blockno;
  //     b->valid = 0;
  //     b->refcnt = 1;
  //     release(&bcache.lock);
  //     acquiresleep(&b->lock);
  //     return b;
  //   }
  // }
  b = 0;
  struct buf *tmp;
  for(int i = id , cycle = 0 ; cycle != NBUCKET ; i = HASH(i + 1)){
    ++cycle;
    if(i == id)continue;
    else {
      if(!holding(&bcache.hash[i].lock))acquire(&bcache.hash[i].lock);
    }
    for(tmp = bcache.hash[i].head.next; tmp != &bcache.hash[i].head; tmp = tmp->next)
      // 使用时间戳进行LRU算法，而不是根据结点在链表中的位置
      if(tmp->refcnt == 0 && (b == 0 || tmp->ticks < b->ticks))
        b = tmp;
    if(b) {
      // 如果是从其他散列桶窃取的，则将其以头插法插入到当前桶
      if(i != id) {
        b->next->prev = b->prev;
        b->prev->next = b->next;
        release(&bcache.hash[i].lock);

        b->next = bcache.hash[id].head.next;
        b->prev = &bcache.hash[id].head;
        bcache.hash[id].head.next->prev = b;
        bcache.hash[id].head.next = b;
      }

      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;

      acquire(&tickslock);
      b->ticks = ticks;
      release(&tickslock);

      release(&bcache.hash[id].lock);
      acquiresleep(&b->lock);
      return b;
  }else{
    if(i != id)
    release(&bcache.hash[id].lock);
    }
  }
  panic("bget: no buffers");
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
  int id = HASH(b->blockno);
  releasesleep(&b->lock);

  acquire(&bcache.hash[id].lock);
  b->refcnt--;
  // if (b->refcnt == 0) {
  //   // no one is waiting for it.
  //   b->next->prev = b->prev;
  //   b->prev->next = b->next;
  //   b->next = bcache.head.next;
  //   b->prev = &bcache.head;
  //   bcache.head.next->prev = b;
  //   bcache.head.next = b;
  // }
  acquire(&tickslock);
  b->ticks = ticks;
  release(&tickslock);
  
  release(&bcache.hash[id].lock);
}

void
bpin(struct buf *b) {
  int id = HASH(b->blockno);
  acquire(&bcache.hash[id].lock);
  b->refcnt++;
  release(&bcache.hash[id].lock);
}

void
bunpin(struct buf *b) {
  int id = HASH(b->blockno);
  acquire(&bcache.hash[id].lock);
  b->refcnt--;
  release(&bcache.hash[id].lock);
}


