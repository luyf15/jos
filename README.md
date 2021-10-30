# Lab 1

```c
running JOS:
$ make qemu-nox-gdb
(0.8s)
printf: OK
backtrace count: OK
backtrace arguments: OK
backtrace symbols: OK
backtrace lines: OK
Score: 50/50
```

- Ex3: At what point does the processor start executing 32-bit code? What exactly causes the switch from 16- to 32-bit mode?
    
    ```c
    lgdt    gdtdesc
    movl    %cr0, %eax
    orl     $CR0_PE_ON, %eax  ;$0x1
    movl    %eax, %cr0
    **ljmp    $PROT_MODE_CSEG, $protcseg  ;cs=$0x8**
    ```
    
    What is the last instruction of the boot loader executed?
    
    ```c
    ((void (*)(void)) (ELFHDR->e_entry))();   //call *0x10018
    ```
    
    What is the first instruction of the kernel it just loaded?
    
    ```c
    entry: 
    	f010000c:   movw	$0x1234,0x472		;warm boot
    ```
    
    Where is the first instruction of the kernel?  ——***0x10018=0xf010000c**
    
    How does the boot loader decide how many sectors it must read in order to fetch the entire kernel from disk? Where does it find this information? ——**Boot loader reads the program header in the kernel ELF header and loads each section.**
    
    ```c
    // load each program segment (ignores ph flags)
    	ph = (struct Proghdr *) ((uint8_t *) ELFHDR + ELFHDR->e_phoff);
    	eph = ph + ELFHDR->e_phnum;
    	for (; ph < eph; ph++)
    		// p_pa is the load address of this segment (as well
    		// as the physical address)
    		readseg(ph->p_pa, ph->p_memsz, ph->p_offset);
    ```
    
- Ex5: Trace through the first few instructions of the boot loader again and identify the first instruction that would "break" or otherwise do the wrong thing if you were to get the boot loader's link address wrong.
    
    **The first instruction including hard-coded address in bootloader is at 0x7c2d:** 
    
    ```jsx
    ljmp  $PROT_MODE_CSEG, $protcseg
    
    **// when modify ld address in boot/Makefrag into 0x0,
    // instruction at 0x7c2d will change to a wrong ljmp**
    ljmp  $0xffc8,&0x80032
    ```
    
- Ex6: Examine the 8 words of memory at 0x00100000 at the point the BIOS enters the boot loader ——**They are all zeros.**
    
    and then again at the point the boot loader enters the kernel
    
    **The kernel code will be settled begining at 0x100000.**
    
    ```c
    (gdb) x/8xi 0x100000
       0x100000:    add    0x1bad(%eax),%dh
       0x100006:    add    %al,(%eax)
       0x100008:    decb   0x52(%edi)
       0x10000b:    in     $0x66,%al
       0x10000d:    movl   $0xb81234,0x472
       0x100017:    and    %dl,(%ecx)
       0x100019:    add    %cl,(%edi)
       0x10001b:    and    %al,%bl
    (gdb) x/8x 0x100000
    0x100000:       0x1badb002      0x00000000      0xe4524ffe    0x7205c766
    0x100010:       0x34000004      0x2000b812      0x220f0011    0xc0200fd8
    (gdb)
    ```
    
- Ex7: Use QEMU and GDB to trace into the JOS kernel and stop at the movl %eax, %cr0. Examine memory at 0x00100000 and at 0xf0100000. Now, single step over that instruction using the stepi GDB command. Again, examine memory at 0x00100000 and at 0xf0100000. Make sure you understand what just happened.
    
    **Before and after paging enabled:**
    
    ```c
    => 0x100025:    mov    %eax,%cr0
    0x00100025 in ?? ()
    (gdb) x/8w 0xf0100000
    0xf0100000 <_start-268435468>:  0x00000000      0x00000000    0x00000000       0x00000000
    0xf0100010 <entry+4>:   0x00000000      0x00000000      0x00000000     0x00000000
    (gdb) si
    => 0x100028:    mov    $0xf010002f,%eax
    0x00100028 in ?? ()
    (gdb) x/8w 0x100000
    0x100000:       0x1badb002      0x00000000      0xe4524ffe    0x7205c766
    0x100010:       0x34000004      0x2000b812      0x220f0011    0xc0200fd8
    (gdb) x/8w 0xf0100000
    0xf0100000 <_start-268435468>:  0x1badb002      0x00000000    0xe4524ffe       0x7205c766
    0xf0100010 <entry+4>:   0x34000004      0x2000b812      0x220f0011     0xc0200fd8
    ```
    
    What is the ﬁrst instruction after the new mapping is established that would fail to work properly if the mapping weren't in place?
    
    ```jsx
    **# To comment it will cause memory access error after PG enabled(at 0x100023)**
    movl	$(RELOC(entry_pgdir)), %eax
    movl	%eax, %cr3
    movl	%cr0, %eax
    orl	$(CR0_PE|CR0_PG|CR0_WP), %eax
    **# when it being commentted, jmp *%eax(at 0x100028) will jump to 0xf010002c,
    # which is not mapped to 0x10002c.
    # So, the region beginning at 0xf010002c is full of 0x0(add %al,(%eax))** 
    movl	%eax, %cr0
    
    mov	$relocated, %eax
    jmp	*%eax
    ```
    
- Ex8: We have omitted a small fragment of code - the code necessary to print octal numbers using patterns of the form "%o". Find and ﬁll in this code fragment.
    
    ```c
    // (unsigned) octal
    case 'o':
    	// Replace this with your code.
    	num = getuint(&ap, lflag);
    	base = 8;
    	goto number;
    ```
    
    Explain the interface between printf.c and console.c. Speciﬁcally, what function does console.c export? How is this function used by printf.c?
    
    ***console.c* exports following functions to *printf.c(/h)*，while the function *cputchar* is used as a func-ptr parameter by the function *vprintfmt* in *printf.c*.**
    
    ```c
    void cputchar(int c);
    int	getchar(void);
    int	iscons(int fd);
    ```
    
    Explain the following from console.c:
    
    ```c
    // What is the purpose of this?
    if (crt_pos >= CRT_SIZE) { 		
    	int i;  		
    	memmove(crt_buf, crt_buf + CRT_COLS, 
    				(CRT_SIZE - CRT_COLS) * sizeof(uint16_t));
     	for (i = CRT_SIZE - CRT_COLS; i < CRT_SIZE; i++) 			
    		crt_buf[i] = 0x0700 | ' '; 		
    	crt_pos -= CRT_COLS;
    }
    ```
    
    **When the screen is full, scroll down a single row to show newer information.**
    
    For the following questions you might wish to consult the notes for Lecture 2. These notes cover GCC's calling convention on the x86. Trace the execution of the following code step-by-step:
    
    ```c
    int x = 1, y = 3, z = 4;
    cprintf("x %d, y %x, z %d\n", x, y, z);
    ```
    
    In the call to cprintf(), to what does fmt point? To what does ap point?
    
    **Parameter** ***fmt* points to the format string, *ap* points to the variable arguments following *fmt*.**
    
    List (in order of execution) each call to cons_putc, va_arg, and vcprintf. For cons_putc, list its argument as well. For va_arg, list what ap points to before and after the call. For vcprintf list the values of its two arguments.
    
    **When the code above is loaded in memory, its execution seems to be:** 
    
    ```c
    0xf01000dc <+50>:    push   $0x4  ;**va_end**
    0xf01000de <+52>:    push   $0x3
    0xf01000e0 <+54>:    push   $0x1  ;**va_start**
    0xf01000e2 <+56>:    lea    -0xf751(%ebx),%eax ;**&fmt = 0xf0101bd7**
    0xf01000e8 <+62>:    push   %eax
    0xf01000e9 <+63>:    call   0xf0100b03 <cprintf>
    ```
    
    **Inside *call cprintf,* the following functions has been invoked:**
    
    ```c
    cprintf (fmt=0xf0101bd7 "x %d, y %d, z %d\n")
    vprintfmt (putch=0xf0100ac1 <putch>, putdat=0xf010ff8c, fmt=0xf0101bd7 "x %d, y %d, z %d\n", 
        ap=0xf010ffc4 "\001")
    cons_putc (c=120)
    cons_putc (c=32)
    printnum (putch=0xf010ff8c, putdat=0xf0100ac1 <putch>, num=1, base=10, width=-1, padc=32)
    ...
    cons_putc (c=52)
    cons_putc (c=10)
    ```
    
    Run the following code. What is the output? Explain how this output is arrived at in the step-by-step manner of the previous exercise.
    
    ```c
    unsigned int i = 0x00646c72;
    cprintf("H%x Wo%s", 57616, &i); //57616=0xe110
    
    **************** in GDB **********************
    (gdb) p ap
    $14 = (va_list) 0xf010ffec "rld"   ;0x72,0x6c,0x64,0x00(little endian)
    ```
    
    **The output is 'He110 World'.**
    
    The output depends on that fact that the x86 is little-endian. If the x86 were instead big-endian
    what would you set i to in order to yield the same output? Would you need to change 57616 to a diﬀerent value?
    
    **i = 0x726c6400, however 57616 won't be needed to change because it's only treated as a numeric value when printing it.**
    
    In the following code, what is going to be printed after 'y='? (note: the answer is not a speciﬁc
    value.) Why does this happen?
    
    ```c
    cprintf("x=%d y=%d", 3);
    ```
    
    **The "y=" depends on the value stored above the *ap* (3) —— *cprintf* (actually *vprintfmt*) will find what to replace the fmt"%" in its call stack.**
    
    Let's say that GCC changed its calling convention so that it pushed arguments on the stack in declaration order, so that the last argument is pushed last. How would you have to change cprintf or its interface so that it would still be possible to pass it a variable number of arguments?
    
    **Add an additional argument (uint, last arg) to indicate the number of va_arg.**
    
- Ex9: Determine where the kernel initializes its stack, and exactly where in memory its stack is located.
    
    **Initialization——in *entry.S*:** 
    
    ```jsx
    # Clear the frame pointer register (EBP)
    # so that once we get into debugging C code,
    # stack backtraces will be terminated properly.
    movl $0x0,%ebp # nuke frame pointer
    # Set the stack pointer
    movl $(bootstacktop),%esp
    ```
    
    **Location——in kernel.asm: $esp=0xf0111000**
    
    ```jsx
    # Set the stack pointer
    movl	$(bootstacktop),%esp
    f0100034:	bc 00 10 11 f0       	**mov    $0xf0111000,%esp**
    ```
    
    How does the kernel reserve space for its stack? And at which "end" of this reserved area is the stack pointer initialized to point to? **——in *entry.S*:  ****
    
    ```jsx
    .data
    ###################################################################
    # boot stack
    ###################################################################
    	**.p2align	PGSHIFT**		# force page alignment
    	.globl		bootstack
    **bootstack**:
    	**.space		KSTKSIZE**
    	.globl		bootstacktop   
    **bootstacktop**:
    ```
    
- Ex10: To become familiar with the C calling conventions on the x86, find the address of the test_backtrace function in obj/kern/kernel.asm, set a breakpoint there, and examine what happens each time it gets called after the kernel starts. How many 32-bit words does each recursive nesting level of test_backtrace push on the stack, and what are those words?
    
    ```jsx
    // Test the stack backtrace function
    void test_backtrace(int x){
    f0100040:	f3 0f 1e fb          	endbr32
    **f0100044:	55                   	push   %ebp
    f0100045:	89 e5                	mov    %esp,%ebp
    f0100047:	56                   	push   %esi
    f0100048:	53                   	push   %ebx**
    f0100049:	e8 e6 01 00 00       	call   f0100234 <__x86.get_pc_thunk.bx>
    f010004e:	81 c3 ba 22 01 00    	add    $0x122ba,%ebx
    f0100054:	8b 75 08             	mov    0x8(%ebp),%esi
    	cprintf("entering test_backtrace %d\n", x);
    f0100057:	83 ec 08             	sub    $0x8,%esp
    f010005a:	56                   	push   %esi
    f010005b:	8d 83 d8 f8 fe ff    	lea    -0x10728(%ebx),%eax
    f0100061:	50                   	push   %eax
    f0100062:	e8 f2 0a 00 00       	call   f0100b59 <cprintf>
    	if (x > 0)
    f0100067:	83 c4 10             	add    $0x10,%esp
    f010006a:	85 f6                	test   %esi,%esi
    f010006c:	7e 29                	jle    f0100097 <test_backtrace+0x57>
    		test_backtrace(x-1);
    f010006e:	83 ec 0c             	sub    $0xc,%esp
    f0100071:	8d 46 ff             	lea    -0x1(%esi),%eax
    f0100074:	50                   	push   %eax
    **f0100075:	e8 c6 ff ff ff       	call   f0100040 <test_backtrace>**
    f010007a:	83 c4 10             	add    $0x10,%esp
    	else
    		mon_backtrace(0, 0, 0);
    	cprintf("leaving test_backtrace %d\n", x);
    f010007d:	83 ec 08             	sub    $0x8,%esp
    f0100080:	56                   	push   %esi
    f0100081:	8d 83 f4 f8 fe ff    	lea    -0x1070c(%ebx),%eax
    f0100087:	50                   	push   %eax
    f0100088:	e8 cc 0a 00 00       	call   f0100b59 <cprintf>
    f010008d:	83 c4 10             	add    $0x10,%esp
    f0100090:	8d 65 f8             	lea    -0x8(%ebp),%esp
    f0100093:	5b                   	pop    %ebx
    f0100094:	5e                   	pop    %esi
    f0100095:	5d                   	pop    %ebp
    f0100096:	c3                   	ret    
    		mon_backtrace(0, 0, 0);
    f0100097:	83 ec 04             	sub    $0x4,%esp
    f010009a:	6a 00                	push   $0x0
    f010009c:	6a 00                	push   $0x0
    f010009e:	6a 00                	push   $0x0
    **f01000a0:	e8 74 08 00 00       	call   f0100919 <mon_backtrace>**
    f01000a5:	83 c4 10             	add    $0x10,%esp
    f01000a8:	eb d3                	jmp    f010007d <test_backtrace+0x3d>
    ```
    
    **In GDB debug:**
    
    ```jsx
    0xf010013c <+146>:   movl   $0x5,(%esp)
    **0xf0100143 <+153>:   call   0xf0100040 <test_backtrace> ;breakpoint
    ......
    <first:test_backtrace(x=5)>
    (gdb) x/4w $esp
    0xf010ffcc:     0xf010012c      0x00000005      0x00001aac      0xf010ffec
    (gdb) info r
    eax            0x0                 0
    ecx            0x3d4               980
    edx            0x3d5               981
    ebx            0xf0111308          -267316472
    esp            0xf010ffcc          0xf010ffcc
    ebp            0xf010fff8          0xf010fff8
    esi            0x10094             65684
    edi            0x0                 0
    eip            0xf0100040          0xf0100040 <test_backtrace>
    ...
    <second:test_backtrace(x=4)>
    (gdb) info r
    eax            0x4                 4
    ecx            0x3d4               980
    edx            0x3d5               981
    ebx            0xf0111308          -267316472
    esp            0xf010ffac          0xf010ffac
    ebp            0xf010ffc8          0xf010ffc8
    esi            0x5                 5
    edi            0x0                 0
    eip            0xf0100040          0xf0100040 <test_backtrace>
    (gdb) x/16w $esp
    0xf010ffac:   0xf010007a      0x00000004      0x00000005      0x00000000
    0xf010ffbc:   0xf010004e      0xf0111308      0x00010094      0xf010fff8
    0xf010ffcc:   0xf010012c      0x00000005      0x00001aac      0xf010ffec
    0xf010ffdc:   0x00000000      0x00000000      0x00000000      0x00000000
    ...
    <third:test_backtrace(x=3)>
    (gdb) info r
    eax            0x3                 3
    ecx            0x3d4               980
    edx            0x3d5               981
    ebx            0xf0111308          -267316472
    esp            0xf010ff8c          0xf010ff8c
    ebp            0xf010ffa8          0xf010ffa8
    esi            0x4                 4
    edi            0x0                 0
    eip            0xf0100040          0xf0100040 <test_backtrace>
    (gdb) x/20w $esp
    
    0xf010ff8c:   0xf010007a      0x00000003      0x00000004      0x00000000
    0xf010ff9c:   0xf010004e      0xf0111308      0x00000005      0xf010ffc8
    0xf010ffac:   0xf010007a      0x00000004      0x00000005      0x00000000
    0xf010ffbc:   0xf010004e      0xf0111308      0x00010094      0xf010fff8
    0xf010ffcc:   0xf010012c      0x00000005      0x00001aac      0xf010ffec
    ......**
    ```
    
    **Apperantly each *test_backtrace* call will get 8 32-bit words pushed in the stack:** 
    
    ```jsx
    return address
    caller test_backtrace's ebp(bottom of call stack frame)
    caller test_backtrace's esi
    caller's ebx
    garbage value
    garbage value
    garbage value(previous pushed esi for *cprintf*)
    callee test_backtrace's argument *x* (overwrite *fmt for cprintf*)
    ```
    
- Ex11: Implement the backtrace function as specified above.
    
    ```c
    int mon_backtrace(int argc, char **argv, struct Trapframe *tf){
    	uint32_t ebp = read_ebp();
    	while(ebp){
    		uint32_t eip = (uint32_t)*((int *)ebp + 1);
    		cprintf("ebp x eip x args",ebp,eip);
    		int *args = (int *)ebp + 2;
    		for (int i = 0; i <= 5; i++)
    			cprintf("%08.x ",args[i]);
    		cprintf("\n");
    		ebp = *(uint32_t *)ebp;
    	}
    	return 0;
    }
    ```
    
    **Result:** 
    
    ```c
    entering test_backtrace 5
    entering test_backtrace 4
    entering test_backtrace 3
    entering test_backtrace 2
    entering test_backtrace 1
    entering test_backtrace 0
    ebp:0xf0110f08, eip:0xf01000a5, args:0 0 0 f010004e f0112308 1 
    ebp:0xf0110f28, eip:0xf010007a, args:0 1 f0110f68 f010004e f0112308 2 
    ebp:0xf0110f48, eip:0xf010007a, args:1 2 f0110f88 f010004e f0112308 3 
    ebp:0xf0110f68, eip:0xf010007a, args:2 3 f0110fa8 f010004e f0112308 4 
    ebp:0xf0110f88, eip:0xf010007a, args:3 4 0 f010004e f0112308 5 
    ebp:0xf0110fa8, eip:0xf010007a, args:4 5 0 f010004e f0112308 10094 
    ebp:0xf0110fc8, eip:0xf010012c, args:5 1aac f0110fec 0 0 0 
    ebp:0xf0110ff8, eip:0xf010003e, args:3 1003 2003 3003 4003 5003 
    leaving test_backtrace 0
    leaving test_backtrace 1
    leaving test_backtrace 2
    leaving test_backtrace 3
    leaving test_backtrace 4
    leaving test_backtrace 5
    ```
    
- Ex12: Modify your stack backtrace function to display, for each eip, the function name, source file name, and line number corresponding to that eip. In `debuginfo_eip`, where do __STAB_* come from? This question has a long answer; to help you to discover the answer, here are some things you might want to do:
    - look in the file *kern/kernel.ld* for  __STAB_*
    - run  **objdump -h obj/kern/kernel**
    - run  **objdump -G obj/kern/kernel**
    - run **gcc -pipe -nostdinc -O2 -fno-builtin -I. -MD -Wall -Wno-format -DJOS_KERNEL -gstabs -c -S kern/init.c**, and look at init.s.
    
    **[STAB](Lab%201%20report/STAB%2074214038fc664f8f93d74911673f78d5.md)=symbol table, it restore all the symbols in the ELF file *kernel.*** 
    
    ```bash
    # objdump -h obj/kern/kernel
    kernel:     file format elf32-i386
    
    Sections:
    Idx Name          Size      VMA       LMA       File off  Algn
      0 .text         00001bcd  f0100000  00100000  00001000  2**4
                      CONTENTS, ALLOC, LOAD, READONLY, CODE
      1 .rodata       000006fc  f0101be0  00101be0  00002be0  2**5
                      CONTENTS, ALLOC, LOAD, READONLY, DATA
      **2 .stab         0000441d  f01022dc  001022dc  000032dc  2**2
                      CONTENTS, ALLOC, LOAD, READONLY, DATA
      3 .stabstr      00001992  f01066f9  001066f9  000076f9  2**0
                      CONTENTS, ALLOC, LOAD, READONLY, DATA**
      4 .data         00009300  f0109000  00109000  0000a000  2**12
                      CONTENTS, ALLOC, LOAD, DATA
      5 .got          00000008  f0112300  00112300  00013300  2**2
                      CONTENTS, ALLOC, LOAD, DATA
      6 .got.plt      0000000c  f0112308  00112308  00013308  2**2
                      CONTENTS, ALLOC, LOAD, DATA
    	...
      9 .bss          00000648  f0114060  00114060  00015060  2**5
                      CONTENTS, ALLOC, LOAD, DATA
    	...
    
    # objdump -G obj/kern/kernel //print SYMBOL TABLE in kernel ELF
    kernel:     file format elf32-i386
    Contents of .stab section:
    
    Symnum n_type n_othr n_desc n_value  n_strx String
    -1     HdrSym 0      1452   00001991 1
    0      SO     0      0      f0100000 1      {standard input}
    1      SOL    0      0      f010000c 18     kern/entry.S
    2      SLINE  0      44     f010000c 0
    3      SLINE  0      57     f0100015 0
    4      SLINE  0      58     f010001a 0
    5      SLINE  0      60     f010001d 0
    6      SLINE  0      61     f0100020 0
    ...
    14     SO     0      2      f0100040 31     kern/entrypgdir.c
    15     OPT    0      0      00000000 49     gcc2_compiled.
    16     LSYM   0      0      00000000 64     int:t(0,1)=r(0,1);-2147483648;2147483647;
    17     LSYM   0      0      00000000 106    char:t(0,2)=r(0,2);0;127;
    18     LSYM   0      0      00000000 132    long int:t(0,3)=r(0,3);-2147483648;2147483647;
    19     LSYM   0      0      00000000 179    unsigned int:t(0,4)=r(0,4);0;4294967295;
    20     LSYM   0      0      00000000 220    long unsigned int:t(0,5)=r(0,5);0;4294967295;
    ...
    41     BINCL  0      0      000159c2 956    ./inc/mmu.h
    42     BINCL  0      0      000060d4 968    ./inc/types.h
    ...
    ```
    
    **After running *gcc -pipe -nostdinc -O2 -fno-builtin -I. -MD -Wall -Wno-format -DJOS_KERNEL -gstabs -c -S kern/init.c*, two new files *init.d init.s was created.***
    
    ```c
    //init.d
    init.o: kern/init.c inc/stdio.h inc/stdarg.h inc/string.h inc/types.h \
     inc/assert.h kern/monitor.h kern/console.h
    
    //init.s
    .file	"init.c"
    	.stabs	"kern/init.c",100,0,2,.Ltext0
    	.text
    .Ltext0:
    	.stabs	"gcc2_compiled.",60,0,0,0
    	.stabs	"int:t(0,1)=r(0,1);-2147483648;2147483647;",128,0,0,0
    	.stabs	"char:t(0,2)=r(0,2);0;127;",128,0,0,0
    	.stabs	"long int:t(0,3)=r(0,3);-9223372036854775808;9223372036854775807;",128,0,0,0
    	.stabs	"unsigned int:t(0,4)=r(0,4);0;4294967295;",128,0,0,0
    	.stabs	"long unsigned int:t(0,5)=r(0,5);0;-1;",128,0,0,0
    	.stabs	"__int128:t(0,6)=r(0,6);0;-1;",128,0,0,0
    	.stabs	"__int128 unsigned:t(0,7)=r(0,7);0;-1;",128,0,0,0
    	.stabs	"long long int:t(0,8)=r(0,8);-9223372036854775808;9223372036854775807;",128,0,0,0
    ...
    //can translate to the output form like 'objdump -G'
    ```
    
    Complete the implementation of `debuginfo_eip` by inserting the call to `stab_binsearch` to find the line number for an address.
    
    **Add codes in *kdebug.c*:**
    
    ```c
    stab_binsearch(stabs, &lline, &rline, N_SLINE, addr);
    info->eip_line = stabs[lline].n_desc;
    ```
    
    **Modify function *mon_backtrace* in *monitor.c*:** 
    
    ```c
    //modify struct Command commands for backtrace display in monitor
    static struct Command commands[] = {
    	{ "help", "Display this list of commands", mon_help },
    	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
    	{ "backtrace", "Backtrace current function callstack", mon_backtrace },
    };
    
    int
    mon_backtrace(int argc, char **argv, struct Trapframe *tf)
    {
    	uint32_t ebp = read_ebp();
    	cprintf(F_yellow"start backtrace\n");//colored terminal
    	char* color[4] = {F_magenta,F_cyan,F_red,F_blue};
    	uint32_t count = 0;
    	while (ebp){
    		count = count % 4;
    		cprintf(color[count]);
    		uint32_t eip = (uint32_t)*((int *)ebp + 1);
    		cprintf("ebp:0x%08x, eip:0x%08x, args:",ebp,eip);
    		int *args = (int *)ebp + 2;
    		for (int i=0;i<4;i++)
    			cprintf("%x ",args[i]);
    		cprintf("\n");
    		struct Eipdebuginfo info;
    		debuginfo_eip(eip,&info);
    		cprintf("\t%s:%d: %.*s+%d\n",
    			info.eip_file,info.eip_line,
    			info.eip_fn_namelen,info.eip_fn_name,eip-info.eip_fn_addr);
    		ebp = *(uint32_t *)ebp;
    		count ++;
    	}
    	cprintf(ATTR_OFF);
    
    	return 0;
    }
    ```
    
    **The final backtrace function works properly.**
    
    ```c
    entering test_backtrace 5
    entering test_backtrace 4
    entering test_backtrace 3
    entering test_backtrace 2
    entering test_backtrace 1
    entering test_backtrace 0
    start backtrace ebp:0xf0110f08, eip:0xf01000a5, args:0 0 0 f010004e
                   kern/init.c:18: test_backtrace+101
    ebp:0xf0110f28, eip:0xf010007a, args:0 1 f0110f68 f010004e
                   kern/init.c:16: test_backtrace+58
    ebp:0xf0110f48, eip:0xf010007a, args:1 2 f0110f88 f010004e
                   kern/init.c:16: test_backtrace+58
    ebp:0xf0110f68, eip:0xf010007a, args:2 3 f0110fa8 f010004e
                   kern/init.c:16: test_backtrace+58
    ebp:0xf0110f88, eip:0xf010007a, args:3 4 0 f010004e
                   kern/init.c:16: test_backtrace+58
    ebp:0xf0110fa8, eip:0xf010007a, args:4 5 0 f010004e
                   kern/init.c:16: test_backtrace+58
    ebp:0xf0110fc8, eip:0xf010013a, args:5 1aac f0110fec 0
                   kern/init.c:50: i386_init+144
    ebp:0xf0110ff8, eip:0xf010003e, args:3 1003 2003 3003
                   kern/entry.S:83: <unknown>+0
    leaving test_backtrace 0
    leaving test_backtrace 1
    leaving test_backtrace 2
    leaving test_backtrace 3
    leaving test_backtrace 4
    leaving test_backtrace 5
    ...
    K> backtrace
    start backtrace
    ebp:0xf0110f48, eip:0xf0100b92, args:1 f0110f70 0 f0100bfa 
                 kern/monitor.c:143: monitor+343
    ebp:0xf0110fc8, eip:0xf0100155, args:0 1aac f0110fec 0 
                 kern/init.c:53: i386_init+171
    ebp:0xf0110ff8, eip:0xf010003e, args:3 1003 2003 3003 
                 kern/entry.S:83: <unknown>+0
    K>
    ```
    

### **CHALLENGE:**

Enhance the console to allow text to be printed in diﬀerent colors. The traditional way to do this is to **make it interpret ANSI escape sequences embedded in the text strings printed to the console**, but you may use any mechanism you like. There is plenty of information on the 6.828 reference page and elsewhere on the web on programming the VGA display hardware. If you're feeling really adventurous, you could try switching the VGA hardware into a graphics mode and making the console draw text onto the graphical frame buﬀer.

**The simplest way is using ANSI escape sequence. The overall form is like:** 

```
\33[字背景颜色;字体颜色m字符串\33[0m
```

1. **Modify *inc/stdio.h* to add ANSI escape sequences (begin with \33[).**
    
    ```c
    #define ESC       "\33"
    #define ATTR_OFF ESC"[0m"                      // default: disable all atrributes
    
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
    ```
    
2. **Modify kern/init.c: add test code as follow**
    
    ```c
    //test for exercise 8
    //int x = 1, y = 3, z = 4;
    //cprintf("x %d, y %d, z %d\n", x, y ,z);
    unsigned int i = 0x00646c72;
    cprintf("H%x Wo%s\n", 57616, &i);
    cprintf("x=%d,y=%d\n",3);
    
    //colored terminal
    cprintf(F_red"red front" B_blue"blue back\n");
    
    cprintf("6828 decimal is %o octal!\n", 6828);
    
    cprintf(F_magenta"magenta front" B_cyan"cyan back\n");
    // Test the stack backtrace function (lab 1 only)
    test_backtrace(5);
    	
    cprintf(ATTR_OFF"default terminal\n");
    // Drop into the kernel monitor.
    while (1)
    	monitor(NULL);
    ```
    
    **The colored terminal in *stdin* :** 
    
    ![Untitled](Lab%201%20report/Untitled.png)
    
3. **However, the QEMU console didn't perform congruously with serial(stdin). It's caused because the ANSI escape sequences weren't supported by *cga_putc* currently, only *stdin* can get ANSI esc-sequences.**
    
    ```c
    // output a character to the console
    static void
    cons_putc(int c)
    {
    	serial_putc(c);    //to the serial == stdin (√)
    	lpt_putc(c);       //to the parallel == stdin (√)
    	cga_putc(c);       //to the CGA/VGA display == QEMU console (×)
    }
    ```
    
4. **Hence we should modify *cga_putc* to support ANSI esc-sequences in QEMU console.**
    
    **Firstly, look inside the VGA Text Mode. To print a character to the screen in VGA text mode, one has to write it to the `VGA text buffer`(a two-dimensional typically (25, 80) array which is directly rendered to the screen). Each array entry describes a single screen character through the following format:**
    
    ![1.png](Lab%201%20report/1.png)
    
    - **The first byte represents the character that should be printed in the [ASCII encoding](https://en.wikipedia.org/wiki/ASCII)  (actually *[code page 437](https://en.wikipedia.org/wiki/Code_page_437)*).**
    - **The second byte defines how the character is displayed. The first four bits define the foreground color, the next three bits the background color, and the last bit whether the character should blink. The following colors are available:**
    
    ![1.png](Lab%201%20report/1%201.png)
    
    **(It's quite diffrent from the ANSI color esc-sequences, so when handling ANSI esc-seq a map between the two color lists is required.)**
    
    ```c
    // color map
    color        ANSI  VGA
    ------------------------
    black         0    0x0
    red           1    0x4
    green         2    0x2
    yellow        3    0xe
    blue          4    0x1
    magenta       5    0x5
    cyan          6    0x3
    white         7    0xf
    ```
    
    **Now, we can add colored character output in *console.c*:** 
    
    ```c
    //old cga_putc will rename to cga_putc_legacy
    static void
    cga_putc_legacy(int c)
    {
    	// if no attribute given, then use black on white
    	if (!(c & ~0xFF))
    		c |= 0x0700;
    
    	switch (c & 0xff) {
    	case '\b':
    		if (crt_pos > 0) {
    			crt_pos--;
    			crt_buf[crt_pos] = (c & ~0xff) | ' ';
    		}
    		break;
    	case '\n':
    		crt_pos += CRT_COLS;
    		/* fallthru */
    	case '\r':
    		crt_pos -= (crt_pos % CRT_COLS);
    		break;
    	case '\t':
    		cons_putc(' ');
    		cons_putc(' ');
    		cons_putc(' ');
    		cons_putc(' ');
    		cons_putc(' ');
    		break;
    	default:
    		crt_buf[crt_pos++] = c;		/* write the character */
    		break;
    	}
    
    	// the purpose of this snippet
    	// Scorll down a single row for newer contents when the console is full
    	if (crt_pos >= CRT_SIZE) {
    		int i;
    		memmove(crt_buf, crt_buf + CRT_COLS, (CRT_SIZE - CRT_COLS) * sizeof(uint16_t));
    		for (i = CRT_SIZE - CRT_COLS; i < CRT_SIZE; i++)
    			crt_buf[i] = 0x0700 | ' ';
    		crt_pos -= CRT_COLS;
    	}
    	...
    }
    
    // number in the esc_seq
    static int
    isdigit(int c)
    {
    	return (c >= '0' && c <='9');
    }
    
    //unique atoi() for ansi_esc_handler
    static int
    atoi(const char *s)
    {
    	int res = 0;
    	for (int i = 0; isdigit(s[i]);i++)
    		res = res * 10 + (s[i] - '0');
    	return res;
    }
    
    //only handle the color attributes(maybe it can use for FONTS/UNDERLINE/BOLD)
    static void
    ansi_esc_seq_handler(const char* buf, int *attr)
    {
    	// mapping color codes in ANSI esc-sequence and thoes for VGA text mode
    	//see in stdio.h
    	static int esc_cgap[]={0x0, 0x4, 0x2, 0xe, 0x1, 0x5, 0x3, 0x7};
    	int tmp = *attr;
    	int n = atoi(buf);
    	if (n >= 30 && n <= 37)
    		tmp = (tmp & ~(0x0f)) | esc_cgap[n - 30];
    	else if (n >= 40 && n <= 47)
    		tmp = (tmp & ~(0xf0)) | (esc_cgap[n - 40] << 4);
    	else if (n == 0)
    		tmp = 0x07;
    
    	*attr = tmp;
    }
    ```
    
    **To manipulate the terminal easily, we add some interfaces in *stdio.h* and implement them in *printf.c*:**
    
    ```c
    //stdio.h
    enum {
        COLOR_BLACK = 0,
        COLOR_RED,
        COLOR_GREEN,
        COLOR_YELLOW,
        COLOR_BLUE,
        COLOR_MAGENTA,
        COLOR_CYAN,
        COLOR_WHITE,
        COLOR_NUM,
    };
     
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
    ```
    
    ```c
    //printf.c
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
    ```
    
    **We implement color settings in several functions in *init.c* and *monitor.c.* The result is as follows:** 
    
    ![Untitled](Lab%201%20report/Untitled%201.png)
    

## **Optimization**

1. Serveral *backspace* can make the cursor in *stdin* terminal get back through the command line "K> ", resulting that *runcmd* can't resolve the command line(*buf*) correctly. 
    
    ![1.png](Lab%201%20report/1%202.png)
    
    ### **Analyzation**
    
    **We analyze what character is inputed when we press *backspace——*the revelant procedure is *readline* in *readline.c*, toggling a breakpoint in it and tracing user's input. The result is that DEL(127/0x7F) is the character corresponding to *backspace*.** 
    
    ```c
    => 0xf01027cd <readline+163>:	call   0xf0100929 <getchar>
    18			c = getchar();
    (gdb) 
    => 0xf01027d4 <readline+170>:	test   %eax,%eax
    19			if (c < 0) {
    (gdb) p c
    $1 = 127
    ```
    
    Look inside the function *readline:* 
    
    ```c
    //readline.c
    char *
    readline(const char *prompt)
    {
    	int i, c, echoing;
    
    	if (prompt != NULL)
    		cprintf("%s", prompt);
    
    	i = 0;
    	echoing = iscons(0);
    	while (1) {
    		c = getchar();
    		if (c < 0) {
    			cprintf("read error: %e\n", c);
    			return NULL;
    		**} else if ((c == '\b' || c == '\x7f') && i > 0) {     //(1)**
    			if (echoing)
    				cputchar('\b');
    			i--;
    		**} else if (c >= ' ' && i < BUFLEN-1) {    //(2)**
    			if (echoing)
    				cputchar(c);
    			buf[i++] = c;
    		} else if (c == '\n' || c == '\r') {
    			if (echoing)
    				cputchar('\n');
    			buf[i] = 0;
    			return buf;
    		}
    	}
    }
    ```
    
    **Now the reason is apperant: when we haven't input anything but *backspace*, the loacl variable *i* is 0, so the branch (1) is avoid; but DEL(0x7F) is larger than ' '(0x20), then the branch (2) is taken that '\b' still being output.** 
    
    ### **Solution**
    
    **Since '~' is the largest visible ASCII character, we modify the condition statement of branch (2) to avoid DEL getting in this branch:** 
    
    ```c
    //readline in readline.c
    		...
    		} else if ((c == '\b' || c == '\x7f') && i > 0) {     //(1)
    			if (echoing)
    				cputchar('\b');
    			i--;
    		**} else if (c >= ' ' && c <= '~' && i < BUFLEN-1) {    //(2)**
    			if (echoing)
    				cputchar(c);
    			buf[i++] = c;
    		...
    ```
    
2. Inconsistant text-deleting behavior between stdin terminal and QEMU console
    
    When *backspace* input, QEMU console can get cursor back and delete current character, since stdin(serial/parallel) can't perform same.
    
    Referring to [http://web.mit.edu/broder/Public/fixing-jos-serial.txt](http://web.mit.edu/broder/Public/fixing-jos-serial.txt), **the original implement of *serial_putc* simply push the char to COM_TX. '\b' only means cursor go back one step.** 
    
    ```c
    //console.c
    static void
    serial_putc(int c)
    {
    	int i;
    
    	for (i = 0;
    	     !(inb(COM1 + COM_LSR) & COM_LSR_TXRDY) && i < 12800;
    	     i++)
    		delay();
    	outb(COM1 + COM_TX, c);
    }
    ```
    
    There're at least 3 conventions for *backspace*: '\b', '\x7f', and "\033[3~".  Ideally, the convention would be that '\b' was backspace and '\x7f' was delete, like the ASCII table says it should be.  The more common convention is that '\x7f' is backspace and "\033[3~" is delete.
    
    ### **Solution**
    
    **Modify *serial_putc* to translate '\b' to '\b \b'.**
    
    ```c
    static void
    serial_putc(int c)
    {
    	int i;
    
    	for (i = 0;
    	     !(inb(COM1 + COM_LSR) & COM_LSR_TXRDY) && i < 12800;
    	     i++)
    		delay();
    	if ((char)c != '\b')
    		outb(COM1 + COM_TX, c);
    	else{
    		//outb(COM1 + COM_TX, 0x7f);
    		outb(COM1 + COM_TX, c);
    		outb(COM1 + COM_TX, ' ');
    		outb(COM1 + COM_TX, c);
    	}
    }
    ```
    
3. Cursor can't move by arrow keys. In user terminal, it displays like follows: 
    
    ![Untitled](Lab%201%20report/Untitled%202.png)
    
    Refer to [https://www.cnblogs.com/demonxian3/p/8963807.html](https://www.cnblogs.com/demonxian3/p/8963807.html), it's clear that when arrow keys pressed, the user terminal received ANSI esc-sequences like \33[A (up), \33[B (down), \33[C (right), \33[D (left).
    
    But QEMU console has a different behavior——no respond. It's necessary to use GDB to intercept the kbd_input.
    
    ![Untitled](Lab%201%20report/Untitled%203.png)
    
    ![Untitled](Lab%201%20report/Untitled%204.png)
    
    In CGA text mode, left arrow = 228, right arrow = 229, up arrow = 226, down arrow = 227
    
    These characters are all larger than '~'(126), so *readline* ignored them (modified for proper DEL perviously) 
    
    ### **Solution**
    
    **We should make left and right cursor move can behave correctly both in stdin terminal and QEMU console. The most confusing is that user terminal treat arrow key as esc-seq since QEMU console treat them as extended character.  To keep the inteface between readline.c and lower *console.c* (*getchar. cputchar*), only *readline* is modified.**
    
    ```c
    //readline.c
    
     // Handle the extended ASCII code inputed by console
     inline static void ext_char_handler(int c);
     // Handle the escape sequence inputed by serial (user-terminal)
     inline static void esc_seq_handler(void);
     
     // Move the cursor right
     inline static void move_right(void);
     // Move the cursor left
     inline static void move_left(void);
     
     // Flush buffer's [cur, tail) to the displays
     // and move the cursor back
     inline static void flush_buf(void);
     
     // Insert char to current cursor
     inline static void insert_char(int c);
     // Remove current cursor's char
     inline static void remove_char(void);
     // Terminate the input
     inline static void finish_input(void);
    
    // Current position of cursor
     static int cur;
     // Tail of buffer
     static int tail;
     
     static int echoing;
    ```
    
    **Unfortunately, we found that it's difficult to control the cursor with serial since '\b' is the only way to make cursor go back. So we can't make '\b' itself delete the character. Hence we scroll back to the pervious implement of *serial_putc* and modify the *cga_putc_legacy* to keey their behavior consistant.**  
    
    ```c
    //console.c
    
    static void
    cga_putc_legacy(int c) {
      // If no attributes are given,
      // then use a default with black character on a white background
      if (!(c & ~0xFF)) c |= 0x700;
      
      // Put the character
      switch (c & 0xFF) {
      case '\b':
        if (crt_pos > 0) {
          crt_pos--;
    			// To comment it will make easier to implement readline
          // crt_buf[crt_pos] = (c & ~0xFF) | ' ';
        }
        break;
      case '\n':
        crt_pos += CRT_COLS;
      case '\r':
        crt_pos -= (crt_pos % CRT_COLS);
        break;
      case '\t':
        cons_putc(' ');
        cons_putc(' ');
        cons_putc(' ');
        cons_putc(' ');
        break;
      default:
        crt_buf[crt_pos++] = c;
        break;
      }
    ```
    
    **Now it's convinient to move back the cursor without side effect. The final *readline.c* is as follows:** 
    
    **(In linux terminal, up arrow can trace previous input since down arrow will do the reverse——maybe clear the command line. We implement a prototype which can trace last input and clear current command line)**
    
    ```c
    //readline.c
    #include <inc/stdio.h>
    #include <inc/error.h>
    #include <inc/string.h>
     
     // Handle the extended ASCII code inputed by console
     inline static void handle_ext_ascii(int c);
     // Handle the escape sequence inputed by serial (user-terminal)
     inline static void handle_esc_seq(void);
     
     // Move the cursor right
     inline static void move_right(void);
     // Move the cursor left
     inline static void move_left(void);
     
     // Flush buffer's [cur, tail) to the displays
     // and move the cursor back
     inline static void flush_buf(void);
     
     // Insert char to current cursor
     inline static void insert_char(int c);
     // Remove current cursor's char
     inline static void remove_char(void);
     // Terminate the input
     inline static void end_input(void);
     
     #define BUFLEN 1024
     static char buf[BUFLEN];
     
     // Current position of cursor
     static int cur;
     // Tail of buffer
     static int tail;
     
     static int echoing;
     
     char *
     readline(const char *prompt)
     {
     	int c;
     
     	if (prompt != NULL)
     		cprintf("%s", prompt);
     
     	cur = tail = 0;
     	echoing = iscons(0);
     	while (1) {
     		c = getchar();
     		if (c < 0) {
     			cprintf("read error: %en", c);
     			return NULL;
     		} else if ((c == 'b' || c == 'x7f') && cur > 0) {
     			remove_char();
     		} else if (c >= ' ' && c <= '~' && tail < BUFLEN-1) {
     			// Must have c <= '~',
     			// because DEL(0x7f) is larger than '~'
     			// and it will be inputed when you push
     			// 'backspace' in user-terminal
     			insert_char(c);
     		} else if (c == 'n' || c == 'r') {
     			end_input();
     			return buf;
     		} else if (c == 'x1b') {
     			handle_esc_seq(); // only serial will input esc
     		} else if (c > 'x7f') {
     			handle_ext_ascii(c); // only console will input extended ascii
     		}
     	}
     }
     
     inline static void 
     flush_buf(void)
     {
     	for (int i = cur; i < tail; ++i)
     		cputchar(buf[i]);
     	for (int i = cur; i < tail; ++i)
     		cputchar('b'); // cursor move back
     }
     
     inline static void 
     insert_char(int c) 
     {
     	if (cur == tail) {
     		tail++, buf[cur++] = c;
     		if (echoing)
     			cputchar(c);
     	} else { // general case
     		memmove(buf + cur + 1, buf + cur, tail - cur);
     		buf[cur] = c, tail++;
     		if (echoing) 
     			flush_buf();
     		move_right();
     	}
     }
     
     inline static void 
     remove_char(void)
     {
     	if (cur == tail) {
     		cur--, tail--;
     		if (echoing)
     			cputchar('b'), cputchar(' '), cputchar('b');
     	} else { // general case
     		memmove(buf + cur - 1, buf + cur, tail - cur);
     		buf[tail - 1] = ' ';
     		move_left();
     		if (echoing)
     			flush_buf();
     		tail--;
     	}
     }
     
     inline static void 
     move_left(void)
     {
     	if (cur > 0) {
     		if (echoing)
     			cputchar('b');
     		cur--;
     	}
     }
     
     inline static void 
     move_right(void)
     {
     	if (cur < tail) {
     		if (echoing)
     			cputchar(buf[cur]);
     		cur++;
     	}
     }
     
     inline static void 
     end_input(void)
     {
     	if (echoing) {
     		for (; cur < tail; cputchar(buf[cur++]))
     			/* move the cursor to the tail */;
     		cputchar('n');
     	}
     	cur = tail;
     	buf[tail] = 0;
     }
     
     #define EXT_ASCII_LF 228
     #define EXT_ASCII_RT 229
     #define EXT_ASCII_UP 226
     #define EXT_ASCII_DN 227
     
     inline static void 
     handle_ext_ascii(int c)
     {
     	switch(c) {
     	case EXT_ASCII_LF:
     		move_left();
     		return;
     	case EXT_ASCII_RT: 
     		move_right();
     		return;
     	}
     	insert_char(c);
     }
     
     #define ESC_LF 'D'
     #define ESC_RT 'C'
     #define ESC_UP 'A'
     #define ESC_DN 'B'
     
     inline static void 
     handle_esc_seq(void)
     {
     	char a, b = 0;
     
     	a = getchar();
     	if (a == '[') {
     		switch(b = getchar()) {
     		case ESC_LF: 
     			move_left();
     			return;
     		case ESC_RT:
     			move_right();
     			return; 
     		}
     	}
     	insert_char(a);
     	if (b)
     		insert_char(b);
     }
    ```