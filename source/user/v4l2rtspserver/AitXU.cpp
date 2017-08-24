/*
 * AitXU.cpp
 *
 *  Created on: Aug 23, 2017
 *      Author: sven
 */


#include <iostream>
#include <sstream>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <byteswap.h>

#include "uvcvideo.h"

#include "AitXU.h"

uvc_xu_tbl_info xu_control_tbl[] = {
// Image Signal Processor
	{"Set ISP", AIT_SET_ISP_CONTROL, sizeof(struct AitIspCmd), 0, AIT_XU_SET, V4L2_CTRL_TYPE_INTEGER, UVC_CTRL_DATA_TYPE_SIGNED},
	{"Get ISP", AIT_GET_ISP_CONTROL, sizeof(struct AitIspCmd), 0, AIT_XU_GET, V4L2_CTRL_TYPE_INTEGER, UVC_CTRL_DATA_TYPE_SIGNED},
// Multimedia Processor
	{"Set MMP", AIT_SET_MMP_CONTROL, sizeof(struct AitMmpCmd), 0, AIT_XU_SET, V4L2_CTRL_TYPE_INTEGER, UVC_CTRL_DATA_TYPE_SIGNED},
	{"Get MMP", AIT_GET_MMP_CONTROL, sizeof(struct AitMmpCmd), 0, AIT_XU_GET, V4L2_CTRL_TYPE_INTEGER, UVC_CTRL_DATA_TYPE_SIGNED},

// current unused commands
//    {"Set MMP16", AIT_SET_MMP16_CONTROL, sizeof(struct AitMmp16Cmd), 0, AIT_XU_SET, V4L2_CTRL_TYPE_INTEGER, UVC_CTRL_DATA_TYPE_SIGNED},
//    {"Get MMP16 Result", AIT_GET_MMP16_CONTROL, sizeof(struct AitMmp16Cmd), 0, AIT_XU_GET, V4L2_CTRL_TYPE_INTEGER, UVC_CTRL_DATA_TYPE_SIGNED},
//    {"Set ISP EX", AIT_SET_ISPEX_CONTROL, sizeof(struct AitIspExCmd), 0, AIT_XU_GET, V4L2_CTRL_TYPE_INTEGER, UVC_CTRL_DATA_TYPE_SIGNED},
//    {"Set ISP EX Result", AIT_GET_ISPEX_CONTROL, sizeof(struct AitIspExCmd), 0, AIT_XU_GET, V4L2_CTRL_TYPE_INTEGER, UVC_CTRL_DATA_TYPE_SIGNED},
//    {"Set MMP MEM", AIT_SET_MMPMEM_CONTROL, sizeof(struct AitMmpMemCmd), 0, AIT_XU_SET, V4L2_CTRL_TYPE_INTEGER, UVC_CTRL_DATA_TYPE_SIGNED},
//    {"Get MMP MEM", AIT_GET_MMPMEM_CONTROL, sizeof(struct AitMmpMemCmd), 0, AIT_XU_GET, V4L2_CTRL_TYPE_INTEGER, UVC_CTRL_DATA_TYPE_SIGNED},

// commands added but not maped
//    {"Set Unknown", 11, 32, 0, AIT_XU_SET, V4L2_CTRL_TYPE_INTEGER, UVC_CTRL_DATA_TYPE_SIGNED},
//    {"Get Unknown", 3, 32, 0, AIT_XU_GET, V4L2_CTRL_TYPE_INTEGER, UVC_CTRL_DATA_TYPE_SIGNED},
};

int uvc_xu_tbl_cnt = sizeof(xu_control_tbl) / sizeof(uvc_xu_tbl_info);

int AitXU::Init(void)
{
    int i = 0;
    int value = 0;
    int ret = 0;
    struct uvc_xu_control_info info = {UVC_GUID_AIT_EXTENSION, 0, 0, 0, 0};
    struct uvc_xu_control_mapping map = {0, "", UVC_GUID_AIT_EXTENSION, 0, 0, 0, V4L2_CTRL_TYPE_INTEGER, 0};

    std::cout << "AitXU::Init" << std::endl;

    if (this->CheckInit())
    {
    	return 0;
    }

    for(i = 0; i < uvc_xu_tbl_cnt; i++)
    {
        info.index = i + 1 ;
        info.selector = xu_control_tbl[i].selector;
        info.size = xu_control_tbl[i].size;
        info.flags = xu_control_tbl[i].flag;

        std::cout << "AitXU::Init: UVCIOC_CTRL_ADD('" << xu_control_tbl[i].name << "')" << std::endl;
        if ((value = ioctl(this->m_fd, UVCIOC_CTRL_ADD, &info)) != -1)
        {
            map.id = V4L2_AIT_XU_ID_BASE + i;
            memcpy(map.name, xu_control_tbl[i].name, 32);
            map.selector = xu_control_tbl[i].selector;
            if(xu_control_tbl[i].size == 0xFF)
            {
                map.size = xu_control_tbl[i].size;
            }
            else
            {
                map.size = xu_control_tbl[i].size*8;
            }
            map.offset = xu_control_tbl[i].offset;
            map.v4l2_type = xu_control_tbl[i].v4l2_type;
            map.data_type = xu_control_tbl[i].data_type;

            std::cout << "AitXU::Init: UVCIOC_CTRL_MAP('" << xu_control_tbl[i].name << "')" << std::endl;
            if ((value = ioctl(this->m_fd, UVCIOC_CTRL_MAP, &map)) < 0)
            {
                ret = 1;
                std::cerr << "AitXU::Init: UVCIOC_CTRL_MAP('" << xu_control_tbl[i].name << "') error: " << strerror(errno) << std::endl;
            }
        }
        else
        {
            ret = 1;
            std::cerr << "AitXU::Init: UVCIOC_CTRL_ADD('" << xu_control_tbl[i].name << "') error: " << strerror(errno) << std::endl;
        }
    }

    return ret;
}

int AitXU::XuCmd_V2(__u8 * data, __u8 selector, __u16 size, __u8 mode, __u8 unit)
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
    return ioctl(this->m_fd, request, &xuctrl);
}

int AitXU::SetMirrFlip(__u8 mode)
{
    struct AitIspCmd cmd = {AIT_ISP_EXTENDED_CMD, AIT_ISP_EX_MIRRFLIP, mode, 0};
    std::cout << "AitXU::SetMirrFlip = " << std::dec << int(mode) << std::endl;
    int ret = this->IspCmd(cmd);
    std::cout << "AitXU::SetMirrFlip result = " << std::dec << int(cmd.data0) << std::endl;
    return ret;
}

int AitXU::SetIRCutMode(__u8 mode)
{
    struct AitIspCmd cmd = {AIT_ISP_EXTENDED_CMD, AIT_ISP_EX_IRCUTMODE, (mode << 8), 0};
    std::cout << "AitXU::SetIRCutMode = " << std::dec << int(mode) << std::endl;
    int ret = this->IspCmd(cmd);
    std::cout << "AitXU::SetIRCutMode result = " << std::dec << int(cmd.data0 >> 8) << std::endl;
    return ret;
}

int AitXU::SetMjpgQuality(__u8 quality)
{
    struct AitIspCmd cmd = {AIT_ISP_EXTENDED_CMD, AIT_ISP_EX_MJPEG_QUALITY, quality, 0};
    std::cout << "AitXU::SetMjpgQuality = " << std::dec << int(quality) << std::endl;
    int ret = this->IspCmd(cmd);
    std::cout << "AitXU::SetMjpgQuality result = " << std::dec << int(cmd.data0) << std::endl;
    return ret;
}

int AitXU::SetFrameRate(__u8 rate)
{
    if (rate < 5)
    {
        rate = 5;
    }

    if (rate > 30)
    {
        rate = 30;
    }
    struct AitIspCmd cmd = {AIT_ISP_FRAMERATE, rate, 0, 0};

    std::cout << "AitXU::SetFrameRate = " << std::dec << int(rate) << std::endl;
    int ret =  this->IspCmd(cmd);
    std::cout << "AitXU::SetFrameRate result = " << std::dec << int(cmd.cmd1) << std::endl;

    return ret;
}

int AitXU::SetBitrate(__u16 rate)
{
    struct AitIspCmd cmd = {AIT_ISP_BITRATE, 0xff, rate, 0};
    std::cout << "AitXU::SetBitrate = " << std::dec << int(rate) << std::endl;
    int ret = this->IspCmd(cmd);
    std::cout << "AitXU::SetBitrate result = " << std::dec << int(cmd.data0) << std::endl;
    return ret;
}

int AitXU::SetIFrame(void)
{
    struct AitIspCmd cmd = {AIT_ISP_IFRAME, 0, 0, 0};
    std::cout << "AitXU::SetIFrame" << std::endl;
    return this->IspCmd(cmd);
}

int AitXU::GetFWBuildDate(char* buf, int len)
{
    if (len < 12)
    {
        return 1;
    }
    struct AitIspCmd cmd = {AIT_ISP_FW_BUILDDATE, 0, 0, 0};
    this->IspCmd(cmd);

    __u8 data[0];
    this->XuCmd(data, AIT_GET_ISP_CONTROL, 8, UVC_GET_CUR);

    sprintf(buf, "20%.2s.%.3s.%.2s", &data[1], &data[3], &data[6]);
    return 0;
}

int AitXU::GetFWVersion(char* buf, int len)
{
    if (len < 18)
    {
        return 1;
    }
    struct AitIspCmd cmd = {AIT_ISP_FW_VERSION, 0, 0, 0};
    this->IspCmd(cmd);

    struct versionDev {
    	__u16 reseverd;
    	__u16 major;
    	__u16 minor;
    	__u16 patch;
    } version;

    this->XuCmd((__u8 *)&version, AIT_GET_ISP_CONTROL, 8, UVC_GET_CUR);

    sprintf(buf, "%d.%d.%d", __bswap_16(version.major), __bswap_16(version.minor), __bswap_16(version.patch));

    return 0;
}

bool AitXU::CheckInit(void)
{
	// concat filename
    std::string initFile = "/var/run/ait_" + std::string(basename(this->m_devName.c_str()));

    // check existing file
    struct stat buf;
    if (stat(initFile.c_str(), &buf) == 0)
    {
    	return true;
    }

    // touch file
    int fd = open(initFile.c_str(), O_CREAT|O_WRONLY, 0644);
    if (fd >= 0) close(fd);
	return false;
}

int AitXU::SetPFrameCount(__u8 count)
{
    struct AitMmpCmd cmd = {AIT_MMP_PFRAMECOUNT, count, 0, 0};

    std::cout << "AitXU::SetPFrameCount = " << std::dec << int(count) << std::endl;
    int ret =  this->XuCmd((__u8 *)&cmd, AIT_SET_MMP_CONTROL, sizeof(struct AitMmpCmd), UVC_SET_CUR);
    std::cout << "AitXU::SetPFrameCount result = " << std::dec << int(cmd.cmd1) << std::endl;

    return ret;
}

int AitXU::SetEncRes(__u8 res)
{
    struct AitMmpCmd cmd = {AIT_MMP_ENCODERES, res, 0, 0};

    std::cout << "AitXU::SetEncRes = " << std::dec << int(res) << std::endl;
    int ret =  this->XuCmd((__u8 *)&cmd, AIT_SET_MMP_CONTROL, sizeof(struct AitMmpCmd), UVC_SET_CUR);
    std::cout << "AitXU::SetEncRes result = " << std::dec << int(cmd.cmd1) << std::endl;

    return ret;
}

int AitXU::SetMode(__u8 mode)
{
    struct AitMmpCmd cmd = {AIT_MMP_MODE, mode, 0, 0};

    std::cout << "AitXU::SetMode = " << std::dec << int(mode) << std::endl;
    int ret =  this->XuCmd((__u8 *)&cmd, AIT_SET_MMP_CONTROL, sizeof(struct AitMmpCmd), UVC_SET_CUR);
    std::cout << "AitXU::SetMode result = " << std::dec << int(cmd.cmd1) << std::endl;

    return ret;
}
