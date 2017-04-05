/*
 ***************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology 5th Rd.
 * Science-based Industrial Park
 * Hsin-chu, Taiwan, R.O.C.
 *
 * (c) Copyright, Ralink Technology, Inc.
 *
 * All rights reserved. Ralink's source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of Ralink Tech. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************
 *
 * $Id: gpio.c,v 1.18.2.1 2012-03-06 13:01:51 kurtis Exp $
 * 2.0 2017-04-05 09:30:00 Sven Fabricius - striped down to fit for CAM
 */

#include <stdio.h>             
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <linux/autoconf.h>
#include "ralink_gpio.h"

#define GPIO_DEV	"/dev/gpio"

#define GPIO_IR		11
#define GPIO_CAM	12
#define GPIO_LED	13

#define GPIO2100_MIN	0
#define GPIO2100_MAX	21
#define GPIO2722_MIN	22
#define GPIO2722_MAX	27

enum {
	gpio_in,
	gpio_out,
};
enum {
	gpio2100,
	gpio2722,
};

void gpio_set(int gpio_num, int mode)
{
	int fd, req, r;
	unsigned int pin = 0;

	// calc gpio reg and pin
	if (gpio_num <= GPIO2100_MAX) {
		r = gpio2100;
		pin = 1 << gpio_num;
	} else if (gpio_num <= GPIO2722_MAX) {
		r = gpio2722;
		pin = 1 << (gpio_num - GPIO2100_MIN);
	} else {
		perror("gpio_num");
		return;
	}

	// open gpio dev
	fd = open(GPIO_DEV, O_RDONLY);
	if (fd < 0) {
		perror(GPIO_DEV);
		return;
	}

	// set dir cmd
	if (r == gpio2722) {
		req = RALINK_GPIO2722_SET_DIR_OUT;
	} else {
		req = RALINK_GPIO_SET_DIR_OUT;
	}

	// set dir on pin
	if (ioctl(fd, req, pin) < 0) {
		perror("ioctl");
		close(fd);
		return -1;
	}

	if (mode == 0) {
		// set clear cmd
		if (r == gpio2722) {
			req = RALINK_GPIO2722_CLEAR;
		} else {
			req = RALINK_GPIO_CLEAR;
		}
	} else {
		// set set cmd
		if (r == gpio2722) {
			req = RALINK_GPIO2722_SET;
		} else {
			req = RALINK_GPIO_SET;
		}
	}

	// set out on pin
	if (ioctl(fd, req, pin) < 0) {
		perror("ioctl");
		close(fd);
		return -1;
	}

	close(fd);
	return 0;

}

void usage(char *cmd)
{
	printf("Usage: %s c <0|1> - disable or enable CAM\n", cmd);
	printf("       %s l <0|1> - disable or enable Wire-LED\n", cmd);
	printf("       %s i <0|1> - disable or enable IR LEDs\n", cmd);
	exit(0);
}

int main(int argc, char *argv[])
{
	if (argc < 3)
		usage(argv[0]);

	switch (argv[1][0]) {
	case 'c':
		gpio_set(GPIO_CAM, atoi(argv[2]));
		break;
	case 'l':
		gpio_set(GPIO_LED, atoi(argv[2]));
		break;
	case 'i':
		gpio_set(GPIO_IR, atoi(argv[2]));
		break;
	default:
		usage(argv[0]);
	}

	return 0;
}

