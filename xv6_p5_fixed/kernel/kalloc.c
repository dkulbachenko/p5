// Physical memory allocator, intended to allocate
// memory for user processes, kernel stacks, page table pages,
// and pipe buffers. Allocates 4096-byte pages.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "spinlock.h"

struct run
{
  struct run *next;
  // uint refcount;
};

struct
{
  struct spinlock lock;
  struct run *freelist;

  /*
  P5 changes
  */
  uint free_pages;                // track free pages
  uint ref_cnt[PHYSTOP / PGSIZE]; // track reference count

} kmem;

int helpFreePages() {
  return kmem.free_pages;
}

extern char end[]; // first address after kernel loaded from ELF file

// Initialize free list of physical pages.
void kinit(void)
{
  char *p;

  kmem.free_pages = 0;
  initlock(&kmem.lock, "kmem");
  p = (char *)PGROUNDUP((uint)end);
  for (; p + PGSIZE <= (char *)PHYSTOP; p += PGSIZE)
  {
    kmem.ref_cnt[(uint)p / PGSIZE] = 0;
    kfree(p);
  }
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void kfree(char *v)
{
  struct run *r;

  if ((uint)v % PGSIZE || v < end || (uint)v >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.

  acquire(&kmem.lock);

  r = (struct run *)v;
  if (kmem.ref_cnt[(uint)v / PGSIZE] > 0)
  {
    kmem.ref_cnt[(uint)v / PGSIZE]--;
  }
  // else
  // {
  //   if (kmem.ref_cnt[(uint)v / PGSIZE] != 1)
  //     cprintf("WARNING: TRYING TO FREE PAGE WITH REFCOUNT %d\n", kmem.ref_cnt[(uint)v / PGSIZE]);
  //   r->next = kmem.freelist;
  //   kmem.freelist = r;
  //   kmem.free_pages++;
  //   cprintf("freed page with pa %d\n", (int)(uint)v);
  //   kmem.ref_cnt[(uint)v / PGSIZE] = 0;
  // }

  // free if ref_cnt becomes 0
  if (kmem.ref_cnt[(uint)v / PGSIZE] == 0)
  {
    memset(v, 1, PGSIZE);
    r->next = kmem.freelist;
    kmem.freelist = r;
    kmem.free_pages++;
  }

  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
char *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if (r)
  {
    kmem.freelist = r->next;
    // set ref_cnt to 1 and decrease free pages
    kmem.ref_cnt[(uint)r / PGSIZE] = 1;
    kmem.free_pages--;
  }

  release(&kmem.lock);
  return (char *)r;
}

void incref(struct run *addr, int amount)
{
  acquire(&kmem.lock);
  // increment entry in ref_cnt by specified amount
  kmem.ref_cnt[(uint)addr / PGSIZE] += amount;
  release(&kmem.lock);
}

// returns reference count at given addr
int getRefs(uint addr)
{
  return kmem.ref_cnt[addr / PGSIZE];
}
