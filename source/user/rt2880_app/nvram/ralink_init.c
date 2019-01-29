#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include "nvram.h"
#include "flash_api.h"

#include <linux/autoconf.h>

#define DEFAULT_FLASH_ZONE_NAME "2860"

int set_usage(char *aout)
{
	printf("Usage example: \n");
	printf("\t%s 2860 lan_ipaddr 1.2.3.4\n", aout);
	return -1;
}

int ra_nv_set(int argc,char **argv)
{
	int index, rc;
	char *fz, *key, *value;

	if (argc == 1 || argc > 5)
		return set_usage(argv[0]);

	if (argc == 2) {
		fz = DEFAULT_FLASH_ZONE_NAME;
		key = argv[1];
		value = "";
	} else if (argc == 3) {
		fz = DEFAULT_FLASH_ZONE_NAME;
		key = argv[1];
		value = argv[2];
	} else if (argc == 4) {
		fz = argv[1];
		key = argv[2];
		value = argv[3];
	}

	if ((index = getNvramIndex(fz)) == -1) {
		printf("%s: Error: \"%s\" flash zone not existed\n", argv[0], fz);
		return set_usage(argv[0]);
	}

	nvram_init(index);
	rc = nvram_set(index, key, value);
	nvram_close(index);
	return rc;
}

int get_usage(char *aout)
{
	printf("Usage: \n");
	printf("\t%s 2860 lan_ipaddr\n", aout);
	return -1;
}

int ra_nv_get(int argc, char *argv[])
{
	char *fz;
	char *key;
	char *value;

	int index;

	if (argc != 3 && argc != 2)
		return get_usage(argv[0]);

	if (argc == 2) {
		fz = DEFAULT_FLASH_ZONE_NAME;
		key = argv[1];
	} else {
		fz = argv[1];
		key = argv[2];
	}

	if ((index = getNvramIndex(fz)) == -1) {
		printf("%s: Error: \"%s\" flash zone not existed\n", argv[0], fz);
		return get_usage(argv[0]);
	}

	nvram_init(index);
	printf("%s\n", nvram_bufget(index, key));
	nvram_close(index);
	return 0;
}

void usage(char *cmd)
{
	printf("Usage:\n");
	printf("  %s <command> [<platform>] [<file>]\n\n", cmd);
	printf("command:\n");
	printf("  rt2860_nvram_show - display rt2860 values in nvram\n");
	printf("  show    - display values in nvram for <platform>\n");
	printf("  renew   - replace nvram values for <platform> with <file>\n");
	printf("  clear   - clear all entries in nvram for <platform>\n");
	printf("platform:\n");
	printf("  2860    - rt2860\n");
	printf("file:\n");
	printf("          - file name for renew command\n");
	exit(0);
}


int renew_nvram(int mode, char *fname)
{
	FILE *fp;
#define BUFSZ 1024
	unsigned char buf[BUFSZ], *p;
	unsigned char wan_mac[32];
	int found = 0, need_commit = 0;

	if (NULL == (fp = fopen(fname, "ro"))) {
		perror("fopen");
		return -1;
	}

	//find "Default" first
	while (NULL != fgets(buf, BUFSZ, fp)) {
		if (buf[0] == '\n' || buf[0] == '#')
			continue;
		if (!strncmp(buf, "Default\n", 8)) {
			found = 1;
			break;
		}
	}
	if (!found) {
		printf("file format error!\n");
		fclose(fp);
		return -1;
	}

	nvram_init(mode);
	while (NULL != fgets(buf, BUFSZ, fp)) {
		if (buf[0] == '\n' || buf[0] == '#')
			continue;
		if (NULL == (p = strchr(buf, '='))) {
			if (need_commit)
				nvram_commit(mode);
			printf("%s file format error!\n", fname);
			fclose(fp);
			return -1;
		}
		buf[strlen(buf) - 1] = '\0'; //remove carriage return
		*p++ = '\0'; //seperate the string
		//printf("bufset %d '%s'='%s'\n", mode, buf, p);
		nvram_bufset(mode, buf, p);
		need_commit = 1;
	}

	//Get wan port mac address, please refer to eeprom format doc
	//0x30000=user configure, 0x32000=rt2860 parameters, 0x40000=RF parameter
	flash_read_mac(buf);
	sprintf(wan_mac,"%02X:%02X:%02X:%02X:%02X:%02X",buf[0],buf[1],buf[2],buf[3],buf[4],buf[5]);
	nvram_bufset(RT2860_NVRAM, "factory_mac", wan_mac);

	flash_read_wifi_mac(buf);
	sprintf(wan_mac,"%02X:%02X:%02X:%02X:%02X:%02X",buf[0],buf[1],buf[2],buf[3],buf[4],buf[5]);
	nvram_bufset(RT2860_NVRAM, "factory_wifimac", wan_mac);

	need_commit = 1;

	if (need_commit)
		nvram_commit(mode);

	nvram_close(mode);
	fclose(fp);
	return 0;
}

int nvram_show(int mode)
{
	char *buffer, *p;
	int rc;
	int crc;
	unsigned int len = 0x4000;

	nvram_init(mode);
	len = getNvramBlockSize(mode);
	buffer = malloc(len);
	if (buffer == NULL) {
		fprintf(stderr, "nvram_show: Can not allocate memory!\n");
		return -1;
	}
	flash_read(buffer, getNvramOffset(mode), len);
	memcpy(&crc, buffer, 4);

	fprintf(stderr, "crc = %x\n", crc);
	p = buffer + 4;
	while (*p != '\0') {
		printf("%s\n", p);
		p += strlen(p) + 1;
	}

	free(buffer);
	return 0;
}

int main(int argc, char *argv[])
{
	char *cmd;


	//call nvram_get or nvram_set
	if ((cmd = strrchr(argv[0], '/')) != NULL)
		cmd++;
	else
		cmd = argv[0];
	if (!strncmp(cmd, "nvram_get", 10))
		return ra_nv_get(argc, argv);
	else if (!strncmp(cmd, "nvram_set", 10))
		return ra_nv_set(argc, argv);

	if (argc < 2)
		usage(argv[0]);

	if (argc == 2) {
		if (!strncmp(argv[1], "rt2860_nvram_show", 18))
			nvram_show(RT2860_NVRAM);
		else
			usage(argv[0]);
	} else if (argc == 3) {
		if (!strncasecmp(argv[1], "show", 5)) {
			if (!strncmp(argv[2], "2860", 5) ||
			    !strncasecmp(argv[2], "rt2860", 7)) //b-compatible
				nvram_show(RT2860_NVRAM);
			else
				usage(argv[0]);
		} else if(!strncasecmp(argv[1], "clear", 6)) {
			if (!strncmp(argv[2], "2860", 5) || 
			    !strncasecmp(argv[2], "rt2860", 7)) //b-compatible
				nvram_clear(RT2860_NVRAM);
			else
				usage(argv[0]);
		} else
			usage(argv[0]);
	} else if (argc == 4) {
		if (!strncasecmp(argv[1], "renew", 6)) {
			if (!strncmp(argv[2], "2860", 5) ||
			    !strncasecmp(argv[2], "rt2860", 7)) //b-compatible
				renew_nvram(RT2860_NVRAM, argv[3]);
		} else
			usage(argv[0]);
	} else
		usage(argv[0]);
	return 0;
}
