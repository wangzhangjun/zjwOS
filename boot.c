typedef void (*init_func)(void);

#define UFCON0	((volatile unsigned int *)(0x50000020))

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

void plat_boot(void)
{
	int i;
	for(i = 0; init[i]; i++) {
		init[i]();
	}
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
会看到hello wzjos
*/