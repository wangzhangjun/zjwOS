# write a simple os 

### 1.启动流程
链接脚本link.lds中指定了代码段（text）的入口是starup。然后在start.s中将所有的代码都编译到了startup section中。入口函数是_start。会进入_vector_reset。
__vector_reset定义在init.s中，会完成堆栈以及bss段的初始化。完后后跳入到主函数plat_boot中（在boot.c中）