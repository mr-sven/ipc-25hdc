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
					" [--help ]........: display this help\n" \
					" [--version ].....: display version information\n" \
					" [--init ]........: init Ait Device\n" \
					" [--framerate ]...: sets the specified framerate: 5-30\n" \
					" [--bitrate ].....: sets the specified bitrate\n" \
					" [--mirror ]......: sets mirror mode: 0 = default, 1 = flip, 2 = mirror, 3 = mirror & flip\n" \
					" [--quality ].....: sets the MJPEG quality: 1-255\n" \
					" [--irmode ]......: sets the IR Cut mode: 0 = default, 1 = day, 2 = night\n" \
					" [--pframecount ].: sets the P-Frame count: 1-255\n", progname);
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

void read_reg(int fd, int reg)
{
	__u8 data[8];
	__u8 cnt;

	memset(data, 0, 8);
	AitXU_ReadReg(fd, reg, data);

	printf("Register 0x%04x: \n", reg);
	for (cnt = 0; cnt < 8; cnt++)
	{
		printf(" 0x%02x", data[cnt]);
	}
	printf("\n");
}

int main(int argc, char *argv[])
{
	char *dev = "/dev/video0";
	int fd = 0;
	int opt = 0;

	int framerate = -1;
	int bitrate = -1;
	int mirror = -1;
	int quality = -1;
	int irmode = -1;
	int pframecount = -1;

	if ((fd = open(dev, O_RDWR)) == -1)
	{
		fprintf(stderr, "ERROR opening V4L interface \n");
		exit(1);
	}

	static struct option long_options[] = {
		{"help",		no_argument,		0, 'h'},
		{"init",		no_argument,		0, 'i'},
		{"framerate",	required_argument,	0, 'r'},
		{"bitrate",		required_argument,	0, 'b'},
		{"mirror",		required_argument,	0, 'm'},
		{"quality",		required_argument,	0, 'q'},
		{"irmode",		required_argument,	0, 'n'},
		{"pframecount",	required_argument,	0, 'p'},
		{"reg",			required_argument,	0, 'R'},
		{0,				0,					0, 0}
	};

	int long_index =0;
	while ((opt = getopt_long_only(argc, argv,"", long_options, &long_index )) != -1)
	{
		switch (opt)
		{
			case 'i':
				init_ait(fd);
				break;
			case 'r':
				framerate = atoi(optarg);
				break;
			case 'b':
				bitrate = atoi(optarg);
				break;
			case 'm':
				mirror = atoi(optarg);
				break;
			case 'q':
				quality = atoi(optarg);
				break;
			case 'n':
				irmode = atoi(optarg);
				break;
			case 'p':
				pframecount = atoi(optarg);
				break;
			case 'R':
				read_reg(fd, strtol(optarg, NULL, 0));
				break;
			default:
				print_usage(argv[0]);
				close(fd);
				exit(EXIT_FAILURE);
		}
	}

	if (framerate != -1)
	{
		if (framerate > AIT_ISP_FRAMERATE_MAX || framerate < AIT_ISP_FRAMERATE_MIN)
		{
			fprintf(stderr, "framerate out of range\n");
		}
		else
		{
			AitXU_SetFrameRate(fd, framerate);
		}
	}

	if (bitrate != -1)
	{
		if (bitrate > AIT_ISP_BITRATE_MAX)
		{
			fprintf(stderr, "bitrate out of range\n");
		}
		else
		{
			AitXU_SetBitrate(fd, bitrate);
		}
	}

	if (mirror != -1)
	{
		if (mirror > (AIT_FM_FLIP | AIT_FM_MIRROR))
		{
			fprintf(stderr, "mirror out of range\n");
		}
		else
		{
			AitXU_SetMirrFlip(fd, mirror);
		}
	}

	if (irmode != -1)
	{
		if (irmode > AIT_IR_NIGHT)
		{
			fprintf(stderr, "irmode out of range\n");
		}
		else
		{
			AitXU_SetIRCutMode(fd, irmode);
		}
	}

	if (quality != -1)
	{
		if (quality > AIT_ISP_EX_MJPEG_QUALITY_MAX)
		{
			fprintf(stderr, "quality out of range\n");
		}
		else
		{
			AitXU_SetMjpgQuality(fd, quality);
		}
	}

	if (pframecount != -1)
	{
		if (pframecount > AIT_MMP_PFRAMECOUNT_MAX)
		{
			fprintf(stderr, "pframecount out of range\n");
		}
		else
		{
			AitXU_SetPFrameCount(fd, pframecount);
		}
	}

	close(fd);

}
