#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include <stddef.h>  

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;


  argint(0, &n);
  if(n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}


#ifdef LAB_PGTBL
int
sys_pgpte(void)
{
  uint64 va;
  struct proc *p;  

  p = myproc();
  argaddr(0, &va);
  pte_t *pte = pgpte(p->pagetable, va);
  if(pte != 0) {
      return (uint64) *pte;
  }
  return 0;
}
#endif

#ifdef LAB_PGTBL
int
sys_kpgtbl(void)
{
  struct proc *p;  

  p = myproc();
  vmprint(p->pagetable);
  return 0;
}
#endif


uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}


#ifdef LAB_PGTBL
int sys_pgaccess(void)
{
  uint64 base;
  int len;
  uint64 mask; 
  uint64 buffer = 0;
  pte_t *pte;

  // Retrieve arguments from user space
  argaddr(0, &base);  // No need to check for return value, the kernel handles errors
  argint(1, &len);    // Same as above
  if (len > 64) {
    printf("Too many pages to be scanned!\n");
    return -1;
  }
  argaddr(2, &mask);  // No need to check for return value, the kernel handles errors

  // Iterate over the pages to check if they have been accessed
  for (int i = 0; i < len; i++) {
    if (base >= MAXVA)  // If the base address exceeds the maximum virtual address
      break;

    pagetable_t pagetable = myproc()->pagetable;
    int level;
    // Walk through the page table levels (PML4, PDPT, etc.)
    for (level = 2; level >= 0; level--) {
      pte = &pagetable[PX(level, base)];
      if (*pte & PTE_V) {  // Check if the page is valid
        pagetable = (pagetable_t)PTE2PA(*pte);  // Move to the next level
      } else {
        pte = NULL;  // Set to NULL if page table entry is invalid
        break;
      }
    }

    if (pte == NULL || (*pte & PTE_V) == 0)  // Check if the page is valid and accessible
      continue;

    // Check if the page has been accessed (PTE_A bit set)
    if (*pte & PTE_A) {
      buffer |= (1L << i);  // Set the corresponding bit in the buffer
      *pte &= ~PTE_A;  // Clear the access bit (PTE_A)
    }

    base += PGSIZE;  // Move to the next page
  }

  // Copy the result (bitmask) back to user space
  if (copyout(myproc()->pagetable, mask, (char *)&buffer, sizeof(uint64)) < 0)
    return -1;
  
  return 0;
}
#endif


