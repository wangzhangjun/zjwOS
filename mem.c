
extern void printk(const char *fmt, ...);

#define NULL ((void *)0)

struct list_head
{
    struct list_head *next, *prev;
};

static inline void INIT_LIST_HEAD(struct list_head *list)
{
    list->next = list;
    list->prev = list;
}

static inline void __list_add(struct list_head *new_lst,
                              struct list_head *prev,
                              struct list_head *next)
{
    next->prev = new_lst;
    new_lst->next = next;
    new_lst->prev = prev;
    prev->next = new_lst;
}

static inline void list_add(struct list_head *new_lst, struct list_head *head)
{
    __list_add(new_lst, head, head->next);
}

static inline void list_add_tail(struct list_head *new_lst, struct list_head *head)
{
    __list_add(new_lst, head->prev, head);
}

static inline void __list_del(struct list_head *prev, struct list_head *next)
{
    next->prev = prev;
    prev->next = next;
}

static inline void list_del(struct list_head *entry)
{
    __list_del(entry->prev, entry->next);
}

static inline void list_remove_chain(struct list_head *ch, struct list_head *ct)
{
    ch->prev->next = ct->next;
    ct->next->prev = ch->prev;
}

static inline void list_add_chain(struct list_head *ch, struct list_head *ct, struct list_head *head)
{
    ch->prev = head;
    ct->next = head->next;
    head->next->prev = ct;
    head->next = ch;
}

static inline void list_add_chain_tail(struct list_head *ch, struct list_head *ct, struct list_head *head)
{
    ch->prev = head->prev;
    head->prev->next = ch;
    head->prev = ct;
    ct->next = head;
}

static inline int list_empty(const struct list_head *head)
{
    return head->next == head;
}

#define offsetof(TYPE, MEMBER) ((unsigned int)&((TYPE *)0)->MEMBER)

#define container_of(ptr, type, member) ({			\
	const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
		(type *)( (char *)__mptr - offsetof(type,member) ); })

#define list_entry(ptr, type, member) \
    container_of(ptr, type, member)

#define list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

#define _MEM_END 0x30700000   //页表最后的起始地址，这个地址往上的一段空间作为伙伴算法
#define _MEM_START 0x300f0000   //操作系统运行时的大小小于f0000

#define PAGE_SHIFT (12)
#define PAGE_SIZE (1 << PAGE_SHIFT)  //页的大小是4k，1左移12位
#define PAGE_MASK (~(PAGE_SIZE - 1))

#define KERNEL_MEM_END (_MEM_END)
/*the bigin and end of the kernel mem which is needed to be paged.*/
#define KERNEL_PAGING_START ((_MEM_START + (~PAGE_MASK)) & ((PAGE_MASK))) //_MEM_START的值按照PAGE_SIZE对齐，不对齐则取整
#define KERNEL_PAGING_END (((KERNEL_MEM_END - KERNEL_PAGING_START) / (PAGE_SIZE + sizeof(struct page))) * (PAGE_SIZE) + KERNEL_PAGING_START)  //所有页的结束位置

/*page number in need */
#define KERNEL_PAGE_NUM ((KERNEL_PAGING_END - KERNEL_PAGING_START) / PAGE_SIZE)  //页是多少个
/*the start and end of the page structure should be storaged in.*/
#define KERNEL_PAGE_END _MEM_END   //struct page的结束的地方就是_MEM_END
#define KERNEL_PAGE_START (KERNEL_PAGE_END - KERNEL_PAGE_NUM * sizeof(struct page))  //存放struct page结构体的开始的地方

/*page flags*/
#define PAGE_AVAILABLE 0x00
#define PAGE_DIRTY 0x01
#define PAGE_PROTECT 0x02
#define PAGE_BUDDY_BUSY 0x04
#define PAGE_IN_CACHE 0x08

struct kmem_cache
{
    unsigned int obj_size;
    unsigned int obj_nr;
    unsigned int page_order;
    unsigned int flags;
    struct page *head_page;
    struct page *end_page;
    void *nf_block;
};

struct page
{
    unsigned int vaddr;
    unsigned int flags;
    int order;
    unsigned int counter;
    struct kmem_cache *cachep;
    struct list_head list; //to string the buddy member
};

#define MAX_BUDDY_PAGE_NUM (9) //所有不同大小的buddy的个数,2的0次方一直到2的8次方，允许我们最大申请256个页， 即1M内存

#define AVERAGE_PAGE_NUM_PER_BUDDY (KERNEL_PAGE_NUM / MAX_BUDDY_PAGE_NUM)
#define PAGE_NUM_FOR_MAX_BUDDY ((1 << MAX_BUDDY_PAGE_NUM) - 1)

struct list_head page_buddy[MAX_BUDDY_PAGE_NUM];  //每一组都用list_head的双向链表连接起来

struct page *virt_to_page(unsigned int addr) //内存地址和 struct page 结构体之间的转换
{
    unsigned int i;
    i = ((addr)-KERNEL_PAGING_START) >> PAGE_SHIFT;
    if (i > KERNEL_PAGE_NUM)
        return NULL;
    return (struct page *)KERNEL_PAGE_START + i;
}

//初始化所有的buddy数组
void init_page_buddy(void)
{
    int i;
    for (i = 0; i < MAX_BUDDY_PAGE_NUM; i++)
    {
        INIT_LIST_HEAD(&page_buddy[i]);
    }
}

//各个buddy的初始化
void init_page_map(void)
{
    int i;
    struct page *pg = (struct page *)KERNEL_PAGE_START;
    init_page_buddy();
    printk("KERNEL_PAGE_NUM:%d\n", KERNEL_PAGE_NUM);  //1541
    printk("PAGE_NUM_FOR_MAX_BUDDY:%d\n", ~PAGE_NUM_FOR_MAX_BUDDY);                          // -512
    printk("page_buddy[8]中的page个数:%d\n", (KERNEL_PAGE_NUM & (~PAGE_NUM_FOR_MAX_BUDDY))); //1536, 这个数字的含义是，1541/256,即256（2的8次方）是一个buddy，一共可以分到1526就不够了
    for (i = 0; i < (KERNEL_PAGE_NUM); pg++, i++)
    {
        /*fill struct page first*/
        pg->vaddr = KERNEL_PAGING_START + i * PAGE_SIZE;
        pg->flags = PAGE_AVAILABLE;
        pg->counter = 0;
        INIT_LIST_HEAD(&(pg->list));

        /* i< 1536. 前1536个page全部挂在了8号数组上 */
        if (i < (KERNEL_PAGE_NUM & (~PAGE_NUM_FOR_MAX_BUDDY)))
        {
            // printk("in if buddy :%d\n", MAX_BUDDY_PAGE_NUM - 1);  //一直都是8
            /*the following code should be dealt carefully,we would change the order field of a head struct page to the corresponding order,and change others to -1*/
            if ((i & PAGE_NUM_FOR_MAX_BUDDY) == 0) //i & 511(1111 1111 1) ,每511个一次循环的第一个是0，即每个page组的第一个struct page设置为阶数
            {
                pg->order = MAX_BUDDY_PAGE_NUM - 1;  //第一个page设置为2的几次方个page
            }
            else
            {
                pg->order = -1;  //其余标记为-1
            }
            list_add_tail(&(pg->list), &page_buddy[MAX_BUDDY_PAGE_NUM - 1]);
            /*the remainder not enough to merge into a max buddy is done as min buddy*/
        }
        else
        {
            pg->order = 0;
            list_add_tail(&(pg->list), &page_buddy[0]);
        }
    }
}

/* we can do these all because the page structure that represents one page aera is continuous */
#define BUDDY_END(x, order) ((x) + (1 << (order)) - 1)  //x是一个page的地址，然后往后跳几个，跳到第一个buddy的末尾，比如2的2次方，list_entry得到的是第一个Buddy中的第一个page的值，那要想得到第一个buddy中的最后一个page的值，就是加上2的2次方减1
#define NEXT_BUDDY_START(x, order) ((x) + (1 << (order)))
#define PREV_BUDDY_START(x, order) ((x) - (1 << (order)))

/*the logic of this function seems good,no bug reported yet*/
/*
struct page
{
    unsigned int vaddr;
    unsigned int flags;
    int order;
    unsigned int counter;
    struct kmem_cache *cachep;
    struct list_head list; //to string the buddy member
};
*/
struct page *get_pages_from_list(int order)
{
    unsigned int vaddr;
    int neworder = order;
    struct page *pg, *ret;
    struct list_head *tlst, *tlst1;
    for (; neworder < MAX_BUDDY_PAGE_NUM/*9*/; neworder++)
    {
        if (list_empty(&page_buddy[neworder]))
        {
            continue;
        }
        else
        {
            pg = list_entry(page_buddy[neworder].next, struct page, list);
            tlst = &(BUDDY_END(pg, neworder)->list);  //找到的这个buddy的最后一个page的->list的地址
            tlst->next->prev = &page_buddy[neworder];  //这两句的意思是把找到的buddy拆下来，前后再接上
            page_buddy[neworder].next = tlst->next;
            goto OUT_OK;
        }
    }
    return NULL;

OUT_OK:
    for (neworder--; neworder >= order; neworder--)  //从实际请求的buddy数减一开始，减到用户请求的buddy阶数。例如请求3，但是在5的链中才找到，那么就要从4开始循环，到3结束
    {

        tlst1 = &(BUDDY_END(pg, neworder)->list);  //巧妙啊
        tlst = &(pg->list);

        pg = NEXT_BUDDY_START(pg, neworder);
        list_entry(tlst, struct page, list)->order = neworder;

        list_add_chain_tail(tlst, tlst1, &page_buddy[neworder]);
    }
    pg->flags |= PAGE_BUDDY_BUSY;
    pg->order = order;
    return pg;
}

void put_pages_to_list(struct page *pg, int order)
{
    struct page *tprev, *tnext;
    if (!(pg->flags & PAGE_BUDDY_BUSY))
    {
        printk("something must be wrong when you see this message,that probably means you are forcing to release a page that was not alloc at all\n");
        return;
    }
    pg->flags &= ~(PAGE_BUDDY_BUSY);

    for (; order < MAX_BUDDY_PAGE_NUM; order++)
    {
        tnext = NEXT_BUDDY_START(pg, order);
        tprev = PREV_BUDDY_START(pg, order);
        if ((!(tnext->flags & PAGE_BUDDY_BUSY)) && (tnext->order == order))
        {
            pg->order++;
            tnext->order = -1;
            list_remove_chain(&(tnext->list), &(BUDDY_END(tnext, order)->list));
            BUDDY_END(pg, order)->list.next = &(tnext->list);
            tnext->list.prev = &(BUDDY_END(pg, order)->list);
            continue;
        }
        else if ((!(tprev->flags & PAGE_BUDDY_BUSY)) && (tprev->order == order))
        {
            pg->order = -1;

            list_remove_chain(&(pg->list), &(BUDDY_END(pg, order)->list));
            BUDDY_END(tprev, order)->list.next = &(pg->list);
            pg->list.prev = &(BUDDY_END(tprev, order)->list);

            pg = tprev;
            pg->order++;
            continue;
        }
        else
        {
            break;
        }
    }

    list_add_chain(&(pg->list), &((tnext - 1)->list), &page_buddy[order]);
}

void *page_address(struct page *pg)
{
    return (void *)(pg->vaddr);
}

struct page *alloc_pages(unsigned int flag, int order)  //flag参数只是保留
{
    struct page *pg;
    int i;
    pg = get_pages_from_list(order);
    if (pg == NULL)
        return NULL;
    for (i = 0; i < (1 << order); i++) //将分配得到的每一个页都至为PAGE_DIRTY
    {
        (pg + i)->flags |= PAGE_DIRTY;  
    }
    return pg;
}

void free_pages(struct page *pg, int order)
{
    int i;
    for (i = 0; i < (1 << order); i++)
    {
        (pg + i)->flags &= ~PAGE_DIRTY;
    }
    put_pages_to_list(pg, order);
}

void *get_free_pages(unsigned int flag, int order)
{
    struct page *page;
    page = alloc_pages(flag, order);
    if (!page)
        return NULL;
    return page_address(page);  //得到内存块的首地址，一一映射的虚拟到物理地址
}

void put_free_pages(void *addr, int order)
{
    free_pages(virt_to_page((unsigned int)addr), order);
}

#define KMEM_CACHE_DEFAULT_ORDER (0)
#define KMEM_CACHE_MAX_ORDER (5) //cache can deal with the memory no less than 32*PAGE_SIZE
#define KMEM_CACHE_SAVE_RATE (0x5a)
#define KMEM_CACHE_PERCENT (0x64)
#define KMEM_CACHE_MAX_WAST (PAGE_SIZE - KMEM_CACHE_SAVE_RATE * PAGE_SIZE / KMEM_CACHE_PERCENT)

int find_right_order(unsigned int size)
{
    int order;
    for (order = 0; order <= KMEM_CACHE_MAX_ORDER; order++)
    {
        if (size <= (KMEM_CACHE_MAX_WAST) * (1 << order))
        {
            return order;
        }
    }
    if (size > (1 << order))
        return -1;
    return order;
}

int kmem_cache_line_object(void *head, unsigned int size, int order)
{
    void **pl;
    char *p;
    pl = (void **)head;
    p = (char *)head + size;
    int i, s = PAGE_SIZE * (1 << order);
    for (i = 0; s > size; i++, s -= size)
    {
        *pl = (void *)p;
        pl = (void **)p;
        p = p + size;
    }
    if (s == size)
        i++;
    return i;
}

struct kmem_cache *kmem_cache_create(struct kmem_cache *cache, unsigned int size, unsigned int flags)
{
    void **nf_block = &(cache->nf_block);

    int order = find_right_order(size);
    if (order == -1)
        return NULL;
    if ((cache->head_page = alloc_pages(0, order)) == NULL)
        return NULL;
    *nf_block = page_address(cache->head_page);

    cache->obj_nr = kmem_cache_line_object(*nf_block, size, order);
    cache->obj_size = size;
    cache->page_order = order;
    cache->flags = flags;
    cache->end_page = BUDDY_END(cache->head_page, order);
    cache->end_page->list.next = NULL;

    return cache;
}

/*FIXME:I dont understand it now*/
void kmem_cache_destroy(struct kmem_cache *cache)
{
    int order = cache->page_order;
    struct page *pg = cache->head_page;
    struct list_head *list;
    while (1)
    {
        list = BUDDY_END(pg, order)->list.next;
        free_pages(pg, order);
        if (list)
        {
            pg = list_entry(list, struct page, list);
        }
        else
        {
            return;
        }
    }
}

void kmem_cache_free(struct kmem_cache *cache, void *objp)
{
    *(void **)objp = cache->nf_block;
    cache->nf_block = objp;
    cache->obj_nr++;
}

void *kmem_cache_alloc(struct kmem_cache *cache, unsigned int flag)
{
    void *p;
    struct page *pg;
    if (cache == NULL)
        return NULL;
    void **nf_block = &(cache->nf_block);
    unsigned int *nr = &(cache->obj_nr);
    int order = cache->page_order;

    if (!*nr)
    {
        if ((pg = alloc_pages(0, order)) == NULL)
            return NULL;
        *nf_block = page_address(pg);
        cache->end_page->list.next = &pg->list;
        cache->end_page = BUDDY_END(pg, order);
        cache->end_page->list.next = NULL;
        *nr += kmem_cache_line_object(*nf_block, cache->obj_size, order);
    }

    (*nr)--;
    p = *nf_block;
    *nf_block = *(void **)p;
    pg = virt_to_page((unsigned int)p);
    pg->cachep = cache; //doubt it???
    return p;
}

#define KMALLOC_BIAS_SHIFT (5) //32byte minimal
#define KMALLOC_MAX_SIZE (4096)
#define KMALLOC_MINIMAL_SIZE_BIAS (1 << (KMALLOC_BIAS_SHIFT))
#define KMALLOC_CACHE_SIZE (KMALLOC_MAX_SIZE / KMALLOC_MINIMAL_SIZE_BIAS)

struct kmem_cache kmalloc_cache[KMALLOC_CACHE_SIZE] = {
    {0, 0, 0, 0, NULL, NULL, NULL},
};
#define kmalloc_cache_size_to_index(size) ((((size)) >> (KMALLOC_BIAS_SHIFT)))

int kmalloc_init(void)
{
    int i = 0;

    for (i = 0; i < KMALLOC_CACHE_SIZE; i++)
    {
        if (kmem_cache_create(&kmalloc_cache[i], (i + 1) * KMALLOC_MINIMAL_SIZE_BIAS, 0) == NULL)
            return -1;
    }
    return 0;
}

void *kmalloc(unsigned int size)
{
    int index = kmalloc_cache_size_to_index(size);
    if (index >= KMALLOC_CACHE_SIZE)
        return NULL;
    return kmem_cache_alloc(&kmalloc_cache[index], 0);
}

void kfree(void *addr)
{
    struct page *pg;
    pg = virt_to_page((unsigned int)addr);
    kmem_cache_free(pg->cachep, addr);
}