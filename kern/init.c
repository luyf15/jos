/* See COPYRIGHT for copyright information. */

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/monitor.h>
#include <kern/console.h>
#include <kern/pmap.h>
#include <kern/kclock.h>
#include <kern/env.h>
#include <kern/trap.h>

static void msr_init(){
	extern void sysenter_handler();
	uint32_t cs;
	asm volatile("movl %%cs,%0":"=r"(cs));
	wrmsr(IA32_SYSENTER_CS,0x0,cs);
	wrmsr(IA32_SYSENTER_EIP,0x0,sysenter_handler);
	wrmsr(IA32_SYSENTER_ESP,0x0,KSTACKTOP);
}

void
i386_init(void)
{
	extern char edata[], end[];
	//static_assert(0);
	// Before doing anything else, complete the ELF loading process.
	// Clear the uninitialized global data (BSS) section of our program.
	// This ensures that all static/global variables start out zero.
	memset(edata, 0, end - edata);

	// Initialize the console.
	// Can't call cprintf until after we do this!
	cons_init();
	cprintf("\n");

	set_fgcolor(COLOR_RED);
	cprintf("red\n");
	set_fgcolor(COLOR_WHITE);
	cprintf("white\n");
	highlight(2);
	set_fgcolor(COLOR_RED);
	cprintf("red\n");
	set_fgcolor(COLOR_GREEN);
	cprintf("green\n");
	set_fgcolor(COLOR_CYAN);
	cprintf("cyan\n");
	set_fgcolor(COLOR_MAGENTA);
	cprintf("magenta\n");
	set_fgcolor(COLOR_WHITE);
	cprintf("white\n");

	print_cpuid(0);
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

#if defined(TEST)
	// Don't touch -- used by grading script!
	ENV_CREATE(TEST, ENV_TYPE_USER);
#else
	// Touch all you want.
	ENV_CREATE(user_breakpoint, ENV_TYPE_USER);
#endif // TEST*

	// We only have one user environment for now, so just run it.
	env_run(&envs[0]);
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
	cprintf("kernel panic at %s:%d: ", file, line);
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