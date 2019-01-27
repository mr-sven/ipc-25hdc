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

#include "AitXU.h"

int main(int argc, char *argv[])
{
    char *dev = "/dev/video0";
    int fd = 0;

    __u8 data[8];

    if ((fd = open(dev, O_RDWR)) == -1) 
    {
        perror("ERROR opening V4L interface \n");
        exit(1);
    }

    AitXU_Init(fd);

    memset(data, 0, 8);
    AitXU_GetFWBuildDate(fd, data, 8);

    int cnt;
    for (cnt = 0; cnt < 8; cnt++) {
        printf(" 0x%02x", data[cnt]);
    }

    printf("\n");

    memset(data, 0, 8);
    AitXU_GetFWVersion(fd, data, 8);

    for (cnt = 0; cnt < 8; cnt++) {
        printf(" 0x%02x", data[cnt]);
    }
    printf("\n");

    H264SetMirr(fd, AIT_FM_NORMAL);
    H264SetIRCutMode(fd, 2);

    AitXU_SetFrameRate(fd, 5);

    close(fd);
}
