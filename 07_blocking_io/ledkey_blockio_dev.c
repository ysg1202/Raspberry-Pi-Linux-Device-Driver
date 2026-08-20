#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/gpio.h>
#include <linux/moduleparam.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/wait.h>

#define GPIOCNT 8
#define LEDKEY_DEV_NAME  "ledkey_dev"
#define LEDKEY_DEV_MAJOR 230

static int onevalue = 0;
static char *twostring = NULL;
static int waitEventCondition = 0;

typedef struct {
	int irqKey[GPIOCNT];
	int keyNum;
} keyDataStruct;

static DEFINE_MUTEX(keyMutex);

module_param(onevalue, int, 0);
module_param(twostring, charp, 0);

DECLARE_WAIT_QUEUE_HEAD(WaitQueue_Read);

static int gpioLed[GPIOCNT] = {518,519,520,521,522,523,524,525};
static int gpioKey[GPIOCNT] = {528,529,530,531,532,533,534,535};

static int gpioLedInit(void);
static void gpioLedSet(long);
static void gpioLedFree(void);
static int gpioKeyInit(void);
static void gpioKeyFree(void);

static irqreturn_t keyIsr(int irq, void *data)
{
	keyDataStruct *pkeyData = (keyDataStruct *)data;

	for(int i = 0; i < GPIOCNT; i++)
	{
		if(irq == pkeyData->irqKey[i])
		{
			if(mutex_trylock(&keyMutex) != 0)
			{
				pkeyData->keyNum = i + 1;
				mutex_unlock(&keyMutex);
				break;
			}
		}
	}

	wake_up_interruptible(&WaitQueue_Read);
	waitEventCondition = 1;

	return IRQ_HANDLED;
}

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

static int gpioIrqInit(keyDataStruct *pkeyData)
{
	for(int i = 0; i < GPIOCNT; i++)
	{
		pkeyData->irqKey[i] = gpio_to_irq(gpioKey[i]);

		if(pkeyData->irqKey[i] < 0)
		{
			printk("Failed gpio_to_irq() gpio%d error\n", gpioKey[i]);
			return pkeyData->irqKey[i];
		}
	}

	return 0;
}

static void gpioKeyFree(void)
{
	for(int i = 0; i < GPIOCNT; i++)
	{
		gpio_free(gpioKey[i]);
	}
}

static void gpioIrqKeyFree(keyDataStruct *pkeyData)
{
	for(int i = 0; i < GPIOCNT; i++)
	{
		free_irq(pkeyData->irqKey[i], pkeyData);
	}
}

static int ledkey_open(struct inode *inode, struct file *filp)
{
	int result = 0;
	keyDataStruct *pkeyData;

	int num0 = MAJOR(inode->i_rdev);
	int num1 = MINOR(inode->i_rdev);

	char *irqName[GPIOCNT] = {
		"irqKey0",
		"irqKey1",
		"irqKey2",
		"irqKey3",
		"irqKey4",
		"irqKey5",
		"irqKey6",
		"irqKey7"
	};

	printk("call open -> major : %d\n", num0);
	printk("call open -> minor : %d\n", num1);

	try_module_get(THIS_MODULE);

	pkeyData = (keyDataStruct *)kmalloc(
		sizeof(keyDataStruct),
		GFP_KERNEL
	);

	memset(pkeyData, 0, sizeof(keyDataStruct));

	result = gpioIrqInit(pkeyData);
	if(result < 0)
		return result;

	for(int i = 0; i < GPIOCNT; i++)
	{
		result = request_irq(
			pkeyData->irqKey[i],
			keyIsr,
			IRQF_TRIGGER_RISING,
			irqName[i],
			pkeyData
		);

		if(result < 0)
			return result;
	}

	filp->private_data = pkeyData;

	return 0;
}

static loff_t ledkey_llseek(
	struct file *filp,
	loff_t off,
	int whence
)
{
	printk(
		"call llseek -> off : %08X, whenec : %08X\n",
		(unsigned int)off,
		whence
	);

	return 0x23;
}

static ssize_t ledkey_read(
	struct file *filp,
	char *buf,
	size_t count,
	loff_t *f_pos
)
{
	int key = 0;

	keyDataStruct *pkeyData =
		(keyDataStruct *)filp->private_data;

	if(!(filp->f_flags & O_NONBLOCK))
		wait_event_interruptible(
			WaitQueue_Read,
			waitEventCondition
		);

	if(mutex_trylock(&keyMutex) != 0)
	{
		if(pkeyData->keyNum != 0)
		{
			key = (char)pkeyData->keyNum;
			pkeyData->keyNum = 0;
		}

		mutex_unlock(&keyMutex);
	}

	put_user(key, buf);

	waitEventCondition = 0;

	return sizeof(key);
}

static ssize_t ledkey_write(
	struct file *filp,
	const char *buf,
	size_t count,
	loff_t *f_pos
)
{
	char led;

	get_user(led, buf);

	gpioLedSet(led);

	return sizeof(led);
}

static long ledkey_ioctl(
	struct file *filp,
	unsigned int cmd,
	unsigned long arg
)
{
	printk(
		"call ioctl -> cmd : %08X, arg : %08X\n",
		cmd,
		(unsigned int)arg
	);

	return 0x53;
}

static int ledkey_release(
	struct inode *inode,
	struct file *filp
)
{
	printk("call release\n");

	module_put(THIS_MODULE);

	gpioIrqKeyFree(filp->private_data);

	if(filp->private_data)
		kfree(filp->private_data);

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

	unregister_chrdev(
		LEDKEY_DEV_MAJOR,
		LEDKEY_DEV_NAME
	);
}

module_init(ledkey_init);
module_exit(ledkey_exit);

MODULE_AUTHOR("KCCI-AIOT");
MODULE_DESCRIPTION("test moudle");
MODULE_LICENSE("Dual BSD/GPL");
