typedef void (*init_func)(void);

#define UFCON0	((volatile unsigned int *)(0x50000020))

extern void init_sys_mmu();
extern void start_mmu();
extern void test_printk(void);
extern void umask_int(unsigned int);
extern void enable_irq(void);
extern void init_page_map(void);
extern void *get_free_pages(unsigned int , int );
extern void put_free_pages(void *, int );
extern void printk(const char *fmt, ...);

#define TIMER_BASE (0xd1000000)
#define TCFG0 ((volatile unsigned int *)(TIMER_BASE + 0x0))
#define TCFG1 ((volatile unsigned int *)(TIMER_BASE + 0x4))
#define TCON ((volatile unsigned int *)(TIMER_BASE + 0x8))
#define TCONB4 ((volatile unsigned int *)(TIMER_BASE + 0x3c))

	void timer_init(void)
{
	*TCFG0 |= 0x800;
	*TCON &= (~(7 << 20));
	*TCON |= (1 << 22);
	*TCON |= (1 << 21);

	*TCONB4 = 10000;

	*TCON |= (1 << 20);
	*TCON &= ~(1 << 21);

	umask_int(14);
	enable_irq();
}

void helloworld(void)
{
	const char *p = "hello wzjos\n";
	while(*p) {
		*UFCON0 = *p++;
	};
}

static init_func init[] = {
	helloworld,
	0,
};

void test_mmu(void)
{
	const char *p = "test_mmu\n";
	while (*p)
	{
		*(volatile unsigned int *)0xd0000020 = *p++;
	};
}

void plat_boot(void)
{
	int i;
	for(i = 0; init[i]; i++) {
		init[i]();
	}
	init_sys_mmu();
	start_mmu();
	test_mmu();
	test_printk();
	//timer_init();

	init_page_map();
	char *p1, *p2, *p3, *p4;
	p1 = (char *)get_free_pages(0, 6);
	printk("6.1_the return address of get_free_pages %x\n", p1);
	p2 = (char *)get_free_pages(0, 6);
	printk("6.1_the return address of get_free_pages %x\n", p2);
	put_free_pages(p2, 6);
	put_free_pages(p1, 6);
	p3 = (char *)get_free_pages(0, 7);
	printk("6.1_the return address of get_free_pages %x\n", p3);
	p4 = (char *)get_free_pages(0, 7);
	printk("6.1_the return address of get_free_pages %x\n", p4);

	while(1);
}
/*
运行：
make:

arm-none-eabi-gcc -O2 -g -c init.s
arm-none-eabi-gcc -O2 -g -c start.s
arm-none-eabi-gcc -O2 -g -c boot.c
arm-none-eabi-gcc -O2 -g -c abnormal.s
arm-none-eabi-gcc -static -nostartfiles -nostdlib -Tlink.lds -Ttext 30000000  init.o start.o boot.o abnormal.o -o zjwos.elf -lgcc
arm-none-eabi-objcopy -O binary zjwos.elf zjwos.bin

skyeye:
hello wzjos
*/