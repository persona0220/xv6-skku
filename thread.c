#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

extern struct _ptable{
	struct spinlock lock;
	struct proc proc[NPROC];
}ptable;

extern struct proc *initproc;
extern void forkret(void);
extern void trapret(void);

int thread_create(void *(*function)(void *), int priority, void *arg, void *stack){

	//allocate

	struct proc *p;
	char *sp;

	if(*proc->ttable == 8){
		//if there are 8 threads already, error
		return -1;
	}

	acquire(&ptable.lock);
	for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
		if(p->state == UNUSED)
			goto found;
	release(&ptable.lock);
	return -1;

found:
	p->ttable = proc->ttable;
	p->state = EMBRYO;
	p->pid = proc->pid;
	p->tid = p->pid + *p->ttable;
	(*p->ttable)++;
	p->nice = priority;
	p->pgdir = proc->pgdir;
	p->sz = proc->sz;
	p->parent = proc->parent;

	release(&ptable.lock);
	
	//set stack
	if((p->kstack = kalloc()) == 0){
		p->state = UNUSED;
		return -1;
	}
	sp = p->kstack + KSTACKSIZE;

	// Leave room for trap frame.
	sp -= sizeof *p->tf;
	p->tf = (struct trapframe*)sp;
	*p->tf = *proc->tf;

	// Set up new context to start executing at forkret,
	// which returns to trapret.
	sp -= 4;
	*(uint*)sp = (uint)trapret;

	sp -= sizeof *p->context;
	p->context = (struct context*)sp;
	memset(p->context, 0, sizeof *p->context);
	p->context->eip = (uint)forkret;
	
	//set register
	p->tf->eax = 0;
	p->tf->eip = (int)function;
	p->tf->esp = (int)(stack + PGSIZE - 8);

	*(int*)(p->tf->esp + 4) = (int)arg;
	*(int*)(p->tf->esp) = 0xffffffff;

	p->stack = stack;

	int i;
	for(i=0; i<NOFILE; i++){
		if(proc -> ofile[i]){
			p->ofile[i] = proc -> ofile[i];
		}
	}
	p->cwd = proc->cwd;

	safestrcpy(p->name, proc->name, sizeof(proc->name));

	acquire(&ptable.lock);
	p->state = RUNNABLE;
	proc->state = RUNNABLE;
	sched();
	release(&ptable.lock);

	return p->tid;
}

void thread_exit(void *retval){
	
	struct proc *p;
	int i, fd;
	int ismain = 0;

	if(proc == initproc)
		panic("init exiting");
	
	if(proc->tid == proc->pid)
		ismain = 1;

	if(ismain){
		// Close all open files.
		for(fd = 0; fd < NOFILE; fd++){
			if(proc->ofile[fd]){
				fileclose(proc->ofile[fd]);
				proc->ofile[fd] = 0;
			}
		}
	}
	else{
		for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
			if(p->pid != proc->pid)
				continue;
			if(p->state == RUNNABLE){
				for(i=0; i<NOFILE; i++){
					if(p->pid != p->tid && proc->ofile[i]){
						p->ofile[i] = proc->ofile[i];
					}
					else if(p->pid == p->tid && proc->ofile[i] && !p->ofile[i]){
						p->ofile[i] = filedup(proc->ofile[i]);
					}
				}
			}
		}
	}

	proc->cwd = 0;
	acquire(&ptable.lock);
	
	for(p=ptable.proc; p<&ptable.proc[NPROC]; p++){
		if(p->pid == proc->pid) 
			wakeup1(p);
	}

	if(ismain){
		//Pass abandoned children to init.
		for(p = ptable.proc; p<&ptable.proc[NPROC]; p++){
			if(p->parent == proc){
				p->parent = initproc;
				if(p->state == ZOMBIE)
					wakeup1(initproc);
			}
		}

		//exit other thread.
		for(p = ptable.proc; p<&ptable.proc[NPROC]; p++){
			if(*proc->ttable == 1) break;
			if(p->pid != proc->pid || p->pid == p->tid) continue;
			if(p->state == RUNNABLE){
				// Found one.
				kfree(p->kstack);
				p->kstack = 0;
				p->pid = 0;
				p->parent = 0;
				p->name[0] = 0;
				p->killed = 0;
				p->nice = -1;
				p->state = UNUSED;
				(*p->ttable)--;
				p->ttable = 0;
				for(i=0; i<NOFILE; i++)
					p->ofile[i] = 0;
			}
		}
	}
	else{
		*(int*)(proc->stack + PGSIZE - 8) = (int)retval;	
	}

	// Jump into the scheduler, never to return.
	proc->state = ZOMBIE;
	sched();
	panic("zombie exit");
}

int thread_join(int tid, void **retval){

	struct proc *p;
	int havekids;

	acquire(&ptable.lock);

	for(;;){
		// Scan through table looking for exited children.
		havekids = 0;
		for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
			if(p->pid != proc->pid || p->tid != tid)
				continue;
			havekids = 1;
			int ismain = p->tid==p->pid?1:0;
			if(p->state == ZOMBIE){
				// Found one.
				*(int*)retval = *(int*)(p->stack + PGSIZE -8);
				kfree(p->kstack);
				p->kstack = 0;
				if(ismain) freevm(p->pgdir);
				p->pid = 0;
				p->parent = 0;
				p->name[0] = 0;
				p->killed = 0;
				p->nice = -1;
				p->state = UNUSED;
				
				if(!ismain){
					(*p->ttable)--;
					p->ttable = 0;
				}

				release(&ptable.lock);
				return 0;
			}
		}

		// No point waiting if we don't have any children.
		if(!havekids || proc->killed){
			release(&ptable.lock);
			return -1;
		}

		// Wait for children to exit.  (See wakeup1 call in proc_exit.)
		sleep(proc, &ptable.lock);  //DOC: wait-sleep
	}

	return -1;
}

int gettid(void){
	return proc->tid;
}
