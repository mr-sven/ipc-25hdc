NVDIR = $(ROOTDIR)/lib/libnvram
CFLAGS += -I$(NVDIR) -I$(ROOTDIR)/$(LINUXDIR)/drivers/char
LDFLAGS += -lnvram
LIBNVRAM = $(ROOTDIR)/lib/libnvram/libnvram-0.9.28.so
CONF_H = $(ROOTDIR)/$(LINUXDIR)/include/linux/autoconf.h
UCONF_H = $(ROOTDIR)/config/autoconf.h
EXEC = factory_mac_update

all: $(EXEC)

factory_mac_update: factory_mac_update.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

factory_mac_update.o: $(CONF_H) $(UCONF_H) $(LIBNVRAM)

romfs:
	$(ROMFSINST) /bin/$(EXEC)

clean:
	@rm -rf $(EXEC) *.o

