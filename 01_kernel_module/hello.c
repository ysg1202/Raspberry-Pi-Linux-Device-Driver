#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

static int hello_init(void)
{
	printk("Hello, world\n");
	return 0;
}

static void hello_exit(void)
{
	printk("Goodbye, world\n");
}

module_init(hello_init); // insmod에 의해 hello_init호출
module_exit(hello_exit); // rmmod에 의해 hello_exit호출 

MODULE_AUTHOR("KCCI-AIOT");
MODULE_DESCRIPTION("test module");
MODULE_LICENSE("Dual BSD/GPL");
