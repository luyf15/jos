#ifndef JOS_KMALLOC_H
#define JOS_KMALLOC_H
#ifndef JOS_KERNEL
# error "This is a JOS kernel header; user programs should not #include it"
#endif

#define MAX_ORDER 10

void    kmem_cache_create();
void    kmem_cache_destory();

void	buddy_init();
void	buddy_destory();


#endif /* !JOS_kMALLOC_H */