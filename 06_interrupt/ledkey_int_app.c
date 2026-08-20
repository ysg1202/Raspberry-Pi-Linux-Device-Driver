#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#define DEVICE_FILENAME "/dev/ledkey"

void print_OX(char key_data);

int main(int argc, char *argv[])
{
	char val = 0;
	char key_data;
	char key_data_old = 0;

	int dev;
	int ret;

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

	write(dev, &val, sizeof(val));

	do
	{
		ret = read(dev, &key_data, sizeof(key_data));

		if(ret < 0)
		{
			perror("read");
			return 2;
		}

		if(key_data != key_data_old)
		{
			key_data_old = key_data;

			if(key_data)
			{
				print_OX(key_data);
				write(dev, &key_data, sizeof(key_data));
			}

			if(key_data == 8)
				break;
		}

	} while(1);

	close(dev);

	return 0;
}

void print_OX(char key_data)
{
	key_data = 1 << (key_data - 1);

	puts("0:1:2:3:4:5:6:7");

	for(int i = 0; i < 8; i++)
	{
		putchar(key_data & (0x01 << i) ? 'O' : 'X');

		if(i != 7)
			putchar(':');
		else
			putchar('\n');
	}

	putchar('\n');
}
