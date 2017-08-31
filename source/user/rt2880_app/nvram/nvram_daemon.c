#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include <linux/autoconf.h>

#include "nvram.h"

#if defined (CONFIG_RALINK_GPIO) || defined (CONFIG_RALINK_GPIO_MODULE)
#include "ralink_gpio.h"
#define GPIO_DEV "/dev/gpio"
#endif

#define RESET_DELAY_SEC          10


static char *saved_pidfile;
struct timeval buttonDownTime;

void loadDefault(int chip_id)
{
    switch(chip_id)
    {
    case 2860:
	system("ralink_init clear 2860");
	system("ralink_init renew 2860 /etc/default/FACTORY.dat");
	break;
    default:
	printf("%s:Wrong chip id\n",__FUNCTION__);
	break;
    }
}

/*
 * gpio interrupt handler -
 *   SIGUSR1 - short press
 *   SIGUSR2 - long press
 */
static void nvramIrqHandler(int signum)
{
	struct timeval buttonUpTime;
	if (signum == SIGUSR1) 
	{
		gettimeofday(&buttonDownTime, NULL);
	}
	else if (signum == SIGUSR2) 
	{
		gettimeofday(&buttonUpTime, NULL);
		if (buttonUpTime.tv_sec - buttonDownTime.tv_sec < RESET_DELAY_SEC)
		{
			return;
		}
		printf("load default and reboot..\n");
		loadDefault(2860);
		system("reboot");
	}
}

/*
 * init gpio interrupt -
 *   1. config gpio interrupt mode
 *   2. register my pid and request gpio pin 0
 *   3. issue a handler to handle SIGUSR1 and SIGUSR2
 */
int initGpio(void)
{
#if !defined (CONFIG_RALINK_GPIO) && !defined (CONFIG_RALINK_GPIO_MODULE)
	signal(SIGUSR1, nvramIrqHandler);
	signal(SIGUSR2, nvramIrqHandler);
	return 0;
#else
	int fd;
	ralink_gpio_reg_info info;

	info.pid = getpid();
	info.irq = 0;

	fd = open(GPIO_DEV, O_RDONLY);
	if (fd < 0) {
		perror(GPIO_DEV);
		return -1;
	}
	//set gpio direction to input
	if (ioctl(fd, RALINK_GPIO_SET_DIR_IN, (1<<info.irq)) < 0)
		goto ioctl_err;

	//enable gpio interrupt
	if (ioctl(fd, RALINK_GPIO_ENABLE_INTP) < 0)
		goto ioctl_err;

	//register my information
	if (ioctl(fd, RALINK_GPIO_REG_IRQ, &info) < 0)
		goto ioctl_err;
	close(fd);

	//load default time
	gettimeofday(&buttonDownTime, NULL);

	//issue a handler to handle SIGUSR1 and SIGUSR2
	signal(SIGUSR1, nvramIrqHandler);
	signal(SIGUSR2, nvramIrqHandler);
	return 0;

ioctl_err:
	perror("ioctl");
	close(fd);
	return -1;
#endif
}

static void pidfile_delete(void)
{
	if (saved_pidfile) unlink(saved_pidfile);
}

int pidfile_acquire(const char *pidfile)
{
	int pid_fd;
	if (!pidfile) return -1;

	pid_fd = open(pidfile, O_CREAT | O_WRONLY, 0644);
	if (pid_fd < 0) {
		printf("Unable to open pidfile %s: %m\n", pidfile);
	} else {
		lockf(pid_fd, F_LOCK, 0);
		if (!saved_pidfile)
			atexit(pidfile_delete);
		saved_pidfile = (char *) pidfile;
	}
	return pid_fd;
}

void pidfile_write_release(int pid_fd)
{
	FILE *out;

	if (pid_fd < 0) return;

	if ((out = fdopen(pid_fd, "w")) != NULL) {
		fprintf(out, "%d\n", getpid());
		fclose(out);
	}
	lockf(pid_fd, F_UNLCK, 0);
	close(pid_fd);
}

int main(int argc,char **argv)
{
	pid_t pid;
	int fd;

	if (strcmp(nvram_bufget(RT2860_NVRAM, "WebInit"),"1")) {
		loadDefault(2860);
	}
	
	nvram_close(RT2860_NVRAM);

	if (initGpio() != 0)
		exit(EXIT_FAILURE);

	fd = pidfile_acquire("/var/run/nvramd.pid");
	pidfile_write_release(fd);

	while (1) {
		pause();
	}

	exit(EXIT_SUCCESS);
}

