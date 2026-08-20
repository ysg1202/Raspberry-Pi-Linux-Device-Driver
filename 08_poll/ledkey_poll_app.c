#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <poll.h>
#include <string.h>

#define DEVICE_FILENAME "/dev/ledkey"

void print_OX(char key_data);

int main(int argc, char *argv[])
{
	char val = 0;
	char key_data;

	int dev;
	int ret;

	char buff[80];

	struct pollfd Events[2];

	if(argc < 2)
	{
		printf(
			"USAGE : %s ledVal[0x00~0xff]\n",
			argv[0]
		);

		return 1;
	}

	val = strtoul(argv[1], NULL, 16);

	if(val < 0 || 0xff < val)
	{
		printf(
			"Usage : %s ledValue(0x00~0xff)\n",
			argv[0]
		);

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

	dev = open(
		DEVICE_FILENAME,
		O_RDWR
	);

	if(dev < 0)
	{
		perror("open");
		return 1;
	}

	memset(
		Events,
		0,
		sizeof(Events)
	);

	Events[0].fd = fileno(stdin);
	Events[0].events = POLLIN;

	Events[1].fd = dev;
	Events[1].events = POLLIN;

	write(
		dev,
		&val,
		sizeof(val)
	);

	do
	{
		ret = poll(
			Events,
			2,
			2000
		);

		if(ret < 0)
		{
			perror("poll()");
			return 2;
		}
		else if(ret == 0)
		{
			continue;
		}

		if(Events[0].revents & POLLIN)
		{
			fgets(
				buff,
				sizeof(buff),
				stdin
			);

			buff[strlen(buff) - 1] = '\0';

			printf(
				"STDIN : %s\n",
				buff
			);

			key_data = 1 << (atoi(buff) - 1);

			print_OX(key_data);

			write(
				dev,
				&key_data,
				sizeof(key_data)
			);

			if(key_data == 0x80)
				break;
		}
		else if(Events[1].revents & POLLIN)
		{
			ret = read(
				dev,
				&key_data,
				sizeof(key_data)
			);

			if(ret < 0)
			{
				perror("read()");
				return 2;
			}

			key_data = 1 << (key_data - 1);

			if(key_data)
			{
				print_OX(key_data);

				write(
					dev,
					&key_data,
					sizeof(key_data)
				);
			}

			if(key_data == 0x80)
				break;
		}

	} while(1);

	close(dev);

	return 0;
}

void print_OX(char key_data)
{
	puts("0:1:2:3:4:5:6:7");

	for(int i = 0; i < 8; i++)
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
}
