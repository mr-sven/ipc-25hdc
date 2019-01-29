#ifndef __FLASH_API
#define __FLASH_API

int flash_read(char *buf, off_t from, size_t len);
int flash_write(char *buf, off_t to, size_t len);
int flash_read_mac(char *buf);
#if ! defined (NO_WIFI_SOC)
int flash_read_wifi_mac(char *buf);
#endif

#endif
