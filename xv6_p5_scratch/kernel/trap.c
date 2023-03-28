#include "types.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "traps.h"
#include "spinlock.h"

// Interrupt descriptor table (shared by all CPUs).
struct gatedesc idt[256];
extern uint vectors[]; // in vectors.S: array of 256 entry pointers
struct spinlock tickslock;
uint ticks;

void tvinit(void)
{
  int i;

  for (i = 0; i < 256; i++)
    SETGATE(idt[i], 0, SEG_KCODE << 3, vectors[i], 0);
  SETGATE(idt[T_SYSCALL], 1, SEG_KCODE << 3, vectors[T_SYSCALL], DPL_USER);

  initlock(&tickslock, "time");
}

void idtinit(void)
{
  lidt(idt, sizeof(idt));
}

void trap(struct trapframe *tf)
{
  if (tf->trapno == T_SYSCALL)
  {
    if (proc->killed)
      exit();
    proc->tf = tf;
    syscall();
    if (proc->killed)
      exit();
    return;
  }

  switch (tf->trapno)
  {
  case T_IRQ0 + IRQ_TIMER:
    if (cpu->id == 0)
    {
      acquire(&tickslock);
      ticks++;
      wakeup(&ticks);
      release(&tickslock);
    }
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE:
    ideintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE + 1:
    // Bochs generates spurious IDE1 interrupts.
    break;
  case T_IRQ0 + IRQ_KBD:
    kbdintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_COM1:
    uartintr();
    lapiceoi();
    break;
  case T_IRQ0 + 7:
  case T_IRQ0 + IRQ_SPURIOUS:
    cprintf("cpu%d: spurious interrupt at %x:%x\n",
            cpu->id, tf->cs, tf->eip);
    lapiceoi();
    break;
  case T_PGFLT:
    // code for handling page fault
    cprintf("starting pg fault\n");
    uint addr = rcr2();

    // check validity of va
    if (addr > USERTOP)
    {
      cprintf("CoW: Invalid virtual address\n");
      exit();
    }

    // get pte of faulting addr
    pte_t *pte;
    if ((pte = walkpgdirHelper(getpgdir(), (void *)(addr), 0)) == 0)
    {
      panic("trap should be in pt\n");
    }

    // get physical addr of pte as well as the ref_cnt of that address
    uint pa = PTE_ADDR(*pte);
    int refs = getRefs(pa);

    // if the ref_cnt was zero for some reason
    if (refs == 0)
    {
      cprintf("Shoud have more than 0 Refs if in page table\n");
    }

    // if we don't need to copy, just change permissions
    if (refs == 1)
    {
      // set writeable bits
      *pte = (uint)*pte | PTE_W;
      // flush tlb
      lcr3(PADDR(getpgdir()));
      cprintf("ref count was 1\n");
      break;
    }

    // set to writeable
    *pte = (uint)*pte | PTE_W;
    // set to not present
    // do this because we need to map to new physical addr
    *pte = (uint)*pte & ~PTE_P;
    // flush tlb
    lcr3(PADDR(getpgdir()));
    // get updated flags
    uint flags = PTE_FLAGS(*pte);

    // decrease ref_cnt of physical page
    incref((struct run *)pa, -1);

    // alloc a new page
    char *mem;
    if ((mem = kalloc()) == 0)
      cprintf("failed kalloc\n");

    // copy old page into new page
    memmove(mem, (char *)pa, PGSIZE);

    cprintf("before trap map\n");
    // if (mappagesHelper(getpgdir(), (void *)(addr - (addr % PGSIZE)), PGSIZE, PADDR(mem), flags) < 0)
    //   goto bad;

    // map pt entry to new page that we just allocd
    if (mappagesHelper(getpgdir(), (void *)(addr), PGSIZE, PADDR(mem), flags) < 0)
    {
      cprintf("bad\n");
      freevm(getpgdir());
    }

    cprintf("after trap map\n");

    break;
  default:
    if (proc == 0 || (tf->cs & 3) == 0)
    {
      // In kernel, it must be our mistake.
      cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
              tf->trapno, cpu->id, tf->eip, rcr2());
      panic("trap");
    }
    // In user space, assume process misbehaved.
    cprintf("pid %d %s: trap %d err %d on cpu %d "
            "eip 0x%x addr 0x%x--kill proc\n",
            proc->pid, proc->name, tf->trapno, tf->err, cpu->id, tf->eip,
            rcr2());
    proc->killed = 1;
  }

  // Force process exit if it has been killed and is in user space.
  // (If it is still executing in the kernel, let it keep running
  // until it gets to the regular system call return.)
  if (proc && proc->killed && (tf->cs & 3) == DPL_USER)
    exit();

  // Force process to give up CPU on clock tick.
  // If interrupts were on while locks held, would need to check nlock.
  if (proc && proc->state == RUNNING && tf->trapno == T_IRQ0 + IRQ_TIMER)
    yield();

  // Check if the process has been killed since we yielded
  if (proc && proc->killed && (tf->cs & 3) == DPL_USER)
    exit();
}
