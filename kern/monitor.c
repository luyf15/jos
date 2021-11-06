// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line


struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
	{ "backtrace", "Backtrace current function callstack", mon_backtrace },
	{ "clear", "Clear the console", mon_clear},
	{ "rainbow", "Test the colored console", mon_rainbow},
	{ "cpuid", "CPUID output in console", mon_cpuid}
};

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(commands); i++){
		set_fgcolor(i+1);
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	}
	reset_fgcolor();
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char _start[], entry[], etext[], edata[], end[];

	cprintf(F_blue B_white"Special kernel symbols:"ATTR_OFF"\n");
	set_fgcolor(COLOR_MAGENTA);
	cprintf("  _start                  %08x (phys)\n", _start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		ROUNDUP(end - entry, 1024) / 1024);
	reset_fgcolor();
	return 0;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	uint32_t ebp = read_ebp();
	set_fgcolor(COLOR_CYAN);
	set_bgcolor(COLOR_YELLOW);
	cprintf("Stack backtrace");
	reset_bgcolor();
	set_fgcolor(COLOR_RED);
	while (ebp){
		uint32_t eip = (uint32_t)*((int *)ebp + 1);
		cprintf("\nebp %x  eip %x  args", ebp, eip);
		int *args = (int *)ebp + 2;
		for (int i=0;i<4;i++)
			cprintf(" %08.x ",args[i]);
		cprintf("\n");
		struct Eipdebuginfo info;
		debuginfo_eip(eip,&info);
		cprintf("\t%s:%d: %.*s+%d\n",
			info.eip_file,info.eip_line,
			info.eip_fn_namelen,info.eip_fn_name,eip-info.eip_fn_addr);
		ebp = *(uint32_t *)ebp;
		//count++ ;
	}
	reset_fgcolor();
	return 0;
}

int
mon_clear(int argc, char **argv, struct Trapframe *tf)
{
	return clear();
}


void
rainbow(int stride)
{
	static const char msg[] = "rainbow!";
	for (int i = 0; i < COLOR_NUM; ++i) {
		set_fgcolor(i);
		set_bgcolor((i + stride) % COLOR_NUM);
		cprintf("%c", msg[i % (sizeof(msg) - 1)]);
	}
	reset_fgcolor();
	reset_bgcolor();
	cprintf("\n");
}
 
int 
mon_rainbow(int argc, char **argv, struct Trapframe *tf)
{
	for(int i = 1; i < COLOR_NUM; ++i)
		rainbow(i);
	return 0;
}

int
mon_cpuid(int argc, char **argv, struct Trapframe *tf)
{
	highlight(1);
	if (argc >= 2)
		print_cpuid(atoi(argv[1]));
	else
		print_cpuid(1);
	lightdown();
	return 0;
}
/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < ARRAY_SIZE(commands); i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");


	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}
