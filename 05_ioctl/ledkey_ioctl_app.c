#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include "ioctl_test.h"

#define DEVICE_FILENAME "/dev/ledkey"

int main(int argc, char *argv[])
{
	char val = 0;
	char key_data;
	char key_data_old = 0;

	int i;
	int dev;
	int ret;

	ioctl_test_info ioctl_info;

	if(argc < 2)
	{
		printf("USAGE : %s ledVal[0x00~0xff]\n", argv[0]);
		return 1;
	}

	val = strtoul(argv[1], NULL, 16);

	if(val < 0 || 0xff < val)
	{
		printf("Usage : %s ledValue(0x00~0xff)\n", argv[0]);
		return 2;
	}

	if(access(DEVICE_FILENAME, F_OK))
	{
		ret = mknod(
			DEVICE_FILENAME,
			S_IRWXU | S_IRWXG | S_IRWXO | S_IFCHR,
			(230 << 8) | 32
		);

		if(ret < 0)
		{
			perror("mknod()");
			return 3;
		}
	}

	dev = open(DEVICE_FILENAME, O_RDWR | O_NDELAY);
	if(dev < 0)
	{
		perror("open");
		return 1;
	}

	ret = ioctl(dev, IOCTLTEST_LEDKEYINIT);
	if(ret < 0)
	{
		perror("ioctl()");
		return ret;
	}

	ret = ioctl(dev, IOCTLTEST_LEDON);
	if(ret < 0)
	{
		perror("ioctl()");
		return ret;
	}

	printf("test led on\n");
	sleep(2);

	ret = ioctl(dev, IOCTLTEST_LEDOFF);
	if(ret < 0)
	{
		perror("ioctl()");
		return ret;
	}

	printf("test led off\n");
	sleep(2);

	ret = ioctl(dev, IOCTLTEST_GETSTATE);
	if(ret < 0)
	{
		perror("ioctl()");
		return ret;
	}

	printf("test getstate key: %d\n", ret);

	sleep(2);

	printf("test write_read\n");

	ioctl_info.size = 1;
	ioctl_info.buff[0] = 0xaa;

	ret = ioctl(
		dev,
		IOCTLTEST_WRITE_READ,
		&ioctl_info
	);

	if(ret < 0)
	{
		perror("ioctl()");
		return ret;
	}

	if(ioctl_info.size == 1)
		printf("key : %d\n", ioctl_info.buff[0]);

	do
	{
		usleep(100000);

		ret = ioctl(
			dev,
			IOCTLTEST_READ,
			&ioctl_info
		);

		if(ret < 0)
		{
			perror("ioctl");
			return 2;
		}

		if(ioctl_info.size == 1)
		{
			key_data = ioctl_info.buff[0];

			if(key_data != key_data_old)
			{
				key_data_old = key_data;

				if(key_data)
				{
					puts("0:1:2:3:4:5:6:7");

					for(i = 0; i < 8; i++)
					{
						if(key_data & (0x01 << i))
							putchar('O');
						else
							putchar('X');

						if(i != 7)
							putchar(':');
						else
							putchar('\n');
					}

					putchar('\n');

					ioctl_info.size = 1;
					ioctl_info.buff[0] = key_data;

					ret = ioctl(
						dev,
						IOCTLTEST_WRITE,
						&ioctl_info
					);

					if(ret < 0)
					{
						perror("ioctl");
						return 2;
					}

					if(key_data == 0x80)
						break;
				}
			}
		}
	} while(1);

	ret = ioctl(dev, IOCTLTEST_LEDKEYFREE);
	if(ret < 0)
	{
		perror("ioctl()");
		return ret;
	}

	close(dev);

	return 0;
}ㄴ
