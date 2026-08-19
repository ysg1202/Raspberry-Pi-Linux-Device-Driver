#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#define DEVICE_FILENAME "/dev/ledkey"

int main()
{
	int dev;
	char buff;
	char buffPre = 0;
	int ret;

	if(access(DEVICE_FILENAME, F_OK))
	{
		ret = mknod(
			DEVICE_FILENAME,
			S_IRWXU | S_IRWXG | S_IFCHR,
			(230 << 8) | 32
		);

		if(ret < 0)
		{
			perror("mknod()");
			return 1;
		}
	}

	printf("1) device file open\n");

	dev = open(DEVICE_FILENAME, O_RDWR | O_NDELAY);

	printf("dev : %d\n", dev);

	if(dev < 0)
	{
		perror("open");
		return 1;
	}

	do
	{
		ret = read(dev, &buff, sizeof(buff));

		if(buff != buffPre)
		{
			buffPre = buff;

			if(buff == 0)
			{
				usleep(100000);
				continue;
			}

			printf("key : %#04x\n", buff);

			write(dev, &buff, sizeof(buff));

			if(buff == 0x80)
				break;
		}

		usleep(100);

	} while(1);

	printf("6) device file close\n");

	ret = close(dev);

	printf("ret = %08X\n", ret);

	return 0;
}
