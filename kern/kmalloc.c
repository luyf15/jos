#include <kern/kmalloc.h>
#include <kern/pmap.h>
#include <inc/memlayout.h>
#include <inc/types.h>

free_area_t buddy_area[MAX_ORDER+1];	// Free list of buddy-allocating memory