#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/gpio.h>
#include <linux/uaccess.h>

#include "ioctl_test.h"

#define LEDKEY_DEV_NAME  "ledkey_dev"
#define LEDKEY_DEV_MAJOR 230
#define GPIOCNT 8

static int gpioLed[GPIOCNT] = {518,519,520,521,522,523,524,525};
static int gpioKey[GPIOCNT] = {528,529,530,531,532,533,534,535};

static int ioctl_gpio_read(unsigned long arg, int size);
static int ioctl_gpio_write(unsigned long arg, int size);

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
	int major = MAJOR(inode->i_rdev);
	int minor = MINOR(inode->i_rdev);

	printk("call open -> major : %d\n", major);
	printk("call open -> minor : %d\n", minor);

	try_module_get(THIS_MODULE);

	return 0;
}

static loff_t ledkey_llseek(struct file *filp, loff_t off, int whence)
{
	printk("call llseek -> off : %08X, whence : %08X\n",
	       (unsigned int)off, whence);

	return 0x23;
}

static ssize_t ledkey_read(struct file *filp,
			   char __user *buf,
			   size_t count,
			   loff_t *f_pos)
{
	char key = (char)gpioKeyGet();

	if(copy_to_user(buf, &key, sizeof(key)) != 0)
		return -EFAULT;

	return sizeof(key);
}

static ssize_t ledkey_write(struct file *filp,
			    const char __user *buf,
			    size_t count,
			    loff_t *f_pos)
{
	char led;

	if(copy_from_user(&led, buf, sizeof(led)) != 0)
		return -EFAULT;

	gpioLedSet(led);

	return sizeof(led);
}

static long ledkey_ioctl(struct file *filp,
			 unsigned int cmd,
			 unsigned long arg)
{
	int err = 0;
	int size;

	printk("call ioctl -> cmd : %#04X, arg : %d\n",
	       cmd, (unsigned int)arg);

	if(_IOC_TYPE(cmd) != IOCTLTEST_MAGIC)
		return -EINVAL;

	if(_IOC_NR(cmd) >= IOCTLTEST_MAXNR)
		return -EINVAL;

	size = _IOC_SIZE(cmd);

	if(size)
	{
		if(_IOC_DIR(cmd) & _IOC_READ)
			err = access_ok((void __user *)arg, size);

		if(_IOC_DIR(cmd) & _IOC_WRITE)
			err = access_ok((void __user *)arg, size);

		if(!err)
			return -EFAULT;
	}

	switch(cmd)
	{
	case IOCTLTEST_LEDKEYINIT:
		err = gpioLedInit();
		if(err < 0)
			return err;

		err = gpioKeyInit();
		if(err < 0)
			return err;
		break;

	case IOCTLTEST_LEDKEYFREE:
		gpioLedFree();
		gpioKeyFree();
		break;

	case IOCTLTEST_LEDON:
		gpioLedSet(0xff);
		break;

	case IOCTLTEST_LEDOFF:
		gpioLedSet(0x00);
		break;

	case IOCTLTEST_GETSTATE:
		return gpioKeyGet();

	case IOCTLTEST_READ:
		err = ioctl_gpio_read(arg, size);
		if(err < 0)
			return err;
		break;

	case IOCTLTEST_WRITE:
		err = ioctl_gpio_write(arg, size);
		if(err < 0)
			return err;
		break;

	case IOCTLTEST_WRITE_READ:
		err = ioctl_gpio_read(arg, size);
		if(err < 0)
			return err;

		err = ioctl_gpio_write(arg, size);
		if(err < 0)
			return err;
		break;
	}

	return 0;
}

static int ioctl_gpio_read(unsigned long arg, int size)
{
	int key;
	ioctl_test_info ctrl_info;

	key = gpioKeyGet();

	ctrl_info.size = 1;
	ctrl_info.buff[0] = key;

	if(copy_to_user((void __user *)arg, &ctrl_info, size) != 0)
		return -EFAULT;

	return 0;
}

static int ioctl_gpio_write(unsigned long arg, int size)
{
	ioctl_test_info ctrl_info;

	if(copy_from_user(&ctrl_info, (void __user *)arg, size) != 0)
		return -EFAULT;

	if(ctrl_info.size == 1)
		gpioLedSet(ctrl_info.buff[0]);

	return 0;
}

static int ledkey_release(struct inode *inode, struct file *filp)
{
	printk("call release\n");

	module_put(THIS_MODULE);

	return 0;
}

static struct file_operations ledkey_fops =
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

	unregister_chrdev(
		LEDKEY_DEV_MAJOR,
		LEDKEY_DEV_NAME
	);
}

module_init(ledkey_init);
module_exit(ledkey_exit);

MODULE_AUTHOR("KCCI-AIOT");
MODULE_DESCRIPTION("LED KEY ioctl driver");
MODULE_LICENSE("Dual BSD/GPL");
