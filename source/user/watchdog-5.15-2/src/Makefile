EXE = watchdog

CFLAGS += -I../include/ -DHAVE_CONFIG_H

SOURCES = wd_keepalive.c logmessage.c lock_mem.c daemon-pid.c xmalloc.c \
	configfile.c keep_alive.c read-conf.c sigterm.c pidfile.c

all: $(EXE)

watchdog: $(SOURCES)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

romfs:
	$(ROMFSINST) /sbin/watchdog

clean:
	rm -f *.o $(EXE)
