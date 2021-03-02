OUTPUT_ARCH(arm)
ENTRY(_start)   /*指定哪一个函数为程序的入口,_start在start.s中通过global定义*/

SECTIONS
{
	. = 0x00000000;
	.text :
	{
		*(.startup)
		*(.text)
	}
	. = ALIGN(32);
	.data :
	{
		*(.data)
	}
	. = ALIGN(32);
	__bss_start__ = .;
	.bss :
	{
		*(.bss)
	}
	__bss_end__ = .;
}
