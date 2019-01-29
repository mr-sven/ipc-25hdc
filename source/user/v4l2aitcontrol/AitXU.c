#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <errno.h>

#include "uvcvideo.h"

#include "AitXU.h"

uvc_xu_tbl_info xu_control_tbl[] = {
// Image Signal Processor
	{ "Set ISP", AIT_SET_ISP_CONTROL, sizeof(struct AitIspCmd), 0, AIT_XU_SET, V4L2_CTRL_TYPE_INTEGER, UVC_CTRL_DATA_TYPE_SIGNED },
	{ "Get ISP", AIT_GET_ISP_CONTROL, sizeof(struct AitIspCmd), 0, AIT_XU_GET, V4L2_CTRL_TYPE_INTEGER, UVC_CTRL_DATA_TYPE_SIGNED },
// Multimedia Processor
	{ "Set MMP", AIT_SET_MMP_CONTROL, sizeof(struct AitMmpCmd), 0, AIT_XU_SET, V4L2_CTRL_TYPE_INTEGER, UVC_CTRL_DATA_TYPE_SIGNED },
	{ "Get MMP", AIT_GET_MMP_CONTROL, sizeof(struct AitMmpCmd), 0, AIT_XU_GET, V4L2_CTRL_TYPE_INTEGER, UVC_CTRL_DATA_TYPE_SIGNED },
// Multimedia Processor 16
	{"Set MMP16", AIT_SET_MMP16_CONTROL, sizeof(struct AitMmp16Cmd), 0, AIT_XU_SET, V4L2_CTRL_TYPE_INTEGER, UVC_CTRL_DATA_TYPE_SIGNED},
	{"Get MMP16 Result", AIT_GET_MMP16_CONTROL, sizeof(struct AitMmp16Cmd), 0, AIT_XU_GET, V4L2_CTRL_TYPE_INTEGER, UVC_CTRL_DATA_TYPE_SIGNED},

/* {"Set ISP EX", AIT_SET_ISPEX_CONTROL, sizeof(struct AitIspExCmd), 0, AIT_XU_GET, V4L2_CTRL_TYPE_INTEGER, UVC_CTRL_DATA_TYPE_SIGNED},
 {"Set ISP EX Result", AIT_GET_ISPEX_CONTROL, sizeof(struct AitIspExCmd), 0, AIT_XU_GET, V4L2_CTRL_TYPE_INTEGER, UVC_CTRL_DATA_TYPE_SIGNED},
 {"Set MMP MEM", AIT_SET_MMPMEM_CONTROL, sizeof(struct AitMmpMemCmd), 0, AIT_XU_SET, V4L2_CTRL_TYPE_INTEGER, UVC_CTRL_DATA_TYPE_SIGNED},
 {"Get MMP MEM", AIT_GET_MMPMEM_CONTROL, sizeof(struct AitMmpMemCmd), 0, AIT_XU_GET, V4L2_CTRL_TYPE_INTEGER, UVC_CTRL_DATA_TYPE_SIGNED},*/
// commands added but not maped
//	{"Set Unknown", 11, 32, 0, AIT_XU_SET, V4L2_CTRL_TYPE_INTEGER, UVC_CTRL_DATA_TYPE_SIGNED},
//	{"Get Unknown", 3, 32, 0, AIT_XU_SET, V4L2_CTRL_TYPE_INTEGER, UVC_CTRL_DATA_TYPE_SIGNED},
};

int uvc_xu_tbl_cnt = sizeof(xu_control_tbl) / sizeof(uvc_xu_tbl_info);

int AitXU_Init(int fd)
{
	int i = 0;
	int value = 0;
	int ret = 0;
	struct uvc_xu_control_info info = { UVC_GUID_AIT_EXTENSION, 0, 0, 0, 0 };
	struct uvc_xu_control_mapping map = { 0, "", UVC_GUID_AIT_EXTENSION, 0, 0, 0, 0, 0, 0 };

	printf("AitXU_Init\n");
	for (i = 0; i < uvc_xu_tbl_cnt; i++)
	{
		info.index = i + 1;
		info.selector = xu_control_tbl[i].selector;
		info.size = xu_control_tbl[i].size;
		info.flags = xu_control_tbl[i].flag;

		printf("AitXU_Init: UVCIOC_CTRL_ADD('%s')\n", xu_control_tbl[i].name);
		if ((value = ioctl(fd, UVCIOC_CTRL_ADD, &info)) != -1)
		{
			map.id = V4L2_AIT_XU_ID_BASE + i;
			memcpy(map.name, xu_control_tbl[i].name, 32);
			map.selector = xu_control_tbl[i].selector;
			if (xu_control_tbl[i].size == 0xFF)
			{
				map.size = xu_control_tbl[i].size;
			}
			else
			{
				map.size = xu_control_tbl[i].size * 8;
			}
			map.offset = xu_control_tbl[i].offset;
			map.v4l2_type = xu_control_tbl[i].v4l2_type;
			map.data_type = xu_control_tbl[i].data_type;

			printf("AitXU_Init: UVCIOC_CTRL_MAP('%s')\n", xu_control_tbl[i].name);
			if ((value = ioctl(fd, UVCIOC_CTRL_MAP, &map)) < 0)
			{
				ret = 1;
				fprintf(stderr, "AitXU_Init: UVCIOC_CTRL_MAP('%s') error: %s\n", xu_control_tbl[i].name, strerror(errno));
			}
		}
		else
		{
			if (errno == EEXIST)
			{
				printf("AitXU_Init: XU '%s' added already\n", xu_control_tbl[i].name);
			}
			else
			{
				ret = 1;
				fprintf(stderr, "AitXU_Init: UVCIOC_CTRL_ADD('%s') error: %s\n", xu_control_tbl[i].name, strerror(errno));
			}
		}
	}

	return ret;
}

int UVC_XuCmd_V2(int fd, __u8 * data, __u8 selector, __u16 size, __u8 mode, __u8 unit)
{
	unsigned long request;
	struct uvc_xu_control xuctrl;

	xuctrl.unit = unit;
	xuctrl.selector = selector;
	xuctrl.size = size;
	xuctrl.data = data;

	if (mode == UVC_SET_CUR)
	{
		request = UVCIOC_CTRL_SET;
	}
	else
	{
		request = UVCIOC_CTRL_GET;
	}
	return ioctl(fd, request, &xuctrl);
}

static inline int AitXU_XuCmd(int fd, __u8 * data, __u8 selector, __u16 size, __u8 mode)
{
	return UVC_XuCmd_V2(fd, data, selector, size, mode, UVC_VC_EXTENSION_UNIT);
}

static inline int AitXU_IspCmd(int fd, struct AitIspCmd data)
{
	return AitXU_XuCmd(fd, (__u8 *) &data, AIT_SET_ISP_CONTROL, sizeof(struct AitIspCmd), UVC_SET_CUR);
}

int AitXU_SetMirrFlip(int fd, __u8 mode)
{
	struct AitIspCmd cmd = { AIT_ISP_EXTENDED_CMD, AIT_ISP_EX_MIRRFLIP, mode, 0 };
	int ret = AitXU_IspCmd(fd, cmd);

	printf("AitXU_SetMirrFlip: mode = %d, ret = %d\n", mode, ret);
	return ret;
}

int AitXU_SetIRCutMode(int fd, __u8 mode)
{
	struct AitIspCmd cmd = { AIT_ISP_EXTENDED_CMD, AIT_ISP_EX_IRCUTMODE, (mode << 8), 0 };
	int ret = AitXU_IspCmd(fd, cmd);

	printf("AitXU_SetIRCutMode: mode = %d, ret = %d\n", mode, ret);
	return ret;
}

int AitXU_SetMjpgQuality(int fd, __u8 quality)
{
	struct AitIspCmd cmd = { AIT_ISP_EXTENDED_CMD, AIT_ISP_EX_MJPEG_QUALITY, quality, 0 };
	int ret = AitXU_IspCmd(fd, cmd);

	printf("AitXU_SetMjpgQuality: quality = %d, ret = %d\n", quality, ret);
	return ret;
}

int AitXU_SetFrameRate(int fd, __u8 rate)
{
	if (rate < AIT_ISP_FRAMERATE_MIN)
	{
		rate = AIT_ISP_FRAMERATE_MIN;
	}

	if (rate > AIT_ISP_FRAMERATE_MAX)
	{
		rate = AIT_ISP_FRAMERATE_MAX;
	}
	struct AitIspCmd cmd = { AIT_ISP_FRAMERATE, rate, 0, 0 };
	int ret = AitXU_IspCmd(fd, cmd);

	printf("AitXU_SetFrameRate: rate = %d, ret = %d\n", rate, ret);
	return ret;
}

int AitXU_SetBitrate(int fd, int rate)
{
	if (rate > AIT_ISP_BITRATE_MAX)
	{
		return 1;
	}

	struct AitIspCmd cmd = { AIT_ISP_BITRATE, 0xff, rate, 0 };
	int ret = AitXU_IspCmd(fd, cmd);

	printf("AitXU_SetBitrate: rate = %d, ret = %d\n", rate, ret);
	return ret;
}

int AitXU_SetIFrame(int fd)
{
	struct AitIspCmd cmd = { AIT_ISP_IFRAME, 0, 0, 0 };
	int ret = AitXU_IspCmd(fd, cmd);

	printf("AitXU_SetIFrame: ret = %d\n", ret);
	return ret;
}

int AitXU_GetFWBuildDate(int fd, __u8 * data, int len)
{
	if (len != 8)
	{
		return 1;
	}
	struct AitIspCmd cmd = { AIT_ISP_FW_BUILDDATE, 0, 0, 0 };

	AitXU_IspCmd(fd, cmd);
	return AitXU_XuCmd(fd, data, AIT_GET_ISP_CONTROL, 8, UVC_GET_CUR);
}

int AitXU_GetFWVersion(int fd, __u8 * data, int len)
{
	if (len != 8)
	{
		return 1;
	}
	struct AitIspCmd cmd = { AIT_ISP_FW_VERSION, 0, 0, 0 };

	AitXU_IspCmd(fd, cmd);
	return AitXU_XuCmd(fd, data, AIT_GET_ISP_CONTROL, 8, UVC_GET_CUR);
}

int AitXU_SetPFrameCount(int fd, __u8 count)
{
	struct AitMmpCmd cmd = { AIT_MMP_PFRAMECOUNT, count, 0, 0 };
	int ret = AitXU_XuCmd(fd, (__u8 *) &cmd, AIT_SET_MMP_CONTROL, sizeof(struct AitMmpCmd), UVC_SET_CUR);

	printf("AitXU_SetPFrameCount: count = %d, ret = %d\n", count, ret);
	return ret;
}

int AitXU_SetEncRes(int fd, __u8 res)
{
	struct AitMmpCmd cmd = { AIT_MMP_ENCODERES, res, 0, 0 };
	int ret = AitXU_XuCmd(fd, (__u8 *) &cmd, AIT_SET_MMP_CONTROL, sizeof(struct AitMmpCmd), UVC_SET_CUR);

	printf("AitXU_SetEncRes: res = %d, ret = %d\n", res, ret);
	return ret;
}

int AitXU_SetMode(int fd, __u8 mode)
{
	struct AitMmpCmd cmd = { AIT_MMP_MODE, mode, 0, 0 };
	int ret = AitXU_XuCmd(fd, (__u8 *) &cmd, AIT_SET_MMP_CONTROL, sizeof(struct AitMmpCmd), UVC_SET_CUR);

	printf("AitXU_SetMode: mode = %d, ret = %d\n", mode, ret);
	return ret;
}

int AitXU_TriggerMJPEG(int fd)
{
	struct AitMmp16Cmd cmd = { AIT_MMP16_TRIGGER, 0 };
	int ret = AitXU_XuCmd(fd, (__u8 *) &cmd, AIT_SET_MMP16_CONTROL, sizeof(cmd), UVC_SET_CUR);

	printf("AitXU_TriggerMJPEG: ret = %d\n", ret);
	return ret;
}

int AitXU_ReadReg(int fd, __u16 reg, __u8 * data)
{
	struct AitIspCmd cmd = { AIT_ISP_REGISTER, 0, reg, 0 };

	AitXU_IspCmd(fd, cmd);
	return AitXU_XuCmd(fd, data, AIT_GET_ISP_CONTROL, 8, UVC_GET_CUR);
}

int AitXU_GetVideoOption(int fd, __u8 * data)
{
	struct AitMmp16Cmd cmd = { AIT_MMP16_VIDEO_OPTION, AIT_MMP16_VIDEO_OPTION_GET, 0};
	int ret = AitXU_XuCmd(fd, (__u8 *) &cmd, AIT_SET_MMP16_CONTROL, sizeof(cmd), UVC_SET_CUR);
	return AitXU_XuCmd(fd, data, AIT_GET_MMP16_CONTROL, sizeof(cmd), UVC_GET_CUR);
}
