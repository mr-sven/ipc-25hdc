#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <wait.h>

#include "include/linux/autoconf.h"	/* !!! for CONFIG_MTD_KERNEL_PART_SIZ  !!! */
                                        /*   CONFIG_RT2880_ROOTFS_IN_FLASH */
                                        /*   CONFIG_RT2880_ROOTFS_IN_RAM   */

#include "fwflash.h"

void print_usage(char *progname)
{
	fprintf(stderr, "------------------------------------------------------------------\n");
	fprintf(stderr, "Usage: %s <uImage>\n" \
			" <uImage>........: path to image file\n", progname);
	fprintf(stderr, "------------------------------------------------------------------\n");
}

unsigned int getMTDPartSize(char *part)
{
	char buf[128], name[32], size[32], dev[32], erase[32];
	unsigned int result=0;
	FILE *fp = fopen("/proc/mtd", "r");

	if(!fp)
	{
		fprintf(stderr, "mtd support not enable?");
		return 0;
	}

	while(fgets(buf, sizeof(buf), fp))
	{
		sscanf(buf, "%s %s %s %s", dev, size, erase, name);
		if(!strcmp(name, part))
		{
			result = strtol(size, NULL, 16);
			break;
		}
	}

	fclose(fp);
	return result;
}

/*
 *  taken from "mkimage -l" with few modified....
 */
int check(char *imagefile, int offset, int len, char *err_msg)
{
	struct stat sbuf;

	int  data_len;
	char *data;
	unsigned char *ptr;
	unsigned long checksum;

	image_header_t header;
	image_header_t *hdr = &header;

	int ifd;

	if ((unsigned)len < sizeof(image_header_t))
	{
		sprintf (err_msg, "Bad size: \"%s\" is no valid image\n", imagefile);
		return 0;
	}

	ifd = open(imagefile, O_RDONLY);
	if(!ifd)
	{
		sprintf (err_msg, "Can't open %s: %s\n", imagefile, strerror(errno));
		return 0;
	}

	if (fstat(ifd, &sbuf) < 0)
	{
		close(ifd);
		sprintf (err_msg, "Can't stat %s: %s\n", imagefile, strerror(errno));
		return 0;
	}

	ptr = (unsigned char *) mmap(0, sbuf.st_size, PROT_READ, MAP_SHARED, ifd, 0);
	if ((caddr_t)ptr == (caddr_t)-1)
	{
		close(ifd);
		sprintf (err_msg, "Can't mmap %s: %s\n", imagefile, strerror(errno));
		return 0;
	}
	ptr += offset;

	// handle Header CRC32
	memcpy (hdr, ptr, sizeof(image_header_t));
	if (ntohl(hdr->ih_magic) != IH_MAGIC)
	{
		munmap(ptr, len);
		close(ifd);
		sprintf (err_msg, "Bad Magic Number: \"%s\" is no valid image\n", imagefile);
		return 0;
	}

	data = (char *)hdr;

	checksum = ntohl(hdr->ih_hcrc);
	
	// clear for re-calculation
	hdr->ih_hcrc = htonl(0);

	if (crc32 (0, data, sizeof(image_header_t)) != checksum)
	{
		munmap(ptr, len);
		close(ifd);
		sprintf (err_msg, "*** Warning: \"%s\" has bad header checksum!\n", imagefile);
		return 0;
	}

	// handle Data CRC32
	data = (char *)(ptr + sizeof(image_header_t));
	data_len  = len - sizeof(image_header_t) ;

	if (crc32 (0, data, data_len) != ntohl(hdr->ih_dcrc))
	{
		munmap(ptr, len);
		close(ifd);
		sprintf (err_msg, "*** Warning: \"%s\" has corrupted data!\n", imagefile);
		return 0;
	}

	// compare MTD partition size and image size
#if defined (CONFIG_RT2880_ROOTFS_IN_RAM)
	if(len > getMTDPartSize("\"Kernel\""))
	{
		munmap(ptr, len);
		close(ifd);
		sprintf(err_msg, "*** Warning: the image file(0x%x) is bigger than Kernel MTD partition.\n", len);
		return 0;
	}
#elif defined (CONFIG_RT2880_ROOTFS_IN_FLASH)
#ifdef CONFIG_ROOTFS_IN_FLASH_NO_PADDING
	if(len > getMTDPartSize("\"Kernel_RootFS\""))
	{
		munmap(ptr, len);
		close(ifd);
		sprintf(err_msg, "*** Warning: the image file(0x%x) is bigger than Kernel_RootFS MTD partition.\n", len);
		return 0;
	}
#else
	if(len < CONFIG_MTD_KERNEL_PART_SIZ)
	{
		munmap(ptr, len);
		close(ifd);
		sprintf(err_msg, "*** Warning: the image file(0x%x) size doesn't make sense.\n", len);
		return 0;
	}

	if((len - CONFIG_MTD_KERNEL_PART_SIZ) > getMTDPartSize("\"RootFS\""))
	{
		munmap(ptr, len);
		close(ifd);
		sprintf(err_msg, "*** Warning: the image file(0x%x) is bigger than RootFS MTD partition.\n", len - CONFIG_MTD_KERNEL_PART_SIZ);
		return 0;
	}
#endif
#else
#error "fwflash: no CONFIG_RT2880_ROOTFS defined!"
#endif
	munmap(ptr, len);
	close(ifd);

	return 1;
}

int mtd_write_firmware(char *filename, int offset, int len)
{
	char cmd[512];
	int status;
#if defined (CONFIG_RT2880_FLASH_8M) || defined (CONFIG_RT2880_FLASH_16M)
	// workaround: erase 8k sector by myself instead of mtd_erase
	// this is for bottom 8M NOR flash only
	snprintf(cmd, sizeof(cmd), "/bin/flash -f 0x400000 -l 0x40ffff");
	system(cmd);
#endif

#if defined (CONFIG_RT2880_ROOTFS_IN_RAM)
	snprintf(cmd, sizeof(cmd), "/bin/mtd_write -o %d -l %d write %s Kernel", offset, len, filename);
	status = system(cmd);
#elif defined (CONFIG_RT2880_ROOTFS_IN_FLASH)
#ifdef CONFIG_ROOTFS_IN_FLASH_NO_PADDING
	snprintf(cmd, sizeof(cmd), "/bin/mtd_write -o %d -l %d write %s Kernel_RootFS", offset, len, filename);
	status = system(cmd);
#else
	snprintf(cmd, sizeof(cmd), "/bin/mtd_write -o %d -l %d write %s Kernel", offset,  CONFIG_MTD_KERNEL_PART_SIZ, filename);
	status = system(cmd);
	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
	{
		return -1;
	}
	if(len > CONFIG_MTD_KERNEL_PART_SIZ )
	{
		snprintf(cmd, sizeof(cmd), "/bin/mtd_write -o %d -l %d write %s RootFS", offset + CONFIG_MTD_KERNEL_PART_SIZ, len - CONFIG_MTD_KERNEL_PART_SIZ, filename);
		status = system(cmd);
	}
#endif
#else
#error "fwflash: no CONFIG_RT2880_ROOTFS defined!"
#endif
	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
	{
		return -1;
	}
	return 0;
}


int main(int argc, char *argv[])
{
	struct stat stat_buf;
	char err_msg[256];

	if(argc < 2)
	{
		print_usage(argv[0]);
		exit(-1);
	}

	if(stat(argv[1], &stat_buf) == -1)
	{
		perror("stat");
		exit(-1);
	}

	// examination
	if(!check(argv[1], 0, stat_buf.st_size, err_msg))
	{
		fprintf(stderr, "Not a valid firmware: %s", err_msg);
		exit(-1);
	}

	// flash write
	if(mtd_write_firmware(argv[1], 0, stat_buf.st_size) == -1)
	{
		fprintf(stderr, "mtd_write fatal error! The corrupted image has ruined the flash!!");
		exit(-1);
	}

	return 0;
}
