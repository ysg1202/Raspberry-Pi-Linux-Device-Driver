#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio.h>

#define GPIOCNT 8

static int gpioLed[GPIOCNT] = {518,519,520,521,522,523,524,525};
static int gpioKey[GPIOCNT] = {528,529,530,531,532,533,534,535};

static int gpioLedInit(void);
static void gpioLedSet(long);
static void gpioLedFree(void);
static int gpioKeyInit(void);
static int gpioKeyGet(void);
static void gpioKeyFree(void);

static int gpioLedInit(void)
{
	int ret=0;
	char gpioName[10];
	
	for(int i=0;i<GPIOCNT;i++)
	{
		sprintf(gpioName,"gpio%d",i);
		ret=gpio_request(gpioLed[i],gpioName);
		if(ret < 0)
		{
			printk("Failed request gpio%d error\n",gpioLed[i]);
			return ret;
		}

		ret=gpio_direction_output(gpioLed[i],0);   //출력으로 설정 및 gpio low 출력
		if(ret < 0)
		{
			printk("Failed direction gpio%d error\n",gpioLed[i]);
			return ret;
		}
	}

	return ret;
}

static void gpioLedSet(long val)
{
	for(int i=0;i<GPIOCNT;i++)
	{
		gpio_set_value(gpioLed[i],(val>>i)&0x01);
	}
}

static void gpioLedFree(void)
{
	for(int i=0;i<GPIOCNT;i++)
	{
		gpio_free(gpioLed[i]);
	}
}

static int gpioKeyInit(void)
{
	int ret=0;
	char gpioName[10];

	for(int i=0;i<GPIOCNT;i++)
	{
		sprintf(gpioName,"key%d",i);

		ret = gpio_request(gpioKey[i], gpioName);
		if(ret < 0) {
			printk("Failed Request gpio%d error\n", gpioKey[i]);
			return ret;
		}

		ret = gpio_direction_input(gpioKey[i]);
		if(ret < 0) {
			printk("Failed direction_output gpio%d error\n", gpioKey[i]);
       	 	return ret;
		}
	}

	return ret;
}

static int gpioKeyGet(void)
{
	int ret;
	int keyData=0;

	for(int i=0;i<GPIOCNT;i++)
	{
		ret=gpio_get_value(gpioKey[i]);
		keyData |= ( ret << i );
	}

	return keyData;
}

static void gpioKeyFree(void)
{
	for(int i=0;i<GPIOCNT;i++)
	{
		gpio_free(gpioKey[i]);
	}
}

static int hello_init(void)
{
	int ret=0;
//	unsigned long val = 0xff;

    ret=gpioLedInit();
    if(ret<0)
        return ret;

    ret=gpioKeyInit();
    if(ret<0)
        return ret;

    ret=gpioKeyGet();
    if(ret<0)
        return ret;

    gpioLedSet(ret);

	printk(KERN_INFO "Hello, world(key:%#04x)\n",(unsigned int)ret);

	return 0;
}

static void hello_exit(void)
{
//	int ret=0;
	unsigned long val = 0;

	printk(KERN_INFO "Goodbye, world(val:%ld)\n",val);

    gpioLedSet(val);
    gpioLedFree();
	gpioKeyFree();
}

module_init(hello_init);    //insmod hello_init() 호출 
module_exit(hello_exit);	//rmmod hello_exit() 호출

MODULE_AUTHOR("KCCI-AIOT");
MODULE_DESCRIPTION("test moudle");
MODULE_LICENSE("Dual BSD/GPL");
