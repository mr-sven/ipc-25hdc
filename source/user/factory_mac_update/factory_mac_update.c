#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include "nvram.h"

#define ETH_MAC_OFFSET  0x2E
#define WIFI_MAC_OFFSET 0x04

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

void get_nv_mac(const char *flag, const unsigned char mac[])
{
    const char* nv_mac = nvram_bufget(RT2860_NVRAM, flag);
//    printf("factory_wifimac=%s\n", factory_wifimac);
    sscanf(nv_mac, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
}

void check_mac(void)
{
    unsigned char nv_mac[6];
    unsigned char fc_mac[6];

    // init NVRAM 
    nvram_init(RT2860_NVRAM);

    // init MTD
    int fd = mtd_open("Factory", O_RDONLY);

    // load MAC from NVRAM
    get_nv_mac("factory_mac", nv_mac);

    // load MAC from MTD
    lseek(fd, ETH_MAC_OFFSET, SEEK_SET);
    read(fd, fc_mac, 6);

    if (memcmp(nv_mac, fc_mac, 6) != 0)
    {
        printf("not match\n");
        close(fd);
        nvram_close(RT2860_NVRAM);
        return;
    }

    // load MAC from NVRAM
    get_nv_mac("factory_wifimac", nv_mac);

    // load MAC from MTD
    lseek(fd, WIFI_MAC_OFFSET, SEEK_SET);
    read(fd, fc_mac, 6);

    if (memcmp(nv_mac, fc_mac, 6) != 0)
    {
        printf("not match\n");
        close(fd);
        nvram_close(RT2860_NVRAM);
        return;
    }

//    printf("%02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    printf("match\n");

    close(fd);
    nvram_close(RT2860_NVRAM);
}

void update_mac(void)
{
    unsigned char nv_mac[6];

    // init NVRAM 
    nvram_init(RT2860_NVRAM);

    // init MTD
    int fd = mtd_open("Factory", O_RDWR);

    // load MAC from NVRAM
    get_nv_mac("factory_mac", nv_mac);

    // load MAC to MTD
    lseek(fd, ETH_MAC_OFFSET, SEEK_SET);
    write(fd, nv_mac, 6);

    // load MAC from NVRAM
    get_nv_mac("factory_wifimac", nv_mac);

    // load MAC to MTD
    lseek(fd, WIFI_MAC_OFFSET, SEEK_SET);
    write(fd, nv_mac, 6);

    close(fd);
    nvram_close(RT2860_NVRAM);

}

void usage(char *cmd)
{
    printf("Usage: %s c - check NVRAM macs\n", cmd);
    printf("       %s u - update from NVRAM to factory\n", cmd);
    exit(0);
}

int main(int argc, char *argv[])
{
    if (argc < 2)
        usage(argv[0]);

    switch (argv[1][0]) {
    case 'c':
        check_mac();
        break;
    case 'u':
        update_mac();
        break;
    default:
        usage(argv[0]);
    }

    return 0;
}
