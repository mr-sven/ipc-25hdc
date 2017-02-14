EXE = watchdog wd_keepalive
OBJS = file_stat.o file_table.o fstab.o iface.o ifdown.o keep_alive.o load.o \
       lomount.o memory.o mntent.o mount.o net.o nfsmount.o nfsmount_clnt.o \
       nfsmount_xdr.o pidfile.o shutdown.o sundries.o temp.o test_binary.o \
       umount.o version.o watchdog.o

CFLAGS += -I../include/ -DHAVE_CONFIG_H

all: $(EXE)

watchdog: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS) $(LDLIBS)

wd_keepalive: wd_keepalive.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

romfs:
	$(ROMFSINST) /sbin/watchdog
	$(ROMFSINST) /sbin/wd_keepalive

clean:
	rm -f *.o $(EXE)

