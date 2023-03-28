// Physical memory allocator, intended to allocate
// memory for user processes, kernel stacks, page table pages,
// and pipe buffers. Allocates 4096-byte pages.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "spinlock.h"
#include "kalloc.h"

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

extern char end[]; // first address after kernel loaded from ELF file

// Initialize free list of physical pages.
void kinit(void)
{
  char *p;
  int num = 0;

  initlock(&kmem.lock, "kmem");
  p = (char *)PGROUNDUP((uint)end);
  for (; p + PGSIZE <= (char *)PHYSTOP; p += PGSIZE)
  {
    kfree(p);
    num++;
  }
  kmem.free_pages = num;
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
  memset(v, 1, PGSIZE);

  acquire(&kmem.lock);

  r = (struct run *)v;
  if (kmem.ref_cnt[(uint)v / PGSIZE] != 1)
    cprintf("WARNING: TRYING TO FREE PAGE WITH REFCOUNT %d\n", kmem.ref_cnt[(uint)v / PGSIZE]);

  r->next = kmem.freelist;
  kmem.freelist = r;
  kmem.free_pages++;

  kmem.ref_cnt[(uint)v / PGSIZE] = 0;

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
    kmem.ref_cnt[(uint)r / PGSIZE] = 1;
    kmem.free_pages--;
    // r->refcount = 1;
  }

  release(&kmem.lock);
  return (char *)r;
}

void incref(struct run *page, int amount)
{
  acquire(&kmem.lock);
  // panic("incrementing ref count of page\n");
  kmem.ref_cnt[(uint)page / PGSIZE] += amount;
  // page->refcount += amount;
  release(&kmem.lock);
}

int helpFreePages(void)
{
  return (int)kmem.free_pages;
}

int getRefs(uint addr)
{
  return kmem.ref_cnt[addr / PGSIZE];
}