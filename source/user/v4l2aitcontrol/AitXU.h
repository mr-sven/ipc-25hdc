
#ifndef _AITXU_H_
#define _AITXU_H_

#define UVC_SET_CUR					0x01
#define UVC_GET_CUR					0x81

#define UVC_VC_EXTENSION_UNIT				0x06

#define UVC_GUID_AIT_EXTENSION      {0xd0, 0x9e, 0xe4, 0x23, 0x78, 0x11, 0x31, 0x4f, 0xae, 0x52, 0xd2, 0xfb, 0x8a, 0x8d, 0x3b, 0x48}
#define V4L2_AIT_XU_ID_BASE         0x08000000

#define AIT_XU_GET                  (UVC_CONTROL_GET_CUR | UVC_CONTROL_GET_DEF | UVC_CONTROL_GET_MIN | UVC_CONTROL_GET_MAX)
#define AIT_XU_SET                  (UVC_CONTROL_SET_CUR | UVC_CONTROL_GET_DEF | UVC_CONTROL_GET_MIN | UVC_CONTROL_GET_MAX)

#define AIT_SET_ISP_CONTROL         1
#define AIT_GET_ISP_CONTROL         2
#define AIT_SET_MMP_CONTROL         4
#define AIT_GET_MMP_CONTROL         5
#define AIT_SET_MMP16_CONTROL       14
#define AIT_GET_MMP16_CONTROL       15
#define AIT_SET_ISPEX_CONTROL       6
#define AIT_GET_ISPEX_CONTROL       7
#define AIT_SET_MMPMEM_CONTROL      9
#define AIT_GET_MMPMEM_CONTROL      8

#define AIT_FM_NORMAL               0
#define AIT_FM_FLIP                 1
#define AIT_FM_MIRROR               2

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

int AitXU_Init(int fd);
int H264SetMirr(int fd, __u8 mode);
int H264SetIRCutMode(int fd, __u8 mode);
int MjpgSetQuality(int fd, __u8 quality);
int AitXU_SetFrameRate(int fd, __u8 rate);
int AitXU_SetBitrate(int fd, int rate);
int AitXU_SetIFrame(int fd);
int AitXU_GetFWBuildDate(int fd, __u8 * data, int len);
int AitXU_GetFWVersion(int fd, __u8 * data, int len);

#endif /* _AITXU_H_ */
