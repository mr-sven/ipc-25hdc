EXE = watchdog

CFLAGS += -I../include/ -DHAVE_CONFIG_H

all: $(EXE)

watchdog: wd_keepalive.c lock_mem.c daemon-pid.c logmessage.c keep_alive.c configfile.c read-conf.c xmalloc.c sigterm.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

romfs:
	$(ROMFSINST) /sbin/watchdog

clean:
	rm -f *.o $(EXE)
