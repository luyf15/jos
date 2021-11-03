// Simple implementation of cprintf console output for the kernel,
// based on printfmt() and the kernel console's cputchar().

#include <inc/types.h>
#include <inc/stdio.h>
#include <inc/stdarg.h>


static void
putch(int ch, int *cnt)
{
	cputchar(ch);
	*cnt++;
}

int
vcprintf(const char *fmt, va_list ap)
{
	int cnt = 0;

	vprintfmt((void*)putch, &cnt, fmt, ap);
	return cnt;
}

int
cprintf(const char *fmt, ...)
{
	va_list ap;
	int cnt;

	va_start(ap, fmt);
	cnt = vcprintf(fmt, ap);
	va_end(ap);

	return cnt;
}

//for fg/bg color
static int fgcolor = -1;
static int bgcolor = -1;
static int enable_hli = 0;

static const char numbers[]={"01234567"};

void
set_fgcolor(int color)
{
	fgcolor = color;
	if (enable_hli > 0)
		cprintf("\33[9%cm",numbers[color]);
	else
		cprintf("\33[3%cm",numbers[color]);
}

void
set_bgcolor(int color)
{
	bgcolor = color;
	if (enable_hli < 0)
		cprintf("\33[10%cm",numbers[color]);
	else
		cprintf("\33[4%cm",numbers[color]);
}

void
reset_fgcolor()
{
	cprintf(ATTR_OFF);
	if (bgcolor != -1)
		cprintf("\33[4%cm",numbers[bgcolor]);
	fgcolor = -1;
}

void
reset_bgcolor()
{
	cprintf(ATTR_OFF);
	if (fgcolor != -1)
		cprintf("\33[3%cm",numbers[fgcolor]);
	bgcolor = -1;
}

void
lightdown()
{
	cprintf(ATTR_OFF);
	if (fgcolor != -1)
		cprintf("\33[3%cm",numbers[fgcolor]);
	if (bgcolor != -1)
		cprintf("\33[4%cm",numbers[fgcolor]);
	enable_hli = 0;
}

void
highlight(int c)
{	
	enable_hli = c;
}

void
reset_attr()
{
	cprintf(ATTR_OFF);
}

int
clear()
{
	int cnt = cprintf(SCR_clear);
	if (!cnt)
		return -1;
	else return 0;
}

