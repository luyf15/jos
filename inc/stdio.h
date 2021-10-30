#ifndef JOS_INC_STDIO_H
#define JOS_INC_STDIO_H

#include <inc/stdarg.h>

#ifndef NULL
#define NULL	((void *) 0)
#endif /* !NULL */

#define ESC       "\33"
#define ATTR_OFF ESC"[0m"                      // default: disable all atrributes

#define L_move(n)       "\33[" #n "D"           // n: row
#define R_move(n)       "\33[" #n "C"
#define U_move(n)       "\33[" #n "A"
#define D_move(n)       "\33[" #n "B"

// accurate cursor location
#define CUR_goto(x, y) "\33[" #x ";" #y "H"     // target (x, y)
#define CUR_mark        ESC"[s"                 // mark current
#define CUR_back        ESC"[u"                 // back to make

#define SCR_clear       ESC"[2J"                // identical system("clear")
#define ATTR_bold       ESC"[1m"                // identical bold
#define ATTR_underline ESC"[4m"                 // identical underline

// Front color
#define F_black         ESC"[30m"
#define F_red           ESC"[31m"
#define F_green         ESC"[32m"
#define F_yellow        ESC"[33m"
#define F_blue          ESC"[34m"
#define F_magenta       ESC"[35m"               // 品红
#define F_cyan          ESC"[36m"               // 青色
#define F_white         ESC"[37m"

// Background color
#define B_black         ESC"[40m"
#define B_red           ESC"[41m"
#define B_green         ESC"[42m"
#define B_yellow        ESC"[43m"
#define B_blue          ESC"[44m"
#define B_magenta       ESC"[45m"
#define B_cyan          ESC"[46m"
#define B_white         ESC"[47m"

enum{
    COLOR_BLACK = 0,
    COLOR_RED,
    COLOR_GREEN,
    COLOR_YELLOW,
    COLOR_BLUE,
    COLOR_MAGENTA,
    COLOR_CYAN,
    COLOR_WHITE,
    COLOR_NUM,  //number of colors supported
}cons_color;


// lib/console.c
void	cputchar(int c);
int	getchar(void);
int	iscons(int fd);

// lib/printfmt.c
void	printfmt(void (*putch)(int, void*), void *putdat, const char *fmt, ...);
void	vprintfmt(void (*putch)(int, void*), void *putdat, const char *fmt, va_list);
int	snprintf(char *str, int size, const char *fmt, ...);
int	vsnprintf(char *str, int size, const char *fmt, va_list);

// lib/printf.c
int	cprintf(const char *fmt, ...);
int	vcprintf(const char *fmt, va_list);

// lib/fprintf.c
int	printf(const char *fmt, ...);
int	fprintf(int fd, const char *fmt, ...);
int	vfprintf(int fd, const char *fmt, va_list);

// lib/readline.c
char*	readline(const char *prompt);

//set and reset foreground color
void set_fgcolor(int color);
void reset_fgcolor();

//set and reset background color
void set_bgcolor(int color);
void reset_bgcolor();

void highlight(int c);
void lightdown();
void reset_attr();
int clear();

#endif /* !JOS_INC_STDIO_H */
