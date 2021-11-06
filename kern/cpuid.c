#include <inc/stdio.h>
#include <inc/x86.h>
#include <inc/types.h>

unsigned int DBaseIndex, DFeInfo, DFeInfo2, DCPUBaseInfo;
unsigned int DFeIndex, DCPUExInfo;
unsigned int DOther[4], DTLB[4], DProceSN[2];
char cCom[13] = {0};
char cProStr[49];
static uint32_t saved = 0;

void
intel_detect(){
    unsigned int gabb;
    cpuid(0, &DBaseIndex, (uint32_t *)&cCom[0], (uint32_t *)&cCom[4],(uint32_t *) &cCom[8]);
    cpuid(1, &DCPUBaseInfo, &DFeInfo, &gabb, &DFeInfo2);
    cpuid(0x80000000, &DFeIndex, &gabb, &gabb, &gabb);
    cpuid(0x80000001, &DCPUExInfo, &gabb, &gabb, &gabb);
    cpuid(0x80000002, (uint32_t *)&cProStr[0], (uint32_t *)&cProStr[4], (uint32_t *)&cProStr[8], (uint32_t *)&cProStr[12]);
    cpuid(0x80000003, (uint32_t *)&cProStr[16], (uint32_t *)&cProStr[20], (uint32_t *)&cProStr[24], (uint32_t *)&cProStr[28]);
    cpuid(0x80000004, (uint32_t *)&cProStr[32], (uint32_t *)&cProStr[36], (uint32_t *)&cProStr[40], (uint32_t *)&cProStr[44]);    
    
    //extra info (except other)
    if (DBaseIndex >= 2){
        cpuid(2, &DTLB[0], &DTLB[2], &DTLB[3], &DTLB[4]);
    }
    if (DBaseIndex == 3){
        cpuid(3, &gabb, &gabb, &DProceSN[0], &DProceSN[1]);
    }
}

// show > 0: print to console, storage won't modify
// show < 0: no print, next call will refresh storage
// show = 0: nothing occured
void
print_cpuid(int show){
    if (!saved){
        intel_detect();
        saved = 1;
    }
    if (show > 0){
        unsigned long i;
        unsigned int j;
        highlight(1);
        set_fgcolor(COLOR_MAGENTA);
        cprintf("Vender ID: %s\n",cCom);
        cprintf("Processer Name: %s\n",cProStr);
        cprintf("CPU base info: Family:%x   Model:%x    Stepping ID:%x\n",
            (DCPUBaseInfo&0x0F00)>>8,(DCPUBaseInfo&0xF0)>>4,DCPUBaseInfo&0x0F);
        cprintf("CPU extra info:Family:%x   Model:%x    Stepping ID:%x\n",
            (DCPUExInfo&0x0F00)>>8,(DCPUExInfo&0xF0)>>4,DCPUExInfo&0x0F);
        set_fgcolor(COLOR_RED);
        cprintf("CPU count: 0x%x\n",DFeIndex&0xFF);
        cprintf("CLFLUSH line size: 0x%x\n",(DFeIndex&0xFF00)>>8);
        cprintf("Processor APIC physical ID: 0x%x\n",(DFeIndex&0xF000)>>24);
        if (DBaseIndex >= 2){
            cprintf( "CPU Cache & TLB Information: " );
            for(j = 0; j < 4; j++)
            {
            if( !(DTLB[j] & 0xFF) ) cprintf( "%.2x ", DTLB[j] & 0xFF );
            if( !((DTLB[j] & 0xFF) >> 8) ) cprintf( "%.2x ", (DTLB[j] & 0xFF00) >> 8 );
            if( !((DTLB[j] & 0xFF0000) >> 16) ) cprintf( "%.2x ",( DTLB[j] & 0xFF0000) >> 16);
            if( !((DTLB[j] & 0xFF000000) >> 24) ) cprintf( "%.2x ",( DTLB[j] & 0xFF000000) >> 24);
            }
            cprintf("\n");
        }
        set_fgcolor(COLOR_BLUE);
        if (DBaseIndex == 3){
            cprintf( "CPU Serial ID:    %x%x\n", DProceSN[0], DProceSN[1] );
        }
        cprintf( "FPU:  %d\t", DFeInfo2 & 0x00000001 ); 
        cprintf( "VME:  %d\t", (DFeInfo2 & 0x00000002 ) >> 1 );
        cprintf( "DE:   %d\n", (DFeInfo2 & 0x00000004 ) >> 2 );
        cprintf( "PSE:  %d\t", (DFeInfo2 & 0x00000008 ) >> 3 );
        cprintf( "TSC:  %d\t", (DFeInfo2 & 0x00000010 ) >> 4 );
        cprintf( "MSR:  %d\n", (DFeInfo2 & 0x00000020 ) >> 5 );
        cprintf( "PAE:  %d\t", (DFeInfo2 & 0x00000040 ) >> 6 );
        cprintf( "MCE:  %d\t", (DFeInfo2 & 0x00000080 ) >> 7 );
        cprintf( "CX8:  %d\n", (DFeInfo2 & 0x00000100 ) >> 8 );
        cprintf( "APIC: %d\t", (DFeInfo2 & 0x00000200 ) >> 9 );
        cprintf( "SEP:  %d\t", (DFeInfo2 & 0x00000800 ) >> 11 );
        cprintf( "MTRR: %d\n", (DFeInfo2 & 0x00001000 ) >> 12 );
        cprintf( "PGE:  %d\t", (DFeInfo2 & 0x00002000 ) >> 13 );
        cprintf( "MCA:  %d\t", (DFeInfo2 & 0x00004000 ) >> 14 );
        cprintf( "CMOV: %d\n", (DFeInfo2 & 0x00008000 ) >> 15 );
        cprintf( "PAT:  %d\t", (DFeInfo2 & 0x00010000 ) >> 16 );
        cprintf( "PSE36:%d\t", (DFeInfo2 & 0x00020000 ) >> 17 );
        cprintf( "PSN:  %d\n", (DFeInfo2 & 0x00040000 ) >> 18 );
        cprintf( "CLFSN:%d\t", (DFeInfo2 & 0x00080000 ) >> 19 );
        cprintf( "DS:   %d\t", (DFeInfo2 & 0x00200000 ) >> 21 );
        cprintf( "ACPI: %d\n", (DFeInfo2 & 0x00400000 ) >> 22 );
        cprintf( "MMX:  %d\t", (DFeInfo2 & 0x00800000 ) >> 23 );
        cprintf( "FXSR: %d\t", (DFeInfo2 & 0x01000000 ) >> 24 );
        cprintf( "SSE:  %d\n", (DFeInfo2 & 0x02000000 ) >> 25 );
        cprintf( "SSE2: %d\t", (DFeInfo2 & 0x04000000 ) >> 26 );
        cprintf( "SS:   %d\t", (DFeInfo2 & 0x08000000 ) >> 27 );
        cprintf( "TM:   %d\n", (DFeInfo2 & 0x20000000 ) >> 29 );
        set_fgcolor(COLOR_GREEN);
        cprintf("Other:\n");
        cprintf("--------------------------------------------------------\n");
        cprintf("In \t\tEAX\t\tEBX\t\tECX\t\tEDX");
        for( i = 0x80000004; i <= DFeIndex; ++i ){
            DOther[0] = DOther[1] = DOther[2] = DOther[3] = 0;
            cpuid(i,&DOther[0],&DOther[1],&DOther[2],&DOther[3]);
            cprintf( "\n0x%.8x   0x%.8x\t\t0x%.8x\t\t0x%.8x\t\t0x%.8x", i, DOther[0], DOther[1], DOther[2], DOther[3] );
        }
        cprintf("\n");
        reset_attr();
    }
    else if (show < 0)
        saved = 0;
}

