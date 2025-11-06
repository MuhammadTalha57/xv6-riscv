#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

// MLFQ Configuration
#define NMLFQ 4                 // Number of Queues
#define BOOST_INTERVAL 100      // Priority boost every 100 ticks

// Time allotments per queue (in timer ticks)
int time_allotment[NMLFQ] = {1, 2, 4, 8};

// Queue Heads (Circuar Linked Lists)
struct {
  struct proc* head;
  struct proc* tail;
  struct spinlock lock;
} mlfq[NMLFQ];

// Global State
int time_since_boost = 0;
struct spinlock mlfq_lock;

struct cpu cpus[NCPU];

struct proc proc[NPROC];

struct proc *initproc;

int nextpid = 1;
struct spinlock pid_lock;

extern void forkret(void);
static void freeproc(struct proc *p);

extern char trampoline[]; // trampoline.S

// helps ensure that wakeups of wait()ing
// parents are not lost. helps obey the
// memory model when using p->parent.
// must be acquired before any p->lock.
struct spinlock wait_lock;

// Allocate a page for each process's kernel stack.
// Map it high in memory, followed by an invalid
// guard page.
void
proc_mapstacks(pagetable_t kpgtbl)
{
  struct proc *p;
  
  for(p = proc; p < &proc[NPROC]; p++) {
    char *pa = kalloc();
    if(pa == 0)
      panic("kalloc");
    uint64 va = KSTACK((int) (p - proc));
    kvmmap(kpgtbl, va, (uint64)pa, PGSIZE, PTE_R | PTE_W);
  }
}

// initialize the proc table.
void
procinit(void)
{
  struct proc *p;
  
  initlock(&pid_lock, "nextpid");
  initlock(&wait_lock, "wait_lock");
  for(p = proc; p < &proc[NPROC]; p++) {
      initlock(&p->lock, "proc");
      p->state = UNUSED;
      p->kstack = KSTACK((int) (p - proc));
  }
}

// Must be called with interrupts disabled,
// to prevent race with process being moved
// to a different CPU.
int
cpuid()
{
  int id = r_tp();
  return id;
}

// Return this CPU's cpu struct.
// Interrupts must be disabled.
struct cpu*
mycpu(void)
{
  int id = cpuid();
  struct cpu *c = &cpus[id];
  return c;
}

// Return the current struct proc *, or zero if none.
struct proc*
myproc(void)
{
  push_off();
  struct cpu *c = mycpu();
  struct proc *p = c->proc;
  pop_off();
  return p;
}

int
allocpid()
{
  int pid;
  
  acquire(&pid_lock);
  pid = nextpid;
  nextpid = nextpid + 1;
  release(&pid_lock);

  return pid;
}

// Look in the process table for an UNUSED proc.
// If found, initialize state required to run in the kernel,
// and return with p->lock held.
// If there are no free procs, or a memory allocation fails, return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);
    if(p->state == UNUSED) {
      goto found;
    } else {
      release(&p->lock);
    }
  }
  return 0;

found:
  p->pid = allocpid();
  p->state = USED;
  
  // Initialize MLFQ fields
  p->queue_level = 0;
  p->time_in_queue = 0;
  p->next = 0;

  // Allocate a trapframe page.
  if((p->trapframe = (struct trapframe *)kalloc()) == 0){
    freeproc(p);
    release(&p->lock);
    return 0;
  }

  // An empty user page table.
  p->pagetable = proc_pagetable(p);
  if(p->pagetable == 0){
    freeproc(p);
    release(&p->lock);
    return 0;
  }

  // Set up new context to start executing at forkret,
  // which returns to user space.
  memset(&p->context, 0, sizeof(p->context));
  p->context.ra = (uint64)forkret;
  p->context.sp = p->kstack + PGSIZE;

  return p;
}

// free a proc structure and the data hanging from it,
// including user pages.
// p->lock must be held.
static void
freeproc(struct proc *p)
{
  if(p->trapframe)
    kfree((void*)p->trapframe);
  p->trapframe = 0;
  if(p->pagetable)
    proc_freepagetable(p->pagetable, p->sz);
  p->pagetable = 0;
  p->sz = 0;
  p->pid = 0;
  p->parent = 0;
  p->name[0] = 0;
  p->chan = 0;
  p->killed = 0;
  p->xstate = 0;
  
  // Clear MLFQ fields
  p->queue_level = 0;
  p->time_in_queue = 0;
  p->next = 0;
  
  p->state = UNUSED;
}

// Create a user page table for a given process, with no user memory,
// but with trampoline and trapframe pages.
pagetable_t
proc_pagetable(struct proc *p)
{
  pagetable_t pagetable;

  // An empty page table.
  pagetable = uvmcreate();
  if(pagetable == 0)
    return 0;

  // map the trampoline code (for system call return)
  // at the highest user virtual address.
  // only the supervisor uses it, on the way
  // to/from user space, so not PTE_U.
  if(mappages(pagetable, TRAMPOLINE, PGSIZE,
              (uint64)trampoline, PTE_R | PTE_X) < 0){
    uvmfree(pagetable, 0);
    return 0;
  }

  // map the trapframe page just below the trampoline page, for
  // trampoline.S.
  if(mappages(pagetable, TRAPFRAME, PGSIZE,
              (uint64)(p->trapframe), PTE_R | PTE_W) < 0){
    uvmunmap(pagetable, TRAMPOLINE, 1, 0);
    uvmfree(pagetable, 0);
    return 0;
  }

  return pagetable;
}

// Free a process's page table, and free the
// physical memory it refers to.
void
proc_freepagetable(pagetable_t pagetable, uint64 sz)
{
  uvmunmap(pagetable, TRAMPOLINE, 1, 0);
  uvmunmap(pagetable, TRAPFRAME, 1, 0);
  uvmfree(pagetable, sz);
}

// Set up first user process.
void
userinit(void)
{
  struct proc *p;

  p = allocproc();
  initproc = p;
  
  p->cwd = namei("/");

  // Initialize MLFQ fields
  p->queue_level = 0;
  p->time_in_queue = 0;
  p->next = 0;

  p->state = RUNNABLE;
  
  // Enqueue to highest priority queue
  mlfq_enqueue(p, 0);

  release(&p->lock);
}
// Grow or shrink user memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint64 sz;
  struct proc *p = myproc();

  sz = p->sz;
  if(n > 0){
    if(sz + n > TRAPFRAME) {
      return -1;
    }
    if((sz = uvmalloc(p->pagetable, sz, sz + n, PTE_W)) == 0) {
      return -1;
    }
  } else if(n < 0){
    sz = uvmdealloc(p->pagetable, sz, sz + n);
  }
  p->sz = sz;
  return 0;
}

// Create a new process, copying the parent.
// Sets up child kernel stack to return as if from fork() system call.
int
kfork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *p = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy user memory from parent to child.
  if(uvmcopy(p->pagetable, np->pagetable, p->sz) < 0){
    freeproc(np);
    release(&np->lock);
    return -1;
  }
  np->sz = p->sz;

  // copy saved user registers.
  *(np->trapframe) = *(p->trapframe);

  // Cause fork to return 0 in the child.
  np->trapframe->a0 = 0;

  // increment reference counts on open file descriptors.
  for(i = 0; i < NOFILE; i++)
    if(p->ofile[i])
      np->ofile[i] = filedup(p->ofile[i]);
  np->cwd = idup(p->cwd);

  safestrcpy(np->name, p->name, sizeof(p->name));

  pid = np->pid;

  release(&np->lock);

  acquire(&wait_lock);
  np->parent = p;
  release(&wait_lock);

  acquire(&np->lock);
  
  // Initialize MLFQ fields - new process starts at highest priority
  np->queue_level = 0;
  np->time_in_queue = 0;
  np->next = 0;
  
  np->state = RUNNABLE;
  
  // Enqueue to highest priority queue
  mlfq_enqueue(np, 0);
  
  release(&np->lock);

  return pid;
}
// Pass p's abandoned children to init.
// Caller must hold wait_lock.
void
reparent(struct proc *p)
{
  struct proc *pp;

  for(pp = proc; pp < &proc[NPROC]; pp++){
    if(pp->parent == p){
      pp->parent = initproc;
      wakeup(initproc);
    }
  }
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait().
void
kexit(int status)
{
  struct proc *p = myproc();

  if(p == initproc)
    panic("init exiting");

  // Close all open files.
  for(int fd = 0; fd < NOFILE; fd++){
    if(p->ofile[fd]){
      struct file *f = p->ofile[fd];
      fileclose(f);
      p->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(p->cwd);
  end_op();
  p->cwd = 0;

  acquire(&wait_lock);

  // Give any children to init.
  reparent(p);

  // Parent might be sleeping in wait().
  wakeup(p->parent);
  
  acquire(&p->lock);
  
  // Remove from MLFQ if still in queue
  if(p->state == RUNNABLE) {
    mlfq_remove(p);
  }

  p->xstate = status;
  p->state = ZOMBIE;

  release(&wait_lock);

  // Jump into the scheduler, never to return.
  sched();
  panic("zombie exit");
}


// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
kwait(uint64 addr)
{
  struct proc *pp;
  int havekids, pid;
  struct proc *p = myproc();

  acquire(&wait_lock);

  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(pp = proc; pp < &proc[NPROC]; pp++){
      if(pp->parent == p){
        // make sure the child isn't still in exit() or swtch().
        acquire(&pp->lock);

        havekids = 1;
        if(pp->state == ZOMBIE){
          // Found one.
          pid = pp->pid;
          if(addr != 0 && copyout(p->pagetable, addr, (char *)&pp->xstate,
                                  sizeof(pp->xstate)) < 0) {
            release(&pp->lock);
            release(&wait_lock);
            return -1;
          }
          freeproc(pp);
          release(&pp->lock);
          release(&wait_lock);
          return pid;
        }
        release(&pp->lock);
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || killed(p)){
      release(&wait_lock);
      return -1;
    }
    
    // Wait for a child to exit.
    sleep(p, &wait_lock);  //DOC: wait-sleep
  }
}

// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run.
//  - swtch to start running that process.
//  - eventually that process transfers control
//    via swtch back to the scheduler.
// MLFQ Scheduler
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();

  c->proc = 0;
  for(;;){
    // Enable interrupts to avoid deadlock
    intr_on();
    
    int found = 0;
    
    // Iterate through queues from highest to lowest priority
    for(int q = 0; q < NMLFQ; q++) {
      acquire(&mlfq[q].lock);
      
      // Check if queue has processes
      if(mlfq[q].head != 0) {
        p = mlfq[q].head;
        
        // Find first RUNNABLE process in this queue
        while(p != 0) {
          acquire(&p->lock);
          
          if(p->state == RUNNABLE) {
            // Remove from queue
            if(p == mlfq[q].head) {
              mlfq[q].head = p->next;
              if(mlfq[q].head == 0)
                mlfq[q].tail = 0;
            }
            p->next = 0;
            
            release(&mlfq[q].lock);
            
            // Switch to chosen process
            p->state = RUNNING;
            c->proc = p;
            
            swtch(&c->context, &p->context);
            
            // Process is done running
            c->proc = 0;
            found = 1;
            
            release(&p->lock);
            break;
          }
          
          release(&p->lock);
          p = p->next;
        }
        
        if(!found)
          release(&mlfq[q].lock);
        
        if(found)
          break;
      } else {
        release(&mlfq[q].lock);
      }
    }
    
    if(found == 0) {
      intr_on();
      asm volatile("wfi");
    }
  }
}



// Switch to scheduler.  Must hold only p->lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->noff, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&p->lock))
    panic("sched p->lock");
  if(mycpu()->noff != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched RUNNING");
  if(intr_get())
    panic("sched interruptible");

  intena = mycpu()->intena;
  swtch(&p->context, &mycpu()->context);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  struct proc *p = myproc();
  acquire(&p->lock);
  
  // Check if process exceeded time allotment
  if(p->time_in_queue >= time_allotment[p->queue_level]) {
    // Demote to lower priority queue (if not already at lowest)
    if(p->queue_level < NMLFQ - 1) {
      p->queue_level++;
    }
    p->time_in_queue = 0;
  }
  
  p->state = RUNNABLE;
  
  // Re-enqueue at current priority level
  int queue = p->queue_level;
  release(&p->lock);
  
  mlfq_enqueue(p, queue);
  
  acquire(&p->lock);
  sched();
  release(&p->lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch to forkret.
void
forkret(void)
{
  extern char userret[];
  static int first = 1;
  struct proc *p = myproc();

  // Still holding p->lock from scheduler.
  release(&p->lock);

  if (first) {
    // File system initialization must be run in the context of a
    // regular process (e.g., because it calls sleep), and thus cannot
    // be run from main().
    fsinit(ROOTDEV);

    first = 0;
    // ensure other cores see first=0.
    __sync_synchronize();

    // We can invoke kexec() now that file system is initialized.
    // Put the return value (argc) of kexec into a0.
    p->trapframe->a0 = kexec("/init", (char *[]){ "/init", 0 });
    if (p->trapframe->a0 == -1) {
      panic("exec");
    }
  }

  // return to user space, mimicing usertrap()'s return.
  prepare_return();
  uint64 satp = MAKE_SATP(p->pagetable);
  uint64 trampoline_userret = TRAMPOLINE + (userret - trampoline);
  ((void (*)(uint64))trampoline_userret)(satp);
}

// Sleep on channel chan, releasing condition lock lk.
// Re-acquires lk when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  // Must acquire p->lock in order to
  // change p->state and then call sched.
  acquire(&p->lock);
  release(lk);

  // Remove from MLFQ before sleeping
  if(p->state == RUNNABLE) {
    mlfq_remove(p);
  }

  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  release(&p->lock);
  acquire(lk);
}

// Wake up all processes sleeping on channel chan.
// Caller should hold the condition lock.
void
wakeup(void *chan)
{
  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++) {
    if(p != myproc()){
      acquire(&p->lock);
      if(p->state == SLEEPING && p->chan == chan) {
        p->state = RUNNABLE;
        // Re-enqueue at current priority level
        int queue = p->queue_level;
        mlfq_enqueue(p, queue);
      }
      release(&p->lock);
    }
  }
}

// Kill the process with the given pid.
// The victim won't exit until it tries to return
// to user space (see usertrap() in trap.c).
int
kkill(int pid)
{
  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++){
    acquire(&p->lock);
    if(p->pid == pid){
      p->killed = 1;
      if(p->state == SLEEPING){
        // Wake process from sleep().
        p->state = RUNNABLE;
      }
      release(&p->lock);
      return 0;
    }
    release(&p->lock);
  }
  return -1;
}

void
setkilled(struct proc *p)
{
  acquire(&p->lock);
  p->killed = 1;
  release(&p->lock);
}

int
killed(struct proc *p)
{
  int k;
  
  acquire(&p->lock);
  k = p->killed;
  release(&p->lock);
  return k;
}

// Copy to either a user address, or kernel address,
// depending on usr_dst.
// Returns 0 on success, -1 on error.
int
either_copyout(int user_dst, uint64 dst, void *src, uint64 len)
{
  struct proc *p = myproc();
  if(user_dst){
    return copyout(p->pagetable, dst, src, len);
  } else {
    memmove((char *)dst, src, len);
    return 0;
  }
}

// Copy from either a user address, or kernel address,
// depending on usr_src.
// Returns 0 on success, -1 on error.
int
either_copyin(void *dst, int user_src, uint64 src, uint64 len)
{
  struct proc *p = myproc();
  if(user_src){
    return copyin(p->pagetable, dst, src, len);
  } else {
    memmove(dst, (char*)src, len);
    return 0;
  }
}

// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [USED]      "used",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  struct proc *p;
  char *state;

  printf("\n");
  for(p = proc; p < &proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    printf("%d %s %s", p->pid, state, p->name);
    printf("\n");
  }
}


void
mlfq_tick(void)
{
  struct proc *p = myproc();
  
  if(p != 0 && p->state == RUNNING) {
    acquire(&p->lock);
    p->time_in_queue++;
    release(&p->lock);
  }
  
  // Increment global tick counter
  acquire(&mlfq_lock);
  time_since_boost++;
  
  // Priority boost: move all processes to highest queue
  if(time_since_boost >= BOOST_INTERVAL) {
    time_since_boost = 0;
    mlfq_boost();
  }
  release(&mlfq_lock);
}

// Boost all processes to highest priority queue
void
mlfq_boost(void)
{
  struct proc *p;
  
  // Move all processes from lower queues to queue 0
  for(int q = 1; q < NMLFQ; q++) {
    acquire(&mlfq[q].lock);
    
    while(mlfq[q].head != 0) {
      p = mlfq[q].head;
      mlfq[q].head = p->next;
      
      acquire(&p->lock);
      if(p->state == RUNNABLE) {
        p->queue_level = 0;
        p->time_in_queue = 0;
        p->next = 0;
        release(&p->lock);
        
        // Don't call mlfq_enqueue here to avoid nested locks
        // Instead manually add to queue 0
        acquire(&mlfq[0].lock);
        if(mlfq[0].tail) {
          mlfq[0].tail->next = p;
        } else {
          mlfq[0].head = p;
        }
        mlfq[0].tail = p;
        release(&mlfq[0].lock);
      } else {
        release(&p->lock);
      }
    }
    
    mlfq[q].tail = 0;
    release(&mlfq[q].lock);
  }
}

// Initialize MLFQ
void
mlfq_init(void)
{
  initlock(&mlfq_lock, "mlfq");
  for(int i = 0; i < NMLFQ; i++) {
    mlfq[i].head = 0;
    mlfq[i].tail = 0;
    initlock(&mlfq[i].lock, "mlfq_queue");
  }
}

// Enqueue process to specified queue
void
mlfq_enqueue(struct proc *p, int queue)
{
  acquire(&mlfq[queue].lock);
  
  p->next = 0;
  p->queue_level = queue;
  p->time_in_queue = 0;  // Reset time in new queue
  
  if(mlfq[queue].tail) {
    mlfq[queue].tail->next = p;
  } else {
    mlfq[queue].head = p;
  }
  mlfq[queue].tail = p;
  
  release(&mlfq[queue].lock);
}

// Dequeue process from specified queue
struct proc*
mlfq_dequeue(int queue)
{
  acquire(&mlfq[queue].lock);
  
  struct proc *p = mlfq[queue].head;
  if(p) {
    mlfq[queue].head = p->next;
    if(mlfq[queue].head == 0)
      mlfq[queue].tail = 0;
    p->next = 0;
  }
  
  release(&mlfq[queue].lock);
  return p;
}

// Remove specific process from its queue (for sleep/exit)
void
mlfq_remove(struct proc *p)
{
  int queue = p->queue_level;
  acquire(&mlfq[queue].lock);
  
  struct proc *curr = mlfq[queue].head;
  struct proc *prev = 0;
  
  while(curr) {
    if(curr == p) {
      if(prev)
        prev->next = curr->next;
      else
        mlfq[queue].head = curr->next;
      
      if(mlfq[queue].tail == curr)
        mlfq[queue].tail = prev;
      
      p->next = 0;
      break;
    }
    prev = curr;
    curr = curr->next;
  }
  
  release(&mlfq[queue].lock);
}