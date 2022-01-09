/* See COPYRIGHT for copyright information. */

#ifndef JOS_KERN_PMAP_H
#define JOS_KERN_PMAP_H
#ifndef JOS_KERNEL
# error "This is a JOS kernel header; user programs should not #include it"
#endif

#include <inc/memlayout.h>
#include <inc/assert.h>

extern char bootstacktop[], bootstack[];

extern struct Page *pages;
extern size_t npages;

extern pde_t *kern_pgdir;

/* This macro takes a kernel virtual address -- an address that points above
 * KERNBASE, where the machine's maximum 256MB of physical memory is mapped --
 * and returns the corresponding physical address.  It panics if you pass it a
 * non-kernel virtual address.
 */
#define PADDR(kva) _paddr(__FILE__, __LINE__, kva)

static inline physaddr_t
_paddr(const char *file, int line, void *kva)
{
	if ((uint32_t)kva < KERNBASE)
		_panic(file, line, "PADDR called with invalid kva %08lx", kva);
	return (physaddr_t)kva - KERNBASE;
}

/* This macro takes a physical address and returns the corresponding kernel
 * virtual address.  It panics if you pass an invalid physical address. */
#define KADDR(pa) _kaddr(__FILE__, __LINE__, pa)

static inline void*
_kaddr(const char *file, int line, physaddr_t pa)
{
	if (PGNUM(pa) >= npages)
		_panic(file, line, "KADDR called with invalid pa %08lx", pa);
	return (void *)(pa + KERNBASE);
}

struct pmm_manager {
	const char *name;
	void (*init) (void);
	void (*page_init) (void);
	struct Page *(*alloc_pages) (size_t n, int alloc_flags);
	void (*free_pages) (struct Page * base, size_t n);
	size_t (*nr_free_pages) (void);
	void (*check) (void);
};

enum {
	// For page_alloc, zero the returned physical page.
	ALLOC_ZERO = 1<<0,
};

void	mem_init(void);

struct Page *alloc_pages(size_t n, int alloc_flags);
void	free_pages(struct Page *base, size_t n);
#define alloc_page(alloc_flag) alloc_pages(1, alloc_flag)
#define pse_alloc_pages(alloc_flag) alloc_pages(1024, alloc_flag)
#define	free_page(pp) free_pages(pp, 1)
size_t nr_free_pages(void);

int		page_insert(pde_t *pgdir, struct Page *pp, void *va, int perm);
void	page_remove(pde_t *pgdir, void *va);
struct Page *page_lookup(pde_t *pgdir, void *va, pte_t **pte_store);
void	page_decref(struct Page *pp);

void	tlb_invalidate(pde_t *pgdir, void *va);

static inline ppn_t page2ppn(struct Page *page)
{
	return page - pages;
}

static inline physaddr_t
page2pa(struct Page *pp)
{
	return page2ppn(pp) << PGSHIFT;
}

static inline struct Page*
pa2page(physaddr_t pa)
{
	if (PGNUM(pa) >= npages)
		panic("pa2page called with invalid pa");
	return &pages[PGNUM(pa)];
}

static inline void*
page2kva(struct Page *pp)
{
	return KADDR(page2pa(pp));
}

static inline struct Page *kva2page(void *kva)
{
	return pa2page(PADDR(kva));
}

static inline struct Page *pte2page(pte_t pte)
{
	if (!(pte & PTE_P)) {
		panic("pte2page called with invalid pte");
	}
	return pa2page(PTE_ADDR(pte));
}

pte_t *pgdir_walk(pde_t *pgdir, const void *va, int create);

#endif /* !JOS_KERN_PMAP_H */
