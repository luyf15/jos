#include <inc/stdio.h>
#include <inc/x86.h>
#include <inc/mmu.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <inc/list.h>
#include <kern/pmap.h>
#include <kern/kclock.h>
#include <inc/memlayout.h>
#include <kern/default_pmm.h>
#include <inc/atomic.h>


extern physaddr_t boot_alloc_end;		// the top of initialized kernel
extern size_t npages;			// Amount of physical memory (in pages)
extern size_t npages_basemem;	// Amount of base memory (in pages)

static free_area_t free_area;	// Free list of physical pages
#define page_free_list (free_area.free_list)
#define nr_free (free_area.nr_free)

// Initialize page structure and memory free list.
// After this is done, NEVER use boot_alloc again.  ONLY use the page
// allocator functions below to allocate and deallocate physical
// memory via the page_free_list.
static void
default_init(void)
{
    list_init(&page_free_list);
	nr_free = 0;
}

static void
default_page_init(void)
{
	// The example code here marks all physical pages as free.
	// However this is not truly the case.  What memory is free?
	//  1) Mark physical page 0 as in use.
	//     This way we preserve the real-mode IDT and BIOS structures
	//     in case we ever need them.  (Currently we don't, but...)
	//  2) The rest of base memory, [PGSIZE, npages_basemem * PGSIZE)
	//     is free.
	//  3) Then comes the IO hole [IOPHYSMEM, EXTPHYSMEM), which must
	//     never be allocated.
	//  4) Then extended memory [EXTPHYSMEM, ...).
	//     Some of it is in use, some is free. Where is the kernel
	//     in physical memory?  Which pages are already in use for
	//     page tables and other data structures?
	//  5) Mark the physical page at MPENTRY_PADDR as in use
	// pages[_i].flags = 0x2;
#define MARK_FREE(_i) do {\
    set_page_ref(&pages[_i], 0);\
    list_add_before(&page_free_list, &(pages[_i].pp_link));\
	SetPageProperty(&pages[_i]);\
	pages[_i].property = 0;\
	nr_free++;\
	} while(0)

#define MARK_USE(_i) do {\
    set_page_ref(&pages[_i], 0);\
    SetPageReserved(&pages[_i]);\
    pages[_i].pp_link.next = &(pages[_i].pp_link);\
    pages[_i].pp_link.prev = &(pages[_i].pp_link);\
	} while(0)

	size_t i;

#define MPCT MPENTRY_PADDR / PGSIZE

    MARK_USE(0);
    for (i = 1; i < MPCT; ++i)
        MARK_FREE(i);
	pages[1].property = MPCT - 1;

	// jump the physical page at MPENTRY_PADDR
	MARK_USE(MPCT);
	for (i = MPCT + 1; i < npages_basemem; ++i)
        MARK_FREE(i);
	pages[MPCT + 1].property = npages_basemem - 1 - MPCT;
	// jump over the gap between Base(IO) and Extended
    for (i = IOPHYSMEM / PGSIZE; i < EXTPHYSMEM / PGSIZE; ++i)
        MARK_USE(i);
	
	// kernel_base to last boot_alloc end
    for (i = EXTPHYSMEM / PGSIZE; i < boot_alloc_end / PGSIZE; ++i)
        MARK_USE(i);
    for (i = boot_alloc_end / PGSIZE; i < npages; ++i)
        MARK_FREE(i);
	pages[boot_alloc_end / PGSIZE].property = npages - boot_alloc_end / PGSIZE;

#undef MPCT
#undef MARK_USE
#undef MARK_FREE
}


// Allocates a physical page.  If (alloc_flags & ALLOC_ZERO), fills the entire
// returned physical page with '\0' bytes.  Does NOT increment the reference
// count of the page - the caller must do these if necessary (either explicitly
// or via page_insert).
//
// Be sure to set the pp_link field of the allocated page to NULL so
// page_free can check for double-free bugs.
//
// Returns NULL if out of free memory.
//
// Hint: use page2kva and memset
static struct Page *
default_alloc_pages(size_t n, int alloc_flags)
{
	// Fill this function in
	assert(n > 0);
    if (n > nr_free)
        return NULL;
    list_entry_t *le, *len;
    le = &page_free_list;

    while ((le=list_next(le)) != &page_free_list){
    	struct Page *p = le2page(le, pp_link);
    	if(p->property >= n){
			int i;
			for(i = 0; i < n; i ++){
				len = list_next(le);
				struct Page *pp = le2page(le, pp_link);
				SetPageReserved(pp);
				ClearPageProperty(pp);
				list_del(le);
				le = len;
			}
			if(p->property > n)
				(le2page(le,pp_link))->property = p->property - n;

			ClearPageProperty(p);
			SetPageReserved(p);
			nr_free -= n;
			if (alloc_flags & ALLOC_ZERO)
				memset(page2kva(p), 0, PGSIZE * n);		//init pages from kaddr(target)
			return p;
    	}
    }
    return NULL;
}

// Return a page to the free list.
// (This function should only be called when pp->pp_ref reaches 0.)
//
static void
default_free_pages(struct Page *base, size_t n)
{
	// Fill this function in
	// Hint: You may want to panic if pp->pp_ref is nonzero or
	// pp->pp_link is not NULL.
	assert(PageReserved(base));
	assert(!page_ref(base));
	assert(n > 0);
	list_entry_t *le = &page_free_list;
	struct Page *p;

	// Find the exact position of page_free_list to insert pages
	// The worse case: le = &page_free_list——insert begins from tail
	while((le=list_next(le)) != &page_free_list) {
    	p = le2page(le, pp_link);
    	if(p > base) break;
    }

	for(p = base; p < base + n; p++)
    	list_add_before(le, &(p->pp_link));
    base->flags = 0;
    SetPageProperty(base);
    base->property = n;
    
	// merge the current continuous free physical pages
    p = le2page(le,pp_link) ;
    if( base + n == p ){
      	base->property += p->property;
      	p->property = 0;
    }
    le = list_prev(&(base->pp_link));
    p = le2page(le, pp_link);
    if(le != &page_free_list && p == base - 1){ //still some free pages before "base", may be able to merge
    	while (le != &page_free_list){
    		if(p->property){	// the head of free page block
				p->property += base->property;
				base->property = 0;
				break;
        	}
			le = list_prev(le);
			p = le2page(le,pp_link);
      	}
    }
    nr_free += n;
    return;
}

//default_nr_free_pages - get the nr: the number of free pages
static inline size_t
default_nr_free_pages(void)
{
	return nr_free;
}

//
// Check the physical page allocator (alloc_page(), free_page(),
// and page_init()).
//
static void
default_check_page_alloc(void)
{
	struct Page *pp, *pp0, *pp1, *pp2;
	int nfree = 0;
	struct Page *fl;
	char *c;
	int i;
	list_entry_t *le;
	le = &page_free_list;

	if (!pages)
		panic("'pages' is a null pointer!");

	// check number of free pages
	while((le=list_next(le)) != &page_free_list)
		++nfree;

	assert(nr_free == nfree);
	// should be able to allocate three pages
	pp0 = pp1 = pp2 = 0;
	assert((pp0 = alloc_page(0)));
	assert((pp1 = alloc_page(0)));
	assert((pp2 = alloc_page(0)));

	assert(pp0);
	assert(pp1 && pp1 != pp0);
	assert(pp2 && pp2 != pp1 && pp2 != pp0);
	assert(page2pa(pp0) < npages*PGSIZE);
	assert(page2pa(pp1) < npages*PGSIZE);
	assert(page2pa(pp2) < npages*PGSIZE);

	// temporarily steal the rest of the free pages
	le = &page_free_list;
	list_entry_t *tprev, *tnext;
	tprev = le->prev, tnext = le->next;
	uint32_t ntemp = nr_free;
	nr_free = 0;
	//fl = le2page(list_next(le),pp_link);
	list_init(&page_free_list);

	// should be no free memory
	assert(!alloc_page(0));

	// free and re-allocate?
	free_page(pp0);
	free_page(pp1);
	free_page(pp2);
	pp0 = pp1 = pp2 = 0;
	assert((pp0 = alloc_page(0)));
	assert((pp1 = alloc_page(0)));
	assert((pp2 = alloc_page(0)));
	assert(pp0);
	assert(pp1 && pp1 != pp0);
	assert(pp2 && pp2 != pp1 && pp2 != pp0);
	assert(!alloc_page(0));

	// test flags
	memset(page2kva(pp0), 1, PGSIZE);
	free_page(pp0);
	assert((pp = alloc_page(ALLOC_ZERO)));
	assert(pp && pp0 == pp);
	c = page2kva(pp);
	for (i = 0; i < PGSIZE; i++)
		assert(c[i] == 0);

	// give free list back
	le->prev = tprev, le->next = tnext;
	nr_free = ntemp;

	// free the pages we took
	free_page(pp0);
	free_page(pp1);
	free_page(pp2);

	// number of free pages should be the same
	while((le=list_next(le)) != &page_free_list)
		--nfree;
	assert(nfree == 0);

	cprintf("check_page_alloc() succeeded!\n");
}

static void
default_check(void) {
    int count = 0, total = 0;
    list_entry_t *le = &page_free_list;
    while ((le = list_next(le)) != &page_free_list) {
        struct Page *p = le2page(le, pp_link);
        assert(PageProperty(p));
        count ++, total += p->property;
    }
    assert(total == nr_free_pages());

    default_check_page_alloc();

    struct Page *p0 = alloc_pages(5,ALLOC_ZERO), *p1, *p2;
    assert(p0 != NULL);
    assert(!PageProperty(p0));

    list_entry_t free_list_store = page_free_list;
    list_init(&page_free_list);
    assert(list_empty(&page_free_list));
    assert(alloc_page(ALLOC_ZERO) == NULL);

    unsigned int nr_free_store = nr_free;
    nr_free = 0;

    free_pages(p0 + 2, 3);
    assert(alloc_pages(4,ALLOC_ZERO) == NULL);
    assert(PageProperty(p0 + 2) && p0[2].property == 3);
    assert((p1 = alloc_pages(3,ALLOC_ZERO)) != NULL);
    assert(alloc_page(ALLOC_ZERO) == NULL);
    assert(p0 + 2 == p1);

    p2 = p0 + 1;
    free_page(p0);
    free_pages(p1, 3);
    assert(PageProperty(p0) && p0->property == 1);
    assert(PageProperty(p1) && p1->property == 3);

    assert((p0 = alloc_page(ALLOC_ZERO)) == p2 - 1);
    free_page(p0);
    assert((p0 = alloc_pages(2,ALLOC_ZERO)) == p2 + 1);

    free_pages(p0, 2);
    free_page(p2);

    assert((p0 = alloc_pages(5,ALLOC_ZERO)) != NULL);
    assert(alloc_page(ALLOC_ZERO) == NULL);

    assert(nr_free == 0);
    nr_free = nr_free_store;

    page_free_list = free_list_store;
    free_pages(p0, 5);

    le = &page_free_list;
    while ((le = list_next(le)) != &page_free_list) {
        struct Page *p = le2page(le, pp_link);
        count --, total -= p->property;
    }
    assert(count == 0);
    assert(total == 0);
}

const struct pmm_manager default_pmm_manager = {
	.name = "default_pmm_manager",
	.init = default_init,
	.page_init = default_page_init,
	.alloc_pages = default_alloc_pages,
	.free_pages = default_free_pages,
	.nr_free_pages = default_nr_free_pages,
	.check = default_check_page_alloc,
};