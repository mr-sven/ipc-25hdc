/*
 * AitXU.h
 *
 *  Created on: Aug 23, 2017
 *      Author: sven
 */

#ifndef INCLUDE_AITXU_H_
#define INCLUDE_AITXU_H_

#include <string>
#include <linux/videodev2.h>

#define UVC_SET_CUR					0x01
#define UVC_GET_CUR					0x81

#define UVC_VC_EXTENSION_UNIT		0x06

#define UVC_GUID_AIT_EXTENSION		{0xd0, 0x9e, 0xe4, 0x23, 0x78, 0x11, 0x31, 0x4f, 0xae, 0x52, 0xd2, 0xfb, 0x8a, 0x8d, 0x3b, 0x48}
#define V4L2_AIT_XU_ID_BASE			0x08000000

#define AIT_XU_GET					(UVC_CONTROL_GET_CUR | UVC_CONTROL_GET_DEF | UVC_CONTROL_GET_MIN | UVC_CONTROL_GET_MAX)
#define AIT_XU_SET					(UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_DEF | UVC_CONTROL_GET_MIN | UVC_CONTROL_GET_MAX)

#define AIT_SET_ISP_CONTROL			1
#define AIT_GET_ISP_CONTROL			2
#define AIT_SET_MMP_CONTROL			4
#define AIT_GET_MMP_CONTROL			5
#define AIT_SET_MMP16_CONTROL		14
#define AIT_GET_MMP16_CONTROL		15
#define AIT_SET_ISPEX_CONTROL		6
#define AIT_GET_ISPEX_CONTROL		7
#define AIT_SET_MMPMEM_CONTROL		9
#define AIT_GET_MMPMEM_CONTROL		8

#define AIT_FM_NORMAL				0
#define AIT_FM_FLIP					1
#define AIT_FM_MIRROR				2

#define AIT_IR_DEFAULT				0
#define AIT_IR_DAY					1
#define AIT_IR_NIGHT				2

#define AIT_RES_1					1
#define AIT_RES_2					2
#define AIT_RES_H264_1280X720		3

#define AIT_MODE_88					0x88

// Image Signal Processor
#define AIT_ISP_FRAMERATE			2
#define AIT_ISP_IFRAME				4
#define AIT_ISP_BITRATE				8
#define AIT_ISP_FW_VERSION			11
#define AIT_ISP_FW_BUILDDATE		12
#define AIT_ISP_EXTENDED_CMD		0xff

#define AIT_ISP_EX_MIRRFLIP			17
#define AIT_ISP_EX_MJPEG_QUALITY	19
#define AIT_ISP_EX_IRCUTMODE		39

// Multimedia Processor
#define AIT_MMP_PFRAMECOUNT			9
#define AIT_MMP_ENCODERES			11
#define AIT_MMP_MODE				12

typedef struct uvc_xu_tbl_info {
    __u8 name[32];
    __u8 selector;
    __u16 size;
    __u8 offset;
    __u32 flag;
    enum v4l2_ctrl_type v4l2_type;
    __u32 data_type;
} uvc_xu_tbl_info;

struct AitIspCmd {
    __u8 cmd0;
    __u8 cmd1;
    __u16 data0;
    __u32 data1;
};

struct AitMmpCmd {
    __u8 cmd0;
    __u8 cmd1;
    __u16 data0;
    __u32 data1;
};

class AitXU
{
	public:
		AitXU(int fd, std::string devName) : m_fd(fd), m_devName(devName) {}
		int Init(void);
		int SetMirrFlip(__u8 mode);
		int SetIRCutMode(__u8 mode);
		int SetMjpgQuality(__u8 quality);
		int SetFrameRate(__u8 rate);
		int SetBitrate(__u16 rate);
		int SetIFrame(void);
		int GetFWBuildDate(char* buf, int len);
		int GetFWVersion(char* buf, int len);
		int SetPFrameCount(__u8 count);
		int SetEncRes(__u8 res);
		int SetMode(__u8 mode);
	private:
		int m_fd;
		std::string m_devName;

		int XuCmd_V2(__u8 * data, __u8 selector, __u16 size, __u8 mode, __u8 unit);
		int XuCmd(__u8 * data, __u8 selector, __u16 size, __u8 mode)
		{
		    return this->XuCmd_V2(data, selector, size, mode, UVC_VC_EXTENSION_UNIT);
		}
		int IspCmd(struct AitIspCmd data)
		{
		    return this->XuCmd((__u8 *)&data, AIT_SET_ISP_CONTROL, sizeof(struct AitIspCmd), UVC_SET_CUR);
		}

		bool CheckInit(void);
};

#endif /* INCLUDE_AITXU_H_ */
