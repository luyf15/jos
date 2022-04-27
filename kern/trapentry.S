/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>

#include <kern/picirq.h>


###################################################################
# exceptions/interrupts
###################################################################

/* TRAPHANDLER defines a globally-visible function for handling a trap.
 * It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 *
 * You shouldn't call a TRAPHANDLER function from C, but you may
 * need to _declare_ one in C (for instance, to get a function pointer
 * during IDT setup).  You can declare the function with
 *   void NAME();
 * where NAME is the argument passed to TRAPHANDLER.
 */
#define TRAPHANDLER(name, num)						\
	.globl name;		/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;		/* align function definition */		\
	name:			/* function starts here */		\
	pushl $(num);							\
	jmp _alltraps

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)					\
	.globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
	pushl $0;							\
	pushl $(num);							\
	jmp _alltraps

.text

/*
 * Lab 3: Your code here for generating entry points for the different traps.
 */
#define TH(n) TRAPHANDLER_NOEC(handler##n, n)
#define THE(n) TRAPHANDLER(handler##n, n)

#include <kern/trapvector.inc>

/*
 * Lab 3: Your code here for _alltraps
 */
_alltraps:
	# push registers to build a trap frame 
	# therefore make the stack look like a struct trapframe
	pushl %ds
	pushl %es
	pushal

	# load GD_KD into %ds, %es
	movw $(GD_KD), %ax
	movw %ax, %ds
	movw %ax, %es

	# pass a pointer to the trap frame for function trap  
	pushl %esp
	call trap
spin_trap:
	jmp spin_trap

 .globl sysenter_handler
 sysenter_handler:

	# prepare arguments for syscall
	pushl -4(%ebp) # only four parameters supported to pass by regs
	pushl %edi # arg4
	pushl %ebx # arg3
	pushl %ecx # arg2
	pushl %edx # arg1
	pushl %eax # sysno
	call fast_syscall

	# pepare user space information to restore
	movl %esi,%edx # return eip
	movl %ebp,%ecx # user space esp
	
	sysexit

.data