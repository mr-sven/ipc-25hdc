/*
 * Copyright (c) 1993, 1994, 1995 Rick Sladkey <jrs@world.std.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *	$Id: proc.c,v 1.1.1.1 2007-03-22 09:04:29 yy Exp $
 */

#include "defs.h"

#include <asm/ioctl.h>

// missing in ioclt.h
#define IOC_VOID	(_IOC_NONE << _IOC_DIRSHIFT)
#define IOCDIR_MASK	(_IOC_DIRMASK << _IOC_DIRSHIFT)

struct uvc_xu_control {
    unsigned char unit;
    unsigned char selector;
    unsigned short size;
    unsigned char *data;
};

int
usb_ioctl(tcp, code, arg)
struct tcb *tcp;
long code, arg;
{
	unsigned short size;
	char type;
	unsigned char number;
	unsigned char *data = NULL;
	unsigned short cnt;

	if (exiting(tcp)) {
		return 1;
	}

	size = (unsigned short)_IOC_SIZE(code);
	type = (char)_IOC_TYPE(code);
	number = (unsigned char)_IOC_NR(code);

	tprintf(" {");
	switch (code & IOCDIR_MASK) {
	case IOC_VOID:  tprintf("_IO('%c', %d, %d)", type, number, size); break;
	case IOC_IN:    tprintf("_IOW('%c', %d, %d)", type, number, size); break;
	case IOC_OUT:   tprintf("_IOR('%c', %d, %d)", type, number, size); break;
	case IOC_INOUT: tprintf("_IOWR('%c', %d, %d)", type, number, size); break;
	default: break;
	}
	tprintf("}, 0x%lx", arg);
	if ((number == 3 || number == 4) && size == 8) {
		struct uvc_xu_control xuctrl;

		if (umoven(tcp, arg, size, (char *)&xuctrl) == -1) {
			return 1;
		}
		
		tprintf(" {uvc_xu_control(unit = %d, selector = %d, size = %d, data =", xuctrl.unit, xuctrl.selector, xuctrl.size);

		data = malloc(xuctrl.size);
		if (umoven(tcp, xuctrl.data, xuctrl.size, (char *) data) == -1) {
			return 1;
		}

		for (cnt = 0; cnt < xuctrl.size; cnt++) {
			tprintf(" 0x%02x", data[cnt]);
		}
		
		tprintf("}");
	}
	else {
		data = malloc(size);
		if (umoven(tcp, arg, size, (char *) data) == -1) {
			return 1;
		}

		tprintf("{");
		for (cnt = 0; cnt < size; cnt++) {
			tprintf("0x%02x ", data[cnt]);
		}
		tprintf("}");
	}
	return 1;
}
