/* See COPYRIGHT for copyright information. */

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <inc/x86.h>
#include <inc/.config>

#include <kern/monitor.h>
#include <kern/console.h>
#include <kern/pmap.h>
#include <kern/kclock.h>
#include <kern/env.h>
#include <kern/trap.h>
#include <kern/sched.h>
#include <kern/picirq.h>
#include <kern/cpu.h>
#include <kern/spinlock.h>

static void boot_aps(void);

static void msr_init(){
	extern void sysenter_handler();
	uint32_t cs;
	asm volatile("movl %%cs,%0":"=r"(cs));
	wrmsr(IA32_SYSENTER_CS, 0x0, cs);
	wrmsr(IA32_SYSENTER_EIP, 0x0, sysenter_handler);
	wrmsr(IA32_SYSENTER_ESP, 0x0,
			KSTACKTOP - cpunum() * (KSTKGAP + KSTKSIZE));	//Multi-processor
}

void
i386_init(void)
{

	// extern char edata[], end[];
	// //static_assert(0);
	// // Before doing anything else, complete the ELF loading process.
	// // Clear the uninitialized global data (BSS) section of our program.
	// // This ensures that all static/global variables start out zero.
	// memset(edata, 0, end - edata);


	// Initialize the console.
	// Can't call cprintf until after we do this!
	cons_init();
	cprintf("\n");

	// set_fgcolor(COLOR_RED);
	// cprintf("red\n");
	// set_fgcolor(COLOR_WHITE);
	// cprintf("white\n");
	// highlight(2);
	// set_fgcolor(COLOR_RED);
	// cprintf("red\n");
	// set_fgcolor(COLOR_GREEN);
	// cprintf("green\n");
	// set_fgcolor(COLOR_CYAN);
	// cprintf("cyan\n");
	// set_fgcolor(COLOR_MAGENTA);
	// cprintf("magenta\n");
	// set_fgcolor(COLOR_WHITE);
	// cprintf("white\n");
	init_cpuid();
	cprintf("\33[31;5;46;33;1;42mabcdefg\b\33[0m\n");
    
	reset_attr();
	//unsigned int i = 0x00646c72;
    //cprintf("H%x Wo%s\n", 57616, &i);

	// Lab 2 memory management initialization functions
	mem_init();

	// Lab 3 user environment initialization functions
	env_init();
	trap_init();
	msr_init();

	// Lab 4 multiprocessor initialization functions
	mp_init();
	lapic_init();

	// Lab 4 multitasking initialization functions
	pic_init();

	// Acquire the big kernel lock before waking up APs
	// Your code here:
	lock_kernel();

	// Starting non-boot CPUs
	boot_aps();

#if defined(TEST)
	// Don't touch -- used by grading script!
	ENV_CREATE(TEST, ENV_TYPE_USER);
#else
	// Touch all you want.
	ENV_CREATE(user_faultalloc, ENV_TYPE_USER);
	// ENV_CREATE(user_hello, ENV_TYPE_USER, 10);

#endif // TEST*

	// Schedule and run the first user environment!
	sched_yield();
}

// While boot_aps is booting a given CPU, it communicates the per-core
// stack pointer that should be loaded by mpentry.S to that CPU in
// this variable.
void *mpentry_kstack;

// Start the non-boot (AP) processors.
static void
boot_aps(void)
{
	extern unsigned char mpentry_start[], mpentry_end[];
	void *code;
	struct CpuInfo *c;

	// Write entry code to unused memory at MPENTRY_PADDR
	code = KADDR(MPENTRY_PADDR);
	memmove(code, mpentry_start, mpentry_end - mpentry_start);

	// Boot each AP one at a time
	for (c = cpus; c < cpus + ncpu; c++) {
		if (c == cpus + cpunum())  // We've started already.
			continue;

		// Tell mpentry.S what stack to use 
		mpentry_kstack = percpu_kstacks[c - cpus] + KSTKSIZE;
		// Start the CPU at mpentry_start
		lapic_startap(c->cpu_id, PADDR(code));
		// Wait for the CPU to finish some basic setup in mp_main()
		while(c->cpu_status != CPU_STARTED)
			;
	}
}

// Setup code for APs
void
mp_main(void)
{
#ifdef CONF_HUGE_PAGE
	extern bool pae_support, pse_support;
	// Enable page size extension
	if (pae_support && pse_support)
		lcr4(rcr4()|CR4_PSE);
#endif

	// We are in high EIP now, safe to switch to kern_pgdir 
	lcr3(PADDR(kern_pgdir));
	cprintf("SMP: CPU %d starting\n", cpunum());

	//each processor need initialize their seperated states
	msr_init();
	lapic_init();
	env_init_percpu();
	trap_init_percpu();
	xchg(&thiscpu->cpu_status, CPU_STARTED); // tqell boot_aps() we're up

	// ENV_CREATE(user_breakpoint, ENV_TYPE_USER, 10);
	// Now that we have finished some basic setup, call sched_yield()
	// to start running processes on this CPU.  But make sure that
	// only one CPU can enter the scheduler at a time!
	//
	// Your code here:
	lock_kernel();
	sched_yield();
	// Remove this after you finish Exercise 6
	// for (;;);
}

/*
 * Variable panicstr contains argument to first call to panic; used as flag
 * to indicate that the kernel has already called panic.
 */
const char *panicstr;

/*
 * Panic is called on unresolvable fatal errors.
 * It prints "panic: mesg", and then enters the kernel monitor.
 */
void
_panic(const char *file, int line, const char *fmt,...)
{
	va_list ap;

	if (panicstr)
		goto dead;
	panicstr = fmt;

	// Be extra sure that the machine is in as reasonable state
	asm volatile("cli; cld");

	va_start(ap, fmt);
	cprintf("kernel panic on CPU %d at %s:%d: ", cpunum(), file, line);
	vcprintf(fmt, ap);
	cprintf("\n");
	va_end(ap);

dead:
	/* break into the kernel monitor */
	while (1)
		monitor(NULL);
}

/* like panic, but don't */
void
_warn(const char *file, int line, const char *fmt,...)
{
	va_list ap;

	va_start(ap, fmt);
	cprintf("kernel warning at %s:%d: ", file, line);
	vcprintf(fmt, ap);
	cprintf("\n");
	va_end(ap);
}