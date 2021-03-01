CC=arm-none-eabi-gcc
LD=arm-none-eabi-ld
OBJCOPY=arm-none-eabi-objcopy

CFLAGS= -O2 -g
ASFLAGS= -O2 -g
LDFLAGS=-Tlink.lds -Ttext 30000000 

OBJS=init.o start.o boot.o abnormal.o mmu.o print.o interrupt.o mem.o

.c.o:
	$(CC) $(CFLAGS) -c $<
.s.o:
	$(CC) $(ASFLAGS) -c $<

zjwos.elf:$(OBJS)
	$(CC) -static -nostartfiles -nostdlib $(LDFLAGS) $? -o $@ -lgcc
	$(OBJCOPY) -O binary $@ zjwos.bin

clean:
	rm *.o zjwos.elf zjwos.bin -f
