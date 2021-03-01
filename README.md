# write a simple os 

#### 1. 启动流程
链接脚本link.lds中指定了代码段（text）的入口是starup。然后在start.s中将所有的代码都编译到了startup section中。入口函数是_start。会进入_vector_reset。
__vector_reset定义在init.s中，会完成堆栈以及bss段的初始化。完后后跳入到主函数plat_boot中（在boot.c中）

#### 2. MMU
在mmu.c中的init_sys_mmu完成页表项的初始化，即指明了页表基地址从哪里开始，以及每个页表项的内容等。start_mmu函数中完成对cp15协处理的处理化，激活MMU。test_mmu主要是把一个虚拟地址映射到了物理串口地址，来测试mmu的生效。

#### 3.打印函数
在boot.c中调用print.c中的printk函数，会测试打印的输出，还是输出到串口

#### 4.中断
采用中断嵌套，来提升中断的处理方式。进入中断，保存r14和cspr之后，切换到SVC模式，执行中断。

#### 5.伙伴算法 && slab && kmalloc
mem.c 和 boot.c两个文件。