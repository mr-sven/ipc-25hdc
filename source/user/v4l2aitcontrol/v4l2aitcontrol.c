#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <linux/videodev.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <getopt.h>
#include <byteswap.h>

#include "AitXU.h"

void print_usage(char *progname)
{
	fprintf(stderr, "------------------------------------------------------------------\n");
	fprintf(stderr, "Usage: %s\n" \
					" [-h | --help ]........: display this help\n" \
					" [-v | --version ].....: display version information\n" \
					" [-i | --init].........: init Ait Device\n", progname);
	fprintf(stderr, "------------------------------------------------------------------\n");
}

void init_ait(int fd)
{
	__u8 data[8];

	AitXU_Init(fd);

	memset(data, 0, 8);

	AitXU_GetFWBuildDate(fd, data, 8);
	printf("AIT Firmware Build Date: 20%.2s.%.3s.%.2s\n", &data[1], &data[3], &data[6]);

    struct versionDev {
		__u16 reseverd;
		__u16 major;
		__u16 minor;
		__u16 patch;
    } version;

	AitXU_GetFWVersion(fd, (__u8 *)&version, sizeof(version));
	printf("AIT Firmware Build Date: %d.%d.%d\n", __bswap_16(version.major), __bswap_16(version.minor), __bswap_16(version.patch));
}

int main(int argc, char *argv[])
{
	char *dev = "/dev/video0";
	int fd = 0;
	int opt = 0;

	if ((fd = open(dev, O_RDWR)) == -1)
	{
		fprintf(stderr, "ERROR opening V4L interface \n");
		exit(1);
	}

	static struct option long_options[] = {
		{"help",		no_argument,		0, 'h'},
		{"init",		no_argument,		0, 'i'},
		{0,				0,					0, 0}
	};

	int long_index =0;
	while ((opt = getopt_long_only(argc, argv,"", long_options, &long_index )) != -1)
	{
		switch (opt)
		{
			case 'i' :
				init_ait(fd);
				break;
			default:
				print_usage(argv[0]);
				close(fd);
				exit(EXIT_FAILURE);
		}
	}
	close(fd);
//
//	H264SetMirr(fd, AIT_FM_NORMAL);
//	H264SetIRCutMode(fd, AIT_IR_NIGHT);
//
//	AitXU_SetFrameRate(fd, 5);
//
//	close(fd);
}
