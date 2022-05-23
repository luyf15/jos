// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>
#include <inc/stdio.h>
#include <inc/assert.h>
#include <inc/.config>


extern volatile pte_t uvpt[];     // VA of "virtual page table"
extern volatile pde_t uvpd[];     // VA of current page directory
extern void _pgfault_upcall(void);

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
	addr = ROUNDDOWN(addr, PGSIZE);
	if (!(	(uvpd[PDX(addr)] & PTE_P) && 
            (uvpt[PGNUM(addr)] & PTE_P) && 
            (uvpt[PGNUM(addr)] & PTE_U) && 
            (uvpt[PGNUM(addr)] & PTE_COW) && 
            (err & FEC_WR)
		))
        panic(
            "[0x%08x] user page fault va 0x%08x ip 0x%08x: "
            "[%s, %s, %s]",
            sys_getenvid(),
            utf->utf_fault_va,
            utf->utf_eip,
            err & 4 ? "user" : "kernel",
            err & 2 ? "write" : "read",
            err & 1 ? "protection" : "not-present"
        );

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.
    if (r = sys_page_alloc(0, PFTEMP, PTE_P | PTE_U | PTE_W), r < 0)
        panic("page fault handler: %e", r);
    memmove(PFTEMP, addr, PGSIZE);

    if (r = sys_page_map(0, PFTEMP, 0, addr, PTE_P | PTE_U | PTE_W), r < 0)
        panic("page fault handler: %e", r);

    if (r = sys_page_unmap(0, PFTEMP), r < 0)
        panic("page fault handler: %e", r);

    return;
	// panic("pgfault not implemented");
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	int r;

	// LAB 4: Your code here.
	// panic("duppage not implemented");
	void *addr = (void *)(pn * PGSIZE);
	pte_t pte = uvpt[pn];

	assert((pte & PTE_P) && (pte & PTE_U));
	if ((pte & PTE_W) || (pte & PTE_COW)) {
		if ((r = sys_page_map(0, addr, envid, addr, PTE_P | PTE_U | PTE_COW)) < 0)
			return r;
		if ((r = sys_page_map(0, addr, 0, addr, PTE_P | PTE_U | PTE_COW)) < 0)
			return r;
	} else {
		if ((r = sys_page_map(0, addr, envid, addr, PTE_P | PTE_U)) < 0)
			return r;
	}
	return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error .
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
	// panic("fork not implemented");
	int r;
	envid_t child_eid;

	set_pgfault_handler(pgfault);

#ifdef BATCH_SYSCALL_FORK

	extern unsigned char end[];

	set_pgfault_handler(pgfault);
	if (!(child_eid = sys_fork(end)))
		thisenv = &envs[ENVX(sys_getenvid())];

#else
	if (!(child_eid = sys_exofork()))
		thisenv = &envs[ENVX(sys_getenvid())];
	else if (child_eid > 0) {
		// assume UTOP == UXSTACKTOP
		// don't copy the UXSTACK(aka pn<=UTOP/PGSIZE-1)
		for (unsigned pn = 0; pn < UTOP / PGSIZE - 1;) {
			pde_t pde = uvpd[pn / NPDENTRIES];
			if (!(pde & PTE_P))
				pn += NPDENTRIES;
			else {
				unsigned next = MIN(UTOP / PGSIZE - 1, pn + NPDENTRIES);
                for (; pn < next; ++pn) {
                    pte_t pte = uvpt[pn];
                    if ((pte & PTE_P) && (pte & PTE_U))
                        if (r = duppage(child_eid, pn), r < 0)
                            panic("fork: %e", r);
				}
			}
		}

        if (r = sys_page_alloc(child_eid, (void*)(UXSTACKTOP - PGSIZE), PTE_P | PTE_U | PTE_W), r < 0)
            panic("fork: %e", r);
        if (r = sys_env_set_pgfault_upcall(child_eid, _pgfault_upcall), r < 0)
            panic("fork: %e", r);
        if (r = sys_env_set_status(child_eid, ENV_RUNNABLE), r < 0)
            panic("fork: %e", r);
	}
#endif

	return child_eid;
}

// fork with a batch of syscall functions in one single syscall trap
// envid_t
// bfork(void)
// {
// 	extern unsigned char end[];
// 	envid_t child_eid;
// 	int r;

// 	set_pgfault_handler(pgfault);
// 	if (!(child_eid = sys_fork(end)))
// 		thisenv = &envs[ENVX(sys_getenvid())];

// 	return child_eid;
// }

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
