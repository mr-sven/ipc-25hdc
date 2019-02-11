EXEC = fwflash
OBJS = fwflash.o crc32.o

CONF	= $(ROOTDIR)/$(LINUXDIR)
CONF_H	= $(ROOTDIR)/$(LINUXDIR)/include/linux/autoconf.h

CFLAGS += -O0 -I$(CONF) -DUSE_HOSTCC

.PHONY: libs all clean romfs

all: $(OBJS)
	$(CC) -o $(EXEC) $(OBJS) $(LDFLAGS)

.o: $(CONF_H)
	$(CC) $(CFLAGS) -c $<

romfs:
	$(ROMFSINST) /bin/$(EXEC)

clean:
	@rm -rf $(EXEC) *.o

