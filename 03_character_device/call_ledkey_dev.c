#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/gpio.h>

#define LEDKEY_DEV_NAME  "ledkey_dev"
#define LEDKEY_DEV_MAJOR 230
#define GPIOCNT 8

#define DEBUG

static int gpioLed[GPIOCNT] = {518,519,520,521,522,523,524,525};
static int gpioKey[GPIOCNT] = {528,529,530,531,532,533,534,535};

static int gpioLedInit(void)
{
	int ret = 0;
	char gpioName[10];

	for(int i = 0; i < GPIOCNT; i++)
	{
		sprintf(gpioName, "gpio%d", i);

		ret = gpio_request(gpioLed[i], gpioName);
		if(ret < 0)
		{
			printk("Failed request gpio%d error\n", gpioLed[i]);
			return ret;
		}

		ret = gpio_direction_output(gpioLed[i], 0);
		if(ret < 0)
		{
			printk("Failed direction gpio%d error\n", gpioLed[i]);
			return ret;
		}
	}

	return ret;
}

static void gpioLedSet(long val)
{
	for(int i = 0; i < GPIOCNT; i++)
	{
		gpio_set_value(gpioLed[i], (val >> i) & 0x01);
	}
}

static void gpioLedFree(void)
{
	for(int i = 0; i < GPIOCNT; i++)
	{
		gpio_free(gpioLed[i]);
	}
}

static int gpioKeyInit(void)
{
	int ret = 0;
	char gpioName[10];

	for(int i = 0; i < GPIOCNT; i++)
	{
		sprintf(gpioName, "key%d", i);

		ret = gpio_request(gpioKey[i], gpioName);
		if(ret < 0)
		{
			printk("Failed Request gpio%d error\n", gpioKey[i]);
			return ret;
		}

		ret = gpio_direction_input(gpioKey[i]);
		if(ret < 0)
		{
			printk("Failed direction_output gpio%d error\n", gpioKey[i]);
			return ret;
		}
	}

	return ret;
}

static int gpioKeyGet(void)
{
	int ret;
	int keyData = 0;

	for(int i = 0; i < GPIOCNT; i++)
	{
		ret = gpio_get_value(gpioKey[i]);
		keyData |= (ret << i);
	}

	return keyData;
}

static void gpioKeyFree(void)
{
	for(int i = 0; i < GPIOCNT; i++)
	{
		gpio_free(gpioKey[i]);
	}
}

static int ledkey_open(struct inode *inode, struct file *filp)
{
	int num0 = MAJOR(inode->i_rdev);
	int num1 = MINOR(inode->i_rdev);

	printk("call open -> major : %d\n", num0);
	printk("call open -> minor : %d\n", num1);

	try_module_get(THIS_MODULE);

	return 0;
}

static loff_t ledkey_llseek(struct file *filp, loff_t off, int whence)
{
	printk("call llseek -> off : %08X, whenec : %08X\n",
	       (unsigned int)off, whence);

	return 0x23;
}

static ssize_t ledkey_read(struct file *filp,
			   char *buf,
			   size_t count,
			   loff_t *f_pos)
{
	char key = (char)gpioKeyGet();
	int ret;

	ret = copy_to_user(buf, &key, sizeof(key));

	if(ret < 0)
		return ret;

#ifdef DEBUG
	printk("call read -> key: %#04X, count : %d\n",
	       (unsigned int)key, count);
#endif

	return sizeof(key);
}

static ssize_t ledkey_write(struct file *filp,
			    const char *buf,
			    size_t count,
			    loff_t *f_pos)
{
	char led;
	int ret;

	ret = copy_from_user(&led, buf, sizeof(led));

	if(ret < 0)
		return ret;

	gpioLedSet(led);

#ifdef DEBUG
	printk("call write -> led : %08X, count : %d\n",
	       (unsigned int)led, count);
#endif

	return sizeof(led);
}

static long ledkey_ioctl(struct file *filp,
			 unsigned int cmd,
			 unsigned long arg)
{
	printk("call ioctl -> cmd : %#04X, arg : %08X\n",
	       cmd, (unsigned int)arg);

	return 0x53;
}

static int ledkey_release(struct inode *inode, struct file *filp)
{
	printk("call release\n");

	module_put(THIS_MODULE);

	return 0;
}

struct file_operations ledkey_fops =
{
	.open           = ledkey_open,
	.read           = ledkey_read,
	.write          = ledkey_write,
	.unlocked_ioctl = ledkey_ioctl,
	.llseek         = ledkey_llseek,
	.release        = ledkey_release,
};

static int ledkey_init(void)
{
	int result;

	printk("call ledkey_init\n");

	result = gpioLedInit();
	if(result < 0)
		return result;

	result = gpioKeyInit();
	if(result < 0)
		return result;

	result = register_chrdev(
		LEDKEY_DEV_MAJOR,
		LEDKEY_DEV_NAME,
		&ledkey_fops
	);

	if(result < 0)
		return result;

	return 0;
}

static void ledkey_exit(void)
{
	printk("call ledkey_exit\n");

	gpioLedFree();
	gpioKeyFree();

	unregister_chrdev(LEDKEY_DEV_MAJOR, LEDKEY_DEV_NAME);
}

module_init(ledkey_init);
module_exit(ledkey_exit);

MODULE_AUTHOR("KCCI-AIOT");
MODULE_DESCRIPTION("test moudle");
MODULE_LICENSE("Dual BSD/GPL");
