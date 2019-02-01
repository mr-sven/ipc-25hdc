sbin_PROGRAMS = watchdog wd_keepalive wd_identify

watchdog_SOURCES = file_stat.c file_table.c fstab.c iface.c ifdown.c keep_alive.c \
			load.c lomount.c memory.c mntent.c mount.c net.c nfsmount.c \
			nfsmount_clnt.c nfsmount_xdr.c pidfile.c shutdown.c sundries.c \
			temp.c test_binary.c umount.c version.c watchdog.c \
			logmessage.c xmalloc.c heartbeat.c lock_mem.c daemon-pid.c configfile.c \
			errorcodes.c read-conf.c sigterm.c

wd_keepalive_SOURCES = wd_keepalive.c logmessage.c lock_mem.c daemon-pid.c xmalloc.c \
			configfile.c keep_alive.c read-conf.c sigterm.c

wd_identify_SOURCES = wd_identify.c logmessage.c xmalloc.c configfile.c read-conf.c

AM_CPPFLAGS = -I@top_srcdir@/include

distclean-depend:
	rm -rf .deps
