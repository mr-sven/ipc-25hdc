#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include "nvram.h"


int mtd_open(const char *mtd, int flags)
{
    FILE *fp;
    char dev[PATH_MAX];
    int i;
    int ret;

    if ((fp = fopen("/proc/mtd", "r"))) 
    {
        while (fgets(dev, sizeof(dev), fp)) 
        {
            if (sscanf(dev, "mtd%d:", &i) && strstr(dev, mtd)) 
            {
                snprintf(dev, sizeof(dev), "/dev/mtd/%d", i);
                if ((ret = open(dev, flags)) < 0) 
                {
                    snprintf(dev, sizeof(dev), "/dev/mtd%d", i);
                    ret = open(dev, flags);
                }
                fclose(fp);
                return ret;
            }
        }
        fclose(fp);
    }

    return open(mtd, flags);
}


int main (void)
{
    nvram_init(RT2860_NVRAM);
    const char* factory_wifimac = nvram_bufget(RT2860_NVRAM, "factory_wifimac");
    const char* factory_mac = nvram_bufget(RT2860_NVRAM, "factory_mac");

    printf("factory_wifimac=%s\n", factory_wifimac);
    printf("factory_mac=%s\n", factory_mac);

    int fd = mtd_open("Factory", O_RDONLY);

    unsigned char mac_buf[6];

    lseek(fd, 0x04, SEEK_SET);
    read(fd, mac_buf, 6);

    printf("factory_wifimac=%02X:%02X:%02X:%02X:%02X:%02X\n", mac_buf[0], mac_buf[1], mac_buf[2], mac_buf[3], mac_buf[4], mac_buf[5]);

    lseek(fd, 0x2e, SEEK_SET);
    read(fd, mac_buf, 6);

    printf("factory_mac=%02X:%02X:%02X:%02X:%02X:%02X\n", mac_buf[0], mac_buf[1], mac_buf[2], mac_buf[3], mac_buf[4], mac_buf[5]);

    close(fd);
    nvram_close(RT2860_NVRAM);
    return 0;
}
